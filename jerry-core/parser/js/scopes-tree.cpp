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

#define OPCODE(op) (__op__idx_##op)
#define HASH_SIZE 128

static hash_table lit_id_to_uid = null_hash;
static opcode_counter_t global_oc;
static idx_t next_uid;

static void
assert_tree (scopes_tree t)
{
  JERRY_ASSERT (t != NULL);
}

static idx_t
get_uid (op_meta *op, uint8_t i)
{
  JERRY_ASSERT (i < 4);
  raw_opcode *raw = (raw_opcode *) &op->op;
  return raw->uids[i + 1];
}

static void
set_uid (op_meta *op, uint8_t i, idx_t uid)
{
  JERRY_ASSERT (i < 4);
  raw_opcode *raw = (raw_opcode *) &op->op;
  raw->uids[i + 1] = uid;
}

opcode_counter_t
scopes_tree_opcodes_num (scopes_tree t)
{
  assert_tree (t);
  return t->opcodes_num;
}

void
scopes_tree_add_op_meta (scopes_tree tree, op_meta op)
{
  assert_tree (tree);
  linked_list_set_element (tree->opcodes, tree->opcodes_num++, &op);
}

void
scopes_tree_set_op_meta (scopes_tree tree, opcode_counter_t oc, op_meta op)
{
  assert_tree (tree);
  JERRY_ASSERT (oc < tree->opcodes_num);
  linked_list_set_element (tree->opcodes, oc, &op);
}

void
scopes_tree_set_opcodes_num (scopes_tree tree, opcode_counter_t oc)
{
  assert_tree (tree);
  JERRY_ASSERT (oc < tree->opcodes_num);
  tree->opcodes_num = oc;
}

op_meta
scopes_tree_op_meta (scopes_tree tree, opcode_counter_t oc)
{
  assert_tree (tree);
  JERRY_ASSERT (oc < tree->opcodes_num);
  return *(op_meta *) linked_list_element (tree->opcodes, oc);
}

opcode_counter_t
scopes_tree_count_opcodes (scopes_tree t)
{
  assert_tree (t);
  opcode_counter_t res = t->opcodes_num;
  for (uint8_t i = 0; i < t->t.children_num; i++)
  {
    res = (opcode_counter_t) (
      res + scopes_tree_count_opcodes (
        *(scopes_tree *) linked_list_element (t->t.children, i)));
  }
  return res;
}

static uint16_t
lit_id_hash (void * lit_id)
{
  return ((lit_cpointer_t *) lit_id)->packed_value % HASH_SIZE;
}

static void
start_new_block_if_necessary (void)
{
  if (global_oc % BLOCK_SIZE == 0)
  {
    next_uid = 0;
    if (lit_id_to_uid != null_hash)
    {
      hash_table_free (lit_id_to_uid);
      lit_id_to_uid = null_hash;
    }
    lit_id_to_uid = hash_table_init (sizeof (lit_cpointer_t), sizeof (idx_t), HASH_SIZE, lit_id_hash);
  }
}

static bool
is_possible_literal (uint16_t mask, uint8_t index)
{
  int res;
  switch (index)
  {
    case 0:
    {
      res = mask >> 8;
      break;
    }
    case 1:
    {
      res = (mask & 0xF0) >> 4;
      break;
    }
    default:
    {
      JERRY_ASSERT (index = 2);
      res = mask & 0x0F;
    }
  }
  JERRY_ASSERT (res == 0 || res == 1);
  return res == 1;
}

static void
change_uid (op_meta *om, lit_id_hash_table *lit_ids, uint16_t mask)
{
  for (uint8_t i = 0; i < 3; i++)
  {
    if (is_possible_literal (mask, i))
    {
      if (get_uid (om, i) == LITERAL_TO_REWRITE)
      {
        JERRY_ASSERT (om->lit_id[i].packed_value != MEM_CP_NULL);
        lit_cpointer_t lit_id = om->lit_id[i];
        idx_t *uid = (idx_t *) hash_table_lookup (lit_id_to_uid, &lit_id);
        if (uid == NULL)
        {
          hash_table_insert (lit_id_to_uid, &lit_id, &next_uid);
          lit_id_hash_table_insert (lit_ids, next_uid, global_oc, lit_id);
          uid = (idx_t *) hash_table_lookup (lit_id_to_uid, &lit_id);
          JERRY_ASSERT (uid != NULL);
          JERRY_ASSERT (*uid == next_uid);
          next_uid++;
        }
        set_uid (om, i, *uid);
      }
      else
      {
        JERRY_ASSERT (om->lit_id[i].packed_value == MEM_CP_NULL);
      }
    }
    else
    {
      JERRY_ASSERT (om->lit_id[i].packed_value == MEM_CP_NULL);
    }
  }
}

