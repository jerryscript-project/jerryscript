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

#include "serializer.h"
#include "bytecode-data.h"
#include "pretty-printer.h"
#include "array-list.h"
#include "scopes-tree.h"

static bytecode_data_t bytecode_data;
static scopes_tree current_scope;
static bool print_instrs;

static void
serializer_print_instrs (const vm_instr_t *instrs_p,
                         size_t instrs_count);

op_meta
serializer_get_op_meta (vm_instr_counter_t oc)
{
  JERRY_ASSERT (current_scope);
  return scopes_tree_op_meta (current_scope, oc);
}

/**
 * Get variable declaration of the current scope
 *
 * @return variable declaration instruction
 */
op_meta
serializer_get_var_decl (vm_instr_counter_t oc) /**< index of variable declaration */
{
  JERRY_ASSERT (current_scope);
  return scopes_tree_var_decl (current_scope, oc);
} /* serializer_get_var_decl */

/**
 * Get byte-code instruction from current scope, or specified byte-code array
 *
 * @return byte-code instruction
 */
vm_instr_t
serializer_get_instr (const vm_instr_t *instrs_p, /**< pointer to byte-code array (or NULL,
                                                   *   if instruction should be taken from
                                                   *   instruction list of current scope) */
                      vm_instr_counter_t oc) /**< position of the intruction */
{
  if (instrs_p == NULL)
  {
    return serializer_get_op_meta (oc).op;
  }
  else
  {
    JERRY_ASSERT (oc < GET_BYTECODE_HEADER (instrs_p)->instructions_number);
    return instrs_p[oc];
  }
} /* serializer_get_instr */

/**
 * Convert literal id (operand value of instruction) to compressed pointer to literal
 *
 * Bytecode is divided into blocks of fixed size and each block has independent encoding of variable names,
 * which are represented by 8 bit numbers - ids.
 * This function performs conversion from id to literal.
 *
 * @return compressed pointer to literal
 */
lit_cpointer_t
serializer_get_literal_cp_by_uid (uint8_t id, /**< literal idx */
                                  const vm_instr_t *instrs_p, /**< pointer to bytecode */
                                  vm_instr_counter_t oc) /**< position in the bytecode */
{
  lit_id_hash_table *lit_id_hash = GET_HASH_TABLE_FOR_BYTECODE (instrs_p == NULL ? bytecode_data.instrs_p : instrs_p);
  if (lit_id_hash == null_hash)
  {
    return INVALID_LITERAL;
  }
  return lit_id_hash_table_lookup (lit_id_hash, id, oc);
} /* serializer_get_literal_cp_by_uid */

void
serializer_set_strings_buffer (const ecma_char_t *s)
{
  bytecode_data.strings_buffer = s;
}

void
serializer_set_scope (scopes_tree new_scope)
{
  current_scope = new_scope;
}

/**
 * Dump scope to current scope
 *
 * NOTE:
 *   This function is used for processing of function expressions as they should not be hoisted.
 *   After parsing a function expression, it is immediately dumped to current scope via call of this function.
 */
void
serializer_dump_subscope (scopes_tree tree) /**< scope to dump */
{
  JERRY_ASSERT (tree != NULL);
  vm_instr_counter_t instr_pos;
  bool header = true;
  for (instr_pos = 0; instr_pos < tree->instrs_count; instr_pos++)
  {
    op_meta *om_p = (op_meta *) linked_list_element (tree->instrs, instr_pos);
    if (om_p->op.op_idx != VM_OP_VAR_DECL
        && om_p->op.op_idx != VM_OP_META && !header)
    {
      break;
    }
    if (om_p->op.op_idx == VM_OP_REG_VAR_DECL)
    {
      header = false;
    }
    scopes_tree_add_op_meta (current_scope, *om_p);
  }
  for (vm_instr_counter_t var_decl_pos = 0; var_decl_pos < tree->var_decls_cout; var_decl_pos++)
  {
    op_meta *om_p = (op_meta *) linked_list_element (tree->var_decls, var_decl_pos);
    scopes_tree_add_op_meta (current_scope, *om_p);
  }
  for (uint8_t child_id = 0; child_id < tree->t.children_num; child_id++)
  {
    serializer_dump_subscope (*(scopes_tree *) linked_list_element (tree->t.children, child_id));
  }
  for (; instr_pos < tree->instrs_count; instr_pos++)
  {
    op_meta *om_p = (op_meta *) linked_list_element (tree->instrs, instr_pos);
    scopes_tree_add_op_meta (current_scope, *om_p);
  }
} /* serializer_dump_subscope */

