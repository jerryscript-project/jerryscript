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
    [ 0] = getop_reg_var_decl (240, 255),
    [ 1] = getop_jmp_down (0, 2),
    [ 2] = getop_exitval (1),

    /* var a, b; */
    [ 3] = getop_var_decl (0),
    [ 4] = getop_var_decl (1),

    /* b = null; */
    [ 5] = getop_assignment (1, OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_NULL),

    /* a = [12, 'length', b]; */
    [ 6] = getop_array_decl (0, 3),
    [ 7] = getop_assignment (240, OPCODE_ARG_TYPE_SMALLINT, 12),
    [ 8] = getop_meta (OPCODE_META_TYPE_VARG, 240, 255),
    [ 9] = getop_assignment (240, OPCODE_ARG_TYPE_STRING, 2),
    [10] = getop_meta (OPCODE_META_TYPE_VARG, 240, 255),
    [11] = getop_assignment (240, OPCODE_ARG_TYPE_VARIABLE, 1),
    [12] = getop_meta (OPCODE_META_TYPE_VARG, 240, 255),

    /* assert (a.length === 3); */
    [13] = getop_assignment (240, OPCODE_ARG_TYPE_STRING, 2),
    [14] = getop_prop_getter (240, 0, 240),
    [15] = getop_assignment (241, OPCODE_ARG_TYPE_SMALLINT, 3),
    [16] = getop_equal_value_type (240, 240, 241),
    [17] = getop_is_false_jmp_up (240, 0, 15),

    /* assert (a[0] === 12.0); */
    [18] = getop_assignment (240, OPCODE_ARG_TYPE_SMALLINT, 0),
    [19] = getop_prop_getter (240, 0, 240),
    [20] = getop_assignment (241, OPCODE_ARG_TYPE_NUMBER, 5),
    [21] = getop_equal_value_type (240, 240, 241),
    [22] = getop_is_false_jmp_up (240, 0, 20),

    /* assert (a['1'] === 'length'); */
    [23] = getop_assignment (240, OPCODE_ARG_TYPE_STRING, 3),
    [24] = getop_prop_getter (240, 0, 240),
    [25] = getop_assignment (241, OPCODE_ARG_TYPE_STRING, 2),
    [26] = getop_equal_value_type (240, 240, 241),
    [27] = getop_is_false_jmp_up (240, 0, 25),

    /* assert (a[2.0] === null); */
    [28] = getop_assignment (240, OPCODE_ARG_TYPE_NUMBER, 4),
    [29] = getop_prop_getter (240, 0, 240),
    [30] = getop_assignment (241, OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_NULL),
    [31] = getop_equal_value_type (240, 240, 241),
    [32] = getop_is_false_jmp_up (240, 0, 30),

    /* assert (a[2.5] === undefined); */
    [33] = getop_assignment (240, OPCODE_ARG_TYPE_NUMBER, 6),
    [34] = getop_prop_getter (240, 0, 240),
    [35] = getop_assignment (241, OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_UNDEFINED),
    [36] = getop_equal_value_type (240, 240, 241),
    [37] = getop_is_false_jmp_up (240, 0, 35),

    /* a.length = 1; */
    [38] = getop_assignment (240, OPCODE_ARG_TYPE_STRING, 2),
    [39] = getop_assignment (241, OPCODE_ARG_TYPE_SMALLINT, 1),
    [40] = getop_prop_setter (0, 240, 241),

    /* assert (a.length === 1); */
    [41] = getop_assignment (240, OPCODE_ARG_TYPE_STRING, 2),
    [42] = getop_prop_getter (240, 0, 240),
    [43] = getop_assignment (241, OPCODE_ARG_TYPE_SMALLINT, 1),
    [44] = getop_equal_value_type (240, 240, 241),
    [45] = getop_is_false_jmp_up (240, 0, 43),

     /* assert (a[0] === 12.0); */
    [46] = getop_assignment (240, OPCODE_ARG_TYPE_SMALLINT, 0),
    [47] = getop_prop_getter (240, 0, 240),
    [48] = getop_assignment (241, OPCODE_ARG_TYPE_NUMBER, 5),
    [49] = getop_equal_value_type (240, 240, 241),
    [50] = getop_is_false_jmp_up (240, 0, 48),

    /* assert (a['1'] === undefined); */
    [51] = getop_assignment (240, OPCODE_ARG_TYPE_STRING, 3),
    [52] = getop_prop_getter (240, 0, 240),
    [53] = getop_assignment (241, OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_UNDEFINED),
    [54] = getop_equal_value_type (240, 240, 241),
    [55] = getop_is_false_jmp_up (240, 0, 53),

    /* assert (a[2.0] === undefined); */
    [56] = getop_assignment (240, OPCODE_ARG_TYPE_NUMBER, 4),
    [57] = getop_prop_getter (240, 0, 240),
    [58] = getop_equal_value_type (240, 240, 241),
    [59] = getop_is_false_jmp_up (240, 0, 57),

    /* a.length = 8; */
    [60] = getop_assignment (240, OPCODE_ARG_TYPE_STRING, 2),
    [61] = getop_assignment (241, OPCODE_ARG_TYPE_SMALLINT, 8),
    [62] = getop_prop_setter (0, 240, 241),

    /* assert (a.length === 8); */
    [63] = getop_assignment (240, OPCODE_ARG_TYPE_STRING, 2),
    [64] = getop_prop_getter (240, 0, 240),
    [65] = getop_assignment (241, OPCODE_ARG_TYPE_SMALLINT, 8),
    [66] = getop_equal_value_type (240, 240, 241),
    [67] = getop_is_false_jmp_up (240, 0, 65),

    /* a[10] = true; */
    [68] = getop_assignment (240, OPCODE_ARG_TYPE_SMALLINT, 10),
    [69] = getop_assignment (241, OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_TRUE),
    [70] = getop_prop_setter (0, 240, 241),

    /* assert (a.length === 11); */
    [71] = getop_assignment (240, OPCODE_ARG_TYPE_STRING, 2),
    [72] = getop_prop_getter (240, 0, 240),
    [73] = getop_assignment (241, OPCODE_ARG_TYPE_SMALLINT, 11),
    [74] = getop_equal_value_type (240, 240, 241),
    [75] = getop_is_false_jmp_up (240, 0, 73),

     /* assert (a[0] === 12.0); */
    [76] = getop_assignment (240, OPCODE_ARG_TYPE_SMALLINT, 0),
    [77] = getop_prop_getter (240, 0, 240),
    [78] = getop_assignment (241, OPCODE_ARG_TYPE_NUMBER, 5),
    [79] = getop_equal_value_type (240, 240, 241),
    [80] = getop_is_false_jmp_up (240, 0, 78),

    /* assert (a['1'] === undefined); */
    [81] = getop_assignment (240, OPCODE_ARG_TYPE_STRING, 3),
    [82] = getop_prop_getter (240, 0, 240),
    [83] = getop_assignment (241, OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_UNDEFINED),
    [84] = getop_equal_value_type (240, 240, 241),
    [85] = getop_is_false_jmp_up (240, 0, 83),

    /* assert (a[2.0] === undefined); */
    [86] = getop_assignment (240, OPCODE_ARG_TYPE_NUMBER, 4),
    [87] = getop_prop_getter (240, 0, 240),
    [88] = getop_equal_value_type (240, 240, 241),
    [89] = getop_is_false_jmp_up (240, 0, 87),

    /* assert (a[17] === true); */
    [90] = getop_assignment (240, OPCODE_ARG_TYPE_SMALLINT, 10),
    [91] = getop_prop_getter (240, 0, 240),
    [92] = getop_assignment (241, OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_TRUE),
    [93] = getop_equal_value_type (240, 240, 241),
    [94] = getop_is_false_jmp_up (240, 0, 92),

    [95] = getop_exitval (0),
  };

  mem_init();
  serializer_init (false);

  const literal literals[] = { LP("a"),
                               LP("b"),
                               LP("length"),
                               LP("1"),
                               NUM(2.0),
                               NUM(12.0),
                               NUM(2.5) };
  serializer_dump_literals (literals, 7);

  init_int( test_program, false);

  bool status = run_int();

  serializer_free ();
  mem_finalize (false);

  return (status ? 0 : 1);
} /* main */
