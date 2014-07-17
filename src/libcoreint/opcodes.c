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

#include "ecma-helpers.h"
#include "ecma-operations.h"
#include "interpreter.h"
#include "jerry-libc.h"
#include "opcodes.h"

#define OP_UNIMPLEMENTED_LIST(op) \
    op(is_true_jmp)                     \
    op(is_false_jmp)                    \
    op(loop_init_num)                   \
    op(loop_precond_begin_num)          \
    op(loop_precond_end_num)            \
    op(loop_postcond)                   \
    op(call_2)                          \
    op(call_n)                          \
    op(func_decl_1)                     \
    op(func_decl_2)                     \
    op(func_decl_n)                     \
    op(varg_1)                          \
    op(varg_1_end)                      \
    op(varg_2)                          \
    op(varg_2_end)                      \
    op(varg_3)                          \
    op(varg_3_end)                      \
    op(retval)                          \
    op(ret)                             \
    op(assignment)                      \
    op(b_shift_left)                    \
    op(b_shift_right)                   \
    op(b_shift_uright)                  \
    op(b_and)                           \
    op(b_or)                            \
    op(b_xor)                           \
    op(logical_and)                     \
    op(logical_or)                      \
    op(equal_value)                     \
    op(not_equal_value)                 \
    op(equal_value_type)                \
    op(not_equal_value_type)            \
    op(less_than)                       \
    op(greater_than)                    \
    op(less_or_equal_than)              \
    op(greater_or_equal_than)           \
    op(addition)                        \
    op(substraction)                    \
    op(division)                        \
    op(multiplication)                  \
    op(remainder)                       \
    op(jmp_up)                          \
    op(jmp_down)                        \
    op(nop)                             \
    op(var_decl)

#define DEFINE_UNIMPLEMENTED_OP(op) \
  ecma_CompletionValue_t opfunc_ ## op(OPCODE opdata, struct __int_data *int_data) { \
    JERRY_UNIMPLEMENTED_REF_UNUSED_VARS( opdata, int_data); \
  }
OP_UNIMPLEMENTED_LIST(DEFINE_UNIMPLEMENTED_OP)
#undef DEFINE_UNIMPLEMENTED_OP

ecma_CompletionValue_t
opfunc_loop_inf (OPCODE opdata, struct __int_data *int_data)
{
#ifdef __HOST
  __printf ("%d::loop_inf:idx:%d\n",
          int_data->pos,
          opdata.data.loop_inf.loop_root);
#endif

  int_data->pos = opdata.data.loop_inf.loop_root;

  return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                   ecma_MakeSimpleValue( ECMA_SIMPLE_VALUE_EMPTY),
                                   ECMA_TARGET_ID_RESERVED);
}

ecma_CompletionValue_t
opfunc_call_1 (OPCODE opdata __unused, struct __int_data *int_data)
{
#ifdef __HOST
  __printf ("%d::op_call_1:idx:%d:%d\n",
          int_data->pos,
          opdata.data.call_1.name_lit_idx,
          opdata.data.call_1.arg1_lit_idx);
#endif

  int_data->pos++;

  // FIXME
  return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                   ecma_MakeSimpleValue( ECMA_SIMPLE_VALUE_EMPTY),
                                   ECMA_TARGET_ID_RESERVED);
}

ecma_CompletionValue_t
opfunc_jmp (OPCODE opdata, struct __int_data *int_data)
{
#ifdef __HOST
  __printf ("%d::op_jmp:idx:%d\n",
          int_data->pos,
          opdata.data.jmp.opcode_idx);
#endif

  int_data->pos = opdata.data.jmp.opcode_idx;

  return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                   ecma_MakeSimpleValue( ECMA_SIMPLE_VALUE_EMPTY),
                                   ECMA_TARGET_ID_RESERVED);
}

/** Opcode generators.  */
GETOP_IMPL_2 (is_true_jmp, value, opcode)
GETOP_IMPL_2 (is_false_jmp, value, opcode)
GETOP_IMPL_1 (jmp, opcode_idx)
GETOP_IMPL_1 (jmp_up, opcode_count)
GETOP_IMPL_1 (jmp_down, opcode_count)
GETOP_IMPL_3 (addition, dst, var_left, var_right)
GETOP_IMPL_3 (substraction, dst, var_left, var_right)
GETOP_IMPL_3 (division, dst, var_left, var_right)
GETOP_IMPL_3 (multiplication, dst, var_left, var_right)
GETOP_IMPL_3 (remainder, dst, var_left, var_right)
GETOP_IMPL_3 (b_shift_left, dst, var_left, var_right)
GETOP_IMPL_3 (b_shift_right, dst, var_left, var_right)
GETOP_IMPL_3 (b_shift_uright, dst, var_left, var_right)
GETOP_IMPL_3 (b_and, dst, var_left, var_right)
GETOP_IMPL_3 (b_or, dst, var_left, var_right)
GETOP_IMPL_3 (b_xor, dst, var_left, var_right)
GETOP_IMPL_3 (logical_and, dst, var_left, var_right)
GETOP_IMPL_3 (logical_or, dst, var_left, var_right)
GETOP_IMPL_3 (equal_value, dst, var_left, var_right)
GETOP_IMPL_3 (not_equal_value, dst, var_left, var_right)
GETOP_IMPL_3 (equal_value_type, dst, var_left, var_right)
GETOP_IMPL_3 (not_equal_value_type, dst, var_left, var_right)
GETOP_IMPL_3 (less_than, dst, var_left, var_right)
GETOP_IMPL_3 (greater_than, dst, var_left, var_right)
GETOP_IMPL_3 (less_or_equal_than, dst, var_left, var_right)
GETOP_IMPL_3 (greater_or_equal_than, dst, var_left, var_right)
GETOP_IMPL_2 (assignment, value_left, value_right)
GETOP_IMPL_2 (call_1, name_lit_idx, arg1_lit_idx)
GETOP_IMPL_3 (call_2, name_lit_idx, arg1_lit_idx, arg2_lit_idx)
GETOP_IMPL_3 (call_n, name_lit_idx, arg1_lit_idx, arg2_lit_idx)
GETOP_IMPL_2 (func_decl_1, name_lit_idx, arg1_lit_idx)
GETOP_IMPL_3 (func_decl_2, name_lit_idx, arg1_lit_idx, arg2_lit_idx)
GETOP_IMPL_3 (func_decl_n, name_lit_idx, arg1_lit_idx, arg2_lit_idx)
GETOP_IMPL_1 (varg_1, arg1_lit_idx)
GETOP_IMPL_1 (varg_1_end, arg1_lit_idx)
GETOP_IMPL_2 (varg_2, arg1_lit_idx, arg2_lit_idx)
GETOP_IMPL_2 (varg_2_end, arg1_lit_idx, arg2_lit_idx)
GETOP_IMPL_3 (varg_3, arg1_lit_idx, arg2_lit_idx, arg3_lit_idx)
GETOP_IMPL_3 (varg_3_end, arg1_lit_idx, arg2_lit_idx, arg3_lit_idx)
GETOP_IMPL_1 (retval, ret_value)
GETOP_IMPL_0 (ret)
GETOP_IMPL_0 (nop)
GETOP_IMPL_1 (loop_inf, loop_root)
GETOP_IMPL_3 (loop_init_num, start, stop, step)
GETOP_IMPL_2 (loop_precond_begin_num, condition, after_loop_op)
GETOP_IMPL_3 (loop_precond_end_num, iterator, step, precond_begin)
GETOP_IMPL_2 (loop_postcond, condition, body_root)
GETOP_IMPL_1 (var_decl, variable)

