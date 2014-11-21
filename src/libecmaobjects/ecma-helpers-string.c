/* Copyright 2014 Samsung Electronics Co., Ltd.
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

#include "deserializer.h"
#include "ecma-alloc.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "globals.h"
#include "jerry-libc.h"
#include "interpreter.h"

/**
 * Value for length field of ecma_string_t indicating that
 * length of string is unknown and should be calculated.
 */
#define ECMA_STRING_LENGTH_SHOULD_BE_CALCULATED ((ecma_length_t)-1)

/**
 * Maximum length of strings' concatenation
 */
#define ECMA_STRING_MAX_CONCATENATION_LENGTH (CONFIG_ECMA_STRING_MAX_CONCATENATION_LENGTH)

/**
 * The length should be representable with int32_t.
 */
JERRY_STATIC_ASSERT ((uint32_t) ((int32_t) ECMA_STRING_MAX_CONCATENATION_LENGTH) ==
                     ECMA_STRING_MAX_CONCATENATION_LENGTH);

/**
 * Lengths of magic strings
 */
static ecma_length_t ecma_magic_string_lengths [ECMA_MAGIC_STRING__COUNT];

/**
 * Maximum length among lengths of magic strings
 */
static ecma_length_t ecma_magic_string_max_length;

static void
ecma_init_ecma_string_from_lit_index (ecma_string_t *string_p,
                                      literal_index_t lit_index,
                                      bool is_stack_var);
static void
ecma_init_ecma_string_from_magic_string_id (ecma_string_t *string_p,
                                            ecma_magic_string_id_t magic_string_id,
                                            bool is_stack_var);

/**
 * Initialize data for string helpers
 */
void
ecma_strings_init (void)
{
  /* Initializing magic strings information */

  ecma_magic_string_max_length = 0;

  for (ecma_magic_string_id_t id = 0;
       id < ECMA_MAGIC_STRING__COUNT;
       id++)
  {
    ecma_magic_string_lengths [id] = ecma_zt_string_length (ecma_get_magic_string_zt (id));

    ecma_magic_string_max_length = JERRY_MAX (ecma_magic_string_max_length, ecma_magic_string_lengths [id]);
  }
} /* ecma_strings_init */

/**
 * Initialize ecma-string descriptor with string described by index in literal table
 */
