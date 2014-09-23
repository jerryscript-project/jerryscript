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
#include "common.h"

/**
 * Unit test's main function.
 */
int
main( int __unused argc,
      char __unused **argv)
{
  const opcode_t test_program[] = {
    [0] = getop_reg_var_decl( 255, 255),
    [1] = getop_var_decl( 0),
    [2] = getop_is_true_jmp_down( 0, 0, 2),
    [3] = getop_exitval( 0),
    [4] = getop_exitval( 1)
  };

  mem_init();
  serializer_init (false);

  const lp_string strings[] = { LP("a"), 
                                LP("b") };
  ecma_number_t nums [] = { 2.0 };
  serializer_dump_strings_and_nums (strings, 2, nums, 1);

  init_int( test_program, false);

  bool status = run_int();

  serializer_free ();
  mem_finalize (false);

  return (status ? 0 : 1);
} /* main */
