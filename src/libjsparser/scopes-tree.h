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

#ifndef SCOPES_TREE_H
#define SCOPES_TREE_H

#include "tree.h"
#include "linked-list.h"
#include "opcodes.h"

typedef struct
{
  linked_list opcodes;
  tree_header t;
  opcode_counter_t opcodes_num;
  unsigned strict_mode:1;
}
__packed
scopes_tree_int;

typedef scopes_tree_int * scopes_tree;

scopes_tree scopes_tree_init (scopes_tree);
void scopes_tree_free (scopes_tree);
opcode_counter_t scopes_tree_opcodes_num (scopes_tree);
void scopes_tree_add_opcode (scopes_tree, opcode_t);
void scopes_tree_set_opcode (scopes_tree, opcode_counter_t, opcode_t);
void scopes_tree_set_opcodes_num (scopes_tree, opcode_counter_t);
opcode_t scopes_tree_opcode (scopes_tree, opcode_counter_t);
opcode_counter_t scopes_tree_count_opcodes (scopes_tree);
opcode_t *scopes_tree_raw_data (scopes_tree, opcode_counter_t *);
void scopes_tree_set_strict_mode (scopes_tree, bool);
bool scopes_tree_strict_mode (scopes_tree);

#endif /* SCOPES_TREE_H */