static void
ecma_init_ecma_string_from_lit_index (ecma_string_t *string_p, /**< descriptor to initialize */
                                      literal_index_t lit_index, /**< index in the literal table */
                                      bool is_stack_var) /**< flag indicating whether the string descriptor
                                                              is placed on stack (true) or in the heap (false) */
{
#ifndef JERRY_NDEBUG
  JERRY_ASSERT (is_stack_var == (!mem_is_heap_pointer (string_p)));
#endif /* !JERRY_NDEBUG */

  const literal lit = deserialize_literal_by_id ((idx_t) lit_index);
  if (lit.type == LIT_MAGIC_STR)
  {
    ecma_init_ecma_string_from_magic_string_id (string_p,
                                                lit.data.magic_str_id,
                                                is_stack_var);

    return;
  }

  JERRY_ASSERT (lit.type == LIT_STR);

  string_p->refs = 1;
  string_p->is_stack_var = is_stack_var;

  string_p->length = lit.data.lp.length;
  string_p->container = ECMA_STRING_CONTAINER_LIT_TABLE;

  string_p->u.lit_index = lit_index;
} /* ecma_init_ecma_string_from_lit_index */

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
  string_p->is_stack_var = is_stack_var;

  string_p->length = ecma_magic_string_lengths [magic_string_id];
  string_p->container = ECMA_STRING_CONTAINER_MAGIC_STRING;

  string_p->u.magic_string_id = magic_string_id;
} /* ecma_init_ecma_string_from_magic_string_id */

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

  ecma_length_t length = 0;
  const ecma_char_t *iter_p = string_p;

  while (*iter_p++)
  {
    length++;
  }

  const size_t bytes_needed_for_current_string = sizeof (ecma_char_t) * length;
  const size_t bytes_for_chars_in_string_descriptor = JERRY_SIZE_OF_STRUCT_MEMBER (ecma_string_t, u.chars);
  JERRY_STATIC_ASSERT (bytes_for_chars_in_string_descriptor % sizeof (ecma_char_t) == 0);

  ecma_string_t* string_desc_p = ecma_alloc_string ();
  string_desc_p->refs = 1;
  string_desc_p->is_stack_var = false;
  string_desc_p->length = length;

  if (bytes_needed_for_current_string <= bytes_for_chars_in_string_descriptor)
  {
    string_desc_p->container = ECMA_STRING_CONTAINER_CHARS_IN_DESC;
    __memcpy (string_desc_p->u.chars, string_p, bytes_needed_for_current_string);

    return string_desc_p;
  }

  string_desc_p->container = ECMA_STRING_CONTAINER_HEAP_CHUNKS;

  const ecma_char_t* src_p = string_p;

  ecma_length_t chars_left = length;
  JERRY_ASSERT (chars_left > 0);

  ecma_collection_chunk_t *string_chunk_p = ecma_alloc_collection_chunk ();

  ECMA_SET_NON_NULL_POINTER (string_desc_p->u.chunk_cp, string_chunk_p);

  const ecma_length_t max_chars_in_chunk = sizeof (string_chunk_p->data) / sizeof (ecma_char_t);
  while (chars_left > 0)
  {
    ecma_length_t chars_to_copy = JERRY_MIN(chars_left, max_chars_in_chunk);

    __memcpy (string_chunk_p->data, src_p, chars_to_copy * sizeof (ecma_char_t));
    chars_left = (ecma_length_t) (chars_left - chars_to_copy);
    src_p += chars_to_copy;

    ecma_collection_chunk_t* next_string_chunk_p = ecma_alloc_collection_chunk ();
    ECMA_SET_NON_NULL_POINTER (string_chunk_p->next_chunk_cp, next_string_chunk_p);
    string_chunk_p = next_string_chunk_p;
  }

  string_chunk_p->next_chunk_cp = ECMA_NULL_POINTER;

  return string_desc_p;
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
  string_desc_p->u.uint32_number = uint32_number;

  const uint32_t max_uint32_len = 10;
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

  string_desc_p->length = 1;

  while (string_desc_p->length < max_uint32_len
         && uint32_number >= nums_with_ascending_length [string_desc_p->length])
  {
    string_desc_p->length++;
  }

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

  ecma_char_t buffer[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER + 1];
  ecma_length_t length = ecma_number_to_zt_string (num,
                                                   buffer,
                                                   sizeof (buffer));

  ecma_string_t* string_desc_p = ecma_alloc_string ();
  string_desc_p->refs = 1;
  string_desc_p->is_stack_var = false;
  string_desc_p->length = length;
  string_desc_p->container = ECMA_STRING_CONTAINER_HEAP_NUMBER;

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
ecma_new_ecma_string_on_stack_from_lit_index (ecma_string_t *string_p, /**< pointer to the ecma-string
                                                                            descriptor to initialize */
                                              literal_index_t lit_index) /**< index in the literal table */
{
  ecma_init_ecma_string_from_lit_index (string_p, lit_index, true);
} /* ecma_new_ecma_string_on_stack_from_lit_index */

/**
 * Allocate new ecma-string and fill it with reference to string literal
 *
 * @return pointer to ecma-string descriptor
 */
