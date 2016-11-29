/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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

#include "ecma-alloc.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lcache.h"
#include "jrt.h"
#include "jrt-libc-includes.h"
#include "lit-char-helpers.h"
#include "lit-magic-strings.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

JERRY_STATIC_ASSERT (ECMA_STRING_CONTAINER_MASK + 1 == ECMA_STRING_REF_ONE,
                     ecma_string_ref_counter_should_start_after_the_container_field);

JERRY_STATIC_ASSERT (ECMA_STRING_CONTAINER_MASK >= ECMA_STRING_CONTAINER__MAX,
                     ecma_string_container_types_must_be_lower_than_the_container_mask);

JERRY_STATIC_ASSERT ((ECMA_STRING_MAX_REF | ECMA_STRING_CONTAINER_MASK) == UINT16_MAX,
                     ecma_string_ref_and_container_fields_should_fill_the_16_bit_field);

JERRY_STATIC_ASSERT (ECMA_STRING_NOT_ARRAY_INDEX == UINT32_MAX,
                     ecma_string_not_array_index_must_be_equal_to_uint32_max);

static void
ecma_init_ecma_string_from_magic_string_id (ecma_string_t *string_p,
                                            lit_magic_string_id_t magic_string_id);

static void
ecma_init_ecma_string_from_magic_string_ex_id (ecma_string_t *string_p,
                                               lit_magic_string_ex_id_t magic_string_ex_id);

/**
 * Initialize ecma-string descriptor with specified magic string
 */
static void
ecma_init_ecma_string_from_magic_string_id (ecma_string_t *string_p, /**< descriptor to initialize */
                                            lit_magic_string_id_t magic_string_id) /**< identifier of
                                                                                         the magic string */
{
  string_p->refs_and_container = ECMA_STRING_CONTAINER_MAGIC_STRING | ECMA_STRING_REF_ONE;
  string_p->hash = (lit_string_hash_t) magic_string_id;

  string_p->u.magic_string_id = magic_string_id;
} /* ecma_init_ecma_string_from_magic_string_id */

/**
 * Initialize external ecma-string descriptor with specified magic string
 */
static void
ecma_init_ecma_string_from_magic_string_ex_id (ecma_string_t *string_p, /**< descriptor to initialize */
                                               lit_magic_string_ex_id_t magic_string_ex_id) /**< identifier of
                                                                                           the external magic string */
{
  string_p->refs_and_container = ECMA_STRING_CONTAINER_MAGIC_STRING_EX | ECMA_STRING_REF_ONE;
  string_p->hash = (lit_string_hash_t) (LIT_MAGIC_STRING__COUNT + magic_string_ex_id);

  string_p->u.magic_string_ex_id = magic_string_ex_id;
} /* ecma_init_ecma_string_from_magic_string_ex_id */

/**
 * Convert a string to an unsigned 32 bit value if possible
 *
 * @return true if the conversion is successful
 *         false otherwise
 */
static bool
ecma_string_to_array_index (const lit_utf8_byte_t *string_p, /**< utf-8 string */
                            lit_utf8_size_t string_size, /**< string size */
                            uint32_t *result_p) /**< [out] converted value */
{
  JERRY_ASSERT (string_size > 0 && *string_p >= LIT_CHAR_0 && *string_p <= LIT_CHAR_9);

  if (*string_p == LIT_CHAR_0)
  {
    *result_p = 0;
    return (string_size == 1);
  }

  if (string_size > ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32)
  {
    return false;
  }

  uint32_t index = 0;
  const lit_utf8_byte_t *string_end_p = string_p + string_size;

  if (string_size == ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32)
  {
    string_end_p--;
  }

  do
  {
    if (*string_p > LIT_CHAR_9 || *string_p < LIT_CHAR_0)
    {
      return false;
    }

    index = (index * 10) + (uint32_t) (*string_p++ - LIT_CHAR_0);
  }
  while (string_p < string_end_p);

  if (string_size < ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32)
  {
    *result_p = index;
    return true;
  }

  /* Overflow must be checked as well when size is
   * equal to ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32. */
  if (*string_p > LIT_CHAR_9
      || *string_p < LIT_CHAR_0
      || index > (UINT32_MAX / 10)
      || (index == (UINT32_MAX / 10) && *string_p > LIT_CHAR_5))
  {
    return false;
  }

  *result_p = (index * 10) + (uint32_t) (*string_p - LIT_CHAR_0);
  return true;
} /* ecma_string_to_array_index */

/**
 * Allocate new ecma-string and fill it with characters from the utf8 string
 *
 * @return pointer to ecma-string descriptor
 */
ecma_string_t *
ecma_new_ecma_string_from_utf8 (const lit_utf8_byte_t *string_p, /**< utf-8 string */
                                lit_utf8_size_t string_size) /**< string size */
{
  JERRY_ASSERT (string_p != NULL || string_size == 0);
  JERRY_ASSERT (lit_is_cesu8_string_valid (string_p, string_size));

  lit_magic_string_id_t magic_string_id = lit_is_utf8_string_magic (string_p, string_size);

  if (magic_string_id != LIT_MAGIC_STRING__COUNT)
  {
    return ecma_get_magic_string (magic_string_id);
  }

  JERRY_ASSERT (string_size > 0);

  if (*string_p >= LIT_CHAR_0 && *string_p <= LIT_CHAR_9)
  {
    uint32_t array_index;

    if (ecma_string_to_array_index (string_p, string_size, &array_index))
    {
      return ecma_new_ecma_string_from_uint32 (array_index);
    }
  }

  if (lit_get_magic_string_ex_count () > 0)
  {
    lit_magic_string_ex_id_t magic_string_ex_id = lit_is_ex_utf8_string_magic (string_p, string_size);

    if (magic_string_ex_id < lit_get_magic_string_ex_count ())
    {
      return ecma_get_magic_string_ex (magic_string_ex_id);
    }
  }

  ecma_string_t *string_desc_p;
  lit_utf8_byte_t *data_p;

  if (likely (string_size <= UINT16_MAX))
  {
    string_desc_p = jmem_heap_alloc_block (sizeof (ecma_string_t) + string_size);

    string_desc_p->refs_and_container = ECMA_STRING_CONTAINER_HEAP_UTF8_STRING | ECMA_STRING_REF_ONE;
    string_desc_p->u.common_uint32_field = 0;
    string_desc_p->u.utf8_string.size = (uint16_t) string_size;
    string_desc_p->u.utf8_string.length = (uint16_t) lit_utf8_string_length (string_p, string_size);

    data_p = (lit_utf8_byte_t *) (string_desc_p + 1);
  }
  else
  {
    string_desc_p = jmem_heap_alloc_block (sizeof (ecma_long_string_t) + string_size);

    string_desc_p->refs_and_container = ECMA_STRING_CONTAINER_HEAP_LONG_UTF8_STRING | ECMA_STRING_REF_ONE;
    string_desc_p->u.common_uint32_field = 0;
    string_desc_p->u.long_utf8_string_size = string_size;

    ecma_long_string_t *long_string_desc_p = (ecma_long_string_t *) string_desc_p;
    long_string_desc_p->long_utf8_string_length = lit_utf8_string_length (string_p, string_size);

    data_p = (lit_utf8_byte_t *) (long_string_desc_p + 1);
  }

  string_desc_p->hash = lit_utf8_string_calc_hash (string_p, string_size);
  memcpy (data_p, string_p, string_size);
  return string_desc_p;
} /* ecma_new_ecma_string_from_utf8 */

/**
 * Allocate a new ecma-string and initialize it from the utf8 string argument.
 * All 4-bytes long unicode sequences are converted into two 3-bytes long sequences.
 *
 * @return pointer to ecma-string descriptor
 */