static void
insert_uids_to_lit_id_map (op_meta *om, uint16_t mask)
{
  for (uint8_t i = 0; i < 3; i++)
  {
    if (is_possible_literal (mask, i))
    {
      if (get_uid (om, i) == LITERAL_TO_REWRITE)
      {
        JERRY_ASSERT (om->lit_id[i].packed_value != MEM_CP_NULL);
        lit_cpointer_t lit_id = om->lit_id[i];
        idx_t *uid = (idx_t *) hash_table_lookup (lit_id_to_uid, &lit_id);
        if (uid == NULL)
        {
          hash_table_insert (lit_id_to_uid, &lit_id, &next_uid);
          uid = (idx_t *) hash_table_lookup (lit_id_to_uid, &lit_id);
          JERRY_ASSERT (uid != NULL);
          JERRY_ASSERT (*uid == next_uid);
          next_uid++;
        }
      }
      else
      {
        JERRY_ASSERT (om->lit_id[i].packed_value == MEM_CP_NULL);
      }
    }
    else
    {
      JERRY_ASSERT (om->lit_id[i].packed_value == MEM_CP_NULL);
    }
  }
}

static op_meta *
extract_op_meta (scopes_tree tree, opcode_counter_t opc_index)
{
  return (op_meta *) linked_list_element (tree->opcodes, opc_index);
}

static opcode_t
generate_opcode (scopes_tree tree, opcode_counter_t opc_index, lit_id_hash_table *lit_ids)
{
  start_new_block_if_necessary ();
  op_meta *om = extract_op_meta (tree, opc_index);
  /* Now we should change uids of opcodes.
     Since different opcodes has different literals/tmps in different places,
     we should change only them.
     For each case possible literal positions are shown as 0xYYY literal,
     where Y is set to '1' when there is a possible literal in this position,
     and '0' otherwise.  */
  switch (om->op.op_idx)
  {
    case OPCODE (prop_getter):
    case OPCODE (prop_setter):
    case OPCODE (delete_prop):
    case OPCODE (b_shift_left):
    case OPCODE (b_shift_right):
    case OPCODE (b_shift_uright):
    case OPCODE (b_and):
    case OPCODE (b_or):
    case OPCODE (b_xor):
    case OPCODE (equal_value):
    case OPCODE (not_equal_value):
    case OPCODE (equal_value_type):
    case OPCODE (not_equal_value_type):
    case OPCODE (less_than):
    case OPCODE (greater_than):
    case OPCODE (less_or_equal_than):
    case OPCODE (greater_or_equal_than):
    case OPCODE (instanceof):
    case OPCODE (in):
    case OPCODE (addition):
    case OPCODE (substraction):
    case OPCODE (division):
    case OPCODE (multiplication):
    case OPCODE (remainder):
    {
      change_uid (om, lit_ids, 0x111);
      break;
    }
    case OPCODE (call_n):
    case OPCODE (native_call):
    case OPCODE (construct_n):
    case OPCODE (func_expr_n):
    case OPCODE (delete_var):
    case OPCODE (typeof):
    case OPCODE (b_not):
    case OPCODE (logical_not):
    case OPCODE (post_incr):
    case OPCODE (post_decr):
    case OPCODE (pre_incr):
    case OPCODE (pre_decr):
    case OPCODE (unary_plus):
    case OPCODE (unary_minus):
    {
      change_uid (om, lit_ids, 0x110);
      break;
    }
    case OPCODE (assignment):
    {
      switch (om->op.data.assignment.type_value_right)
      {
        case OPCODE_ARG_TYPE_SIMPLE:
        case OPCODE_ARG_TYPE_SMALLINT:
        case OPCODE_ARG_TYPE_SMALLINT_NEGATE:
        {
          change_uid (om, lit_ids, 0x100);
          break;
        }
        case OPCODE_ARG_TYPE_NUMBER:
        case OPCODE_ARG_TYPE_NUMBER_NEGATE:
        case OPCODE_ARG_TYPE_REGEXP:
        case OPCODE_ARG_TYPE_STRING:
        case OPCODE_ARG_TYPE_VARIABLE:
        {
          change_uid (om, lit_ids, 0x101);
          break;
        }
      }
      break;
    }
    case OPCODE (func_decl_n):
    case OPCODE (array_decl):
    case OPCODE (obj_decl):
    case OPCODE (this_binding):
    case OPCODE (with):
    case OPCODE (throw_value):
    case OPCODE (is_true_jmp_up):
    case OPCODE (is_true_jmp_down):
    case OPCODE (is_false_jmp_up):
    case OPCODE (is_false_jmp_down):
    case OPCODE (var_decl):
    case OPCODE (retval):
    {
      change_uid (om, lit_ids, 0x100);
      break;
    }
    case OPCODE (exitval):
    case OPCODE (ret):
    case OPCODE (try_block):
    case OPCODE (jmp_up):
    case OPCODE (jmp_down):
    case OPCODE (nop):
    case OPCODE (reg_var_decl):
    {
      change_uid (om, lit_ids, 0x000);
      break;
    }
    case OPCODE (meta):
    {
      switch (om->op.data.meta.type)
      {
        case OPCODE_META_TYPE_VARG_PROP_DATA:
        case OPCODE_META_TYPE_VARG_PROP_GETTER:
        case OPCODE_META_TYPE_VARG_PROP_SETTER:
        {
          change_uid (om, lit_ids, 0x011);
          break;
        }
        case OPCODE_META_TYPE_VARG:
        case OPCODE_META_TYPE_CATCH_EXCEPTION_IDENTIFIER:
        {
          change_uid (om, lit_ids, 0x010);
          break;
        }
        case OPCODE_META_TYPE_UNDEFINED:
        case OPCODE_META_TYPE_END_WITH:
        case OPCODE_META_TYPE_FUNCTION_END:
        case OPCODE_META_TYPE_CATCH:
        case OPCODE_META_TYPE_FINALLY:
        case OPCODE_META_TYPE_END_TRY_CATCH_FINALLY:
        case OPCODE_META_TYPE_CALL_SITE_INFO:
        case OPCODE_META_TYPE_SCOPE_CODE_FLAGS:
        {
          change_uid (om, lit_ids, 0x000);
          break;
        }
      }
      break;
    }
  }
  return om->op;
}

