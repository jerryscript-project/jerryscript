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
   * uint32_t alignment. Otherwise some bytes after the header are wasted. */
  uint32_t magic; /**< four byte magic number */
  uint32_t version; /**< version number */
  uint32_t feature_flags; /**< combination of jerry_snapshot_global_flags_t */
  uint32_t lit_table_offset; /**< byte offset of the literal table */
  uint32_t number_of_funcs; /**< number of primary ECMAScript functions */
  uint32_t func_offsets[1]; /**< function offsets (lowest bit: global(0) or eval(1) context) */
} jerry_snapshot_header_t;

/**
 * Jerry snapshot magic marker.
 */
#define JERRY_SNAPSHOT_MAGIC (0x5952524Au)

/**
 * Feature flags for snapshot execution.
 */
typedef enum
{
  JERRY_SNAPSHOT_FEATURE_NONE = 0,
  JERRY_SNAPSHOT_FEATURE_REGEXP = (1u << 0), /**< feature flag for JERRY_BUILTIN_REGEXP */
  JERRY_SNAPSHOT_FEATURE_MODULE = (1u << 1), /**< feature flag for JERRY_MODULE_SYSTEM */
  JERRY_SNAPSHOT_FEATURE_DEBUGGER = (1u << 2), /**< feature flag for JERRY_DEBUGGER */
  JERRY_SNAPSHOT_FEATURE_ESNEXT = (1u << 3), /**< feature flag for JERRY_ESNEXT */
} jerry_snapshot_feature_flags_t;

#endif /* !JERRY_SNAPSHOT_H */
