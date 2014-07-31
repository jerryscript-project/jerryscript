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
#include "interpreter.h"
#include "mem-allocator.h"
#include "opcodes.h"
#include "serializer.h"
#include "optimizer-passes.h"
#include "jerry-libc.h"
#include "deserializer.h"
#include "common.h"

/**
 * Unit test's main function.
 */
int
main( int __unused argc,
      char __unused **argv)
{
  OPCODE test_program[] = {
    [0] = getop_assignment( 0, OPCODE_ARG_TYPE_STRING, 1),
    [1] = getop_assignment( 1, OPCODE_ARG_TYPE_VARIABLE, 0),
    [2] = getop_is_false_jmp (0, 10),
    [3] = getop_is_true_jmp (0, 6),
    [4] = getop_jmp_up (1),
    [5] = getop_jmp_up (4),
    [6] = getop_jmp_down (1),
    [7] = getop_jmp_down (2),
    [8] = getop_jmp_down (2),
    [9] = getop_assignment (0, OPCODE_ARG_TYPE_SMALLINT, 253),
    [10] = getop_exitval (0)
  };

  mem_init();

  const char *strings[] = { "a",
                            "b" };
  int nums [] = { 2 };
  serializer_init (true);
  uint16_t offset = serializer_dump_strings (strings, 2);
  serializer_dump_nums (nums, 1, offset, 2);

  for (int i = 0; i < 11; i++)
    serializer_dump_opcode (test_program[i]);

  OPCODE * opcodes = (OPCODE *) deserialize_bytecode ();

  optimizer_move_opcodes (opcodes + 9, opcodes + 2, 1);
  if (!opcodes_equal (opcodes, (OPCODE[]) {
    [0] = getop_assignment( 0, OPCODE_ARG_TYPE_STRING, 1),
    [1] = getop_assignment( 1, OPCODE_ARG_TYPE_VARIABLE, 0),
    [2] = getop_assignment (0, OPCODE_ARG_TYPE_SMALLINT, 253),
    [3] = getop_is_false_jmp (0, 10),
    [4] = getop_is_true_jmp (0, 6),
    [5] = getop_jmp_up (1),
    [6] = getop_jmp_up (4),
    [7] = getop_jmp_down (1),
    [8] = getop_jmp_down (2),
    [9] = getop_jmp_down (2),
    [10] = getop_exitval (0)
  }, 11))
    return 1;

  optimizer_adjust_jumps (opcodes + 3, opcodes + 10, 1);
  if (!opcodes_equal (opcodes, (OPCODE[]) {
    [0] = getop_assignment( 0, OPCODE_ARG_TYPE_STRING, 1),
    [1] = getop_assignment( 1, OPCODE_ARG_TYPE_VARIABLE, 0),
    [2] = getop_assignment (0, OPCODE_ARG_TYPE_SMALLINT, 253),
    [3] = getop_is_false_jmp (0, 10),
    [4] = getop_is_true_jmp (0, 7),
    [5] = getop_jmp_up (1),
    [6] = getop_jmp_up (5),
    [7] = getop_jmp_down (1),
    [8] = getop_jmp_down (2),
    [9] = getop_jmp_down (1),
    [10] = getop_exitval (0)
  }, 11))
    return 1;

  return 0;
} 
