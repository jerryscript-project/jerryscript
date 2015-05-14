/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

#include "ecma-alloc.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lcache.h"
#include "jrt.h"
#include "jrt-libc-includes.h"
#include "serializer.h"
#include "vm.h"

/**
 * Maximum length of strings' concatenation
 */
#define ECMA_STRING_MAX_CONCATENATION_LENGTH (CONFIG_ECMA_STRING_MAX_CONCATENATION_LENGTH)

/**
 * Limit for magic string length
 */
#define ECMA_STRING_MAGIC_STRING_LENGTH_LIMIT 32

/**
 * The length should be representable with int32_t.
 */
JERRY_STATIC_ASSERT ((uint32_t) ((int32_t) ECMA_STRING_MAX_CONCATENATION_LENGTH) ==
                     ECMA_STRING_MAX_CONCATENATION_LENGTH);

/**
 * Lengths of magic strings
 */
static ecma_length_t ecma_magic_string_lengths[ECMA_MAGIC_STRING__COUNT];

/**
 * External magic strings data array, count and lengths
 */
static const ecma_char_ptr_t* ecma_magic_string_ex_array = NULL;
static uint32_t ecma_magic_string_ex_count = 0;
static const ecma_length_t* ecma_magic_string_ex_lengths = NULL;

#ifndef JERRY_NDEBUG
/**
 * Maximum length among lengths of magic strings
 */
static ecma_length_t ecma_magic_string_max_length;
#endif /* !JERRY_NDEBUG */

static void
ecma_init_ecma_string_from_lit_cp (ecma_string_t *string_p,
                                   lit_cpointer_t lit_index,
                                   bool is_stack_var);
static void
ecma_init_ecma_string_from_magic_string_id (ecma_string_t *string_p,
                                            ecma_magic_string_id_t magic_string_id,
                                            bool is_stack_var);

static void
ecma_init_ecma_string_from_magic_string_ex_id (ecma_string_t *string_p,
                                               ecma_magic_string_ex_id_t magic_string_ex_id,
                                               bool is_stack_var);
/**
 * Allocate a collection of ecma-chars.
 *
 * @return pointer to the collection's header
 */
static ecma_collection_header_t*
ecma_new_chars_collection (const ecma_char_t chars_buffer[], /**< ecma-chars */
                           ecma_length_t chars_number) /**< number of ecma-chars */
{
  JERRY_ASSERT (chars_buffer != NULL);
  JERRY_ASSERT (chars_number > 0);

  ecma_collection_header_t* collection_p = ecma_alloc_collection_header ();

  collection_p->unit_number = chars_number;

  mem_cpointer_t* next_chunk_cp_p = &collection_p->next_chunk_cp;
  ecma_char_t* cur_char_buf_iter_p = (ecma_char_t*) collection_p->data;
  ecma_char_t* cur_char_buf_end_p = cur_char_buf_iter_p + sizeof (collection_p->data) / sizeof (ecma_char_t);

  for (ecma_length_t char_index = 0;
       char_index < chars_number;
       char_index++)
  {
    if (unlikely (cur_char_buf_iter_p == cur_char_buf_end_p))
    {
      ecma_collection_chunk_t *chunk_p = ecma_alloc_collection_chunk ();
      ECMA_SET_NON_NULL_POINTER (*next_chunk_cp_p, chunk_p);
      next_chunk_cp_p = &chunk_p->next_chunk_cp;

      cur_char_buf_iter_p = (ecma_char_t*) chunk_p->data;
      cur_char_buf_end_p = cur_char_buf_iter_p + sizeof (chunk_p->data) / sizeof (ecma_char_t);
    }

    JERRY_ASSERT (cur_char_buf_iter_p + 1 <= cur_char_buf_end_p);

    *cur_char_buf_iter_p++ = chars_buffer[char_index];
  }

  *next_chunk_cp_p = ECMA_NULL_POINTER;

  return collection_p;
} /* ecma_new_chars_collection */

/**
 * Compare two collection of ecma-chars.
 *
 * @return true - if collections are equal,
 *         false - otherwise.
 */
static bool
ecma_compare_chars_collection (const ecma_collection_header_t* header1_p, /**< first collection's header */
                               const ecma_collection_header_t* header2_p) /**< second collection's header */
{
  JERRY_ASSERT (header1_p != NULL && header2_p != NULL);

  if (header1_p->unit_number != header2_p->unit_number)
  {
    return false;
  }

  const ecma_length_t chars_number = header1_p->unit_number;

  const ecma_char_t* cur_char_buf1_iter_p = (ecma_char_t*) header1_p->data;
  const ecma_char_t* cur_char_buf1_end_p = cur_char_buf1_iter_p + sizeof (header1_p->data) / sizeof (ecma_char_t);
  const ecma_char_t* cur_char_buf2_iter_p = (ecma_char_t*) header2_p->data;
  const ecma_char_t* cur_char_buf2_end_p = cur_char_buf2_iter_p + sizeof (header2_p->data) / sizeof (ecma_char_t);

  mem_cpointer_t next_chunk1_cp = header1_p->next_chunk_cp;
  mem_cpointer_t next_chunk2_cp = header2_p->next_chunk_cp;

  for (ecma_length_t char_index = 0;
       char_index < chars_number;
       char_index++)
  {
    if (unlikely (cur_char_buf1_iter_p == cur_char_buf1_end_p))
    {
      JERRY_ASSERT (cur_char_buf2_iter_p == cur_char_buf2_end_p);

      const ecma_collection_chunk_t *chunk1_p = ECMA_GET_NON_NULL_POINTER (ecma_collection_chunk_t, next_chunk1_cp);
      const ecma_collection_chunk_t *chunk2_p = ECMA_GET_NON_NULL_POINTER (ecma_collection_chunk_t, next_chunk2_cp);

      cur_char_buf1_iter_p = (ecma_char_t*) chunk1_p->data;
      cur_char_buf1_end_p = cur_char_buf1_iter_p + sizeof (chunk1_p->data) / sizeof (ecma_char_t);
      cur_char_buf2_iter_p = (ecma_char_t*) chunk2_p->data;
      cur_char_buf2_end_p = cur_char_buf2_iter_p + sizeof (chunk2_p->data) / sizeof (ecma_char_t);

      next_chunk1_cp = chunk1_p->next_chunk_cp;
      next_chunk2_cp = chunk2_p->next_chunk_cp;
    }

    JERRY_ASSERT (cur_char_buf1_iter_p + 1 <= cur_char_buf1_end_p);
    JERRY_ASSERT (cur_char_buf2_iter_p + 1 <= cur_char_buf2_end_p);

    if (*cur_char_buf1_iter_p++ != *cur_char_buf2_iter_p++)
    {
      return false;
    }
  }

  return true;
} /* ecma_compare_chars_collection */

/**
 * Copy the collection of ecma-chars.
 *
 * @return pointer to collection copy
 */
static ecma_collection_header_t*
ecma_copy_chars_collection (const ecma_collection_header_t* collection_p) /**< collection's header */
{
  JERRY_ASSERT (collection_p != NULL);

  ecma_collection_header_t *new_header_p = ecma_alloc_collection_header ();
  *new_header_p = *collection_p;

  mem_cpointer_t* next_chunk_cp_p = &new_header_p->next_chunk_cp;

  ecma_collection_chunk_t *chunk_p = ECMA_GET_POINTER (ecma_collection_chunk_t,
                                                       collection_p->next_chunk_cp);

  while (chunk_p != NULL)
  {
    ecma_collection_chunk_t *new_chunk_p = ecma_alloc_collection_chunk ();
    *new_chunk_p = *chunk_p;

    ECMA_SET_NON_NULL_POINTER (*next_chunk_cp_p, new_chunk_p);
    next_chunk_cp_p = &new_chunk_p->next_chunk_cp;

    chunk_p = ECMA_GET_POINTER (ecma_collection_chunk_t,
                                chunk_p->next_chunk_cp);
  }

  *next_chunk_cp_p = ECMA_NULL_POINTER;

  return new_header_p;
} /* ecma_copy_chars_collection */

/**
 * Copy characters of the collection to buffer
 */
