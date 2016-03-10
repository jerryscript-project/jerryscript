/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2015-2016 University of Szeged
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
#include "lit-cpointer.h"

#include "ecma-helpers.h"

lit_record_t *lit_storage = NULL;

/**
 * Create charset record in the literal storage
 *
 * @return pointer to the created record
 */
lit_record_t *
lit_create_charset_literal (const lit_utf8_byte_t *str_p, /**< string to be placed into the record */
                            const lit_utf8_size_t buf_size) /**< size in bytes of the buffer which holds the string */
{
  lit_charset_record_t *rec_p = (lit_charset_record_t *) mem_heap_alloc_block (buf_size + LIT_CHARSET_HEADER_SIZE);

  rec_p->type = LIT_RECORD_TYPE_CHARSET;
  rec_p->next = (uint16_t) lit_cpointer_compress (lit_storage);
  lit_storage = (lit_record_t *) rec_p;

  rec_p->hash = (uint8_t) lit_utf8_string_calc_hash (str_p, buf_size);
  rec_p->size = (uint16_t) buf_size;
  rec_p->length = (uint16_t) lit_utf8_string_length (str_p, buf_size);
  memcpy (rec_p + 1, str_p, buf_size);

  return (lit_record_t *) rec_p;
} /* lit_create_charset_literal */

/**
 * Create magic string record in the literal storage.
 *
 * @return  pointer to the created record
 */
lit_record_t *
lit_create_magic_literal (const lit_magic_string_id_t id) /**< id of magic string */
{
  lit_magic_record_t *rec_p = (lit_magic_record_t *) mem_heap_alloc_block (sizeof (lit_magic_record_t));
  rec_p->type = LIT_RECORD_TYPE_MAGIC_STR;
  rec_p->next = (uint16_t) lit_cpointer_compress (lit_storage);
  lit_storage = (lit_record_t *) rec_p;

  rec_p->magic_id = (uint32_t) id;

  return (lit_record_t *) rec_p;
} /* lit_create_magic_literal */

/**
 * Create external magic string record in the literal storage.
 *
 * @return  pointer to the created record
 */
lit_record_t *
lit_create_magic_literal_ex (const lit_magic_string_ex_id_t id) /**< id of magic string */
{
  lit_magic_record_t *rec_p = (lit_magic_record_t *) mem_heap_alloc_block (sizeof (lit_magic_record_t));
  rec_p->type = LIT_RECORD_TYPE_MAGIC_STR_EX;
  rec_p->next = (uint16_t) lit_cpointer_compress (lit_storage);
  lit_storage = (lit_record_t *) rec_p;

  rec_p->magic_id = (uint32_t) id;

  return (lit_record_t *) rec_p;
} /* lit_create_magic_literal_ex */

/**
 * Create number record in the literal storage.
 *
 * @return  pointer to the created record
 */
lit_record_t *
lit_create_number_literal (const ecma_number_t num) /**< numeric value */
{
  lit_number_record_t *rec_p = (lit_number_record_t *) mem_heap_alloc_block (sizeof (lit_number_record_t));

  rec_p->type = (uint8_t) LIT_RECORD_TYPE_NUMBER;
  rec_p->next = (uint16_t) lit_cpointer_compress (lit_storage);
  lit_storage = (lit_record_t *) rec_p;

  rec_p->number = num;

  return (lit_record_t *) rec_p;
} /* lit_create_number_literal */

/**
 * Get size of stored literal
 *
 * @return size of literal
 */
size_t __attr_pure___
lit_get_literal_size (const lit_record_t *lit_p) /**< literal record */
{
  const lit_record_type_t type = (const lit_record_type_t) lit_p->type;
  size_t size = 0;

  switch (type)
  {
    case LIT_RECORD_TYPE_NUMBER:
    {
      size = sizeof (lit_number_record_t);
      break;
    }
    case LIT_RECORD_TYPE_CHARSET:
    {
      const lit_charset_record_t *const rec_p = (const lit_charset_record_t *) lit_p;
      size = rec_p->size + LIT_CHARSET_HEADER_SIZE;
      break;
    }
    case LIT_RECORD_TYPE_MAGIC_STR:
    case LIT_RECORD_TYPE_MAGIC_STR_EX:
    {
      size = sizeof (lit_magic_record_t);
      break;
    }
    default:
    {
      JERRY_UNREACHABLE ();
      break;
    }
  }

  JERRY_ASSERT (size > 0);
  return size;
} /* lit_get_literal_size */