ecma_string_t *
ecma_new_ecma_string_from_utf8_converted_to_cesu8 (const lit_utf8_byte_t *string_p, /**< utf-8 string */
                                                   lit_utf8_size_t string_size) /**< utf-8 string size */
{
  JERRY_ASSERT (string_p != NULL || string_size == 0);

  ecma_string_t *string_desc_p = NULL;

  ecma_length_t converted_string_length = 0;
  lit_utf8_size_t converted_string_size = 0;
  lit_utf8_size_t pos = 0;

  /* Calculate the required length and size information of the converted cesu-8 encoded string */
  while (pos < string_size)
  {
    if ((string_p[pos] & LIT_UTF8_1_BYTE_MASK) == LIT_UTF8_1_BYTE_MARKER)
    {
      pos++;
    }
    else if ((string_p[pos] & LIT_UTF8_2_BYTE_MASK) == LIT_UTF8_2_BYTE_MARKER)
    {
      pos += 2;
    }
    else if ((string_p[pos] & LIT_UTF8_3_BYTE_MASK) == LIT_UTF8_3_BYTE_MARKER)
    {
      pos += 3;
    }
    else
    {
      JERRY_ASSERT ((string_p[pos] & LIT_UTF8_4_BYTE_MASK) == LIT_UTF8_4_BYTE_MARKER);
      pos += 4;
      converted_string_size += 2;
      converted_string_length++;
    }

    converted_string_length++;
  }

  JERRY_ASSERT (pos == string_size);

  if (converted_string_size == 0)
  {
    return ecma_new_ecma_string_from_utf8 (string_p, string_size);
  }
  else
  {
    converted_string_size += string_size;

    JERRY_ASSERT (lit_is_utf8_string_valid (string_p, string_size));

    lit_utf8_byte_t *data_p;

    if (likely (string_size <= UINT16_MAX))
    {
      string_desc_p = jmem_heap_alloc_block (sizeof (ecma_string_t) + converted_string_size);

      string_desc_p->refs_and_container = ECMA_STRING_CONTAINER_HEAP_UTF8_STRING | ECMA_STRING_REF_ONE;
      string_desc_p->u.common_uint32_field = 0;
      string_desc_p->u.utf8_string.size = (uint16_t) converted_string_size;
      string_desc_p->u.utf8_string.length = (uint16_t) converted_string_length;

      data_p = (lit_utf8_byte_t *) (string_desc_p + 1);
    }
    else
    {
      string_desc_p = jmem_heap_alloc_block (sizeof (ecma_long_string_t) + converted_string_size);

      string_desc_p->refs_and_container = ECMA_STRING_CONTAINER_HEAP_LONG_UTF8_STRING | ECMA_STRING_REF_ONE;
      string_desc_p->u.common_uint32_field = 0;
      string_desc_p->u.long_utf8_string_size = converted_string_size;

      ecma_long_string_t *long_string_desc_p = (ecma_long_string_t *) string_desc_p;
      long_string_desc_p->long_utf8_string_length = converted_string_length;

      data_p = (lit_utf8_byte_t *) (long_string_desc_p + 1);
    }

    pos = 0;

    while (pos < string_size)
    {
      if ((string_p[pos] & LIT_UTF8_4_BYTE_MASK) == LIT_UTF8_4_BYTE_MARKER)
      {
        /* Processing 4 byte unicode sequence. Always converted to two 3 byte long sequence. */
        uint32_t character = ((((uint32_t) string_p[pos++]) & 0x7) << 18);
        character |= ((((uint32_t) string_p[pos++]) & LIT_UTF8_LAST_6_BITS_MASK) << 12);
        character |= ((((uint32_t) string_p[pos++]) & LIT_UTF8_LAST_6_BITS_MASK) << 6);
        character |= (((uint32_t) string_p[pos++]) & LIT_UTF8_LAST_6_BITS_MASK);

        JERRY_ASSERT (character >= 0x10000);
        character -= 0x10000;

        data_p += lit_char_to_utf8_bytes (data_p, (ecma_char_t) (0xd800 | (character >> 10)));
        data_p += lit_char_to_utf8_bytes (data_p, (ecma_char_t) (0xdc00 | (character & LIT_UTF16_LAST_10_BITS_MASK)));
      }
      else
      {
        *data_p++ = string_p[pos++];
      }
    }

    JERRY_ASSERT (pos == string_size);

    string_desc_p->hash = lit_utf8_string_calc_hash (data_p, converted_string_size);
  }

  return string_desc_p;
} /* ecma_new_ecma_string_from_utf8_converted_to_cesu8 */

/**
 * Allocate new ecma-string and fill it with cesu-8 character which represents specified code unit
 *
 * @return pointer to ecma-string descriptor
 */
ecma_string_t *
ecma_new_ecma_string_from_code_unit (ecma_char_t code_unit) /**< code unit */
{
  lit_utf8_byte_t lit_utf8_bytes[LIT_UTF8_MAX_BYTES_IN_CODE_UNIT];
  lit_utf8_size_t bytes_size = lit_code_unit_to_utf8 (code_unit, lit_utf8_bytes);

  return ecma_new_ecma_string_from_utf8 (lit_utf8_bytes, bytes_size);
} /* ecma_new_ecma_string_from_code_unit */

/**
 * Initialize an ecma-string with an ecma-number
 */
inline void __attr_always_inline___
ecma_init_ecma_string_from_uint32 (ecma_string_t *string_desc_p, /**< ecma-string */
                                   uint32_t uint32_number) /**< uint32 value of the string */
{
  string_desc_p->refs_and_container = ECMA_STRING_CONTAINER_UINT32_IN_DESC | ECMA_STRING_REF_ONE;
  string_desc_p->hash = (lit_string_hash_t) uint32_number;

  string_desc_p->u.uint32_number = uint32_number;
} /* ecma_init_ecma_string_from_uint32 */

/**
 * Initialize a length ecma-string
 */
inline void __attr_always_inline___
ecma_init_ecma_length_string (ecma_string_t *string_desc_p) /**< ecma-string */
{
  string_desc_p->refs_and_container = ECMA_STRING_CONTAINER_MAGIC_STRING | ECMA_STRING_REF_ONE;
  string_desc_p->hash = LIT_MAGIC_STRING_LENGTH;

  string_desc_p->u.magic_string_id = LIT_MAGIC_STRING_LENGTH;
} /* ecma_init_ecma_length_string */

/**
 * Allocate new ecma-string and fill it with ecma-number
 *
 * @return pointer to ecma-string descriptor
 */
ecma_string_t *
ecma_new_ecma_string_from_uint32 (uint32_t uint32_number) /**< uint32 value of the string */
{
  ecma_string_t *string_desc_p = ecma_alloc_string ();

  ecma_init_ecma_string_from_uint32 (string_desc_p, uint32_number);
  return string_desc_p;
} /* ecma_new_ecma_string_from_uint32 */

/**
 * Allocate new ecma-string and fill it with ecma-number
 *
 * @return pointer to ecma-string descriptor
 */
ecma_string_t *
ecma_new_ecma_string_from_number (ecma_number_t num) /**< ecma-number */
{
  uint32_t uint32_num = ecma_number_to_uint32 (num);
  if (num == ((ecma_number_t) uint32_num))
  {
    return ecma_new_ecma_string_from_uint32 (uint32_num);
  }

  if (ecma_number_is_nan (num))
  {
    return ecma_get_magic_string (LIT_MAGIC_STRING_NAN);
  }

  if (ecma_number_is_infinity (num))
  {
    lit_magic_string_id_t id = (ecma_number_is_negative (num) ? LIT_MAGIC_STRING_NEGATIVE_INFINITY_UL
                                                              : LIT_MAGIC_STRING_INFINITY_UL);
    return ecma_get_magic_string (id);
  }

  lit_utf8_byte_t str_buf[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER];
  lit_utf8_size_t str_size = ecma_number_to_utf8_string (num, str_buf, sizeof (str_buf));

  JERRY_ASSERT (str_size > 0);
#ifndef JERRY_NDEBUG
  JERRY_ASSERT (lit_is_utf8_string_magic (str_buf, str_size) == LIT_MAGIC_STRING__COUNT
                && lit_is_ex_utf8_string_magic (str_buf, str_size) == lit_get_magic_string_ex_count ());
#endif /* !JERRY_NDEBUG */

  ecma_string_t *string_desc_p = jmem_heap_alloc_block (sizeof (ecma_string_t) + str_size);

  string_desc_p->refs_and_container = ECMA_STRING_CONTAINER_HEAP_UTF8_STRING | ECMA_STRING_REF_ONE;
  string_desc_p->hash = lit_utf8_string_calc_hash (str_buf, str_size);
  string_desc_p->u.common_uint32_field = 0;
  string_desc_p->u.utf8_string.size = (uint16_t) str_size;
  string_desc_p->u.utf8_string.length = (uint16_t) str_size;

  lit_utf8_byte_t *data_p = (lit_utf8_byte_t *) (string_desc_p + 1);
  memcpy (data_p, str_buf, str_size);
  return string_desc_p;
} /* ecma_new_ecma_string_from_number */