static void
ecma_copy_chars_collection_to_buffer (const ecma_collection_header_t *collection_p, /**< collection header */
                                      ecma_char_t chars_buffer[], /**< buffer for characters */
                                      size_t buffer_size) /**< size of the buffer */
{
  JERRY_ASSERT (collection_p != NULL);

  ecma_char_t *out_chars_buf_iter_p = chars_buffer;

  const ecma_length_t chars_number = collection_p->unit_number;

  mem_cpointer_t next_chunk_cp = collection_p->next_chunk_cp;
  const ecma_char_t* cur_char_buf_iter_p = (ecma_char_t*) collection_p->data;
  const ecma_char_t* cur_char_buf_end_p = cur_char_buf_iter_p + sizeof (collection_p->data) / sizeof (ecma_char_t);

  for (ecma_length_t char_index = 0;
       char_index < chars_number;
       char_index++)
  {
    if (unlikely (cur_char_buf_iter_p == cur_char_buf_end_p))
    {
      const ecma_collection_chunk_t *chunk_p = ECMA_GET_NON_NULL_POINTER (ecma_collection_chunk_t, next_chunk_cp);

      cur_char_buf_iter_p = (ecma_char_t*) chunk_p->data;
      cur_char_buf_end_p = cur_char_buf_iter_p + sizeof (chunk_p->data) / sizeof (ecma_char_t);

      next_chunk_cp = chunk_p->next_chunk_cp;
    }

    JERRY_ASSERT (cur_char_buf_iter_p + 1 <= cur_char_buf_end_p);

    *out_chars_buf_iter_p++ = *cur_char_buf_iter_p++;
  }

  *out_chars_buf_iter_p = ECMA_CHAR_NULL;

  JERRY_ASSERT (out_chars_buf_iter_p - chars_buffer <= (ssize_t) buffer_size);
} /* ecma_copy_chars_collection_to_buffer */

/**
 * Free the collection of ecma-chars.
 */
static void
ecma_free_chars_collection (ecma_collection_header_t* collection_p) /**< collection's header */
{
  JERRY_ASSERT (collection_p != NULL);

  ecma_collection_chunk_t *chunk_p = ECMA_GET_POINTER (ecma_collection_chunk_t,
                                                       collection_p->next_chunk_cp);

  while (chunk_p != NULL)
  {
    ecma_collection_chunk_t *next_chunk_p = ECMA_GET_POINTER (ecma_collection_chunk_t,
                                                              chunk_p->next_chunk_cp);
    ecma_dealloc_collection_chunk (chunk_p);

    chunk_p = next_chunk_p;
  }

  ecma_dealloc_collection_header (collection_p);
} /* ecma_free_chars_collection */

/**
 * Initialize data for string helpers
 */
void
ecma_strings_init (void)
{
  /* Initializing magic strings information */

#ifndef JERRY_NDEBUG
  ecma_magic_string_max_length = 0;
#endif /* !JERRY_NDEBUG */

  for (ecma_magic_string_id_t id = (ecma_magic_string_id_t) 0;
       id < ECMA_MAGIC_STRING__COUNT;
       id = (ecma_magic_string_id_t) (id + 1))
  {
    ecma_magic_string_lengths[id] = ecma_zt_string_length (ecma_get_magic_string_zt (id));

#ifndef JERRY_NDEBUG
    ecma_magic_string_max_length = JERRY_MAX (ecma_magic_string_max_length, ecma_magic_string_lengths[id]);

    JERRY_ASSERT (ecma_magic_string_max_length <= ECMA_STRING_MAGIC_STRING_LENGTH_LIMIT);
#endif /* !JERRY_NDEBUG */
  }
} /* ecma_strings_init */

/**
 * Initialize external magic strings
 */
void
ecma_strings_ex_init (void)
{
  ecma_magic_string_ex_array = NULL;
  ecma_magic_string_ex_count = 0;
  ecma_magic_string_ex_lengths = NULL;
} /* ecma_strings_ex_init */

/**
 * Register external magic strings
 */
void
ecma_strings_ex_set (const ecma_char_ptr_t* ex_str_items, /**< character arrays, representing
                                                           *   external magic strings' contents */
                     uint32_t count,                      /**< number of the strings */
                     const ecma_length_t* ex_str_lengths) /**< lengths of the strings */
{
  JERRY_ASSERT (ex_str_items != NULL);
  JERRY_ASSERT (count > 0);
  JERRY_ASSERT (ex_str_lengths != NULL);

  JERRY_ASSERT (ecma_magic_string_ex_array == NULL);
  JERRY_ASSERT (ecma_magic_string_ex_count == 0);
  JERRY_ASSERT (ecma_magic_string_ex_lengths == NULL);

  /* Set external magic strings information */
  ecma_magic_string_ex_array = ex_str_items;
  ecma_magic_string_ex_count = count;
  ecma_magic_string_ex_lengths = ex_str_lengths;

#ifndef JERRY_NDEBUG
  for (ecma_magic_string_ex_id_t id = (ecma_magic_string_ex_id_t) 0;
       id < ecma_magic_string_ex_count;
       id = (ecma_magic_string_ex_id_t) (id + 1))
  {
    JERRY_ASSERT (ecma_magic_string_ex_lengths[id] == ecma_zt_string_length (ecma_get_magic_string_ex_zt (id)));

    ecma_magic_string_max_length = JERRY_MAX (ecma_magic_string_max_length, ecma_magic_string_ex_lengths[id]);

    JERRY_ASSERT (ecma_magic_string_max_length <= ECMA_STRING_MAGIC_STRING_LENGTH_LIMIT);
  }
#endif /* !JERRY_NDEBUG */
} /* ecma_strings_ex_init */

/**
 * Get number of external magic strings
 *
 * @return number of the strings, if there were registered,
 *         zero - otherwise.
 */
uint32_t
ecma_get_magic_string_ex_count (void)
{
  return ecma_magic_string_ex_count;
} /* ecma_get_magic_string_ex_count */

/**
 * Initialize ecma-string descriptor with string described by index in literal table
 */
static void
ecma_init_ecma_string_from_lit_cp (ecma_string_t *string_p, /**< descriptor to initialize */
                                   lit_cpointer_t lit_cp, /**< compressed pointer to literal */
                                   bool is_stack_var) /**< flag indicating whether the string descriptor
                                                              is placed on stack (true) or in the heap (false) */
{
#ifndef JERRY_NDEBUG
  JERRY_ASSERT (is_stack_var == (!mem_is_heap_pointer (string_p)));
#endif /* !JERRY_NDEBUG */

  literal_t lit = lit_get_literal_by_cp (lit_cp);
  if (lit->get_type () == LIT_MAGIC_STR_T)
  {
    ecma_init_ecma_string_from_magic_string_id (string_p,
                                                lit_magic_record_get_magic_str_id (lit),
                                                is_stack_var);

    return;
  }
  else if (lit->get_type () == LIT_MAGIC_STR_EX_T)
  {
    ecma_init_ecma_string_from_magic_string_ex_id (string_p,
                                                   lit_magic_record_ex_get_magic_str_id (lit),
                                                   is_stack_var);
    return;
  }

  JERRY_ASSERT (lit->get_type () == LIT_STR_T);

  string_p->refs = 1;
  string_p->is_stack_var = (is_stack_var != 0);
  string_p->container = ECMA_STRING_CONTAINER_LIT_TABLE;
  string_p->hash = lit_charset_literal_get_hash (lit);

  string_p->u.common_field = 0;
  string_p->u.lit_cp = lit_cp;
} /* ecma_init_ecma_string_from_lit_cp */

/**
 * Initialize ecma-string descriptor with specified magic string
 */
static void
ecma_init_ecma_string_from_magic_string_id (ecma_string_t *string_p, /**< descriptor to initialize */
                                            ecma_magic_string_id_t magic_string_id, /**< identifier of
                                                                                         the magic string */
                                            bool is_stack_var) /**< flag indicating whether the string descriptor
                                                                    is placed on stack (true) or in the heap (false) */
{
#ifndef JERRY_NDEBUG
  JERRY_ASSERT (is_stack_var == (!mem_is_heap_pointer (string_p)));
#endif /* !JERRY_NDEBUG */

  string_p->refs = 1;
  string_p->is_stack_var = (is_stack_var != 0);
  string_p->container = ECMA_STRING_CONTAINER_MAGIC_STRING;
  string_p->hash = ecma_chars_buffer_calc_hash_last_chars (ecma_get_magic_string_zt (magic_string_id),
                                                           ecma_magic_string_lengths[magic_string_id]);

  string_p->u.common_field = 0;
  string_p->u.magic_string_id = magic_string_id;
} /* ecma_init_ecma_string_from_magic_string_id */

/**
 * Initialize external ecma-string descriptor with specified magic string
 */
static void
ecma_init_ecma_string_from_magic_string_ex_id (ecma_string_t *string_p, /**< descriptor to initialize */
                                               ecma_magic_string_ex_id_t magic_string_ex_id, /**< identifier of
                                                                                           the external magic string */
                                               bool is_stack_var) /**< flag indicating whether the string descriptor
                                                                    is placed on stack (true) or in the heap (false) */
{
#ifndef JERRY_NDEBUG
  JERRY_ASSERT (is_stack_var == (!mem_is_heap_pointer (string_p)));
#endif /* !JERRY_NDEBUG */

  string_p->refs = 1;
  string_p->is_stack_var = (is_stack_var != 0);
  string_p->container = ECMA_STRING_CONTAINER_MAGIC_STRING_EX;
  string_p->hash = ecma_chars_buffer_calc_hash_last_chars (ecma_get_magic_string_ex_zt (magic_string_ex_id),
                                                           ecma_magic_string_ex_lengths[magic_string_ex_id]);

  string_p->u.common_field = 0;
  string_p->u.magic_string_ex_id = magic_string_ex_id;
} /* ecma_init_ecma_string_from_magic_string_ex_id */

