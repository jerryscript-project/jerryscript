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

#include "lit-snapshot.h"

#include "lit-literal.h"
#include "lit-literal-storage.h"
#include "rcs-allocator.h"
#include "rcs-iterator.h"
#include "rcs-records.h"

#ifdef JERRY_ENABLE_SNAPSHOT

/**
 * Dump a record to specified snapshot buffer.
 *
 * @return number of bytes dumped,
 *         or 0 - upon dump failure
 */
static uint32_t
lit_snapshot_dump (lit_literal_t lit, /**< literal to dump */
                   uint8_t *buffer_p, /**< buffer to dump to */
                   size_t buffer_size, /**< buffer size */
                   size_t *in_out_buffer_offset_p) /**< in-out: buffer write offset */
{
  rcs_record_type_t record_type = rcs_record_get_type (lit);

  if (RCS_RECORD_TYPE_IS_NUMBER (record_type))
  {
    double num_value = rcs_record_get_number (&rcs_lit_storage, lit);
    size_t size = sizeof (num_value);

    if (!jrt_write_to_buffer_by_offset (buffer_p,
                                        buffer_size,
                                        in_out_buffer_offset_p,
                                        &num_value,
                                        size))
    {
      return 0;
    }

    return (uint32_t) size;
  }

  if (RCS_RECORD_TYPE_IS_CHARSET (record_type))
  {
    lit_utf8_size_t length = rcs_record_get_length (lit);
    if (!jrt_write_to_buffer_by_offset (buffer_p,
                                        buffer_size,
                                        in_out_buffer_offset_p,
                                        &length,
                                        sizeof (length)))
    {
      return 0;
    }

    rcs_iterator_t it_ctx = rcs_iterator_create (&rcs_lit_storage, lit);
    rcs_iterator_skip (&it_ctx, RCS_CHARSET_HEADER_SIZE);

    lit_utf8_size_t i;
    for (i = 0; i < length; ++i)
    {
      lit_utf8_byte_t next_byte;
      rcs_iterator_read (&it_ctx, &next_byte, sizeof (lit_utf8_byte_t));

      if (!jrt_write_to_buffer_by_offset (buffer_p,
                                          buffer_size,
                                          in_out_buffer_offset_p,
                                          &next_byte,
                                          sizeof (next_byte)))
      {
        return 0;
      }

      rcs_iterator_skip (&it_ctx, sizeof (lit_utf8_byte_t));
    }

    return (uint32_t) (sizeof (uint32_t) + length * sizeof (uint8_t));
  }

  if (RCS_RECORD_TYPE_IS_MAGIC_STR (record_type))
  {
    lit_magic_string_id_t id = rcs_record_get_magic_str_id (lit);

    if (!jrt_write_to_buffer_by_offset (buffer_p,
                                        buffer_size,
                                        in_out_buffer_offset_p,
                                        &id,
                                        sizeof (id)))
    {
      return 0;
    }

    return (uint32_t) sizeof (lit_magic_string_id_t);
  }

  if (RCS_RECORD_TYPE_IS_MAGIC_STR_EX (record_type))
  {
    lit_magic_string_ex_id_t id = rcs_record_get_magic_str_ex_id (lit);

    if (!jrt_write_to_buffer_by_offset (buffer_p,
                                        buffer_size,
                                        in_out_buffer_offset_p,
                                        &id,
                                        sizeof (id)))
    {
      return 0;
    }

    return (uint32_t) sizeof (lit_magic_string_ex_id_t);
  }

  JERRY_UNREACHABLE ();
  return 0;
} /* rcs_snapshot_dump */

/**
 * Dump literals to specified snapshot buffer.
 *
 * @return true, if dump was performed successfully (i.e. buffer size is sufficient),
 *         false - otherwise.
 */