/**
 * Allocate new ecma-string and fill it with reference to ECMA magic string
 *
 * @return pointer to ecma-string descriptor
 */
ecma_string_t *
ecma_new_ecma_string_from_magic_string_id (lit_magic_string_id_t id) /**< identifier of magic string */
{
  JERRY_ASSERT (id < LIT_MAGIC_STRING__COUNT);

  ecma_string_t *string_desc_p = ecma_alloc_string ();
  ecma_init_ecma_string_from_magic_string_id (string_desc_p, id);

  return string_desc_p;
} /* ecma_new_ecma_string_from_magic_string_id */

/**
 * Allocate new ecma-string and fill it with reference to ECMA magic string
 *
 * @return pointer to ecma-string descriptor
 */
ecma_string_t *
ecma_new_ecma_string_from_magic_string_ex_id (lit_magic_string_ex_id_t id) /**< identifier of externl magic string */
{
  JERRY_ASSERT (id < lit_get_magic_string_ex_count ());

  ecma_string_t *string_desc_p = ecma_alloc_string ();
  ecma_init_ecma_string_from_magic_string_ex_id (string_desc_p, id);

  return string_desc_p;
} /* ecma_new_ecma_string_from_magic_string_ex_id */

/**
 * Allocate new ecma-string and fill it with reference to length magic string
 *
 * @return pointer to ecma-string descriptor
 */
ecma_string_t *
ecma_new_ecma_length_string (void)
{
  ecma_string_t *string_desc_p = ecma_alloc_string ();
  ecma_init_ecma_length_string (string_desc_p);

  return string_desc_p;
} /* ecma_new_ecma_length_string */

/**
 * Concatenate ecma-strings
 *
 * @return concatenation of two ecma-strings
 */
ecma_string_t *
ecma_concat_ecma_strings (ecma_string_t *string1_p, /**< first ecma-string */
                          ecma_string_t *string2_p) /**< second ecma-string */
{
  JERRY_ASSERT (string1_p != NULL
                && string2_p != NULL);

  if (ecma_string_is_empty (string1_p))
  {
    ecma_ref_ecma_string (string2_p);
    return string2_p;
  }
  else if (ecma_string_is_empty (string2_p))
  {
    ecma_ref_ecma_string (string1_p);
    return string1_p;
  }

  const lit_utf8_byte_t *utf8_string1_p, *utf8_string2_p;
  lit_utf8_size_t utf8_string1_size, utf8_string2_size;
  lit_utf8_size_t utf8_string1_length, utf8_string2_length;

  lit_utf8_byte_t uint32_to_string_buffer1[ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32];
  lit_utf8_byte_t uint32_to_string_buffer2[ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32];

  bool string1_is_uint32 = false;
  bool string1_rehash_needed = false;

  switch (ECMA_STRING_GET_CONTAINER (string1_p))
  {
    case ECMA_STRING_CONTAINER_HEAP_UTF8_STRING:
    {
      utf8_string1_p = (lit_utf8_byte_t *) (string1_p + 1);
      utf8_string1_size = string1_p->u.utf8_string.size;
      utf8_string1_length = string1_p->u.utf8_string.length;
      break;
    }
    case ECMA_STRING_CONTAINER_HEAP_LONG_UTF8_STRING:
    {
      ecma_long_string_t *long_string_desc_p = (ecma_long_string_t *) string1_p;

      utf8_string1_p = (lit_utf8_byte_t *) (long_string_desc_p + 1);
      utf8_string1_size = string1_p->u.long_utf8_string_size;
      utf8_string1_length = long_string_desc_p->long_utf8_string_length;
      break;
    }
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    {
      utf8_string1_size = ecma_uint32_to_utf8_string (string1_p->u.uint32_number,
                                                      uint32_to_string_buffer1,
                                                      ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32);
      utf8_string1_p = uint32_to_string_buffer1;
      utf8_string1_length = utf8_string1_size;
      string1_is_uint32 = true;
      string1_rehash_needed = true;
      break;
    }
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    {
      utf8_string1_p = lit_get_magic_string_utf8 (string1_p->u.magic_string_id);
      utf8_string1_size = lit_get_magic_string_size (string1_p->u.magic_string_id);
      utf8_string1_length = utf8_string1_size;
      string1_rehash_needed = true;
      break;
    }
    default:
    {
      JERRY_ASSERT (ECMA_STRING_GET_CONTAINER (string1_p) == ECMA_STRING_CONTAINER_MAGIC_STRING_EX);

      utf8_string1_p = lit_get_magic_string_ex_utf8 (string1_p->u.magic_string_id);
      utf8_string1_size = lit_get_magic_string_ex_size (string1_p->u.magic_string_id);
      utf8_string1_length = utf8_string1_size;
      string1_rehash_needed = true;
      break;
    }
  }

  switch (ECMA_STRING_GET_CONTAINER (string2_p))
  {
    case ECMA_STRING_CONTAINER_HEAP_UTF8_STRING:
    {
      utf8_string2_p = (lit_utf8_byte_t *) (string2_p + 1);
      utf8_string2_size = string2_p->u.utf8_string.size;
      utf8_string2_length = string2_p->u.utf8_string.length;
      break;
    }
    case ECMA_STRING_CONTAINER_HEAP_LONG_UTF8_STRING:
    {
      ecma_long_string_t *long_string_desc_p = (ecma_long_string_t *) string2_p;

      utf8_string2_p = (lit_utf8_byte_t *) (long_string_desc_p + 1);
      utf8_string2_size = string2_p->u.long_utf8_string_size;
      utf8_string2_length = long_string_desc_p->long_utf8_string_length;
      break;
    }
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    {
      utf8_string2_size = ecma_uint32_to_utf8_string (string2_p->u.uint32_number,
                                                      uint32_to_string_buffer2,
                                                      ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32);
      utf8_string2_p = uint32_to_string_buffer2;
      utf8_string2_length = utf8_string2_size;
      break;
    }
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    {
      utf8_string2_p = lit_get_magic_string_utf8 (string2_p->u.magic_string_id);
      utf8_string2_size = lit_get_magic_string_size (string2_p->u.magic_string_id);
      utf8_string2_length = utf8_string2_size;
      break;
    }
    default:
    {
      JERRY_ASSERT (ECMA_STRING_GET_CONTAINER (string2_p) == ECMA_STRING_CONTAINER_MAGIC_STRING_EX);

      utf8_string2_p = lit_get_magic_string_ex_utf8 (string2_p->u.magic_string_id);
      utf8_string2_size = lit_get_magic_string_ex_size (string2_p->u.magic_string_id);
      utf8_string2_length = utf8_string2_size;
      break;
    }
  }

  JERRY_ASSERT (utf8_string1_size > 0);
  JERRY_ASSERT (utf8_string2_size > 0);
  JERRY_ASSERT (utf8_string1_length <= utf8_string1_size);
  JERRY_ASSERT (utf8_string2_length <= utf8_string2_size);

  lit_utf8_size_t new_size = utf8_string1_size + utf8_string2_size;

  /* It is impossible to allocate this large string. */
  if (new_size < (utf8_string1_size | utf8_string2_size))
  {
    jerry_fatal (ERR_OUT_OF_MEMORY);
  }

  lit_magic_string_id_t magic_string_id;
  magic_string_id = lit_is_utf8_string_pair_magic (utf8_string1_p,
                                                   utf8_string1_size,
                                                   utf8_string2_p,
                                                   utf8_string2_size);

  if (magic_string_id != LIT_MAGIC_STRING__COUNT)
  {
    return ecma_get_magic_string (magic_string_id);
  }

  if (string1_is_uint32 && new_size <= ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32)
  {
    memcpy (uint32_to_string_buffer1 + utf8_string1_size,
            utf8_string2_p,
            utf8_string2_size);

    uint32_t array_index;

    if (ecma_string_to_array_index (uint32_to_string_buffer1, new_size, &array_index))
    {
      return ecma_new_ecma_string_from_uint32 (array_index);
    }
  }

  if (lit_get_magic_string_ex_count () > 0)
  {
    lit_magic_string_ex_id_t magic_string_ex_id;
    magic_string_ex_id = lit_is_ex_utf8_string_pair_magic (utf8_string1_p,
                                                           utf8_string1_size,
                                                           utf8_string2_p,
                                                           utf8_string2_size);

    if (magic_string_ex_id < lit_get_magic_string_ex_count ())
    {
      return ecma_get_magic_string_ex (magic_string_ex_id);
    }
  }

  ecma_string_t *string_desc_p;
  lit_utf8_byte_t *data_p;

  if (likely (new_size <= UINT16_MAX))
  {
    string_desc_p = jmem_heap_alloc_block (sizeof (ecma_string_t) + new_size);

    string_desc_p->refs_and_container = ECMA_STRING_CONTAINER_HEAP_UTF8_STRING | ECMA_STRING_REF_ONE;
    string_desc_p->u.common_uint32_field = 0;
    string_desc_p->u.utf8_string.size = (uint16_t) new_size;
    string_desc_p->u.utf8_string.length = (uint16_t) (utf8_string1_length + utf8_string2_length);

    data_p = (lit_utf8_byte_t *) (string_desc_p + 1);
  }
  else
  {
    string_desc_p = jmem_heap_alloc_block (sizeof (ecma_long_string_t) + new_size);

    string_desc_p->refs_and_container = ECMA_STRING_CONTAINER_HEAP_LONG_UTF8_STRING | ECMA_STRING_REF_ONE;
    string_desc_p->u.common_uint32_field = 0;
    string_desc_p->u.long_utf8_string_size = new_size;

    ecma_long_string_t *long_string_desc_p = (ecma_long_string_t *) string_desc_p;
    long_string_desc_p->long_utf8_string_length = utf8_string1_length + utf8_string2_length;

    data_p = (lit_utf8_byte_t *) (long_string_desc_p + 1);
  }

  lit_string_hash_t hash_start = string1_p->hash;

  if (string1_rehash_needed)
  {
    hash_start = lit_utf8_string_calc_hash (utf8_string1_p, utf8_string1_size);
  }

  string_desc_p->hash = lit_utf8_string_hash_combine (hash_start, utf8_string2_p, utf8_string2_size);

  memcpy (data_p, utf8_string1_p, utf8_string1_size);
  memcpy (data_p + utf8_string1_size, utf8_string2_p, utf8_string2_size);
  return string_desc_p;
} /* ecma_concat_ecma_strings */