ecma_string_t*
ecma_new_ecma_string_from_lit_index (literal_index_t lit_index) /**< index in the literal table */
{
  ecma_string_t* string_desc_p = ecma_alloc_string ();

  ecma_init_ecma_string_from_lit_index (string_desc_p, lit_index, false);

  return string_desc_p;
} /* ecma_new_ecma_string_from_lit_index */

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

  int64_t length = (int64_t)ecma_string_get_length (string1_p) + (int64_t) ecma_string_get_length (string2_p);

  ecma_string_t* string_desc_p = ecma_alloc_string ();
  string_desc_p->refs = 1;
  string_desc_p->is_stack_var = false;
  string_desc_p->container = ECMA_STRING_CONTAINER_CONCATENATION;

  if (length > ECMA_STRING_MAX_CONCATENATION_LENGTH)
  {
    jerry_exit (ERR_OUT_OF_MEMORY);
  }

  if ((int64_t) ((ecma_length_t) length) != length)
  {
    string_desc_p->length = ECMA_STRING_LENGTH_SHOULD_BE_CALCULATED;
  }
  else
  {
    string_desc_p->length = (ecma_length_t) length;
  }

  string1_p = ecma_copy_or_ref_ecma_string (string1_p);
  string2_p = ecma_copy_or_ref_ecma_string (string2_p);

  ECMA_SET_NON_NULL_POINTER (string_desc_p->u.concatenation.string1_cp, string1_p);
  ECMA_SET_NON_NULL_POINTER (string_desc_p->u.concatenation.string2_cp, string2_p);

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
    case ECMA_STRING_CONTAINER_CHARS_IN_DESC:
    case ECMA_STRING_CONTAINER_LIT_TABLE:
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    {
      new_str_p = ecma_alloc_string ();

      *new_str_p = *string_desc_p;

      new_str_p->refs = 1;
      new_str_p->is_stack_var = false;

      break;
    }

    case ECMA_STRING_CONTAINER_CONCATENATION:
    {
      ecma_string_t *part1_p = ECMA_GET_NON_NULL_POINTER (string_desc_p->u.concatenation.string1_cp);
      ecma_string_t *part2_p = ECMA_GET_NON_NULL_POINTER (string_desc_p->u.concatenation.string2_cp);

      new_str_p = ecma_concat_ecma_strings (part1_p, part2_p);

      break;
    }

    case ECMA_STRING_CONTAINER_HEAP_NUMBER:
    {
      ecma_number_t *num_p = ECMA_GET_NON_NULL_POINTER (string_desc_p->u.number_cp);

      new_str_p = ecma_new_ecma_string_from_number (*num_p);

      break;
    }

    case ECMA_STRING_CONTAINER_HEAP_CHUNKS:
    {
      ecma_collection_chunk_t *str_heap_chunk_p = ECMA_GET_NON_NULL_POINTER (string_desc_p->u.chunk_cp);
      JERRY_ASSERT (str_heap_chunk_p != NULL);

      new_str_p = ecma_alloc_string ();
      *new_str_p = *string_desc_p;

      ecma_collection_chunk_t *new_str_heap_chunk_p = ecma_alloc_collection_chunk ();
      ECMA_SET_NON_NULL_POINTER (new_str_p->u.chunk_cp, new_str_heap_chunk_p);

      while (true)
      {
        __memcpy (new_str_heap_chunk_p->data, str_heap_chunk_p->data, sizeof (new_str_heap_chunk_p->data));

        str_heap_chunk_p = ECMA_GET_POINTER (str_heap_chunk_p->next_chunk_cp);

        if (str_heap_chunk_p != NULL)
        {
          ecma_collection_chunk_t *new_str_next_heap_chunk_p = ecma_alloc_collection_chunk ();

          ECMA_SET_NON_NULL_POINTER (new_str_heap_chunk_p->next_chunk_cp, new_str_next_heap_chunk_p);
          new_str_heap_chunk_p = new_str_next_heap_chunk_p;
        }
        else
        {
          break;
        }
      }

      new_str_heap_chunk_p->next_chunk_cp = ECMA_NULL_POINTER;

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
      ecma_gc_run (ECMA_GC_GEN_COUNT - 1);

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
      ecma_collection_chunk_t *chunk_p = ECMA_GET_NON_NULL_POINTER (string_p->u.chunk_cp);

      JERRY_ASSERT (chunk_p != NULL);

      while (chunk_p != NULL)
      {
        ecma_collection_chunk_t *next_chunk_p = ECMA_GET_POINTER (chunk_p->next_chunk_cp);

        ecma_dealloc_collection_chunk (chunk_p);

        chunk_p = next_chunk_p;
      }

      break;
    }
    case ECMA_STRING_CONTAINER_HEAP_NUMBER:
    {
      ecma_number_t *num_p = ECMA_GET_NON_NULL_POINTER (string_p->u.number_cp);

      ecma_dealloc_number (num_p);

      break;
    }
    case ECMA_STRING_CONTAINER_CONCATENATION:
    {
      ecma_string_t *string1_p, *string2_p;

      string1_p = ECMA_GET_NON_NULL_POINTER (string_p->u.concatenation.string1_cp);
      string2_p = ECMA_GET_NON_NULL_POINTER (string_p->u.concatenation.string2_cp);

      ecma_deref_ecma_string (string1_p);
      ecma_deref_ecma_string (string2_p);

      break;
    }
    case ECMA_STRING_CONTAINER_CHARS_IN_DESC:
    case ECMA_STRING_CONTAINER_LIT_TABLE:
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
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

  ecma_string_container_t container_type = string_p->container;

  JERRY_ASSERT (container_type == ECMA_STRING_CONTAINER_LIT_TABLE ||
                container_type == ECMA_STRING_CONTAINER_MAGIC_STRING ||
                container_type == ECMA_STRING_CONTAINER_UINT32_IN_DESC ||
                container_type == ECMA_STRING_CONTAINER_CHARS_IN_DESC);
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
      ecma_number_t *num_p = ECMA_GET_NON_NULL_POINTER (str_p->u.number_cp);

      return *num_p;
    }

    case ECMA_STRING_CONTAINER_CHARS_IN_DESC:
    case ECMA_STRING_CONTAINER_LIT_TABLE:
    case ECMA_STRING_CONTAINER_HEAP_CHUNKS:
    case ECMA_STRING_CONTAINER_CONCATENATION:
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    {
      ecma_char_t zt_string_buffer [ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER + 1];

      ssize_t bytes_copied = ecma_string_to_zt_string (str_p,
                                                       zt_string_buffer,
                                                       (ssize_t) sizeof (zt_string_buffer));
      if (bytes_copied < 0)
      {
        ecma_char_t *heap_buffer_p = mem_heap_alloc_block ((size_t) -bytes_copied, MEM_HEAP_ALLOC_SHORT_TERM);
        if (heap_buffer_p == NULL)
        {
          jerry_exit (ERR_OUT_OF_MEMORY);
        }

        bytes_copied = ecma_string_to_zt_string (str_p,
                                                 heap_buffer_p,
                                                 -bytes_copied);

        JERRY_ASSERT (bytes_copied > 0);

        ecma_number_t num = ecma_zt_string_to_number (heap_buffer_p);

        mem_heap_free_block (heap_buffer_p);

        return num;
      }
      else
      {
        return ecma_zt_string_to_number (zt_string_buffer);
      }
    }
  }

  JERRY_UNREACHABLE ();
} /* ecma_string_to_number */

