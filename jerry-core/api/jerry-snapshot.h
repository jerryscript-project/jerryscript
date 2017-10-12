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
  uint32_t global_flags; /**< global configuration and feature flags */
  uint32_t lit_table_offset; /**< byte offset of the literal table */
  uint32_t number_of_funcs; /**< number of primary ECMAScript functions */
  uint32_t func_offsets[1]; /**< function offsets (lowest bit: global(0) or eval(1) context) */
} jerry_snapshot_header_t;

/**
 * Evaluate this function on the top of the scope chain.
 */
#define JERRY_SNAPSHOT_EVAL_CONTEXT 0x1u

/**
 * Jerry snapshot magic marker.
 */
#define JERRY_SNAPSHOT_MAGIC (0x5952524Au)

/**
 * Jerry snapshot format version.
 */
#define JERRY_SNAPSHOT_VERSION (8u)

/**
 * Snapshot configuration flags.
 */
typedef enum
{
  /* 8 bits are reserved for dynamic features */
  JERRY_SNAPSHOT_HAS_REGEX_LITERAL = (1u << 0), /**< byte code has regex literal */
  /* 24 bits are reserved for compile time features */
  JERRY_SNAPSHOT_FOUR_BYTE_CPOINTER = (1u << 8) /**< compressed pointers are four byte long */
} jerry_snapshot_global_flags_t;

#endif /* !JERRY_SNAPSHOT_H */
