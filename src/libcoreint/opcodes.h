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

#ifndef OPCODES_H
#define OPCODES_H

#include "ecma-globals.h"
#include "globals.h"

#define T_IDX uint8_t /** index values */
#define OPCODE __opcode

#define OP_STRUCT_FIELD(name) __op_##name name;
#define OP_ENUM_FIELD(name) __op__idx_##name ,
#define OP_FUNC_DECL(name) ecma_completion_value_t opfunc_##name (__opcode, __int_data*);

typedef uint16_t opcode_counter_t;

typedef struct
{
  opcode_counter_t pos; /**< current opcode to execute */
  ecma_value_t this_binding; /**< this binding for current context */
  ecma_object_t *lex_env_p; /**< current lexical environment */
  bool is_strict; /**< is current code execution mode strict? */
  bool is_eval_code; /**< is current code executed with eval */
  T_IDX min_reg_num; /**< minimum idx used for register identification */
  T_IDX max_reg_num; /**< maximum idx used for register identification */
  ecma_value_t *regs_p; /**< register variables */
} __int_data;

#define OP_CALLS_AND_ARGS(op)            \
    op (call_0)                          \
    op (call_1)                          \
    op (call_n)                          \
    op (native_call)                     \
    op (construct_decl)                  \
    op (func_decl_0)                     \
    op (func_decl_1)                     \
    op (func_decl_2)                     \
    op (func_decl_n)                     \
    op (varg_list)                       \
    op (exitval)                         \
    op (retval)                          \
    op (ret)

#define OP_INITS(op)                     \
    op (array_decl)                      \
    op (prop)                            \
    op (prop_getter)                     \
    op (prop_setter)                     \
    op (prop_get_decl)                   \
    op (prop_set_decl)                   \
    op (obj_decl)                        \
    op (this)                            \
    op (delete)                          \
    op (typeof)                          \
    op (with)                            \
    op (end_with)

#define OP_ASSIGNMENTS(op)               \
    op (assignment)

#define OP_B_SHIFTS(op)                  \
    op (b_shift_left)                    \
    op (b_shift_right)                   \
    op (b_shift_uright)

#define OP_B_BITWISE(op)                 \
    op (b_and)                           \
    op (b_or)                            \
    op (b_xor)                           \
    op (b_not)

#define OP_B_LOGICAL(op)                 \
    op (logical_and)                     \
    op (logical_or)                      \
    op (logical_not)

#define OP_EQUALITY(op)                  \
    op (equal_value)                     \
    op (not_equal_value)                 \
    op (equal_value_type)                \
    op (not_equal_value_type)

#define OP_RELATIONAL(op)                \
    op (less_than)                       \
    op (greater_than)                    \
    op (less_or_equal_than)              \
    op (greater_or_equal_than)           \
    op (instanceof)                      \
    op (in)

#define OP_ARITHMETIC(op)                \
    op (post_incr)                       \
    op (post_decr)                       \
    op (pre_incr)                        \
    op (pre_decr)                        \
    op (addition)                        \
    op (substraction)                    \
    op (division)                        \
    op (multiplication)                  \
    op (remainder)

#define OP_UNCONDITIONAL_JUMPS(op)       \
    op (jmp)                             \
    op (jmp_up)                          \
    op (jmp_down)                        \
    op (nop)

#define OP_UNARY_OPS(op)                 \
    op (is_true_jmp)                     \
    op (is_false_jmp)

#define OP_LIST(op)                      \
  OP_CALLS_AND_ARGS (op)                 \
  OP_INITS (op)                          \
  OP_ASSIGNMENTS (op)                    \
  OP_B_LOGICAL (op)                      \
  OP_B_BITWISE (op)                      \
  OP_B_SHIFTS (op)                       \
  OP_EQUALITY (op)                       \
  OP_RELATIONAL (op)                     \
  OP_ARITHMETIC (op)                     \
  OP_UNCONDITIONAL_JUMPS (op)            \
  OP_UNARY_OPS (op)                      \
  op (var_decl)                          \
  op (reg_var_decl)\
  op (meta)

#define OP_DATA (name, list) typedef struct { list ; } __op_##name;

#define OP_DATA_0(name) \
        typedef struct \
        { \
          T_IDX __do_not_use; \
        } __op_##name

#define OP_DATA_1(name, arg1) \
        typedef struct \
        { \
          T_IDX arg1; \
        } __op_##name

#define OP_DATA_2(name, arg1, arg2) \
        typedef struct \
        { \
          T_IDX arg1; \
          T_IDX arg2; \
        } __op_##name

#define OP_DATA_3(name, arg1, arg2, arg3) \
        typedef struct \
        { \
          T_IDX arg1; \
          T_IDX arg2; \
          T_IDX arg3; \
        } __op_##name

