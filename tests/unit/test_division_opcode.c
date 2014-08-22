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
#include "opcodes-support.h"
#include "serializer.h"

/**
 * Unit test's main function.
 */
int
main( int __unused argc,
      char __unused **argv)
{
  const OPCODE test_program[] = {
    getop_reg_var_decl( 255, 255),
    getop_var_decl( 0),
    getop_var_decl( 1),
    getop_assignment( 0, OPCODE_ARG_TYPE_SMALLINT, 253),
    getop_assignment( 1, OPCODE_ARG_TYPE_NUMBER, 2),
    getop_division( 0, 0, 1),
    getop_exitval( 0)
  };

  mem_init();

  const char *strings[] = { "a",
                            "b" };
  ecma_number_t nums [] = { 2 };
  uint16_t offset = serializer_dump_strings( strings, 2);
  serializer_dump_nums( nums, 1, offset, 2);

  init_int( test_program);

  return run_int() ? 0
                   : 1;
} /* main */