const vm_instr_t *
serializer_merge_scopes_into_bytecode (void)
{
  bytecode_data.instrs_count = scopes_tree_count_instructions (current_scope);

  const size_t buckets_count = scopes_tree_count_literals_in_blocks (current_scope);
  const size_t blocks_count = (size_t) bytecode_data.instrs_count / BLOCK_SIZE + 1;
  const vm_instr_counter_t instrs_count = scopes_tree_count_instructions (current_scope);

  const size_t bytecode_array_size = JERRY_ALIGNUP (sizeof (insts_data_header_t) + instrs_count * sizeof (vm_instr_t),
                                                    MEM_ALIGNMENT);
  const size_t lit_id_hash_table_size = JERRY_ALIGNUP (lit_id_hash_table_get_size_for_table (buckets_count,
                                                                                             blocks_count),
                                                       MEM_ALIGNMENT);

  uint8_t *buffer_p = (uint8_t*) mem_heap_alloc_block (bytecode_array_size + lit_id_hash_table_size,
                                                       MEM_HEAP_ALLOC_LONG_TERM);

  lit_id_hash_table *lit_id_hash = lit_id_hash_table_init (buffer_p + bytecode_array_size,
                                                           lit_id_hash_table_size,
                                                           buckets_count, blocks_count);

  const vm_instr_t *instrs_p = scopes_tree_raw_data (current_scope, buffer_p, bytecode_array_size, lit_id_hash);

  insts_data_header_t *header_p = (insts_data_header_t*) buffer_p;
  MEM_CP_SET_POINTER (header_p->next_instrs_cp, bytecode_data.instrs_p);
  header_p->instructions_number = instrs_count;
  bytecode_data.instrs_p = instrs_p;

  if (print_instrs)
  {
    lit_dump_literals ();
    serializer_print_instrs (instrs_p, bytecode_data.instrs_count);
  }

  return instrs_p;
}

void
serializer_dump_op_meta (op_meta op)
{
  JERRY_ASSERT (scopes_tree_instrs_num (current_scope) < MAX_OPCODES);

  scopes_tree_add_op_meta (current_scope, op);

#ifdef JERRY_ENABLE_PRETTY_PRINTER
  if (print_instrs)
  {
    pp_op_meta (NULL, (vm_instr_counter_t) (scopes_tree_instrs_num (current_scope) - 1), op, false);
  }
#endif
}

/**
 * Dump variable declaration into the current scope
 */
void
serializer_dump_var_decl (op_meta op) /**< variable declaration instruction */
{
  JERRY_ASSERT (scopes_tree_instrs_num (current_scope) + current_scope->var_decls_cout < MAX_OPCODES);

  scopes_tree_add_var_decl (current_scope, op);
} /* serializer_dump_var_decl */

vm_instr_counter_t
serializer_get_current_instr_counter (void)
{
  return scopes_tree_instrs_num (current_scope);
}

/**
 * Get number of variable declarations in the current scope
 *
 * @return count of variable declarations
 */
vm_instr_counter_t
serializer_get_current_var_decls_counter (void)
{
  return scopes_tree_var_decls_num (current_scope);
} /* serializer_get_current_var_decls_counter */

vm_instr_counter_t
serializer_count_instrs_in_subscopes (void)
{
  return (vm_instr_counter_t) (scopes_tree_count_instructions (current_scope) - scopes_tree_instrs_num (current_scope));
}

void
serializer_set_writing_position (vm_instr_counter_t oc)
{
  scopes_tree_set_instrs_num (current_scope, oc);
}

void
serializer_rewrite_op_meta (const vm_instr_counter_t loc, op_meta op)
{
  scopes_tree_set_op_meta (current_scope, loc, op);

#ifdef JERRY_ENABLE_PRETTY_PRINTER
  if (print_instrs)
  {
    pp_op_meta (NULL, loc, op, true);
  }
#endif
}

static void
serializer_print_instrs (const vm_instr_t *instrs_p,
                         size_t instrs_count)
{
#ifdef JERRY_ENABLE_PRETTY_PRINTER
  for (vm_instr_counter_t loc = 0; loc < instrs_count; loc++)
  {
    op_meta opm;

    opm.op = instrs_p[loc];
    for (int i = 0; i < 3; i++)
    {
      opm.lit_id[i] = NOT_A_LITERAL;
    }

    pp_op_meta (instrs_p, loc, opm, false);
  }
#else
  (void) instrs_p;
  (void) instrs_count;
#endif
}

void
serializer_init ()
{
  current_scope = NULL;
  print_instrs = false;

  bytecode_data.strings_buffer = NULL;
  bytecode_data.instrs_p = NULL;

  lit_init ();
}

void serializer_set_show_instrs (bool show_instrs)
{
  print_instrs = show_instrs;
}

/**
 * Deletes bytecode and associated hash table
 */
void
serializer_remove_instructions (const vm_instr_t *instrs_p) /**< pointer to instructions which should be deleted */
{
  insts_data_header_t *prev_header = NULL;
  const vm_instr_t *cur_instrs_p = bytecode_data.instrs_p;
  while (cur_instrs_p != NULL)
  {
    insts_data_header_t *cur_header_p = GET_BYTECODE_HEADER (cur_instrs_p);

    if (cur_instrs_p == instrs_p)
    {
      if (prev_header)
      {
        prev_header->next_instrs_cp = cur_header_p->next_instrs_cp;
      }
      else
      {
        bytecode_data.instrs_p = MEM_CP_GET_POINTER (vm_instr_t, cur_header_p->next_instrs_cp);
      }
      mem_heap_free_block (cur_header_p);
      break;
    }

    prev_header = GET_BYTECODE_HEADER (cur_instrs_p);
    cur_instrs_p = MEM_CP_GET_POINTER (vm_instr_t, cur_header_p->next_instrs_cp);
  }
} /* serializer_remove_instructions */

void
serializer_free (void)
{
  if (bytecode_data.strings_buffer)
  {
    mem_heap_free_block ((uint8_t *) bytecode_data.strings_buffer);
  }

  lit_finalize ();

  while (bytecode_data.instrs_p != NULL)
  {
    insts_data_header_t *header_p = GET_BYTECODE_HEADER (bytecode_data.instrs_p);
    bytecode_data.instrs_p = MEM_CP_GET_POINTER (vm_instr_t, header_p->next_instrs_cp);

    mem_heap_free_block (header_p);
  }
}
