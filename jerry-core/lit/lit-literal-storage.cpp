/* Copyright 2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged
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
#include "rcs-allocator.h"
#include "rcs-iterator.h"
#include "rcs-records.h"

rcs_record_set_t rcs_lit_storage;

/**
 * Create charset record in the literal storage
 *
 * @return pointer to the created record
 */
rcs_record_t *
lit_create_charset_literal (rcs_record_set_t *rec_set_p, /**< recordset */
                            const lit_utf8_byte_t *str_p, /**< string to be placed into the record */
                            const lit_utf8_size_t buf_size) /**< size in bytes of the buffer which holds the string */
{
  const size_t record_size = RCS_CHARSET_HEADER_SIZE + buf_size;
  const size_t aligned_record_size = JERRY_ALIGNUP (record_size, RCS_DYN_STORAGE_LENGTH_UNIT);
  const size_t alignment = aligned_record_size - record_size;

  rcs_record_t *rec_p = rcs_alloc_record (rec_set_p, RCS_RECORD_TYPE_CHARSET, aligned_record_size);

  rcs_record_set_alignment_bytes_count (rec_p, (uint8_t) alignment);
  rcs_record_set_charset (rec_set_p, rec_p, str_p, buf_size);
  rcs_record_set_hash (rec_p, lit_utf8_string_calc_hash (str_p, rcs_record_get_length (rec_p)));

  return rec_p;
} /* lit_create_charset_literal */

/**
 * Create magic string record in the literal storage.
 *
 * @return  pointer to the created record
 */
rcs_record_t *
lit_create_magic_literal (rcs_record_set_t *rec_set_p, /**< recordset */
                          lit_magic_string_id_t id) /**< id of magic string */
{
  rcs_record_t *rec_p = rcs_alloc_record (rec_set_p, RCS_RECORD_TYPE_MAGIC_STR, RCS_MAGIC_STR_HEADER_SIZE);
  rcs_record_set_magic_str_id (rec_p, id);

  return rec_p;
} /* lit_create_magic_literal */

/**
 * Create external magic string record in the literal storage.
 *
 * @return  pointer to the created record
 */
rcs_record_t *
lit_create_magic_literal_ex (rcs_record_set_t *rec_set_p, /**< recordset */
                             lit_magic_string_ex_id_t id) /**< id of magic string */
{
  rcs_record_t *rec_p = rcs_alloc_record (rec_set_p, RCS_RECORD_TYPE_MAGIC_STR_EX, RCS_MAGIC_STR_HEADER_SIZE);
  rcs_record_set_magic_str_ex_id (rec_p, id);

  return rec_p;
} /* lit_create_magic_record_ex */

/**
 * Create number record in the literal storage.
 *
 * @return  pointer to the created record
 */
rcs_record_t *
lit_create_number_literal (rcs_record_set_t *rec_set_p, /** recordset */
                           ecma_number_t num) /**< numeric value */
{
  const size_t record_size = RCS_NUMBER_HEADER_SIZE + sizeof (ecma_number_t);
  rcs_record_t *rec_p = rcs_alloc_record (rec_set_p, RCS_RECORD_TYPE_NUMBER, record_size);

  rcs_iterator_t it_ctx = rcs_iterator_create (rec_set_p, rec_p);
  rcs_iterator_skip (&it_ctx, RCS_NUMBER_HEADER_SIZE);
  rcs_iterator_write (&it_ctx, &num, sizeof (ecma_number_t));

  return rec_p;
} /* lit_create_number_literal */

/**
 * Count literal records in the storage
 *
 * @return number of literals
 */
uint32_t
lit_count_literals (rcs_record_set_t *rec_set_p) /**< recordset */
{
  uint32_t num = 0;
  rcs_record_t *rec_p;

  for (rec_p = rcs_record_get_first (rec_set_p);
       rec_p != NULL;
       rec_p = rcs_record_get_next (rec_set_p, rec_p))
  {
    if (rcs_record_get_type (rec_p) >= RCS_RECORD_TYPE_FIRST)
    {
      num++;
    }
  }

  return num;
} /* lit_count_literals */

/**
 * Dump the contents of the literal storage.
 */
void
lit_dump_literals (rcs_record_set_t *rec_set_p) /**< recordset */
{
  rcs_record_t *rec_p;

  printf ("LITERALS:\n");

  for (rec_p = rcs_record_get_first (rec_set_p);
       rec_p != NULL;
       rec_p = rcs_record_get_next (rec_set_p, rec_p))
  {
    printf ("%p ", rec_p);
    printf ("[%3zu] ", rcs_record_get_size (rec_p));

    switch (rcs_record_get_type (rec_p))
    {
      case RCS_RECORD_TYPE_CHARSET:
      {
        rcs_iterator_t it_ctx = rcs_iterator_create (rec_set_p, rec_p);
        rcs_iterator_skip (&it_ctx, RCS_CHARSET_HEADER_SIZE);

        size_t str_len = rcs_record_get_length (rec_p);
        size_t i;

        for (i = 0; i < str_len; ++i)
        {
          FIXME ("Support proper printing of characters which occupy more than one byte.")

          lit_utf8_byte_t chr;
          rcs_iterator_read (&it_ctx, &chr, sizeof (lit_utf8_byte_t));
          rcs_iterator_skip (&it_ctx, sizeof (lit_utf8_byte_t));

          printf ("%c", chr);
        }

        printf (" : STRING");

        break;
      }
      case RCS_RECORD_TYPE_MAGIC_STR:
      {
        lit_magic_string_id_t id = rcs_record_get_magic_str_id (rec_p);
        printf ("%s : MAGIC STRING", lit_get_magic_string_utf8 (id));
        printf (" [id=%d] ", id);

        break;
      }
      case RCS_RECORD_TYPE_MAGIC_STR_EX:
      {
        lit_magic_string_ex_id_t id = rcs_record_get_magic_str_ex_id (rec_p);
        printf ("%s : EXT MAGIC STRING", lit_get_magic_string_ex_utf8 (id));
        printf (" [id=%d] ", id);

        break;
      }
      case RCS_RECORD_TYPE_NUMBER:
      {
        ecma_number_t value = rcs_record_get_number (rec_set_p, rec_p);

        if (ecma_number_is_nan (value))
        {
          printf ("%s : NUMBER", "NaN");
        }
        else
        {
          lit_utf8_byte_t buff[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER + 1u];
          memset (buff, 0, sizeof (buff));

          lit_utf8_size_t sz = ecma_number_to_utf8_string (value, buff, sizeof (buff));
          JERRY_ASSERT (sz <= ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER);

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
} /* lit_dump_literals */
