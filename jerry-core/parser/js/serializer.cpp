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

static bytecode_data_t bytecode_data;
static scopes_tree current_scope;
static array_list bytecodes_cache; /**< storage of pointers to byetecodes */
static bool print_opcodes;

op_meta
serializer_get_op_meta (opcode_counter_t oc)
{
  JERRY_ASSERT (current_scope);
  return scopes_tree_op_meta (current_scope, oc);
}

opcode_t
serializer_get_opcode (opcode_counter_t oc)
{
  if (bytecode_data.opcodes == NULL)
  {
    return serializer_get_op_meta (oc).op;
  }
  JERRY_ASSERT (oc < bytecode_data.opcodes_count);
  return bytecode_data.opcodes[oc];
}

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
                                  const opcode_t *opcodes_p, /**< pointer to bytecode */
                                  opcode_counter_t oc) /**< position in the bytecode */
{
  lit_id_hash_table *lit_id_hash = GET_HASH_TABLE_FOR_BYTECODE (opcodes_p == NULL ? bytecode_data.opcodes : opcodes_p);
  if (lit_id_hash == null_hash)
  {
    return INVALID_LITERAL;
  }
  return lit_id_hash_table_lookup (lit_id_hash, id, oc);
} /* serializer_get_literal_cp_by_uid */

const void *
serializer_get_bytecode (void)
{
  JERRY_ASSERT (bytecode_data.opcodes != NULL);
  return bytecode_data.opcodes;
}

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

void
serializer_merge_scopes_into_bytecode (void)
{
  bytecode_data.opcodes_count = scopes_tree_count_opcodes (current_scope);
  lit_id_hash_table *lit_id_hash = lit_id_hash_table_init (scopes_tree_count_literals_in_blocks (current_scope),
                                                           (size_t) bytecode_data.opcodes_count / BLOCK_SIZE + 1);
  bytecode_data.opcodes = scopes_tree_raw_data (current_scope, lit_id_hash);
  bytecodes_cache = array_list_append (bytecodes_cache, &bytecode_data.opcodes);
}

void
serializer_dump_literals (void)
{
#ifdef JERRY_ENABLE_PRETTY_PRINTER
  if (print_opcodes)
  {
    lit_dump_literals ();
  }
#endif
}

void
serializer_dump_op_meta (op_meta op)
{
  JERRY_ASSERT (scopes_tree_opcodes_num (current_scope) < MAX_OPCODES);

  scopes_tree_add_op_meta (current_scope, op);

#ifdef JERRY_ENABLE_PRETTY_PRINTER
  if (print_opcodes)
  {
    pp_op_meta ((opcode_counter_t) (scopes_tree_opcodes_num (current_scope) - 1), op, false);
  }
#endif
}

opcode_counter_t
serializer_get_current_opcode_counter (void)
{
  return scopes_tree_opcodes_num (current_scope);
}

opcode_counter_t
serializer_count_opcodes_in_subscopes (void)
{
  return (opcode_counter_t) (scopes_tree_count_opcodes (current_scope) - scopes_tree_opcodes_num (current_scope));
}

void
serializer_set_writing_position (opcode_counter_t oc)
{
  scopes_tree_set_opcodes_num (current_scope, oc);
}

void
serializer_rewrite_op_meta (const opcode_counter_t loc, op_meta op)
{
  scopes_tree_set_op_meta (current_scope, loc, op);

#ifdef JERRY_ENABLE_PRETTY_PRINTER
  if (print_opcodes)
  {
    pp_op_meta (loc, op, true);
  }
#endif
}

void
serializer_print_opcodes (void)
{
#ifdef JERRY_ENABLE_PRETTY_PRINTER
  opcode_counter_t loc;

  if (!print_opcodes)
  {
    return;
  }

  printf ("AFTER OPTIMIZER:\n");

  for (loc = 0; loc < bytecode_data.opcodes_count; loc++)
  {
    op_meta opm;

    opm.op = bytecode_data.opcodes[loc];
    for (int i = 0; i < 3; i++)
    {
      opm.lit_id[i] = NOT_A_LITERAL;
    }

    pp_op_meta (loc, opm, false);
  }
#endif
}

void
serializer_init ()
{
  current_scope = NULL;
  print_opcodes = false;

  bytecode_data.strings_buffer = NULL;
  bytecode_data.opcodes = NULL;

  lit_init ();

  bytecodes_cache = array_list_init (sizeof (opcode_t *));
}

void serializer_set_show_opcodes (bool show_opcodes)
{
  print_opcodes = show_opcodes;
}

void
serializer_free (void)
{
  if (bytecode_data.strings_buffer)
  {
    mem_heap_free_block ((uint8_t *) bytecode_data.strings_buffer);
  }

  lit_finalize ();

  for (size_t i = 0; i < array_list_len (bytecodes_cache); ++i)
  {
    lit_id_hash_table_free (GET_BYTECODE_HEADER (*(opcode_t **) array_list_element (bytecodes_cache, i))->lit_id_hash);
    mem_heap_free_block (GET_BYTECODE_HEADER (*(opcode_t **) array_list_element (bytecodes_cache, i)));
  }
  array_list_free (bytecodes_cache);
}
