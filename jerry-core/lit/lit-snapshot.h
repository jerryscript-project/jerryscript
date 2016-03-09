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

#ifndef LIT_SNAPSHOT_H
#define LIT_SNAPSHOT_H

#include "lit-cpointer.h"
#include "ecma-globals.h"

typedef struct
{
  lit_cpointer_t literal_id;
  uint32_t literal_offset;
} lit_mem_to_snapshot_id_map_entry_t;

#ifdef JERRY_ENABLE_SNAPSHOT_SAVE
extern bool
lit_save_literals_for_snapshot (uint8_t *,
                                size_t,
                                size_t *,
                                lit_mem_to_snapshot_id_map_entry_t **,
                                uint32_t *,
                                uint32_t *);
#endif /* JERRY_ENABLE_SNAPSHOT_SAVE */

#ifdef JERRY_ENABLE_SNAPSHOT_EXEC
extern bool
lit_load_literals_from_snapshot (const uint8_t *,
                                 uint32_t,
                                 lit_mem_to_snapshot_id_map_entry_t **,
                                 uint32_t *);
#endif /* JERRY_ENABLE_SNAPSHOT_EXEC */

#endif /* !LIT_SNAPSHOT_H */
