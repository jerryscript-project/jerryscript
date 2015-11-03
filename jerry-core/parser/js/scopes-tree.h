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
#include "opcodes.h"
#include "lit-id-hash-table.h"
#include "lit-literal.h"

/**
 * Intermediate instruction descriptor that additionally to byte-code instruction
 * holds information about literals, associated with corresponding arguments
 * of the instruction.
 */
typedef struct
{
  lit_cpointer_t lit_id[3]; /**< literals, corresponding to arguments
                             *   (NOT_A_LITERAL - if not literal is associated
                             *   with corresponding argument) */
  vm_instr_t op; /**< byte-code instruction */
} op_meta;

typedef struct tree_header
{
  linked_list children;
} tree_header;

/**
 * Scope type
 */
typedef enum __attr_packed___
{
  SCOPE_TYPE_GLOBAL, /**< the Global code scope */
  SCOPE_TYPE_FUNCTION, /**< a function code scope */
  SCOPE_TYPE_EVAL /**< an eval code scope */
} scope_type_t;

/**
 * Structure for holding scope information during parsing
 */
typedef struct
{
  mem_cpointer_t next_scope_cp; /**< next scope with same parent */

  mem_cpointer_t bc_header_cp; /**< pointer to corresponding byte-code header
                                *   (after bc_dump_single_scope) */
  uint16_t child_scopes_num; /**< number of child scopes */
  uint16_t child_scopes_processed_num; /**< number of child scopes, for which
                                        *   byte-code headers were already constructed */

  uint16_t max_uniq_literals_num; /**< upper estimate number of entries
                                   *   in idx-literal hash table */

  vm_instr_counter_t instrs_count; /**< count of instructions */

  linked_list var_decls; /**< instructions for variable declarations */

  scope_type_t type : 2; /**< scope type */
  bool strict_mode: 1; /**< flag, indicating that scope's code should be executed in strict mode */
  bool ref_arguments: 1; /**< flag, indicating that "arguments" variable is used inside the scope
                          *   (not depends on subscopes) */
  bool ref_eval: 1; /**< flag, indicating that "eval" is used inside the scope
                     *   (not depends on subscopes) */
  bool contains_with: 1; /**< flag, indicationg whether 'with' statement is contained in the scope
                          *   (not depends on subscopes) */
  bool contains_try: 1; /**< flag, indicationg whether 'try' statement is contained in the scope
                         *   (not depends on subscopes) */
  bool contains_delete: 1; /**< flag, indicationg whether 'delete' operator is contained in the scope
                            *   (not depends on subscopes) */
  bool contains_functions: 1; /**< flag, indicating that the scope contains a function declaration / expression */
  bool is_vars_and_args_to_regs_possible : 1; /**< the function's variables / arguments can be moved to registers */
} scopes_tree_int;

typedef scopes_tree_int *scopes_tree;

void scopes_tree_init (void);
void scopes_tree_finalize (void);
scopes_tree scopes_tree_new_scope (scopes_tree, scope_type_t);
void scopes_tree_free_scope (scopes_tree);
void scopes_tree_finish_build (void);
size_t scopes_tree_child_scopes_num (scopes_tree);
vm_instr_counter_t scopes_tree_instrs_num (scopes_tree);
vm_instr_counter_t scopes_tree_var_decls_num (scopes_tree);
void scopes_tree_add_var_decl (scopes_tree, op_meta);
op_meta scopes_tree_var_decl (scopes_tree, vm_instr_counter_t);
bool scopes_tree_variable_declaration_exists (scopes_tree, lit_cpointer_t);
void scopes_tree_dump_var_decls (scopes_tree, lit_cpointer_t *);
void scopes_tree_set_strict_mode (scopes_tree, bool);
void scopes_tree_set_arguments_used (scopes_tree);
void scopes_tree_set_eval_used (scopes_tree);
void scopes_tree_set_contains_with (scopes_tree);
void scopes_tree_set_contains_try (scopes_tree);
void scopes_tree_set_contains_delete (scopes_tree);
void scopes_tree_set_contains_functions (scopes_tree);
bool scopes_tree_strict_mode (scopes_tree);
#endif /* SCOPES_TREE_H */
