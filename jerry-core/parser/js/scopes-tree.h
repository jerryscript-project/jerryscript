/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#include "linked-list.h"
#include "lexer.h"
#include "ecma-globals.h"
#include "hash-table.h"
#include "opcodes.h"
#include "lit-id-hash-table.h"
#include "lit-literal.h"

#define NOT_A_LITERAL (lit_cpointer_t::null_cp ())

typedef struct
{
  lit_cpointer_t lit_id[3];
  opcode_t op;
} op_meta;

typedef struct tree_header
{
  struct tree_header *parent;
  linked_list children;
  uint8_t children_num;
} tree_header;

typedef struct
{
  tree_header t;
  linked_list opcodes;
  opcode_counter_t opcodes_num;
  unsigned strict_mode:1;
} scopes_tree_int;

typedef scopes_tree_int * scopes_tree;

scopes_tree scopes_tree_init (scopes_tree);
void scopes_tree_free (scopes_tree);
opcode_counter_t scopes_tree_opcodes_num (scopes_tree);
void scopes_tree_add_op_meta (scopes_tree, op_meta);
void scopes_tree_set_op_meta (scopes_tree, opcode_counter_t, op_meta);
void scopes_tree_set_opcodes_num (scopes_tree, opcode_counter_t);
op_meta scopes_tree_op_meta (scopes_tree, opcode_counter_t);
size_t scopes_tree_count_literals_in_blocks (scopes_tree);
opcode_counter_t scopes_tree_count_opcodes (scopes_tree);
opcode_t *scopes_tree_raw_data (scopes_tree, lit_id_hash_table *);
void scopes_tree_set_strict_mode (scopes_tree, bool);
bool scopes_tree_strict_mode (scopes_tree);

#endif /* SCOPES_TREE_H */