/**
 * Allocate new ecma-string and fill it with specified number of characters from specified buffer
 *
 * @return pointer to ecma-string descriptor
 */
ecma_string_t*
ecma_new_ecma_string (const ecma_char_t *string_p, /**< input string */
                      const ecma_length_t length) /**< number of characters */
{
  JERRY_ASSERT (string_p != NULL);
  JERRY_ASSERT (length > 0);

  ecma_string_t* string_desc_p = ecma_alloc_string ();
  string_desc_p->refs = 1;
  string_desc_p->is_stack_var = false;
  string_desc_p->container = ECMA_STRING_CONTAINER_HEAP_CHUNKS;
  string_desc_p->hash = ecma_chars_buffer_calc_hash_last_chars (string_p, length);

  string_desc_p->u.common_field = 0;
  ecma_collection_header_t *collection_p = ecma_new_chars_collection (string_p, length);
  ECMA_SET_NON_NULL_POINTER (string_desc_p->u.collection_cp, collection_p);

  return string_desc_p;
} /* ecma_new_ecma_string */

/**
 * Allocate new ecma-string and fill it with characters from specified buffer
 *
 * @return pointer to ecma-string descriptor
 */
ecma_string_t*
ecma_new_ecma_string (const ecma_char_t *string_p) /**< zero-terminated string */
{
  JERRY_ASSERT (string_p != NULL);

  ecma_magic_string_id_t magic_string_id;
  if (ecma_is_zt_string_magic (string_p, &magic_string_id))
  {
    return ecma_get_magic_string (magic_string_id);
  }

  ecma_magic_string_ex_id_t magic_string_ex_id;
  if (ecma_is_zt_ex_string_magic (string_p, &magic_string_ex_id))
  {
    return ecma_get_magic_string_ex (magic_string_ex_id);
  }

  ecma_length_t length = 0;
  const ecma_char_t *iter_p = string_p;

  while (*iter_p++)
  {
    length++;
  }

  return ecma_new_ecma_string (string_p, length);
} /* ecma_new_ecma_string */

/**
 * Allocate new ecma-string and fill it with ecma-number
 *
 * @return pointer to ecma-string descriptor
 */
ecma_string_t*
ecma_new_ecma_string_from_uint32 (uint32_t uint32_number) /**< UInt32-represented ecma-number */
{
  ecma_string_t* string_desc_p = ecma_alloc_string ();
  string_desc_p->refs = 1;
  string_desc_p->is_stack_var = false;
  string_desc_p->container = ECMA_STRING_CONTAINER_UINT32_IN_DESC;

  uint32_t last_two_digits = uint32_number % 100;
  uint32_t digit_pl = last_two_digits / 10;
  uint32_t digit_l = last_two_digits % 10;

  FIXME (/* Use digit to char conversion routine */);
  const ecma_char_t digits[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
  const bool is_one_char_or_more = (uint32_number >= 10);
  const ecma_char_t last_chars[ECMA_STRING_HASH_LAST_CHARS_COUNT] =
  {
    is_one_char_or_more ? digits[digit_pl] : digits[digit_l],
    is_one_char_or_more ? digits[digit_l] : ECMA_CHAR_NULL
  };

  /* Only last two chars are really used for hash calculation */
  string_desc_p->hash = ecma_chars_buffer_calc_hash_last_chars (last_chars,
                                                                is_one_char_or_more ? 2 : 1);

#ifndef JERRY_NDEBUG
  ecma_char_t char_buf[ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32];
  ssize_t chars_copied = ecma_uint32_to_string (uint32_number,
                                                char_buf,
                                                ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32);
  JERRY_ASSERT ((ecma_length_t) chars_copied == chars_copied);

  JERRY_ASSERT (string_desc_p->hash == ecma_chars_buffer_calc_hash_last_chars (char_buf,
                                                                               ecma_zt_string_length (char_buf)));
#endif /* !JERRY_NDEBUG */

  string_desc_p->u.common_field = 0;
  string_desc_p->u.uint32_number = uint32_number;

  return string_desc_p;
} /* ecma_new_ecma_string_from_uint32 */

/**
 * Allocate new ecma-string and fill it with ecma-number
 *
 * @return pointer to ecma-string descriptor
 */
ecma_string_t*
ecma_new_ecma_string_from_number (ecma_number_t num) /**< ecma-number */
{
  uint32_t uint32_num = ecma_number_to_uint32 (num);
  if (num == ecma_uint32_to_number (uint32_num))
  {
    return ecma_new_ecma_string_from_uint32 (uint32_num);
  }

  ecma_char_t str_buf[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER + 1];
  ecma_length_t length = ecma_number_to_zt_string (num,
                                                   str_buf,
                                                   sizeof (str_buf));

  ecma_magic_string_id_t magic_string_id;
  if (ecma_is_zt_string_magic (str_buf, &magic_string_id))
  {
    return ecma_get_magic_string (magic_string_id);
  }

  ecma_string_t* string_desc_p = ecma_alloc_string ();
  string_desc_p->refs = 1;
  string_desc_p->is_stack_var = false;
  string_desc_p->container = ECMA_STRING_CONTAINER_HEAP_NUMBER;
  string_desc_p->hash = ecma_chars_buffer_calc_hash_last_chars (str_buf, length);

  string_desc_p->u.common_field = 0;
  ecma_number_t *num_p = ecma_alloc_number ();
  *num_p = num;
  ECMA_SET_POINTER (string_desc_p->u.number_cp, num_p);

  return string_desc_p;
} /* ecma_new_ecma_string_from_number */

/**
 * Initialize ecma-string descriptor placed on stack
 * with string described by index in literal table
 */
void
ecma_new_ecma_string_on_stack_from_lit_cp (ecma_string_t *string_p, /**< pointer to the ecma-string
                                                                            descriptor to initialize */
                                           lit_cpointer_t lit_cp) /**< compressed pointer to literal */
{
  ecma_init_ecma_string_from_lit_cp (string_p, lit_cp, true);
} /* ecma_new_ecma_string_on_stack_from_lit_cp */

/**
 * Allocate new ecma-string and fill it with reference to string literal
 *
 * @return pointer to ecma-string descriptor
 */
ecma_string_t*
ecma_new_ecma_string_from_lit_cp (lit_cpointer_t lit_cp) /**< index in the literal table */
{
  ecma_string_t* string_desc_p = ecma_alloc_string ();

  ecma_init_ecma_string_from_lit_cp (string_desc_p, lit_cp, false);

  return string_desc_p;
} /* ecma_new_ecma_string_from_lit_cp */

/**
 * Initialize ecma-string descriptor placed on stack with specified magic string
 */
void
ecma_new_ecma_string_on_stack_from_magic_string_id (ecma_string_t *string_p, /**< pointer to the ecma-string
                                                                                  descriptor to initialize */
                                                    ecma_magic_string_id_t id) /**< magic string id */
{
  ecma_init_ecma_string_from_magic_string_id (string_p, id, true);
} /* ecma_new_ecma_string_on_stack_from_magic_string_id */

/**
 * Allocate new ecma-string and fill it with reference to ECMA magic string
 *
 * @return pointer to ecma-string descriptor
 */
ecma_string_t*
ecma_new_ecma_string_from_magic_string_id (ecma_magic_string_id_t id) /**< identifier of magic string */
{
  JERRY_ASSERT (id < ECMA_MAGIC_STRING__COUNT);

  ecma_string_t* string_desc_p = ecma_alloc_string ();
  ecma_init_ecma_string_from_magic_string_id (string_desc_p, id, false);

  return string_desc_p;
} /* ecma_new_ecma_string_from_magic_string_id */

/**
 * Allocate new ecma-string and fill it with reference to ECMA magic string
 *
 * @return pointer to ecma-string descriptor
 */
ecma_string_t*
ecma_new_ecma_string_from_magic_string_ex_id (ecma_magic_string_ex_id_t id) /**< identifier of externl magic string */
{
  JERRY_ASSERT (id < ecma_magic_string_ex_count);

  ecma_string_t* string_desc_p = ecma_alloc_string ();
  ecma_init_ecma_string_from_magic_string_ex_id (string_desc_p, id, false);

  return string_desc_p;
} /* ecma_new_ecma_string_from_magic_string_ex_id */


/**
 * Concatenate ecma-strings
 *
 * @return concatenation of two ecma-strings
 */
ecma_string_t*
ecma_concat_ecma_strings (ecma_string_t *string1_p, /**< first ecma-string */
                          ecma_string_t *string2_p) /**< second ecma-string */
{
  JERRY_ASSERT (string1_p != NULL
                && string2_p != NULL);

  uint32_t str1_len = (uint32_t) ecma_string_get_length (string1_p);
  uint32_t str2_len = (uint32_t) ecma_string_get_length (string2_p);

  if (str1_len == 0)
  {
    return ecma_copy_or_ref_ecma_string (string2_p);
  }
  else if (str2_len == 0)
  {
    return ecma_copy_or_ref_ecma_string (string1_p);
  }

  int64_t length = (int64_t) str1_len + (int64_t) str2_len;

  if (length > ECMA_STRING_MAX_CONCATENATION_LENGTH)
  {
    jerry_fatal (ERR_OUT_OF_MEMORY);
  }

  ecma_string_t* string_desc_p = ecma_alloc_string ();
  string_desc_p->refs = 1;
  string_desc_p->is_stack_var = false;
  string_desc_p->container = ECMA_STRING_CONTAINER_CONCATENATION;

  string_desc_p->u.common_field = 0;

  string1_p = ecma_copy_or_ref_ecma_string (string1_p);
  string2_p = ecma_copy_or_ref_ecma_string (string2_p);

  ECMA_SET_NON_NULL_POINTER (string_desc_p->u.concatenation.string1_cp, string1_p);
  ECMA_SET_NON_NULL_POINTER (string_desc_p->u.concatenation.string2_cp, string2_p);

  if (str2_len >= ECMA_STRING_HASH_LAST_CHARS_COUNT)
  {
    string_desc_p->hash = string2_p->hash;
  }
  else
  {
    JERRY_STATIC_ASSERT (ECMA_STRING_HASH_LAST_CHARS_COUNT == 2);
    JERRY_ASSERT (str2_len == 1);

    ecma_char_t chars_buf[ECMA_STRING_HASH_LAST_CHARS_COUNT] =
    {
      ecma_string_get_char_at_pos (string1_p, str1_len - 1u),
      ecma_string_get_char_at_pos (string2_p, 0)
    };

    string_desc_p->hash = ecma_chars_buffer_calc_hash_last_chars (chars_buf, ECMA_STRING_HASH_LAST_CHARS_COUNT);
  }

  return string_desc_p;
} /* ecma_concat_ecma_strings */

/**
 * Copy ecma-string
 *
 * @return pointer to copy of ecma-string with reference counter set to 1
 */
static ecma_string_t*
ecma_copy_ecma_string (ecma_string_t *string_desc_p) /**< string descriptor */
{
  JERRY_ASSERT (string_desc_p != NULL);
  JERRY_ASSERT (string_desc_p->refs > 0);

  ecma_string_t *new_str_p;

  switch ((ecma_string_container_t) string_desc_p->container)
  {
    case ECMA_STRING_CONTAINER_LIT_TABLE:
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    case ECMA_STRING_CONTAINER_MAGIC_STRING_EX:
    {
      new_str_p = ecma_alloc_string ();

      *new_str_p = *string_desc_p;

      new_str_p->refs = 1;
      new_str_p->is_stack_var = false;

      break;
    }

    case ECMA_STRING_CONTAINER_CONCATENATION:
    {
      ecma_string_t *part1_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t, string_desc_p->u.concatenation.string1_cp);
      ecma_string_t *part2_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t, string_desc_p->u.concatenation.string2_cp);

      new_str_p = ecma_concat_ecma_strings (part1_p, part2_p);

      break;
    }

    case ECMA_STRING_CONTAINER_HEAP_NUMBER:
    {
      ecma_number_t *num_p = ECMA_GET_NON_NULL_POINTER (ecma_number_t, string_desc_p->u.number_cp);

      new_str_p = ecma_new_ecma_string_from_number (*num_p);

      break;
    }

    case ECMA_STRING_CONTAINER_HEAP_CHUNKS:
    {
      new_str_p = ecma_alloc_string ();
      *new_str_p = *string_desc_p;

      const ecma_collection_header_t *chars_collection_p = ECMA_GET_NON_NULL_POINTER (ecma_collection_header_t,
                                                                                      string_desc_p->u.collection_cp);
      JERRY_ASSERT (chars_collection_p != NULL);
      ecma_collection_header_t *new_chars_collection_p = ecma_copy_chars_collection (chars_collection_p);

      ECMA_SET_NON_NULL_POINTER (new_str_p->u.collection_cp, new_chars_collection_p);

      break;
    }

    default:
    {
      JERRY_UNREACHABLE ();
    }
  }

  JERRY_ASSERT (ecma_compare_ecma_strings (string_desc_p, new_str_p));

  return new_str_p;
} /* ecma_copy_ecma_string */

