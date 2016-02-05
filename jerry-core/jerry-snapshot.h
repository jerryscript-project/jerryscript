/* Copyright 2016 University of Szeged.
 * Copyright 2016 Samsung Electronics Co., Ltd.
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

#ifndef JERRY_SNAPSHOT_H
#define JERRY_SNAPSHOT_H

#include "ecma-globals.h"

/**
 * Snapshot header
 */
typedef struct
{
  uint32_t last_compiled_code_offset; /**< offset of the last compiled code */
  uint32_t lit_table_size; /**< size of literal table */
  uint32_t is_run_global : 1; /**< flag, indicating whether the snapshot
                               *   was dumped as 'Global scope'-mode code (true)
                               *   or as eval-mode code (false) */
} jerry_snapshot_header_t;

/**
 * Jerry snapshot format version
 */
#define JERRY_SNAPSHOT_VERSION (3u)

#ifdef JERRY_ENABLE_SNAPSHOT_SAVE

/* Snapshot support functions */

extern bool snapshot_report_byte_code_compilation;

extern void
snapshot_add_compiled_code (ecma_compiled_code_t *, const uint8_t *, uint32_t);

#endif /* JERRY_ENABLE_SNAPSHOT_SAVE */

#endif /* !JERRY_SNAPSHOT_H */
