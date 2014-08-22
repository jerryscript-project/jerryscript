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

#ifndef OPCODES_SUPPORT_H
#define OPCODES_SUPPORT_H

#include "opcodes.h"

#define GETOP_DECL_0(name) \
        __opcode getop_##name (void);

#define GETOP_DECL_1(name, field1) \
        __opcode getop_##name (T_IDX);

#define GETOP_DECL_2(name, field1, field2) \
        __opcode getop_##name (T_IDX, T_IDX);

#define GETOP_DECL_3(name, field1, field2, field3) \
        __opcode getop_##name (T_IDX, T_IDX, T_IDX);

GETOP_DECL_3 (construct_decl, lhs, name_lit_idx, arg_list)
GETOP_DECL_1 (func_decl_0, name_lit_idx)
GETOP_DECL_2 (array_decl, lhs, list)
GETOP_DECL_3 (prop, lhs, name, value)
GETOP_DECL_3 (prop_getter, lhs, obj, prop)
GETOP_DECL_3 (prop_setter, obj, prop, rhs)
GETOP_DECL_2 (prop_get_decl, lhs, prop)
GETOP_DECL_3 (prop_set_decl, lhs, prop, arg)
GETOP_DECL_2 (obj_decl, lhs, list)
GETOP_DECL_1 (this, lhs)
GETOP_DECL_2 (delete, lhs, obj)
GETOP_DECL_2 (typeof, lhs, obj)
GETOP_DECL_1 (with, expr)
GETOP_DECL_0 (end_with)
GETOP_DECL_1 (var_decl, variable_name)
GETOP_DECL_2 (reg_var_decl, min, max)
GETOP_DECL_3 (meta, type, data_1, data_2)
GETOP_DECL_2 (is_true_jmp, value, opcode)
GETOP_DECL_2 (is_false_jmp, value, opcode)
GETOP_DECL_1 (jmp, opcode_idx)
GETOP_DECL_1 (jmp_up, opcode_count)
GETOP_DECL_1 (jmp_down, opcode_count)
GETOP_DECL_3 (addition, dst, var_left, var_right)
GETOP_DECL_2 (post_incr, dst, var_right)
GETOP_DECL_2 (post_decr, dst, var_right)
GETOP_DECL_2 (pre_incr, dst, var_right)
GETOP_DECL_2 (pre_decr, dst, var_right)
GETOP_DECL_3 (substraction, dst, var_left, var_right)
GETOP_DECL_3 (division, dst, var_left, var_right)
GETOP_DECL_3 (multiplication, dst, var_left, var_right)
GETOP_DECL_3 (remainder, dst, var_left, var_right)
GETOP_DECL_3 (b_shift_left, dst, var_left, var_right)
GETOP_DECL_3 (b_shift_right, dst, var_left, var_right)
GETOP_DECL_3 (b_shift_uright, dst, var_left, var_right)
GETOP_DECL_3 (b_and, dst, var_left, var_right)
GETOP_DECL_3 (b_or, dst, var_left, var_right)
GETOP_DECL_3 (b_xor, dst, var_left, var_right)
GETOP_DECL_3 (logical_and, dst, var_left, var_right)
GETOP_DECL_3 (logical_or, dst, var_left, var_right)
GETOP_DECL_3 (equal_value, dst, var_left, var_right)
GETOP_DECL_3 (not_equal_value, dst, var_left, var_right)
GETOP_DECL_3 (equal_value_type, dst, var_left, var_right)
GETOP_DECL_3 (not_equal_value_type, dst, var_left, var_right)
GETOP_DECL_3 (less_than, dst, var_left, var_right)
GETOP_DECL_3 (greater_than, dst, var_left, var_right)
GETOP_DECL_3 (less_or_equal_than, dst, var_left, var_right)
GETOP_DECL_3 (greater_or_equal_than, dst, var_left, var_right)
GETOP_DECL_3 (assignment, var_left, type_value_right, value_right)
GETOP_DECL_2 (call_0, lhs, name_lit_idx)
GETOP_DECL_3 (call_1, lhs, name_lit_idx, arg1_lit_idx)
GETOP_DECL_3 (call_n, lhs, name_lit_idx, arg1_lit_idx)
GETOP_DECL_3 (native_call, lhs, name, arg_list)
GETOP_DECL_2 (func_decl_1, name_lit_idx, arg1_lit_idx)
GETOP_DECL_3 (func_decl_2, name_lit_idx, arg1_lit_idx, arg2_lit_idx)
GETOP_DECL_3 (func_decl_n, name_lit_idx, arg1_lit_idx, arg2_lit_idx)
GETOP_DECL_3 (varg_list, arg1_lit_idx, arg2_lit_idx, arg3_lit_idx)
GETOP_DECL_1 (exitval, status_code)
GETOP_DECL_1 (retval, ret_value)
GETOP_DECL_0 (ret)
GETOP_DECL_0 (nop)
GETOP_DECL_2 (b_not, dst, var_right)
GETOP_DECL_2 (logical_not, dst, var_right)
GETOP_DECL_3 (instanceof, dst, var_left, var_right)
GETOP_DECL_3 (in, dst, var_left, var_right)

#endif /* OPCODES_SUPPORT_H */

