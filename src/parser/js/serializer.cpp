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

#include "jrt.h"
#include "serializer.h"
#include "parser.h"
#include "jerry-libc.h"
#include "bytecode-data.h"
#include "deserializer.h"
#include "pretty-printer.h"

bytecode_data_t bytecode_data;
scopes_tree current_scope;

static bool print_opcodes;

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
serializer_dump_literals (const literal literals[], literal_index_t literals_count)
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
serializer_init (bool show_opcodes)
{
  current_scope = NULL;
  print_opcodes = show_opcodes;
}

void
serializer_free (void)
{
}
