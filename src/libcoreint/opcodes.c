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
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-helpers.h"
#include "ecma-number-arithmetic.h"
#include "ecma-operations.h"
#include "globals.h"
#include "interpreter.h"
#include "jerry-libc.h"
#include "mem-heap.h"
#include "opcodes.h"

/**
 * Note:
 *      The note describes exception handling in opcode handlers that perform operations,
 *      that can throw exceptions, and do not themself handle the exceptions.
 *
 *      Generally, each opcode handler consists of sequence of operations.
 *      Some of these operations (exceptionable operations) can throw exceptions and other - cannot.
 *
 *      1. At the beginning of the handler there should be declared opcode handler's 'return value' variable.
 *
 *      2. All exceptionable operations except the last should be enclosed in TRY_CATCH macro.
 *         All subsequent operations in the opcode handler should be placed into block between
 *         the TRY_CATCH and corresponding FINALIZE.
 *
 *      3. The last exceptionable's operation result should be assigned directly to opcode handler's
 *         'return value' variable without using TRY_CATCH macro.
 *
 *      4. After last FINALIZE statement there should be only one operator.
 *         The operator should return from the opcode handler with it's 'return value'.
 *
 *      5. No other operations with opcode handler's 'return value' variable should be performed.
 */

/**
 * The macro defines try-block that initializes variable 'var' with 'op'
 * and checks for exceptions that might be thrown during initialization.
 *
 * If no exception was thrown, then code after the try-block is executed.
 * Otherwise, throw-completion value is just copied to return_value.
 *
 * Note:
 *      Each TRY_CATCH should have it's own corresponding FINALIZE
 *      statement with same argument as corresponding TRY_CATCH's first argument.
 */
#define TRY_CATCH(var, op, return_value) \
  ecma_CompletionValue_t var = op; \
  if ( unlikely( ecma_is_completion_value_throw( var) ) ) \
  { \
    return_value = ecma_copy_completion_value( var); \
  } \
  else \
  { \
    JERRY_ASSERT( ecma_is_completion_value_normal( var) )

/**
 * The macro marks end of code block that is executed if no exception
 * was catched by corresponding TRY_CATCH and frees variable,
 * initialized by the TRY_CATCH.
 *
 * Note:
 *      Each TRY_CATCH should be followed by FINALIZE with same
 *      argument as corresponding TRY_CATCH's first argument.
 */
#define FINALIZE(var) } \
  ecma_free_completion_value( var)

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

/**
 * Perform so-called 'strict eval or arguments reference' check
 * that is used in definition of several statement handling algorithms,
 * but has no ECMA-defined name.
 *
 * @return true - if ref is strict reference
 *                   and it's base is lexical environment
 *                   and it's referenced name is 'eval' or 'arguments';
 *         false - otherwise.
 */
static bool
do_strict_eval_arguments_check( ecma_Reference_t ref) /**< ECMA-reference */
{
  FIXME( Move magic strings to header file and make them ecma_Char_t[] );
  FIXME( Replace strcmp with ecma_Char_t[] comparator );
  return ( ref.is_strict
          && ( __strcmp( (char*)ref.referenced_name_p, "eval") == 0
              || __strcmp( (char*)ref.referenced_name_p, "arguments") == 0 )
          && ( ref.base.ValueType == ECMA_TYPE_OBJECT )
          && ( ecma_GetPointer( ref.base.Value) != NULL )
          && ( ( (ecma_Object_t*) ecma_GetPointer( ref.base.Value) )->IsLexicalEnvironment ) );
} /* do_strict_eval_arguments_check */

/**
 * Get variable's value.
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_CompletionValue_t
get_variable_value(struct __int_data *int_data, /**< interpreter context */
                   T_IDX var_idx, /**< variable identifier */
                   bool do_eval_or_arguments_check) /** run 'strict eval or arguments reference' check
                                                        See also: do_strict_eval_arguments_check */
{
  string_literal_copy var_name;
  ecma_Reference_t ref;
  ecma_CompletionValue_t ret_value;

  init_string_literal_copy( var_idx, &var_name);
  ref = ecma_OpGetIdentifierReference( int_data->lex_env_p,
                                       var_name.str_p,
                                       int_data->is_strict);

  if ( unlikely( do_eval_or_arguments_check
                 && do_strict_eval_arguments_check( ref) ) )
    {
      ret_value = ecma_MakeThrowValue( ecma_NewStandardError( ECMA_ERROR_SYNTAX));
    }
  else
    {
      ret_value = ecma_op_get_value( ref);
    }

  ecma_FreeReference( ref);
  free_string_literal_copy( &var_name);

  return ret_value;
} /* get_variable_value */

