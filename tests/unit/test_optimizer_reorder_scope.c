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
  // Honestly, after RETVAL there must be RET
  OPCODE test_program[] = {
    [0] = getop_reg_var_decl (5, 5), // tmp6
    [1] = getop_assignment (0, OPCODE_ARG_TYPE_STRING, 1), // a = "b"
    [2] = getop_var_decl (1), // var b
    [3] = getop_func_decl_0 (2), // function c() 
    [4] = getop_jmp_down (3), // {
    [5] = getop_var_decl (1), // var b
    [6] = getop_retval (1),  // return b; }
    [7] = getop_assignment (5, OPCODE_ARG_TYPE_STRING, 3), // "use strict"
    [8] = getop_exitval (0) 
  };

  mem_init();

  const char *strings[] = { "a", "b", "c", "use strict" };
  int nums [] = { 2 };
  serializer_init (true);
  uint16_t offset = serializer_dump_strings (strings, 4);
  serializer_dump_nums (nums, 1, offset, 4);

  for (int i = 0; i < 9; i++)
    serializer_dump_opcode (test_program[i]);

  OPCODE * opcodes = (OPCODE *) deserialize_bytecode ();

  optimizer_reorder_scope (1, 8);
  if (!opcodes_equal (opcodes, (OPCODE[]) {
    [0] = getop_reg_var_decl (5, 5), // tmp6
    [1] = getop_assignment (5, OPCODE_ARG_TYPE_STRING, 3), // "use strict"
    [2] = getop_func_decl_0 (2), // function c() 
    [3] = getop_jmp_down (3), // {
    [4] = getop_var_decl (1), // var b
    [5] = getop_retval (1),  // return b; }
    [6] = getop_var_decl (1), // var b
    [7] = getop_assignment (0, OPCODE_ARG_TYPE_STRING, 1), // a = "b"
    [8] = getop_exitval (0) 
  }, 9))
    return 1;


  return 0;
} 
