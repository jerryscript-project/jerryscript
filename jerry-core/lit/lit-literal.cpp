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

#include "lit-literal.h"

#include "ecma-helpers.h"
#include "rcs-allocator.h"
#include "rcs-records.h"
#include "rcs-iterator.h"
#include "lit-literal-storage.h"

/**
 * Initialize literal storage
 */
void
lit_init ()
{
  JERRY_ASSERT (rcs_get_node_data_space_size () % RCS_DYN_STORAGE_LENGTH_UNIT == 0);

  rcs_lit_storage.init ();

  lit_magic_strings_init ();
  lit_magic_strings_ex_init ();
} /* lit_init */

/**
 * Finalize literal storage
 */
void
lit_finalize ()
{
  rcs_lit_storage.cleanup ();
  rcs_lit_storage.free ();
} /* lit_finalize */

/**
 * Dump records from the literal storage
 */
void
lit_dump_literals ()
{
  lit_dump_literals (&rcs_lit_storage);
} /* lit_dump_literals */

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
      return lit_create_magic_literal (&rcs_lit_storage, m_str_id);
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
      return lit_create_magic_literal_ex (&rcs_lit_storage, m_str_ex_id);
    }
  }

  return lit_create_charset_literal (&rcs_lit_storage, str_p, str_size);
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

  for (lit = rcs_record_get_first (&rcs_lit_storage);
       lit != NULL;
       lit = rcs_record_get_next (&rcs_lit_storage, lit))
  {
    rcs_record_type_t type = rcs_record_get_type (lit);

    if (RCS_RECORD_TYPE_IS_CHARSET (type))
    {
      if (rcs_record_get_hash (lit) != str_hash)
      {
        continue;
      }

      if (rcs_record_get_length (lit) != str_size)
      {
        continue;
      }

      if (rcs_record_is_equal_charset (&rcs_lit_storage, lit, str_p, str_size))
      {
        return lit;
      }
    }
    else if (RCS_RECORD_TYPE_IS_MAGIC_STR (type))
    {
      lit_magic_string_id_t magic_id = rcs_record_get_magic_str_id (lit);
      const lit_utf8_byte_t *magic_str_p = lit_get_magic_string_utf8 (magic_id);

      if (lit_get_magic_string_size (magic_id) != str_size)
      {
        continue;
      }

      if (!strncmp ((const char *) magic_str_p, (const char *) str_p, str_size))
      {
        return lit;
      }
    }
    else if (RCS_RECORD_TYPE_IS_MAGIC_STR_EX (type))
    {
      lit_magic_string_ex_id_t magic_id = rcs_record_get_magic_str_ex_id (lit);
      const lit_utf8_byte_t *magic_str_p = lit_get_magic_string_ex_utf8 (magic_id);

      if (lit_get_magic_string_ex_size (magic_id) != str_size)
      {
        continue;
      }

      if (!strncmp ((const char *) magic_str_p, (const char *) str_p, str_size))
      {
        return lit;
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
                                             lit_utf8_size_t str_size) /**< length of the string */
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
lit_literal_t
lit_create_literal_from_num (ecma_number_t num) /**< number to initialize a new number literal */
{
  return lit_create_number_literal (&rcs_lit_storage, num);
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
lit_find_literal_by_num (ecma_number_t num) /**< a number to search for */
{
  lit_literal_t lit;
  for (lit = rcs_record_get_first (&rcs_lit_storage);
       lit != NULL;
       lit = rcs_record_get_next (&rcs_lit_storage, lit))
  {
    rcs_record_type_t type = rcs_record_get_type (lit);

    if (!RCS_RECORD_TYPE_IS_NUMBER (type))
    {
      continue;
    }

    ecma_number_t lit_num = rcs_record_get_number (&rcs_lit_storage, lit);

    if (lit_num == num)
    {
      return lit;
    }
  }

  return NULL;
} /* lit_find_literal_by_num */

/**
 * Check if literal equals to charset record
 *
 * @return true if is_equal
 *         false otherwise
 */
static bool
lit_literal_equal_charset_rec (lit_literal_t lit, /**< literal to compare */
                               lit_literal_t record) /**< charset record to compare */
{
  switch (rcs_record_get_type (lit))
  {
    case RCS_RECORD_TYPE_CHARSET:
    {
      return rcs_record_is_equal (&rcs_lit_storage, lit, record);
    }
    case RCS_RECORD_TYPE_MAGIC_STR:
    {
      lit_magic_string_id_t magic_string_id = rcs_record_get_magic_str_id (lit);
      return rcs_record_is_equal_charset (&rcs_lit_storage,
                                          record,
                                          lit_get_magic_string_utf8 (magic_string_id),
                                          lit_get_magic_string_size (magic_string_id));
    }
    case RCS_RECORD_TYPE_MAGIC_STR_EX:
    {
      lit_magic_string_ex_id_t magic_string_id = rcs_record_get_magic_str_ex_id (lit);

      return rcs_record_is_equal_charset (&rcs_lit_storage,
                                          record,
                                          lit_get_magic_string_ex_utf8 (magic_string_id),
                                          lit_get_magic_string_ex_size (magic_string_id));
    }
    case RCS_RECORD_TYPE_NUMBER:
    {
      ecma_number_t num = rcs_record_get_number (&rcs_lit_storage, lit);

      lit_utf8_byte_t buff[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER];
      lit_utf8_size_t copied = ecma_number_to_utf8_string (num, buff, sizeof (buff));

      return rcs_record_is_equal_charset (&rcs_lit_storage, record, buff, copied);
    }
    default:
    {
      JERRY_UNREACHABLE ();
      return false;
    }
  }
} /* lit_literal_equal_charset_rec */

/**
 * Check if literal equals to utf-8 string
 *
 * @return true if equal
 *         false otherwise
 */
bool
lit_literal_equal_utf8 (lit_literal_t lit, /**< literal to compare */
                        const lit_utf8_byte_t *str_p, /**< utf-8 string to compare */
                        lit_utf8_size_t str_size) /**< string size in bytes */
{
  switch (rcs_record_get_type (lit))
  {
    case RCS_RECORD_TYPE_CHARSET:
    {
      return rcs_record_is_equal_charset (&rcs_lit_storage, lit, str_p, str_size);
    }
    case RCS_RECORD_TYPE_MAGIC_STR:
    {
      lit_magic_string_id_t magic_id = rcs_record_get_magic_str_id (lit);
      return lit_compare_utf8_string_and_magic_string (str_p, str_size, magic_id);
    }
    case RCS_RECORD_TYPE_MAGIC_STR_EX:
    {
      lit_magic_string_ex_id_t magic_id = rcs_record_get_magic_str_ex_id (lit);
      return lit_compare_utf8_string_and_magic_string_ex (str_p, str_size, magic_id);
    }
    case RCS_RECORD_TYPE_NUMBER:
    {
      ecma_number_t num = rcs_record_get_number (&rcs_lit_storage, lit);

      lit_utf8_byte_t num_buf[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER];
      lit_utf8_size_t num_size = ecma_number_to_utf8_string (num, num_buf, sizeof (num_buf));

      return lit_compare_utf8_strings (str_p, str_size, num_buf, num_size);
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* lit_literal_equal_utf8 */

/**
 * Check if literal contains the string equal to the passed number
 *
 * @return true if equal
 *         false otherwise
 */
bool
lit_literal_equal_num (lit_literal_t lit,     /**< literal to check */
                       ecma_number_t num) /**< number to compare with */
{
  lit_utf8_byte_t buff[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER];
  lit_utf8_size_t copied = ecma_number_to_utf8_string (num, buff, sizeof (buff));

  return lit_literal_equal_utf8 (lit, buff, copied);
} /* lit_literal_equal_num */

/**
 * Check if two literals are equal
 *
 * @return true if equal
 *         false otherwise
 */
bool
lit_literal_equal (lit_literal_t lit1, /**< first literal */
                   lit_literal_t lit2) /**< second literal */
{
  switch (rcs_record_get_type (lit2))
  {
    case RCS_RECORD_TYPE_CHARSET:
    {
      return lit_literal_equal_charset_rec (lit1, lit2);
    }
    case RCS_RECORD_TYPE_MAGIC_STR:
    {
      lit_magic_string_id_t magic_str_id = rcs_record_get_magic_str_id (lit2);

      return lit_literal_equal_utf8 (lit1,
                                     lit_get_magic_string_utf8 (magic_str_id),
                                     lit_get_magic_string_size (magic_str_id));
    }
    case RCS_RECORD_TYPE_MAGIC_STR_EX:
    {
      lit_magic_string_ex_id_t magic_str_ex_id = rcs_record_get_magic_str_ex_id (lit2);

      return lit_literal_equal_utf8 (lit1,
                                     lit_get_magic_string_ex_utf8 (magic_str_ex_id),
                                     lit_get_magic_string_ex_size (magic_str_ex_id));
    }
    case RCS_RECORD_TYPE_NUMBER:
    {
      ecma_number_t num = rcs_record_get_number (&rcs_lit_storage, lit2);
      return lit_literal_equal_num (lit1, num);
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* lit_literal_equal */

/**
 * Check if literal equals to utf-8 string.
 * Check that literal is a string literal before performing detailed comparison.
 *
 * @return true if equal
 *         false otherwise
 */
bool
lit_literal_equal_type_utf8 (lit_literal_t lit, /**< literal to compare */
                             const lit_utf8_byte_t *str_p, /**< utf-8 string */
                             lit_utf8_size_t str_size) /**< string size */
{
  rcs_record_type_t type = rcs_record_get_type (lit);

  if (RCS_RECORD_TYPE_IS_NUMBER (type) || RCS_RECORD_TYPE_IS_FREE (type))
  {
    return false;
  }

  return lit_literal_equal_utf8 (lit, str_p, str_size);
} /* lit_literal_equal_type_utf8 */

/**
 * Check if literal equals to C string.
 * Check that literal is a string literal before performing detailed comparison.
 *
 * @return true if equal
 *         false otherwise
 */
bool
lit_literal_equal_type_cstr (lit_literal_t lit, /**< literal to compare */
                             const char *c_str_p) /**< zero-terminated C-string */
{
  return lit_literal_equal_type_utf8 (lit, (const lit_utf8_byte_t *) c_str_p, (lit_utf8_size_t) strlen (c_str_p));
} /* lit_literal_equal_type_cstr */

/**
 * Check if literal contains the string equal to the passed number.
 * Check that literal is a number literal before performing detailed comparison.
 *
 * @return true if equal
 *         false otherwise
 */
bool
lit_literal_equal_type_num (lit_literal_t lit,     /**< literal to check */
                            ecma_number_t num) /**< number to compare with */
{
  if (!RCS_RECORD_IS_NUMBER (lit))
  {
    return false;
  }

  return lit_literal_equal_num (lit, num);
} /* lit_literal_equal_type_num */

/**
 * Check if two literals are equal
 * Compare types of literals before performing detailed comparison.
 *
 * @return true if equal
 *         false otherwise
 */
bool
lit_literal_equal_type (lit_literal_t lit1, /**< first literal */
                        lit_literal_t lit2) /**< second literal */
{
  if (rcs_record_get_type (lit1) != rcs_record_get_type (lit2))
  {
    return false;
  }

  return lit_literal_equal (lit1, lit2);
} /* lit_literal_equal_type */

/**
 * Get the contents of the literal as a zero-terminated string.
 * If literal is a magic string record, the corresponding string is not copied to the buffer,
 * but is returned directly.
 *
 * @return pointer to the zero-terminated string.
 */
const lit_utf8_byte_t *
lit_literal_to_utf8_string (lit_literal_t lit, /**< literal to be processed */
                            lit_utf8_byte_t *buff_p, /**< buffer to use as a string storage */
                            size_t size) /**< size of the buffer */
{
  JERRY_ASSERT (buff_p != NULL && size > 0);
  rcs_record_type_t type = rcs_record_get_type (lit);

  if (RCS_RECORD_TYPE_IS_CHARSET (type))
  {
    rcs_record_get_charset (&rcs_lit_storage, lit, buff_p, size);
    return buff_p;
  }
  else if (RCS_RECORD_TYPE_IS_MAGIC_STR (type))
  {
    return lit_get_magic_string_utf8 (rcs_record_get_magic_str_id (lit));
  }
  else if (RCS_RECORD_TYPE_IS_MAGIC_STR_EX (type))
  {
    return lit_get_magic_string_ex_utf8 (rcs_record_get_magic_str_ex_id (lit));
  }
  else
  {
    JERRY_ASSERT (RCS_RECORD_TYPE_IS_NUMBER (type));

    ecma_number_t number = rcs_record_get_number (&rcs_lit_storage, lit);
    ecma_number_to_utf8_string (number, buff_p, (ssize_t) size);
    return buff_p;
  }
} /* lit_literal_to_utf8_string */

/**
 * Get the contents of the literal as a C string.
 * If literal holds a very long string, it would be trimmed to ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER characters.
 *
 * @return pointer to the C string.
 */
const char *
lit_literal_to_str_internal_buf (lit_literal_t lit) /**< literal */
{
  static lit_utf8_byte_t buff[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER + 1];
  memset (buff, 0, sizeof (buff));

  return (const char *) lit_literal_to_utf8_string (lit, buff, sizeof (buff) - 1);
} /* lit_literal_to_str_internal_buf */


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

  for (current_lit = rcs_record_get_first (&rcs_lit_storage);
       current_lit != NULL;
       current_lit = rcs_record_get_next (&rcs_lit_storage, current_lit))
  {
    if (current_lit == lit)
    {
      return true;
    }
  }

  return false;
} /* lit_literal_exists */

/**
 * Convert compressed pointer to literal
 *
 * @return literal
 */
lit_literal_t
lit_get_literal_by_cp (rcs_cpointer_t lit_cp) /**< compressed pointer to literal */
{
  JERRY_ASSERT (lit_cp.packed_value != MEM_CP_NULL);
  lit_literal_t lit = rcs_cpointer_decompress (lit_cp);
  JERRY_ASSERT (lit_literal_exists (lit));

  return lit;
} /* lit_get_literal_by_cp */

lit_string_hash_t
lit_charset_literal_get_hash (lit_literal_t lit) /**< literal */
{
  return rcs_record_get_hash (lit);
} /* lit_charset_literal_get_hash */

lit_magic_string_id_t
lit_magic_literal_get_magic_str_id (lit_literal_t lit) /**< literal */
{
  return rcs_record_get_magic_str_id (lit);
} /* lit_magic_literal_get_magic_str_id */

lit_magic_string_ex_id_t
lit_magic_literal_ex_get_magic_str_id (lit_literal_t lit) /**< literal */
{
  return rcs_record_get_magic_str_ex_id (lit);
} /* lit_magic_literal_ex_get_magic_str_id */

lit_utf8_size_t
lit_charset_literal_get_size (lit_literal_t lit) /**< literal */
{
  return rcs_record_get_length (lit);
} /* lit_charset_literal_get_size */

/**
 * Get length of the literal
 *
 * @return code units count
 */
ecma_length_t
lit_charset_literal_get_length (lit_literal_t lit) /**< literal */
{
  TODO ("Add special case for literals which doesn't contain long characters");

  rcs_iterator_t it_ctx = rcs_iterator_create (&rcs_lit_storage, lit);
  rcs_iterator_skip (&it_ctx, RCS_CHARSET_HEADER_SIZE);

  lit_utf8_size_t lit_utf8_str_size = rcs_record_get_length (lit);
  ecma_length_t length = 0;
  lit_utf8_size_t i = 0;

  while (i < lit_utf8_str_size)
  {
    lit_utf8_byte_t byte;
    lit_utf8_size_t bytes_to_skip;

    rcs_iterator_read (&it_ctx, &byte, sizeof (lit_utf8_byte_t));
    bytes_to_skip = lit_get_unicode_char_size_by_utf8_first_byte (byte);
    rcs_iterator_skip (&it_ctx, bytes_to_skip);

    i += bytes_to_skip;
    length++;
  }

#ifndef JERRY_NDEBUG
  rcs_iterator_skip (&it_ctx, rcs_record_get_alignment_bytes_count (lit));
  JERRY_ASSERT (rcs_iterator_finished (&it_ctx));
#endif

  return length;
} /* lit_charset_literal_get_length */

ecma_number_t
lit_number_literal_get_number (lit_literal_t lit) /**< literal */
{
  return rcs_record_get_number (&rcs_lit_storage, lit);
} /* lit_number_literal_get_number */