/**
 * Set variable's value.
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_CompletionValue_t
set_variable_value(struct __int_data *int_data, /**< interpreter context */
                   T_IDX var_idx, /**< variable identifier */
                   ecma_Value_t value) /**< value to set */
{
  string_literal_copy var_name;
  ecma_Reference_t ref;
  ecma_CompletionValue_t ret_value;

  init_string_literal_copy( var_idx, &var_name);
  ref = ecma_OpGetIdentifierReference( int_data->lex_env_p,
                                       var_name.str_p,
                                       int_data->is_strict);

  if ( unlikely( do_strict_eval_arguments_check( ref) ) )
    {
      ret_value = ecma_MakeThrowValue( ecma_NewStandardError( ECMA_ERROR_SYNTAX));
    }
  else
    {
      ret_value = ecma_op_put_value( ref, value);
    }

  ecma_FreeReference( ref);
  free_string_literal_copy( &var_name);

  return ret_value;
} /* set_variable_value */

/**
 * Number arithmetic operations.
 */
typedef enum
{
  number_arithmetic_addition, /**< addition */
  number_arithmetic_substraction, /**< substraction */
  number_arithmetic_multiplication, /**< multiplication */
  number_arithmetic_division, /**< division */
  number_arithmetic_remainder, /**< remainder calculation */
} number_arithmetic_op;

/**
 * Perform ECMA number arithmetic operation.
 *
 * The algorithm of the operation is following:
 *   leftNum = ToNumber( leftValue);
 *   rightNum = ToNumber( rightValue);
 *   result = leftNum ArithmeticOp rightNum;
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_CompletionValue_t
do_number_arithmetic(struct __int_data *int_data, /**< interpreter context */
                     T_IDX dst_var_idx, /**< destination variable identifier */
                     number_arithmetic_op op, /**< number arithmetic operation */
                     ecma_Value_t left_value, /**< left value */
                     ecma_Value_t right_value) /** right value */
{
  ecma_CompletionValue_t ret_value;

  TRY_CATCH(num_left_value, ecma_op_to_number( left_value), ret_value);
  TRY_CATCH(num_right_value, ecma_op_to_number( right_value), ret_value);

  ecma_Number_t *left_p, *right_p, *res_p;
  left_p = (ecma_Number_t*)ecma_GetPointer( num_left_value.value.Value);
  right_p = (ecma_Number_t*)ecma_GetPointer( num_right_value.value.Value);

  res_p = ecma_AllocNumber();

  switch ( op )
    {
    case number_arithmetic_addition:
      *res_p = ecma_op_number_add( *left_p, *right_p);
      break;
    case number_arithmetic_substraction:
      *res_p = ecma_op_number_substract( *left_p, *right_p);
      break;
    case number_arithmetic_multiplication:
      *res_p = ecma_op_number_multiply( *left_p, *right_p);
      break;
    case number_arithmetic_division:
      *res_p = ecma_op_number_divide( *left_p, *right_p);
      break;
    case number_arithmetic_remainder:
      *res_p = ecma_op_number_remainder( *left_p, *right_p);
      break;
    }

  ret_value = set_variable_value(int_data,
                                 dst_var_idx,
                                 ecma_MakeNumberValue( res_p));

  ecma_DeallocNumber( res_p);

  FINALIZE( num_right_value);
  FINALIZE( num_left_value);

  return ret_value;
} /* do_number_arithmetic */

