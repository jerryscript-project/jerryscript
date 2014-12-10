/* Copyright 2014 Samsung Electronics Co., Ltd.
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

#ifndef BYTECODE_DATA_H
#define BYTECODE_DATA_H

#include "opcodes.h"
#include "stack.h"
#include "jerry-libc.h"
#include "literal.h"
#include "scopes-tree.h"

#define BLOCK_SIZE 64

typedef struct
{
  opcode_counter_t oc;
  idx_t uid;
  uint8_t reserved;
}
lit_id_table_key;

struct
{
  const literal *literals;
  const opcode_t *opcodes;
  literal_index_t literals_count;
  opcode_counter_t opcodes_count;
  hash_table lit_id_hash;
}
bytecode_data;

lit_id_table_key create_lit_id_table_key (idx_t, opcode_counter_t);

scopes_tree current_scope;

#endif // BYTECODE_DATA_H
