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

/**
 * Unit test's main function.
 */
int
main( int __unused argc,
      char __unused **argv)
{
  const OPCODE test_program[] = {
    /*  0: */ getop_var_decl( 0),
    /*  1: */ getop_var_decl( 1),
    /*  2: */ getop_assignment( 0, OPCODE_ARG_TYPE_STRING, 1),
    /*  3: */ getop_assignment( 1, OPCODE_ARG_TYPE_VARIABLE, 0),
    /*  4: */ getop_is_true_jmp( 1, 6),
    /*  5: */ getop_jmp_down( 5),
    /*  6: */ getop_assignment( 0, OPCODE_ARG_TYPE_SMALLINT, 253),
    /*  7: */ getop_assignment( 1, OPCODE_ARG_TYPE_NUMBER, 2),
    /*  8: */ getop_is_false_jmp( 1, 10),
    /*  9: */ getop_exitval( 0),
    /* 10: */ getop_exitval( 1)
  };

  mem_init();

  init_int( test_program);

  return run_int() ? 0
                   : 1;
} /* main */
