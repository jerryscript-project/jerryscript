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

#include "ecma-alloc.h"
#include "ecma-exceptions.h"
#include "ecma-helpers.h"
#include "ecma-operations.h"
#include "interpreter.h"
#include "jerry-libc.h"
#include "mem-heap.h"
#include "opcodes.h"

/**
 * String literal copy descriptor.
 */
typedef struct
{
  ecma_Char_t *str_p; /**< pointer to copied string literal */
  ecma_Char_t literal_copy[32]; /**< buffer with string literal,
                                     if it is stored locally
                                     (i.e. not in the heap) */
} string_literal_copy;

/**
 * Initialize string literal copy.
 */
static void __unused
init_string_literal_copy(T_IDX idx, /**< literal identifier */
                         string_literal_copy *str_lit_descr_p) /**< pointer to string literal copy descriptor */
{
  JERRY_ASSERT( str_lit_descr_p != NULL );

  ssize_t sz = try_get_string_by_idx( idx,
                                      str_lit_descr_p->literal_copy,
                                      sizeof(str_lit_descr_p->literal_copy));
  if ( sz > 0 )
    {
      str_lit_descr_p->str_p = str_lit_descr_p->literal_copy;
    }
  else
    {
      JERRY_ASSERT( sz < 0 );

      ssize_t req_sz = -sz;

      str_lit_descr_p->str_p = mem_HeapAllocBlock( (size_t)req_sz,
                                                  MEM_HEAP_ALLOC_SHORT_TERM);

      sz = try_get_string_by_idx( idx,
                                  str_lit_descr_p->str_p,
                                  req_sz);

      JERRY_ASSERT( sz > 0 );
    }
} /* init_string_literal */

/**
 * Free string literal copy.
 */
static void __unused
free_string_literal_copy(string_literal_copy *str_lit_descr_p) /**< string literal copy descriptor */
{
  JERRY_ASSERT( str_lit_descr_p != NULL );
  JERRY_ASSERT( str_lit_descr_p->str_p != NULL );

  if ( str_lit_descr_p->str_p == str_lit_descr_p->literal_copy )
    {
      /* copy is inside descriptor */
    }
  else
    {
      mem_HeapFreeBlock( str_lit_descr_p->str_p);
    }

  str_lit_descr_p->str_p = NULL;

  return;
} /* free_string_literal */

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
    op(nop)

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

/**
 * Assignment opcode handler.
 *
 * Note:
 *      This handler implements case of assignment of a literal's or a variable's
 *      value to a variable. Assignment to an object's property is not implemented
 *      by this opcode.
 *
 *      FIXME: Add cross link with 'property accessor assignment` opcode.
 *
 * See also: ECMA-262 v5, 11.13.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_FreeCompletionValue
 */
ecma_CompletionValue_t
opfunc_assignment (OPCODE opdata, /**< operation data */
                   struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.assignment.var_left;
  const opcode_arg_type_operand type_value_right = opdata.data.assignment.type_value_right;
  const T_IDX src_val_descr = opdata.data.assignment.value_right;

  // FIXME:
  const bool is_strict = false;

  int_data->pos++;

  ecma_Value_t right_value;

  switch ( type_value_right )
  {
    case OPCODE_ARG_TYPE_SIMPLE:
      {
        right_value = ecma_MakeSimpleValue( src_val_descr);
        break;
      }
    case OPCODE_ARG_TYPE_STRING:
      {
        string_literal_copy str_value;
        ecma_ArrayFirstChunk_t *ecma_string_p;

        init_string_literal_copy( src_val_descr, &str_value);
        ecma_string_p = ecma_NewEcmaString( str_value.str_p);
        free_string_literal_copy( &str_value);

        right_value.m_ValueType = ECMA_TYPE_STRING;
        ecma_SetPointer( right_value.m_Value, ecma_string_p);
        break;
      }
    case OPCODE_ARG_TYPE_VARIABLE:
      {
        string_literal_copy src_variable_name;
        ecma_Reference_t src_reference;

        init_string_literal_copy( src_val_descr, &src_variable_name);

        src_reference = ecma_OpGetIdentifierReference( int_data->lex_env_p,
                                                       src_variable_name.str_p,
                                                       is_strict);

        ecma_CompletionValue_t get_value_completion = ecma_op_get_value( &src_reference);

        JERRY_ASSERT( get_value_completion.type == ECMA_COMPLETION_TYPE_NORMAL
                      || get_value_completion.type == ECMA_COMPLETION_TYPE_THROW );

        ecma_FreeReference( src_reference);

        free_string_literal_copy( &src_variable_name);

        if ( get_value_completion.type == ECMA_COMPLETION_TYPE_NORMAL )
        {
          right_value = get_value_completion.value;
        } else
        {
          return get_value_completion;
        }

        break;
      }
    case OPCODE_ARG_TYPE_NUMBER:
      {
        ecma_Number_t *num_p = ecma_AllocNumber();
        *num_p = get_number_by_idx( src_val_descr);

        right_value.m_ValueType = ECMA_TYPE_NUMBER;        
        ecma_SetPointer( right_value.m_Value, num_p);

        break;
      }
    case OPCODE_ARG_TYPE_SMALLINT:
      {
        ecma_Number_t *num_p = ecma_AllocNumber();
        *num_p = src_val_descr;

        right_value.m_ValueType = ECMA_TYPE_NUMBER;        
        ecma_SetPointer( right_value.m_Value, num_p);

        break;
      }
      JERRY_UNIMPLEMENTED();
  }

  ecma_CompletionValue_t completion_value;

  string_literal_copy dst_variable_name;
  ecma_Reference_t dst_reference;

  init_string_literal_copy( dst_var_idx, &dst_variable_name);

  dst_reference = ecma_OpGetIdentifierReference( int_data->lex_env_p,
                                                 dst_variable_name.str_p,
                                                 is_strict);

  // FIXME: Move magic strings to header file and make them ecma_Char_t[]
  // FIXME: Replace strcmp with ecma_Char_t[] comparator
  if ( dst_reference.is_strict
       && ( __strcmp( (char*)dst_reference.referenced_name_p, "eval") == 0
            || __strcmp( (char*)dst_reference.referenced_name_p, "arguments") == 0 )
       && ( dst_reference.base.m_ValueType == ECMA_TYPE_OBJECT )
       && ( ecma_GetPointer( dst_reference.base.m_Value) != NULL )
       && ( ( (ecma_Object_t*) ecma_GetPointer( dst_reference.base.m_Value) )->m_IsLexicalEnvironment ) )
  {
    completion_value = ecma_MakeThrowValue( ecma_NewStandardError( ECMA_ERROR_SYNTAX));
  } else
  {
    completion_value = ecma_op_put_value( &dst_reference, right_value);
  }

  ecma_FreeValue( right_value);

  ecma_FreeReference( dst_reference);

  free_string_literal_copy( &dst_variable_name);

  return completion_value;
} /* opfunc_assignment */