/**
 * Increase reference counter of ecma-string.
 *
 * @return pointer to same ecma-string descriptor with increased reference counter
 *         or the ecma-string's copy with reference counter set to 1
 */
ecma_string_t*
ecma_copy_or_ref_ecma_string (ecma_string_t *string_desc_p) /**< string descriptor */
{
  JERRY_ASSERT (string_desc_p != NULL);
  JERRY_ASSERT (string_desc_p->refs > 0);

  if (string_desc_p->is_stack_var)
  {
    return ecma_copy_ecma_string (string_desc_p);
  }
  else
  {
    string_desc_p->refs++;

    if (unlikely (string_desc_p->refs == 0))
    {
      /* reference counter has overflowed */
      string_desc_p->refs--;

      uint32_t current_refs = string_desc_p->refs;

      /* First trying to free unreachable objects that maybe refer to the string */
      ecma_lcache_invalidate_all ();
      ecma_gc_run ();

      if (current_refs == string_desc_p->refs)
      {
        /* reference counter was not changed during GC, copying string */

        return ecma_copy_ecma_string (string_desc_p);
      }

      string_desc_p->refs++;

      JERRY_ASSERT (string_desc_p->refs != 0);
    }
  }

  return string_desc_p;
} /* ecma_copy_or_ref_ecma_string */

/**
 * Decrease reference counter and deallocate ecma-string if
 * after that the counter the counter becomes zero.
 */
void
ecma_deref_ecma_string (ecma_string_t *string_p) /**< ecma-string */
{
  JERRY_ASSERT (string_p != NULL);
  JERRY_ASSERT (string_p->refs != 0);

#ifndef JERRY_NDEBUG
  JERRY_ASSERT (string_p->is_stack_var == (!mem_is_heap_pointer (string_p)));
#endif /* !JERRY_NDEBUG */
  JERRY_ASSERT (!string_p->is_stack_var || string_p->refs == 1);

  string_p->refs--;

  if (string_p->refs != 0)
  {
    return;
  }

  switch ((ecma_string_container_t)string_p->container)
  {
    case ECMA_STRING_CONTAINER_HEAP_CHUNKS:
    {
      ecma_collection_header_t *chars_collection_p = ECMA_GET_NON_NULL_POINTER (ecma_collection_header_t,
                                                                                string_p->u.collection_cp);

      ecma_free_chars_collection (chars_collection_p);

      break;
    }
    case ECMA_STRING_CONTAINER_HEAP_NUMBER:
    {
      ecma_number_t *num_p = ECMA_GET_NON_NULL_POINTER (ecma_number_t,
                                                        string_p->u.number_cp);

      ecma_dealloc_number (num_p);

      break;
    }
    case ECMA_STRING_CONTAINER_CONCATENATION:
    {
      ecma_string_t *string1_p, *string2_p;

      string1_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                             string_p->u.concatenation.string1_cp);
      string2_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                             string_p->u.concatenation.string2_cp);

      ecma_deref_ecma_string (string1_p);
      ecma_deref_ecma_string (string2_p);

      break;
    }
    case ECMA_STRING_CONTAINER_LIT_TABLE:
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    case ECMA_STRING_CONTAINER_MAGIC_STRING_EX:
    {
      /* only the string descriptor itself should be freed */
    }
  }

  if (!string_p->is_stack_var)
  {
    ecma_dealloc_string (string_p);
  }
} /* ecma_deref_ecma_string */

/**
 * Assertion that specified ecma-string need not be freed
 */
void
ecma_check_that_ecma_string_need_not_be_freed (const ecma_string_t *string_p) /**< ecma-string descriptor */
{
#ifdef JERRY_NDEBUG
  (void) string_p;
#else /* JERRY_NDEBUG */
  /* Heap strings always need to be freed */
  JERRY_ASSERT (string_p->is_stack_var);

  /*
   * No reference counter increment or decrement
   * should be performed with ecma-string placed
   * on stack
   */
  JERRY_ASSERT (string_p->refs == 1);

  ecma_string_container_t container_type = (ecma_string_container_t) string_p->container;

  JERRY_ASSERT (container_type == ECMA_STRING_CONTAINER_LIT_TABLE ||
                container_type == ECMA_STRING_CONTAINER_MAGIC_STRING ||
                container_type == ECMA_STRING_CONTAINER_MAGIC_STRING_EX ||
                container_type == ECMA_STRING_CONTAINER_UINT32_IN_DESC);
#endif /* !JERRY_NDEBUG */
} /* ecma_check_that_ecma_string_need_not_be_freed */

/**
 * Convert ecma-string to number
 */
