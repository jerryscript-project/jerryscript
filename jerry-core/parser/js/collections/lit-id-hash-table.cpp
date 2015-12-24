/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#include "lit-id-hash-table.h"
#include "bytecode-data.h"

/** \addtogroup jsparser ECMAScript parser
 * @{
 *
 * \addtogroup collections Collections
 * @{
 *
 * \addtogroup lit_id_hash_table Literal identifiers hash table
 * The hash table connects pairs (instruction block, vm_idx_t value) with literal identifiers.
 * @{
 */

/**
 * Initialize literal identifiers hash table
 *
 * @return pointer to header of the table
 */
lit_id_hash_table *
lit_id_hash_table_init (uint8_t *table_buffer_p, /**< buffer to initialize hash table in */
                        size_t buffer_size, /**< size of the buffer */
                        size_t buckets_count, /**< number of pairs */
                        size_t blocks_count) /**< number of instruction blocks */
{
  const size_t header_size = JERRY_ALIGNUP (sizeof (lit_id_hash_table), MEM_ALIGNMENT);
  const size_t raw_buckets_size = JERRY_ALIGNUP (sizeof (lit_cpointer_t) * buckets_count, MEM_ALIGNMENT);
  const size_t buckets_size = JERRY_ALIGNUP (sizeof (lit_cpointer_t*) * blocks_count, MEM_ALIGNMENT);

  JERRY_ASSERT (header_size + raw_buckets_size + buckets_size <= buffer_size);

  lit_id_hash_table *table_p = (lit_id_hash_table *) table_buffer_p;

  table_p->current_bucket_pos = 0;
  table_p->raw_buckets = (lit_cpointer_t*) (table_buffer_p + header_size);
  table_p->buckets = (lit_cpointer_t **) (table_buffer_p + header_size + raw_buckets_size);

  memset (table_p->buckets, 0, buckets_size);

  return table_p;
} /* lit_id_hash_table_init */

/**
 * Get size of buffer, necessary to hold hash table with specified parameters
 *
 * @return size of buffer
 */
size_t
lit_id_hash_table_get_size_for_table (size_t buckets_count, /**< number of pairs */
                                      size_t blocks_count) /**< number of instructions blocks */
{
  const size_t header_size = JERRY_ALIGNUP (sizeof (lit_id_hash_table), MEM_ALIGNMENT);
  const size_t raw_buckets_size = JERRY_ALIGNUP (sizeof (lit_cpointer_t) * buckets_count, MEM_ALIGNMENT);
  const size_t buckets_size = JERRY_ALIGNUP (sizeof (lit_cpointer_t*) * blocks_count, MEM_ALIGNMENT);

  return header_size + raw_buckets_size + buckets_size;
} /* lit_id_hash_table_get_size_for_table */

/**
 * Free literal identifiers hash table
 */
void
lit_id_hash_table_free (lit_id_hash_table *table_p) /**< table's header */
{
  JERRY_ASSERT (table_p != NULL);

  mem_heap_free_block ((uint8_t *) table_p);
} /* lit_id_hash_table_free */

/**
 * Register literal in the hash table
 *
 * @return corresponding idx
 */
vm_idx_t
lit_id_hash_table_insert (lit_id_hash_table *table_p, /**< table's header */
                          vm_instr_counter_t oc, /**< instruction counter of the instruction */
                          lit_cpointer_t lit_cp) /**< literal identifier */
{
  JERRY_ASSERT (table_p != NULL);

  size_t block_id = oc / BLOCK_SIZE;

  if (table_p->buckets[block_id] == NULL)
  {
    table_p->buckets[block_id] = table_p->raw_buckets + table_p->current_bucket_pos;
  }

  lit_cpointer_t *raw_bucket_iter_p = table_p->raw_buckets + table_p->current_bucket_pos;

  JERRY_ASSERT (raw_bucket_iter_p >= table_p->buckets[block_id]);
  ssize_t bucket_size = (raw_bucket_iter_p - table_p->buckets[block_id]);

  int32_t index;
  for (index = 0; index < bucket_size; index++)
  {
    if (table_p->buckets[block_id][index].packed_value == lit_cp.packed_value)
    {
      break;
    }
  }

  if (index == bucket_size)
  {
    JERRY_ASSERT ((uint8_t *) (table_p->raw_buckets + table_p->current_bucket_pos) < (uint8_t *) (table_p->buckets));

    table_p->buckets[block_id][index] = lit_cp;
    table_p->current_bucket_pos++;
  }

  JERRY_ASSERT (index <= VM_IDX_LITERAL_LAST);

  return (vm_idx_t) index;
} /* lit_id_hash_table_insert */

