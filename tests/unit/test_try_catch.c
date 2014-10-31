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
    [ 0] = getop_reg_var_decl (255, 255),
    [ 1] = getop_var_decl (0),
    [ 2] = getop_var_decl (1),
    [ 3] = getop_try (0, 5),
    [ 4] = getop_assignment (0, OPCODE_ARG_TYPE_STRING, 1),
    [ 5] = getop_assignment (1, OPCODE_ARG_TYPE_VARIABLE, 0),
    [ 6] = getop_throw (1),
    [ 7] = getop_assignment (1, OPCODE_ARG_TYPE_SMALLINT, 12),
    [ 8] = getop_meta (OPCODE_META_TYPE_CATCH, 0, 6),
    [ 9] = getop_meta (OPCODE_META_TYPE_CATCH_EXCEPTION_IDENTIFIER, 2, 255),
    [10] = getop_equal_value_type (0, 1, 2),
    [11] = getop_is_true_jmp_down (0, 0, 3),
    [12] = getop_exitval (1),
    [13] = getop_assignment (0, OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_FALSE),
    [14] = getop_meta (OPCODE_META_TYPE_FINALLY, 0, 4),
    [15] = getop_is_false_jmp_down (0, 0, 2),
    [16] = getop_exitval (0),
    [17] = getop_exitval (1),
    [18] = getop_meta (OPCODE_META_TYPE_END_TRY_CATCH_FINALLY, 255, 255),
    [19] = getop_exitval (1)
  };

  mem_init();
  serializer_init (false);

  const literal lits[] = { LP("a"), 
                           LP("b"),
                           LP("c"),
                           NUM(2.0) };
  serializer_dump_literals (lits, 4);

  init_int( test_program, false);

  bool is_ok = run_int();
 
  serializer_free ();
  mem_finalize (false);

  return (is_ok ? 0 : 1);
} /* main */