/**
 * Variable declaration opcode handler.
 *
 * See also: ECMA-262 v5, 10.5 - Declaration binding instantiation (block 8).
 *
 * @return completion value
 *         Returned value is simple and so need not be freed.
 *         However, ecma_free_completion_value may be called for it, but it is a no-op.
 */
ecma_CompletionValue_t
opfunc_var_decl(OPCODE opdata, /**< operation data */
                struct __int_data *int_data __unused) /**< interpreter context */
{
  string_literal_copy variable_name;
  init_string_literal_copy( opdata.data.var_decl.variable_name, &variable_name);

  if ( ecma_IsCompletionValueNormalFalse( ecma_OpHasBinding( int_data->lex_env_p,
                                                             variable_name.str_p)) )
  {
    ecma_OpCreateMutableBinding( int_data->lex_env_p,
                                 variable_name.str_p,
                                 false); // FIXME: Pass configurableBindings

    /* Skipping SetMutableBinding as we have already checked that there were not
     * any binding with specified name in current lexical environment 
     * and CreateMutableBinding sets the created binding's value to undefined */
    JERRY_ASSERT( ecma_is_completion_value_normal_simple_value( ecma_OpGetBindingValue( int_data->lex_env_p,
                                                                                        variable_name.str_p,
                                                                                        true),
                                                                ECMA_SIMPLE_VALUE_UNDEFINED) );
  }

  free_string_literal_copy( &variable_name);

  int_data->pos++;

  return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                   ecma_MakeSimpleValue( ECMA_SIMPLE_VALUE_EMPTY),
                                   ECMA_TARGET_ID_RESERVED);
} /* opfunc_var_decl */

/**
 * Exit from script with specified status code:
 *   0 - for successful completion
 *   1 - to indicate failure.
 *
 * Note: this is not ECMA specification-defined, but internal
 *       implementation-defined opcode for end of script
 *       and assertions inside of unit tests.
 *
 * @return completion value
 *         Returned value is simple and so need not be freed.
 *         However, ecma_free_completion_value may be called for it, but it is a no-op.
 */
ecma_CompletionValue_t
opfunc_exitval(OPCODE opdata, /**< operation data */
               struct __int_data *int_data __unused) /**< interpreter context */
{
  JERRY_ASSERT( opdata.data.exitval.status_code == 0
                || opdata.data.exitval.status_code == 1 );

  ecma_Value_t exit_status = ecma_MakeSimpleValue( opdata.data.exitval.status_code == 0 ? ECMA_SIMPLE_VALUE_TRUE
                                                                                        : ECMA_SIMPLE_VALUE_FALSE);
  return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_EXIT,
                                   exit_status,
                                   ECMA_TARGET_ID_RESERVED);
} /* opfunc_exitval */


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
GETOP_IMPL_3 (assignment, var_left, type_value_right, value_right)
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
GETOP_IMPL_1 (exitval, status_code)
GETOP_IMPL_1 (retval, ret_value)
GETOP_IMPL_0 (ret)
GETOP_IMPL_0 (nop)
GETOP_IMPL_1 (loop_inf, loop_root)
GETOP_IMPL_3 (loop_init_num, start, stop, step)
GETOP_IMPL_2 (loop_precond_begin_num, condition, after_loop_op)
GETOP_IMPL_3 (loop_precond_end_num, iterator, step, precond_begin)
GETOP_IMPL_2 (loop_postcond, condition, body_root)
GETOP_IMPL_1 (var_decl, variable_name)

