/* Copyright 2015 Samsung Electronics Co., Ltd.
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

#include "lit-magic-strings.h"

#include "ecma-helpers.h"

/**
 * Lengths of magic strings
 */
static ecma_length_t lit_magic_string_lengths[LIT_MAGIC_STRING__COUNT];

/**
 * External magic strings data array, count and lengths
 */
static const ecma_char_ptr_t *lit_magic_string_ex_array = NULL;
static uint32_t lit_magic_string_ex_count = 0;
static const ecma_length_t *lit_magic_string_ex_lengths = NULL;

#ifndef JERRY_NDEBUG
/**
 * Maximum length among lengths of magic strings
 */
static ecma_length_t lit_magic_string_max_length;
#endif /* !JERRY_NDEBUG */

/**
 * Initialize data for string helpers
 */
void
lit_magic_strings_init (void)
{
  /* Initializing magic strings information */

#ifndef JERRY_NDEBUG
  lit_magic_string_max_length = 0;
#endif /* !JERRY_NDEBUG */

  for (lit_magic_string_id_t id = (lit_magic_string_id_t) 0;
       id < LIT_MAGIC_STRING__COUNT;
       id = (lit_magic_string_id_t) (id + 1))
  {
    lit_magic_string_lengths[id] = ecma_zt_string_length (lit_get_magic_string_zt (id));

#ifndef JERRY_NDEBUG
    lit_magic_string_max_length = JERRY_MAX (lit_magic_string_max_length, lit_magic_string_lengths[id]);

    JERRY_ASSERT (lit_magic_string_max_length <= LIT_MAGIC_STRING_LENGTH_LIMIT);
#endif /* !JERRY_NDEBUG */
  }
} /* ecma_strings_init */

/**
 * Initialize external magic strings
 */
void
lit_magic_strings_ex_init (void)
{
  lit_magic_string_ex_array = NULL;
  lit_magic_string_ex_count = 0;
  lit_magic_string_ex_lengths = NULL;
} /* ecma_strings_ex_init */

/**
 * Register external magic strings
 */
void
lit_magic_strings_ex_set (const ecma_char_ptr_t* ex_str_items, /**< character arrays, representing
                                                                *   external magic strings' contents */
                          uint32_t count,                      /**< number of the strings */
                          const ecma_length_t* ex_str_lengths) /**< lengths of the strings */
{
  JERRY_ASSERT (ex_str_items != NULL);
  JERRY_ASSERT (count > 0);
  JERRY_ASSERT (ex_str_lengths != NULL);

  JERRY_ASSERT (lit_magic_string_ex_array == NULL);
  JERRY_ASSERT (lit_magic_string_ex_count == 0);
  JERRY_ASSERT (lit_magic_string_ex_lengths == NULL);

  /* Set external magic strings information */
  lit_magic_string_ex_array = ex_str_items;
  lit_magic_string_ex_count = count;
  lit_magic_string_ex_lengths = ex_str_lengths;

#ifndef JERRY_NDEBUG
  for (lit_magic_string_ex_id_t id = (lit_magic_string_ex_id_t) 0;
       id < lit_magic_string_ex_count;
       id = (lit_magic_string_ex_id_t) (id + 1))
  {
    JERRY_ASSERT (lit_magic_string_ex_lengths[id] == ecma_zt_string_length (lit_get_magic_string_ex_zt (id)));

    lit_magic_string_max_length = JERRY_MAX (lit_magic_string_max_length, lit_magic_string_ex_lengths[id]);

    JERRY_ASSERT (lit_magic_string_max_length <= LIT_MAGIC_STRING_LENGTH_LIMIT);
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
  return lit_magic_string_ex_count;
} /* ecma_get_magic_string_ex_count */

/**
 * Get specified magic string as zero-terminated string
 *
 * @return pointer to zero-terminated magic string
 */
const ecma_char_t *
lit_get_magic_string_zt (lit_magic_string_id_t id) /**< magic string id */
{
  TODO (Support UTF-16);

  switch (id)
  {
#define LIT_MAGIC_STRING_DEF(id, ascii_zt_string) \
     case id: return (ecma_char_t*) ascii_zt_string;
#include "lit-magic-strings.inc.h"
#undef LIT_MAGIC_STRING_DEF

    case LIT_MAGIC_STRING__COUNT: break;
  }

  JERRY_UNREACHABLE ();
} /* lit_get_magic_string_zt */

/**
 * Get length of specified magic string
 *
 * @return length
 */
ecma_length_t
lit_get_magic_string_length (lit_magic_string_id_t id) /**< magic string id */
{
  return lit_magic_string_lengths[id];
} /* ecma_get_magic_string_size */

/**
 * Get specified magic string as zero-terminated string from external table
 *
 * @return pointer to zero-terminated magic string
 */
const ecma_char_t*
lit_get_magic_string_ex_zt (lit_magic_string_ex_id_t id) /**< extern magic string id */
{
  TODO (Support UTF-16);

  if (lit_magic_string_ex_array && id < lit_magic_string_ex_count)
  {
    return lit_magic_string_ex_array[id];
  }

  JERRY_UNREACHABLE ();
} /* lit_get_magic_string_ex_zt */

/**
 * Get length of specified external magic string
 *
 * @return length
 */
ecma_length_t
lit_get_magic_string_ex_length (lit_magic_string_ex_id_t id) /**< external magic string id */
{
  return lit_magic_string_ex_lengths[id];
} /* lit_get_magic_string_ex_length */

/**
 * Check if passed zt-string equals to one of magic strings
 * and if equal magic string was found, return it's id in 'out_id_p' argument.
 *
 * @return true - if magic string equal to passed string was found,
 *         false - otherwise.
 */
bool
lit_is_zt_string_magic (const ecma_char_t *zt_string_p, /**< zero-terminated string */
                         lit_magic_string_id_t *out_id_p) /**< out: magic string's id */
{
  TODO (Improve performance of search);

  for (lit_magic_string_id_t id = (lit_magic_string_id_t) 0;
       id < LIT_MAGIC_STRING__COUNT;
       id = (lit_magic_string_id_t) (id + 1))
  {
    if (ecma_compare_zt_strings (zt_string_p, lit_get_magic_string_zt (id)))
    {
      *out_id_p = id;

      return true;
    }
  }

  *out_id_p = LIT_MAGIC_STRING__COUNT;

  return false;
} /* lit_is_zt_string_magic */

/**
 * Check if passed zt-string equals to one of external magic strings
 * and if equal magic string was found, return it's id in 'out_id_p' argument.
 *
 * @return true - if external magic string equal to passed string was found,
 *         false - otherwise.
 */
bool
lit_is_zt_ex_string_magic (const ecma_char_t *zt_string_p, /**< zero-terminated string */
                            lit_magic_string_ex_id_t *out_id_p) /**< out: external magic string's id */
{
  TODO (Improve performance of search);

  for (lit_magic_string_ex_id_t id = (lit_magic_string_ex_id_t) 0;
       id < lit_magic_string_ex_count;
       id = (lit_magic_string_ex_id_t) (id + 1))
  {
    if (ecma_compare_zt_strings (zt_string_p, lit_get_magic_string_ex_zt (id)))
    {
      *out_id_p = id;

      return true;
    }
  }

  *out_id_p = lit_magic_string_ex_count;

  return false;
} /* lit_is_zt_ex_string_magic */
