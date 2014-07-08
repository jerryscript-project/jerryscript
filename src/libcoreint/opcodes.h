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

#ifdef __HOST
#include <stdio.h>
#endif

#include "globals.h"

#define OPCODE struct __opcode
struct __int_data;

#define OP_STRUCT_FIELD(name) struct __op_##name name;
#define OP_ENUM_FIELD(name) name ,
#define OP_FUNC_DECL(name) void opfunc_##name  (OPCODE, struct __int_data *);

/** A single bytecode instruction is 32bit wide and has an 8bit opcode field
 and several operand of 8 of 16 bit.*/

// OPCODE FIELD TYPES
#define T_IDX uint8_t /** index values */

OPCODE;
typedef void (*opfunc)(OPCODE, struct __int_data *);

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
    op(retval)\
    op(ret)

#define OP_ASSIGNMENTS(op)              \
    op(assignment)                      \
    op(assignment_multiplication)       \
    op(assignment_devision)             \
    op(assignment_remainder)            \
    op(assignment_addition)             \
    op(assignment_substruction)         \
    op(assignment_shift_left)           \
    op(assignment_shift_right)          \
    op(assignment_shift_uright)         \
    op(assignment_b_and)                \
    op(assignment_b_xor)                \
    op(assignment_b_or)

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

#define OP_ARITHMETIC(op)               \
    op(addition)                        \
    op(substraction)                    \
    op(division)                        \
    op(multiplication)                  \
    op(remainder)

#define OP_UNCONDITIONAL_JUMPS(op)      \
    op(jmp)                             \
    op(jmp_up)                          \
    op(jmp_down)

#define OP_UNARY_OPS(op)                \
    op(is_true_jmp)                     \
    op(is_false_jmp)

#define OP_CONDITIONAL_JUMPS(op)        \
    OP_UNARY_OPS(op)                    \
    op(is_less_than)                    \
    op(is_less_or_equal)                \
    op(is_greater_than)                 \
    op(is_greater_or_equal)             \
    op(is_equal_value)                  \
    op(is_not_equal_value)              \
    op(is_equal_value_type)             \
    op(is_not_equal_value_type)

#define OP_LIST(op)                     \
  OP_LOOPS(op)                          \
  OP_CALLS_AND_ARGS(op)                 \
  OP_ASSIGNMENTS(op)                    \
  OP_B_LOGICAL(op)                      \
  OP_B_BITWISE(op)                      \
  OP_B_SHIFTS(op)                       \
  OP_ARITHMETIC(op)                     \
  OP_UNCONDITIONAL_JUMPS(op)            \
  OP_CONDITIONAL_JUMPS(op)

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

void save_op_data (int, OPCODE);

#endif	/* OPCODES_H */

