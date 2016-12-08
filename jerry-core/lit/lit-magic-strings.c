/* Copyright JS Foundation and other contributors, http://js.foundation
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

#include "jcontext.h"
#include "lit-magic-strings.h"
#include "lit-strings.h"

/**
 * Get number of external magic strings
 *
 * @return number of the strings, if there were registered,
 *         zero - otherwise.
 */
inline uint32_t __attr_always_inline___
lit_get_magic_string_ex_count (void)
{
  return JERRY_CONTEXT (lit_magic_string_ex_count);
} /* lit_get_magic_string_ex_count */

/**
 * Get specified magic string as zero-terminated string
 *
 * @return pointer to zero-terminated magic string
 */
const lit_utf8_byte_t *
lit_get_magic_string_utf8 (lit_magic_string_id_t id) /**< magic string id */
{
  static const lit_utf8_byte_t * const lit_magic_strings[] JERRY_CONST_DATA =
  {
#define LIT_MAGIC_STRING_FIRST_STRING_WITH_SIZE(size, id)
#define LIT_MAGIC_STRING_DEF(id, utf8_string) \
    (const lit_utf8_byte_t *) utf8_string,
#include "lit-magic-strings.inc.h"
#undef LIT_MAGIC_STRING_DEF
#undef LIT_MAGIC_STRING_FIRST_STRING_WITH_SIZE
  };

  JERRY_ASSERT (id < LIT_MAGIC_STRING__COUNT);

  return lit_magic_strings[id];
} /* lit_get_magic_string_utf8 */

/**
 * Get size of specified magic string
 *
 * @return size in bytes
 */
lit_utf8_size_t
lit_get_magic_string_size (lit_magic_string_id_t id) /**< magic string id */
{
  static const lit_magic_size_t lit_magic_string_sizes[] JERRY_CONST_DATA =
  {
#define LIT_MAGIC_STRING_FIRST_STRING_WITH_SIZE(size, id)
#define LIT_MAGIC_STRING_DEF(id, utf8_string) \
    sizeof(utf8_string) - 1,
#include "lit-magic-strings.inc.h"
#undef LIT_MAGIC_STRING_DEF
#undef LIT_MAGIC_STRING_FIRST_STRING_WITH_SIZE
  };

  JERRY_ASSERT (id < LIT_MAGIC_STRING__COUNT);

  return lit_magic_string_sizes[id];
} /* lit_get_magic_string_size */

/**
 * Get the block start element with the given size from
 * the list of ECMA and implementation-defined magic string constants
 *
 * @return magic string id
 */
lit_magic_string_id_t
lit_get_magic_string_size_block_start (lit_utf8_size_t size) /**< magic string size */
{
  static const lit_magic_string_id_t const lit_magic_string_size_block_starts[] JERRY_CONST_DATA =
  {
#define LIT_MAGIC_STRING_DEF(id, utf8_string)
#define LIT_MAGIC_STRING_FIRST_STRING_WITH_SIZE(size, id) \
    id,
#include "lit-magic-strings.inc.h"
    LIT_MAGIC_STRING__COUNT
#undef LIT_MAGIC_STRING_DEF
#undef LIT_MAGIC_STRING_FIRST_STRING_WITH_SIZE
  };

  JERRY_ASSERT (size <= (sizeof (lit_magic_string_size_block_starts) / sizeof (lit_magic_string_id_t)));

  return lit_magic_string_size_block_starts[size];
} /* lit_get_magic_string_size_block_start */

/**
 * Get specified magic string as zero-terminated string from external table
 *
 * @return pointer to zero-terminated magic string
 */
const lit_utf8_byte_t *
lit_get_magic_string_ex_utf8 (lit_magic_string_ex_id_t id) /**< extern magic string id */
{
  if (JERRY_CONTEXT (lit_magic_string_ex_array) && id < JERRY_CONTEXT (lit_magic_string_ex_count))
  {
    return JERRY_CONTEXT (lit_magic_string_ex_array)[id];
  }

  JERRY_UNREACHABLE ();
} /* lit_get_magic_string_ex_utf8 */

/**
 * Get size of specified external magic string
 *
 * @return size in bytes
 */
lit_utf8_size_t
lit_get_magic_string_ex_size (lit_magic_string_ex_id_t id) /**< external magic string id */
{
  return JERRY_CONTEXT (lit_magic_string_ex_sizes)[id];
} /* lit_get_magic_string_ex_size */