/**
 * Increase reference counter of ecma-string.
 */
void
ecma_ref_ecma_string (ecma_string_t *string_p) /**< string descriptor */
{
  JERRY_ASSERT (string_p != NULL);
  JERRY_ASSERT (string_p->refs_and_container >= ECMA_STRING_REF_ONE);

  if (likely (string_p->refs_and_container < ECMA_STRING_MAX_REF))
  {
    /* Increase reference counter. */
    string_p->refs_and_container = (uint16_t) (string_p->refs_and_container + ECMA_STRING_REF_ONE);
  }
  else
  {
    jerry_fatal (ERR_REF_COUNT_LIMIT);
  }
} /* ecma_ref_ecma_string */

/**
 * Decrease reference counter and deallocate ecma-string
 * if the counter becomes zero.
 */
void
ecma_deref_ecma_string (ecma_string_t *string_p) /**< ecma-string */
{
  JERRY_ASSERT (string_p != NULL);
  JERRY_ASSERT (string_p->refs_and_container >= ECMA_STRING_REF_ONE);

  /* Decrease reference counter. */
  string_p->refs_and_container = (uint16_t) (string_p->refs_and_container - ECMA_STRING_REF_ONE);

  if (string_p->refs_and_container >= ECMA_STRING_REF_ONE)
  {
    return;
  }

  switch (ECMA_STRING_GET_CONTAINER (string_p))
  {
    case ECMA_STRING_CONTAINER_HEAP_UTF8_STRING:
    {
#ifndef JERRY_NDEBUG
      const lit_utf8_byte_t *chars_p = (const lit_utf8_byte_t *) (string_p + 1);

      if (*chars_p >= LIT_CHAR_0 && *chars_p <= LIT_CHAR_9)
      {
        uint32_t array_index;

        JERRY_ASSERT (!ecma_string_to_array_index (chars_p,
                                                   string_p->u.utf8_string.size,
                                                   &array_index));
      }
#endif /* !JERRY_NDEBUG */

      jmem_heap_free_block (string_p, string_p->u.utf8_string.size + sizeof (ecma_string_t));
      return;
    }
    case ECMA_STRING_CONTAINER_HEAP_LONG_UTF8_STRING:
    {
      JERRY_ASSERT (string_p->u.long_utf8_string_size > UINT16_MAX);

      jmem_heap_free_block (string_p, string_p->u.long_utf8_string_size + sizeof (ecma_long_string_t));
      return;
    }
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    case ECMA_STRING_CONTAINER_MAGIC_STRING_EX:
    {
      /* only the string descriptor itself should be freed */
      break;
    }
    case ECMA_STRING_LITERAL_NUMBER:
    {
      ecma_fast_free_value (string_p->u.lit_number);
      break;
    }
    default:
    {
      JERRY_UNREACHABLE ();
      break;
    }
  }

  ecma_dealloc_string (string_p);
} /* ecma_deref_ecma_string */

/**
 * Convert ecma-string to number
 */
