/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged.
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

#ifndef RE_COMPILER_H
#define RE_COMPILER_H

#include "ecma-globals.h"

#define RE_OP_EOF               0
#define RE_OP_MATCH             1
#define RE_OP_CHAR              2
#define RE_OP_SAVE_AT_START     3
#define RE_OP_SAVE_AND_MATCH    4

typedef uint8_t regexp_opcode_t;
typedef uint8_t regexp_bytecode_t;

typedef struct bytecode_ctx_t
{
  regexp_bytecode_t *block_start_p;
  regexp_bytecode_t *block_end_p;
  regexp_bytecode_t *current_p;
} bytecode_ctx_t;

void
regexp_compile_bytecode (ecma_property_t *bytecode, ecma_string_t *pattern);

regexp_opcode_t
get_opcode (regexp_bytecode_t **bc_p);

uint32_t
get_value (regexp_bytecode_t **bc_p);

#endif /* RE_COMPILER_H */