/**
 * Register external magic strings
 */
void
lit_magic_strings_ex_set (const lit_utf8_byte_t **ex_str_items, /**< character arrays, representing
                                                                 *   external magic strings' contents */
                          uint32_t count,                       /**< number of the strings */
                          const lit_utf8_size_t *ex_str_sizes)  /**< sizes of the strings */
{
  JERRY_ASSERT (ex_str_items != NULL);
  JERRY_ASSERT (count > 0);
  JERRY_ASSERT (ex_str_sizes != NULL);

  JERRY_ASSERT (JERRY_CONTEXT (lit_magic_string_ex_array) == NULL);
  JERRY_ASSERT (JERRY_CONTEXT (lit_magic_string_ex_count) == 0);
  JERRY_ASSERT (JERRY_CONTEXT (lit_magic_string_ex_sizes) == NULL);

  /* Set external magic strings information */
  JERRY_CONTEXT (lit_magic_string_ex_array) = ex_str_items;
  JERRY_CONTEXT (lit_magic_string_ex_count) = count;
  JERRY_CONTEXT (lit_magic_string_ex_sizes) = ex_str_sizes;

#ifndef JERRY_NDEBUG
  for (lit_magic_string_ex_id_t id = (lit_magic_string_ex_id_t) 0;
       id < JERRY_CONTEXT (lit_magic_string_ex_count);
       id = (lit_magic_string_ex_id_t) (id + 1))
  {
    lit_utf8_size_t string_size = lit_zt_utf8_string_size (lit_get_magic_string_ex_utf8 (id));
    JERRY_ASSERT (JERRY_CONTEXT (lit_magic_string_ex_sizes)[id] == string_size);
    JERRY_ASSERT (JERRY_CONTEXT (lit_magic_string_ex_sizes)[id] <= LIT_MAGIC_STRING_LENGTH_LIMIT);
  }
#endif /* !JERRY_NDEBUG */
} /* lit_magic_strings_ex_set */

/**
 * Returns the magic string id of the argument string if it is available.
 *
 * @return id - if magic string id is found,
 *         LIT_MAGIC_STRING__COUNT - otherwise.
 */
lit_magic_string_id_t
lit_is_utf8_string_magic (const lit_utf8_byte_t *string_p, /**< utf-8 string */
                          lit_utf8_size_t string_size) /**< string size in bytes */
{
  if (string_size > lit_get_magic_string_size (LIT_MAGIC_STRING__COUNT - 1))
  {
    return LIT_MAGIC_STRING__COUNT;
  }

  /**< The string must be in this id range. */
  int first = (int) lit_get_magic_string_size_block_start (string_size);
  int last = (int) (lit_get_magic_string_size_block_start (string_size + 1) - 1);

  while (first <= last)
  {
    int middle = ((first + last) / 2); /**< mid point of search */
    int compare = memcmp (lit_get_magic_string_utf8 (middle), string_p, string_size);

    if (compare == 0)
    {
      return (lit_magic_string_id_t) middle;
    }
    else if (compare > 0)
    {
      last = middle - 1;
    }
    else
    {
      first = middle + 1;
    }
  }

  return LIT_MAGIC_STRING__COUNT;
} /* lit_is_utf8_string_magic */

/**
 * Returns the magic string id of the argument string pair if it is available.
 *
 * @return id - if magic string id is found,
 *         LIT_MAGIC_STRING__COUNT - otherwise.
 */
lit_magic_string_id_t
lit_is_utf8_string_pair_magic (const lit_utf8_byte_t *string1_p, /**< first utf-8 string */
                               lit_utf8_size_t string1_size, /**< first string size in bytes */
                               const lit_utf8_byte_t *string2_p, /**< second utf-8 string */
                               lit_utf8_size_t string2_size) /**< second string size in bytes */
{
  lit_utf8_size_t total_string_size = string1_size + string2_size;

  if (total_string_size > lit_get_magic_string_size (LIT_MAGIC_STRING__COUNT - 1))
  {
    return LIT_MAGIC_STRING__COUNT;
  }

  /**< The string must be in this id range. */
  int first = (int) lit_get_magic_string_size_block_start (total_string_size);
  int last = (int) (lit_get_magic_string_size_block_start (total_string_size + 1) - 1);

  while (first <= last)
  {
    int middle = ((first + last) / 2); /**< mid point of search */
    const lit_utf8_byte_t *middle_string_p = lit_get_magic_string_utf8 (middle);

    int compare = memcmp (middle_string_p, string1_p, string1_size);

    if (compare == 0)
    {
      compare = memcmp (middle_string_p + string1_size, string2_p, string2_size);
    }

    if (compare == 0)
    {
      return (lit_magic_string_id_t) middle;
    }
    else if (compare > 0)
    {
      last = middle - 1;
    }
    else
    {
      first = middle + 1;
    }
  }

  return LIT_MAGIC_STRING__COUNT;
} /* lit_is_utf8_string_pair_magic */