ecma_number_t
ecma_string_to_number (const ecma_string_t *str_p) /**< ecma-string */
{
  JERRY_ASSERT (str_p != NULL);

  switch ((ecma_string_container_t)str_p->container)
  {
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    {
      uint32_t uint32_number = str_p->u.uint32_number;

      return ecma_uint32_to_number (uint32_number);
    }

    case ECMA_STRING_CONTAINER_HEAP_NUMBER:
    {
      ecma_number_t *num_p = ECMA_GET_NON_NULL_POINTER (ecma_number_t,
                                                        str_p->u.number_cp);

      return *num_p;
    }

    case ECMA_STRING_CONTAINER_LIT_TABLE:
    case ECMA_STRING_CONTAINER_HEAP_CHUNKS:
    case ECMA_STRING_CONTAINER_CONCATENATION:
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    case ECMA_STRING_CONTAINER_MAGIC_STRING_EX:
    {
      const int32_t string_len = ecma_string_get_length (str_p);
      const size_t string_buf_size = (size_t) (string_len + 1) * sizeof (ecma_char_t);

      ecma_char_t *str_buffer_p = (ecma_char_t*) mem_heap_alloc_block (string_buf_size, MEM_HEAP_ALLOC_SHORT_TERM);
      if (str_buffer_p == NULL)
      {
        jerry_fatal (ERR_OUT_OF_MEMORY);
      }

      ssize_t bytes_copied = ecma_string_to_zt_string (str_p,
                                                       str_buffer_p,
                                                       (ssize_t) string_buf_size);
      JERRY_ASSERT (bytes_copied > 0);

      ecma_number_t num = ecma_zt_string_to_number (str_buffer_p);

      mem_heap_free_block (str_buffer_p);

      return num;
    }
  }

  JERRY_UNREACHABLE ();
} /* ecma_string_to_number */

/**
 * Convert ecma-string's contents to a zero-terminated string and put it to the buffer.
 *
 * @return number of bytes, actually copied to the buffer - if string's content was copied successfully;
 *         otherwise (in case size of buffer is insuficcient) - negative number, which is calculated
 *         as negation of buffer size, that is required to hold the string's content.
 */
ssize_t
ecma_string_to_zt_string (const ecma_string_t *string_desc_p, /**< ecma-string descriptor */
                          ecma_char_t *buffer_p, /**< destination buffer pointer (can be NULL if buffer_size == 0) */
                          ssize_t buffer_size) /**< size of buffer */
{
  JERRY_ASSERT (string_desc_p != NULL);
  JERRY_ASSERT (string_desc_p->refs > 0);
  JERRY_ASSERT (buffer_p != NULL || buffer_size == 0);
  JERRY_ASSERT (buffer_size >= 0);

  ssize_t required_buffer_size = ((ecma_string_get_length (string_desc_p) + 1) * ((ssize_t) sizeof (ecma_char_t)));

  if (required_buffer_size > buffer_size
      || buffer_size == 0)
  {
    return -required_buffer_size;
  }

  switch ((ecma_string_container_t)string_desc_p->container)
  {
    case ECMA_STRING_CONTAINER_HEAP_CHUNKS:
    {
      const ecma_collection_header_t *chars_collection_p = ECMA_GET_NON_NULL_POINTER (ecma_collection_header_t,
                                                                                      string_desc_p->u.collection_cp);

      ecma_copy_chars_collection_to_buffer (chars_collection_p, buffer_p, (size_t) buffer_size);

      break;
    }
    case ECMA_STRING_CONTAINER_LIT_TABLE:
    {
      literal_t lit = lit_get_literal_by_cp (string_desc_p->u.lit_cp);
      JERRY_ASSERT (lit->get_type () == LIT_STR_T);
      lit_literal_to_charset (lit, buffer_p, (size_t) required_buffer_size);
      break;
    }
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    {
      uint32_t uint32_number = string_desc_p->u.uint32_number;
      ssize_t bytes_copied = ecma_uint32_to_string (uint32_number, buffer_p, required_buffer_size);

      JERRY_ASSERT (bytes_copied == required_buffer_size);

      break;
    }
    case ECMA_STRING_CONTAINER_HEAP_NUMBER:
    {
      ecma_number_t *num_p = ECMA_GET_NON_NULL_POINTER (ecma_number_t,
                                                        string_desc_p->u.number_cp);

      ecma_length_t length = ecma_number_to_zt_string (*num_p, buffer_p, buffer_size);

      JERRY_ASSERT (required_buffer_size == (length + 1) * ((ssize_t) sizeof (ecma_char_t)));

      break;
    }
    case ECMA_STRING_CONTAINER_CONCATENATION:
    {
      const ecma_string_t *string1_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                                  string_desc_p->u.concatenation.string1_cp);
      const ecma_string_t *string2_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                                  string_desc_p->u.concatenation.string2_cp);

      ecma_char_t *dest_p = buffer_p;

      ssize_t bytes_copied1, bytes_copied2;

      bytes_copied1 = ecma_string_to_zt_string (string1_p, dest_p, buffer_size);
      JERRY_ASSERT (bytes_copied1 > 0);

      /* one character, which is the null character at end of string, will be overwritten */
      bytes_copied1 -= (ssize_t) sizeof (ecma_char_t);
      dest_p += ecma_string_get_length (string1_p);

      bytes_copied2 = ecma_string_to_zt_string (string2_p,
                                                dest_p,
                                                buffer_size - bytes_copied1);
      JERRY_ASSERT (bytes_copied2 > 0);

      JERRY_ASSERT (required_buffer_size == bytes_copied1 + bytes_copied2);

      break;
    }
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    {
      const ecma_magic_string_id_t id = string_desc_p->u.magic_string_id;
      const size_t length = ecma_magic_string_lengths[id];

      size_t bytes_to_copy = (length + 1) * sizeof (ecma_char_t);

      memcpy (buffer_p, ecma_get_magic_string_zt (id), bytes_to_copy);

      JERRY_ASSERT (required_buffer_size == (ssize_t) bytes_to_copy);

      break;
    }
    case ECMA_STRING_CONTAINER_MAGIC_STRING_EX:
    {
      const ecma_magic_string_ex_id_t id = string_desc_p->u.magic_string_ex_id;
      const size_t length = ecma_magic_string_ex_lengths[id];

      size_t bytes_to_copy = (length + 1) * sizeof (ecma_char_t);

      memcpy (buffer_p, ecma_get_magic_string_ex_zt (id), bytes_to_copy);

      JERRY_ASSERT (required_buffer_size == (ssize_t) bytes_to_copy);

      break;
    }
  }

  return required_buffer_size;
} /* ecma_string_to_zt_string */

/**
 * Long path part of ecma-string to ecma-string comparison routine
 *
 * See also:
 *          ecma_compare_ecma_strings
 *
 * @return true - if strings are equal;
 *         false - otherwise.
 */
static bool __attr_noinline___
ecma_compare_ecma_strings_longpath (const ecma_string_t *string1_p, /* ecma-string */
                                    const ecma_string_t *string2_p) /* ecma-string */
{
  if (string1_p->container == string2_p->container)
  {
    if (string1_p->container == ECMA_STRING_CONTAINER_LIT_TABLE)
    {
      JERRY_ASSERT (string1_p->u.lit_cp.packed_value != string2_p->u.lit_cp.packed_value);

      return false;
    }
    else if (string1_p->container == ECMA_STRING_CONTAINER_MAGIC_STRING)
    {
      JERRY_ASSERT (string1_p->u.magic_string_id != string2_p->u.magic_string_id);

      return false;
    }
    else if (string1_p->container == ECMA_STRING_CONTAINER_MAGIC_STRING_EX)
    {
      JERRY_ASSERT (string1_p->u.magic_string_ex_id != string2_p->u.magic_string_ex_id);

      return false;
    }
    else if (string1_p->container == ECMA_STRING_CONTAINER_UINT32_IN_DESC)
    {
      JERRY_ASSERT (string1_p->u.uint32_number != string2_p->u.uint32_number);

      return false;
    }
  }

  const int32_t string1_len = ecma_string_get_length (string1_p);
  const int32_t string2_len = ecma_string_get_length (string2_p);

  if (string1_len != string2_len)
  {
    return false;
  }

  const int32_t strings_len = string1_len;

  if (strings_len == 0)
  {
    return true;
  }

  if (string1_p->container == string2_p->container)
  {
    switch ((ecma_string_container_t) string1_p->container)
    {
      case ECMA_STRING_CONTAINER_HEAP_NUMBER:
      {
        ecma_number_t *num1_p, *num2_p;
        num1_p = ECMA_GET_NON_NULL_POINTER (ecma_number_t, string1_p->u.number_cp);
        num2_p = ECMA_GET_NON_NULL_POINTER (ecma_number_t, string2_p->u.number_cp);

        if (ecma_number_is_nan (*num1_p)
            && ecma_number_is_nan (*num2_p))
        {
          return true;
        }

        return (*num1_p == *num2_p);
      }
      case ECMA_STRING_CONTAINER_HEAP_CHUNKS:
      {
        const ecma_collection_header_t *chars_collection1_p = ECMA_GET_NON_NULL_POINTER (ecma_collection_header_t,
                                                                                         string1_p->u.collection_cp);
        const ecma_collection_header_t *chars_collection2_p = ECMA_GET_NON_NULL_POINTER (ecma_collection_header_t,
                                                                                         string2_p->u.collection_cp);

        return ecma_compare_chars_collection (chars_collection1_p, chars_collection2_p);
      }
      case ECMA_STRING_CONTAINER_CONCATENATION:
      {
        /* long path */
        break;
      }
      case ECMA_STRING_CONTAINER_LIT_TABLE:
      {
        JERRY_ASSERT (string1_p->u.lit_cp.packed_value != string2_p->u.lit_cp.packed_value);

        return false;
      }
      case ECMA_STRING_CONTAINER_MAGIC_STRING:
      {
        JERRY_ASSERT (string1_p->u.magic_string_id != string2_p->u.magic_string_id);

        return false;
      }
      case ECMA_STRING_CONTAINER_MAGIC_STRING_EX:
      {
        JERRY_ASSERT (string1_p->u.magic_string_ex_id != string2_p->u.magic_string_ex_id);

        return false;
      }
      case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
      {
        JERRY_ASSERT (string1_p->u.uint32_number != string2_p->u.uint32_number);

        return false;
      }
    }
  }

  JERRY_ASSERT (strings_len != 0);

  const size_t string_buf_size = (size_t) (strings_len + 1) * sizeof (ecma_char_t);

  ecma_char_t *string1_buf = (ecma_char_t*) mem_heap_alloc_block (string_buf_size, MEM_HEAP_ALLOC_SHORT_TERM);
  ecma_char_t *string2_buf = (ecma_char_t*) mem_heap_alloc_block (string_buf_size, MEM_HEAP_ALLOC_SHORT_TERM);

  if (string1_buf == NULL
      || string2_buf == NULL)
  {
    jerry_fatal (ERR_OUT_OF_MEMORY);
  }

  ssize_t req_size;

  req_size = ecma_string_to_zt_string (string1_p, string1_buf, (ssize_t) string_buf_size);
  JERRY_ASSERT (req_size > 0);
  req_size = ecma_string_to_zt_string (string2_p, string2_buf, (ssize_t) string_buf_size);
  JERRY_ASSERT (req_size > 0);

  bool is_equal = (memcmp (string1_buf, string2_buf, string_buf_size) == 0);

  mem_heap_free_block (string1_buf);
  mem_heap_free_block (string2_buf);

  return is_equal;
} /* ecma_compare_ecma_strings_longpath */

