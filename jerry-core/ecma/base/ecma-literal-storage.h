/* Copyright 2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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
#include "jmem-allocator.h"
#include "lit-globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmalitstorage Literal storage
 * @{
 */

/**
 * Snapshot literal - offset map
 */
typedef struct
{
  jmem_cpointer_t literal_id; /**< literal id */
  jmem_cpointer_t literal_offset; /**< literal offset */
} lit_mem_to_snapshot_id_map_entry_t;

extern void ecma_finalize_lit_storage (void);

extern jmem_cpointer_t ecma_find_or_create_literal_string (const lit_utf8_byte_t *, lit_utf8_size_t);
extern jmem_cpointer_t ecma_find_or_create_literal_number (ecma_number_t);

#ifdef JERRY_ENABLE_SNAPSHOT_SAVE
extern bool
ecma_save_literals_for_snapshot (uint8_t *, size_t, size_t *,
                                 lit_mem_to_snapshot_id_map_entry_t **, uint32_t *, uint32_t *);
#endif /* JERRY_ENABLE_SNAPSHOT_SAVE */

#ifdef JERRY_ENABLE_SNAPSHOT_EXEC
extern bool
ecma_load_literals_from_snapshot (const uint8_t *, uint32_t,
                                  lit_mem_to_snapshot_id_map_entry_t **, uint32_t *);
#endif /* JERRY_ENABLE_SNAPSHOT_EXEC */

/**
 * @}
 * @}
 */

#endif /* !ECMA_LIT_STORAGE_H */
