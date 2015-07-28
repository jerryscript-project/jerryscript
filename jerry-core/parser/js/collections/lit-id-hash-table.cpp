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
 * Register pair in the hash table
 */
void
lit_id_hash_table_insert (lit_id_hash_table *table_p, /**< table's header */
                          vm_idx_t uid, /**< value of byte-code instruction's argument */
                          vm_instr_counter_t oc, /**< instruction counter of the instruction */
                          lit_cpointer_t lit_cp) /**< literal identifier */
{
  JERRY_ASSERT (table_p != NULL);

  size_t block_id = oc / BLOCK_SIZE;

  if (table_p->buckets[block_id] == NULL)
  {
    table_p->buckets[block_id] = table_p->raw_buckets + table_p->current_bucket_pos;
  }

  table_p->buckets[block_id][uid] = lit_cp;
  table_p->current_bucket_pos++;
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
 * @}
 * @}
 * @}
 */