/**
 * Compare ecma-string to ecma-string if they're hashes are equal
 *
 * @return true - if strings are equal;
 *         false - may be.
 */
bool
ecma_compare_ecma_strings_equal_hashes (const ecma_string_t *string1_p, /* ecma-string */
                                        const ecma_string_t *string2_p) /* ecma-string */
{
  JERRY_ASSERT (string1_p->hash == string2_p->hash);

  if (string1_p->container == string2_p->container
      && string1_p->u.common_field == string2_p->u.common_field)
  {
    return true;
  }
  else
  {
    return false;
  }
} /* ecma_compare_ecma_strings_equal_hashes */

/**
 * Compare ecma-string to ecma-string
 *
 * @return true - if strings are equal;
 *         false - otherwise.
 */
bool
ecma_compare_ecma_strings (const ecma_string_t *string1_p, /* ecma-string */
                           const ecma_string_t *string2_p) /* ecma-string */
{
  JERRY_ASSERT (string1_p != NULL && string2_p != NULL);

  const bool is_equal_hashes = (string1_p->hash == string2_p->hash);

  if (!is_equal_hashes)
  {
    return false;
  }
  const bool is_equal_containers = (string1_p->container == string2_p->container);
  const bool is_equal_fields = (string1_p->u.common_field == string2_p->u.common_field);

  if (is_equal_containers && is_equal_fields)
  {
    return true;
  }
  else
  {
    return ecma_compare_ecma_strings_longpath (string1_p, string2_p);
  }
} /* ecma_compare_ecma_strings */

/**
 * Relational compare of ecma-strings.
 *
 * First string is less than second string if:
 *  - strings are not equal;
 *  - first string is prefix of second or is lexicographically less than second.
 *
 * @return true - if first string is less than second string,
 *         false - otherwise.
 */
bool
ecma_compare_ecma_strings_relational (const ecma_string_t *string1_p, /**< ecma-string */
                                      const ecma_string_t *string2_p) /**< ecma-string */
{
  if (ecma_compare_ecma_strings (string1_p,
                                 string2_p))
  {
    return false;
  }

  const ecma_char_t *zt_string1_p, *zt_string2_p;
  bool is_zt_string1_on_heap = false, is_zt_string2_on_heap = false;
  ecma_char_t zt_string1_buffer[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER + 1];
  ecma_char_t zt_string2_buffer[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER + 1];

  ssize_t req_size = ecma_string_to_zt_string (string1_p,
                                               zt_string1_buffer,
                                               sizeof (zt_string1_buffer));

  if (req_size < 0)
  {
    ecma_char_t *heap_buffer_p = (ecma_char_t*) mem_heap_alloc_block ((size_t) -req_size, MEM_HEAP_ALLOC_SHORT_TERM);
    if (heap_buffer_p == NULL)
    {
      jerry_fatal (ERR_OUT_OF_MEMORY);
    }

    ssize_t bytes_copied = ecma_string_to_zt_string (string1_p,
                                                     heap_buffer_p,
                                                     -req_size);

    JERRY_ASSERT (bytes_copied > 0);

    zt_string1_p = heap_buffer_p;
    is_zt_string1_on_heap = true;
  }
  else
  {
    zt_string1_p = zt_string1_buffer;
  }

  req_size = ecma_string_to_zt_string (string2_p,
                                       zt_string2_buffer,
                                       sizeof (zt_string2_buffer));

  if (req_size < 0)
  {
    ecma_char_t *heap_buffer_p = (ecma_char_t*) mem_heap_alloc_block ((size_t) -req_size, MEM_HEAP_ALLOC_SHORT_TERM);
    if (heap_buffer_p == NULL)
    {
      jerry_fatal (ERR_OUT_OF_MEMORY);
    }

    ssize_t bytes_copied = ecma_string_to_zt_string (string2_p,
                                                     heap_buffer_p,
                                                     -req_size);

    JERRY_ASSERT (bytes_copied > 0);

    zt_string2_p = heap_buffer_p;
    is_zt_string2_on_heap = true;
  }
  else
  {
    zt_string2_p = zt_string2_buffer;
  }

  bool is_first_less_than_second = ecma_compare_zt_strings_relational (zt_string1_p,
                                                                       zt_string2_p);

  if (is_zt_string1_on_heap)
  {
    mem_heap_free_block ((void*) zt_string1_p);
  }

  if (is_zt_string2_on_heap)
  {
    mem_heap_free_block ((void*) zt_string2_p);
  }

  return is_first_less_than_second;
}
/* ecma_compare_ecma_strings_relational */

/**
 * Get length of ecma-string
 *
 * @return number of characters in the string
 */
int32_t
ecma_string_get_length (const ecma_string_t *string_p) /**< ecma-string */
{
  ecma_string_container_t container = (ecma_string_container_t) string_p->container;

  if (container == ECMA_STRING_CONTAINER_LIT_TABLE)
  {
    literal_t lit = lit_get_literal_by_cp (string_p->u.lit_cp);
    JERRY_ASSERT (lit->get_type () == LIT_STR_T);
    return lit_charset_record_get_length (lit);
  }
  else if (container == ECMA_STRING_CONTAINER_MAGIC_STRING)
  {
    return ecma_magic_string_lengths[string_p->u.magic_string_id];
  }
  else if (container == ECMA_STRING_CONTAINER_MAGIC_STRING_EX)
  {
    return ecma_magic_string_ex_lengths[string_p->u.magic_string_ex_id];
  }
  else if (container == ECMA_STRING_CONTAINER_UINT32_IN_DESC)
  {
    const uint32_t uint32_number = string_p->u.uint32_number;
    const int32_t max_uint32_len = 10;
    const uint32_t nums_with_ascending_length[10] =
    {
      1u,
      10u,
      100u,
      1000u,
      10000u,
      100000u,
      1000000u,
      10000000u,
      100000000u,
      1000000000u
    };

    int32_t length = 1;

    while (length < max_uint32_len
           && uint32_number >= nums_with_ascending_length[length])
    {
      length++;
    }

    return length;
  }
  else if (container == ECMA_STRING_CONTAINER_HEAP_NUMBER)
  {
    const ecma_number_t *num_p = ECMA_GET_NON_NULL_POINTER (ecma_number_t,
                                                            string_p->u.number_cp);

    ecma_char_t buffer[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER + 1];

    return ecma_number_to_zt_string (*num_p,
                                     buffer,
                                     sizeof (buffer));
  }
  else if (container == ECMA_STRING_CONTAINER_HEAP_CHUNKS)
  {
    const ecma_collection_header_t *collection_header_p = ECMA_GET_NON_NULL_POINTER (ecma_collection_header_t,
                                                                                     string_p->u.collection_cp);

    return collection_header_p->unit_number;
  }
  else
  {
    JERRY_ASSERT (container == ECMA_STRING_CONTAINER_CONCATENATION);

    const ecma_string_t *string1_p, *string2_p;

    string1_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t, string_p->u.concatenation.string1_cp);
    string2_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t, string_p->u.concatenation.string2_cp);

    return ecma_string_get_length (string1_p) + ecma_string_get_length (string2_p);
  }
} /* ecma_string_get_length */

