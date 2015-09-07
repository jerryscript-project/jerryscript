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

#ifndef LIT_ID_HASH_TABLE
#define LIT_ID_HASH_TABLE

#include "jrt.h"
#include "ecma-globals.h"
#include "opcodes.h"
#include "lit-literal.h"

typedef struct
{
  size_t current_bucket_pos;
  lit_cpointer_t *raw_buckets;
  lit_cpointer_t **buckets;
} lit_id_hash_table;

lit_id_hash_table *lit_id_hash_table_init (uint8_t*, size_t, size_t, size_t);
size_t lit_id_hash_table_get_size_for_table (size_t, size_t);
void lit_id_hash_table_free (lit_id_hash_table *);
void lit_id_hash_table_insert (lit_id_hash_table *, vm_idx_t, vm_instr_counter_t, lit_cpointer_t);
lit_cpointer_t lit_id_hash_table_lookup (lit_id_hash_table *, vm_idx_t, vm_instr_counter_t);
uint32_t lit_id_hash_table_dump_for_snapshot (uint8_t *buffer_p,
                                              size_t buffer_size,
                                              size_t *in_out_buffer_offset_p,
                                              lit_id_hash_table *table_p,
                                              const lit_mem_to_snapshot_id_map_entry_t *lit_map_p,
                                              uint32_t literals_num,
                                              vm_instr_counter_t instrs_num);
bool lit_id_hash_table_load_from_snapshot (size_t blocks_count,
                                           uint32_t idx_num_total,
                                           const uint8_t *idx_to_lit_map_p,
                                           size_t idx_to_lit_map_size,
                                           const lit_mem_to_snapshot_id_map_entry_t *lit_map_p,
                                           uint32_t literals_num,
                                           uint8_t *buffer_for_hash_table_p,
                                           size_t buffer_for_hash_table_size);
#endif /* LIT_ID_HASH_TABLE */
