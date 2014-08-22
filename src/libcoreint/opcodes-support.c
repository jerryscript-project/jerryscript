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

#include "opcodes-support.h"

#define GETOP_DEF_0(name) \
        __opcode getop_##name (void) \
        { \
          __opcode opdata; \
          opdata.op_idx = __op__idx_##name; \
          return opdata; \
        }

#define GETOP_DEF_1(name, field1) \
        __opcode getop_##name (T_IDX arg1) \
        { \
          __opcode opdata; \
          opdata.op_idx = __op__idx_##name; \
          opdata.data.name.field1 = arg1; \
          return opdata; \
        }

#define GETOP_DEF_2(name, field1, field2) \
        __opcode getop_##name (T_IDX arg1, T_IDX arg2) \
        { \
          __opcode opdata; \
          opdata.op_idx = __op__idx_##name; \
          opdata.data.name.field1 = arg1; \
          opdata.data.name.field2 = arg2; \
          return opdata; \
        }

#define GETOP_DEF_3(name, field1, field2, field3) \
        __opcode getop_##name (T_IDX arg1, T_IDX arg2, T_IDX arg3) \
        { \
          __opcode opdata; \
          opdata.op_idx = __op__idx_##name; \
          opdata.data.name.field1 = arg1; \
          opdata.data.name.field2 = arg2; \
          opdata.data.name.field3 = arg3; \
          return opdata; \
        }

GETOP_DEF_3 (construct_decl, lhs, name_lit_idx, arg_list)
GETOP_DEF_1 (func_decl_0, name_lit_idx)
GETOP_DEF_2 (array_decl, lhs, list)
GETOP_DEF_3 (prop, lhs, name, value)
GETOP_DEF_3 (prop_getter, lhs, obj, prop)
GETOP_DEF_3 (prop_setter, obj, prop, rhs)
GETOP_DEF_2 (prop_get_decl, lhs, prop)
GETOP_DEF_3 (prop_set_decl, lhs, prop, arg)
GETOP_DEF_2 (obj_decl, lhs, list)
GETOP_DEF_1 (this, lhs)
GETOP_DEF_2 (delete, lhs, obj)
GETOP_DEF_2 (typeof, lhs, obj)
GETOP_DEF_1 (with, expr)
GETOP_DEF_0 (end_with)
GETOP_DEF_1 (var_decl, variable_name)
GETOP_DEF_2 (reg_var_decl, min, max)
GETOP_DEF_3 (meta, type, data_1, data_2)
GETOP_DEF_2 (is_true_jmp, value, opcode)
GETOP_DEF_2 (is_false_jmp, value, opcode)
GETOP_DEF_1 (jmp, opcode_idx)
GETOP_DEF_1 (jmp_up, opcode_count)
GETOP_DEF_1 (jmp_down, opcode_count)
GETOP_DEF_3 (addition, dst, var_left, var_right)
GETOP_DEF_2 (post_incr, dst, var_right)
GETOP_DEF_2 (post_decr, dst, var_right)
GETOP_DEF_2 (pre_incr, dst, var_right)
GETOP_DEF_2 (pre_decr, dst, var_right)
GETOP_DEF_3 (substraction, dst, var_left, var_right)
GETOP_DEF_3 (division, dst, var_left, var_right)
GETOP_DEF_3 (multiplication, dst, var_left, var_right)
GETOP_DEF_3 (remainder, dst, var_left, var_right)
GETOP_DEF_3 (b_shift_left, dst, var_left, var_right)
GETOP_DEF_3 (b_shift_right, dst, var_left, var_right)
GETOP_DEF_3 (b_shift_uright, dst, var_left, var_right)
GETOP_DEF_3 (b_and, dst, var_left, var_right)
GETOP_DEF_3 (b_or, dst, var_left, var_right)
GETOP_DEF_3 (b_xor, dst, var_left, var_right)
GETOP_DEF_3 (logical_and, dst, var_left, var_right)
GETOP_DEF_3 (logical_or, dst, var_left, var_right)
GETOP_DEF_3 (equal_value, dst, var_left, var_right)
GETOP_DEF_3 (not_equal_value, dst, var_left, var_right)
GETOP_DEF_3 (equal_value_type, dst, var_left, var_right)
GETOP_DEF_3 (not_equal_value_type, dst, var_left, var_right)
GETOP_DEF_3 (less_than, dst, var_left, var_right)
GETOP_DEF_3 (greater_than, dst, var_left, var_right)
GETOP_DEF_3 (less_or_equal_than, dst, var_left, var_right)
GETOP_DEF_3 (greater_or_equal_than, dst, var_left, var_right)
GETOP_DEF_3 (assignment, var_left, type_value_right, value_right)
GETOP_DEF_2 (call_0, lhs, name_lit_idx)
GETOP_DEF_3 (call_1, lhs, name_lit_idx, arg1_lit_idx)
GETOP_DEF_3 (call_n, lhs, name_lit_idx, arg1_lit_idx)
GETOP_DEF_3 (native_call, lhs, name, arg_list)
GETOP_DEF_2 (func_decl_1, name_lit_idx, arg1_lit_idx)
GETOP_DEF_3 (func_decl_2, name_lit_idx, arg1_lit_idx, arg2_lit_idx)
GETOP_DEF_3 (func_decl_n, name_lit_idx, arg1_lit_idx, arg2_lit_idx)
GETOP_DEF_3 (varg_list, arg1_lit_idx, arg2_lit_idx, arg3_lit_idx)
GETOP_DEF_1 (exitval, status_code)
GETOP_DEF_1 (retval, ret_value)
GETOP_DEF_0 (ret)
GETOP_DEF_0 (nop)
GETOP_DEF_2 (b_not, dst, var_right)
GETOP_DEF_2 (logical_not, dst, var_right)
GETOP_DEF_3 (instanceof, dst, var_left, var_right)
GETOP_DEF_3 (in, dst, var_left, var_right)
