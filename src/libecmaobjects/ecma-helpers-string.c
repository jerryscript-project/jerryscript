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

/** \addtogroup ecma ---TODO---
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

#include "ecma-alloc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "globals.h"
#include "jerry-libc.h"

/**
 * Allocate new ecma-string and fill it with characters from specified buffer
 *
 * @return Pointer to first chunk of an array, containing allocated string
 */
ecma_array_first_chunk_t*
ecma_new_ecma_string (const ecma_char_t *string_p) /**< zero-terminated string of ecma-characters */
{
  ecma_length_t length = 0;

  /*
   * TODO: Do not precalculate length.
   */
  if (string_p != NULL)
  {
    const ecma_char_t *iter_p = string_p;
    while (*iter_p++)
    {
      length++;
    }
  }

  ecma_array_first_chunk_t *string_first_chunk_p = ecma_alloc_array_first_chunk ();

  string_first_chunk_p->header.unit_number = length;
  uint8_t *copy_pointer = (uint8_t*) string_p;
  size_t chars_left = length;
  size_t chars_to_copy = JERRY_MIN(length, sizeof (string_first_chunk_p->data) / sizeof (ecma_char_t));
  __memcpy (string_first_chunk_p->data, copy_pointer, chars_to_copy * sizeof (ecma_char_t));
  chars_left -= chars_to_copy;
  copy_pointer += chars_to_copy * sizeof (ecma_char_t);

  ecma_array_non_first_chunk_t *string_non_first_chunk_p;

  JERRY_STATIC_ASSERT(ECMA_POINTER_FIELD_WIDTH <= sizeof (uint16_t) * JERRY_BITSINBYTE);
  uint16_t *next_chunk_compressed_pointer_p = &string_first_chunk_p->header.next_chunk_p;

  while (chars_left > 0)
  {
    string_non_first_chunk_p = ecma_alloc_array_non_first_chunk ();

    size_t chars_to_copy = JERRY_MIN(chars_left, sizeof (string_non_first_chunk_p->data) / sizeof (ecma_char_t));
    __memcpy (string_non_first_chunk_p->data, copy_pointer, chars_to_copy * sizeof (ecma_char_t));
    chars_left -= chars_to_copy;
    copy_pointer += chars_to_copy * sizeof (ecma_char_t);

    ECMA_SET_NON_NULL_POINTER(*next_chunk_compressed_pointer_p, string_non_first_chunk_p);
    next_chunk_compressed_pointer_p = &string_non_first_chunk_p->next_chunk_p;
  }

  *next_chunk_compressed_pointer_p = ECMA_NULL_POINTER;

  return string_first_chunk_p;
} /* ecma_new_ecma_string */

/**
 * Copy ecma-string's contents to a buffer.
 *
 * Buffer will contain length of string, in characters, followed by string's characters.
 *
 * @return number of bytes, actually copied to the buffer, if string's content was copied successfully;
 *         negative number, which is calculated as negation of buffer size, that is required
 *         to hold the string's content (in case size of buffer is insuficcient).
 */
ssize_t
ecma_copy_ecma_string_chars_to_buffer (ecma_array_first_chunk_t *first_chunk_p, /**< first chunk of ecma-string */
                                       uint8_t *buffer_p, /**< destination buffer */
                                       size_t buffer_size) /**< size of buffer */
{
  ecma_length_t string_length = first_chunk_p->header.unit_number;
  size_t required_buffer_size = sizeof (ecma_length_t) + sizeof (ecma_char_t) * string_length;

  if (required_buffer_size > buffer_size)
  {
    return - (ssize_t) required_buffer_size;
  }

  *(ecma_length_t*) buffer_p = string_length;

  size_t chars_left = string_length;
  uint8_t *dest_pointer = buffer_p + sizeof (ecma_length_t);
  size_t copy_chunk_chars = JERRY_MIN(sizeof (first_chunk_p->data) / sizeof (ecma_char_t),
                                      chars_left);
  __memcpy (dest_pointer, first_chunk_p->data, copy_chunk_chars * sizeof (ecma_char_t));
  dest_pointer += copy_chunk_chars * sizeof (ecma_char_t);
  chars_left -= copy_chunk_chars;

  ecma_array_non_first_chunk_t *non_first_chunk_p = ECMA_GET_POINTER(first_chunk_p->header.next_chunk_p);

  while (chars_left > 0)
  {
    JERRY_ASSERT(chars_left < string_length);

    copy_chunk_chars = JERRY_MIN(sizeof (non_first_chunk_p->data) / sizeof (ecma_char_t),
                                 chars_left);
    __memcpy (dest_pointer, non_first_chunk_p->data, copy_chunk_chars * sizeof (ecma_char_t));
    dest_pointer += copy_chunk_chars * sizeof (ecma_char_t);
    chars_left -= copy_chunk_chars;

    non_first_chunk_p = ECMA_GET_POINTER(non_first_chunk_p->next_chunk_p);
  }

  return (ssize_t) required_buffer_size;
} /* ecma_copy_ecma_string_chars_to_buffer */

