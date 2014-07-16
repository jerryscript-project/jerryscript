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
          opdata.op_idx = name; \
          return opdata; \
        }

#define GETOP_IMPL_1(name, field1) \
        OPCODE getop_##name (T_IDX arg1) { \
          OPCODE opdata; \
          opdata.op_idx = name; \
          opdata.data.name.field1 = arg1; \
          return opdata; \
        }

#define GETOP_IMPL_2(name, field1, field2) \
        OPCODE getop_##name (T_IDX arg1, T_IDX arg2) { \
          OPCODE opdata; \
          opdata.op_idx = name; \
          opdata.data.name.field1 = arg1; \
          opdata.data.name.field2 = arg2; \
          return opdata; \
        }

#define GETOP_IMPL_3(name, field1, field2, field3) \
        OPCODE getop_##name (T_IDX arg1, T_IDX arg2, T_IDX arg3) { \
          OPCODE opdata; \
          opdata.op_idx = name; \
          opdata.data.name.field1 = arg1; \
          opdata.data.name.field2 = arg2; \
          opdata.data.name.field3 = arg3; \
          return opdata; \
        }

// All conditional jumps should be followed by a JMP instruction, which holds
// the target of conditional JMP. If condition is TRUE, than JMP to the target.
// Otherwise they fall through to the instruction after the JMP.
// if (true) { JMP } else { INSTR_AFTER_JMP }
// NOTE: We cannot swap L < R to R > L, because of floating-point semantics.

/** L < R   */
OP_CODE_DECL (is_less_than, T_IDX_IDX,
              value_left,
              value_right)

/** L <= R  */
OP_CODE_DECL (is_less_or_equal, T_IDX_IDX,
              value_left,
              value_right)

/** L > R   */
OP_CODE_DECL (is_greater_than, T_IDX_IDX,
              value_left,
              value_right)

/** L >= R  */
OP_CODE_DECL (is_greater_or_equal, T_IDX_IDX,
              value_left,
              value_right)

/** L == R  */
OP_CODE_DECL (is_equal_value, T_IDX_IDX,
              value_left,
              value_right)

/** L != R  */
OP_CODE_DECL (is_not_equal_value, T_IDX_IDX,
              value_left,
              value_right)

/** L === R */
OP_CODE_DECL (is_equal_value_type, T_IDX_IDX,
              value_left,
              value_right)

/** L !== R */
OP_CODE_DECL (is_not_equal_value_type, T_IDX_IDX,
              value_left,
              value_right)

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

// Assignment operators.
// Assign value to LEFT operand based on value of RIGHT operand.

/** L = R */
OP_CODE_DECL (assignment, T_IDX_IDX,
              value_left,
              value_right)

// Functions calls, declarations and argument handling

/** name(arg1); */
OP_CODE_DECL (call_1, T_IDX_IDX,
              name_lit_idx,
              arg1_lit_idx)

/** name(arg1, arg2); */
OP_CODE_DECL (call_2, T_IDX_IDX_IDX,
              name_lit_idx,
              arg1_lit_idx,
              arg2_lit_idx)

/** name(arg1, arg2, ... */
OP_CODE_DECL (call_n, T_IDX_IDX_IDX,
              name_lit_idx,
              arg1_lit_idx,
              arg2_lit_idx)

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

/** ..., arg1, ... */
OP_CODE_DECL (varg_1, T_IDX,
              arg1_lit_idx)

/** ..., arg1); */
OP_CODE_DECL (varg_1_end, T_IDX,
              arg1_lit_idx)

/** ..., arg1, arg2, ... */
OP_CODE_DECL (varg_2, T_IDX_IDX,
              arg1_lit_idx,
              arg2_lit_idx)

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

/** return value; */
OP_CODE_DECL (retval, T_IDX,
              ret_value)
OP_CODE_DECL_VOID (ret)

OP_CODE_DECL_VOID (nop)

// LOOPS
// Lately, all loops should be translated into different JMPs in an optimizer.

/** End of body of infinite loop should be ended with unconditional JMP
 *  to loop_root (ie. next op after loop condition)  */
OP_CODE_DECL (loop_inf, T_IDX,
              loop_root)

/** Numeric loop initialization.
 *  for (start,stop,step)
 */
OP_CODE_DECL (loop_init_num, T_IDX_IDX_IDX,
              start,
              stop,
              step)

/** Check loop (condition).
 *  if (loop cond is true)
 *  { next_op }
 *  else
 *  { goto after_loop_op; }
 */
OP_CODE_DECL (loop_precond_begin_num, T_IDX_IDX,
              condition,
              after_loop_op)

/** i++;
 * Increment iterator on step and JMP to PRECOND
 */
OP_CODE_DECL (loop_precond_end_num, T_IDX_IDX_IDX,
              iterator,
              step,
              precond_begin)

/** do {...} while (cond);
 *  If condition is true, JMP to BODY_ROOT
 */
OP_CODE_DECL (loop_postcond, T_IDX_IDX,
              condition,
              body_root)

///** for vars...in iter, state, ctl */
//OP_CODE_DECL (loop_init, T_IDX_IDX_IDX,             
//              start_idx, stop_idx, step_idx)
///** loop (condition) */
//OP_CODE_DECL (loop_cond_pre_begin, T_IDX_IDX,       
//              condition, body_root)
///** i++;*/
//OP_CODE_DECL (loop_cond_pre_end, T_IDX,             
//              iterator, body_root)

// Property accessors (array, objects, strings)
/** Array ops for ILMIR*/
//OP_CODE_DECL (array_copy, T_IDX_IDX,                /** L = R */
//              var_left, var_right)
//OP_CODE_DECL (array_set, T_IDX_IDX_IDX,             /** array[index] = src */
//              dst, var_left, var_right)
//OP_CODE_DECL (array_get, T_IDX_IDX_IDX,             /** dst = array[index] */
//              dst, array, index)

//// TODO

// Variable declaration
OP_CODE_DECL (decl_var, T_IDX,
              variable)
        
// TODO New constructor

#endif	/* OPCODE_STRUCTURES_H */
        