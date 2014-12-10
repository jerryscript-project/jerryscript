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

#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "globals.h"
#include "ecma-globals.h"
#include "opcodes.h"
#include "interpreter.h"
#include "literal.h"
#include "scopes-tree.h"

void serializer_init (bool show_opcodes);
void serializer_dump_literals (const literal *, literal_index_t);
void serializer_set_scope (scopes_tree);
void serializer_merge_scopes_into_bytecode (void);
void serializer_dump_op_meta (op_meta);
opcode_counter_t serializer_get_current_opcode_counter (void);
opcode_counter_t serializer_count_opcodes_in_subscopes (void);
void serializer_set_writing_position (opcode_counter_t);
void serializer_rewrite_op_meta (opcode_counter_t, op_meta);
void serializer_print_opcodes (void);
void serializer_free (void);

#endif // SERIALIZER_H
