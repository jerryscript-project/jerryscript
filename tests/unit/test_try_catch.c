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

/**
 * Unit test's main function.
 */
int
main( int __unused argc,
      char __unused **argv)
{
  const opcode_t test_program[] = {
    [ 0] = getop_reg_var_decl (255, 255),
    [ 1] = getop_var_decl (0),
    [ 2] = getop_var_decl (1),
    [ 3] = getop_try (0, 8),
    [ 4] = getop_assignment (0, OPCODE_ARG_TYPE_STRING, 1),
    [ 5] = getop_assignment (1, OPCODE_ARG_TYPE_VARIABLE, 0),
    [ 6] = getop_throw (1),
    [ 7] = getop_assignment (1, OPCODE_ARG_TYPE_SMALLINT, 12),
    [ 8] = getop_meta (OPCODE_META_TYPE_CATCH, 0, 14),
    [ 9] = getop_meta (OPCODE_META_TYPE_CATCH_EXCEPTION_IDENTIFIER, 2, 255),
    [10] = getop_equal_value_type (0, 1, 2),
    [11] = getop_is_true_jmp (0, 14),
    [12] = getop_exitval (1),
    [13] = getop_assignment (0, OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_FALSE),
    [14] = getop_meta (OPCODE_META_TYPE_FINALLY, 0, 18),
    [15] = getop_is_false_jmp (0, 17),
    [16] = getop_exitval (0),
    [17] = getop_exitval (1),
    [18] = getop_meta (OPCODE_META_TYPE_END_TRY_CATCH_FINALLY, 255, 255),
    [19] = getop_exitval (1)
  };

  mem_init();

  const char *strings[] = { "a",
                            "b",
                            "c" };
  ecma_number_t nums [] = { 2.0 };
  uint16_t offset = serializer_dump_strings( strings, 3);
  serializer_dump_nums( nums, 1, offset, 3);

  init_int( test_program);

  bool is_ok = run_int();
 
  serializer_free ();
  mem_finalize (false);

  return (is_ok ? 0 : 1);
} /* main */