ecma_number_t
ecma_string_to_number (const ecma_string_t *str_p) /**< ecma-string */
{
  JERRY_ASSERT (str_p != NULL);

  switch (ECMA_STRING_GET_CONTAINER (str_p))
  {
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    {
      return ((ecma_number_t) str_p->u.uint32_number);
    }

    case ECMA_STRING_CONTAINER_HEAP_UTF8_STRING:
    case ECMA_STRING_CONTAINER_HEAP_LONG_UTF8_STRING:
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    case ECMA_STRING_CONTAINER_MAGIC_STRING_EX:
    {
      ecma_number_t num;

      ECMA_STRING_TO_UTF8_STRING (str_p, str_buffer_p, str_buffer_size);

      if (str_buffer_size == 0)
      {
        return ECMA_NUMBER_ZERO;
      }

      num = ecma_utf8_string_to_number (str_buffer_p, str_buffer_size);

      ECMA_FINALIZE_UTF8_STRING (str_buffer_p, str_buffer_size);

      return num;
    }

    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* ecma_string_to_number */

/**
 * Check if string is array index.
 *
 * @return ECMA_STRING_NOT_ARRAY_INDEX if string is not array index
 *         the array index otherwise
 */
inline uint32_t __attr_always_inline___
ecma_string_get_array_index (const ecma_string_t *str_p) /**< ecma-string */
{
  const ecma_string_container_t type = ECMA_STRING_GET_CONTAINER (str_p);

  if (type == ECMA_STRING_CONTAINER_UINT32_IN_DESC)
  {
    /* When the uint32_number is equal to the maximum value of 32 bit
     * unsigned integer number, it is also an invalid array index.
     * The comparison to ECMA_STRING_NOT_ARRAY_INDEX will be true
     * in this case. */
    return str_p->u.uint32_number;
  }

  return ECMA_STRING_NOT_ARRAY_INDEX;
} /* ecma_string_get_array_index */

/**
 * Convert ecma-string's contents to a cesu-8 string and put it to the buffer.
 * It is the caller's responsibility to make sure that the string fits in the buffer.
 *
 * @return number of bytes, actually copied to the buffer.
 */
lit_utf8_size_t __attr_return_value_should_be_checked___
ecma_string_copy_to_utf8_buffer (const ecma_string_t *string_desc_p, /**< ecma-string descriptor */
                                 lit_utf8_byte_t *buffer_p, /**< destination buffer pointer
                                                             * (can be NULL if buffer_size == 0) */
                                 lit_utf8_size_t buffer_size) /**< size of buffer */
{
  JERRY_ASSERT (string_desc_p != NULL);
  JERRY_ASSERT (string_desc_p->refs_and_container >= ECMA_STRING_REF_ONE);
  JERRY_ASSERT (buffer_p != NULL || buffer_size == 0);
  JERRY_ASSERT (ecma_string_get_size (string_desc_p) <= buffer_size);

  lit_utf8_size_t size;

  switch (ECMA_STRING_GET_CONTAINER (string_desc_p))
  {
    case ECMA_STRING_CONTAINER_HEAP_UTF8_STRING:
    {
      size = string_desc_p->u.utf8_string.size;
      memcpy (buffer_p, string_desc_p + 1, size);
      break;
    }
    case ECMA_STRING_CONTAINER_HEAP_LONG_UTF8_STRING:
    {
      size = string_desc_p->u.long_utf8_string_size;
      memcpy (buffer_p, ((ecma_long_string_t *) string_desc_p) + 1, size);
      break;
    }
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    {
      const uint32_t uint32_number = string_desc_p->u.uint32_number;
      size = ecma_uint32_to_utf8_string (uint32_number, buffer_p, buffer_size);
      break;
    }
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    {
      const lit_magic_string_id_t id = string_desc_p->u.magic_string_id;
      size = lit_get_magic_string_size (id);
      memcpy (buffer_p, lit_get_magic_string_utf8 (id), size);
      break;
    }
    default:
    {
      JERRY_ASSERT (ECMA_STRING_GET_CONTAINER (string_desc_p) == ECMA_STRING_CONTAINER_MAGIC_STRING_EX);

      const lit_magic_string_ex_id_t id = string_desc_p->u.magic_string_ex_id;
      size = lit_get_magic_string_ex_size (id);
      memcpy (buffer_p, lit_get_magic_string_ex_utf8 (id), size);
      break;
    }
  }

  JERRY_ASSERT (size <= buffer_size);
  return size;
} /* ecma_string_copy_to_utf8_buffer */

/**
 * Convert ecma-string's contents to a cesu-8 string and put it to the buffer.
 * It is the caller's responsibility to make sure that the string fits in the buffer.
 * Check if the size of the string is equal with the size of the buffer.
 */
void __attr_always_inline___
ecma_string_to_utf8_bytes (const ecma_string_t *string_desc_p, /**< ecma-string descriptor */
                           lit_utf8_byte_t *buffer_p, /**< destination buffer pointer
                                                       * (can be NULL if buffer_size == 0) */
                           lit_utf8_size_t buffer_size) /**< size of buffer */
{
  const lit_utf8_size_t size = ecma_string_copy_to_utf8_buffer (string_desc_p, buffer_p, buffer_size);
  JERRY_ASSERT (size == buffer_size);
} /* ecma_string_to_utf8_bytes */

/**
 * Lengths for numeric string values
 */
static const uint32_t nums_with_ascending_length[] =
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

/**
 * Maximum length of numeric strings
 */
static const uint32_t max_uint32_len = (uint32_t) (sizeof (nums_with_ascending_length) / sizeof (uint32_t));

/**
 * Get size of the number stored locally in the string's descriptor
 *
 * Note: the represented number size and length are equal
 *
 * @return size in bytes
 */
static inline ecma_length_t __attr_always_inline___
ecma_string_get_number_in_desc_size (const uint32_t uint32_number) /**< number in the string-descriptor */
{
  ecma_length_t size = 1;

  while (size < max_uint32_len && uint32_number >= nums_with_ascending_length[size])
  {
    size++;
  }
  return size;
} /* ecma_string_get_number_in_desc_size */

/**
 * Checks whether the given string is a sequence of ascii characters.
 */
#define ECMA_STRING_IS_ASCII(char_p, size) ((size) == lit_utf8_string_length ((char_p), (size)))

/**
 * Returns with the raw byte array of the string, if it is available.
 *
 * @return byte array start - if the byte array of a string is available
 *         NULL - otherwise
 */
const lit_utf8_byte_t *
ecma_string_raw_chars (const ecma_string_t *string_p, /**< ecma-string */
                       lit_utf8_size_t *size_p, /**< [out] size of the ecma string */
                       bool *is_ascii_p) /**< [out] true, if the string is an ascii
                                          *               character sequence (size == length)
                                          *         false, otherwise */
{
  ecma_length_t length;
  lit_utf8_size_t size;
  const lit_utf8_byte_t *result_p;

  switch (ECMA_STRING_GET_CONTAINER (string_p))
  {
    case ECMA_STRING_CONTAINER_HEAP_UTF8_STRING:
    {
      size = string_p->u.utf8_string.size;
      length = string_p->u.utf8_string.length;
      result_p = (const lit_utf8_byte_t *) (string_p + 1);
      break;
    }
    case ECMA_STRING_CONTAINER_HEAP_LONG_UTF8_STRING:
    {
      size = string_p->u.long_utf8_string_size;
      ecma_long_string_t *long_string_p = (ecma_long_string_t *) string_p;
      length = long_string_p->long_utf8_string_length;
      result_p = (const lit_utf8_byte_t *) (long_string_p + 1);
      break;
    }
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    {
      size = (lit_utf8_size_t) ecma_string_get_number_in_desc_size (string_p->u.uint32_number);

      /* All numbers must be ascii strings. */
      JERRY_ASSERT (ecma_string_get_length (string_p) == size);

      length = size;
      result_p = NULL;
      break;
    }
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    {
      size = lit_get_magic_string_size (string_p->u.magic_string_id);

      length = size;
      result_p = lit_get_magic_string_utf8 (string_p->u.magic_string_id);

      /* All magic strings must be ascii strings. */
      JERRY_ASSERT (ECMA_STRING_IS_ASCII (result_p, size));
      break;
    }
    default:
    {
      JERRY_ASSERT (ECMA_STRING_GET_CONTAINER (string_p) == ECMA_STRING_CONTAINER_MAGIC_STRING_EX);

      size = lit_get_magic_string_ex_size (string_p->u.magic_string_ex_id);
      length = size;

      result_p = lit_get_magic_string_ex_utf8 (string_p->u.magic_string_ex_id);

      /* All extended magic strings must be ascii strings. */
      JERRY_ASSERT (ECMA_STRING_IS_ASCII (result_p, size));
      break;
    }
  }

  *size_p = size;
  *is_ascii_p = (length == size);
  return result_p;
} /* ecma_string_raw_chars */

/**
 * Checks whether ecma string is empty or not
 *
 * @return true  - if empty
 *         false - otherwise
 */
bool
ecma_string_is_empty (const ecma_string_t *str_p) /**< ecma-string */
{
  return (ECMA_STRING_GET_CONTAINER (str_p) == ECMA_STRING_CONTAINER_MAGIC_STRING
          && str_p->u.magic_string_id == LIT_MAGIC_STRING__EMPTY);
} /* ecma_string_is_empty */

/**
 * Checks whether the string equals to "length".
 *
 * @return true if the string equals to "length"
 *         false otherwise
 */
inline bool __attr_always_inline___
ecma_string_is_length (const ecma_string_t *string_p) /**< property name */
{
  return (ECMA_STRING_GET_CONTAINER (string_p) == ECMA_STRING_CONTAINER_MAGIC_STRING
          && string_p->u.magic_string_id == LIT_MAGIC_STRING_LENGTH);
} /* ecma_string_is_length */

/**
 * Converts a string into a property name
 *
 * @return the compressed pointer part of the name
 */
inline jmem_cpointer_t __attr_always_inline___
ecma_string_to_property_name (ecma_string_t *prop_name_p, /**< property name */
                              ecma_property_t *name_type_p) /**< [out] property name type */
{
  ecma_string_container_t container = ECMA_STRING_GET_CONTAINER (prop_name_p);

  switch (container)
  {
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    case ECMA_STRING_CONTAINER_MAGIC_STRING_EX:
    {
#ifdef JERRY_CPOINTER_32_BIT

      *name_type_p = (ecma_property_t) (container << ECMA_PROPERTY_NAME_TYPE_SHIFT);
      return (jmem_cpointer_t) prop_name_p->u.common_uint32_field;

#else /* !JERRY_CPOINTER_32_BIT */

      if (prop_name_p->u.common_uint32_field < (UINT16_MAX + 1))
      {
        *name_type_p = (ecma_property_t) (container << ECMA_PROPERTY_NAME_TYPE_SHIFT);
        return (jmem_cpointer_t) prop_name_p->u.common_uint32_field;
      }

#endif /* JERRY_CPOINTER_32_BIT */

      break;
    }
    default:
    {
      break;
    }
  }

  *name_type_p = ECMA_PROPERTY_NAME_TYPE_STRING << ECMA_PROPERTY_NAME_TYPE_SHIFT;

  ecma_ref_ecma_string (prop_name_p);

  jmem_cpointer_t prop_name_cp;
  ECMA_SET_NON_NULL_POINTER (prop_name_cp, prop_name_p);
  return prop_name_cp;
} /* ecma_string_to_property_name */

/**
 * Converts a property name into a string
 *
 * @return the string pointer
 */
ecma_string_t *
ecma_string_from_property_name (ecma_property_t property, /**< property name type */
                                jmem_cpointer_t prop_name_cp) /**< property name compressed pointer */
{
  /* If string_buf_p is NULL this function returns with a new string
   * instance which needs to be released with ecma_deref_ecma_string. */

  switch (ECMA_PROPERTY_GET_NAME_TYPE (property))
  {
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    {
      return ecma_new_ecma_string_from_uint32 ((uint32_t) prop_name_cp);
    }
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    {
      return ecma_new_ecma_string_from_magic_string_id ((lit_magic_string_id_t) prop_name_cp);
    }
    case ECMA_STRING_CONTAINER_MAGIC_STRING_EX:
    {
      return ecma_new_ecma_string_from_magic_string_ex_id ((lit_magic_string_ex_id_t) prop_name_cp);
    }
    default:
    {
      JERRY_ASSERT (ECMA_PROPERTY_GET_NAME_TYPE (property) == ECMA_PROPERTY_NAME_TYPE_STRING);

      ecma_string_t *prop_name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t, prop_name_cp);
      ecma_ref_ecma_string (prop_name_p);
      return prop_name_p;
    }
  }
} /* ecma_string_from_property_name */

/**
 * Get hash code of property name
 *
 * @return hash code of property name
 */
inline lit_string_hash_t __attr_always_inline___
ecma_string_get_property_name_hash (ecma_property_t property, /**< property name type */
                                    jmem_cpointer_t prop_name_cp) /**< property name compressed pointer */
{
  switch (ECMA_PROPERTY_GET_NAME_TYPE (property))
  {
    case ECMA_STRING_CONTAINER_MAGIC_STRING_EX:
    {
      return (lit_string_hash_t) (LIT_MAGIC_STRING__COUNT + prop_name_cp);
    }
    case ECMA_PROPERTY_NAME_TYPE_STRING:
    {
      ecma_string_t *prop_name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t, prop_name_cp);
      return ecma_string_hash (prop_name_p);
    }
    default:
    {
      return (lit_string_hash_t) prop_name_cp;
    }
  }
} /* ecma_string_get_property_name_hash */

