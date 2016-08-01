/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
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
uint32_t
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
  static const lit_utf8_byte_t * const magic_strings[] JERRY_CONST_DATA =
  {
#define LIT_MAGIC_STRING_DEF(id, utf8_string) \
    (const lit_utf8_byte_t *) utf8_string,
#include "lit-magic-strings.inc.h"
#undef LIT_MAGIC_STRING_DEF
  };

  JERRY_ASSERT (id < LIT_MAGIC_STRING__COUNT);

  return magic_strings[id];
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
#define LIT_MAGIC_STRING_DEF(id, utf8_string) \
    sizeof(utf8_string) - 1,
#include "lit-magic-strings.inc.h"
#undef LIT_MAGIC_STRING_DEF
  };

  JERRY_ASSERT (id < LIT_MAGIC_STRING__COUNT);

  return lit_magic_string_sizes[id];
} /* lit_get_magic_string_size */

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
 * Check if passed cesu-8 string equals to one of magic strings
 * and if equal magic string was found, return it's id in 'out_id_p' argument.
 *
 * @return true - if magic string equal to passed string was found,
 *         false - otherwise.
 */
bool
lit_is_utf8_string_magic (const lit_utf8_byte_t *string_p, /**< utf-8 string */
                          lit_utf8_size_t string_size, /**< string size in bytes */
                          lit_magic_string_id_t *out_id_p) /**< [out] magic string's id */
{
  /* TODO: Improve performance of search */

  for (lit_magic_string_id_t id = (lit_magic_string_id_t) 0;
       id < LIT_MAGIC_STRING__COUNT;
       id = (lit_magic_string_id_t) (id + 1))
  {
    if (lit_compare_utf8_string_and_magic_string (string_p, string_size, id))
    {
      *out_id_p = id;

      return true;
    }
  }

  *out_id_p = LIT_MAGIC_STRING__COUNT;

  return false;
} /* lit_is_utf8_string_magic */

/**
 * Check if passed utf-8 string equals to one of external magic strings
 * and if equal magic string was found, return it's id in 'out_id_p' argument.
 *
 * @return true - if external magic string equal to passed string was found,
 *         false - otherwise.
 */
bool lit_is_ex_utf8_string_magic (const lit_utf8_byte_t *string_p, /**< utf-8 string */
                                  lit_utf8_size_t string_size, /**< string size in bytes */
                                  lit_magic_string_ex_id_t *out_id_p) /**< [out] magic string's id */
{
  /* TODO: Improve performance of search */

  for (lit_magic_string_ex_id_t id = (lit_magic_string_ex_id_t) 0;
       id < JERRY_CONTEXT (lit_magic_string_ex_count);
       id = (lit_magic_string_ex_id_t) (id + 1))
  {
    if (lit_compare_utf8_string_and_magic_string_ex (string_p, string_size, id))
    {
      *out_id_p = id;

      return true;
    }
  }

  *out_id_p = JERRY_CONTEXT (lit_magic_string_ex_count);

  return false;
} /* lit_is_ex_utf8_string_magic */

/**
 * Compare utf-8 string and magic string for equality
 *
 * @return true if strings are equal
 *         false otherwise
 */
bool
lit_compare_utf8_string_and_magic_string (const lit_utf8_byte_t *string_p, /**< utf-8 string */
                                          lit_utf8_size_t string_size, /**< string size in bytes */
                                          lit_magic_string_id_t magic_string_id) /**< magic string's id */
{
  return lit_compare_utf8_strings (string_p,
                                   string_size,
                                   lit_get_magic_string_utf8 (magic_string_id),
                                   lit_get_magic_string_size (magic_string_id));
} /* lit_compare_utf8_string_and_magic_string */

/**
 * Compare utf-8 string and external magic string for equality
 *
 * @return true if strings are equal
 *         false otherwise
 */
bool
lit_compare_utf8_string_and_magic_string_ex (const lit_utf8_byte_t *string_p, /**< utf-8 string */
                                             lit_utf8_size_t string_size, /**< string size in bytes */
                                             lit_magic_string_ex_id_t magic_string_ex_id) /**<  external magic string's
                                                                                           * id */
{
  return lit_compare_utf8_strings (string_p,
                                   string_size,
                                   lit_get_magic_string_ex_utf8 (magic_string_ex_id),
                                   lit_get_magic_string_ex_size (magic_string_ex_id));
} /* lit_compare_utf8_string_and_magic_string_ex */

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
