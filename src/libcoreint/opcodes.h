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
#define	OPCODES_H

#include "ecma-globals.h"
#include "globals.h"

#define OPCODE struct __opcode
struct __int_data;

#define OP_STRUCT_FIELD(name) struct __op_##name name;
#define OP_ENUM_FIELD(name) __op__idx_##name ,
#define OP_FUNC_DECL(name) ecma_CompletionValue_t opfunc_##name  (OPCODE, struct __int_data *);

/** A single bytecode instruction is 32bit wide and has an 8bit opcode field
 and several operand of 8 of 16 bit.*/

// OPCODE FIELD TYPES
#define T_IDX uint8_t /** index values */

OPCODE;
typedef ecma_CompletionValue_t (*opfunc)(OPCODE, struct __int_data *);

#define OP_LOOPS(op)                    \
    op(loop_inf)                        \
    op(loop_init_num)                   \
    op(loop_precond_begin_num)          \
    op(loop_precond_end_num)            \
    op(loop_postcond)

#define OP_CALLS_AND_ARGS(op)           \
    op(call_1)                          \
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
    op(exitval)                         \
    op(retval)                          \
    op(ret)

#define OP_ASSIGNMENTS(op)              \
    op(assignment)

#define OP_B_SHIFTS(op)                 \
    op(b_shift_left)                    \
    op(b_shift_right)                   \
    op(b_shift_uright)

#define OP_B_BITWISE(op)                \
    op(b_and)                           \
    op(b_or)                            \
    op(b_xor)

#define OP_B_LOGICAL(op)                \
    op(logical_and)                     \
    op(logical_or)

#define OP_EQUALITY(op)                 \
    op(equal_value)                     \
    op(not_equal_value)                 \
    op(equal_value_type)                \
    op(not_equal_value_type)            

#define OP_RELATIONAL(op)               \
    op(less_than)                       \
    op(greater_than)                    \
    op(less_or_equal_than)              \
    op(greater_or_equal_than)

#define OP_ARITHMETIC(op)               \
    op(addition)                        \
    op(substraction)                    \
    op(division)                        \
    op(multiplication)                  \
    op(remainder)

#define OP_UNCONDITIONAL_JUMPS(op)      \
    op(jmp)                             \
    op(jmp_up)                          \
    op(jmp_down)                        \
    op(nop)

#define OP_UNARY_OPS(op)                \
    op(is_true_jmp)                     \
    op(is_false_jmp)

#define OP_LIST(op)                     \
  OP_LOOPS(op)                          \
  OP_CALLS_AND_ARGS(op)                 \
  OP_ASSIGNMENTS(op)                    \
  OP_B_LOGICAL(op)                      \
  OP_B_BITWISE(op)                      \
  OP_B_SHIFTS(op)                       \
  OP_EQUALITY(op)                       \
  OP_RELATIONAL(op)                     \
  OP_ARITHMETIC(op)                     \
  OP_UNCONDITIONAL_JUMPS(op)            \
  OP_UNARY_OPS(op)                      \
  op(var_decl)

#include "opcode-structures.h"

enum __opcode_idx
{
  OP_LIST (OP_ENUM_FIELD)
  LAST_OP
};

union __opdata
{
  OP_LIST (OP_STRUCT_FIELD)
};

OP_LIST (OP_FUNC_DECL)

OPCODE
{
  T_IDX op_idx;
  union __opdata data;
}
__packed;

/**
 * Descriptor of assignment's second argument
 * that specifies type of third argument.
 */
typedef enum
{
  OPCODE_ARG_TYPE_SIMPLE, /**< ecma_SimpleValue_t */
  OPCODE_ARG_TYPE_SMALLINT, /**< small integer: from -128 to 127 */
  OPCODE_ARG_TYPE_NUMBER, /**< index of number literal */
  OPCODE_ARG_TYPE_STRING, /**< index of string literal */
  OPCODE_ARG_TYPE_VARIABLE /**< index of variable name */
} opcode_arg_type_operand;

#endif	/* OPCODES_H */

