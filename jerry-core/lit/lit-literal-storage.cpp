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

#include "lit-literal-storage.h"
#include "ecma-helpers.h"

/**
 * Literal storage
 * As our libc library doesn't call constructors of static variables, lit_storage is initialized
 * in lit_init function by placement new operator.
 */
lit_literal_storage_t lit_storage;

/**
 * Get pointer to the previous record inside the literal storage
 *
 * @return pointer to the previous record of NULL if this is the first record ('prev' field in the header)
 */
rcs_record_t *
lit_charset_record_t::get_prev () const
{
  rcs_record_iterator_t it ((rcs_recordset_t *)&lit_storage, (rcs_record_t *)this);
  it.skip (RCS_DYN_STORAGE_LENGTH_UNIT);

  cpointer_t cpointer;
  cpointer.packed_value = it.read<uint16_t> ();

  return cpointer_t::decompress (cpointer);
} /* lit_charset_record_t::get_prev */

/**
 * Set the pointer to the previous record inside the literal storage (sets 'prev' field in the header)
 */
void
lit_charset_record_t::set_prev (rcs_record_t *prev_rec_p) /**< pointer to the record to set as previous */
{
  rcs_record_iterator_t it ((rcs_recordset_t *)&lit_storage, (rcs_record_t *)this);
  it.skip (RCS_DYN_STORAGE_LENGTH_UNIT);

  it.write<uint16_t> (cpointer_t::compress (prev_rec_p).packed_value);
} /* lit_charset_record_t::set_prev */

/**
 * Set the charset of the record
 */
void
lit_charset_record_t::set_charset (const ecma_char_t *str, /**< buffer containing characters to set */
                                   size_t size)            /**< size of the buffer in bytes */
{
  JERRY_ASSERT (header_size () + size == get_size () - get_alignment_bytes_count ());

  rcs_record_iterator_t it ((rcs_recordset_t *)&lit_storage, (rcs_record_t *)this);
  it.skip (header_size ());

  for (size_t i = 0; i < get_length (); ++i)
  {
    it.write<ecma_char_t> (str[i]);
    it.skip<ecma_char_t> ();
  }
} /* lit_charset_record_t::set_charset */

/**
 * Get the characters which are stored to the record
 *
 * @return number of code units written to the buffer
 */
ecma_length_t
lit_charset_record_t::get_charset (ecma_char_t *buff, /**< output buffer */
                                   size_t size) /**< size of the output buffer in bytes */
{
  JERRY_ASSERT (buff && size >= sizeof (ecma_char_t));

  rcs_record_iterator_t it ((rcs_recordset_t *)&lit_storage, (rcs_record_t *)this);
  it.skip (header_size ());
  ecma_length_t len = get_length ();
  size_t i;

  for (i = 0; i < len && size > sizeof (ecma_char_t); ++i)
  {
    buff[i] = it.read<ecma_char_t> ();
    it.skip<ecma_char_t> ();
    size -= sizeof (ecma_char_t);
  }

  return (ecma_length_t) i;
} /* lit_charset_record_t::get_charset */

/**
 * Compares characters from the record to the string
 *
 * @return  0 if strings are equal
 *         -1 if str2 is greater
 *          1 if str2 is less
 */
int
lit_charset_record_t::compare_zt (const ecma_char_t *str_to_compare_with, /**< buffer with string to compare */
                                  size_t length)                          /**< length of the string in buffer str2 */
{
  size_t i;

  if (get_length () == 0)
  {
    if (str_to_compare_with != NULL)
    {
      return -1;
    }
    else
    {
      return 0;
    }
  }

  if (str_to_compare_with == NULL)
  {
    return 1;
  }

  rcs_record_iterator_t it_this (&lit_storage, this);

  it_this.skip (header_size ());

  for (i = 0; i < get_length () && i < length; i++)
  {
    ecma_char_t chr = it_this.read<ecma_char_t> ();

    if (chr > str_to_compare_with[i])
    {
      return 1;
    }
    else if (chr < str_to_compare_with[i])
    {
      return -1;
    }

    it_this.skip<ecma_char_t> ();
  }

  if (i < length)
  {
    return -1;
  }

  return 0;
} /* lit_charset_record_t::compare_zt */

/**
 * Compares two lit_charset_record_t records for equality
 *
 * @return  true if strings inside records are equal
 *          false otherwise
 */
