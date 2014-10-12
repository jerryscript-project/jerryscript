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

    /* b = 'property1'; */
    [ 5] = getop_assignment (1, OPCODE_ARG_TYPE_STRING, 2),

    /* a = { */
    [ 6] = getop_obj_decl (0, 4),

    /* 'property1' : 'value1', */
    [ 7] = getop_assignment (240, OPCODE_ARG_TYPE_STRING, 5),
    [ 8] = getop_meta (OPCODE_META_TYPE_VARG_PROP_DATA, 2, 240),

    /* get property2 () { return 1; }, */
    [ 9] = getop_func_expr_n (240, 243 /* any tmp reg */, 0),
    [10] = getop_meta (OPCODE_META_TYPE_FUNCTION_END, 0, 4),
    [11] = getop_reg_var_decl (250, 255),
    [12] = getop_assignment (250, OPCODE_ARG_TYPE_SMALLINT, 1),
    [13] = getop_retval (250),
    [14] = getop_meta (OPCODE_META_TYPE_VARG_PROP_GETTER, 3, 240),

    /* set property2 (a) { this.property3 = a * 10; }, */
    [15] = getop_func_expr_n (250, 243 /* any tmp reg */, 1),
    [16] = getop_meta (OPCODE_META_TYPE_VARG, 0, 255),
    [17] = getop_meta (OPCODE_META_TYPE_FUNCTION_END, 0, 8),
    [18] = getop_reg_var_decl (250, 255),
    [19] = getop_this (250),
    [20] = getop_assignment (251, OPCODE_ARG_TYPE_STRING, 4),
    [21] = getop_assignment (252, OPCODE_ARG_TYPE_SMALLINT, 10),
    [22] = getop_multiplication (252, 0, 252),
    [23] = getop_prop_setter (250, 251, 252),
    [24] = getop_ret (),
    [25] = getop_meta (OPCODE_META_TYPE_VARG_PROP_SETTER, 3, 250),

    /* set property3 (b) { this.property1 = b; } }; */
    [26] = getop_func_expr_n (250, 243 /* any tmp reg */, 1),
    [27] = getop_meta (OPCODE_META_TYPE_VARG, 1, 255),
    [28] = getop_meta (OPCODE_META_TYPE_FUNCTION_END, 0, 6),
    [29] = getop_reg_var_decl (250, 255),
    [30] = getop_this (250),
    [31] = getop_assignment (251, OPCODE_ARG_TYPE_STRING, 2),
    [32] = getop_prop_setter (250, 251, 1),
    [33] = getop_ret (),
    [34] = getop_meta (OPCODE_META_TYPE_VARG_PROP_SETTER, 4, 250),

    /* assert (a.property1 === 'value1'); */
    [35] = getop_assignment (240, OPCODE_ARG_TYPE_STRING, 2),
    [36] = getop_prop_getter (240, 0, 240),
    [37] = getop_assignment (241, OPCODE_ARG_TYPE_STRING, 5),
    [38] = getop_equal_value_type (240, 240, 241),
    [39] = getop_is_false_jmp_up (240, 0, 37),

    /* assert (a.property2 === 1); */
    [40] = getop_assignment (240, OPCODE_ARG_TYPE_STRING, 3),
    [41] = getop_prop_getter (240, 0, 240),
    [42] = getop_assignment (241, OPCODE_ARG_TYPE_SMALLINT, 1),
    [43] = getop_equal_value_type (240, 240, 241),
    [44] = getop_is_false_jmp_up (240, 0, 42),

    /* a.property3 = 'value2'; */
    [45] = getop_assignment (240, OPCODE_ARG_TYPE_STRING, 4),
    [46] = getop_assignment (241, OPCODE_ARG_TYPE_STRING, 6),
    [47] = getop_prop_setter (0, 240, 241),

    /* assert (a.property1 === 'value2'); */
    [48] = getop_assignment (240, OPCODE_ARG_TYPE_STRING, 2),
    [49] = getop_prop_getter (240, 0, 240),
    [50] = getop_assignment (241, OPCODE_ARG_TYPE_STRING, 6),
    [51] = getop_equal_value_type (240, 240, 241),
    [52] = getop_is_false_jmp_up (240, 0, 50),

    /* a.property2 = 2.5; */
    [53] = getop_assignment (240, OPCODE_ARG_TYPE_STRING, 3),
    [54] = getop_assignment (241, OPCODE_ARG_TYPE_NUMBER, 7),
    [55] = getop_prop_setter (0, 240, 241),

    /* assert (a.property1 === 25); */
    [56] = getop_assignment (240, OPCODE_ARG_TYPE_STRING, 2),
    [57] = getop_prop_getter (240, 0, 240),
    [58] = getop_assignment (241, OPCODE_ARG_TYPE_SMALLINT, 25),
    [59] = getop_equal_value_type (240, 240, 241),
    [60] = getop_is_false_jmp_up (240, 0, 58),

    /* b = delete a[b]; */
    [61] = getop_delete_prop (1, 0, 1),

    /* assert (b === true); */
    [62] = getop_assignment (240, OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_TRUE),
    [63] = getop_equal_value_type (240, 240, 1),
    [64] = getop_is_false_jmp_up (240, 0, 62),

    /* assert (a.property1 === undefined); */
    [65] = getop_assignment (240, OPCODE_ARG_TYPE_STRING, 2),
    [66] = getop_prop_getter (240, 0, 240),
    [67] = getop_assignment (241, OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_UNDEFINED),
    [68] = getop_equal_value_type (240, 240, 241),
    [69] = getop_is_false_jmp_up (240, 0, 67),

    [70] = getop_exitval (0)
  };

  mem_init();
  serializer_init (false);

  const lp_string strings[] = { LP("a"),
                                LP("b"),
                                LP("property1"),
                                LP("property2"),
                                LP("property3"),
                                LP("value1"),
                                LP("value2") };
  ecma_number_t nums [] = { 2.5 };
  serializer_dump_strings_and_nums (strings, 7, nums, 1);

  init_int( test_program, false);

  bool status = run_int();

  serializer_free ();
  mem_finalize (false);

  return (status ? 0 : 1);
} /* main */
