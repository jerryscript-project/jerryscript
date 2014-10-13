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

#include "scopes-tree.h"
#include "mem-heap.h"
#include "jerry-libc.h"

#define NAME_TO_ID(op) (__op__idx_##op)

static void
assert_tree (scopes_tree t)
{
  JERRY_ASSERT (t != NULL);
  JERRY_ASSERT (t->t.magic == TREE_MAGIC);
}

opcode_counter_t
scopes_tree_opcodes_num (scopes_tree t)
{
  assert_tree (t);
  return t->opcodes_num;
}

void
scopes_tree_add_opcode (scopes_tree tree, opcode_t op)
{
  assert_tree (tree);
  linked_list_set_element (tree->opcodes, sizeof (opcode_t), tree->opcodes_num++, &op);
}

void
scopes_tree_set_opcode (scopes_tree tree, opcode_counter_t oc, opcode_t op)
{
  assert_tree (tree);
  JERRY_ASSERT (oc < tree->opcodes_num);
  linked_list_set_element (tree->opcodes, sizeof (opcode_t), oc, &op);
}

void
scopes_tree_set_opcodes_num (scopes_tree tree, opcode_counter_t oc)
{
  assert_tree (tree);
  JERRY_ASSERT (oc < tree->opcodes_num);
  tree->opcodes_num = oc;
}

opcode_t
scopes_tree_opcode (scopes_tree tree, opcode_counter_t oc)
{
  assert_tree (tree);
  JERRY_ASSERT (oc < tree->opcodes_num);
  return *(opcode_t *) linked_list_element (tree->opcodes, sizeof (opcode_t), oc);
}

opcode_counter_t
scopes_tree_count_opcodes (scopes_tree t)
{
  assert_tree (t);
  opcode_counter_t res = t->opcodes_num;
  for (uint8_t i = 0; i < t->t.children_num; i++)
  {
    res = (opcode_counter_t) (res
                              + scopes_tree_count_opcodes (*(scopes_tree *) linked_list_element (t->t.children,
                                                                                             sizeof (scopes_tree),
                                                                                             i)));
  }
  return res;
}

static opcode_counter_t
merge_subscopes (scopes_tree tree, opcode_t *data)
{
  assert_tree (tree);
  JERRY_ASSERT (data);
  opcode_counter_t opc_index, data_index;
  bool header = true;
  for (opc_index = 0, data_index = 0; opc_index < tree->opcodes_num; opc_index++, data_index++)
  {
    opcode_t *op = (opcode_t *) linked_list_element (tree->opcodes, sizeof (opcode_t), opc_index);
    JERRY_ASSERT (op);
    if (op->op_idx != NAME_TO_ID (var_decl)
        && op->op_idx != NAME_TO_ID (nop)
        && op->op_idx != NAME_TO_ID (meta) && !header)
    {
      break;
    }
    if (op->op_idx == NAME_TO_ID (reg_var_decl))
    {
      header = false;
    }
    data[data_index] = *op;
  }
  for (uint8_t child_id = 0; child_id < tree->t.children_num; child_id++)
  {
    data_index = (opcode_counter_t) (data_index
                                     + merge_subscopes (*(scopes_tree *) linked_list_element (tree->t.children,
                                                                                              sizeof (scopes_tree),
                                                                                              child_id),
                                                        data + data_index));
  }
  for (; opc_index < tree->opcodes_num; opc_index++, data_index++)
  {
    opcode_t *op = (opcode_t *) linked_list_element (tree->opcodes, sizeof (opcode_t), opc_index);
    data[data_index] = *op;
  }
  return data_index;
}

opcode_t *
scopes_tree_raw_data (scopes_tree tree, opcode_counter_t *num)
{
  assert_tree (tree);
  opcode_counter_t res = scopes_tree_count_opcodes (tree);
  size_t size = ((size_t) (res + 1) * sizeof (opcode_t)); // +1 for valgrind
  opcode_t *opcodes = (opcode_t *) mem_heap_alloc_block (size, MEM_HEAP_ALLOC_LONG_TERM);
  __memset (opcodes, 0, size);
  opcode_counter_t merged = merge_subscopes (tree, opcodes);
  JERRY_ASSERT (merged == res);
  *num = res;
  return opcodes;
}

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
  scopes_tree tree = (scopes_tree) mem_heap_alloc_block (sizeof (scopes_tree_int), MEM_HEAP_ALLOC_SHORT_TERM);
  __memset (tree, 0, sizeof (scopes_tree));
  tree->t.magic = TREE_MAGIC;
  tree->t.parent = (tree_header *) parent;
  tree->t.children = null_list;
  tree->t.children_num = 0;
  if (parent != NULL)
  {
    if (parent->t.children_num == 0)
    {
      parent->t.children = linked_list_init (sizeof (scopes_tree));
    }
    linked_list_set_element (parent->t.children, sizeof (scopes_tree), parent->t.children_num, &tree);
    void *added = linked_list_element (parent->t.children, sizeof (scopes_tree), parent->t.children_num);
    JERRY_ASSERT (*(scopes_tree *) added == tree);
    parent->t.children_num++;
  }
  tree->opcodes_num = 0;
  tree->strict_mode = 0;
  tree->opcodes = linked_list_init (sizeof (opcode_t));
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
      scopes_tree_free (*(scopes_tree *) linked_list_element (tree->t.children, sizeof (scopes_tree), i));
    }
    linked_list_free (tree->t.children);
  }
  linked_list_free (tree->opcodes);
  mem_heap_free_block ((uint8_t *) tree);
}
