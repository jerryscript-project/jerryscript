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

#ifndef OPCODE_STRUCTURES_H
#define	OPCODE_STRUCTURES_H

// Jerry bytecode ver:07/04/2014 
#define OP_DEF(name, list) struct __op_##name { list ;  } ;


#define OP_CODE_DECL_VOID(name) \
        struct __op_##name { T_IDX __do_not_use; }; \
        OPCODE getop_##name ( void );

#define OP_CODE_DECL(name, type, ... ) \
        OP_DEF (name, type##_DECL( __VA_ARGS__ ) ) \
        OPCODE getop_##name ( type );

#define T_IDX_IDX T_IDX, T_IDX
#define T_IDX_IDX_IDX T_IDX, T_IDX, T_IDX

#define T_IDX_DECL(name) T_IDX name
#define T_IDX_IDX_DECL(name1, name2) \
        T_IDX_DECL( name1 ) ; \
        T_IDX_DECL( name2 )
#define T_IDX_IDX_IDX_DECL(name1, name2, name3) \
        T_IDX_DECL( name1 ) ; \
        T_IDX_DECL( name2 ); \
        T_IDX_DECL( name3 )

#define GETOP_IMPL_0(name) \
        OPCODE getop_##name () { \
          OPCODE opdata; \
          opdata.op_idx = __op__idx_##name; \
          return opdata; \
        }

#define GETOP_IMPL_1(name, field1) \
        OPCODE getop_##name (T_IDX arg1) { \
          OPCODE opdata; \
          opdata.op_idx = __op__idx_##name; \
          opdata.data.name.field1 = arg1; \
          return opdata; \
        }

#define GETOP_IMPL_2(name, field1, field2) \
        OPCODE getop_##name (T_IDX arg1, T_IDX arg2) { \
          OPCODE opdata; \
          opdata.op_idx = __op__idx_##name; \
          opdata.data.name.field1 = arg1; \
          opdata.data.name.field2 = arg2; \
          return opdata; \
        }

#define GETOP_IMPL_3(name, field1, field2, field3) \
        OPCODE getop_##name (T_IDX arg1, T_IDX arg2, T_IDX arg3) { \
          OPCODE opdata; \
          opdata.op_idx = __op__idx_##name; \
          opdata.data.name.field1 = arg1; \
          opdata.data.name.field2 = arg2; \
          opdata.data.name.field3 = arg3; \
          return opdata; \
        }

/** Instruction tests if BOOLEAN value is TRUE and JMP to DST */
OP_CODE_DECL (is_true_jmp, T_IDX_IDX,
              value,
              opcode)

/** Instruction tests if BOOLEAN value is FALSE and JMP to DST */
OP_CODE_DECL (is_false_jmp, T_IDX_IDX,
              value,
              opcode)

/** Unconditional JMP to the specified opcode index */
OP_CODE_DECL (jmp, T_IDX,
              opcode_idx)

/** Unconditional JMP on opcode_count up */
OP_CODE_DECL (jmp_up, T_IDX,
              opcode_count)

/** Unconditional JMP on opcode_count down */
OP_CODE_DECL (jmp_down, T_IDX,
              opcode_count)