/**
 * Copy ecma-string's contents to a buffer.
 *
 * Buffer will contain length of string, in characters, followed by string's characters.
 *
 * @return number of bytes, actually copied to the buffer - if string's content was copied successfully;
 *         otherwise (in case size of buffer is insuficcient) - negative number, which is calculated
 *         as negation of buffer size, that is required to hold the string's content.
 */
ssize_t
ecma_string_to_zt_string (const ecma_string_t *string_desc_p, /**< ecma-string descriptor */
                          ecma_char_t *buffer_p, /**< destination buffer */
                          ssize_t buffer_size) /**< size of buffer */
{
  JERRY_ASSERT (string_desc_p != NULL);
  JERRY_ASSERT (string_desc_p->refs > 0);
  JERRY_ASSERT (buffer_p != NULL);
  JERRY_ASSERT (buffer_size > 0);

  ssize_t required_buffer_size = ((ecma_string_get_length (string_desc_p) + 1) * ((ssize_t) sizeof (ecma_char_t)));
  ssize_t bytes_copied = 0;

  if (required_buffer_size > buffer_size)
  {
    return -required_buffer_size;
  }

  ecma_char_t *dest_p = buffer_p;

  switch ((ecma_string_container_t)string_desc_p->container)
  {
    case ECMA_STRING_CONTAINER_CHARS_IN_DESC:
    {
      ecma_length_t string_length = string_desc_p->length;

      __memcpy (dest_p, string_desc_p->u.chars, string_length * sizeof (ecma_char_t));
      dest_p += string_length;
      *dest_p++ = '\0';
      bytes_copied = (dest_p - buffer_p) * ((ssize_t) sizeof (ecma_char_t));

      break;
    }
    case ECMA_STRING_CONTAINER_HEAP_CHUNKS:
    {
      ecma_length_t string_length = string_desc_p->length;

      ecma_collection_chunk_t *string_chunk_p = ECMA_GET_NON_NULL_POINTER (string_desc_p->u.chunk_cp);

      const ecma_length_t max_chars_in_chunk = sizeof (string_chunk_p->data) / sizeof (ecma_char_t);

      ecma_length_t chars_left = string_length;
      while (chars_left > 0)
      {
        JERRY_ASSERT(chars_left <= string_length);

        ecma_length_t chars_to_copy = JERRY_MIN(max_chars_in_chunk, chars_left);

        __memcpy (dest_p, string_chunk_p->data, chars_to_copy * sizeof (ecma_char_t));
        dest_p += chars_to_copy;
        chars_left = (ecma_length_t) (chars_left - chars_to_copy);

        string_chunk_p = ECMA_GET_POINTER(string_chunk_p->next_chunk_cp);
      }
      *dest_p++ = '\0';
      bytes_copied = (dest_p - buffer_p) * ((ssize_t) sizeof (ecma_char_t));

      break;
    }
    case ECMA_STRING_CONTAINER_LIT_TABLE:
    {
      const literal lit = deserialize_literal_by_id ((idx_t) string_desc_p->u.lit_index);
      JERRY_ASSERT (lit.type == LIT_STR);
      const ecma_char_t *str_p = literal_to_zt (lit);
      JERRY_ASSERT (str_p != NULL);

      ecma_copy_zt_string_to_buffer (str_p, buffer_p, required_buffer_size);
      JERRY_ASSERT (__strlen ((char*)buffer_p) == (size_t) ecma_string_get_length (string_desc_p));

      bytes_copied = (ssize_t) ((ecma_string_get_length (string_desc_p) + 1) * ((ssize_t) sizeof (ecma_char_t)));
      break;
    }
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    {
      uint32_t uint32_number = string_desc_p->u.uint32_number;
      bytes_copied = ecma_uint32_to_string (uint32_number, buffer_p, required_buffer_size);

      break;
    }
    case ECMA_STRING_CONTAINER_HEAP_NUMBER:
    {
      ecma_number_t *num_p = ECMA_GET_NON_NULL_POINTER (string_desc_p->u.number_cp);

      ecma_length_t length = ecma_number_to_zt_string (*num_p, buffer_p, buffer_size);

      JERRY_ASSERT (ecma_string_get_length (string_desc_p) == length);

      bytes_copied = (length + 1) * ((ssize_t) sizeof (ecma_char_t));

      break;
    }
    case ECMA_STRING_CONTAINER_CONCATENATION:
    {
      const ecma_string_t *string1_p = ECMA_GET_NON_NULL_POINTER (string_desc_p->u.concatenation.string1_cp);
      const ecma_string_t *string2_p = ECMA_GET_NON_NULL_POINTER (string_desc_p->u.concatenation.string2_cp);

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

      bytes_copied = bytes_copied1 + bytes_copied2;

      break;
    }
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    {
      const ecma_magic_string_id_t id = string_desc_p->u.magic_string_id;
      const size_t length = ecma_magic_string_lengths [id];

      JERRY_ASSERT (length == (size_t) ecma_string_get_length (string_desc_p));

      size_t bytes_to_copy = (length + 1) * sizeof (ecma_char_t);

      __memcpy (buffer_p, ecma_get_magic_string_zt (id), bytes_to_copy);

      bytes_copied = (ssize_t) bytes_to_copy;
    }
  }

  JERRY_ASSERT (bytes_copied > 0 && bytes_copied <= required_buffer_size);

  return bytes_copied;
} /* ecma_string_to_zt_string */

