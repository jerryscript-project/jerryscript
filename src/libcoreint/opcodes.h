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

#define OP_0(action, name) \
        __##action (name, void, void, void)

#define OP_1(action, name, field1) \
        __##action (name, field1, void, void)

#define OP_2(action, name, field1, field2) \
        __##action (name, field1, field2, void)

#define OP_3(action, name, field1, field2, field3) \
        __##action (name, field1, field2, field3)

typedef uint16_t opcode_counter_t; /** opcode counters */
typedef uint8_t idx_t; /** index values */

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

/**
 * Types of data in 'meta' opcode.
 */
typedef enum
{
  OPCODE_META_TYPE_THIS_ARG /**< value of this used during call */
} opcode_meta_type;

typedef struct
{
  opcode_counter_t pos; /**< current opcode to execute */
  ecma_value_t this_binding; /**< this binding for current context */
  ecma_object_t *lex_env_p; /**< current lexical environment */
  bool is_strict; /**< is current code execution mode strict? */
  bool is_eval_code; /**< is current code executed with eval */
  idx_t min_reg_num; /**< minimum idx used for register identification */
  idx_t max_reg_num; /**< maximum idx used for register identification */
  ecma_value_t *regs_p; /**< register variables */
} int_data_t;

#define OP_CALLS_AND_ARGS(p, a)                                              \
        p##_2 (a, call_0, lhs, name_lit_idx)                                 \
        p##_3 (a, call_1, lhs, name_lit_idx, arg1_lit_idx)                   \
        p##_3 (a, call_n, lhs, name_lit_idx, arg_list)                       \
        p##_3 (a, native_call, lhs, name, arg_list)                          \
        p##_3 (a, construct_n, lhs, name_lit_idx, arg_list)                  \
        p##_1 (a, func_decl_0, name_lit_idx)                                 \
        p##_2 (a, func_decl_1, name_lit_idx, arg1_lit_idx)                   \
        p##_3 (a, func_decl_2, name_lit_idx, arg1_lit_idx, arg2_lit_idx)     \
        p##_2 (a, func_decl_n, name_lit_idx, arg_list)                       \
        p##_3 (a, func_expr_n, lhs, name_lit_idx, arg_list)                  \
        p##_3 (a, varg_list, arg1_lit_idx, arg2_lit_idx, arg3_lit_idx)       \
        p##_1 (a, exitval, status_code)                                      \
        p##_1 (a, retval, ret_value)                                         \
        p##_0 (a, ret)

#define OP_INITS(p, a)                                                       \
        p##_2 (a, array_decl, lhs, list)                                     \
        p##_3 (a, prop, lhs, name, value)                                    \
        p##_3 (a, prop_getter, lhs, obj, prop)                               \
        p##_3 (a, prop_setter, obj, prop, rhs)                               \
        p##_2 (a, prop_get_decl, lhs, prop)                                  \
        p##_3 (a, prop_set_decl, lhs, prop, arg)                             \
        p##_2 (a, obj_decl, lhs, list)                                       \
        p##_1 (a, this, lhs)                                                 \
        p##_2 (a, delete, lhs, obj)                                          \
        p##_2 (a, typeof, lhs, obj)                                          \
        p##_1 (a, with, expr)                                                \
        p##_0 (a, end_with)

#define OP_ASSIGNMENTS(p, a)                                                 \
        p##_3 (a, assignment, var_left, type_value_right, value_right)

#define OP_B_SHIFTS(p, a)                                                    \
        p##_3 (a, b_shift_left, dst, var_left, var_right)                    \
        p##_3 (a, b_shift_right, dst, var_left, var_right)                   \
        p##_3 (a, b_shift_uright, dst, var_left, var_right)

#define OP_B_BITWISE(p, a)                                                   \
        p##_3 (a, b_and, dst, var_left, var_right)                           \
        p##_3 (a, b_or, dst, var_left, var_right)                            \
        p##_3 (a, b_xor, dst, var_left, var_right)                           \
        p##_2 (a, b_not, dst, var_right)

#define OP_B_LOGICAL(p, a)                                                   \
        p##_3 (a, logical_and, dst, var_left, var_right)                     \
        p##_3 (a, logical_or, dst, var_left, var_right)                      \
        p##_2 (a, logical_not, dst, var_right)

#define OP_EQUALITY(p, a)                                                    \
        p##_3 (a, equal_value, dst, var_left, var_right)                     \
        p##_3 (a, not_equal_value, dst, var_left, var_right)                 \
        p##_3 (a, equal_value_type, dst, var_left, var_right)                \
        p##_3 (a, not_equal_value_type, dst, var_left, var_right)

