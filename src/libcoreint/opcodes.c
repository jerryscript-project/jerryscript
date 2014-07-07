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

#include "error.h"
#include "opcodes.h"
#include "interpreter.h"

void
save_op_data (OPCODE opdata)
{
  __program[__int_data.pos++] = opdata;
}

void opfunc_loop_init_num (OPCODE opdata) { unreachable (); }
void opfunc_loop_precond_begin_num (OPCODE opdata) { unreachable (); }
void opfunc_loop_precond_end_num (OPCODE opdata) { unreachable (); }
void opfunc_loop_postcond (OPCODE opdata) { unreachable (); }
void opfunc_call_2 (OPCODE opdata) { unreachable (); }
void opfunc_call_n (OPCODE opdata) { unreachable (); }
void opfunc_func_decl_1 (OPCODE opdata) { unreachable (); }
void opfunc_func_decl_2 (OPCODE opdata) { unreachable (); }
void opfunc_func_decl_n (OPCODE opdata) { unreachable (); }
void opfunc_varg_1 (OPCODE opdata) { unreachable (); }
void opfunc_varg_1_end (OPCODE opdata) { unreachable (); }
void opfunc_varg_2 (OPCODE opdata) { unreachable (); }
void opfunc_varg_2_end (OPCODE opdata) { unreachable (); }
void opfunc_varg_3 (OPCODE opdata) { unreachable (); }
void opfunc_varg_3_end (OPCODE opdata) { unreachable (); }
void opfunc_retval (OPCODE opdata) { unreachable (); }
void opfunc_assignment (OPCODE opdata) { unreachable (); }
void opfunc_assignment_multiplication (OPCODE opdata) { unreachable (); }
void opfunc_assignment_devision (OPCODE opdata) { unreachable (); }
void opfunc_assignment_remainder (OPCODE opdata) { unreachable (); }
void opfunc_assignment_addition (OPCODE opdata) { unreachable (); }
void opfunc_assignment_substruction (OPCODE opdata) { unreachable (); }
void opfunc_assignment_shift_left (OPCODE opdata) { unreachable (); }
void opfunc_assignment_shift_right (OPCODE opdata) { unreachable (); }
void opfunc_assignment_shift_uright (OPCODE opdata) { unreachable (); }
void opfunc_assignment_b_and (OPCODE opdata) { unreachable (); }
void opfunc_assignment_b_xor (OPCODE opdata) { unreachable (); }
void opfunc_assignment_b_or (OPCODE opdata) { unreachable (); }
void opfunc_logical_and (OPCODE opdata) { unreachable (); }
void opfunc_logical_or (OPCODE opdata) { unreachable (); }
void opfunc_b_and (OPCODE opdata) { unreachable (); }
void opfunc_b_or (OPCODE opdata) { unreachable (); }
void opfunc_b_xor (OPCODE opdata) { unreachable (); }
void opfunc_b_shift_left (OPCODE opdata) { unreachable (); }
void opfunc_b_shift_right (OPCODE opdata) { unreachable (); }
void opfunc_b_shift_uright (OPCODE opdata) { unreachable (); }
void opfunc_addition (OPCODE opdata) { unreachable (); }
void opfunc_substraction (OPCODE opdata) { unreachable (); }
void opfunc_division (OPCODE opdata) { unreachable (); }
void opfunc_multiplication (OPCODE opdata) { unreachable (); }
void opfunc_remainder (OPCODE opdata) { unreachable (); }
void opfunc_jmp_up (OPCODE opdata) { unreachable (); }
void opfunc_jmp_down (OPCODE opdata) { unreachable (); }
void opfunc_is_true_jmp (OPCODE opdata) { unreachable (); }
void opfunc_is_false_jmp (OPCODE opdata) { unreachable (); }
void opfunc_is_less_than (OPCODE opdata) { unreachable (); }
void opfunc_is_less_or_equal (OPCODE opdata) { unreachable (); }
void opfunc_is_greater_than (OPCODE opdata) { unreachable (); }
void opfunc_is_greater_or_equal (OPCODE opdata) { unreachable (); }
void opfunc_is_equal_value (OPCODE opdata) { unreachable (); }
void opfunc_is_not_equal_value (OPCODE opdata) { unreachable (); }
void opfunc_is_equal_value_type (OPCODE opdata) { unreachable (); }
void opfunc_is_not_equal_value_type (OPCODE opdata) { unreachable (); }

void
opfunc_loop_inf (OPCODE opdata)
{
#ifdef __HOST
  printf ("%d::loop_inf:idx:%d\n",
          __int_data.pos,
          opdata.data.loop_inf.loop_root);
#endif

  __int_data.pos = opdata.data.loop_inf.loop_root;
}

void
opfunc_call_1 (OPCODE opdata)
{
#ifdef __HOST
  printf ("%d::op_call_1:idx:%d:%d\n",
          __int_data.pos,
          opdata.data.call_1.name_lit_idx,
          opdata.data.call_1.arg1_lit_idx);
#endif

  __int_data.pos++;
}

void
opfunc_jmp (OPCODE opdata)
{
#ifdef __HOST
  printf ("%d::op_jmp:idx:%d\n",
          __int_data.pos,
          opdata.data.jmp.opcode_idx);
#endif

  __int_data.pos = opdata.data.jmp.opcode_idx;
}

OPCODE
getop_jmp (T_IDX arg1)
{
  OPCODE opdata;

  opdata.op_idx = jmp;
  opdata.data.jmp.opcode_idx = arg1;

  return opdata;
}

OPCODE
getop_call_1 (T_IDX arg1, T_IDX arg2)
{
  OPCODE opdata;

  opdata.op_idx = call_1;
  opdata.data.call_1.name_lit_idx = arg1;
  opdata.data.call_1.arg1_lit_idx = arg2;

  return opdata;
}

OPCODE
getop_loop_inf (T_IDX arg1)
{
  OPCODE opdata;

  opdata.op_idx = loop_inf;
  opdata.data.loop_inf.loop_root = arg1;

  return opdata;
}