bool
lit_dump_literals_for_snapshot (uint8_t *buffer_p, /**< output snapshot buffer */
                                size_t buffer_size, /**< size of the buffer */
                                size_t *in_out_buffer_offset_p, /**< in-out: write position in the buffer */
                                lit_mem_to_snapshot_id_map_entry_t **out_map_p, /**< out: map from literal identifiers
                                                                                 *        to the literal offsets
                                                                                 *        in snapshot */
                                uint32_t *out_map_num_p, /**< out: number of literals */
                                uint32_t *out_lit_table_size_p) /**< out: number of bytes, dumped to snapshot buffer */
{
  uint32_t literals_num = lit_count_literals (&rcs_lit_storage);
  uint32_t lit_table_size = 0;

  *out_map_p = NULL;
  *out_map_num_p = 0;
  *out_lit_table_size_p = 0;

  if (!jrt_write_to_buffer_by_offset (buffer_p,
                                      buffer_size,
                                      in_out_buffer_offset_p,
                                      &literals_num,
                                      sizeof (literals_num)))
  {
    return false;
  }

  lit_table_size += (uint32_t) sizeof (literals_num);

  if (literals_num != 0)
  {
    bool is_ok = true;

    size_t id_map_size = sizeof (lit_mem_to_snapshot_id_map_entry_t) * literals_num;
    lit_mem_to_snapshot_id_map_entry_t *id_map_p;
    id_map_p = (lit_mem_to_snapshot_id_map_entry_t *) mem_heap_alloc_block (id_map_size, MEM_HEAP_ALLOC_SHORT_TERM);

    uint32_t literal_index = 0;
    lit_literal_t lit;

    for (lit = rcs_record_get_first (&rcs_lit_storage);
         lit != NULL;
         lit = rcs_record_get_next (&rcs_lit_storage, lit))
    {
      rcs_record_type_t record_type = rcs_record_get_type (lit);

      if (RCS_RECORD_TYPE_IS_FREE (record_type))
      {
        continue;
      }

      if (!jrt_write_to_buffer_by_offset (buffer_p,
                                          buffer_size,
                                          in_out_buffer_offset_p,
                                          &record_type,
                                          sizeof (record_type)))
      {
        is_ok = false;
        break;
      }

      uint32_t bytes = lit_snapshot_dump (lit, buffer_p, buffer_size, in_out_buffer_offset_p);

      if (bytes == 0)
      {
        /* write failed */
        is_ok = false;
        break;
      }

      rcs_cpointer_t lit_cp = rcs_cpointer_compress (lit);
      id_map_p[literal_index].literal_id = lit_cp;
      id_map_p[literal_index].literal_offset = lit_table_size;

      lit_table_size += (uint32_t) sizeof (record_type);
      lit_table_size += bytes;

      literal_index++;
    }

    if (!is_ok)
    {
      mem_heap_free_block (id_map_p);
      return false;
    }

    JERRY_ASSERT (literal_index == literals_num);
    *out_map_p = id_map_p;
  }

  uint32_t aligned_size = JERRY_ALIGNUP (lit_table_size, MEM_ALIGNMENT);

  if (aligned_size != lit_table_size)
  {
    JERRY_ASSERT (aligned_size > lit_table_size);

    uint8_t padding = 0;
    uint32_t padding_bytes_num = (uint32_t) (aligned_size - lit_table_size);
    uint32_t i;

    for (i = 0; i < padding_bytes_num; i++)
    {
      if (!jrt_write_to_buffer_by_offset (buffer_p,
                                          buffer_size,
                                          in_out_buffer_offset_p,
                                          &padding,
                                          sizeof (uint8_t)))
      {
        return false;
      }
    }
  }

  *out_map_num_p = literals_num;
  *out_lit_table_size_p = aligned_size;
  return true;
} /* lit_dump_literals_for_snapshot */

/**
 * Load literals from snapshot.
 *
 * @return true, if load was performed successfully (i.e. literals dump in the snapshot is consistent),
 *         false - otherwise (i.e. snapshot is incorrect).
 */