OP_DATA_2 (is_true_jmp, value, opcode);
OP_DATA_2 (is_false_jmp, value, opcode);
OP_DATA_1 (jmp, opcode_idx);
OP_DATA_1 (jmp_up, opcode_count);
OP_DATA_1 (jmp_down, opcode_count);
OP_DATA_3 (addition, dst, var_left, var_right);
OP_DATA_2 (post_incr, dst, var_right);
OP_DATA_2 (post_decr, dst, var_right);
OP_DATA_2 (pre_incr, dst, var_right);
OP_DATA_2 (pre_decr, dst, var_right);
OP_DATA_3 (substraction, dst, var_left, var_right);
OP_DATA_3 (division, dst, var_left, var_right);
OP_DATA_3 (multiplication, dst, var_left, var_right);
OP_DATA_3 (remainder, dst, var_left, var_right);
OP_DATA_3 (b_shift_left, dst, var_left, var_right);
OP_DATA_3 (b_shift_right, dst, var_left, var_right);
OP_DATA_3 (b_shift_uright, dst, var_left, var_right);
OP_DATA_3 (b_and, dst, var_left, var_right);
OP_DATA_3 (b_or, dst, var_left, var_right);
OP_DATA_3 (b_xor, dst, var_left, var_right);
OP_DATA_3 (logical_and, dst, var_left, var_right);
OP_DATA_3 (logical_or, dst, var_left, var_right);
OP_DATA_3 (equal_value, dst, var_left, var_right);
OP_DATA_3 (not_equal_value, dst, var_left, var_right);
OP_DATA_3 (equal_value_type, dst, var_left, var_right);
OP_DATA_3 (not_equal_value_type, dst, var_left, var_right);
OP_DATA_3 (less_than, dst, var_left, var_right);
OP_DATA_3 (greater_than, dst, var_left, var_right);
OP_DATA_3 (less_or_equal_than, dst, var_left, var_right);
OP_DATA_3 (greater_or_equal_than, dst, var_left, var_right);
OP_DATA_3 (assignment, var_left, type_value_right, value_right);
OP_DATA_2 (call_0, lhs, name_lit_idx);
OP_DATA_3 (call_1, lhs, name_lit_idx, arg1_lit_idx);
OP_DATA_3 (call_n, lhs, name_lit_idx, arg1_lit_idx);
OP_DATA_3 (native_call, lhs, name, arg_list);
OP_DATA_2 (func_decl_1, name_lit_idx, arg1_lit_idx);
OP_DATA_3 (func_decl_2, name_lit_idx, arg1_lit_idx, arg2_lit_idx);
OP_DATA_3 (func_decl_n, name_lit_idx, arg1_lit_idx, arg2_lit_idx);
OP_DATA_3 (varg_list, arg1_lit_idx, arg2_lit_idx, arg3_lit_idx);
OP_DATA_1 (exitval, status_code);
OP_DATA_1 (retval, ret_value);
OP_DATA_0 (ret);
OP_DATA_0 (nop);
OP_DATA_1 (var_decl, variable_name);
OP_DATA_2 (b_not, dst, var_right);
OP_DATA_2 (logical_not, dst, var_right);
OP_DATA_3 (instanceof, dst, var_left, var_right);
OP_DATA_3 (in, dst, var_left, var_right);
OP_DATA_3 (construct_decl, lhs, name_lit_idx, arg_list);
OP_DATA_1 (func_decl_0, name_lit_idx);
OP_DATA_2 (array_decl, lhs, list);
OP_DATA_3 (prop, lhs, name, value);
OP_DATA_3 (prop_getter, lhs, obj, prop);
OP_DATA_3 (prop_setter, obj, prop, rhs);
OP_DATA_2 (prop_get_decl, lhs, prop);
OP_DATA_3 (prop_set_decl, lhs, prop, arg);
OP_DATA_2 (obj_decl, lhs, list);
OP_DATA_1 (this, lhs);
OP_DATA_2 (delete, lhs, obj);
OP_DATA_2 (typeof, lhs, obj);
OP_DATA_1 (with, expr);
OP_DATA_0 (end_with);
OP_DATA_2 (reg_var_decl, min, max);
OP_DATA_3 (meta, type, data_1, data_2);

typedef struct
{
  T_IDX op_idx;
  union
  {
    OP_LIST (OP_STRUCT_FIELD)
  } data;
} __opcode;

enum __opcode_idx
{
  OP_LIST (OP_ENUM_FIELD)
  LAST_OP
};

OP_LIST (OP_FUNC_DECL)


typedef ecma_completion_value_t (*opfunc) (__opcode, __int_data *);

/**
 * Descriptor of assignment's second argument
 * that specifies type of third argument.
 */
typedef enum
{
  OPCODE_ARG_TYPE_SIMPLE, /**< ecma_simple_value_t */
  OPCODE_ARG_TYPE_SMALLINT, /**< small integer: from -128 to 127 */
  OPCODE_ARG_TYPE_NUMBER, /**< index of number literal */
  OPCODE_ARG_TYPE_STRING, /**< index of string literal */
  OPCODE_ARG_TYPE_VARIABLE /**< index of variable name */
} opcode_arg_type_operand;

#endif /* OPCODES_H */