static idx_t
count_new_literals_in_opcode (scopes_tree tree, opcode_counter_t opc_index)
{
  start_new_block_if_necessary ();
  idx_t current_uid = next_uid;
  op_meta *om = extract_op_meta (tree, opc_index);
  switch (om->op.op_idx)
  {
    case OPCODE (prop_getter):
    case OPCODE (prop_setter):
    case OPCODE (delete_prop):
    case OPCODE (b_shift_left):
    case OPCODE (b_shift_right):
    case OPCODE (b_shift_uright):
    case OPCODE (b_and):
    case OPCODE (b_or):
    case OPCODE (b_xor):
    case OPCODE (equal_value):
    case OPCODE (not_equal_value):
    case OPCODE (equal_value_type):
    case OPCODE (not_equal_value_type):
    case OPCODE (less_than):
    case OPCODE (greater_than):
    case OPCODE (less_or_equal_than):
    case OPCODE (greater_or_equal_than):
    case OPCODE (instanceof):
    case OPCODE (in):
    case OPCODE (addition):
    case OPCODE (substraction):
    case OPCODE (division):
    case OPCODE (multiplication):
    case OPCODE (remainder):
    {
      insert_uids_to_lit_id_map (om, 0x111);
      break;
    }
    case OPCODE (call_n):
    case OPCODE (native_call):
    case OPCODE (construct_n):
    case OPCODE (func_expr_n):
    case OPCODE (delete_var):
    case OPCODE (typeof):
    case OPCODE (b_not):
    case OPCODE (logical_not):
    case OPCODE (post_incr):
    case OPCODE (post_decr):
    case OPCODE (pre_incr):
    case OPCODE (pre_decr):
    case OPCODE (unary_plus):
    case OPCODE (unary_minus):
    {
      insert_uids_to_lit_id_map (om, 0x110);
      break;
    }
    case OPCODE (assignment):
    {
      switch (om->op.data.assignment.type_value_right)
      {
        case OPCODE_ARG_TYPE_SIMPLE:
        case OPCODE_ARG_TYPE_SMALLINT:
        case OPCODE_ARG_TYPE_SMALLINT_NEGATE:
        {
          insert_uids_to_lit_id_map (om, 0x100);
          break;
        }
        case OPCODE_ARG_TYPE_NUMBER:
        case OPCODE_ARG_TYPE_NUMBER_NEGATE:
        case OPCODE_ARG_TYPE_REGEXP:
        case OPCODE_ARG_TYPE_STRING:
        case OPCODE_ARG_TYPE_VARIABLE:
        {
          insert_uids_to_lit_id_map (om, 0x101);
          break;
        }
      }
      break;
    }
    case OPCODE (func_decl_n):
    case OPCODE (array_decl):
    case OPCODE (obj_decl):
    case OPCODE (this_binding):
    case OPCODE (with):
    case OPCODE (throw_value):
    case OPCODE (is_true_jmp_up):
    case OPCODE (is_true_jmp_down):
    case OPCODE (is_false_jmp_up):
    case OPCODE (is_false_jmp_down):
    case OPCODE (var_decl):
    case OPCODE (retval):
    {
      insert_uids_to_lit_id_map (om, 0x100);
      break;
    }
    case OPCODE (exitval):
    case OPCODE (ret):
    case OPCODE (try_block):
    case OPCODE (jmp_up):
    case OPCODE (jmp_down):
    case OPCODE (nop):
    case OPCODE (reg_var_decl):
    {
      insert_uids_to_lit_id_map (om, 0x000);
      break;
    }
    case OPCODE (meta):
    {
      switch (om->op.data.meta.type)
      {
        case OPCODE_META_TYPE_VARG_PROP_DATA:
        case OPCODE_META_TYPE_VARG_PROP_GETTER:
        case OPCODE_META_TYPE_VARG_PROP_SETTER:
        {
          insert_uids_to_lit_id_map (om, 0x011);
          break;
        }
        case OPCODE_META_TYPE_VARG:
        case OPCODE_META_TYPE_CATCH_EXCEPTION_IDENTIFIER:
        {
          insert_uids_to_lit_id_map (om, 0x010);
          break;
        }
        case OPCODE_META_TYPE_UNDEFINED:
        case OPCODE_META_TYPE_END_WITH:
        case OPCODE_META_TYPE_FUNCTION_END:
        case OPCODE_META_TYPE_CATCH:
        case OPCODE_META_TYPE_FINALLY:
        case OPCODE_META_TYPE_END_TRY_CATCH_FINALLY:
        case OPCODE_META_TYPE_CALL_SITE_INFO:
        case OPCODE_META_TYPE_SCOPE_CODE_FLAGS:
        {
          insert_uids_to_lit_id_map (om, 0x000);
          break;
        }
      }
      break;
    }
  }
  return (idx_t) (next_uid - current_uid);
}