/**
 * Compare two ecma-strings stored in string chunk chains
 *
 * @return true - if strings are equal;
 *         false - otherwise.
 */
static bool __noinline
ecma_compare_strings_in_heap_chunks (const ecma_string_t *string1_p, /* ecma-string */
                                     const ecma_string_t *string2_p) /* ecma-string */
{
  JERRY_ASSERT (string1_p->container == ECMA_STRING_CONTAINER_HEAP_CHUNKS
                && string2_p->container == ECMA_STRING_CONTAINER_HEAP_CHUNKS);
  JERRY_ASSERT (ecma_string_get_length (string1_p) == ecma_string_get_length (string2_p));

  ecma_collection_chunk_t *string1_chunk_p = ECMA_GET_NON_NULL_POINTER (string1_p->u.chunk_cp);
  ecma_collection_chunk_t *string2_chunk_p = ECMA_GET_NON_NULL_POINTER (string2_p->u.chunk_cp);

  ecma_length_t chars_left = string1_p->length;
  const ecma_length_t max_chars_in_chunk = sizeof (string1_chunk_p->data) / sizeof (ecma_char_t);

  while (chars_left > 0)
  {
    JERRY_ASSERT (string1_chunk_p != NULL);
    JERRY_ASSERT (string2_chunk_p != NULL);

    ecma_length_t chars_to_compare = JERRY_MIN (chars_left, max_chars_in_chunk);
    size_t size_to_compare = chars_to_compare * sizeof (ecma_char_t);

    if (__memcmp (string1_chunk_p->data, string2_chunk_p->data, size_to_compare) != 0)
    {
      return false;
    }

    JERRY_ASSERT (chars_left >= chars_to_compare);
    chars_left = (ecma_length_t) (chars_left - chars_to_compare);

    string1_chunk_p = ECMA_GET_POINTER (string1_chunk_p->next_chunk_cp);
    string2_chunk_p = ECMA_GET_POINTER (string2_chunk_p->next_chunk_cp);
  }

  return true;
} /* ecma_compare_strings_in_heap_chunks */

