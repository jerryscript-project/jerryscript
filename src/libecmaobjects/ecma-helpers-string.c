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
  string_desc_p->length = length;
  string_desc_p->container = ECMA_STRING_CONTAINER_HEAP_NUMBER;

  ecma_number_t *num_p = ecma_alloc_number ();
  *num_p = num;
  ECMA_SET_POINTER (string_desc_p->u.number_cp, num_p);

  return string_desc_p;
} /* ecma_new_ecma_string_from_number */

/**
 * Allocate new ecma-string and fill it with reference to string literal
 *
 * @return pointer to ecma-string descriptor
 */
ecma_string_t*
ecma_new_ecma_string_from_lit_index (literal_index_t lit_index) /**< ecma-number */
{
  ecma_string_t* string_desc_p = ecma_alloc_string ();
  string_desc_p->refs = 1;

  FIXME (/* Interface for getting literal's length without string's characters iteration */);
  const ecma_char_t *str_p = deserialize_string_by_id ((idx_t) lit_index);
  JERRY_ASSERT (str_p != NULL);
  ecma_length_t length = (ecma_length_t) __strlen ((const char*)str_p);

  string_desc_p->length = length;
  string_desc_p->container = ECMA_STRING_CONTAINER_LIT_TABLE;

  string_desc_p->u.lit_index = lit_index;

  return string_desc_p;
} /* ecma_new_ecma_string_from_lit_index */

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

  string_desc_p->refs++;

  if (unlikely (string_desc_p->refs == 0))
  {
    string_desc_p->refs--;

    if (string_desc_p->container == ECMA_STRING_CONTAINER_CHARS_IN_DESC
        || string_desc_p->container == ECMA_STRING_CONTAINER_LIT_TABLE
        || string_desc_p->container == ECMA_STRING_CONTAINER_UINT32_IN_DESC)
    {
      ecma_string_t *new_str_p = ecma_alloc_string ();

      *new_str_p = *string_desc_p;

      new_str_p->refs = 1;

      return new_str_p;
    }
    else
    {
      JERRY_UNIMPLEMENTED ();
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

  string_p->refs--;

  if (string_p->refs != 0)
  {
    return;
  }

  switch ((ecma_string_container_t)string_p->container)
  {
    case ECMA_STRING_CONTAINER_HEAP_CHUNKS:
    {
      ecma_collection_chunk_t *chunk_p = ECMA_GET_POINTER (string_p->u.chunk_cp);

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
      ecma_number_t *num_p = ECMA_GET_POINTER (string_p->u.number_cp);

      ecma_dealloc_number (num_p);

      break;
    }
    case ECMA_STRING_CONTAINER_CONCATENATION:
    {
      ecma_string_t *string1_p, *string2_p;

      string1_p = ECMA_GET_POINTER (string_p->u.concatenation.string1_cp);
      string2_p = ECMA_GET_POINTER (string_p->u.concatenation.string2_cp);

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

  ecma_dealloc_string (string_p);
} /* ecma_deref_ecma_string */

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
      ecma_number_t *num_p = ECMA_GET_POINTER (str_p->u.number_cp);

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

      ecma_collection_chunk_t *string_chunk_p = ECMA_GET_POINTER (string_desc_p->u.chunk_cp);

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
      const ecma_char_t *str_p = deserialize_string_by_id ((idx_t) string_desc_p->u.lit_index);
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
      ecma_number_t *num_p = ECMA_GET_POINTER (string_desc_p->u.number_cp);

      ecma_length_t length = ecma_number_to_zt_string (*num_p, buffer_p, buffer_size);

      JERRY_ASSERT (ecma_string_get_length (string_desc_p) == length);

      bytes_copied = (length + 1) * ((ssize_t) sizeof (ecma_char_t));

      break;
    }
    case ECMA_STRING_CONTAINER_CONCATENATION:
    {
      const ecma_string_t *string1_p = ECMA_GET_POINTER (string_desc_p->u.concatenation.string1_cp);
      const ecma_string_t *string2_p = ECMA_GET_POINTER (string_desc_p->u.concatenation.string2_cp);

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
 *          ecma_compare_ecma_strings
 *
 * @return true - if strings are equal;
 *         false - otherwise.
 */
static bool __noinline
ecma_compare_ecma_strings_longpath (const ecma_string_t *string1_p, /* ecma-string */
                                    const ecma_string_t *string2_p) /* ecma-string */
{
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

  if (likely (ecma_string_get_length (string1_p) != ecma_string_get_length (string2_p)))
  {
    return false;
  }

  if (string1_p->container == string2_p->container)
  {
    if (likely (string1_p->container == ECMA_STRING_CONTAINER_LIT_TABLE))
    {
      return (string1_p->u.lit_index == string2_p->u.lit_index);
    }

    switch ((ecma_string_container_t) string1_p->container)
    {
      case ECMA_STRING_CONTAINER_HEAP_NUMBER:
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
      case ECMA_STRING_CONTAINER_UINT32_IN_DESC:
      {
        return (string1_p->u.uint32_number == string2_p->u.uint32_number);
      }
      case ECMA_STRING_CONTAINER_MAGIC_STRING:
      {
        return (string1_p->u.magic_string_id == string2_p->u.magic_string_id);
      }
      case ECMA_STRING_CONTAINER_CONCATENATION:
      {
        /* long path */
        break;
      }
      case ECMA_STRING_CONTAINER_LIT_TABLE:
      {
        JERRY_UNREACHABLE ();
      }
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

    string1_p = ECMA_GET_POINTER (string_p->u.concatenation.string1_cp);
    string2_p = ECMA_GET_POINTER (string_p->u.concatenation.string2_cp);

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
#ifdef CONFIG_ECMA_CHAR_ASCII
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
#elif defined (CONFIG_ECMA_CHAR_UTF16)
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
#else /* !CONFIG_ECMA_CHAR_ASCII && !CONFIG_ECMA_CHAR_UTF16 */
# error "!CONFIG_ECMA_CHAR_ASCII && !CONFIG_ECMA_CHAR_UTF16"
#endif /* !CONFIG_ECMA_CHAR_ASCII && !CONFIG_ECMA_CHAR_UTF16 */
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
 * Allocate new ecma-string and fill it with reference to ECMA magic string
 *
 * @return pointer to ecma-string descriptor
 */
ecma_string_t*
ecma_new_ecma_string_from_magic_string_id (ecma_magic_string_id_t id) /**< identifier of magic string */
{
  JERRY_ASSERT (id < ECMA_MAGIC_STRING__COUNT);

  ecma_string_t* string_desc_p = ecma_alloc_string ();
  string_desc_p->refs = 1;

  string_desc_p->length = ecma_magic_string_lengths [id];
  string_desc_p->container = ECMA_STRING_CONTAINER_MAGIC_STRING;

  string_desc_p->u.magic_string_id = id;

  return string_desc_p;
} /* ecma_new_ecma_string_from_magic_string_id */

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
    case ECMA_MAGIC_STRING_ARGUMENTS: return (ecma_char_t*) "arguments";
    case ECMA_MAGIC_STRING_EVAL: return (ecma_char_t*) "eval";
    case ECMA_MAGIC_STRING_PROTOTYPE: return (ecma_char_t*) "prototype";
    case ECMA_MAGIC_STRING_CONSTRUCTOR: return (ecma_char_t*) "constructor";
    case ECMA_MAGIC_STRING_CALLER: return (ecma_char_t*) "caller";
    case ECMA_MAGIC_STRING_CALLEE: return (ecma_char_t*) "callee";
    case ECMA_MAGIC_STRING_UNDEFINED: return (ecma_char_t*) "undefined";
    case ECMA_MAGIC_STRING_NULL: return (ecma_char_t*) "null";
    case ECMA_MAGIC_STRING_FALSE: return (ecma_char_t*) "false";
    case ECMA_MAGIC_STRING_TRUE: return (ecma_char_t*) "true";
    case ECMA_MAGIC_STRING_BOOLEAN: return (ecma_char_t*) "boolean";
    case ECMA_MAGIC_STRING_NUMBER: return (ecma_char_t*) "number";
    case ECMA_MAGIC_STRING_STRING: return (ecma_char_t*) "string";
    case ECMA_MAGIC_STRING_OBJECT: return (ecma_char_t*) "object";
    case ECMA_MAGIC_STRING_FUNCTION: return (ecma_char_t*) "function";
    case ECMA_MAGIC_STRING_LENGTH: return (ecma_char_t*) "length";
    case ECMA_MAGIC_STRING_NAN: return (ecma_char_t*) "NaN";
    case ECMA_MAGIC_STRING_INFINITY_UL: return (ecma_char_t*) "Infinity";
    case ECMA_MAGIC_STRING_UNDEFINED_UL: return (ecma_char_t*) "Undefined";
    case ECMA_MAGIC_STRING_NULL_UL: return (ecma_char_t*) "Null";
    case ECMA_MAGIC_STRING_OBJECT_UL: return (ecma_char_t*) "Object";
    case ECMA_MAGIC_STRING_FUNCTION_UL: return (ecma_char_t*) "Function";
    case ECMA_MAGIC_STRING_ARRAY_UL: return (ecma_char_t*) "Array";
    case ECMA_MAGIC_STRING_ARGUMENTS_UL: return (ecma_char_t*) "Arguments";
    case ECMA_MAGIC_STRING_STRING_UL: return (ecma_char_t*) "String";
    case ECMA_MAGIC_STRING_BOOLEAN_UL: return (ecma_char_t*) "Boolean";
    case ECMA_MAGIC_STRING_NUMBER_UL: return (ecma_char_t*) "Number";
    case ECMA_MAGIC_STRING_DATE_UL: return (ecma_char_t*) "Date";
    case ECMA_MAGIC_STRING_REG_EXP_UL: return (ecma_char_t*) "RegExp";
    case ECMA_MAGIC_STRING_ERROR_UL: return (ecma_char_t*) "Error";
    case ECMA_MAGIC_STRING_EVAL_ERROR_UL: return (ecma_char_t*) "EvalError";
    case ECMA_MAGIC_STRING_RANGE_ERROR_UL: return (ecma_char_t*) "RangeError";
    case ECMA_MAGIC_STRING_REFERENCE_ERROR_UL: return (ecma_char_t*) "ReferenceError";
    case ECMA_MAGIC_STRING_SYNTAX_ERROR_UL: return (ecma_char_t*) "SyntaxError";
    case ECMA_MAGIC_STRING_TYPE_ERROR_UL: return (ecma_char_t*) "TypeError";
    case ECMA_MAGIC_STRING_URI_ERROR_UL: return (ecma_char_t*) "URIError";
    case ECMA_MAGIC_STRING_MATH_UL: return (ecma_char_t*) "Math";
    case ECMA_MAGIC_STRING_JSON_U: return (ecma_char_t*) "JSON";
    case ECMA_MAGIC_STRING_PARSE_INT: return (ecma_char_t*) "parseInt";
    case ECMA_MAGIC_STRING_PARSE_FLOAT: return (ecma_char_t*) "parseFloat";
    case ECMA_MAGIC_STRING_IS_NAN: return (ecma_char_t*) "isNaN";
    case ECMA_MAGIC_STRING_IS_FINITE: return (ecma_char_t*) "isFinite";
    case ECMA_MAGIC_STRING_DECODE_URI: return (ecma_char_t*) "decodeURI";
    case ECMA_MAGIC_STRING_DECODE_URI_COMPONENT: return (ecma_char_t*) "decodeURIComponent";
    case ECMA_MAGIC_STRING_ENCODE_URI: return (ecma_char_t*) "encodeURI";
    case ECMA_MAGIC_STRING_ENCODE_URI_COMPONENT: return (ecma_char_t*) "encodeURIComponent";
    case ECMA_MAGIC_STRING_GET_PROTOTYPE_OF_UL: return (ecma_char_t*) "getPrototypeOf";
    case ECMA_MAGIC_STRING_GET_OWN_PROPERTY_DESCRIPTOR_UL: return (ecma_char_t*) "getOwnPropertyDescriptor";
    case ECMA_MAGIC_STRING_GET_OWN_PROPERTY_NAMES_UL: return (ecma_char_t*) "getOwnPropertyNames";
    case ECMA_MAGIC_STRING_CREATE: return (ecma_char_t*) "create";
    case ECMA_MAGIC_STRING_DEFINE_PROPERTY_UL: return (ecma_char_t*) "defineProperty";
    case ECMA_MAGIC_STRING_DEFINE_PROPERTIES_UL: return (ecma_char_t*) "defineProperties";
    case ECMA_MAGIC_STRING_SEAL: return (ecma_char_t*) "seal";
    case ECMA_MAGIC_STRING_FREEZE: return (ecma_char_t*) "freeze";
    case ECMA_MAGIC_STRING_PREVENT_EXTENSIONS_UL: return (ecma_char_t*) "preventExtensions";
    case ECMA_MAGIC_STRING_IS_SEALED_UL: return (ecma_char_t*) "isSealed";
    case ECMA_MAGIC_STRING_IS_FROZEN_UL: return (ecma_char_t*) "isFrozen";
    case ECMA_MAGIC_STRING_IS_EXTENSIBLE: return (ecma_char_t*) "isExtensible";
    case ECMA_MAGIC_STRING_KEYS: return (ecma_char_t*) "keys";
    case ECMA_MAGIC_STRING_WRITABLE: return (ecma_char_t*) "writable";
    case ECMA_MAGIC_STRING_ENUMERABLE: return (ecma_char_t*) "enumerable";
    case ECMA_MAGIC_STRING_CONFIGURABLE: return (ecma_char_t*) "configurable";
    case ECMA_MAGIC_STRING_VALUE: return (ecma_char_t*) "value";
    case ECMA_MAGIC_STRING_GET: return (ecma_char_t*) "get";
    case ECMA_MAGIC_STRING_SET: return (ecma_char_t*) "set";
    case ECMA_MAGIC_STRING_E_U: return (ecma_char_t*) "E";
    case ECMA_MAGIC_STRING_LN10_U: return (ecma_char_t*) "LN10";
    case ECMA_MAGIC_STRING_LN2_U: return (ecma_char_t*) "LN2";
    case ECMA_MAGIC_STRING_LOG2E_U: return (ecma_char_t*) "LOG2E";
    case ECMA_MAGIC_STRING_LOG10E_U: return (ecma_char_t*) "LOG10E";
    case ECMA_MAGIC_STRING_PI_U: return (ecma_char_t*) "PI";
    case ECMA_MAGIC_STRING_SQRT1_2_U: return (ecma_char_t*) "SQRT1_2";
    case ECMA_MAGIC_STRING_SQRT2_U: return (ecma_char_t*) "SQRT2";
    case ECMA_MAGIC_STRING_ABS: return (ecma_char_t*) "abs";
    case ECMA_MAGIC_STRING_ACOS: return (ecma_char_t*) "acos";
    case ECMA_MAGIC_STRING_ASIN: return (ecma_char_t*) "asin";
    case ECMA_MAGIC_STRING_ATAN: return (ecma_char_t*) "atan";
    case ECMA_MAGIC_STRING_ATAN2: return (ecma_char_t*) "atan2";
    case ECMA_MAGIC_STRING_CEIL: return (ecma_char_t*) "ceil";
    case ECMA_MAGIC_STRING_COS: return (ecma_char_t*) "cos";
    case ECMA_MAGIC_STRING_EXP: return (ecma_char_t*) "exp";
    case ECMA_MAGIC_STRING_FLOOR: return (ecma_char_t*) "floor";
    case ECMA_MAGIC_STRING_LOG: return (ecma_char_t*) "log";
    case ECMA_MAGIC_STRING_MAX: return (ecma_char_t*) "max";
    case ECMA_MAGIC_STRING_MIN: return (ecma_char_t*) "min";
    case ECMA_MAGIC_STRING_POW: return (ecma_char_t*) "pow";
    case ECMA_MAGIC_STRING_RANDOM: return (ecma_char_t*) "random";
    case ECMA_MAGIC_STRING_ROUND: return (ecma_char_t*) "round";
    case ECMA_MAGIC_STRING_SIN: return (ecma_char_t*) "sin";
    case ECMA_MAGIC_STRING_SQRT: return (ecma_char_t*) "sqrt";
    case ECMA_MAGIC_STRING_TAN: return (ecma_char_t*) "tan";
    case ECMA_MAGIC_STRING_FROM_CHAR_CODE_UL: return (ecma_char_t*) "fromCharCode";
    case ECMA_MAGIC_STRING_IS_ARRAY_UL: return (ecma_char_t*) "isArray";
    case ECMA_MAGIC_STRING_TO_STRING_UL: return (ecma_char_t*) "toString";
    case ECMA_MAGIC_STRING_VALUE_OF_UL: return (ecma_char_t*) "valueOf";
    case ECMA_MAGIC_STRING_TO_LOCALE_STRING_UL: return (ecma_char_t*) "toLocaleString";
    case ECMA_MAGIC_STRING_HAS_OWN_PROPERTY_UL: return (ecma_char_t*) "hasOwnProperty";
    case ECMA_MAGIC_STRING_IS_PROTOTYPE_OF_UL: return (ecma_char_t*) "isPrototypeOf";
    case ECMA_MAGIC_STRING_PROPERTY_IS_ENUMERABLE_UL: return (ecma_char_t*) "propertyIsEnumerable";
    case ECMA_MAGIC_STRING_CONCAT: return (ecma_char_t*) "concat";
    case ECMA_MAGIC_STRING_POP: return (ecma_char_t*) "pop";
    case ECMA_MAGIC_STRING_JOIN: return (ecma_char_t*) "join";
    case ECMA_MAGIC_STRING_PUSH: return (ecma_char_t*) "push";
    case ECMA_MAGIC_STRING_REVERSE: return (ecma_char_t*) "reverse";
    case ECMA_MAGIC_STRING_SHIFT: return (ecma_char_t*) "shift";
    case ECMA_MAGIC_STRING_SLICE: return (ecma_char_t*) "slice";
    case ECMA_MAGIC_STRING_SORT: return (ecma_char_t*) "sort";
    case ECMA_MAGIC_STRING_SPLICE: return (ecma_char_t*) "splice";
    case ECMA_MAGIC_STRING_UNSHIFT: return (ecma_char_t*) "unshift";
    case ECMA_MAGIC_STRING_INDEX_OF_UL: return (ecma_char_t*) "indexOf";
    case ECMA_MAGIC_STRING_LAST_INDEX_OF_UL: return (ecma_char_t*) "lastIndexOf";
    case ECMA_MAGIC_STRING_EVERY: return (ecma_char_t*) "every";
    case ECMA_MAGIC_STRING_SOME: return (ecma_char_t*) "some";
    case ECMA_MAGIC_STRING_FOR_EACH_UL: return (ecma_char_t*) "forEach";
    case ECMA_MAGIC_STRING_MAP: return (ecma_char_t*) "map";
    case ECMA_MAGIC_STRING_FILTER: return (ecma_char_t*) "filter";
    case ECMA_MAGIC_STRING_REDUCE: return (ecma_char_t*) "reduce";
    case ECMA_MAGIC_STRING_REDUCE_RIGHT_UL: return (ecma_char_t*) "reduceRight";
    case ECMA_MAGIC_STRING_CHAR_AT_UL: return (ecma_char_t*) "charAt";
    case ECMA_MAGIC_STRING_CHAR_CODE_AT_UL: return (ecma_char_t*) "charCodeAt";
    case ECMA_MAGIC_STRING_LOCALE_COMPARE_UL: return (ecma_char_t*) "localeCompare";
    case ECMA_MAGIC_STRING_MATCH: return (ecma_char_t*) "match";
    case ECMA_MAGIC_STRING_REPLACE: return (ecma_char_t*) "replace";
    case ECMA_MAGIC_STRING_SEARCH: return (ecma_char_t*) "search";
    case ECMA_MAGIC_STRING_SPLIT: return (ecma_char_t*) "split";
    case ECMA_MAGIC_STRING_SUBSTRING: return (ecma_char_t*) "substring";
    case ECMA_MAGIC_STRING_TO_LOWER_CASE_UL: return (ecma_char_t*) "toLowerCase";
    case ECMA_MAGIC_STRING_TO_LOCALE_LOWER_CASE_UL: return (ecma_char_t*) "toLocaleLowerCase";
    case ECMA_MAGIC_STRING_TO_UPPER_CASE_UL: return (ecma_char_t*) "toUpperCase";
    case ECMA_MAGIC_STRING_TO_LOCALE_UPPER_CASE_UL: return (ecma_char_t*) "toLocaleUpperCase";
    case ECMA_MAGIC_STRING_TRIM: return (ecma_char_t*) "trim";
    case ECMA_MAGIC_STRING_TO_FIXED_UL: return (ecma_char_t*) "toFixed";
    case ECMA_MAGIC_STRING_TO_EXPONENTIAL_UL: return (ecma_char_t*) "toExponential";
    case ECMA_MAGIC_STRING_TO_PRECISION_UL: return (ecma_char_t*) "toPrecision";
    case ECMA_MAGIC_STRING_TO_DATE_STRING_UL: return (ecma_char_t*) "toDateString";
    case ECMA_MAGIC_STRING_TO_TIME_STRING_UL: return (ecma_char_t*) "toTimeString";
    case ECMA_MAGIC_STRING_TO_LOCALE_DATE_STRING_UL: return (ecma_char_t*) "toLocaleDateString";
    case ECMA_MAGIC_STRING_TO_LOCALE_TIME_STRING_UL: return (ecma_char_t*) "toLocaleTimeString";
    case ECMA_MAGIC_STRING_GET_TIME_UL: return (ecma_char_t*) "getTime";
    case ECMA_MAGIC_STRING_GET_FULL_YEAR_UL: return (ecma_char_t*) "getFullYear";
    case ECMA_MAGIC_STRING_GET_UTC_FULL_YEAR_UL: return (ecma_char_t*) "getUTCFullYear";
    case ECMA_MAGIC_STRING_GET_MONTH_UL: return (ecma_char_t*) "getMonth";
    case ECMA_MAGIC_STRING_GET_UTC_MONTH_UL: return (ecma_char_t*) "getUTCMonth";
    case ECMA_MAGIC_STRING_GET_DATE_UL: return (ecma_char_t*) "getDate";
    case ECMA_MAGIC_STRING_GET_UTC_DATE_UL: return (ecma_char_t*) "getUTCDate";
    case ECMA_MAGIC_STRING_GET_DAY_UL: return (ecma_char_t*) "getDay";
    case ECMA_MAGIC_STRING_GET_UTC_DAY_UL: return (ecma_char_t*) "getUTCDay";
    case ECMA_MAGIC_STRING_GET_HOURS_UL: return (ecma_char_t*) "getHours";
    case ECMA_MAGIC_STRING_GET_UTC_HOURS_UL: return (ecma_char_t*) "getUTCHours";
    case ECMA_MAGIC_STRING_GET_MINUTES_UL: return (ecma_char_t*) "getMinutes";
    case ECMA_MAGIC_STRING_GET_UTC_MINUTES_UL: return (ecma_char_t*) "getUTCMinutes";
    case ECMA_MAGIC_STRING_GET_SECONDS_UL: return (ecma_char_t*) "getSeconds";
    case ECMA_MAGIC_STRING_GET_UTC_SECONDS_UL: return (ecma_char_t*) "getUTCSeconds";
    case ECMA_MAGIC_STRING_GET_MILLISECONDS_UL: return (ecma_char_t*) "getMilliseconds";
    case ECMA_MAGIC_STRING_GET_UTC_MILLISECONDS_UL: return (ecma_char_t*) "getUTCMilliseconds";
    case ECMA_MAGIC_STRING_GET_TIMEZONE_OFFSET_UL: return (ecma_char_t*) "getTimezoneOffset";
    case ECMA_MAGIC_STRING_SET_TIME_UL: return (ecma_char_t*) "setTime";
    case ECMA_MAGIC_STRING_SET_MILLISECONDS_UL: return (ecma_char_t*) "setMilliseconds";
    case ECMA_MAGIC_STRING_SET_UTC_MILLISECONDS_UL: return (ecma_char_t*) "setUTCMilliseconds";
    case ECMA_MAGIC_STRING_SET_SECONDS_UL: return (ecma_char_t*) "setSeconds";
    case ECMA_MAGIC_STRING_SET_UTC_SECONDS_UL: return (ecma_char_t*) "setUTCSeconds";
    case ECMA_MAGIC_STRING_SET_MINUTES_UL: return (ecma_char_t*) "setMinutes";
    case ECMA_MAGIC_STRING_SET_UTC_MINUTES_UL: return (ecma_char_t*) "setUTCMinutes";
    case ECMA_MAGIC_STRING_SET_HOURS_UL: return (ecma_char_t*) "setHours";
    case ECMA_MAGIC_STRING_SET_UTC_HOURS_UL: return (ecma_char_t*) "setUTCHours";
    case ECMA_MAGIC_STRING_SET_DATE_UL: return (ecma_char_t*) "setDate";
    case ECMA_MAGIC_STRING_SET_UTC_DATE_UL: return (ecma_char_t*) "setUTCDate";
    case ECMA_MAGIC_STRING_SET_MONTH_UL: return (ecma_char_t*) "setMonth";
    case ECMA_MAGIC_STRING_SET_UTC_MONTH_UL: return (ecma_char_t*) "setUTCMonth";
    case ECMA_MAGIC_STRING_SET_FULL_YEAR_UL: return (ecma_char_t*) "setFullYear";
    case ECMA_MAGIC_STRING_SET_UTC_FULL_YEAR_UL: return (ecma_char_t*) "setUTCFullYear";
    case ECMA_MAGIC_STRING_TO_UTC_STRING_UL: return (ecma_char_t*) "toUTCString";
    case ECMA_MAGIC_STRING_TO_ISO_STRING_UL: return (ecma_char_t*) "toISOString";
    case ECMA_MAGIC_STRING_TO_JSON_UL: return (ecma_char_t*) "toJSON";
    case ECMA_MAGIC_STRING_MAX_VALUE_U: return (ecma_char_t*) "MAX_VALUE";
    case ECMA_MAGIC_STRING_MIN_VALUE_U: return (ecma_char_t*) "MIN_VALUE";
    case ECMA_MAGIC_STRING_POSITIVE_INFINITY_U: return (ecma_char_t*) "POSITIVE_INFINITY";
    case ECMA_MAGIC_STRING_NEGATIVE_INFINITY_U: return (ecma_char_t*) "NEGATIVE_INFINITY";
    case ECMA_MAGIC_STRING_APPLY: return (ecma_char_t*) "apply";
    case ECMA_MAGIC_STRING_CALL: return (ecma_char_t*) "call";
    case ECMA_MAGIC_STRING_BIND: return (ecma_char_t*) "bind";
    case ECMA_MAGIC_STRING_EXEC: return (ecma_char_t*) "exec";
    case ECMA_MAGIC_STRING_TEST: return (ecma_char_t*) "test";
    case ECMA_MAGIC_STRING_NAME: return (ecma_char_t*) "name";
    case ECMA_MAGIC_STRING_MESSAGE: return (ecma_char_t*) "message";
    case ECMA_MAGIC_STRING_LEFT_SQUARE_CHAR: return (ecma_char_t*) "[";
    case ECMA_MAGIC_STRING_RIGHT_SQUARE_CHAR: return (ecma_char_t*) "]";
    case ECMA_MAGIC_STRING_SPACE_CHAR: return (ecma_char_t*) " ";
    case ECMA_MAGIC_STRING__EMPTY: return (ecma_char_t*) "";
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
  return ecma_new_ecma_string (ecma_get_magic_string_zt (id));
} /* ecma_get_magic_string */

/**
 * Check if passed zt-string equals to one of magic strings
 * and if equal magic string was found, return it's id in 'out_id_p' argument.
 *
 * @return true - if magic string equal to passed string was found,
 *         false - otherwise.
 */
bool
ecma_is_zt_string_magic (ecma_char_t *zt_string_p, /**< zero-terminated string */
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
 * Check if passed string equals to one of magic strings
 * and if equal magic string was found, return it's id in 'out_id_p' argument.
 *
 * @return true - if magic string equal to passed string was found,
 *         false - otherwise.
 */
bool
ecma_is_string_magic (ecma_string_t *string_p, /**< ecma-string */
                      ecma_magic_string_id_t *out_id_p) /**< out: magic string's id */
{
  if (string_p->container == ECMA_STRING_CONTAINER_MAGIC_STRING)
  {
    JERRY_ASSERT (string_p->u.magic_string_id < ECMA_MAGIC_STRING__COUNT);

    *out_id_p = (ecma_magic_string_id_t) string_p->u.magic_string_id;

    return true;
  }
  else if (ecma_string_get_length (string_p) > ecma_magic_string_max_length)
  {
    return false;
  }
  else
  {
    ecma_char_t zt_string_buffer [ecma_magic_string_max_length + 1];

    ssize_t copied = ecma_string_to_zt_string (string_p, zt_string_buffer, (ssize_t) sizeof (zt_string_buffer));
    JERRY_ASSERT (copied > 0);

    return ecma_is_zt_string_magic (zt_string_buffer, out_id_p);
  }
} /* ecma_is_string_magic */

 /**
 * @}
 * @}
 */
