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

#include "bytecode-data.h"
#include "jsp-mm.h"
#include "scopes-tree.h"

scopes_tree scopes_tree_root_node_p = NULL;
scopes_tree scopes_tree_last_node_p = NULL;

static void
assert_tree (scopes_tree t)
{
  JERRY_ASSERT (t != NULL);
}

vm_instr_counter_t
scopes_tree_instrs_num (scopes_tree t)
{
  assert_tree (t);
  return t->instrs_count;
}

/**
 * Get number of variable declarations in the scope
 *
 * @return number of variable declarations
 */
vm_instr_counter_t
scopes_tree_var_decls_num (scopes_tree t) /**< scope */
{
  assert_tree (t);
  return linked_list_get_length (t->var_decls);
} /* scopes_tree_var_decls_num */

/**
 * Add variable declaration to a scope
 */
void
scopes_tree_add_var_decl (scopes_tree tree, /**< scope, to which variable declaration is added */
                          op_meta op) /**< variable declaration instruction */
{
  assert_tree (tree);
  linked_list_set_element (tree->var_decls, linked_list_get_length (tree->var_decls), &op);
} /* scopes_tree_add_var_decl */

/**
 * Get variable declaration for the specified scope
 *
 * @return instruction, declaring a variable
 */
op_meta
scopes_tree_var_decl (scopes_tree tree, /**< scope, from which variable declaration is retrieved */
                      vm_instr_counter_t oc) /**< number of variable declaration in the scope */
{
  assert_tree (tree);
  JERRY_ASSERT (oc < linked_list_get_length (tree->var_decls));
  return *(op_meta *) linked_list_element (tree->var_decls, oc);
} /* scopes_tree_var_decl */

/**
 * Checks if variable declaration exists in the scope
 *
 * @return true / false
 */
bool
scopes_tree_variable_declaration_exists (scopes_tree tree, /**< scope */
                                         lit_cpointer_t lit_id) /**< literal which holds variable's name */
{
  assert_tree (tree);

  for (vm_instr_counter_t oc = 0u;
       oc < linked_list_get_length (tree->var_decls);
       oc++)
  {
    const op_meta* var_decl_om_p = (op_meta *) linked_list_element (tree->var_decls, oc);

    JERRY_ASSERT (var_decl_om_p->op.op_idx == VM_OP_VAR_DECL);
    JERRY_ASSERT (var_decl_om_p->op.data.var_decl.variable_name == VM_IDX_REWRITE_LITERAL_UID);

    if (var_decl_om_p->lit_id[0].packed_value == lit_id.packed_value)
    {
      return true;
    }
  }

  return false;
} /* scopes_tree_variable_declaration_exists */

/**
 * Fill variable declaration list of bytecode header
 */
void
scopes_tree_dump_var_decls (scopes_tree scope, /**< scopes tree */
                            lit_cpointer_t *var_decls_p) /**< pointer to bytecode header's declarations table,
                                                          *   where variables' lit_cp's should be stored */
{
  for (uint32_t i = 0; i < scopes_tree_var_decls_num (scope); ++i)
  {
    op_meta var_decl_op_meta = *(op_meta *) linked_list_element (scope->var_decls, i);
    var_decls_p[i] = var_decl_op_meta.lit_id[0];
  }
} /* scopes_tree_dump_var_decls */

/**
 * Set up a flag, indicating that scope should be executed in strict mode
 */
void
scopes_tree_set_strict_mode (scopes_tree tree, /**< scope */
                             bool strict_mode) /**< value of the strict mode flag */
{
  assert_tree (tree);
  tree->strict_mode = strict_mode;
} /* scopes_tree_set_strict_mode */

/**
 * Set up a flag, indicating that "arguments" is used inside a scope
 */
void
scopes_tree_set_arguments_used (scopes_tree tree) /**< scope */
{
  assert_tree (tree);
  tree->ref_arguments = true;
} /* merge_subscopes */

/**
 * Set up a flag, indicating that "eval" is used inside a scope
 */
void
scopes_tree_set_eval_used (scopes_tree tree) /**< scope */
{
  assert_tree (tree);
  tree->ref_eval = true;
} /* scopes_tree_set_eval_used */

/**
 * Set up a flag, indicating that 'with' statement is contained in a scope
 */
