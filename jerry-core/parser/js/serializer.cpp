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
#include "jrt.h"
#include "parser.h"
#include "jrt-libc-includes.h"
#include "pretty-printer.h"

static bytecode_data_t bytecode_data;
static scopes_tree current_scope;

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

literal
serializer_get_literal_by_id (literal_index_t id)
{
  JERRY_ASSERT (id != INVALID_LITERAL);
  JERRY_ASSERT (id < bytecode_data.literals_count);
  return bytecode_data.literals[id];
}

literal_index_t
serializer_get_literal_id_by_uid (uint8_t id, opcode_counter_t oc)
{
  if (bytecode_data.lit_id_hash == null_hash)
  {
    return INVALID_LITERAL;
  }
  return lit_id_hash_table_lookup (bytecode_data.lit_id_hash, id, oc);
}

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
  JERRY_ASSERT (bytecode_data.lit_id_hash == null_hash);
  bytecode_data.opcodes_count = scopes_tree_count_opcodes (current_scope);
  bytecode_data.lit_id_hash = lit_id_hash_table_init (scopes_tree_count_literals_in_blocks (current_scope),
                                                      bytecode_data.opcodes_count);
  bytecode_data.opcodes = scopes_tree_raw_data (current_scope, bytecode_data.lit_id_hash);
}

void
serializer_dump_literals (const literal *literals, literal_index_t literals_count)
{
#ifdef JERRY_ENABLE_PRETTY_PRINTER
  if (print_opcodes)
  {
    pp_literals (literals, literals_count);
  }
#endif

  bytecode_data.literals_count = literals_count;
  bytecode_data.literals = literals;
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
      opm.lit_id [i] = NOT_A_LITERAL;
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
  bytecode_data.literals = NULL;
  bytecode_data.opcodes = NULL;
  bytecode_data.lit_id_hash = null_hash;
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
  if (bytecode_data.lit_id_hash != null_hash)
  {
    lit_id_hash_table_free (bytecode_data.lit_id_hash);
  }
  if (bytecode_data.literals != NULL)
  {
    mem_heap_free_block ((uint8_t *) bytecode_data.literals);
  }
  mem_heap_free_block ((uint8_t *) bytecode_data.opcodes);
}
