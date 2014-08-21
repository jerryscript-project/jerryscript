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

#include "deserializer.h"
#include "ecma-alloc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "globals.h"
#include "jerry-libc.h"
#include "interpreter.h"

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
  string_desc_p->is_length_valid = true;

  if (bytes_needed_for_current_string <= bytes_for_chars_in_string_descriptor)
  {
    string_desc_p->container = ECMA_STRING_CONTAINER_IN_DESCRIPTOR;
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
ecma_new_ecma_string_from_number (ecma_number_t num) /**< ecma-number */
{
  ecma_string_t* string_desc_p = ecma_alloc_string ();
  string_desc_p->refs = 1;
  string_desc_p->length = 0;
  string_desc_p->is_length_valid = false;
  string_desc_p->container = ECMA_STRING_CONTAINER_HEAP_NUMBER;

  ecma_number_t *num_p = ecma_alloc_number ();
  *num_p = num;
  ECMA_SET_POINTER (string_desc_p->u.number_cp, num_p);

  return string_desc_p;
} /* ecma_new_ecma_string_from_number */

/**
 * Allocate new ecma-string and fill it with reference string literal
 *
 * @return pointer to ecma-string descriptor
 */
ecma_string_t*
ecma_new_ecma_string_from_lit_index (literal_index_t lit_index) /**< ecma-number */
{
  ecma_string_t* string_desc_p = ecma_alloc_string ();
  string_desc_p->refs = 1;

  FIXME (/* Interface for getting literal's length */);
  TODO (/* Lazy calculate literal's length */);

  ssize_t size_required = try_get_string_by_idx ((uint8_t) lit_index, NULL, 0);
  JERRY_ASSERT (size_required < 0);

  string_desc_p->length = (ecma_length_t) ((size_t)-size_required / sizeof (ecma_char_t) - 1);
  string_desc_p->is_length_valid = true;
  string_desc_p->container = ECMA_STRING_CONTAINER_LIT_TABLE;

  string_desc_p->u.lit_index = lit_index;

  return string_desc_p;
} /* ecma_new_ecma_string_from_lit_index */

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
 * Decrease reference counter and deallocate ecma-string if
 * after that the counter the counter becomes zero.
 */
void
ecma_deref_ecma_string (ecma_string_t *string_p) /**< ecma-string */
{
  JERRY_ASSERT (string_p != NULL);
  JERRY_ASSERT (string_p->refs != 0);

  string_p->refs--;

  if (string_p->refs != 0)
  {
    return;
  }

  if (string_p->container == ECMA_STRING_CONTAINER_HEAP_CHUNKS)
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
  else if (string_p->container == ECMA_STRING_CONTAINER_HEAP_NUMBER)
  {
    ecma_number_t *num_p = ECMA_GET_POINTER (string_p->u.number_cp);

    ecma_dealloc_number (num_p);
  }
  else
  {
    JERRY_ASSERT (string_p->container == ECMA_STRING_CONTAINER_IN_DESCRIPTOR
                  || string_p->container == ECMA_STRING_CONTAINER_LIT_TABLE);

    /* only the string descriptor itself should be freed */
  }

  ecma_dealloc_string (string_p);
} /* ecma_deref_ecma_string */

/**
 * Get length of the ecma-string
 *
 * @return length
 */
ecma_length_t
ecma_get_ecma_string_length (ecma_string_t *string_desc_p) /**< ecma-string descriptor */
{
  switch ((ecma_string_container_t)string_desc_p->container)
  {
    case ECMA_STRING_CONTAINER_HEAP_CHUNKS:
    case ECMA_STRING_CONTAINER_LIT_TABLE:
    case ECMA_STRING_CONTAINER_IN_DESCRIPTOR:
    {
      JERRY_ASSERT (string_desc_p->is_length_valid);

      return string_desc_p->length;
    }

    case ECMA_STRING_CONTAINER_HEAP_NUMBER:
    {
      if (string_desc_p->is_length_valid)
      {
        return string_desc_p->length;
      }
      else
      {
        /* calculate length */
        JERRY_UNIMPLEMENTED();
      }
    }
  }

  JERRY_UNREACHABLE();
} /* ecma_get_ecma_string_length */

/**
 * Convert ecma-string to number
 */
ecma_number_t
ecma_string_to_number (ecma_string_t *str_p) /**< ecma-string */
{
  JERRY_ASSERT (str_p != NULL);

  if (str_p->container == ECMA_STRING_CONTAINER_HEAP_NUMBER)
  {
    ecma_number_t *num_p = ECMA_GET_POINTER (str_p->u.number_cp);

    return *num_p;
  }
  else
  {
    JERRY_ASSERT (str_p->is_length_valid);

    ecma_char_t zt_string_buffer [str_p->length + 1];

    ssize_t bytes_copied = ecma_string_to_zt_string (str_p,
                                                     zt_string_buffer,
                                                     (ssize_t) sizeof (zt_string_buffer));
    JERRY_ASSERT (bytes_copied > 0);

    return ecma_zt_string_to_number (zt_string_buffer);
  }
} /* ecma_string_to_number */

/**
 * Get size of buffer required to store the ecma-string in zt-form.
 */
static ssize_t
ecma_string_get_required_buffer_size_for_zt_form (const ecma_string_t *string_desc_p) /**< ecma-string */
{
  ecma_length_t string_length = 0;

  switch ((ecma_string_container_t)string_desc_p->container)
  {
    case ECMA_STRING_CONTAINER_IN_DESCRIPTOR:
    case ECMA_STRING_CONTAINER_HEAP_CHUNKS:
    case ECMA_STRING_CONTAINER_LIT_TABLE:
    {
      JERRY_ASSERT (string_desc_p->is_length_valid);

      string_length = string_desc_p->length;

      break;
    }

    case ECMA_STRING_CONTAINER_HEAP_NUMBER:
    {
      string_length = (string_desc_p->is_length_valid ?
                       string_desc_p->length : ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER);

      break;
    }
  }

  return (ssize_t) ((ssize_t) sizeof (ecma_char_t) * (string_length + 1));
} /* ecma_string_get_required_buffer_size_for_zt_form */

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
                          ssize_t buffer_size) /**< size of buffer */
{
  JERRY_ASSERT (string_desc_p != NULL);
  JERRY_ASSERT (string_desc_p->refs > 0);
  JERRY_ASSERT (buffer_p != NULL);
  JERRY_ASSERT (buffer_size > 0);

  ssize_t required_buffer_size = ecma_string_get_required_buffer_size_for_zt_form (string_desc_p);
  ssize_t bytes_copied = 0;

  if (required_buffer_size > buffer_size)
  {
    return -required_buffer_size;
  }

  ecma_char_t *dest_p = buffer_p;

  switch ((ecma_string_container_t)string_desc_p->container)
  {
    case ECMA_STRING_CONTAINER_IN_DESCRIPTOR:
    {
      JERRY_ASSERT (string_desc_p->is_length_valid);
      ecma_length_t string_length = string_desc_p->length;

      __memcpy (dest_p, string_desc_p->u.chars, string_length * sizeof (ecma_char_t));
      dest_p += string_length;
      *dest_p++ = '\0';
      bytes_copied = (dest_p - buffer_p) * ((ssize_t) sizeof (ecma_char_t));

      break;
    }
    case ECMA_STRING_CONTAINER_HEAP_CHUNKS:
    {
      JERRY_ASSERT (string_desc_p->is_length_valid);
      ecma_length_t string_length = string_desc_p->length;

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
      *dest_p++ = '\0';
      bytes_copied = (dest_p - buffer_p) * ((ssize_t) sizeof (ecma_char_t));

      break;
    }
    case ECMA_STRING_CONTAINER_LIT_TABLE:
    {
      bytes_copied = try_get_string_by_idx ((uint8_t) string_desc_p->u.lit_index,
                                            buffer_p,
                                            buffer_size);

      break;
    }
    case ECMA_STRING_CONTAINER_HEAP_NUMBER:
    {
      const ssize_t buffer_size_required = (ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER + 1) * sizeof (ecma_char_t);
      if (buffer_size_required < buffer_size)
      {
        return -(ssize_t) buffer_size_required;
      }

      ecma_number_t *num_p = ECMA_GET_POINTER (string_desc_p->u.number_cp);

      ecma_length_t length = ecma_number_to_zt_string (*num_p, buffer_p, buffer_size);

      if (!string_desc_p->is_length_valid)
      {
        string_desc_p->length = length;
        string_desc_p->is_length_valid = true;
      }

      JERRY_ASSERT (string_desc_p->is_length_valid
                    && string_desc_p->length == length);

      bytes_copied = (length + 1) * ((ssize_t) sizeof (ecma_char_t));
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
  JERRY_ASSERT (string1_p->is_length_valid);
  JERRY_ASSERT (string2_p->is_length_valid);
  JERRY_ASSERT (string1_p->length == string2_p->length);

  ecma_collection_chunk_t *string1_chunk_p = ECMA_GET_POINTER (string1_p->u.chunk_cp);
  ecma_collection_chunk_t *string2_chunk_p = ECMA_GET_POINTER (string2_p->u.chunk_cp);

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
 * Compare ecma-string to zt-string
 *
 * @return true - if strings are equal;
 *         false - otherwise.
 */
static bool __noinline
ecma_compare_ecma_string_to_zt_string (const ecma_string_t *string_p, /**< ecma-string */
                                       const ecma_char_t *zt_string_p) /**< zt-string */
{
  JERRY_ASSERT (string_p->container == ECMA_STRING_CONTAINER_HEAP_CHUNKS);
  JERRY_ASSERT (string_p->is_length_valid);

  ecma_collection_chunk_t *string_chunk_p = ECMA_GET_POINTER (string_p->u.chunk_cp);

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
 *          ecma_compare_ecma_string_to_ecma_string
 *
 * @return true - if strings are equal;
 *         false - otherwise.
 */
static bool __noinline
ecma_compare_ecma_string_to_ecma_string_longpath (ecma_string_t *string1_p, /* ecma-string */
                                                  ecma_string_t *string2_p) /* ecma-string */
{
  if (string1_p->container == ECMA_STRING_CONTAINER_HEAP_CHUNKS)
  {
    ecma_string_t *tmp_string_p = string1_p;
    string1_p = string2_p;
    string2_p = tmp_string_p;
  }
  JERRY_ASSERT (string1_p->container != ECMA_STRING_CONTAINER_HEAP_CHUNKS);

  const ecma_char_t *zt_string1_p;
  bool is_zt_string1_on_heap = false;
  ecma_char_t zt_string1_buffer [ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER];

  if (string1_p->container == ECMA_STRING_CONTAINER_LIT_TABLE)
  {
    FIXME (uint8_t -> literal_index_t);
    zt_string1_p = deserialize_string_by_id ((uint8_t) string1_p->u.lit_index);
  }
  else
  {
    ssize_t req_size = ecma_string_to_zt_string (string1_p,
                                                 zt_string1_buffer,
                                                 sizeof (zt_string1_buffer));

    if (req_size < 0)
    {
      ecma_char_t *heap_buffer_p = mem_heap_alloc_block ((size_t) -req_size, MEM_HEAP_ALLOC_SHORT_TERM);

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
    ecma_char_t zt_string2_buffer [ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER];

    if (string2_p->container == ECMA_STRING_CONTAINER_LIT_TABLE)
    {
      FIXME (uint8_t -> literal_index_t);
      zt_string2_p = deserialize_string_by_id ((uint8_t) string2_p->u.lit_index);
    }
    else
    {
      ssize_t req_size = ecma_string_to_zt_string (string2_p,
                                                   zt_string2_buffer,
                                                   sizeof (zt_string2_buffer));

      if (req_size < 0)
      {
        ecma_char_t *heap_buffer_p = mem_heap_alloc_block ((size_t) -req_size, MEM_HEAP_ALLOC_SHORT_TERM);

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

    is_equal = ecma_compare_zt_string_to_zt_string (zt_string1_p, zt_string2_p);

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
} /* ecma_compare_ecma_string_to_ecma_string_longpath */

/**
 * Compare ecma-string to ecma-string
 *
 * @return true - if strings are equal;
 *         false - otherwise.
 */
bool
ecma_compare_ecma_string_to_ecma_string (ecma_string_t *string1_p, /* ecma-string */
                                         ecma_string_t *string2_p) /* ecma-string */
{
  JERRY_ASSERT (string1_p != NULL && string2_p != NULL);

  if (unlikely (string1_p == string2_p))
  {
    return true;
  }

  if (string1_p->is_length_valid
      && string2_p->is_length_valid)
  {
    if (likely (string1_p->length != string2_p->length))
    {
      return false;
    }
  }

  if (string1_p->container == string2_p->container)
  {
    if (likely (string1_p->container == ECMA_STRING_CONTAINER_LIT_TABLE))
    {
      return (string1_p->u.lit_index == string2_p->u.lit_index);
    }
    else if (string1_p->container == ECMA_STRING_CONTAINER_HEAP_NUMBER)
    {
      ecma_number_t *num1_p, *num2_p;
      num1_p = ECMA_GET_POINTER (string1_p->u.number_cp);
      num2_p = ECMA_GET_POINTER (string2_p->u.number_cp);

      if (ecma_number_is_nan (*num1_p)
          && ecma_number_is_nan (*num2_p))
      {
        return true;
      }

      return (*num1_p == *num2_p);
    }
    else if (string1_p->container == ECMA_STRING_CONTAINER_IN_DESCRIPTOR)
    {
      JERRY_ASSERT (string1_p->is_length_valid);
      JERRY_ASSERT (string2_p->is_length_valid);
      JERRY_ASSERT (string1_p->length == string2_p->length);

      return (__memcmp (string1_p->u.chars, string2_p->u.chars, string1_p->length * sizeof (ecma_char_t)) == 0);
    }
    else if (string1_p->container == ECMA_STRING_CONTAINER_HEAP_CHUNKS)
    {
      return ecma_compare_strings_in_heap_chunks (string1_p, string2_p);
    }
    else
    {
      JERRY_UNREACHABLE ();
    }
  }

  return ecma_compare_ecma_string_to_ecma_string_longpath (string1_p, string2_p);
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
 * @}
 * @}
 */