void
scopes_tree_set_contains_with (scopes_tree tree) /**< scope */
{
  assert_tree (tree);
  tree->contains_with = true;
} /* scopes_tree_set_contains_with */

/**
 * Set up a flag, indicating that 'try' statement is contained in a scope
 */
void
scopes_tree_set_contains_try (scopes_tree tree) /**< scope */
{
  assert_tree (tree);
  tree->contains_try = true;
} /* scopes_tree_set_contains_try */

/**
 * Set up a flag, indicating that 'delete' operator is contained in a scope
 */
void
scopes_tree_set_contains_delete (scopes_tree tree) /**< scope */
{
  assert_tree (tree);
  tree->contains_delete = true;
} /* scopes_tree_set_contains_delete */

/**
 * Set up a flag, indicating that there is a function declaration / expression inside a scope
 */
void
scopes_tree_set_contains_functions (scopes_tree tree) /**< scope */
{
  assert_tree (tree);
  tree->contains_functions = true;
} /* scopes_tree_set_contains_functions */

bool
scopes_tree_strict_mode (scopes_tree tree)
{
  assert_tree (tree);
  return (bool) tree->strict_mode;
}

/**
 * Get number of subscopes (immediate function declarations / expressions) of the specified scope
 *
 * @return the number
 */
size_t
scopes_tree_child_scopes_num (scopes_tree tree) /**< a scopes tree node */
{
  return tree->child_scopes_num;
} /* scopes_tree_child_scopes_num */

void
scopes_tree_init (void)
{
  scopes_tree_root_node_p = NULL;
  scopes_tree_last_node_p = NULL;
} /* scopes_tree_init */

void
scopes_tree_finalize (void)
{
  JERRY_ASSERT (scopes_tree_root_node_p == NULL);
  JERRY_ASSERT (scopes_tree_last_node_p == NULL);
} /* scopes_tree_finalize */

/**
 * Initialize a scope
 *
 * @return initialized scope
 */
scopes_tree
scopes_tree_new_scope (scopes_tree parent, /**< parent scope */
                       scope_type_t type) /**< scope type */
{
  scopes_tree tree = (scopes_tree) jsp_mm_alloc (sizeof (scopes_tree_int));

  tree->child_scopes_num = 0;
  tree->child_scopes_processed_num = 0;
  tree->max_uniq_literals_num = 0;

  tree->bc_header_cp = MEM_CP_NULL;

  tree->next_scope_cp = MEM_CP_NULL;
  tree->instrs_count = 0;
  tree->type = type;
  tree->strict_mode = false;
  tree->ref_arguments = false;
  tree->ref_eval = false;
  tree->contains_with = false;
  tree->contains_try = false;
  tree->contains_delete = false;
  tree->contains_functions = false;
  tree->is_vars_and_args_to_regs_possible = false;
  tree->var_decls = linked_list_init (sizeof (op_meta));

  if (parent != NULL)
  {
    JERRY_ASSERT (scopes_tree_root_node_p != NULL);
    JERRY_ASSERT (scopes_tree_last_node_p != NULL);

    parent->child_scopes_num++;
  }
  else
  {
    JERRY_ASSERT (scopes_tree_root_node_p == NULL);
    JERRY_ASSERT (scopes_tree_last_node_p == NULL);

    scopes_tree_root_node_p = tree;
    scopes_tree_last_node_p = tree;
  }

  MEM_CP_SET_NON_NULL_POINTER (scopes_tree_last_node_p->next_scope_cp, tree);
  tree->next_scope_cp = MEM_CP_NULL;

  scopes_tree_last_node_p = tree;

  return tree;
} /* scopes_tree_new_scope */

void
scopes_tree_finish_build (void)
{
  JERRY_ASSERT (scopes_tree_root_node_p != NULL);
  JERRY_ASSERT (scopes_tree_last_node_p != NULL);

  scopes_tree_root_node_p = NULL;
  scopes_tree_last_node_p = NULL;
} /* scopes_tree_finish_build */

void
scopes_tree_free_scope (scopes_tree scope_p)
{
  assert_tree (scope_p);

  JERRY_ASSERT (scopes_tree_root_node_p == NULL);
  JERRY_ASSERT (scopes_tree_last_node_p == NULL);

  linked_list_free (scope_p->var_decls);

  jsp_mm_free (scope_p);
} /* scopes_tree_free_scope */