/**
 * Get character from specified position in the ecma-string.
 *
 * @return character value
 */
ecma_char_t
ecma_string_get_char_at_pos (const ecma_string_t *string_p, /**< ecma-string */
                             uint32_t index) /**< index of character */
{
  uint32_t length = (uint32_t) ecma_string_get_length (string_p);
  JERRY_ASSERT (index < (uint32_t) length);

  size_t buffer_size = sizeof (ecma_char_t) * (length + 1);
  ecma_char_t *zt_str_p = (ecma_char_t*) mem_heap_alloc_block (buffer_size,
                                                               MEM_HEAP_ALLOC_SHORT_TERM);

  if (zt_str_p == NULL)
  {
    jerry_fatal (ERR_OUT_OF_MEMORY);
  }

  ecma_string_to_zt_string (string_p, zt_str_p, (ssize_t) buffer_size);

  ecma_char_t ch = zt_str_p[index];

  mem_heap_free_block (zt_str_p);

  return ch;
} /* ecma_string_get_char_at_pos */

/**
 * Compare zero-terminated string to zero-terminated string
 *
 * @return  true - if strings are equal;
 *          false - otherwise.
 */
bool
ecma_compare_zt_strings (const ecma_char_t *string1_p, /**< zero-terminated string */
                         const ecma_char_t *string2_p) /**< zero-terminated string */
{
  const ecma_char_t *iter_1_p = string1_p;
  const ecma_char_t *iter_2_p = string2_p;

  while (*iter_1_p != ECMA_CHAR_NULL)
  {
    if (*iter_1_p++ != *iter_2_p++)
    {
      return false;
    }
  }

  return (*iter_2_p == ECMA_CHAR_NULL);
} /* ecma_compare_zt_strings */

/**
 * Relational compare of zero-terminated strings
 *
 * First string is less than second string if:
 *  - strings are not equal;
 *  - first string is prefix of second or is lexicographically less than second.
 *
 * @return true - if first string is less than second string,
 *         false - otherwise.
 */
bool
ecma_compare_zt_strings_relational (const ecma_char_t *string1_p, /**< zero-terminated string */
                                    const ecma_char_t *string2_p) /**< zero-terminated string */
{
  const ecma_char_t *iter_1_p = string1_p;
  const ecma_char_t *iter_2_p = string2_p;

  while (*iter_1_p != ECMA_CHAR_NULL
         && *iter_2_p != ECMA_CHAR_NULL)
  {
#if CONFIG_ECMA_CHAR_ENCODING == CONFIG_ECMA_CHAR_ASCII
    const ecma_char_t chr_1 = *iter_1_p++;
    const ecma_char_t chr_2 = *iter_2_p++;

    if (chr_1 < chr_2)
    {
      return true;
    }
    else if (chr_1 > chr_2)
    {
      return false;
    }
#elif CONFIG_ECMA_CHAR_ENCODING == CONFIG_ECMA_CHAR_UTF16
    const ecma_char_t first_in_pair_range_begin = 0xD800;
    const ecma_char_t first_in_pair_range_end = 0xDBFF;
    const ecma_char_t second_in_pair_range_begin = 0xDC00;
    const ecma_char_t second_in_pair_range_end = 0xDFFF;

    const bool iter_1_at_first_in_pair = (*iter_1_p >= first_in_pair_range_begin
                                          && *iter_1_p <= first_in_pair_range_end);
    const bool iter_2_at_first_in_pair = (*iter_2_p >= first_in_pair_range_begin
                                          && *iter_2_p <= first_in_pair_range_end);
    const bool iter_1_at_second_in_pair = (*iter_1_p >= second_in_pair_range_begin
                                           && *iter_1_p <= second_in_pair_range_end);
    const bool iter_2_at_second_in_pair = (*iter_2_p >= second_in_pair_range_begin
                                           && *iter_2_p <= second_in_pair_range_end);

    JERRY_ASSERT (!iter_1_at_second_in_pair
                  && !iter_2_at_second_in_pair);

    /* Pairs encode range U+010000 to U+10FFFF,
       while single chars encode U+0000 to U+7DFF and U+E000 to U+FFFF */
    if (iter_1_at_first_in_pair
        && !iter_2_at_first_in_pair)
    {
      return false;
    }
    else if (!iter_1_at_first_in_pair
             && iter_2_at_first_in_pair)
    {
      return true;
    }
    else if (!iter_1_at_first_in_pair
             && !iter_2_at_first_in_pair)
    {
      const ecma_char_t chr_1 = *iter_1_p;
      const ecma_char_t chr_2 = *iter_2_p;

      if (chr_1 < chr_2)
      {
        return true;
      }
      else
      {
        return false;
      }

      iter_1_p++;
      iter_2_p++;
    }
    else
    {
      JERRY_ASSERT (iter_1_at_first_in_pair
                    && iter_2_at_first_in_pair);

      uint32_t chr1, chr2;

      chr1 = *iter_1_p++ - first_in_pair_range_begin;
      chr1 <<= 10;
      JERRY_ASSERT (*iter_1_p >= second_in_pair_range_begin
                    && *iter_1_p <= second_in_pair_range_end);
      chr1 += *iter_1_p++ - second_in_pair_range_begin;

      chr2 = *iter_2_p++ - first_in_pair_range_begin;
      chr2 <<= 10;
      JERRY_ASSERT (*iter_2_p >= second_in_pair_range_begin
                    && *iter_2_p <= second_in_pair_range_end);
      chr2 += *iter_2_p++ - second_in_pair_range_begin;

      if (chr1 < chr2)
      {
        return true;
      }
      else if (chr1 > chr2)
      {
        return false;
      }
    }
#endif /* CONFIG_ECMA_CHAR_ENCODING == CONFIG_ECMA_CHAR_UTF16 */
  }

  return (*iter_1_p == ECMA_CHAR_NULL
          && *iter_2_p != ECMA_CHAR_NULL);
} /* ecma_compare_zt_strings_relational */

/**
 * Copy zero-terminated string to buffer
 *
 * Warning:
 *         the routine requires that buffer size is enough
 *
 * @return pointer to null character of copied string in destination buffer
 */
ecma_char_t*
ecma_copy_zt_string_to_buffer (const ecma_char_t *string_p, /**< zero-terminated string */
                               ecma_char_t *buffer_p, /**< destination buffer */
                               ssize_t buffer_size) /**< size of buffer */
{
  const ecma_char_t *str_iter_p = string_p;
  ecma_char_t *buf_iter_p = buffer_p;
  ssize_t bytes_copied = 0;

  while (*str_iter_p != ECMA_CHAR_NULL)
  {
    bytes_copied += (ssize_t) sizeof (ecma_char_t);
    JERRY_ASSERT (bytes_copied <= buffer_size);

    *buf_iter_p++ = *str_iter_p++;
  }

  bytes_copied += (ssize_t) sizeof (ecma_char_t);
  JERRY_ASSERT (bytes_copied <= buffer_size);

  *buf_iter_p = ECMA_CHAR_NULL;

  return buf_iter_p;
} /* ecma_copy_zt_string_to_buffer */

/**
 * Calculate zero-terminated string's length
 *
 * @return length of string
 */
ecma_length_t
ecma_zt_string_length (const ecma_char_t *string_p) /**< zero-terminated string */
{
  const ecma_char_t *str_iter_p = string_p;

  ecma_length_t length = 0;

  while (*str_iter_p++)
  {
    length++;

    /* checking overflow */
    JERRY_ASSERT (length != 0);
  }

  return length;
} /* ecma_zt_string_length */

/**
 * Get specified magic string as zero-terminated string
 *
 * @return pointer to zero-terminated magic string
 */
const ecma_char_t*
ecma_get_magic_string_zt (ecma_magic_string_id_t id) /**< magic string id */
{
  TODO (Support UTF-16);

  switch (id)
  {
#define ECMA_MAGIC_STRING_DEF(id, ascii_zt_string) \
     case id: return (ecma_char_t*) ascii_zt_string;
#include "ecma-magic-strings.inc.h"
#undef ECMA_MAGIC_STRING_DEF

    case ECMA_MAGIC_STRING__COUNT: break;
  }

  JERRY_UNREACHABLE ();
} /* ecma_get_magic_string_zt */

/**
 * Get specified magic string as zero-terminated string from external table
 *
 * @return pointer to zero-terminated magic string
 */
const ecma_char_t*
ecma_get_magic_string_ex_zt (ecma_magic_string_ex_id_t id) /**< extern magic string id */
{
  TODO (Support UTF-16);

  if (ecma_magic_string_ex_array && id < ecma_magic_string_ex_count)
  {
    return ecma_magic_string_ex_array[id];
  }

  JERRY_UNREACHABLE ();
} /* ecma_get_magic_string_ex_zt */