#define OP_RELATIONAL(p, a)                                                  \
        p##_3 (a, less_than, dst, var_left, var_right)                       \
        p##_3 (a, greater_than, dst, var_left, var_right)                    \
        p##_3 (a, less_or_equal_than, dst, var_left, var_right)              \
        p##_3 (a, greater_or_equal_than, dst, var_left, var_right)           \
        p##_3 (a, instanceof, dst, var_left, var_right)                      \
        p##_3 (a, in, dst, var_left, var_right)

#define OP_ARITHMETIC(p, a)                                                  \
        p##_2 (a, post_incr, dst, var_right)                                 \
        p##_2 (a, post_decr, dst, var_right)                                 \
        p##_2 (a, pre_incr, dst, var_right)                                  \
        p##_2 (a, pre_decr, dst, var_right)                                  \
        p##_3 (a, addition, dst, var_left, var_right)                        \
        p##_3 (a, substraction, dst, var_left, var_right)                    \
        p##_3 (a, division, dst, var_left, var_right)                        \
        p##_3 (a, multiplication, dst, var_left, var_right)                  \
        p##_3 (a, remainder, dst, var_left, var_right)

#define OP_JUMPS(p, a)                                                       \
        p##_1 (a, jmp, opcode_idx)                                           \
        p##_1 (a, jmp_up, opcode_count)                                      \
        p##_1 (a, jmp_down, opcode_count)                                    \
        p##_0 (a, nop)                                                       \
        p##_2 (a, is_true_jmp, value, opcode)                                \
        p##_2 (a, is_false_jmp, value, opcode)

#define OP_LIST_FULL(p, a)                                                   \
        OP_CALLS_AND_ARGS (p, a)                                             \
        OP_INITS (p, a)                                                      \
        OP_ASSIGNMENTS (p, a)                                                \
        OP_B_LOGICAL (p, a)                                                  \
        OP_B_BITWISE (p, a)                                                  \
        OP_B_SHIFTS (p, a)                                                   \
        OP_EQUALITY (p, a)                                                   \
        OP_RELATIONAL (p, a)                                                 \
        OP_ARITHMETIC (p, a)                                                 \
        OP_JUMPS (p, a)                                                      \
        p##_1 (a, var_decl, variable_name)                                   \
        p##_2 (a, reg_var_decl, min, max)                                    \
        p##_3 (a, meta, type, data_1, data_2)

#define OP_LIST(a) OP_LIST_FULL (OP, a)
#define OP_ARGS_LIST(a) OP_LIST_FULL (a, void)

#define OP_DATA_0(action, name) \
        typedef struct \
        { \
          idx_t __do_not_use; \
        } __op_##name;

#define OP_DATA_1(action, name, arg1) \
        typedef struct \
        { \
          idx_t arg1; \
        } __op_##name;

#define OP_DATA_2(action, name, arg1, arg2) \
        typedef struct \
        { \
          idx_t arg1; \
          idx_t arg2; \
        } __op_##name;

#define OP_DATA_3(action, name, arg1, arg2, arg3) \
        typedef struct \
        { \
          idx_t arg1; \
          idx_t arg2; \
          idx_t arg3; \
        } __op_##name;

OP_ARGS_LIST (OP_DATA)

#define __OP_STRUCT_FIELD(name, arg1, arg2, arg3) __op_##name name;
typedef struct
{
  idx_t op_idx;
  union
  {
    OP_LIST (OP_STRUCT_FIELD)
  } data;
} opcode_t;
#undef __OP_STRUCT_FIELD

#define __OP_ENUM_FIELD(name, arg1, arg2, arg3) __op__idx_##name ,
enum __opcode_idx
{
  OP_LIST (OP_ENUM_FIELD)
  LAST_OP
};
#undef __OP_ENUM_FIELD

#define __OP_FUNC_DECL(name, arg1, arg2, arg3) ecma_completion_value_t opfunc_##name (opcode_t, int_data_t*);
OP_LIST (OP_FUNC_DECL)
#undef __OP_FUNC_DECL

typedef ecma_completion_value_t (*opfunc) (opcode_t, int_data_t *);

#define GETOP_DECL_0(a, name) \
        opcode_t getop_##name (void);

#define GETOP_DECL_1(a, name, arg1) \
        opcode_t getop_##name (idx_t);

#define GETOP_DECL_2(a, name, arg1, arg2) \
        opcode_t getop_##name (idx_t, idx_t);

#define GETOP_DECL_3(a, name, arg1, arg2, arg3) \
        opcode_t getop_##name (idx_t, idx_t, idx_t);

#define GETOP_DEF_0(a, name) \
        opcode_t getop_##name (void) \
        { \
          opcode_t opdata; \
          opdata.op_idx = __op__idx_##name; \
          return opdata; \
        }

OP_ARGS_LIST (GETOP_DECL)
#undef GETOP_DECL_0
#undef GETOP_DECL_1
#undef GETOP_DECL_2
#undef GETOP_DECL_3


#endif /* OPCODES_H */