/**
 * Check if property name is array index.
 *
 * @return ECMA_STRING_NOT_ARRAY_INDEX if string is not array index
 *         the array index otherwise
 */
uint32_t
ecma_string_get_property_index (ecma_property_t property, /**< property name type */
                                jmem_cpointer_t prop_name_cp) /**< property name compressed pointer */
{
  switch (ECMA_PROPERTY_GET_NAME_TYPE (property))
  {
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    {
      return (uint32_t) prop_name_cp;
    }
#ifndef JERRY_CPOINTER_32_BIT
    case ECMA_PROPERTY_NAME_TYPE_STRING:
    {
      ecma_string_t *prop_name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t, prop_name_cp);
      return ecma_string_get_array_index (prop_name_p);
    }
#endif /* !JERRY_CPOINTER_32_BIT */
    default:
    {
      return ECMA_STRING_NOT_ARRAY_INDEX;
    }
  }
} /* ecma_string_get_property_index */

/**
 * Compare a property name to a string
 *
 * @return true if they are equals
 *         false otherwise
 */
inline bool __attr_always_inline___
ecma_string_compare_to_property_name (ecma_property_t property, /**< property name type */
                                      jmem_cpointer_t prop_name_cp, /**< property name compressed pointer */
                                      const ecma_string_t *string_p) /**< other string */
{
  uint32_t property_name_type = ECMA_PROPERTY_GET_NAME_TYPE (property);

  switch (property_name_type)
  {
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    case ECMA_STRING_CONTAINER_MAGIC_STRING_EX:
    {
      return (ECMA_STRING_GET_CONTAINER (string_p) == property_name_type
              && string_p->u.common_uint32_field == (uint32_t) prop_name_cp);
    }
    default:
    {
      JERRY_ASSERT (property_name_type == ECMA_PROPERTY_NAME_TYPE_STRING);

      ecma_string_t *prop_name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t, prop_name_cp);
      return ecma_compare_ecma_strings (prop_name_p, string_p);
    }
  }
} /* ecma_string_compare_to_property_name */

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
  JERRY_ASSERT (ECMA_STRING_GET_CONTAINER (string1_p) == ECMA_STRING_GET_CONTAINER (string2_p));

  const lit_utf8_byte_t *utf8_string1_p, *utf8_string2_p;
  lit_utf8_size_t utf8_string1_size, utf8_string2_size;

  if (ECMA_STRING_GET_CONTAINER (string1_p) == ECMA_STRING_CONTAINER_HEAP_UTF8_STRING)
  {
    utf8_string1_p = (lit_utf8_byte_t *) (string1_p + 1);
    utf8_string1_size = string1_p->u.utf8_string.size;
    utf8_string2_p = (lit_utf8_byte_t *) (string2_p + 1);
    utf8_string2_size = string2_p->u.utf8_string.size;
  }
  else
  {
    JERRY_ASSERT (ECMA_STRING_GET_CONTAINER (string1_p) == ECMA_STRING_CONTAINER_HEAP_LONG_UTF8_STRING);

    utf8_string1_p = (lit_utf8_byte_t *) (((ecma_long_string_t *) string1_p) + 1);
    utf8_string1_size = string1_p->u.long_utf8_string_size;
    utf8_string2_p = (lit_utf8_byte_t *) (((ecma_long_string_t *) string2_p) + 1);
    utf8_string2_size = string2_p->u.long_utf8_string_size;
  }

  if (utf8_string1_size != utf8_string2_size)
  {
    return false;
  }

  return !memcmp ((char *) utf8_string1_p, (char *) utf8_string2_p, utf8_string1_size);
} /* ecma_compare_ecma_strings_longpath */

/**
 * Compare ecma-string to ecma-string
 *
 * @return true - if strings are equal;
 *         false - otherwise.
 */
