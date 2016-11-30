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

#ifndef JERRY_SNAPSHOT_H
#define JERRY_SNAPSHOT_H

#include "ecma-globals.h"

/**
 * Snapshot header
 */
typedef struct
{
  /* The size of this structure is recommended to be divisible by
   * JMEM_ALIGNMENT. Otherwise some bytes after the header are wasted. */
  uint32_t version; /**< version number */
  uint32_t lit_table_offset; /**< offset of the literal table */
  uint32_t lit_table_size; /**< size of literal table */
  uint32_t is_run_global; /**< flag, indicating whether the snapshot
                            *   was saved as 'Global scope'-mode code (true)
                            *   or as eval-mode code (false) */
} jerry_snapshot_header_t;

/**
 * Jerry snapshot format version
 */
#define JERRY_SNAPSHOT_VERSION (6u)

#endif /* !JERRY_SNAPSHOT_H */
