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
#include "lit-magic-strings.h"

/**
 * Initialize literal storage
 */
void
lit_init ()
{
  new (&lit_storage) lit_literal_storage_t ();
  lit_storage.init ();
  lit_magic_strings_init ();
  lit_magic_strings_ex_init ();
} /* lit_init */

/**
 * Finalize literal storage
 */
void
lit_finalize ()
{
  lit_storage.cleanup ();
  lit_storage.finalize ();
} /* lit_finalize */

/**
 * Dump records from the literal storage
 */
void
lit_dump_literals ()
{
  lit_storage.dump ();
} /* lit_dump_literals */

/**
 * Create new literal in literal storage from characters buffer.
 * Don't check if the same literal already exists.
 *
 * @return pointer to created record
 */
literal_t
lit_create_literal_from_utf8_string (const lit_utf8_byte_t *str_p, /**< string to initialize the record,
                                                                  * could be non-zero-terminated */
                                     lit_utf8_size_t str_size) /**< length of the string */
{
  JERRY_ASSERT (str_p || !str_size);
  for (lit_magic_string_id_t msi = (lit_magic_string_id_t) 0;
       msi < LIT_MAGIC_STRING__COUNT;
       msi = (lit_magic_string_id_t) (msi + 1))
  {
    if (lit_get_magic_string_size (msi) != str_size)
    {
      continue;
    }

    if (!strncmp ((const char *) str_p, (const char *) lit_get_magic_string_utf8 (msi), str_size))
    {
      return lit_storage.create_magic_record (msi);
    }
  }

  for (lit_magic_string_ex_id_t msi = (lit_magic_string_ex_id_t) 0;
       msi < lit_get_magic_string_ex_count ();
       msi = (lit_magic_string_ex_id_t) (msi + 1))
  {
    if (lit_get_magic_string_ex_size (msi) != str_size)
    {
      continue;
    }

    if (!strncmp ((const char *) str_p, (const char *) lit_get_magic_string_ex_utf8 (msi), str_size))
    {
      return lit_storage.create_magic_record_ex (msi);
    }
  }

  return lit_storage.create_charset_record (str_p, str_size);
} /* lit_create_literal_from_utf8_string */

/**
 * Find a literal in literal storage.
 * Only charset and magic string records are checked during search.
 *
 * @return pointer to a literal or NULL if no corresponding literal exists
 */