/**
 * Lookup literal identifier by pair
 *
 * @return literal identifier
 */
lit_cpointer_t
lit_id_hash_table_lookup (lit_id_hash_table *table_p, /**< table's header */
                          vm_idx_t uid, /**< value of byte-code instruction's argument */
                          vm_instr_counter_t oc) /**< instruction counter of the instruction */
{
  JERRY_ASSERT (table_p != NULL);

  size_t block_id = oc / BLOCK_SIZE;
  JERRY_ASSERT (table_p->buckets[block_id] != NULL);

  return table_p->buckets[block_id][uid];
} /* lit_id_hash_table_lookup */

/**
 * Dump literal identifiers hash table to snapshot buffer
 *
 * @return number of bytes dumper - upon success,
 *         0 - upon failure
 */
uint32_t
lit_id_hash_table_dump_for_snapshot (uint8_t *buffer_p, /**< buffer to dump to */
                                     size_t buffer_size, /**< buffer size */
                                     size_t *in_out_buffer_offset_p, /**< in-out: buffer write offset */
                                     lit_id_hash_table *table_p, /**< hash table to dump */
                                     const lit_mem_to_snapshot_id_map_entry_t *lit_map_p, /**< map from literal
                                                                                           *   identifiers in
                                                                                           *   literal storage
                                                                                           *   to literal offsets
                                                                                           *   in snapshot */
                                     uint32_t literals_num, /**< number of literals */
                                     vm_instr_counter_t instrs_num) /**< number of instructions in corresponding
                                                                     *   byte-code array */
{
  size_t begin_offset = *in_out_buffer_offset_p;

  uint32_t idx_num_total = (uint32_t) table_p->current_bucket_pos;
  JERRY_ASSERT (idx_num_total == table_p->current_bucket_pos);

  if (!jrt_write_to_buffer_by_offset (buffer_p,
                                      buffer_size,
                                      in_out_buffer_offset_p,
                                      &idx_num_total,
                                      sizeof (idx_num_total)))
  {
    return 0;
  }

  size_t blocks_num = JERRY_ALIGNUP (instrs_num, BLOCK_SIZE) / BLOCK_SIZE;

  for (size_t block_index = 0, next_block_index;
       block_index < blocks_num;
      )
  {
    uint32_t idx_num_in_block;

    next_block_index = block_index + 1u;

    while (next_block_index < blocks_num
           && table_p->buckets[next_block_index] == NULL)
    {
      next_block_index++;
    }

    if (next_block_index != blocks_num)
    {
      idx_num_in_block = (uint32_t) (table_p->buckets[next_block_index] - table_p->buckets[block_index]);
    }
    else if (table_p->buckets[block_index] != NULL)
    {
      idx_num_in_block = (uint32_t) (table_p->current_bucket_pos
                                     - (size_t) (table_p->buckets[block_index] - table_p->buckets[0]));
    }
    else
    {
      idx_num_in_block = 0;
    }

    if (!jrt_write_to_buffer_by_offset (buffer_p,
                                        buffer_size,
                                        in_out_buffer_offset_p,
                                        &idx_num_in_block,
                                        sizeof (idx_num_in_block)))
    {
      return 0;
    }

    for (size_t block_idx_pair_index = 0;
         block_idx_pair_index < idx_num_in_block;
         block_idx_pair_index++)
    {
      lit_cpointer_t lit_cp = table_p->buckets[block_index][block_idx_pair_index];

      uint32_t offset = bc_find_lit_offset (lit_cp, lit_map_p, literals_num);

      if (!jrt_write_to_buffer_by_offset (buffer_p, buffer_size, in_out_buffer_offset_p, &offset, sizeof (offset)))
      {
        return 0;
      }
    }

    while (++block_index < next_block_index)
    {
      idx_num_in_block = 0;

      if (!jrt_write_to_buffer_by_offset (buffer_p,
                                          buffer_size,
                                          in_out_buffer_offset_p,
                                          &idx_num_in_block,
                                          sizeof (idx_num_in_block)))
      {
        return 0;
      }
    }
  }

  size_t bytes_written = (*in_out_buffer_offset_p - begin_offset);

  JERRY_ASSERT (bytes_written == (uint32_t) bytes_written);
  return (uint32_t) bytes_written;
} /* lit_id_hash_table_dump_for_snapshot */

