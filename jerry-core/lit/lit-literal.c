/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
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

#include "lit-literal.h"

#include "ecma-helpers.h"
#include "lit-cpointer.h"
#include "lit-magic-strings.h"
#include "lit-literal-storage.h"


/**
 * Initialize literal storage
 */
void
lit_init (void)
{
  lit_magic_strings_ex_init ();
} /* lit_init */

/**
 * Finalize literal storage
 */
void
lit_finalize (void)
{
#ifdef JERRY_ENABLE_LOG
  lit_dump_literals ();
#endif /* JERRY_ENABLE_LOG */

  while (lit_storage)
  {
    lit_storage = lit_free_literal (lit_storage);
  }
} /* lit_finalize */

/**
 * Create new literal in literal storage from characters buffer.
 * Don't check if the same literal already exists.
 *
 * @return pointer to created record
 */
lit_literal_t
lit_create_literal_from_utf8_string (const lit_utf8_byte_t *str_p, /**< string to initialize the record,
                                                                    * could be non-zero-terminated */
                                     lit_utf8_size_t str_size) /**< length of the string */
{
  JERRY_ASSERT (str_p || !str_size);

  lit_magic_string_id_t m_str_id;
  for (m_str_id = (lit_magic_string_id_t) 0;
       m_str_id < LIT_MAGIC_STRING__COUNT;
       m_str_id = (lit_magic_string_id_t) (m_str_id + 1))
  {
    if (lit_get_magic_string_size (m_str_id) != str_size)
    {
      continue;
    }

    if (!strncmp ((const char *) str_p, (const char *) lit_get_magic_string_utf8 (m_str_id), str_size))
    {
      return lit_create_magic_literal (m_str_id);
    }
  }

  lit_magic_string_ex_id_t m_str_ex_id;
  for (m_str_ex_id = (lit_magic_string_ex_id_t) 0;
       m_str_ex_id < lit_get_magic_string_ex_count ();
       m_str_ex_id = (lit_magic_string_ex_id_t) (m_str_ex_id + 1))
  {
    if (lit_get_magic_string_ex_size (m_str_ex_id) != str_size)
    {
      continue;
    }

    if (!strncmp ((const char *) str_p, (const char *) lit_get_magic_string_ex_utf8 (m_str_ex_id), str_size))
    {
      return lit_create_magic_literal_ex (m_str_ex_id);
    }
  }

  return lit_create_charset_literal (str_p, str_size);
} /* lit_create_literal_from_utf8_string */

/**
 * Find a literal in literal storage.
 * Only charset and magic string records are checked during search.
 *
 * @return pointer to a literal or NULL if no corresponding literal exists
 */
lit_literal_t
lit_find_literal_by_utf8_string (const lit_utf8_byte_t *str_p, /**< a string to search for */
                                 lit_utf8_size_t str_size)        /**< length of the string */
{
  JERRY_ASSERT (str_p || !str_size);

  lit_string_hash_t str_hash = lit_utf8_string_calc_hash (str_p, str_size);

  lit_literal_t lit;

  for (lit = lit_storage;
       lit != NULL;
       lit = lit_cpointer_decompress (lit->next))
  {
    const lit_record_type_t type = (lit_record_type_t) lit->type;

    switch (type)
    {
      case LIT_RECORD_TYPE_CHARSET:
      {
        const lit_charset_record_t *const rec_p = (const lit_charset_record_t *) lit;

        if (rec_p->hash != str_hash)
        {
          continue;
        }

        if (rec_p->size != str_size)
        {
          continue;
        }

        if (!strncmp ((const char *) (rec_p + 1), (const char *) str_p, str_size))
        {
          return lit;
        }

        break;
      }
      case LIT_RECORD_TYPE_MAGIC_STR:
      {
        lit_magic_string_id_t magic_id = (lit_magic_string_id_t) ((lit_magic_record_t *) lit)->magic_id;
        const lit_utf8_byte_t *magic_str_p = lit_get_magic_string_utf8 (magic_id);

        if (lit_get_magic_string_size (magic_id) != str_size)
        {
          continue;
        }

        if (!strncmp ((const char *) magic_str_p, (const char *) str_p, str_size))
        {
          return lit;
        }

        break;
      }
      case LIT_RECORD_TYPE_MAGIC_STR_EX:
      {
        lit_magic_string_ex_id_t magic_id = ((lit_magic_record_t *) lit)->magic_id;
        const lit_utf8_byte_t *magic_str_p = lit_get_magic_string_ex_utf8 (magic_id);

        if (lit_get_magic_string_ex_size (magic_id) != str_size)
        {
          continue;
        }

        if (!strncmp ((const char *) magic_str_p, (const char *) str_p, str_size))
        {
          return lit;
        }

        break;
      }
      default:
      {
        JERRY_ASSERT (type == LIT_RECORD_TYPE_NUMBER);
        break;
      }
    }
  }

  return NULL;
} /* lit_find_literal_by_utf8_string */

/**
 * Check if a literal which holds the passed string exists.
 * If it doesn't exist, create a new one.
 *
 * @return pointer to existing or newly created record
 */
lit_literal_t
lit_find_or_create_literal_from_utf8_string (const lit_utf8_byte_t *str_p, /**< string, could be non-zero-terminated */
                                             const lit_utf8_size_t str_size) /**< length of the string */
{
  lit_literal_t lit = lit_find_literal_by_utf8_string (str_p, str_size);

  if (lit == NULL)
  {
    lit = lit_create_literal_from_utf8_string (str_p, str_size);
  }

  return lit;
} /* lit_find_or_create_literal_from_utf8_string */


