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

/**
 * Initialize literal storage
 */
void
lit_init ()
{
  new (&lit_storage) lit_literal_storage_t ();
  lit_storage.init ();
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
lit_create_literal_from_charset (const ecma_char_t *str, /**< string to initialize the record,
                                                          * could be non-zero-terminated */
                                 ecma_length_t len)      /**< length of the string */
{
  JERRY_ASSERT (str || !len);
  for (ecma_magic_string_id_t msi = (ecma_magic_string_id_t) 0;
       msi < ECMA_MAGIC_STRING__COUNT;
       msi = (ecma_magic_string_id_t) (msi + 1))
  {
    if (ecma_zt_string_length (ecma_get_magic_string_zt (msi)) != len)
    {
      continue;
    }

    if (!strncmp ((const char *) str, (const char *) ecma_get_magic_string_zt (msi), len))
    {
      return lit_storage.create_magic_record (msi);
    }
  }

  for (ecma_magic_string_ex_id_t msi = (ecma_magic_string_ex_id_t) 0;
       msi < ecma_get_magic_string_ex_count ();
       msi = (ecma_magic_string_ex_id_t) (msi + 1))
  {
    if (ecma_zt_string_length (ecma_get_magic_string_ex_zt (msi)) != len)
    {
      continue;
    }

    if (!strncmp ((const char *) str, (const char *) ecma_get_magic_string_ex_zt (msi), len))
    {
      return lit_storage.create_magic_record_ex (msi);
    }
  }

  return lit_storage.create_charset_record (str, len * sizeof (ecma_char_t));
} /* lit_create_literal_from_charset */

/**
 * Find a literal in literal storage.
 * Only charset and magic string records are checked during search.
 *
 * @return pointer to a literal or NULL if no corresponding literal exists
 */
literal_t
lit_find_literal_by_charset (const ecma_char_t *str, /**< a string to search for */
                             ecma_length_t len)      /**< length of the string */
{
  JERRY_ASSERT (str || !len);
  for (literal_t lit = lit_storage.get_first (); lit != NULL; lit = lit_storage.get_next (lit))
  {
    rcs_record_t::type_t type = lit->get_type ();

    if (type == LIT_STR_T)
    {
      if (static_cast<lit_charset_record_t *>(lit)->get_length () != len)
      {
        continue;
      }

      if (!static_cast<lit_charset_record_t *>(lit)->compare_zt (str, len))
      {
        return lit;
      }
    }
    else if (type == LIT_MAGIC_STR_T)
    {
      ecma_magic_string_id_t magic_id = lit_magic_record_get_magic_str_id (lit);
      const char *magic_str = (const char *) ecma_get_magic_string_zt (magic_id);

      if (strlen (magic_str) != len)
      {
        continue;
      }

      if (!strncmp (magic_str, (const char *) str, strlen (magic_str)))
      {
        return lit;
      }
    }
    else if (type == LIT_MAGIC_STR_EX_T)
    {
      ecma_magic_string_ex_id_t magic_id = lit_magic_record_ex_get_magic_str_id (lit);
      const char *magic_str = (const char *) ecma_get_magic_string_ex_zt (magic_id);

      if (strlen (magic_str) != len)
      {
        continue;
      }

      if (!strncmp (magic_str, (const char *) str, strlen (magic_str)))
      {
        return lit;
      }
    }
  }

  return NULL;
} /* lit_find_literal_by_charset */

/**
 * Check if a literal which holds the passed string exists.
 * If it doesn't exist, create a new one.
 *
 * @return pointer to existing or newly created record
 */
literal_t
lit_find_or_create_literal_from_charset (const ecma_char_t *str, /**< string, could be non-zero-terminated */
                                         ecma_length_t len)      /**< length of the string */
{
  literal_t lit = lit_find_literal_by_charset (str, len);

  if (lit == NULL)
  {
    lit = lit_create_literal_from_charset (str, len);
  }

  return lit;
} /* lit_find_or_create_literal_from_s */


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
 * @return true if equal
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
      return static_cast<lit_charset_record_t *>(lit)->equal (record);
    }
    case LIT_MAGIC_STR_T:
    {
      return record->equal_zt (ecma_get_magic_string_zt (lit_magic_record_get_magic_str_id (lit)));
    }
    case LIT_MAGIC_STR_EX_T:
    {
      return record->equal_zt (ecma_get_magic_string_ex_zt (lit_magic_record_ex_get_magic_str_id (lit)));
    }
    case LIT_NUMBER_T:
    {
      ecma_char_t buff[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER];
      ecma_number_to_zt_string (static_cast<lit_number_record_t *>(lit)->get_number (),
                                buff,
                                ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER);

      return record->equal_zt (buff);
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* lit_literal_equal_charset_rec */

/**
 * Check if literal equals to zero-terminated string
 *
 * @return true if equal
 *         false otherwise
 */
bool
lit_literal_equal_zt (literal_t lit,          /**< literal to compare */
                      const ecma_char_t *str) /**< zero-terminated string to compare */
{
  switch (lit->get_type ())
  {
    case LIT_STR_T:
    {
      return static_cast<lit_charset_record_t *>(lit)->equal_zt (str);
    }
    case LIT_MAGIC_STR_T:
    {
      ecma_magic_string_id_t magic_id = lit_magic_record_get_magic_str_id (lit);
      return ecma_compare_zt_strings (str, ecma_get_magic_string_zt (magic_id));
    }
    case LIT_MAGIC_STR_EX_T:
    {
      ecma_magic_string_ex_id_t magic_id = lit_magic_record_ex_get_magic_str_id (lit);
      return ecma_compare_zt_strings (str, ecma_get_magic_string_ex_zt (magic_id));
    }
    case LIT_NUMBER_T:
    {
      ecma_char_t buff[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER];
      ecma_number_to_zt_string (static_cast<lit_number_record_t *>(lit)->get_number (),
                                buff,
                                ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER);

      return ecma_compare_zt_strings (str, buff);
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* lit_literal_equal_zt */

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
  ecma_char_t buff[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER];
  ecma_number_to_zt_string (num, buff, ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER);

  return lit_literal_equal_zt (lit, buff);
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
      return lit_literal_equal_zt (lit1, ecma_get_magic_string_zt (lit_magic_record_get_magic_str_id (lit2)));
    }
    case lit_literal_storage_t::LIT_MAGIC_STR_EX:
    {
      return lit_literal_equal_zt (lit1, ecma_get_magic_string_ex_zt (lit_magic_record_ex_get_magic_str_id (lit2)));
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
 * Check if literal equals to zero-terminated string.
 * Check that literal is a string literal before performing detailed comparison.
 *
 * @return true if equal
 *         false otherwise
 */
bool
lit_literal_equal_type_zt (literal_t lit,          /**< literal to compare */
                           const ecma_char_t *str) /**< zero-terminated string */
{
  if (lit->get_type () != LIT_STR_T
      && lit->get_type () != LIT_MAGIC_STR_T
      && lit->get_type () != LIT_MAGIC_STR_EX_T)
  {
    return false;
  }

  return lit_literal_equal_zt (lit, str);
} /* lit_literal_equal_type_zt */

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
const ecma_char_t *
lit_literal_to_charset (literal_t lit,     /**< literal to be processed */
                        ecma_char_t *buff, /**< buffer to use as a string storage */
                        size_t size)       /**< size of the buffer */
{
  JERRY_ASSERT (buff != NULL && size > sizeof (ecma_char_t));
  rcs_record_t::type_t type = lit->get_type ();

  switch (type)
  {
    case LIT_STR_T:
    {
      lit_charset_record_t *ch_rec_p = static_cast<lit_charset_record_t *> (lit);
      ecma_length_t index = ch_rec_p->get_charset (buff, size);

      if (index != 0 && ((size_t)index + 1) * sizeof (ecma_char_t) > size)
      {
        index--;
      }
      buff[index] = '\0';

      return buff;
    }
    case LIT_MAGIC_STR_T:
    {
      return ecma_get_magic_string_zt (lit_magic_record_get_magic_str_id (lit));
    }
    case LIT_MAGIC_STR_EX_T:
    {
      return ecma_get_magic_string_ex_zt (lit_magic_record_ex_get_magic_str_id (lit));
    }
    case LIT_NUMBER_T:
    {
      ecma_number_to_zt_string (static_cast<lit_number_record_t *> (lit)->get_number (), buff, (ssize_t)size);

      return buff;
    }
    default: JERRY_UNREACHABLE ();
  }

  JERRY_UNREACHABLE ();
} /* lit_literal_to_charset */

/**
 * Get the contents of the literal as a C string.
 * If literal holds a very long string, it would be trimmed to ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER characters.
 *
 * @return pointer to the C string.
 */
const char *
lit_literal_to_str_internal_buf (literal_t lit) /**< literal */
{
  const ecma_length_t buff_size = ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER;
  static ecma_char_t buff[buff_size];

  return (const char *)lit_literal_to_charset (lit, buff, buff_size);
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

ecma_string_hash_t
lit_charset_literal_get_hash (literal_t lit) /**< literal */
{
  return static_cast<lit_charset_record_t *> (lit)->get_hash ();
} /* lit_charset_literal_get_hash */

ecma_magic_string_id_t
lit_magic_record_get_magic_str_id (literal_t lit) /**< literal */
{
  return static_cast<lit_magic_record_t *> (lit)->get_magic_str_id<ecma_magic_string_id_t> ();
} /* lit_magic_record_get_magic_str_id */

ecma_magic_string_ex_id_t
lit_magic_record_ex_get_magic_str_id (literal_t lit) /**< literal */
{
  return static_cast<lit_magic_record_t *> (lit)->get_magic_str_id<ecma_magic_string_ex_id_t> ();
} /* lit_magic_record_ex_get_magic_str_id */

int32_t
lit_charset_record_get_length (literal_t lit) /**< literal */
{
  return static_cast<lit_charset_record_t *> (lit)->get_length ();;
} /* lit_charset_record_get_length */

ecma_number_t
lit_charset_literal_get_number (literal_t lit) /**< literal */
{
  return static_cast<lit_number_record_t *> (lit)->get_number ();;
} /* lit_charset_literal_get_number */