/* Before filling literal indexes 'hash' table we shall initiate it with number of neccesary literal indexes.
   Since bytecode is divided into blocks and id of the block is a part of hash key, we shall divide bytecode
   into blocks and count unique literal indexes used in each block. */
size_t
scopes_tree_count_literals_in_blocks (scopes_tree tree)
{
  assert_tree (tree);
  size_t result = 0;
  if (lit_id_to_uid != null_hash)
  {
    hash_table_free (lit_id_to_uid);
    lit_id_to_uid = null_hash;
  }
  next_uid = 0;
  global_oc = 0;

  assert_tree (tree);
  opcode_counter_t opc_index;
  bool header = true;
  for (opc_index = 0; opc_index < tree->opcodes_num; opc_index++)
  {
    op_meta *om = extract_op_meta (tree, opc_index);
    if (om->op.op_idx != OPCODE (var_decl)
        && om->op.op_idx != OPCODE (meta) && !header)
    {
      break;
    }
    if (om->op.op_idx == OPCODE (reg_var_decl))
    {
      header = false;
    }
    result += count_new_literals_in_opcode (tree, opc_index);
  }
  for (uint8_t child_id = 0; child_id < tree->t.children_num; child_id++)
  {
    result += scopes_tree_count_literals_in_blocks (*(scopes_tree *) linked_list_element (tree->t.children, child_id));
  }
  for (; opc_index < tree->opcodes_num; opc_index++)
  {
    result += count_new_literals_in_opcode (tree, opc_index);
  }

  return result;
}

/* This function performs functions hoisting.

   Each scope consists of four parts:
   1) Header with 'use strict' marker and reg_var_decl opcode
   2) Variable declarations, dumped by the preparser
   3) Function declarations
   4) Computational code

   Header and var_decls are dumped first,
   then we shall recursively dump function declaration,
   and finally, other opcodes.

   For each opcodes block (size of block is defined in bytecode-data.h)
   literal indexes 'hash' table is filled. */
