/* Copyright JS Foundation and other contributors, http://js.foundation
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

#ifndef ECMA_LIT_STORAGE_H
#define ECMA_LIT_STORAGE_H

#include "ecma-globals.h"
#include "jmem.h"
#include "lit-globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmalitstorage Literal storage
 * @{
 */

#if ENABLED (JERRY_SNAPSHOT_SAVE)
/**
 * Snapshot literal - offset map
 */
typedef struct
{
  ecma_value_t literal_id; /**< literal id */
  ecma_value_t literal_offset; /**< literal offset */
} lit_mem_to_snapshot_id_map_entry_t;
#endif /* ENABLED (JERRY_SNAPSHOT_SAVE) */

void ecma_finalize_lit_storage (void);

ecma_value_t ecma_find_or_create_literal_string (const lit_utf8_byte_t *chars_p, lit_utf8_size_t size);
ecma_value_t ecma_find_or_create_literal_number (ecma_number_t number_arg);

#if ENABLED (JERRY_SNAPSHOT_SAVE)
void ecma_save_literals_append_value (ecma_value_t value, ecma_collection_t *lit_pool_p);
void ecma_save_literals_add_compiled_code (const ecma_compiled_code_t *compiled_code_p,
                                           ecma_collection_t *lit_pool_p);
bool ecma_save_literals_for_snapshot (ecma_collection_t *lit_pool_p, uint32_t *buffer_p, size_t buffer_size,
                                      size_t *in_out_buffer_offset_p, lit_mem_to_snapshot_id_map_entry_t **out_map_p,
                                      uint32_t *out_map_len_p);
#endif /* ENABLED (JERRY_SNAPSHOT_SAVE) */

#if ENABLED (JERRY_SNAPSHOT_EXEC) || ENABLED (JERRY_SNAPSHOT_SAVE)
ecma_value_t
ecma_snapshot_get_literal (const uint8_t *literal_base_p, ecma_value_t literal_value);
#endif /* ENABLED (JERRY_SNAPSHOT_EXEC) || ENABLED (JERRY_SNAPSHOT_SAVE) */

/**
 * @}
 * @}
 */

#endif /* !ECMA_LIT_STORAGE_H */