#define OP_UNIMPLEMENTED_LIST(op) \
    op(is_true_jmp)                     \
    op(is_false_jmp)                    \
    op(loop_init_num)                   \
    op(loop_precond_begin_num)          \
    op(loop_precond_end_num)            \
    op(loop_postcond)                   \
    op(call_0)                          \
    op(call_n)                          \
    op(func_decl_1)                     \
    op(func_decl_2)                     \
    op(func_decl_n)                     \
    op(varg_1_end)                      \
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
    op(jmp_up)                          \
    op(jmp_down)                        \
    op(nop)                             \
    op(construct_0)                     \
    op(construct_1)                     \
    op(construct_n)                     \
    op(func_decl_0)                     \
    op(func_expr_0)                     \
    op(func_expr_1)                     \
    op(func_expr_n)                     \
    op(array_0)                         \
    op(array_1)                         \
    op(array_2)                         \
    op(array_n)                         \
    op(prop)                            \
    op(prop_access)                     \
    op(prop_get_decl)                   \
    op(prop_set_decl)                   \
    op(obj_0)                           \
    op(obj_1)                           \
    op(obj_2)                           \
    op(obj_n)                           \
    op(this)                            \
    op(delete)                          \
    op(typeof)                          \
    op(with)                            \
    op(end_with)                        \
    op(logical_not)                     \
    op(b_not)                           \
    op(instanceof)                      \
    op(in)                   

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
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_CompletionValue_t
opfunc_assignment (OPCODE opdata, /**< operation data */
                   struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.assignment.var_left;
  const opcode_arg_type_operand type_value_right = opdata.data.assignment.type_value_right;
  const T_IDX src_val_descr = opdata.data.assignment.value_right;

  int_data->pos++;

  ecma_CompletionValue_t get_value_completion;

  switch ( type_value_right )
  {
    case OPCODE_ARG_TYPE_SIMPLE:
      {
        get_value_completion = ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                                         ecma_MakeSimpleValue( src_val_descr),
                                                         ECMA_TARGET_ID_RESERVED);
        break;
      }
    case OPCODE_ARG_TYPE_STRING:
      {
        string_literal_copy str_value;
        ecma_ArrayFirstChunk_t *ecma_string_p;

        init_string_literal_copy( src_val_descr, &str_value);
        ecma_string_p = ecma_NewEcmaString( str_value.str_p);
        free_string_literal_copy( &str_value);

        get_value_completion = ecma_MakeCompletionValue(ECMA_COMPLETION_TYPE_NORMAL,
                                                        ecma_make_string_value( ecma_string_p),
                                                        ECMA_TARGET_ID_RESERVED);
        break;
      }
    case OPCODE_ARG_TYPE_VARIABLE:
      {
        get_value_completion = get_variable_value(int_data,
                                                  src_val_descr,
                                                  false);

        break;
      }
    case OPCODE_ARG_TYPE_NUMBER:
      {
        ecma_Number_t *num_p = ecma_AllocNumber();
        *num_p = get_number_by_idx( src_val_descr);

        get_value_completion = ecma_MakeCompletionValue(ECMA_COMPLETION_TYPE_NORMAL,
                                                        ecma_MakeNumberValue( num_p),
                                                        ECMA_TARGET_ID_RESERVED);
        break;
      }
    case OPCODE_ARG_TYPE_SMALLINT:
      {
        ecma_Number_t *num_p = ecma_AllocNumber();
        *num_p = src_val_descr;

        get_value_completion = ecma_MakeCompletionValue(ECMA_COMPLETION_TYPE_NORMAL,
                                                        ecma_MakeNumberValue( num_p),
                                                        ECMA_TARGET_ID_RESERVED);
        break;
      }
      JERRY_UNIMPLEMENTED();
  }

  if ( unlikely( ecma_is_completion_value_throw( get_value_completion) ) )
    {
      return get_value_completion;
    }
  else
    {
      JERRY_ASSERT( ecma_is_completion_value_normal( get_value_completion) );
      
      ecma_CompletionValue_t assignment_completion_value = set_variable_value(int_data,
                                                                              dst_var_idx,
                                                                              get_value_completion.value);
      
      ecma_free_completion_value( get_value_completion);

      return assignment_completion_value;
    }
} /* opfunc_assignment */