/**
 * Returns the ex magic string id of the argument string if it is available.
 *
 * @return id - if magic string id is found,
 *         lit_get_magic_string_ex_count () - otherwise.
 */
lit_magic_string_ex_id_t
lit_is_ex_utf8_string_magic (const lit_utf8_byte_t *string_p, /**< utf-8 string */
                             lit_utf8_size_t string_size) /**< string size in bytes */
{
  /* TODO: Improve performance of search */

  for (lit_magic_string_ex_id_t id = (lit_magic_string_ex_id_t) 0;
       id < JERRY_CONTEXT (lit_magic_string_ex_count);
       id = (lit_magic_string_ex_id_t) (id + 1))
  {
    if (string_size == lit_get_magic_string_ex_size (id))
    {
      if (memcmp (string_p, lit_get_magic_string_ex_utf8 (id), string_size) == 0)
      {
        return id;
      }
    }
  }

  return JERRY_CONTEXT (lit_magic_string_ex_count);
} /* lit_is_ex_utf8_string_magic */

/**
 * Returns the ex magic string id of the argument string pair if it is available.
 *
 * @return id - if magic string id is found,
 *         lit_get_magic_string_ex_count () - otherwise.
 */
lit_magic_string_ex_id_t
lit_is_ex_utf8_string_pair_magic (const lit_utf8_byte_t *string1_p, /**< first utf-8 string */
                                  lit_utf8_size_t string1_size, /**< first string size in bytes */
                                  const lit_utf8_byte_t *string2_p, /**< second utf-8 string */
                                  lit_utf8_size_t string2_size) /**< second string size in bytes */
{
  /* TODO: Improve performance of search */
  lit_utf8_size_t total_string_size = string1_size + string2_size;

  for (lit_magic_string_ex_id_t id = (lit_magic_string_ex_id_t) 0;
       id < JERRY_CONTEXT (lit_magic_string_ex_count);
       id = (lit_magic_string_ex_id_t) (id + 1))
  {
    if (total_string_size == lit_get_magic_string_ex_size (id))
    {
      const lit_utf8_byte_t *ex_magic_string_p = lit_get_magic_string_ex_utf8 (id);

      if (memcmp (string1_p, ex_magic_string_p, string1_size) == 0
          && memcmp (string2_p, ex_magic_string_p + string1_size, string2_size) == 0)
      {
        return id;
      }
    }
  }

  return JERRY_CONTEXT (lit_magic_string_ex_count);
} /* lit_is_ex_utf8_string_pair_magic */

/**
 * Copy magic string to buffer
 *
 * Warning:
 *         the routine requires that buffer size is enough
 *
 * @return pointer to the byte next to the last copied in the buffer
 */
extern lit_utf8_byte_t *
lit_copy_magic_string_to_buffer (lit_magic_string_id_t id, /**< magic string id */
                                 lit_utf8_byte_t *buffer_p, /**< destination buffer */
                                 lit_utf8_size_t buffer_size) /**< size of buffer */
{
  const lit_utf8_byte_t *magic_string_bytes_p = lit_get_magic_string_utf8 (id);
  lit_utf8_size_t magic_string_bytes_count = lit_get_magic_string_size (id);

  const lit_utf8_byte_t *str_iter_p = magic_string_bytes_p;
  lit_utf8_byte_t *buf_iter_p = buffer_p;
  lit_utf8_size_t bytes_copied = 0;

  while (magic_string_bytes_count--)
  {
    bytes_copied ++;
    JERRY_ASSERT (bytes_copied <= buffer_size);

    *buf_iter_p++ = *str_iter_p++;
  }

  return buf_iter_p;
} /* lit_copy_magic_string_to_buffer */