/**
 * Get specified magic string
 *
 * @return ecma-string containing specified magic string
 */
ecma_string_t*
ecma_get_magic_string (ecma_magic_string_id_t id) /**< magic string id */
{
  return ecma_new_ecma_string_from_magic_string_id (id);
} /* ecma_get_magic_string */

/**
 * Get specified external magic string
 *
 * @return ecma-string containing specified external magic string
 */
ecma_string_t*
ecma_get_magic_string_ex (ecma_magic_string_ex_id_t id) /**< external magic string id */
{
  return ecma_new_ecma_string_from_magic_string_ex_id (id);
} /* ecma_get_magic_string_ex */

/**
 * Check if passed zt-string equals to one of magic strings
 * and if equal magic string was found, return it's id in 'out_id_p' argument.
 *
 * @return true - if magic string equal to passed string was found,
 *         false - otherwise.
 */
bool
ecma_is_zt_string_magic (const ecma_char_t *zt_string_p, /**< zero-terminated string */
                         ecma_magic_string_id_t *out_id_p) /**< out: magic string's id */
{
  TODO (Improve performance of search);

  for (ecma_magic_string_id_t id = (ecma_magic_string_id_t) 0;
       id < ECMA_MAGIC_STRING__COUNT;
       id = (ecma_magic_string_id_t) (id + 1))
  {
    if (ecma_compare_zt_strings (zt_string_p, ecma_get_magic_string_zt (id)))
    {
      *out_id_p = id;

      return true;
    }
  }

  *out_id_p = ECMA_MAGIC_STRING__COUNT;

  return false;
} /* ecma_is_zt_string_magic */

/**
 * Check if passed zt-string equals to one of external magic strings
 * and if equal magic string was found, return it's id in 'out_id_p' argument.
 *
 * @return true - if external magic string equal to passed string was found,
 *         false - otherwise.
 */
bool
ecma_is_zt_ex_string_magic (const ecma_char_t *zt_string_p, /**< zero-terminated string */
                            ecma_magic_string_ex_id_t *out_id_p) /**< out: external magic string's id */
{
  TODO (Improve performance of search);

  for (ecma_magic_string_ex_id_t id = (ecma_magic_string_ex_id_t) 0;
       id < ecma_magic_string_ex_count;
       id = (ecma_magic_string_ex_id_t) (id + 1))
  {
    if (ecma_compare_zt_strings (zt_string_p, ecma_get_magic_string_ex_zt (id)))
    {
      *out_id_p = id;

      return true;
    }
  }

  *out_id_p = ecma_magic_string_ex_count;

  return false;
} /* ecma_is_zt_ex_string_magic */

/**
 * Long path part of ecma_is_string_magic
 *
 * Converts passed ecma-string to zt-string and
 * checks if it is equal to one of magic string
 *
 * @return true - if magic string equal to passed string was found,
 *         false - otherwise.
 */
static bool
ecma_is_string_magic_longpath (const ecma_string_t *string_p, /**< ecma-string */
                               ecma_magic_string_id_t *out_id_p) /**< out: magic string's id */
{
  ecma_char_t zt_string_buffer[ECMA_STRING_MAGIC_STRING_LENGTH_LIMIT + 1];

  ssize_t copied = ecma_string_to_zt_string (string_p, zt_string_buffer, (ssize_t) sizeof (zt_string_buffer));
  JERRY_ASSERT (copied > 0);

  return ecma_is_zt_string_magic (zt_string_buffer, out_id_p);
} /* ecma_is_string_magic_longpath */

/**
 * Long path part of ecma_is_ex_string_magic
 *
 * Converts passed ecma-string to zt-string and
 * checks if it is equal to one of magic string
 *
 * @return true - if magic string equal to passed string was found,
 *         false - otherwise.
 */
static bool
ecma_is_ex_string_magic_longpath (const ecma_string_t *string_p, /**< ecma-string */
                                  ecma_magic_string_ex_id_t *out_id_p) /**< out: external magic string's id */
{
  ecma_char_t zt_string_buffer[ECMA_STRING_MAGIC_STRING_LENGTH_LIMIT + 1];

  ssize_t copied = ecma_string_to_zt_string (string_p, zt_string_buffer, (ssize_t) sizeof (zt_string_buffer));
  JERRY_ASSERT (copied > 0);

  return ecma_is_zt_ex_string_magic (zt_string_buffer, out_id_p);
} /* ecma_is_ex_string_magic_longpath */

/**
 * Check if passed string equals to one of magic strings
 * and if equal magic string was found, return it's id in 'out_id_p' argument.
 *
 * @return true - if magic string equal to passed string was found,
 *         false - otherwise.
 */
bool
ecma_is_string_magic (const ecma_string_t *string_p, /**< ecma-string */
                      ecma_magic_string_id_t *out_id_p) /**< out: magic string's id */
{
  if (string_p->container == ECMA_STRING_CONTAINER_MAGIC_STRING)
  {
    JERRY_ASSERT (string_p->u.magic_string_id < ECMA_MAGIC_STRING__COUNT);

    *out_id_p = (ecma_magic_string_id_t) string_p->u.magic_string_id;

    return true;
  }
  else if (string_p->container == ECMA_STRING_CONTAINER_CONCATENATION
           && ecma_string_get_length (string_p) <= ECMA_STRING_MAGIC_STRING_LENGTH_LIMIT)
  {
    return ecma_is_string_magic_longpath (string_p, out_id_p);
  }
  else
  {
    /*
     * Any ecma-string constructor except ecma_concat_ecma_strings
     * should return ecma-string with ECMA_STRING_CONTAINER_MAGIC_STRING
     * container type if new ecma-string's content is equal to one of magic strings.
     */
    JERRY_ASSERT (ecma_string_get_length (string_p) > ECMA_STRING_MAGIC_STRING_LENGTH_LIMIT
                  || !ecma_is_string_magic_longpath (string_p, out_id_p));

    return false;
  }
} /* ecma_is_string_magic */

/**
 * Check if passed string equals to one of external magic strings
 * and if equal external magic string was found, return it's id in 'out_id_p' argument.
 *
 * @return true - if external magic string equal to passed string was found,
 *         false - otherwise.
 */
bool
ecma_is_ex_string_magic (const ecma_string_t *string_p, /**< ecma-string */
                         ecma_magic_string_ex_id_t *out_id_p) /**< out: external magic string's id */
{
  if (string_p->container == ECMA_STRING_CONTAINER_MAGIC_STRING_EX)
  {
    JERRY_ASSERT (string_p->u.magic_string_ex_id < ecma_magic_string_ex_count);

    *out_id_p = (ecma_magic_string_ex_id_t) string_p->u.magic_string_ex_id;

    return true;
  }
  else if (string_p->container == ECMA_STRING_CONTAINER_CONCATENATION
           && ecma_string_get_length (string_p) <= ECMA_STRING_MAGIC_STRING_LENGTH_LIMIT)
  {
    return ecma_is_ex_string_magic_longpath (string_p, out_id_p);
  }
  else
  {
    /*
     * Any ecma-string constructor except ecma_concat_ecma_strings
     * should return ecma-string with ECMA_STRING_CONTAINER_MAGIC_STRING_EX
     * container type if new ecma-string's content is equal to one of external magic strings.
     */
    JERRY_ASSERT (ecma_string_get_length (string_p) > ECMA_STRING_MAGIC_STRING_LENGTH_LIMIT
                  || !ecma_is_ex_string_magic_longpath (string_p, out_id_p));

    return false;
  }
} /* ecma_is_ex_string_magic */

/**
 * Try to calculate hash of the ecma-string
 *
 * @return calculated hash
 */
ecma_string_hash_t
ecma_string_hash (const ecma_string_t *string_p) /**< ecma-string to calculate hash for */

{
  return (string_p->hash);
} /* ecma_string_try_hash */

/**
 * Calculate hash from last ECMA_STRING_HASH_LAST_CHARS_COUNT characters from the buffer.
 *
 * @return ecma-string's hash
 */
ecma_string_hash_t
ecma_chars_buffer_calc_hash_last_chars (const ecma_char_t *chars, /**< characters buffer */
                                        ecma_length_t length) /**< number of characters in the buffer */
{
  JERRY_ASSERT (chars != NULL);

  ecma_char_t char1 = (length > 0) ? chars[length - 1] : ECMA_CHAR_NULL;
  ecma_char_t char2 = (length > 1) ? chars[length - 2] : ECMA_CHAR_NULL;

  uint32_t t1 = (uint32_t) char1 + (uint32_t) char2;
  uint32_t t2 = t1 * 0x24418b66;
  uint32_t t3 = (t2 >> 16) ^ (t2 & 0xffffu);
  uint32_t t4 = (t3 >> 8) ^ (t3 & 0xffu);

  return (ecma_string_hash_t) t4;
} /* ecma_chars_buffer_calc_hash_last_chars */

/**
 * @}
 * @}
 */