/**
 * Addition opcode handler.
 *
 * See also: ECMA-262 v5, 11.6.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_CompletionValue_t
opfunc_addition(OPCODE opdata, /**< operation data */
                struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.addition.dst;
  const T_IDX left_var_idx = opdata.data.addition.var_left;
  const T_IDX right_var_idx = opdata.data.addition.var_right;

  int_data->pos++;

  ecma_CompletionValue_t ret_value;

  TRY_CATCH(left_value, get_variable_value( int_data, left_var_idx, false), ret_value);
  TRY_CATCH(right_value, get_variable_value( int_data, right_var_idx, false), ret_value);
  TRY_CATCH(prim_left_value, ecma_op_to_primitive( left_value.value), ret_value);
  TRY_CATCH(prim_right_value, ecma_op_to_primitive( right_value.value), ret_value);

  if ( prim_left_value.value.ValueType == ECMA_TYPE_STRING
       || prim_right_value.value.ValueType == ECMA_TYPE_STRING )
    {
      JERRY_UNIMPLEMENTED();
    }
  else
    {
      ret_value = do_number_arithmetic(int_data,
                                       dst_var_idx,
                                       number_arithmetic_addition,
                                       prim_left_value.value,
                                       prim_right_value.value);
    }

  FINALIZE(prim_right_value);
  FINALIZE(prim_left_value);
  FINALIZE(right_value);
  FINALIZE(left_value);

  return ret_value;
} /* opfunc_addition */

/**
 * Substraction opcode handler.
 *
 * See also: ECMA-262 v5, 11.6.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_CompletionValue_t
opfunc_substraction(OPCODE opdata, /**< operation data */
                    struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.substraction.dst;
  const T_IDX left_var_idx = opdata.data.substraction.var_left;
  const T_IDX right_var_idx = opdata.data.substraction.var_right;

  int_data->pos++;

  ecma_CompletionValue_t ret_value;

  TRY_CATCH(left_value, get_variable_value( int_data, left_var_idx, false), ret_value);
  TRY_CATCH(right_value, get_variable_value( int_data, right_var_idx, false), ret_value);

  ret_value = do_number_arithmetic(int_data,
                                   dst_var_idx,
                                   number_arithmetic_substraction,
                                   left_value.value,
                                   right_value.value);

  FINALIZE(right_value);
  FINALIZE(left_value);

  return ret_value;
} /* opfunc_substraction */

/**
 * Multiplication opcode handler.
 *
 * See also: ECMA-262 v5, 11.5, 11.5.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_CompletionValue_t
opfunc_multiplication(OPCODE opdata, /**< operation data */
                      struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.multiplication.dst;
  const T_IDX left_var_idx = opdata.data.multiplication.var_left;
  const T_IDX right_var_idx = opdata.data.multiplication.var_right;

  int_data->pos++;

  ecma_CompletionValue_t ret_value;

  TRY_CATCH(left_value, get_variable_value( int_data, left_var_idx, false), ret_value);
  TRY_CATCH(right_value, get_variable_value( int_data, right_var_idx, false), ret_value);

  ret_value = do_number_arithmetic(int_data,
                                   dst_var_idx,
                                   number_arithmetic_multiplication,
                                   left_value.value,
                                   right_value.value);

  FINALIZE(right_value);
  FINALIZE(left_value);

  return ret_value;
} /* opfunc_multiplication */

/**
 * Division opcode handler.
 *
 * See also: ECMA-262 v5, 11.5, 11.5.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_CompletionValue_t
opfunc_division(OPCODE opdata, /**< operation data */
                struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.division.dst;
  const T_IDX left_var_idx = opdata.data.division.var_left;
  const T_IDX right_var_idx = opdata.data.division.var_right;

  int_data->pos++;

  ecma_CompletionValue_t ret_value;

  TRY_CATCH(left_value, get_variable_value( int_data, left_var_idx, false), ret_value);
  TRY_CATCH(right_value, get_variable_value( int_data, right_var_idx, false), ret_value);

  ret_value = do_number_arithmetic(int_data,
                                   dst_var_idx,
                                   number_arithmetic_division,
                                   left_value.value,
                                   right_value.value);

  FINALIZE(right_value);
  FINALIZE(left_value);

  return ret_value;
} /* opfunc_division */

