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

#include "globals.h"
#include "serializer.h"
#include "parser.h"
#include "jerry-libc.h"
#include "bytecode-data.h"
#include "deserializer.h"
#include "pretty-printer.h"

static bool print_opcodes;

void
serializer_set_scope (scopes_tree new_scope)
{
  current_scope = new_scope;
}

void
serializer_merge_scopes_into_bytecode (void)
{
  bytecode_data.opcodes = scopes_tree_raw_data (current_scope, &bytecode_data.opcodes_count);
}


void
serializer_dump_strings_and_nums (const lp_string strings[], uint8_t strs_count,
                                  const ecma_number_t nums[], uint8_t nums_count)
{
#ifdef JERRY_ENABLE_PP
  if (print_opcodes)
  {
    pp_strings (strings, strs_count);
    pp_nums (nums, nums_count, strs_count);
  }
#endif

  bytecode_data.strs_count = strs_count;
  bytecode_data.nums_count = nums_count;
  bytecode_data.strings = strings;
  bytecode_data.nums = nums;
}

void
serializer_dump_opcode (opcode_t opcode)
{
  JERRY_ASSERT (scopes_tree_opcodes_num (current_scope) < MAX_OPCODES);

#ifdef JERRY_ENABLE_PP
  if (print_opcodes)
  {
    pp_opcode (scopes_tree_opcodes_num (current_scope), opcode, false);
  }
#endif

  scopes_tree_add_opcode (current_scope, opcode);
}

void
serializer_set_writing_position (opcode_counter_t oc)
{
  scopes_tree_set_opcodes_num (current_scope, oc);
}

void
serializer_rewrite_opcode (const opcode_counter_t loc, opcode_t opcode)
{
  scopes_tree_set_opcode (current_scope, loc, opcode);

#ifdef JERRY_ENABLE_PP
  if (print_opcodes)
  {
    pp_opcode (loc, opcode, true);
  }
#endif
}

void
serializer_print_opcodes (void)
{
#ifdef JERRY_ENABLE_PP
  opcode_counter_t loc;

  if (!print_opcodes)
  {
    return;
  }

  __printf ("AFTER OPTIMIZER:\n");

  for (loc = 0; loc < bytecode_data.opcodes_count; loc++)
  {
    pp_opcode (loc, bytecode_data.opcodes[loc], false);
  }
#endif
}

/* Make lp_strings also zero-terminated.  */
void
serializer_adjust_strings (void)
{
  for (uint8_t i = 0; i < bytecode_data.strs_count; ++i)
  {
    ecma_length_t len = bytecode_data.strings[i].length;
    ecma_char_t *str = bytecode_data.strings[i].str;
    str[len] = '\0';
  }
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