static void
merge_subscopes (scopes_tree tree, opcode_t *data, lit_id_hash_table *lit_ids)
{
  assert_tree (tree);
  JERRY_ASSERT (data);
  opcode_counter_t opc_index;
  bool header = true;
  for (opc_index = 0; opc_index < tree->opcodes_num; opc_index++)
  {
    op_meta *om = extract_op_meta (tree, opc_index);
    if (om->op.op_idx != OPCODE (var_decl)
        && om->op.op_idx != OPCODE (meta) && !header)
    {
      break;
    }
    if (om->op.op_idx == OPCODE (reg_var_decl))
    {
      header = false;
    }
    data[global_oc] = generate_opcode (tree, opc_index, lit_ids);
    global_oc++;
  }
  for (uint8_t child_id = 0; child_id < tree->t.children_num; child_id++)
  {
    merge_subscopes (*(scopes_tree *) linked_list_element (tree->t.children, child_id),
                     data, lit_ids);
  }
  for (; opc_index < tree->opcodes_num; opc_index++)
  {
    data[global_oc] = generate_opcode (tree, opc_index, lit_ids);
    global_oc++;
  }
}

/* Postparser.
   Init literal indexes 'hash' table.
   Reorder function declarations.
   Rewrite opcodes' temporary uids with their keys in literal indexes 'hash' table. */
opcode_t *
scopes_tree_raw_data (scopes_tree tree, /**< scopes tree to convert to byte-code array */
                      uint8_t *buffer_p, /**< buffer for byte-code array and literal identifiers hash table */
                      size_t opcodes_array_size, /**< size of space for byte-code array */
                      lit_id_hash_table *lit_ids) /**< literal identifiers hash table */
{
  JERRY_ASSERT (lit_ids);
  assert_tree (tree);
  if (lit_id_to_uid != null_hash)
  {
    hash_table_free (lit_id_to_uid);
    lit_id_to_uid = null_hash;
  }
  next_uid = 0;
  global_oc = 0;

  /* Dump bytecode and fill literal indexes 'hash' table. */
  JERRY_ASSERT (opcodes_array_size >=
                sizeof (opcodes_header_t) + (size_t) (scopes_tree_count_opcodes (tree)) * sizeof (opcode_t));

  opcodes_header_t *opcodes_data = (opcodes_header_t *) buffer_p;
  memset (opcodes_data, 0, opcodes_array_size);

  opcode_t *opcodes = (opcode_t *)(((uint8_t*) opcodes_data) + sizeof (opcodes_header_t));
  merge_subscopes (tree, opcodes, lit_ids);
  if (lit_id_to_uid != null_hash)
  {
    hash_table_free (lit_id_to_uid);
    lit_id_to_uid = null_hash;
  }

  MEM_CP_SET_POINTER (opcodes_data->lit_id_hash_cp, lit_ids);

  return opcodes;
} /* scopes_tree_raw_data */

void
scopes_tree_set_strict_mode (scopes_tree tree, bool strict_mode)
{
  assert_tree (tree);
  tree->strict_mode = strict_mode ? 1 : 0;
}

bool
scopes_tree_strict_mode (scopes_tree tree)
{
  assert_tree (tree);
  return (bool) tree->strict_mode;
}

scopes_tree
scopes_tree_init (scopes_tree parent)
{
  scopes_tree tree = (scopes_tree) jsp_mm_alloc (sizeof (scopes_tree_int));
  memset (tree, 0, sizeof (scopes_tree_int));
  tree->t.parent = (tree_header *) parent;
  tree->t.children = null_list;
  tree->t.children_num = 0;
  if (parent != NULL)
  {
    if (parent->t.children_num == 0)
    {
      parent->t.children = linked_list_init (sizeof (scopes_tree));
    }
    linked_list_set_element (parent->t.children, parent->t.children_num, &tree);
    void *added = linked_list_element (parent->t.children, parent->t.children_num);
    JERRY_ASSERT (*(scopes_tree *) added == tree);
    parent->t.children_num++;
  }
  tree->opcodes_num = 0;
  tree->strict_mode = 0;
  tree->opcodes = linked_list_init (sizeof (op_meta));
  return tree;
}

void
scopes_tree_free (scopes_tree tree)
{
  assert_tree (tree);
  if (tree->t.children_num != 0)
  {
    for (uint8_t i = 0; i < tree->t.children_num; ++i)
    {
      scopes_tree_free (*(scopes_tree *) linked_list_element (tree->t.children, i));
    }
    linked_list_free (tree->t.children);
  }
  linked_list_free (tree->opcodes);
  jsp_mm_free (tree);
}