bool
lit_load_literals_from_snapshot (const uint8_t *lit_table_p, /**< buffer with literal table in snapshot */
                                 uint32_t lit_table_size, /**< size of literal table in snapshot */
                                 lit_mem_to_snapshot_id_map_entry_t **out_map_p, /**< out: map from literal offsets
                                                                                  *   in snapshot to identifiers
                                                                                  *   of loaded literals in literal
                                                                                  *   storage */
                                 uint32_t *out_map_num_p) /**< out: literals number */
{
  *out_map_p = NULL;
  *out_map_num_p = 0;

  size_t lit_table_read = 0;

  uint32_t literals_num;
  if (!jrt_read_from_buffer_by_offset (lit_table_p,
                                       lit_table_size,
                                       &lit_table_read,
                                       &literals_num,
                                       sizeof (literals_num)))
  {
    return false;
  }

  if (literals_num == 0)
  {
    return true;
  }

  size_t id_map_size = sizeof (lit_mem_to_snapshot_id_map_entry_t) * literals_num;
  lit_mem_to_snapshot_id_map_entry_t *id_map_p;
  id_map_p = (lit_mem_to_snapshot_id_map_entry_t *) mem_heap_alloc_block (id_map_size, MEM_HEAP_ALLOC_SHORT_TERM);

  bool is_ok = true;
  uint32_t lit_index;

  for (lit_index = 0; lit_index < literals_num; ++lit_index)
  {
    uint32_t offset = (uint32_t) lit_table_read;
    JERRY_ASSERT (offset == lit_table_read);

    rcs_record_type_t type;
    if (!jrt_read_from_buffer_by_offset (lit_table_p,
                                         lit_table_size,
                                         &lit_table_read,
                                         &type,
                                         sizeof (type)))
    {
      is_ok = false;
      break;
    }

    lit_literal_t lit;

    if (RCS_RECORD_TYPE_IS_CHARSET (type))
    {
      lit_utf8_size_t length;
      if (!jrt_read_from_buffer_by_offset (lit_table_p,
                                           lit_table_size,
                                           &lit_table_read,
                                           &length,
                                           sizeof (length))
          || (lit_table_read + length > lit_table_size))
      {
        is_ok = false;
        break;
      }

      lit = (lit_literal_t) lit_find_or_create_literal_from_utf8_string (lit_table_p + lit_table_read, length);
      lit_table_read += length;
    }
    else if (RCS_RECORD_TYPE_IS_MAGIC_STR (type))
    {
      lit_magic_string_id_t id;
      if (!jrt_read_from_buffer_by_offset (lit_table_p,
                                           lit_table_size,
                                           &lit_table_read,
                                           &id,
                                           sizeof (id)))
      {
        is_ok = false;
        break;
      }

      const lit_utf8_byte_t *magic_str_p = lit_get_magic_string_utf8 (id);
      lit_utf8_size_t magic_str_sz = lit_get_magic_string_size (id);

      /*
       * TODO:
       *      Consider searching literal storage by magic string identifier instead of by its value
       */
      lit = (lit_literal_t) lit_find_or_create_literal_from_utf8_string (magic_str_p, magic_str_sz);
    }
    else if (RCS_RECORD_TYPE_IS_MAGIC_STR_EX (type))
    {
      lit_magic_string_ex_id_t id;
      if (!jrt_read_from_buffer_by_offset (lit_table_p,
                                           lit_table_size,
                                           &lit_table_read,
                                           &id,
                                           sizeof (id)))
      {
        is_ok = false;
        break;
      }

      const lit_utf8_byte_t *magic_str_ex_p = lit_get_magic_string_ex_utf8 (id);
      lit_utf8_size_t magic_str_ex_sz = lit_get_magic_string_ex_size (id);

      /*
       * TODO:
       *      Consider searching literal storage by magic string identifier instead of by its value
       */
      lit = (lit_literal_t) lit_find_or_create_literal_from_utf8_string (magic_str_ex_p, magic_str_ex_sz);
    }
    else if (RCS_RECORD_TYPE_IS_NUMBER (type))
    {
      double num;
      if (!jrt_read_from_buffer_by_offset (lit_table_p,
                                           lit_table_size,
                                           &lit_table_read,
                                           &num,
                                           sizeof (num)))
      {
        is_ok = false;
        break;
      }

      lit = (lit_literal_t) lit_find_or_create_literal_from_num ((ecma_number_t) num);
    }
    else
    {
      is_ok = false;
      break;
    }

    id_map_p[lit_index].literal_offset = offset;
    id_map_p[lit_index].literal_id = rcs_cpointer_compress (lit);
  }

  if (is_ok)
  {
    *out_map_p = id_map_p;
    *out_map_num_p = literals_num;

    return true;
  }

  mem_heap_free_block (id_map_p);
  return false;
} /* rcs_load_literals_from_snapshot */

#endif /* JERRY_ENABLE_SNAPSHOT */