/**
 * Create new literal in literal storage from number.
 *
 * @return pointer to a newly created record
 */
inline lit_literal_t __attr_always_inline___
lit_create_literal_from_num (const ecma_number_t num) /**< number to initialize a new number literal */
{
  return lit_create_number_literal (num);
} /* lit_create_literal_from_num */

/**
 * Find existing or create new number literal in literal storage.
 *
 * @return pointer to existing or a newly created record
 */
lit_literal_t
lit_find_or_create_literal_from_num (ecma_number_t num) /**< number which a literal should contain */
{
  lit_literal_t lit = lit_find_literal_by_num (num);

  if (lit == NULL)
  {
    lit = lit_create_literal_from_num (num);
  }

  return lit;
} /* lit_find_or_create_literal_from_num */

/**
 * Find an existing number literal which contains the passed number
 *
 * @return pointer to existing or null
 */
lit_literal_t
lit_find_literal_by_num (const ecma_number_t num) /**< a number to search for */
{
  lit_literal_t lit;
  for (lit = lit_storage;
       lit != NULL;
       lit = lit_cpointer_decompress (lit->next))
  {
    const lit_record_type_t type = (lit_record_type_t) lit->type;

    if (type != LIT_RECORD_TYPE_NUMBER)
    {
      continue;
    }

    const ecma_number_t lit_num = lit_number_literal_get_number (lit);

    if (lit_num == num)
    {
      return lit;
    }
  }

  return NULL;
} /* lit_find_literal_by_num */

#ifndef JERRY_NDEBUG
/**
 * Check if literal really exists in the storage
 *
 * @return true if literal exists in the storage
 *         false otherwise
 */
static bool
lit_literal_exists (lit_literal_t lit) /**< literal to check for existence */
{
  lit_literal_t current_lit;

  for (current_lit = lit_storage;
       current_lit != NULL;
       current_lit = lit_cpointer_decompress (current_lit->next))
  {
    if (current_lit == lit)
    {
      return true;
    }
  }

  return false;
} /* lit_literal_exists */
#endif /* !JERRY_NDEBUG */

/**
 * Convert compressed pointer to literal
 *
 * @return literal
 */
lit_literal_t
lit_get_literal_by_cp (lit_cpointer_t lit_cp) /**< compressed pointer to literal */
{
  lit_literal_t lit = lit_cpointer_decompress (lit_cp);
#ifndef JERRY_NDEBUG
  JERRY_ASSERT (lit_literal_exists (lit));
#endif /* !JERRY_NDEBUG */

  return lit;
} /* lit_get_literal_by_cp */

lit_string_hash_t
lit_charset_literal_get_hash (lit_literal_t lit) /**< literal */
{
  JERRY_ASSERT (lit->type == LIT_RECORD_TYPE_CHARSET);

  const lit_charset_record_t *const rec_p = (const lit_charset_record_t *) lit;

  return (lit_string_hash_t) rec_p->hash;
} /* lit_charset_literal_get_hash */

lit_magic_string_id_t
lit_magic_literal_get_magic_str_id (lit_literal_t lit) /**< literal */
{
  JERRY_ASSERT (lit->type == LIT_RECORD_TYPE_MAGIC_STR);

  const lit_magic_record_t *const rec_p = (const lit_magic_record_t *) lit;

  return (lit_magic_string_id_t) rec_p->magic_id;
} /* lit_magic_literal_get_magic_str_id */

lit_magic_string_ex_id_t
lit_magic_literal_get_magic_str_ex_id (lit_literal_t lit) /**< literal */
{
  JERRY_ASSERT (lit->type == LIT_RECORD_TYPE_MAGIC_STR_EX);

  const lit_magic_record_t *const rec_p = (const lit_magic_record_t *) lit;

  return (lit_magic_string_ex_id_t) rec_p->magic_id;
} /* lit_magic_literal_get_magic_str_ex_id */

inline lit_utf8_size_t __attr_always_inline___ __attr_pure___
lit_charset_literal_get_size (lit_literal_t lit) /**< literal */
{
  JERRY_ASSERT (lit->type == LIT_RECORD_TYPE_CHARSET);

  const lit_charset_record_t *const rec_p = (const lit_charset_record_t *) lit;

  return (lit_utf8_size_t) rec_p->size;
} /* lit_charset_literal_get_size */

/**
 * Get length of the literal
 *
 * @return code units count
 */
inline ecma_length_t __attr_always_inline___ __attr_pure___
lit_charset_literal_get_length (lit_literal_t lit) /**< literal */
{
  JERRY_ASSERT (lit->type == LIT_RECORD_TYPE_CHARSET);

  const lit_charset_record_t *const rec_p = (const lit_charset_record_t *) lit;

  return (ecma_length_t) rec_p->length;
} /* lit_charset_literal_get_length */

inline ecma_number_t __attr_always_inline___ __attr_pure___
lit_number_literal_get_number (lit_literal_t lit) /**< literal */
{
  JERRY_ASSERT (lit->type == LIT_RECORD_TYPE_NUMBER);

  const lit_number_record_t *const rec_p = (const lit_number_record_t *) lit;

  return rec_p->number;
} /* lit_number_literal_get_number */

inline lit_utf8_byte_t * __attr_always_inline___ __attr_pure___
lit_charset_literal_get_charset (lit_literal_t lit) /**< literal */
{
  JERRY_ASSERT (lit->type == LIT_RECORD_TYPE_CHARSET);

  const lit_charset_record_t *const rec_p = (const lit_charset_record_t *) lit;

  return (lit_utf8_byte_t *) (rec_p + 1);
} /* lit_charset_literal_get_charset */
