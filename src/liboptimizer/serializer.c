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

#include "serializer.h"
#include "parser.h"
#include "jerry-libc.h"
#include "bytecode-data.h"
#include "deserializer.h"
#include "pretty-printer.h"

static bool print_opcodes;

void
serializer_dump_strings_and_nums (const lp_string strings[], uint8_t strs_count,
                                  const ecma_number_t nums[], uint8_t nums_count)
{
  if (print_opcodes)
  {
    pp_strings (strings, strs_count);
  }

  bytecode_data.strs_count = strs_count;
  bytecode_data.nums_count = nums_count;
  bytecode_data.strings = strings;
  bytecode_data.nums = nums;
}

void
serializer_dump_opcode (opcode_t opcode)
{
  JERRY_ASSERT (STACK_SIZE (bytecode_opcodes) < MAX_OPCODES);

  if (print_opcodes)
  {
    pp_opcode (STACK_SIZE (bytecode_opcodes), opcode, false);
  }

  STACK_PUSH (bytecode_opcodes, opcode);
}

void
serializer_set_writing_position (opcode_counter_t oc)
{
  JERRY_ASSERT (oc < STACK_SIZE (bytecode_opcodes));
  STACK_DROP (bytecode_opcodes, STACK_SIZE (bytecode_opcodes) - oc);
}

void
serializer_rewrite_opcode (const opcode_counter_t loc, opcode_t opcode)
{
  JERRY_ASSERT (loc < STACK_SIZE (bytecode_opcodes));
  STACK_SET_ELEMENT (bytecode_opcodes, loc, opcode);

  if (print_opcodes)
  {
    pp_opcode (loc, opcode, true);
  }
}

void
serializer_print_opcodes (void)
{
  opcode_counter_t loc;

  if (!print_opcodes)
  {
    return;
  }

  __printf ("AFTER OPTIMIZER:\n");

  for (loc = 0; loc < STACK_SIZE (bytecode_opcodes); loc++)
  {
    pp_opcode (loc, STACK_ELEMENT (bytecode_opcodes, loc), false);
  }
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
  print_opcodes = show_opcodes;
}

void
serializer_free (void)
{
}
