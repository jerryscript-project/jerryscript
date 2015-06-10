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

lit_id_hash_table *
lit_id_hash_table_init (size_t buckets_count, size_t blocks_count)
{
  size_t size = mem_heap_recommend_allocation_size (sizeof (lit_id_hash_table));
  lit_id_hash_table *table = (lit_id_hash_table *) mem_heap_alloc_block (size, MEM_HEAP_ALLOC_LONG_TERM);
  memset (table, 0, size);
  size = mem_heap_recommend_allocation_size (sizeof (lit_cpointer_t) * buckets_count);
  table->raw_buckets = (lit_cpointer_t *) mem_heap_alloc_block (size, MEM_HEAP_ALLOC_LONG_TERM);
  memset (table->raw_buckets, 0, size);
  size = mem_heap_recommend_allocation_size (sizeof (lit_cpointer_t *) * blocks_count);
  table->buckets = (lit_cpointer_t **) mem_heap_alloc_block (size, MEM_HEAP_ALLOC_LONG_TERM);
  memset (table->buckets, 0, size);
  table->current_bucket_pos = 0;
  return table;
}

void
lit_id_hash_table_free (lit_id_hash_table *table)
{
  JERRY_ASSERT (table);
  mem_heap_free_block ((uint8_t *) table->raw_buckets);
  mem_heap_free_block ((uint8_t *) table->buckets);
  mem_heap_free_block ((uint8_t *) table);
}

void
lit_id_hash_table_insert (lit_id_hash_table *table, idx_t uid, opcode_counter_t oc, lit_cpointer_t lit_cp)
{
  JERRY_ASSERT (table);
  size_t block_id = oc / BLOCK_SIZE;
  if (table->buckets[block_id] == NULL)
  {
    table->buckets[block_id] = table->raw_buckets + table->current_bucket_pos;
  }
  table->buckets[block_id][uid] = lit_cp;
  table->current_bucket_pos++;
}

lit_cpointer_t
lit_id_hash_table_lookup (lit_id_hash_table *table, idx_t uid, opcode_counter_t oc)
{
  JERRY_ASSERT (table);
  size_t block_id = oc / BLOCK_SIZE;
  JERRY_ASSERT (table->buckets[block_id]);
  return table->buckets[block_id][uid];
}