/**
 * Duplicate an ecma-string.
 *
 * @return pointer to new ecma-string's first chunk
 */
ecma_array_first_chunk_t*
ecma_duplicate_ecma_string (ecma_array_first_chunk_t *first_chunk_p) /**< first chunk of string to duplicate */
{
  JERRY_ASSERT(first_chunk_p != NULL);

  ecma_array_first_chunk_t *first_chunk_copy_p = ecma_alloc_array_first_chunk ();
  __memcpy (first_chunk_copy_p, first_chunk_p, sizeof (ecma_array_first_chunk_t));

  ecma_array_non_first_chunk_t *non_first_chunk_p, *non_first_chunk_copy_p;
  non_first_chunk_p = ECMA_GET_POINTER(first_chunk_p->header.next_chunk_p);
  uint16_t *next_pointer_p = &first_chunk_copy_p->header.next_chunk_p;

  while (non_first_chunk_p != NULL)
  {
    non_first_chunk_copy_p = ecma_alloc_array_non_first_chunk ();
    ECMA_SET_POINTER(*next_pointer_p, non_first_chunk_copy_p);
    next_pointer_p = &non_first_chunk_copy_p->next_chunk_p;

    __memcpy (non_first_chunk_copy_p, non_first_chunk_p, sizeof (ecma_array_non_first_chunk_t));

    non_first_chunk_p = ECMA_GET_POINTER(non_first_chunk_p->next_chunk_p);
  }

  *next_pointer_p = ECMA_NULL_POINTER;

  return first_chunk_copy_p;
} /* ecma_duplicate_ecma_string */

/**
 * Compare ecma-string to ecma-string
 *
 * @return true - if strings are equal;
 *         false - otherwise.
 */
bool
ecma_compare_ecma_string_to_ecma_string (const ecma_array_first_chunk_t *string1_p, /* ecma-string */
                                         const ecma_array_first_chunk_t *string2_p) /* ecma-string */
{
  JERRY_ASSERT (string1_p != NULL && string2_p != NULL);

  if (string1_p->header.unit_number != string2_p->header.unit_number)
  {
    return false;
  }

  ecma_length_t chars_left = string1_p->header.unit_number;

  JERRY_STATIC_ASSERT (ECMA_POINTER_FIELD_WIDTH <= sizeof (uint16_t) * JERRY_BITSINBYTE);
  const uint16_t *next_chunk_compressed_pointer_1_p = &string1_p->header.next_chunk_p;
  const uint16_t *next_chunk_compressed_pointer_2_p = &string2_p->header.next_chunk_p;
  const ecma_char_t *cur_char_array_1_p = string1_p->data;
  const ecma_char_t *cur_char_array_1_end_p = string1_p->data + sizeof (string1_p->data);
  const ecma_char_t *cur_char_array_2_p = string2_p->data;
  const ecma_char_t *cur_char_array_2_end_p = string2_p->data + sizeof (string2_p->data);

  while (chars_left > 0)
  {
    if (cur_char_array_1_p != cur_char_array_1_end_p)
    {
      JERRY_ASSERT (cur_char_array_2_p < cur_char_array_2_end_p);

      if (*cur_char_array_1_p++ != *cur_char_array_2_p++)
      {
        return false;
      }

      chars_left--;
    }
    else
    {
      ecma_array_non_first_chunk_t *non_first_chunk_1_p = ECMA_GET_POINTER (*next_chunk_compressed_pointer_1_p);
      ecma_array_non_first_chunk_t *non_first_chunk_2_p = ECMA_GET_POINTER (*next_chunk_compressed_pointer_2_p);

      JERRY_ASSERT (non_first_chunk_1_p != NULL && non_first_chunk_2_p != NULL);

      cur_char_array_1_p = non_first_chunk_1_p->data;
      cur_char_array_1_end_p = non_first_chunk_1_p->data + sizeof (non_first_chunk_1_p->data);
      cur_char_array_2_p = non_first_chunk_2_p->data;
      cur_char_array_2_end_p = non_first_chunk_2_p->data + sizeof (non_first_chunk_2_p->data);
    }
  }

  return true;
} /* ecma_compare_ecma_string_to_ecma_string */

