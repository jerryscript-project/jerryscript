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
 * @return pointer to ecma-string descriptor
 */
ecma_string_t*
ecma_new_ecma_string (const ecma_char_t *string_p) /**< zero-terminated string */
{
  JERRY_ASSERT (string_p != NULL);

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
  string_desc_p->length = length;

  if (bytes_needed_for_current_string <= bytes_for_chars_in_string_descriptor)
  {
    string_desc_p->container = ECMA_STRING_CONTAINER_IN_DESCRIPTOR;
    __memcpy (string_desc_p->u.chars, string_p, bytes_needed_for_current_string);

    return string_desc_p;
  }

  string_desc_p->container = ECMA_STRING_CONTAINER_HEAP;

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
 * Decrease reference counter and deallocate ecma-string if
 * after that the counter the counter becomes zero.
 */
void
ecma_free_string (ecma_string_t *string_p) /**< ecma-string */
{
  JERRY_ASSERT (string_p != NULL);
  JERRY_ASSERT (string_p->refs != 0);

  string_p->refs--;

  if (string_p->refs != 0)
  {
    return;
  }

  if (string_p->container == ECMA_STRING_CONTAINER_HEAP)
  {
    ecma_collection_chunk_t *chunk_p = ECMA_GET_POINTER (string_p->u.chunk_cp);

    JERRY_ASSERT (chunk_p != NULL);

    while (chunk_p != NULL)
    {
      ecma_collection_chunk_t *next_chunk_p = ECMA_GET_POINTER (chunk_p->next_chunk_cp);

      ecma_dealloc_collection_chunk (chunk_p);

      chunk_p = next_chunk_p;
    }
  }
  else
  {
    JERRY_ASSERT (string_p->container == ECMA_STRING_CONTAINER_IN_DESCRIPTOR
                  || string_p->container == ECMA_STRING_CONTAINER_LIT_TABLE);

    /* only the string descriptor itself should be freed */
  }

  ecma_dealloc_string (string_p);
} /* ecma_free_string */

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
ecma_string_to_zt_string (ecma_string_t *string_desc_p, /**< ecma-string descriptor */
                          ecma_char_t *buffer_p, /**< destination buffer */
                          size_t buffer_size) /**< size of buffer */
{
  JERRY_ASSERT (string_desc_p != NULL);
  JERRY_ASSERT (string_desc_p->refs > 0);
  JERRY_ASSERT (buffer_p != NULL);
  JERRY_ASSERT (buffer_size > 0);

  const ecma_length_t string_length = string_desc_p->length;
  size_t required_buffer_size = sizeof (ecma_char_t) * string_length + 1 /* for zero char */;

  if (required_buffer_size > buffer_size)
  {
    return - (ssize_t) required_buffer_size;
  }

  ecma_char_t *dest_p = buffer_p;

  switch ((ecma_string_container_t)string_desc_p->container)
  {
    case ECMA_STRING_CONTAINER_IN_DESCRIPTOR:
    {
      __memcpy (dest_p, string_desc_p->u.chars, string_length * sizeof (ecma_char_t));
      dest_p += string_length;

      break;
    }
    case ECMA_STRING_CONTAINER_HEAP:
    {
      ecma_collection_chunk_t *string_chunk_p = ECMA_GET_POINTER (string_desc_p->u.chunk_cp);

      const ecma_length_t max_chars_in_chunk = sizeof (string_chunk_p->data) / sizeof (ecma_char_t);

      ecma_length_t chars_left = string_length;
      while (chars_left > 0)
      {
        JERRY_ASSERT(chars_left < string_length);

        ecma_length_t chars_to_copy = JERRY_MIN(max_chars_in_chunk, chars_left);

        __memcpy (dest_p, string_chunk_p->data, chars_to_copy * sizeof (ecma_char_t));
        dest_p += chars_to_copy;
        chars_left = (ecma_length_t) (chars_left - chars_to_copy);

        string_chunk_p = ECMA_GET_POINTER(string_chunk_p->next_chunk_cp);
      }
      break;
    }
    case ECMA_STRING_CONTAINER_LIT_TABLE:
    {
      JERRY_UNIMPLEMENTED();
    }
  }

  *dest_p = '\0';

  return (ssize_t) required_buffer_size;
} /* ecma_string_to_zt_string */

/**
 * Increase reference counter of ecma-string.
 *
 * @return pointer to same ecma-string descriptor with increased reference counter
 */
void
ecma_ref_ecma_string (ecma_string_t *string_desc_p) /**< string descriptor */
{
  JERRY_ASSERT (string_desc_p != NULL);
  JERRY_ASSERT (string_desc_p->refs > 0);

  string_desc_p->refs++;

  /* Check for overflow */
  JERRY_ASSERT (string_desc_p->refs > 0);
} /* ecma_ref_ecma_string */

/**
 * Compare ecma-string to ecma-string
 *
 * @return true - if strings are equal;
 *         false - otherwise.
 */
bool
ecma_compare_ecma_string_to_ecma_string (const ecma_string_t *string1_p, /* ecma-string */
                                         const ecma_string_t *string2_p) /* ecma-string */
{
  JERRY_ASSERT (string1_p != NULL && string2_p != NULL);

  if (string1_p == string2_p)
  {
    return true;
  }

  if (string1_p->length != string2_p->length)
  {
    return false;
  }

  ecma_length_t chars_left = string1_p->length;

  if (string1_p->container == ECMA_STRING_CONTAINER_IN_DESCRIPTOR
      && string2_p->container == ECMA_STRING_CONTAINER_IN_DESCRIPTOR)
  {
    return __memcmp (string1_p->u.chars, string2_p->u.chars, chars_left * sizeof (ecma_char_t));
  }

  if (string1_p->container == ECMA_STRING_CONTAINER_LIT_TABLE
      || string2_p->container == ECMA_STRING_CONTAINER_LIT_TABLE)
  {
    JERRY_UNIMPLEMENTED();
  }

  if (string1_p->container == ECMA_STRING_CONTAINER_HEAP
      && string2_p->container == ECMA_STRING_CONTAINER_IN_DESCRIPTOR)
  {
    const ecma_string_t *tmp_string_p = string1_p;
    string1_p = string2_p;
    string2_p = tmp_string_p;
  }
  JERRY_ASSERT (string2_p->container == ECMA_STRING_CONTAINER_HEAP);

  ecma_collection_chunk_t *string1_chunk_p, *string2_chunk_p;
  const ecma_char_t *cur_char_buffer_1_iter_p, *cur_char_buffer_2_iter_p;
  const ecma_char_t *cur_char_buffer_1_end_p, *cur_char_buffer_2_end_p;
  if (string1_p->container == ECMA_STRING_CONTAINER_IN_DESCRIPTOR)
  {
    string1_chunk_p = NULL;
    cur_char_buffer_1_iter_p = string1_p->u.chars;
    cur_char_buffer_1_end_p = cur_char_buffer_1_iter_p + sizeof (string1_p->u.chars) / sizeof (ecma_char_t);
  }
  else
  {
    string1_chunk_p = ECMA_GET_POINTER (string1_p->u.chunk_cp);
    cur_char_buffer_1_iter_p = string1_chunk_p->data;
    cur_char_buffer_1_end_p = cur_char_buffer_1_iter_p + sizeof (string1_chunk_p->data) / sizeof (ecma_char_t);
  }

  string2_chunk_p = ECMA_GET_POINTER (string2_p->u.chunk_cp);
  cur_char_buffer_2_iter_p = string2_chunk_p->data;
  cur_char_buffer_2_end_p = cur_char_buffer_2_iter_p + sizeof (string2_chunk_p->data) / sizeof (ecma_char_t);

  /*
   * The assertion check allows to combine update of cur_char_buffer_1_iter_p and cur_char_buffer_2_iter_p pointers,
   * because:
   *         - if we reached end of string in descriptor then we reached end of string => chars_left is zero;
   *         - if we reached end of string chunk in heap then we reached end of other string's chunk too
   *           (they're lengths are equal) => both pointer should be updated.
   */
  JERRY_STATIC_ASSERT (sizeof (string1_p->u.chars) / sizeof (ecma_char_t)
                       < sizeof (string1_chunk_p->data) / sizeof (ecma_char_t));

  while (chars_left > 0)
  {
    if (cur_char_buffer_1_iter_p != cur_char_buffer_1_end_p
        && cur_char_buffer_2_iter_p != cur_char_buffer_2_end_p)
    {
      JERRY_ASSERT (cur_char_buffer_2_iter_p < cur_char_buffer_2_end_p);

      if (*cur_char_buffer_1_iter_p++ != *cur_char_buffer_2_iter_p++)
      {
        return false;
      }

      chars_left--;

      continue;
    }
    
    if (cur_char_buffer_1_iter_p == cur_char_buffer_1_end_p)
    {
      JERRY_ASSERT (string1_p->container == ECMA_STRING_CONTAINER_HEAP
                    && string2_p->container == ECMA_STRING_CONTAINER_HEAP);
      JERRY_ASSERT (cur_char_buffer_2_iter_p == cur_char_buffer_2_end_p);

      string1_chunk_p = ECMA_GET_POINTER (string1_chunk_p->next_chunk_cp);
      JERRY_ASSERT (string1_chunk_p != NULL);
      cur_char_buffer_1_iter_p = string1_chunk_p->data;
      cur_char_buffer_1_end_p = cur_char_buffer_1_iter_p + sizeof (string1_chunk_p->data) / sizeof (ecma_char_t);

      string2_chunk_p = ECMA_GET_POINTER (string2_chunk_p->next_chunk_cp);
      JERRY_ASSERT (string2_chunk_p != NULL);
      cur_char_buffer_2_iter_p = string2_chunk_p->data;
      cur_char_buffer_2_end_p = cur_char_buffer_2_iter_p + sizeof (string2_chunk_p->data) / sizeof (ecma_char_t);
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
                                       const ecma_string_t *ecma_string_p) /* ecma-string */
{
  JERRY_ASSERT(string_p != NULL);
  JERRY_ASSERT(ecma_string_p != NULL);

  const ecma_char_t *str_iter_p = string_p;
  ecma_length_t ecma_str_len = ecma_string_p->length;
  const ecma_char_t *current_chunk_chars_cur;
  const ecma_char_t *current_chunk_chars_end;
  ecma_collection_chunk_t *string_chunk_p = NULL;

  if (ecma_string_p->container == ECMA_STRING_CONTAINER_IN_DESCRIPTOR)
  {
    current_chunk_chars_cur = ecma_string_p->u.chars;
    current_chunk_chars_end = current_chunk_chars_cur + sizeof (ecma_string_p->u.chars) / sizeof (ecma_char_t);
  }
  else if (ecma_string_p->container == ECMA_STRING_CONTAINER_HEAP)
  {
    string_chunk_p = ECMA_GET_POINTER (ecma_string_p->u.chunk_cp);
    current_chunk_chars_cur = string_chunk_p->data;
    current_chunk_chars_end = current_chunk_chars_cur + sizeof (string_chunk_p->data) / sizeof (ecma_char_t);
  }
  else
  {
    JERRY_ASSERT (ecma_string_p->container == ECMA_STRING_CONTAINER_LIT_TABLE);

    JERRY_UNIMPLEMENTED();
  }

  for (ecma_length_t str_index = 0;
       str_index < ecma_str_len;
       str_index++, str_iter_p++, current_chunk_chars_cur++)
  {
    JERRY_ASSERT(current_chunk_chars_cur <= current_chunk_chars_end);

    if (current_chunk_chars_cur == current_chunk_chars_end)
    {
      JERRY_ASSERT (ecma_string_p->container == ECMA_STRING_CONTAINER_HEAP);

      /* switching to next chunk */
      string_chunk_p = ECMA_GET_POINTER (string_chunk_p->next_chunk_cp);

      JERRY_ASSERT(string_chunk_p != NULL);

      current_chunk_chars_cur = string_chunk_p->data;
      current_chunk_chars_end = current_chunk_chars_cur + sizeof (string_chunk_p->data) / sizeof (ecma_char_t);
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
  return (*str_iter_p == '\0');
} /* ecma_compare_zt_string_to_ecma_string */

/**
 * @}
 * @}
 */