/**
 * Compare zt-string to ecma-string stored in heap chunks
 *
 * @return true - if strings are equal;
 *         false - otherwise.
 */
static bool __noinline
ecma_compare_ecma_string_to_zt_string (const ecma_string_t *string_p, /**< ecma-string */
                                       const ecma_char_t *zt_string_p) /**< zt-string */
{
  JERRY_ASSERT (string_p->container == ECMA_STRING_CONTAINER_HEAP_CHUNKS);

  ecma_collection_chunk_t *string_chunk_p = ECMA_GET_NON_NULL_POINTER (string_p->u.chunk_cp);

  ecma_length_t chars_left = string_p->length;
  const ecma_length_t max_chars_in_chunk = sizeof (string_chunk_p->data) / sizeof (ecma_char_t);

  const ecma_char_t *zt_string_iter_p = zt_string_p;
  const ecma_char_t *string_chunk_iter_p = string_chunk_p->data;
  const ecma_char_t *string_chunk_end_p = string_chunk_iter_p + max_chars_in_chunk;

  while (chars_left > 0)
  {
    JERRY_ASSERT (string_chunk_p != NULL);

    if (unlikely (string_chunk_iter_p == string_chunk_end_p))
    {
      string_chunk_p = ECMA_GET_POINTER (string_chunk_p->next_chunk_cp);
      string_chunk_iter_p = string_chunk_p->data;
      string_chunk_end_p = string_chunk_iter_p + max_chars_in_chunk;
    }

    if (*string_chunk_iter_p++ != *zt_string_iter_p++)
    {
      return false;
    }

    JERRY_ASSERT (chars_left >= 1);
    chars_left = (ecma_length_t) (chars_left - 1);
  }

  return true;
} /* ecma_compare_ecma_string_to_zt_string */

/**
 * Long path part of ecma-string to ecma-string comparison routine
 *
 * See also:
 *          ecma_compare_ecma_strings
 *
 * @return true - if strings are equal;
 *         false - otherwise.
 */
