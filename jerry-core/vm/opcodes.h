/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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
#include "ecma-stack.h"
#include "jrt.h"

/* Maximum opcodes number in bytecode.  */
#define MAX_OPCODES (256*256 - 1)

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
  OPCODE_ARG_TYPE_SMALLINT, /**< small integer: from 0 to 255 */
  OPCODE_ARG_TYPE_SMALLINT_NEGATE, /**< small integer: from -255 to -0 */
  OPCODE_ARG_TYPE_NUMBER, /**< index of number literal */
  OPCODE_ARG_TYPE_NUMBER_NEGATE, /**< index of number literal with negation */
  OPCODE_ARG_TYPE_STRING, /**< index of string literal */
  OPCODE_ARG_TYPE_VARIABLE /**< index of variable name */
} opcode_arg_type_operand;

/**
 * Types of data in 'meta' opcode.
 */
typedef enum
{
  OPCODE_META_TYPE_UNDEFINED, /**< undefined meta (should be rewritten) */
  OPCODE_META_TYPE_THIS_ARG, /**< value (var_idx) of this used during call */
  OPCODE_META_TYPE_VARG, /**< element (var_idx) of arguments' list */
  OPCODE_META_TYPE_VARG_PROP_DATA, /**< name (lit_idx) and value (var_idx) for a data property descriptor */
  OPCODE_META_TYPE_VARG_PROP_GETTER, /**< name (lit_idx) and getter (var_idx) for an accessor property descriptor */
  OPCODE_META_TYPE_VARG_PROP_SETTER, /**< name (lit_idx) and setter (var_idx) for an accessor property descriptor */
  OPCODE_META_TYPE_END_WITH, /**< end of with statement */
  OPCODE_META_TYPE_FUNCTION_END, /**< offset to function end */
  OPCODE_META_TYPE_CATCH, /**< mark of beginning of catch block containing pointer to end of catch block */
  OPCODE_META_TYPE_CATCH_EXCEPTION_IDENTIFIER, /**< literal index containing name of variable with exception object */
  OPCODE_META_TYPE_FINALLY, /**< mark of beginning of finally block containing pointer to end of finally block */
  OPCODE_META_TYPE_END_TRY_CATCH_FINALLY, /**< mark of end of try-catch, try-finally, try-catch-finally blocks */
  OPCODE_META_TYPE_SCOPE_CODE_FLAGS /**< set of flags indicating various properties of the scope's code
                                     *   (See also: opcode_scope_code_flags_t) */
} opcode_meta_type;

/**
 * Flags indicating various properties of a scope's code
 */
typedef enum : idx_t
{
  OPCODE_SCOPE_CODE_FLAGS__NO_FLAGS         = (0u),      /**< initializer for empty flag set */
  OPCODE_SCOPE_CODE_FLAGS_STRICT            = (1u << 0), /**< code is strict mode code */
} opcode_scope_code_flags_t;

/**
 * Interpreter context
 */
typedef struct
{
  opcode_counter_t pos; /**< current opcode to execute */
  ecma_value_t this_binding; /**< this binding for current context */
  ecma_object_t *lex_env_p; /**< current lexical environment */
  bool is_strict; /**< is current code execution mode strict? */
  bool is_eval_code; /**< is current code executed with eval */
  idx_t min_reg_num; /**< minimum idx used for register identification */
  idx_t max_reg_num; /**< maximum idx used for register identification */
  ecma_number_t* tmp_num_p; /**< an allocated number (to reduce temporary allocations) */
  ecma_stack_frame_t stack_frame; /**< ecma-stack frame associated with the context */

#ifdef MEM_STATS
  size_t context_peak_allocated_heap_bytes;
  size_t context_peak_waste_heap_bytes;
  size_t context_peak_pools_count;
  size_t context_peak_allocated_pool_chunks;

  mem_heap_stats_t heap_stats_context_enter;
  mem_pools_stats_t pools_stats_context_enter;
#endif /* MEM_STATS */
} int_data_t;

opcode_counter_t calc_opcode_counter_from_idx_idx (const idx_t oc_idx_1, const idx_t oc_idx_2);
opcode_counter_t read_meta_opcode_counter (opcode_meta_type expected_type, int_data_t *int_data);

#define OP_CALLS_AND_ARGS(p, a)                                              \
        p##_3 (a, call_n, lhs, name_lit_idx, arg_list)                       \
        p##_3 (a, native_call, lhs, name, arg_list)                          \
        p##_3 (a, construct_n, lhs, name_lit_idx, arg_list)                  \
        p##_2 (a, func_decl_n, name_lit_idx, arg_list)                       \
        p##_3 (a, func_expr_n, lhs, name_lit_idx, arg_list)                  \
        p##_1 (a, exitval, status_code)                                      \
        p##_1 (a, retval, ret_value)                                         \
        p##_0 (a, ret)

#define OP_INITS(p, a)                                                       \
        p##_2 (a, array_decl, lhs, list)                                     \
        p##_3 (a, prop_getter, lhs, obj, prop)                               \
        p##_3 (a, prop_setter, obj, prop, rhs)                               \
        p##_2 (a, obj_decl, lhs, list)                                       \
        p##_1 (a, this_binding, lhs)                                         \
        p##_2 (a, delete_var, lhs, name)                                     \
        p##_3 (a, delete_prop, lhs, base, name)                              \
        p##_2 (a, typeof, lhs, obj)                                          \
        p##_1 (a, with, expr)                                                \
        p##_2 (a, try_block, oc_idx_1, oc_idx_2)                             \
        p##_1 (a, throw_value, var)

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
        p##_3 (a, remainder, dst, var_left, var_right)                       \
        p##_2 (a, unary_minus, dst, var)                                     \
        p##_2 (a, unary_plus, dst, var)

#define OP_JUMPS(p, a)                                                       \
        p##_2 (a, jmp_up, opcode_1, opcode_2)                                \
        p##_2 (a, jmp_down, opcode_1, opcode_2)                              \
        p##_0 (a, nop)                                                       \
        p##_3 (a, is_true_jmp_up, value, opcode_1, opcode_2)                 \
        p##_3 (a, is_true_jmp_down, value, opcode_1, opcode_2)               \
        p##_3 (a, is_false_jmp_up, value, opcode_1, opcode_2)                \
        p##_3 (a, is_false_jmp_down, value, opcode_1, opcode_2)

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

typedef struct
{
  uint8_t uids[4];
} raw_opcode;

#endif /* OPCODES_H */