bool
lit_charset_record_t::equal (lit_charset_record_t *rec) /**< charset record to compare with */
{
  if (get_length () != rec->get_length ())
  {
    return false;
  }

  rcs_record_iterator_t it_this (&lit_storage, this);
  rcs_record_iterator_t it_record (&lit_storage, rec);

  it_this.skip (header_size ());
  it_record.skip (rec->header_size ());

  for (ecma_length_t i = 0; i < get_length (); i++)
  {
    if (it_this.read<ecma_char_t> () != it_record.read<ecma_char_t> ())
    {
      return false;
    }

    it_this.skip<ecma_char_t> ();
    it_record.skip<ecma_char_t> ();
  }

  return true;
} /* lit_charset_record_t::equal */

/**
 * Compares this lit_charset_record_t records with zero-terminated string for equality
 *
 * @return  true if compared instances are equal
 *          false otherwise
 */
bool
lit_charset_record_t::equal_zt (const ecma_char_t *str) /**< zero-terminated string */
{
  return equal_non_zt (str, ecma_zt_string_length (str));
} /* lit_charset_record_t::equal_zt */

/**
 * Compare this lit_charset_record_t record with string (which could contain '\0' characters) for equality
 *
 * @return  true if compared instances are equal
 *          false otherwise
 */
bool
lit_charset_record_t::equal_non_zt (const ecma_char_t *str, /**< string to compare with */
                                    ecma_length_t len)      /**< length of the string */
{
  rcs_record_iterator_t it_this (&lit_storage, this);

  it_this.skip (header_size ());

  for (ecma_length_t i = 0; i < get_length () && i < len; i++)
  {
    if (it_this.read<ecma_char_t> () != str[i])
    {
      return false;
    }

    it_this.skip<ecma_char_t> ();
  }

  return get_length () == len;
} /* lit_charset_record_t::equal_non_zt */

/**
 * Create charset record in the literal storage
 *
 * @return  pointer to the created record
 */
lit_charset_record_t *
lit_literal_storage_t::create_charset_record (const ecma_char_t *str, /**< string to be placed in the record */
                                              size_t buf_size)        /**< size in bytes of the buffer which holds the
                                                                       * string */
{
  const size_t alignment = lit_charset_record_t::size (buf_size) - (lit_charset_record_t::header_size () + buf_size);

  lit_charset_record_t *ret = alloc_record<lit_charset_record_t, size_t> (LIT_STR, buf_size);

  ret->set_alignment_bytes_count (alignment);
  ret->set_charset (str, buf_size);
  ret->set_hash (ecma_chars_buffer_calc_hash_last_chars (str, ret->get_length ()));

  return ret;
} /* lit_literal_storage_t::create_charset_record */

/**
 * Create magic string record in the literal storage
 *
 * @return  pointer to the created record
 */
lit_magic_record_t *
lit_literal_storage_t::create_magic_record (ecma_magic_string_id_t id) /**< id of a magic string */
{
  lit_magic_record_t *ret = alloc_record<lit_magic_record_t> (LIT_MAGIC_STR);

  ret->set_magic_str_id (id);

  return ret;
} /* lit_literal_storage_t::create_magic_record */

/**
 * Create number record in the literal storage
 *
 * @return  pointer to the created record
 */
lit_number_record_t *
lit_literal_storage_t::create_number_record (ecma_number_t num) /**< number */
{
  lit_number_record_t *ret = alloc_record<lit_number_record_t> (LIT_NUMBER);
  rcs_record_iterator_t it_this (this, ret);

  it_this.skip (ret->header_size ());
  it_this.write<ecma_number_t> (num);

  return ret;
} /* lit_literal_storage_t::create_number_record */

/**
 * Dump the contents of the literal storage
 */
void
lit_literal_storage_t::dump ()
{
  printf ("LITERALS:\n");

  for (rcs_record_t *rec_p = lit_storage.get_first (); rec_p != NULL; rec_p = lit_storage.get_next (rec_p))
  {
    printf ("%p ", rec_p);
    printf ("[%3zu] ", get_record_size (rec_p));

    switch (rec_p->get_type ())
    {
      case LIT_STR:
      {
        lit_charset_record_t *lit_p = static_cast<lit_charset_record_t *> (rec_p);
        rcs_record_iterator_t it_this (this, rec_p);

        it_this.skip (lit_charset_record_t::header_size ());

        for (size_t i = 0; i < lit_p->get_length (); ++i)
        {
          printf ("%c", it_this.read<ecma_char_t> ());
          it_this.skip<ecma_char_t> ();
        }

        printf (" : STRING");

        break;
      }
      case LIT_MAGIC_STR:
      {
        lit_magic_record_t *lit_p = static_cast<lit_magic_record_t *> (rec_p);

        printf ("%s : MAGIC STRING", ecma_get_magic_string_zt (lit_p->get_magic_str_id ()));
        printf (" [id=%d] ", lit_p->get_magic_str_id ());

        break;
      }
      case LIT_NUMBER:
      {
        lit_number_record_t *lit_p = static_cast<lit_number_record_t *> (rec_p);

        if (ecma_number_is_nan (lit_p->get_number ()))
        {
          printf ("%s : NUMBER", "NaN");
        }
        else
        {
          ecma_char_t buff[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER];
          ecma_number_to_zt_string (lit_p->get_number (), buff, ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER);
          printf ("%s : NUMBER", buff);
        }

        break;
      }
      default:
      {
        printf (" : EMPTY RECORD");
      }
    }

    printf ("\n");
  }
} /* lit_literal_storage_t::dump */