/**
 * Compare zero-terminated string to zero-terminated string
 *
 * @return  0 - if strings are equal;
 *         -1 - if first string is lexicographically less than second;
 *          1 - otherwise.
 */
int32_t
ecma_compare_zt_string_to_zt_string (const ecma_char_t *string1_p, /**< zero-terminated string */
                                     const ecma_char_t *string2_p) /**< zero-terminated string */
{
  TODO (Implement comparison that supports UTF-16);

  return __strcmp ( (char*)string1_p, (char*)string2_p);
} /* ecma_compare_zt_string_to_zt_string */

/**
 * Compare zero-terminated string to ecma-string
 *
 * @return true - if strings are equal;
 *         false - otherwise.
 */
bool
ecma_compare_zt_string_to_ecma_string (const ecma_char_t *string_p, /**< zero-terminated string */
                                       const ecma_array_first_chunk_t *ecma_string_p) /* ecma-string */
{
  JERRY_ASSERT(string_p != NULL);
  JERRY_ASSERT(ecma_string_p != NULL);

  const ecma_char_t *str_iter_p = string_p;
  ecma_length_t ecma_str_len = ecma_string_p->header.unit_number;
  const ecma_char_t *current_chunk_chars_cur = (const ecma_char_t*) ecma_string_p->data;
  const ecma_char_t *current_chunk_chars_end = (const ecma_char_t*) (ecma_string_p->data +
                                                                     sizeof (ecma_string_p->data));

  JERRY_STATIC_ASSERT(ECMA_POINTER_FIELD_WIDTH <= sizeof (uint16_t) * JERRY_BITSINBYTE);
  const uint16_t *next_chunk_compressed_pointer_p = &ecma_string_p->header.next_chunk_p;

  for (ecma_length_t str_index = 0;
       str_index < ecma_str_len;
       str_index++, str_iter_p++, current_chunk_chars_cur++)
  {
    JERRY_ASSERT(current_chunk_chars_cur <= current_chunk_chars_end);

    if (current_chunk_chars_cur == current_chunk_chars_end)
    {
      /* switching to next chunk */
      ecma_array_non_first_chunk_t *next_chunk_p = ECMA_GET_POINTER(*next_chunk_compressed_pointer_p);

      JERRY_ASSERT(next_chunk_p != NULL);

      current_chunk_chars_cur = (const ecma_char_t*) next_chunk_p->data;
      current_chunk_chars_end = (const ecma_char_t*) (next_chunk_p->data + sizeof (next_chunk_p->data));

      next_chunk_compressed_pointer_p = &next_chunk_p->next_chunk_p;
    }

    if (*str_iter_p != *current_chunk_chars_cur)
    {
      /*
       * Either *str_iter_p is 0 (zero-terminated string is shorter),
       * or the character is just different.
       *
       * In both cases strings are not equal.
       */
      return false;
    }
  }

  /*
   * Now, we have reached end of ecma-string.
   *
   * If we have also reached end of zero-terminated string, than strings are equal.
   * Otherwise zero-terminated string is longer.
   */
  return (*str_iter_p == 0);
} /* ecma_compare_zt_string_to_ecma_string */

/**
 * @}
 * @}
 */