literal_t
lit_find_literal_by_utf8_string (const lit_utf8_byte_t *str_p, /**< a string to search for */
                                 lit_utf8_size_t str_size)        /**< length of the string */
{
  JERRY_ASSERT (str_p || !str_size);
  for (literal_t lit = lit_storage.get_first (); lit != NULL; lit = lit_storage.get_next (lit))
  {
    rcs_record_t::type_t type = lit->get_type ();

    if (type == LIT_STR_T)
    {
      if (static_cast<lit_charset_record_t *>(lit)->get_length () != str_size)
      {
        continue;
      }

      if (!static_cast<lit_charset_record_t *>(lit)->compare_utf8 (str_p, str_size))
      {
        return lit;
      }
    }
    else if (type == LIT_MAGIC_STR_T)
    {
      lit_magic_string_id_t magic_id = lit_magic_record_get_magic_str_id (lit);
      const lit_utf8_byte_t *magic_str_p = lit_get_magic_string_utf8 (magic_id);

      if (lit_zt_utf8_string_size (magic_str_p) != str_size)
      {
        continue;
      }

      if (!strncmp ((const char *) magic_str_p, (const char *) str_p, str_size))
      {
        return lit;
      }
    }
    else if (type == LIT_MAGIC_STR_EX_T)
    {
      lit_magic_string_ex_id_t magic_id = lit_magic_record_ex_get_magic_str_id (lit);
      const lit_utf8_byte_t *magic_str_p = lit_get_magic_string_ex_utf8 (magic_id);

      if (lit_zt_utf8_string_size (magic_str_p) != str_size)
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
literal_t
lit_find_or_create_literal_from_utf8_string (const lit_utf8_byte_t *str_p, /**< string, could be non-zero-terminated */
                                             lit_utf8_size_t str_size) /**< length of the string */
{
  literal_t lit = lit_find_literal_by_utf8_string (str_p, str_size);

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
literal_t
lit_create_literal_from_num (ecma_number_t num) /**< number to initialize a new number literal */
{
  return lit_storage.create_number_record (num);
} /* lit_create_literal_from_num */

/**
 * Find existing or create new number literal in literal storage.
 *
 * @return pointer to existing or a newly created record
 */
literal_t
lit_find_or_create_literal_from_num (ecma_number_t num) /**< number which a literal should contain */
{
  literal_t lit = lit_find_literal_by_num (num);

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
literal_t
lit_find_literal_by_num (ecma_number_t num) /**< a number to search for */
{
  for (literal_t lit = lit_storage.get_first (); lit != NULL; lit = lit_storage.get_next (lit))
  {
    rcs_record_t::type_t type = lit->get_type ();

    if (type != LIT_NUMBER_T)
    {
      continue;
    }

    ecma_number_t lit_num = static_cast<lit_number_record_t *>(lit)->get_number ();

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
lit_literal_equal_charset_rec (literal_t lit,                /**< literal to compare */
                               lit_charset_record_t *record) /**< charset record to compare */
{
  switch (lit->get_type ())
  {
    case LIT_STR_T:
    {
      return static_cast<lit_charset_record_t *>(lit)->is_equal (record);
    }
    case LIT_MAGIC_STR_T:
    {
      lit_magic_string_id_t magic_string_id = lit_magic_record_get_magic_str_id (lit);
      return record->is_equal_utf8_string (lit_get_magic_string_utf8 (magic_string_id),
                                           lit_get_magic_string_size (magic_string_id));
    }
    case LIT_MAGIC_STR_EX_T:
    {
      lit_magic_string_ex_id_t magic_string_id = lit_magic_record_ex_get_magic_str_id (lit);
      return record->is_equal_utf8_string (lit_get_magic_string_ex_utf8 (magic_string_id),
                                           lit_get_magic_string_ex_size (magic_string_id));
    }
    case LIT_NUMBER_T:
    {
      lit_utf8_byte_t buff[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER];
      lit_utf8_size_t copied = ecma_number_to_utf8_string (static_cast<lit_number_record_t *>(lit)->get_number (),
                                                           buff,
                                                           sizeof (buff));

      return record->is_equal_utf8_string (buff, copied);
    }
    default:
    {
      JERRY_UNREACHABLE ();
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
lit_literal_equal_utf8 (literal_t lit, /**< literal to compare */
                        const lit_utf8_byte_t *str_p, /**< utf-8 string to compare */
                        lit_utf8_size_t str_size) /**< string size in bytes */
{
  switch (lit->get_type ())
  {
    case LIT_STR_T:
    {
      return static_cast<lit_charset_record_t *>(lit)->is_equal_utf8_string (str_p, str_size);
    }
    case LIT_MAGIC_STR_T:
    {
      lit_magic_string_id_t magic_id = lit_magic_record_get_magic_str_id (lit);
      return lit_compare_utf8_string_and_magic_string (str_p, str_size, magic_id);
    }
    case LIT_MAGIC_STR_EX_T:
    {
      lit_magic_string_ex_id_t magic_id = lit_magic_record_ex_get_magic_str_id (lit);
      return lit_compare_utf8_string_and_magic_string_ex (str_p, str_size, magic_id);
    }
    case LIT_NUMBER_T:
    {
      lit_utf8_byte_t num_buf[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER];
      lit_utf8_size_t num_size = ecma_number_to_utf8_string (static_cast<lit_number_record_t *>(lit)->get_number (),
                                                             num_buf,
                                                             sizeof (num_buf));

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
lit_literal_equal_num (literal_t lit,     /**< literal to check */
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
lit_literal_equal (literal_t lit1, /**< first literal */
                   literal_t lit2) /**< second literal */
{
  switch (lit2->get_type ())
  {
    case lit_literal_storage_t::LIT_STR:
    {
      return lit_literal_equal_charset_rec (lit1, static_cast<lit_charset_record_t *>(lit2));
    }
    case lit_literal_storage_t::LIT_MAGIC_STR:
    {
      lit_magic_string_id_t magic_str_id = lit_magic_record_get_magic_str_id (lit2);
      return lit_literal_equal_utf8 (lit1,
                                     lit_get_magic_string_utf8 (magic_str_id),
                                     lit_get_magic_string_size (magic_str_id));
    }
    case lit_literal_storage_t::LIT_MAGIC_STR_EX:
    {
      lit_magic_string_ex_id_t magic_str_ex_id = lit_magic_record_ex_get_magic_str_id (lit2);
      return lit_literal_equal_utf8 (lit1,
                                     lit_get_magic_string_ex_utf8 (magic_str_ex_id),
                                     lit_get_magic_string_ex_size (magic_str_ex_id));
    }
    case lit_literal_storage_t::LIT_NUMBER:
    {
      return lit_literal_equal_num (lit1, static_cast<lit_number_record_t *>(lit2)->get_number ());
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
lit_literal_equal_type_utf8 (literal_t lit, /**< literal to compare */
                             const lit_utf8_byte_t *str_p, /**< utf-8 string */
                             lit_utf8_size_t str_size) /**< string size */
{
  if (lit->get_type () != LIT_STR_T
      && lit->get_type () != LIT_MAGIC_STR_T
      && lit->get_type () != LIT_MAGIC_STR_EX_T)
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
lit_literal_equal_type_cstr (literal_t lit, /**< literal to compare */
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
lit_literal_equal_type_num (literal_t lit,     /**< literal to check */
                            ecma_number_t num) /**< number to compare with */
{
  if (lit->get_type () != lit_literal_storage_t::LIT_NUMBER)
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
lit_literal_equal_type (literal_t lit1, /**< first literal */
                        literal_t lit2) /**< second literal */
{
  if (lit1->get_type () != lit2->get_type ())
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
lit_literal_to_utf8_string (literal_t lit, /**< literal to be processed */
                            lit_utf8_byte_t *buff_p, /**< buffer to use as a string storage */
                            size_t size) /**< size of the buffer */
{
  JERRY_ASSERT (buff_p != NULL && size > 0);
  rcs_record_t::type_t type = lit->get_type ();

  switch (type)
  {
    case LIT_STR_T:
    {
      lit_charset_record_t *ch_rec_p = static_cast<lit_charset_record_t *> (lit);
      ch_rec_p->get_charset (buff_p, size);
      return buff_p;
    }
    case LIT_MAGIC_STR_T:
    {
      return lit_get_magic_string_utf8 (lit_magic_record_get_magic_str_id (lit));
    }
    case LIT_MAGIC_STR_EX_T:
    {
      return lit_get_magic_string_ex_utf8 (lit_magic_record_ex_get_magic_str_id (lit));
    }
    case LIT_NUMBER_T:
    {
      ecma_number_to_utf8_string (static_cast<lit_number_record_t *> (lit)->get_number (), buff_p, (ssize_t)size);

      return buff_p;
    }
    default: JERRY_UNREACHABLE ();
  }

  JERRY_UNREACHABLE ();
} /* lit_literal_to_utf8_string */

/**
 * Get the contents of the literal as a C string.
 * If literal holds a very long string, it would be trimmed to ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER characters.
 *
 * @return pointer to the C string.
 */
const char *
lit_literal_to_str_internal_buf (literal_t lit) /**< literal */
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
lit_literal_exists (literal_t lit) /**< literal to check for existence */
{
  for (literal_t l = lit_storage.get_first (); l != NULL; l = lit_storage.get_next (l))
  {
    if (l == lit)
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
literal_t
lit_get_literal_by_cp (lit_cpointer_t lit_cp) /**< compressed pointer to literal */
{
  JERRY_ASSERT (lit_cp.packed_value != MEM_CP_NULL);
  literal_t lit = lit_cpointer_t::decompress (lit_cp);
  JERRY_ASSERT (lit_literal_exists (lit));

  return lit;
} /* lit_get_literal_by_cp */

lit_string_hash_t
lit_charset_literal_get_hash (literal_t lit) /**< literal */
{
  return static_cast<lit_charset_record_t *> (lit)->get_hash ();
} /* lit_charset_literal_get_hash */

lit_magic_string_id_t
lit_magic_record_get_magic_str_id (literal_t lit) /**< literal */
{
  return static_cast<lit_magic_record_t *> (lit)->get_magic_str_id<lit_magic_string_id_t> ();
} /* lit_magic_record_get_magic_str_id */

lit_magic_string_ex_id_t
lit_magic_record_ex_get_magic_str_id (literal_t lit) /**< literal */
{
  return static_cast<lit_magic_record_t *> (lit)->get_magic_str_id<lit_magic_string_ex_id_t> ();
} /* lit_magic_record_ex_get_magic_str_id */

lit_utf8_size_t
lit_charset_record_get_size (literal_t lit) /**< literal */
{
  return static_cast<lit_charset_record_t *> (lit)->get_length ();
} /* lit_charset_record_get_size */

/**
 * Get length of the literal
 *
 * @return code units count
 */
ecma_length_t
lit_charset_record_get_length (literal_t lit) /**< literal */
{
  TODO ("Add special case for literals which doesn't contain long characters");

  lit_charset_record_t *charset_record_p = static_cast<lit_charset_record_t *> (lit);
  rcs_record_iterator_t lit_iter (&lit_storage, lit);
  lit_iter.skip (lit_charset_record_t::header_size ());

  lit_utf8_size_t lit_utf8_str_size = charset_record_p->get_length ();
  ecma_length_t length = 0;
  for (lit_utf8_size_t i = 0; i < lit_utf8_str_size;)
  {
    lit_utf8_byte_t byte = lit_iter.read <lit_utf8_byte_t> ();
    lit_utf8_size_t bytes_to_skip = lit_get_unicode_char_size_by_utf8_first_byte (byte);
    lit_iter.skip (bytes_to_skip);
    i += bytes_to_skip;

    length += (bytes_to_skip > LIT_UTF8_MAX_BYTES_IN_CODE_UNIT) ? 2 : 1;
  }

#ifndef JERRY_NDEBUG
  lit_iter.skip (charset_record_p->get_alignment_bytes_count ());
  JERRY_ASSERT (lit_iter.finished ());
#endif

  return length;
} /* lit_charset_record_get_length */

ecma_number_t
lit_charset_literal_get_number (literal_t lit) /**< literal */
{
  return static_cast<lit_number_record_t *> (lit)->get_number ();;
} /* lit_charset_literal_get_number */