bool __attr_always_inline___
ecma_compare_ecma_strings (const ecma_string_t *string1_p, /* ecma-string */
                           const ecma_string_t *string2_p) /* ecma-string */
{
  JERRY_ASSERT (string1_p != NULL && string2_p != NULL);

  /* Fast paths first. */
  if (string1_p == string2_p)
  {
    return true;
  }

  if (string1_p->hash != string2_p->hash)
  {
    return false;
  }

  ecma_string_container_t string1_container = ECMA_STRING_GET_CONTAINER (string1_p);

  if (string1_container != ECMA_STRING_GET_CONTAINER (string2_p))
  {
    return false;
  }

  if (string1_container < ECMA_STRING_CONTAINER_HEAP_UTF8_STRING)
  {
    return string1_p->u.common_uint32_field == string2_p->u.common_uint32_field;
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

  const lit_utf8_byte_t *utf8_string1_p, *utf8_string2_p;
  lit_utf8_size_t utf8_string1_size, utf8_string2_size;

  lit_utf8_byte_t uint32_to_string_buffer1[ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32];
  lit_utf8_byte_t uint32_to_string_buffer2[ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32];

  switch (ECMA_STRING_GET_CONTAINER (string1_p))
  {
    case ECMA_STRING_CONTAINER_HEAP_UTF8_STRING:
    {
      utf8_string1_p = (lit_utf8_byte_t *) (string1_p + 1);
      utf8_string1_size = string1_p->u.utf8_string.size;
      break;
    }
    case ECMA_STRING_CONTAINER_HEAP_LONG_UTF8_STRING:
    {
      utf8_string1_p = (lit_utf8_byte_t *) (((ecma_long_string_t *) string1_p) + 1);
      utf8_string1_size = string1_p->u.long_utf8_string_size;
      break;
    }
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    {
      utf8_string1_size = ecma_uint32_to_utf8_string (string1_p->u.uint32_number,
                                                      uint32_to_string_buffer1,
                                                      ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32);
      utf8_string1_p = uint32_to_string_buffer1;
      break;
    }
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    {
      utf8_string1_p = lit_get_magic_string_utf8 (string1_p->u.magic_string_id);
      utf8_string1_size = lit_get_magic_string_size (string1_p->u.magic_string_id);
      break;
    }
    default:
    {
      JERRY_ASSERT (ECMA_STRING_GET_CONTAINER (string1_p) == ECMA_STRING_CONTAINER_MAGIC_STRING_EX);

      utf8_string1_p = lit_get_magic_string_ex_utf8 (string1_p->u.magic_string_id);
      utf8_string1_size = lit_get_magic_string_ex_size (string1_p->u.magic_string_id);
      break;
    }
  }

  switch (ECMA_STRING_GET_CONTAINER (string2_p))
  {
    case ECMA_STRING_CONTAINER_HEAP_UTF8_STRING:
    {
      utf8_string2_p = (lit_utf8_byte_t *) (string2_p + 1);
      utf8_string2_size = string2_p->u.utf8_string.size;
      break;
    }
    case ECMA_STRING_CONTAINER_HEAP_LONG_UTF8_STRING:
    {
      utf8_string2_p = (lit_utf8_byte_t *) (((ecma_long_string_t *) string2_p) + 1);
      utf8_string2_size = string2_p->u.long_utf8_string_size;
      break;
    }
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    {
      utf8_string2_size = ecma_uint32_to_utf8_string (string2_p->u.uint32_number,
                                                      uint32_to_string_buffer2,
                                                      ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32);
      utf8_string2_p = uint32_to_string_buffer2;
      break;
    }
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    {
      utf8_string2_p = lit_get_magic_string_utf8 (string2_p->u.magic_string_id);
      utf8_string2_size = lit_get_magic_string_size (string2_p->u.magic_string_id);
      break;
    }
    default:
    {
      JERRY_ASSERT (ECMA_STRING_GET_CONTAINER (string2_p) == ECMA_STRING_CONTAINER_MAGIC_STRING_EX);

      utf8_string2_p = lit_get_magic_string_ex_utf8 (string2_p->u.magic_string_id);
      utf8_string2_size = lit_get_magic_string_ex_size (string2_p->u.magic_string_id);
      break;
    }
  }

  return lit_compare_utf8_strings_relational (utf8_string1_p,
                                              utf8_string1_size,
                                              utf8_string2_p,
                                              utf8_string2_size);
} /* ecma_compare_ecma_strings_relational */

/**
 * Get length of ecma-string
 *
 * @return number of characters in the string
 */
ecma_length_t
ecma_string_get_length (const ecma_string_t *string_p) /**< ecma-string */
{
  switch (ECMA_STRING_GET_CONTAINER (string_p))
  {
    case ECMA_STRING_CONTAINER_HEAP_UTF8_STRING:
    {
      return (ecma_length_t) (string_p->u.utf8_string.length);
    }
    case ECMA_STRING_CONTAINER_HEAP_LONG_UTF8_STRING:
    {
      return (ecma_length_t) (((ecma_long_string_t *) string_p)->long_utf8_string_length);
    }
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    {
      return ecma_string_get_number_in_desc_size (string_p->u.uint32_number);
    }
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    {
      JERRY_ASSERT (ECMA_STRING_IS_ASCII (lit_get_magic_string_utf8 (string_p->u.magic_string_id),
                                          lit_get_magic_string_size (string_p->u.magic_string_id)));
      return lit_get_magic_string_size (string_p->u.magic_string_id);
    }
    default:
    {
      JERRY_ASSERT (ECMA_STRING_GET_CONTAINER (string_p) == ECMA_STRING_CONTAINER_MAGIC_STRING_EX);

      JERRY_ASSERT (ECMA_STRING_IS_ASCII (lit_get_magic_string_ex_utf8 (string_p->u.magic_string_ex_id),
                                          lit_get_magic_string_ex_size (string_p->u.magic_string_ex_id)));
      return lit_get_magic_string_ex_size (string_p->u.magic_string_ex_id);
    }
  }
} /* ecma_string_get_length */

/**
 * Get length of UTF-8 encoded string length from ecma-string
 *
 * @return number of characters in the UTF-8 encoded string
 */
ecma_length_t
ecma_string_get_utf8_length (const ecma_string_t *string_p) /**< ecma-string */
{
  switch (ECMA_STRING_GET_CONTAINER (string_p))
  {
    case ECMA_STRING_CONTAINER_HEAP_UTF8_STRING:
    {
      if (string_p->u.utf8_string.size == (lit_utf8_size_t) string_p->u.utf8_string.length)
      {
        return (ecma_length_t) (string_p->u.utf8_string.length);
      }

      return lit_get_utf8_length_of_cesu8_string ((const lit_utf8_byte_t *) (string_p + 1),
                                                  (lit_utf8_size_t) string_p->u.utf8_string.size);
    }
    case ECMA_STRING_CONTAINER_HEAP_LONG_UTF8_STRING:
    {
      ecma_long_string_t *long_string_p = (ecma_long_string_t *) string_p;
      if (string_p->u.long_utf8_string_size == (lit_utf8_size_t) long_string_p->long_utf8_string_length)
      {
        return (ecma_length_t) (long_string_p->long_utf8_string_length);
      }

      return lit_get_utf8_length_of_cesu8_string ((const lit_utf8_byte_t *) (string_p + 1),
                                                  (lit_utf8_size_t) string_p->u.long_utf8_string_size);
    }
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    {
      return ecma_string_get_number_in_desc_size (string_p->u.uint32_number);
    }
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    {
      JERRY_ASSERT (ECMA_STRING_IS_ASCII (lit_get_magic_string_utf8 (string_p->u.magic_string_id),
                                          lit_get_magic_string_size (string_p->u.magic_string_id)));
      return lit_get_magic_string_size (string_p->u.magic_string_id);
    }
    default:
    {
      JERRY_ASSERT (ECMA_STRING_GET_CONTAINER (string_p) == ECMA_STRING_CONTAINER_MAGIC_STRING_EX);

      return lit_get_utf8_length_of_cesu8_string (lit_get_magic_string_ex_utf8 (string_p->u.magic_string_ex_id),
                                                  lit_get_magic_string_ex_size (string_p->u.magic_string_ex_id));
    }
  }
} /* ecma_string_get_utf8_length */

/**
 * Get size of ecma-string
 *
 * @return number of bytes in the buffer needed to represent the string
 */
lit_utf8_size_t
ecma_string_get_size (const ecma_string_t *string_p) /**< ecma-string */
{
  switch (ECMA_STRING_GET_CONTAINER (string_p))
  {
    case ECMA_STRING_CONTAINER_HEAP_UTF8_STRING:
    {
      return (lit_utf8_size_t) string_p->u.utf8_string.size;
    }
    case ECMA_STRING_CONTAINER_HEAP_LONG_UTF8_STRING:
    {
      return (lit_utf8_size_t) string_p->u.long_utf8_string_size;
    }
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    {
      return (lit_utf8_size_t) ecma_string_get_number_in_desc_size (string_p->u.uint32_number);
    }
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    {
      return lit_get_magic_string_size (string_p->u.magic_string_id);
    }
    default:
    {
      JERRY_ASSERT (ECMA_STRING_GET_CONTAINER (string_p) == ECMA_STRING_CONTAINER_MAGIC_STRING_EX);
      return lit_get_magic_string_ex_size (string_p->u.magic_string_ex_id);
    }
  }
} /* ecma_string_get_size */

/**
 * Get the UTF-8 encoded string size from ecma-string
 *
 * @return number of bytes in the buffer needed to represent an UTF-8 encoded string
 */
lit_utf8_size_t
ecma_string_get_utf8_size (const ecma_string_t *string_p) /**< ecma-string */
{
  switch (ECMA_STRING_GET_CONTAINER (string_p))
  {
    case ECMA_STRING_CONTAINER_HEAP_UTF8_STRING:
    {
      if (string_p->u.utf8_string.size == (lit_utf8_size_t) string_p->u.utf8_string.length)
      {
        return (lit_utf8_size_t) string_p->u.utf8_string.size;
      }

      return lit_get_utf8_size_of_cesu8_string ((const lit_utf8_byte_t *) (string_p + 1),
                                                (lit_utf8_size_t) string_p->u.utf8_string.size);
    }
    case ECMA_STRING_CONTAINER_HEAP_LONG_UTF8_STRING:
    {
      ecma_long_string_t *long_string_p = (ecma_long_string_t *) string_p;
      if (string_p->u.long_utf8_string_size == (lit_utf8_size_t) long_string_p->long_utf8_string_length)
      {
        return (lit_utf8_size_t) string_p->u.long_utf8_string_size;
      }

      return lit_get_utf8_size_of_cesu8_string ((const lit_utf8_byte_t *) (string_p + 1),
                                                (lit_utf8_size_t) string_p->u.long_utf8_string_size);
    }
    case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
    {
      return (lit_utf8_size_t) ecma_string_get_number_in_desc_size (string_p->u.uint32_number);
    }
    case ECMA_STRING_CONTAINER_MAGIC_STRING:
    {
      return lit_get_magic_string_size (string_p->u.magic_string_id);
    }
    default:
    {
      JERRY_ASSERT (ECMA_STRING_GET_CONTAINER (string_p) == ECMA_STRING_CONTAINER_MAGIC_STRING_EX);

      return lit_get_utf8_size_of_cesu8_string (lit_get_magic_string_ex_utf8 (string_p->u.magic_string_ex_id),
                                                lit_get_magic_string_ex_size (string_p->u.magic_string_ex_id));
    }
  }
} /* ecma_string_get_utf8_size */

/**
 * Get character from specified position in the ecma-string.
 *
 * @return character value
 */
ecma_char_t
ecma_string_get_char_at_pos (const ecma_string_t *string_p, /**< ecma-string */
                             ecma_length_t index) /**< index of character */
{
  JERRY_ASSERT (index < ecma_string_get_length (string_p));

  lit_utf8_size_t buffer_size;
  bool is_ascii;
  const lit_utf8_byte_t *chars_p = ecma_string_raw_chars (string_p, &buffer_size, &is_ascii);

  if (chars_p != NULL)
  {
    if (is_ascii)
    {
      return chars_p[index];
    }

    return lit_utf8_string_code_unit_at (chars_p, buffer_size, index);
  }

  ecma_char_t ch;

  JMEM_DEFINE_LOCAL_ARRAY (utf8_str_p, buffer_size, lit_utf8_byte_t);

  ecma_string_to_utf8_bytes (string_p, utf8_str_p, buffer_size);

  JERRY_ASSERT (ECMA_STRING_GET_CONTAINER (string_p) == ECMA_STRING_CONTAINER_UINT32_IN_DESC);
  /* Both above must be ascii strings. */
  JERRY_ASSERT (is_ascii);

  ch = utf8_str_p[index];

  JMEM_FINALIZE_LOCAL_ARRAY (utf8_str_p);

  return ch;
} /* ecma_string_get_char_at_pos */

/**
 * Get specified magic string
 *
 * @return ecma-string containing specified magic string
 */
ecma_string_t *
ecma_get_magic_string (lit_magic_string_id_t id) /**< magic string id */
{
  return ecma_new_ecma_string_from_magic_string_id (id);
} /* ecma_get_magic_string */

/**
 * Get specified external magic string
 *
 * @return ecma-string containing specified external magic string
 */
ecma_string_t *
ecma_get_magic_string_ex (lit_magic_string_ex_id_t id) /**< external magic string id */
{
  return ecma_new_ecma_string_from_magic_string_ex_id (id);
} /* ecma_get_magic_string_ex */

/**
 * Check if passed string equals to one of magic strings
 * and if equal magic string was found, return it's id in 'out_id_p' argument.
 *
 * @return id - if magic string equal to passed string was found,
 *         LIT_MAGIC_STRING__COUNT - otherwise.
 */
lit_magic_string_id_t
ecma_get_string_magic (const ecma_string_t *string_p) /**< ecma-string */
{
  if (ECMA_STRING_GET_CONTAINER (string_p) == ECMA_STRING_CONTAINER_MAGIC_STRING)
  {
    return (lit_magic_string_id_t) string_p->u.magic_string_id;
  }

  return LIT_MAGIC_STRING__COUNT;
} /* ecma_get_string_magic */

/**
 * Try to calculate hash of the ecma-string
 *
 * @return calculated hash
 */
lit_string_hash_t
ecma_string_hash (const ecma_string_t *string_p) /**< ecma-string to calculate hash for */
{
  return (string_p->hash);
} /* ecma_string_hash */

/**
 * Create a substring from an ecma string
 *
 * @return a newly consturcted ecma string with its value initialized to a copy of a substring of the first argument
 */
ecma_string_t *
ecma_string_substr (const ecma_string_t *string_p, /**< pointer to an ecma string */
                    ecma_length_t start_pos, /**< start position, should be less or equal than string length */
                    ecma_length_t end_pos) /**< end position, should be less or equal than string length */
{
#ifndef JERRY_NDEBUG
  const ecma_length_t string_length = ecma_string_get_length (string_p);
  JERRY_ASSERT (start_pos <= string_length);
  JERRY_ASSERT (end_pos <= string_length);
#endif /* !JERRY_NDEBUG */

  if (start_pos < end_pos)
  {
    lit_utf8_size_t buffer_size;
    bool is_ascii;
    const lit_utf8_byte_t *start_p = ecma_string_raw_chars (string_p, &buffer_size, &is_ascii);

    end_pos -= start_pos;

    if (start_p != NULL)
    {
      if (is_ascii)
      {
        return ecma_new_ecma_string_from_utf8 (start_p + start_pos,
                                               (lit_utf8_size_t) end_pos);
      }

      while (start_pos--)
      {
        start_p += lit_get_unicode_char_size_by_utf8_first_byte (*start_p);
      }

      const lit_utf8_byte_t *end_p = start_p;
      while (end_pos--)
      {
        end_p += lit_get_unicode_char_size_by_utf8_first_byte (*end_p);
      }

      return ecma_new_ecma_string_from_utf8 (start_p, (lit_utf8_size_t) (end_p - start_p));
    }

    /**
     * I. Dump original string to plain buffer
     */
    ecma_string_t *ecma_string_p;

    JMEM_DEFINE_LOCAL_ARRAY (utf8_str_p, buffer_size, lit_utf8_byte_t);

    ecma_string_to_utf8_bytes (string_p, utf8_str_p, buffer_size);

    /**
     * II. Extract substring
     */
    start_p = utf8_str_p;

    if (is_ascii)
    {
      ecma_string_p = ecma_new_ecma_string_from_utf8 (start_p + start_pos,
                                                      (lit_utf8_size_t) end_pos);
    }
    else
    {
      while (start_pos--)
      {
        start_p += lit_get_unicode_char_size_by_utf8_first_byte (*start_p);
      }

      const lit_utf8_byte_t *end_p = start_p;
      while (end_pos--)
      {
        end_p += lit_get_unicode_char_size_by_utf8_first_byte (*end_p);
      }

      ecma_string_p = ecma_new_ecma_string_from_utf8 (start_p, (lit_utf8_size_t) (end_p - start_p));
    }

    JMEM_FINALIZE_LOCAL_ARRAY (utf8_str_p);

    return ecma_string_p;
  }
  else
  {
    return ecma_new_ecma_string_from_utf8 (NULL, 0);
  }

  JERRY_UNREACHABLE ();
} /* ecma_string_substr */

/**
 * Trim leading and trailing whitespace characters from string.
 *
 * @return trimmed ecma string
 */
ecma_string_t *
ecma_string_trim (const ecma_string_t *string_p) /**< pointer to an ecma string */
{
  ecma_string_t *ret_string_p;

  ECMA_STRING_TO_UTF8_STRING (string_p, utf8_str_p, utf8_str_size);

  if (utf8_str_size > 0)
  {
    ecma_char_t ch;
    lit_utf8_size_t read_size;
    const lit_utf8_byte_t *nonws_start_p = utf8_str_p + utf8_str_size;
    const lit_utf8_byte_t *current_p = utf8_str_p;

    /* Trim front. */
    while (current_p < nonws_start_p)
    {
      read_size = lit_read_code_unit_from_utf8 (current_p, &ch);

      if (!lit_char_is_white_space (ch)
          && !lit_char_is_line_terminator (ch))
      {
        nonws_start_p = current_p;
        break;
      }

      current_p += read_size;
    }

    current_p = utf8_str_p + utf8_str_size;

    /* Trim back. */
    while (current_p > utf8_str_p)
    {
      read_size = lit_read_prev_code_unit_from_utf8 (current_p, &ch);

      if (!lit_char_is_white_space (ch)
          && !lit_char_is_line_terminator (ch))
      {
        break;
      }

      current_p -= read_size;
    }

    /* Construct new string. */
    if (current_p > nonws_start_p)
    {
      ret_string_p = ecma_new_ecma_string_from_utf8 (nonws_start_p,
                                                     (lit_utf8_size_t) (current_p - nonws_start_p));
    }
    else
    {
      ret_string_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
    }
  }
  else
  {
    ret_string_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
  }

  ECMA_FINALIZE_UTF8_STRING (utf8_str_p, utf8_str_size);

  return ret_string_p;
} /* ecma_string_trim */

/**
 * @}
 * @}
 */