/**
 * Free stored literal
 *
 * @return pointer to the next literal in the list
 */
lit_record_t *
lit_free_literal (lit_record_t *lit_p) /**< literal record */
{
  lit_record_t *const ret_p = lit_cpointer_decompress (lit_p->next);
  mem_heap_free_block (lit_p, lit_get_literal_size (lit_p));
  return ret_p;
} /* lit_free_literal */

/**
 * Count literal records in the storage
 *
 * @return number of literals
 */
uint32_t
lit_count_literals ()
{
  uint32_t num = 0;
  lit_record_t *rec_p;

  for (rec_p = lit_storage;
       rec_p != NULL;
       rec_p = lit_cpointer_decompress (rec_p->next))
  {
    if (rec_p->type > LIT_RECORD_TYPE_FREE)
    {
      num++;
    }
  }

  return num;
} /* lit_count_literals */

#ifdef JERRY_ENABLE_LOG
/**
 * Dump the contents of the literal storage.
 */
void
lit_dump_literals ()
{
  lit_record_t *rec_p;
  size_t i;

  JERRY_DLOG ("LITERALS:\n");

  for (rec_p = lit_storage;
       rec_p != NULL;
       rec_p = lit_cpointer_decompress (rec_p->next))
  {
    JERRY_DLOG ("%p ", rec_p);
    JERRY_DLOG ("[%3zu] ", lit_get_literal_size (rec_p));

    switch (rec_p->type)
    {
      case LIT_RECORD_TYPE_CHARSET:
      {
        const lit_charset_record_t *const record_p = (const lit_charset_record_t *) rec_p;
        char *str = (char *) (record_p + 1);
        for (i = 0; i < record_p->size; ++i, ++str)
        {
          FIXME ("Support proper printing of characters which occupy more than one byte.")

          JERRY_DLOG ("%c", *str);
        }

        JERRY_DLOG (" : STRING");

        break;
      }
      case LIT_RECORD_TYPE_MAGIC_STR:
      {
        lit_magic_string_id_t id = (lit_magic_string_id_t) ((lit_magic_record_t *) rec_p)->magic_id;
        JERRY_DLOG ("%s : MAGIC STRING", lit_get_magic_string_utf8 (id));
        JERRY_DLOG (" [id=%d] ", id);

        break;
      }
      case LIT_RECORD_TYPE_MAGIC_STR_EX:
      {
        lit_magic_string_ex_id_t id = ((lit_magic_record_t *) rec_p)->magic_id;
        JERRY_DLOG ("%s : EXT MAGIC STRING", lit_get_magic_string_ex_utf8 (id));
        JERRY_DLOG (" [id=%d] ", id);

        break;
      }
      case LIT_RECORD_TYPE_NUMBER:
      {
        const lit_number_record_t *const record_p = (const lit_number_record_t *) rec_p;
        ecma_number_t value;
        memcpy (&value, &record_p->number, sizeof (ecma_number_t));

        if (ecma_number_is_nan (value))
        {
          JERRY_DLOG ("%s : NUMBER", "NaN");
        }
        else
        {
          lit_utf8_byte_t buff[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER + 1u];
          memset (buff, 0, sizeof (buff));

          lit_utf8_size_t sz = ecma_number_to_utf8_string (value, buff, sizeof (buff));
          JERRY_ASSERT (sz <= ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER);

          JERRY_DLOG ("%s : NUMBER", buff);
        }

        break;
      }
      default:
      {
        JERRY_UNREACHABLE ();
      }
    }

    JERRY_DLOG ("\n");
  }
} /* lit_dump_literals */
#endif /* JERRY_ENABLE_LOG */