/**
 * Get previous record in the literal storage
 *
 * @return  pointer to the previous record relative to rec_p, or NULL if rec_p is the first record in the storage
 */
rcs_record_t *
lit_literal_storage_t::get_prev (rcs_record_t *rec_p) /**< record, relative to which previous should be found */
{
  switch (rec_p->get_type ())
  {
    case LIT_STR:
    {
      return (static_cast<lit_charset_record_t *> (rec_p))->get_prev ();
    }
    case LIT_MAGIC_STR:
    {
      return (static_cast<lit_magic_record_t *> (rec_p))->get_prev ();
    }
    case LIT_NUMBER:
    {
      return (static_cast<lit_number_record_t *> (rec_p))->get_prev ();
    }
    default:
    {
      JERRY_ASSERT (rec_p->get_type () < _first_type_id);

      return rcs_recordset_t::get_prev (rec_p);
    }
  }
} /* lit_literal_storage_t::get_prev */

/**
 * Set pointer to the previous record
 */
void
lit_literal_storage_t::set_prev (rcs_record_t *rec_p, /**< record to modify */
                                 rcs_record_t *prev_rec_p) /**< record which should be set as previous */
{
  switch (rec_p->get_type ())
  {
    case LIT_STR:
    {
      return (static_cast<lit_charset_record_t *> (rec_p))->set_prev (prev_rec_p);
    }
    case LIT_MAGIC_STR:
    {
      return (static_cast<lit_magic_record_t *> (rec_p))->set_prev (prev_rec_p);
    }
    case LIT_NUMBER:
    {
      return (static_cast<lit_number_record_t *> (rec_p))->set_prev (prev_rec_p);
    }
    default:
    {
      JERRY_ASSERT (rec_p->get_type () < _first_type_id);

      return rcs_recordset_t::set_prev (rec_p, prev_rec_p);
    }
  }
} /* lit_literal_storage_t::set_prev */

/**
 * Get size of a record
 *
 * @return size of a record in bytes
 */
size_t
lit_literal_storage_t::get_record_size (rcs_record_t* rec_p) /**< pointer to a record */
{
  switch (rec_p->get_type ())
  {
    case LIT_STR:
    {
      return (static_cast<lit_charset_record_t *> (rec_p))->get_size ();
    }
    case LIT_MAGIC_STR:
    {
      return lit_magic_record_t::size ();
    }
    case LIT_NUMBER:
    {
      return lit_number_record_t::size ();
    }
    default:
    {
      JERRY_ASSERT (rec_p->get_type () < _first_type_id);

      return rcs_recordset_t::get_record_size (rec_p);
    }
  }
} /* lit_literal_storage_t::get_record_size */

template void rcs_record_iterator_t::skip<ecma_char_t> ();
template void rcs_record_iterator_t::skip<uint16_t> ();
template void rcs_record_iterator_t::skip<uint32_t> ();

template void rcs_record_iterator_t::write<ecma_char_t> (ecma_char_t);
template ecma_char_t rcs_record_iterator_t::read<ecma_char_t> ();

template void rcs_record_iterator_t::write<ecma_number_t> (ecma_number_t);
template ecma_number_t rcs_record_iterator_t::read<ecma_number_t> ();

template void rcs_record_iterator_t::write<uint16_t> (uint16_t);
template uint16_t rcs_record_iterator_t::read<uint16_t> ();

template lit_charset_record_t *rcs_recordset_t::alloc_record<lit_charset_record_t> (rcs_record_t::type_t type,
                                                                                    size_t size);
template lit_magic_record_t *rcs_recordset_t::alloc_record<lit_magic_record_t> (rcs_record_t::type_t type);
template lit_number_record_t *rcs_recordset_t::alloc_record<lit_number_record_t> (rcs_record_t::type_t type);