/** dst = L + R */
OP_CODE_DECL (addition, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = L - R */
OP_CODE_DECL (substraction, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = L / R */
OP_CODE_DECL (division, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = L * R */
OP_CODE_DECL (multiplication, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = L % R */
OP_CODE_DECL (remainder, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = L << R */
OP_CODE_DECL (b_shift_left, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = L >> R */
OP_CODE_DECL (b_shift_right, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = L >>> R */
OP_CODE_DECL (b_shift_uright, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

// Binary bitwise operators.
// Operands is a set of 32 bits
// Returns numerical.

/** dst = L & R */
OP_CODE_DECL (b_and, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = L | R */
OP_CODE_DECL (b_or, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = L ^ R */
OP_CODE_DECL (b_xor, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = ~ R */
OP_CODE_DECL (b_not, T_IDX_IDX,
              dst,
              var_right)

// Binary logical operators.
// Operands are booleans.
// Return boolean.

/** dst = L && R */
OP_CODE_DECL (logical_and, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = L || R */
OP_CODE_DECL (logical_or, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = ! R */
OP_CODE_DECL (logical_not, T_IDX_IDX,
              dst,
              var_right)

// Equality operations.

/** dst = L == R.  */
OP_CODE_DECL (equal_value, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = L != R.  */
OP_CODE_DECL (not_equal_value, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = L === R.  */
OP_CODE_DECL (equal_value_type, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = L !== R.  */
OP_CODE_DECL (not_equal_value_type, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

// Relational operations.

/** dst = L < R.  */
OP_CODE_DECL (less_than, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = L > R.  */
OP_CODE_DECL (greater_than, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = L <= R.  */
OP_CODE_DECL (less_or_equal_than, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = L >= R.  */
OP_CODE_DECL (greater_or_equal_than, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = L instanceof R.  */
OP_CODE_DECL (instanceof, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = L in R.  */
OP_CODE_DECL (in, T_IDX_IDX_IDX,
              dst,
              var_left,
              var_right)

/** dst = var_right++.  */
OP_CODE_DECL (post_incr, T_IDX_IDX,
              dst,
              var_right)

/** dst = var_right--.  */
OP_CODE_DECL (post_decr, T_IDX_IDX,
              dst,
              var_right)

/** dst = ++var_right.  */
OP_CODE_DECL (pre_incr, T_IDX_IDX,
              dst,
              var_right)

/** dst = --var_right.  */
OP_CODE_DECL (pre_decr, T_IDX_IDX,
              dst,
              var_right)

// Assignment operators.
// Assign value to LEFT operand based on value of RIGHT operand.

/** L = R */
OP_CODE_DECL (assignment, T_IDX_IDX_IDX,
              var_left,
              type_value_right,
              value_right)

// Functions calls, declarations and argument handling

/** a = name(); */
OP_CODE_DECL (call_0, T_IDX_IDX,
              lhs,
              name_lit_idx)

/** a = name(arg1); */
OP_CODE_DECL (call_1, T_IDX_IDX_IDX,
              lhs,
              name_lit_idx,
              arg1_lit_idx)

/** a = name(arg1, ... */
OP_CODE_DECL (call_n, T_IDX_IDX_IDX,
              lhs,
              name_lit_idx,
              arg1_lit_idx)

/** a = new name(); */
OP_CODE_DECL (construct_0, T_IDX_IDX,
              lhs,
              name_lit_idx)

/** a = new name(arg1); */
OP_CODE_DECL (construct_1, T_IDX_IDX_IDX,
              lhs,
              name_lit_idx,
              arg1_lit_idx)

/** a = new name(arg1, ... */
OP_CODE_DECL (construct_n, T_IDX_IDX_IDX,
              lhs,
              name_lit_idx,
              arg1_lit_idx)

/** name(); */
OP_CODE_DECL (func_decl_0, T_IDX,
              name_lit_idx)

/** name(arg1); */
OP_CODE_DECL (func_decl_1, T_IDX_IDX,
              name_lit_idx,
              arg1_lit_idx)

/** name(arg1, arg2); */
OP_CODE_DECL (func_decl_2, T_IDX_IDX_IDX,
              name_lit_idx,
              arg1_lit_idx,
              arg2_lit_idx)

/** name(arg1, arg2, ... */
OP_CODE_DECL (func_decl_n, T_IDX_IDX_IDX,
              name_lit_idx,
              arg1_lit_idx,
              arg2_lit_idx)

/** a = name(); */
OP_CODE_DECL (func_expr_0, T_IDX_IDX,
              lhs,
              name_lit_idx)

/** a = name(arg1); */
OP_CODE_DECL (func_expr_1, T_IDX_IDX_IDX,
              lhs,
              name_lit_idx,
              arg1_lit_idx)

/** a = name(arg1,  ... */
OP_CODE_DECL (func_expr_n, T_IDX_IDX_IDX,
              lhs,
              name_lit_idx,
              arg1_lit_idx)

/** ..., arg1); */
OP_CODE_DECL (varg_1_end, T_IDX,
              arg1_lit_idx)

/** ..., arg1, arg2); */
OP_CODE_DECL (varg_2_end, T_IDX_IDX,
              arg1_lit_idx,
              arg2_lit_idx)

/** arg1, arg2, arg3, ... */
OP_CODE_DECL (varg_3, T_IDX_IDX_IDX,
              arg1_lit_idx,
              arg2_lit_idx,
              arg3_lit_idx)

/** arg1, arg2, arg3); */
OP_CODE_DECL (varg_3_end, T_IDX_IDX_IDX,
              arg1_lit_idx,
              arg2_lit_idx,
              arg3_lit_idx)

/** exit with status code; */
OP_CODE_DECL (exitval, T_IDX,
              status_code)

/** return value; */
OP_CODE_DECL (retval, T_IDX,
              ret_value)
OP_CODE_DECL_VOID (ret)

OP_CODE_DECL_VOID (nop)

/** a = []  */
OP_CODE_DECL (array_0, T_IDX,
              lhs)

/** a = [b]  */
OP_CODE_DECL (array_1, T_IDX_IDX,
              lhs,
              elem1)

/** a = [b, c]  */
OP_CODE_DECL (array_2, T_IDX_IDX_IDX,
              lhs,
              elem1,
              elem2)

/** a = [b, c ...  */
OP_CODE_DECL (array_n, T_IDX_IDX_IDX,
              lhs,
              elem1,
              elem2)

/** a = b : c  */
OP_CODE_DECL (prop, T_IDX_IDX_IDX,
              lhs,
              name,
              value)

/** a = b.c OR a = b[c]  */
OP_CODE_DECL (prop_access, T_IDX_IDX_IDX,
              lhs,
              obj,
              prop)

/** a = get prop ()  */
OP_CODE_DECL (prop_get_decl, T_IDX_IDX,
              lhs,
              prop)

/** a = set prop (arg)  */
OP_CODE_DECL (prop_set_decl, T_IDX_IDX_IDX,
              lhs,
              prop,
              arg)

/** a = { }  */
OP_CODE_DECL (obj_0, T_IDX,
              lhs)

/** a = { b }  */
OP_CODE_DECL (obj_1, T_IDX_IDX,
              lhs,
              arg1)

/** a = { b, c }  */
OP_CODE_DECL (obj_2, T_IDX_IDX_IDX,
              lhs,
              arg1,
              arg2)

/** a = { b, c ...  */
OP_CODE_DECL (obj_n, T_IDX_IDX_IDX,
              lhs,
              arg1,
              arg2)

/** a = this  */
OP_CODE_DECL (this, T_IDX,
              lhs)

/** a = delete b  */
OP_CODE_DECL (delete, T_IDX_IDX,
              lhs,
              obj)

/** a = typeof b  */
OP_CODE_DECL (typeof, T_IDX_IDX,
              lhs,
              obj)

/** with (b) {  */
OP_CODE_DECL (with, T_IDX,
              expr)

/** }  */
OP_CODE_DECL_VOID (end_with)

// Variable declaration
OP_CODE_DECL (var_decl, T_IDX,
              variable_name)

OP_CODE_DECL (reg_var_decl, T_IDX_IDX,
              min,
              max)
        

#endif	/* OPCODE_STRUCTURES_H */
        