static bool __noinline
ecma_compare_ecma_strings_longpath (const ecma_string_t *string1_p, /* ecma-string */
                                    const ecma_string_t *string2_p) /* ecma-string */
{
  if (likely (ecma_string_get_length (string1_p) != ecma_string_get_length (string2_p)))
  {
    return false;
  }

  if (string1_p->container == string2_p->container)
  {
    switch ((ecma_string_container_t) string1_p->container)
    {
      case ECMA_STRING_CONTAINER_HEAP_NUMBER:
      {
        ecma_number_t *num1_p, *num2_p;
        num1_p = ECMA_GET_NON_NULL_POINTER (string1_p->u.number_cp);
        num2_p = ECMA_GET_NON_NULL_POINTER (string2_p->u.number_cp);

        if (ecma_number_is_nan (*num1_p)
            && ecma_number_is_nan (*num2_p))
        {
          return true;
        }

        return (*num1_p == *num2_p);
      }
      case ECMA_STRING_CONTAINER_CHARS_IN_DESC:
      {
        JERRY_ASSERT (ecma_string_get_length (string1_p) == ecma_string_get_length (string2_p));

        return (__memcmp (string1_p->u.chars,
                          string2_p->u.chars,
                          (size_t) (ecma_string_get_length (string1_p) * ((ssize_t) sizeof (ecma_char_t)))) == 0);
      }
      case ECMA_STRING_CONTAINER_HEAP_CHUNKS:
      {
        return ecma_compare_strings_in_heap_chunks (string1_p, string2_p);
      }
      case ECMA_STRING_CONTAINER_CONCATENATION:
      {
        /* long path */
        break;
      }
      case ECMA_STRING_CONTAINER_LIT_TABLE:
      case ECMA_STRING_CONTAINER_MAGIC_STRING:
      case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
      {
        JERRY_UNREACHABLE ();
      }
    }
  }

  if (string1_p->container == ECMA_STRING_CONTAINER_HEAP_CHUNKS)
  {
    const ecma_string_t *tmp_string_p = string1_p;
    string1_p = string2_p;
    string2_p = tmp_string_p;
  }
  JERRY_ASSERT (string1_p->container != ECMA_STRING_CONTAINER_HEAP_CHUNKS);

  const ecma_char_t *zt_string1_p;
  bool is_zt_string1_on_heap = false;
  ecma_char_t zt_string1_buffer [ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER + 1];

  if (string1_p->container == ECMA_STRING_CONTAINER_LIT_TABLE)
  {
    FIXME (uint8_t -> literal_index_t);
    const literal lit = deserialize_literal_by_id ((uint8_t) string1_p->u.lit_index);
    JERRY_ASSERT (lit.type == LIT_STR);
    zt_string1_p = literal_to_zt (lit);
  }
  else
  {
    ssize_t req_size = ecma_string_to_zt_string (string1_p,
                                                 zt_string1_buffer,
                                                 sizeof (zt_string1_buffer));

    if (req_size < 0)
    {
      ecma_char_t *heap_buffer_p = mem_heap_alloc_block ((size_t) -req_size, MEM_HEAP_ALLOC_SHORT_TERM);
      if (heap_buffer_p == NULL)
      {
        jerry_exit (ERR_OUT_OF_MEMORY);
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
  }

  bool is_equal;

  if (string2_p->container == ECMA_STRING_CONTAINER_HEAP_CHUNKS)
  {
    is_equal = ecma_compare_ecma_string_to_zt_string (string2_p, zt_string1_p);
  }
  else
  {
    const ecma_char_t *zt_string2_p;
    bool is_zt_string2_on_heap = false;
    ecma_char_t zt_string2_buffer [ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER + 1];

    if (string2_p->container == ECMA_STRING_CONTAINER_LIT_TABLE)
    {
      FIXME (uint8_t -> literal_index_t);
      const literal lit = deserialize_literal_by_id ((uint8_t) string2_p->u.lit_index);
      JERRY_ASSERT (lit.type == LIT_STR);
      zt_string2_p = literal_to_zt (lit);
    }
    else
    {
      ssize_t req_size = ecma_string_to_zt_string (string2_p,
                                                   zt_string2_buffer,
                                                   sizeof (zt_string2_buffer));

      if (req_size < 0)
      {
        ecma_char_t *heap_buffer_p = mem_heap_alloc_block ((size_t) -req_size, MEM_HEAP_ALLOC_SHORT_TERM);
        if (heap_buffer_p == NULL)
        {
          jerry_exit (ERR_OUT_OF_MEMORY);
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
    }

    is_equal = ecma_compare_zt_strings (zt_string1_p, zt_string2_p);

    if (is_zt_string2_on_heap)
    {
      mem_heap_free_block ((void*) zt_string2_p);
    }
  }

  if (is_zt_string1_on_heap)
  {
    mem_heap_free_block ((void*) zt_string1_p);
  }

  return is_equal;
} /* ecma_compare_ecma_strings_longpath */

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

  if (unlikely (string1_p == string2_p))
  {
    return true;
  }

  if (string1_p->container == string2_p->container)
  {
    if (likely (string1_p->container == ECMA_STRING_CONTAINER_LIT_TABLE))
    {
      return (string1_p->u.lit_index == string2_p->u.lit_index);
    }
    else if (likely (string1_p->container == ECMA_STRING_CONTAINER_MAGIC_STRING))
    {
      return (string1_p->u.magic_string_id == string2_p->u.magic_string_id);
    }
    else if (string1_p->container == ECMA_STRING_CONTAINER_UINT32_IN_DESC)
    {
      return (string1_p->u.uint32_number == string2_p->u.uint32_number);
    }
  }

  return ecma_compare_ecma_strings_longpath (string1_p, string2_p);
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
  ecma_char_t zt_string1_buffer [ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER + 1];
  ecma_char_t zt_string2_buffer [ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER + 1];

  if (string1_p->container == ECMA_STRING_CONTAINER_LIT_TABLE)
  {
    FIXME (uint8_t -> literal_index_t);
    const literal lit = deserialize_literal_by_id ((uint8_t) string1_p->u.lit_index);
    JERRY_ASSERT (lit.type == LIT_STR);
    zt_string1_p = literal_to_zt (lit);
  }
  else
  {
    ssize_t req_size = ecma_string_to_zt_string (string1_p,
                                                 zt_string1_buffer,
                                                 sizeof (zt_string1_buffer));

    if (req_size < 0)
    {
      ecma_char_t *heap_buffer_p = mem_heap_alloc_block ((size_t) -req_size, MEM_HEAP_ALLOC_SHORT_TERM);
      if (heap_buffer_p == NULL)
      {
        jerry_exit (ERR_OUT_OF_MEMORY);
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
  }

  if (string2_p->container == ECMA_STRING_CONTAINER_LIT_TABLE)
  {
    FIXME (uint8_t -> literal_index_t);
    const literal lit = deserialize_literal_by_id ((uint8_t) string2_p->u.lit_index);
    JERRY_ASSERT (lit.type == LIT_STR);
    zt_string2_p = literal_to_zt (lit);
  }
  else
  {
    ssize_t req_size = ecma_string_to_zt_string (string2_p,
                                                 zt_string2_buffer,
                                                 sizeof (zt_string2_buffer));

    if (req_size < 0)
    {
      ecma_char_t *heap_buffer_p = mem_heap_alloc_block ((size_t) -req_size, MEM_HEAP_ALLOC_SHORT_TERM);
      if (heap_buffer_p == NULL)
      {
        jerry_exit (ERR_OUT_OF_MEMORY);
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
  if (likely (string_p->length != ECMA_STRING_LENGTH_SHOULD_BE_CALCULATED))
  {
    return string_p->length;
  }
  else if (string_p->container != ECMA_STRING_CONTAINER_CONCATENATION)
  {
    return string_p->length;
  }
  else
  {
    const ecma_string_t *string1_p, *string2_p;

    string1_p = ECMA_GET_NON_NULL_POINTER (string_p->u.concatenation.string1_cp);
    string2_p = ECMA_GET_NON_NULL_POINTER (string_p->u.concatenation.string2_cp);

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
  ecma_char_t *zt_str_p = mem_heap_alloc_block (buffer_size,
                                                MEM_HEAP_ALLOC_SHORT_TERM);

  if (zt_str_p == NULL)
  {
    jerry_exit (ERR_OUT_OF_MEMORY);
  }

  ecma_string_to_zt_string (string_p, zt_str_p, (ssize_t) buffer_size);

  ecma_char_t ch = zt_str_p [index];

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
  TODO(Support UTF-16);

  switch (id)
  {
#define ECMA_MAGIC_STRING_DEF(id, ascii_zt_string) \
     case id: return (ecma_char_t*) ascii_zt_string;
#include "ecma-magic-strings.inc.h"
#undef ECMA_MAGIC_STRING_DEF

    case ECMA_MAGIC_STRING__COUNT: break;
  }

  JERRY_UNREACHABLE();
} /* ecma_get_magic_string_zt */

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

  for (ecma_magic_string_id_t id = 0;
       id < ECMA_MAGIC_STRING__COUNT;
       id++)
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
  JERRY_ASSERT (ecma_string_get_length (string_p) <= ecma_magic_string_max_length);
  ecma_char_t zt_string_buffer [ecma_magic_string_max_length + 1];

  ssize_t copied = ecma_string_to_zt_string (string_p, zt_string_buffer, (ssize_t) sizeof (zt_string_buffer));
  JERRY_ASSERT (copied > 0);

  return ecma_is_zt_string_magic (zt_string_buffer, out_id_p);
} /* ecma_is_string_magic_longpath */

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
           && ecma_string_get_length (string_p) <= ecma_magic_string_max_length)
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
    JERRY_ASSERT (ecma_string_get_length (string_p) > ecma_magic_string_max_length
                  || !ecma_is_string_magic_longpath (string_p, out_id_p));

    return false;
  }
} /* ecma_is_string_magic */

/**
 * Try to calculate hash of the ecma-string
 *
 * Note:
 *      Not all ecma-string containers provide ability to calculate hash.
 *      For now, only strings stored in literal table support calculating of hash.
 *
 * Warning:
 *      Anyway, if ecma-string is hashable, for given length of hash,
 *      the hash values should always be equal for equal strings
 *      irrespective of the strings' container type.
 *
 *      If a ecma-string stored in a container of some type is hashable,
 *      then the ecma-string should always be hashable if stored in container
 *      of the same type.
 *
 * @return true - if hash was calculated,
 *         false - otherwise (the string is not hashable).
 */
bool
ecma_string_try_hash (const ecma_string_t *string_p, /**< ecma-string to calculate hash for */
                      uint32_t hash_length_bits, /**< length of hash value, in bits */
                      uint32_t *out_hash_p) /**< out: hash value, if calculated */

{
  JERRY_ASSERT (hash_length_bits < sizeof (uint32_t) * JERRY_BITSINBYTE);
  uint32_t hash_mask = ((1u << hash_length_bits) - 1);

  if (string_p->container == ECMA_STRING_CONTAINER_LIT_TABLE)
  {
    *out_hash_p = (string_p->u.lit_index) & hash_mask;

    return true;
  }

  return false;
} /* ecma_string_try_hash */

/**
 * @}
 * @}
 */