/**
 * Load literal identifiers hash table from specified snapshot buffer
 *
 * @return true - upon successful load (i.e. data in snapshot is consistent),
 *         false - upon failure (in case, snapshot is incorrect)
 */
bool
lit_id_hash_table_load_from_snapshot (size_t blocks_count, /**< number of byte-code blocks
                                                            *   in corresponding byte-code array */
                                      uint32_t idx_num_total, /**< total number of (byte-code block, idx)
                                                               *   pairs in snapshot */
                                      const uint8_t *idx_to_lit_map_p, /**< idx-to-lit map in snapshot */
                                      size_t idx_to_lit_map_size, /**< size of the map */
                                      const lit_mem_to_snapshot_id_map_entry_t *lit_map_p, /**< map of in-snapshot
                                                                                            *   literal offsets
                                                                                            *   to literal identifiers,
                                                                                            *   created in literal
                                                                                            *   storage */
                                      uint32_t literals_num, /**< number of literals in snapshot */
                                      uint8_t *buffer_for_hash_table_p, /**< buffer to initialize hash table in */
                                      size_t buffer_for_hash_table_size) /**< size of the buffer */
{
  lit_id_hash_table *hash_table_p = lit_id_hash_table_init (buffer_for_hash_table_p,
                                                            buffer_for_hash_table_size,
                                                            idx_num_total,
                                                            blocks_count);

  size_t idx_to_lit_map_offset = 0;
  uint32_t idx_num_counter = 0;
  for (size_t block_idx = 0; block_idx < blocks_count; block_idx++)
  {
    uint32_t idx_num_in_block;
    if (!jrt_read_from_buffer_by_offset (idx_to_lit_map_p,
                                         idx_to_lit_map_size,
                                         &idx_to_lit_map_offset,
                                         &idx_num_in_block,
                                         sizeof (idx_num_in_block)))
    {
      return false;
    }

    hash_table_p->buckets[block_idx] = hash_table_p->raw_buckets + hash_table_p->current_bucket_pos;

    if (idx_num_counter + idx_num_in_block < idx_num_counter)
    {
      return false;
    }
    idx_num_counter += idx_num_in_block;

    if (idx_num_counter > idx_num_total)
    {
      return false;
    }

    for (uint32_t idx_in_block = 0; idx_in_block < idx_num_in_block; idx_in_block++)
    {
      uint32_t lit_offset_from_snapshot;
      if (!jrt_read_from_buffer_by_offset (idx_to_lit_map_p,
                                           idx_to_lit_map_size,
                                           &idx_to_lit_map_offset,
                                           &lit_offset_from_snapshot,
                                           sizeof (lit_offset_from_snapshot)))
      {
        return false;
      }

      /**
       * TODO: implement binary search here
       */
      lit_cpointer_t lit_cp = rcs_cpointer_null_cp ();
      uint32_t i;
      for (i = 0; i < literals_num; i++)
      {
        if (lit_map_p[i].literal_offset == lit_offset_from_snapshot)
        {
          lit_cp.packed_value = lit_map_p[i].literal_id.packed_value;
          break;
        }
      }

      if (i == literals_num)
      {
        return false;
      }

      JERRY_ASSERT (hash_table_p->current_bucket_pos < idx_num_total);
      hash_table_p->raw_buckets[hash_table_p->current_bucket_pos++] = lit_cp;
    }
  }

  return true;
} /* lit_id_hash_table_load_from_snapshot */


/**
 * @}
 * @}
 * @}
 */