/**
 * Remainder calculation opcode handler.
 *
 * See also: ECMA-262 v5, 11.5, 11.5.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_CompletionValue_t
opfunc_remainder(OPCODE opdata, /**< operation data */
                 struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.remainder.dst;
  const T_IDX left_var_idx = opdata.data.remainder.var_left;
  const T_IDX right_var_idx = opdata.data.remainder.var_right;

  int_data->pos++;

  ecma_CompletionValue_t ret_value;

  TRY_CATCH(left_value, get_variable_value( int_data, left_var_idx, false), ret_value);
  TRY_CATCH(right_value, get_variable_value( int_data, right_var_idx, false), ret_value);

  ret_value = do_number_arithmetic(int_data,
                                   dst_var_idx,
                                   number_arithmetic_remainder,
                                   left_value.value,
                                   right_value.value);

  FINALIZE(right_value);
  FINALIZE(left_value);

  return ret_value;
} /* opfunc_remainder */

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
    FIXME( Pass configurableBindings that is true if and only if current code is eval code );
    ecma_OpCreateMutableBinding( int_data->lex_env_p,
                                 variable_name.str_p,
                                 false);

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
GETOP_IMPL_2 (call_0, lhs, name_lit_idx)
GETOP_IMPL_3 (call_1, lhs, name_lit_idx, arg1_lit_idx)
GETOP_IMPL_3 (call_n, lhs, name_lit_idx, arg1_lit_idx)
GETOP_IMPL_2 (func_decl_1, name_lit_idx, arg1_lit_idx)
GETOP_IMPL_3 (func_decl_2, name_lit_idx, arg1_lit_idx, arg2_lit_idx)
GETOP_IMPL_3 (func_decl_n, name_lit_idx, arg1_lit_idx, arg2_lit_idx)
GETOP_IMPL_1 (varg_1_end, arg1_lit_idx)
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
GETOP_IMPL_2 (b_not, dst, var_right)
GETOP_IMPL_2 (logical_not, dst, var_right)
GETOP_IMPL_3 (instanceof, dst, var_left, var_right)
GETOP_IMPL_3 (in, dst, var_left, var_right)
GETOP_IMPL_2 (construct_0, lhs, name_lit_idx)
GETOP_IMPL_3 (construct_1, lhs, name_lit_idx, arg1_lit_idx)
GETOP_IMPL_3 (construct_n, lhs, name_lit_idx, arg1_lit_idx)
GETOP_IMPL_1 (func_decl_0, name_lit_idx)
GETOP_IMPL_2 (func_expr_0, lhs, name_lit_idx)
GETOP_IMPL_3 (func_expr_1, lhs, name_lit_idx, arg1_lit_idx)
GETOP_IMPL_3 (func_expr_n, lhs, name_lit_idx, arg1_lit_idx)
GETOP_IMPL_1 (array_0, lhs)
GETOP_IMPL_2 (array_1, lhs, elem1)
GETOP_IMPL_3 (array_2, lhs, elem1, elem2)
GETOP_IMPL_3 (array_n, lhs, elem1, elem2)
GETOP_IMPL_3 (prop, lhs, name, value)
GETOP_IMPL_3 (prop_access, lhs, obj, prop)
GETOP_IMPL_2 (prop_get_decl, lhs, prop)
GETOP_IMPL_3 (prop_set_decl, lhs, prop, arg)
GETOP_IMPL_1 (obj_0, lhs)
GETOP_IMPL_2 (obj_1, lhs, arg1)
GETOP_IMPL_3 (obj_2, lhs, arg1, arg2)
GETOP_IMPL_3 (obj_n, lhs, arg1, arg2)
GETOP_IMPL_1 (this, lhs)
GETOP_IMPL_2 (delete, lhs, obj)
GETOP_IMPL_2 (typeof, lhs, obj)
GETOP_IMPL_1 (with, expr)
GETOP_IMPL_0 (end_with)

