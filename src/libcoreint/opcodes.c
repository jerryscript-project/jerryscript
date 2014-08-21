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
#include "ecma-comparison.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-magic-strings.h"
#include "ecma-number-arithmetic.h"
#include "ecma-operations.h"
#include "ecma-try-catch-macro.h"
#include "globals.h"
#include "interpreter.h"
#include "jerry-libc.h"
#include "mem-heap.h"
#include "opcodes.h"

#include "actuators.h"
#include "common-io.h"
#include "sensors.h"

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
 *      2. All exceptionable operations except the last should be enclosed in ECMA_TRY_CATCH macro.
 *         All subsequent operations in the opcode handler should be placed into block between
 *         the ECMA_TRY_CATCH and corresponding ECMA_FINALIZE.
 *
 *      3. The last exceptionable's operation result should be assigned directly to opcode handler's
 *         'return value' variable without using ECMA_TRY_CATCH macro.
 *
 *      4. After last ECMA_FINALIZE statement there should be only one operator.
 *         The operator should return from the opcode handler with it's 'return value'.
 *
 *      5. No other operations with opcode handler's 'return value' variable should be performed.
 */

/**
 * String literal copy descriptor.
 */
typedef struct
{
  ecma_char_t *str_p; /**< pointer to copied string literal */
  ecma_char_t literal_copy[32]; /**< buffer with string literal,
                                     if it is stored locally
                                     (i.e. not in the heap) */
} string_literal_copy;

/**
 * Initialize string literal copy.
 */
static void
init_string_literal_copy (T_IDX idx, /**< literal identifier */
                          string_literal_copy *str_lit_descr_p) /**< pointer to string literal copy descriptor */
{
  JERRY_ASSERT (str_lit_descr_p != NULL);

  ssize_t sz = try_get_string_by_idx (idx,
                                      str_lit_descr_p->literal_copy,
                                      sizeof (str_lit_descr_p->literal_copy));
  if (sz > 0)
  {
    str_lit_descr_p->str_p = str_lit_descr_p->literal_copy;
  }
  else
  {
    JERRY_ASSERT (sz < 0);

    ssize_t req_sz = -sz;

    str_lit_descr_p->str_p = mem_heap_alloc_block ((size_t) req_sz,
                                                   MEM_HEAP_ALLOC_SHORT_TERM);

    sz = try_get_string_by_idx (idx,
                                str_lit_descr_p->str_p,
                                req_sz);

    JERRY_ASSERT (sz > 0);
  }
} /* init_string_literal */

/**
 * Free string literal copy.
 */
static void
free_string_literal_copy (string_literal_copy *str_lit_descr_p) /**< string literal copy descriptor */
{
  JERRY_ASSERT (str_lit_descr_p != NULL);
  JERRY_ASSERT (str_lit_descr_p->str_p != NULL);

  if (str_lit_descr_p->str_p == str_lit_descr_p->literal_copy)
  {
    /* copy is inside descriptor */
  }
  else
  {
    mem_heap_free_block (str_lit_descr_p->str_p);
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
do_strict_eval_arguments_check (ecma_reference_t ref) /**< ECMA-reference */
{
  bool ret;

  if (ref.is_strict
      && (ref.base.value_type == ECMA_TYPE_OBJECT)
      && (ECMA_GET_POINTER (ref.base.value) != NULL)
      && (((ecma_object_t*) ECMA_GET_POINTER (ref.base.value))->is_lexical_environment))
  {
    ecma_string_t* magic_string_eval = ecma_get_magic_string (ECMA_MAGIC_STRING_EVAL);
    ecma_string_t* magic_string_arguments = ecma_get_magic_string (ECMA_MAGIC_STRING_ARGUMENTS);

    ret = (ecma_compare_ecma_string_to_ecma_string (ref.referenced_name_p,
                                                    magic_string_eval) == 0
           || ecma_compare_ecma_string_to_ecma_string (ref.referenced_name_p,
                                                       magic_string_arguments) == 0);

    ecma_deref_ecma_string (magic_string_eval);
    ecma_deref_ecma_string (magic_string_arguments);
  }
  else
  {
    ret = false;
  }

  return ret;
} /* do_strict_eval_arguments_check */

/**
 * Get variable's value.
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
get_variable_value (struct __int_data *int_data, /**< interpreter context */
                    T_IDX var_idx, /**< variable identifier */
                    bool do_eval_or_arguments_check) /** run 'strict eval or arguments reference' check
                                                        See also: do_strict_eval_arguments_check */
{
  ecma_completion_value_t ret_value;

  if (var_idx >= int_data->min_reg_num
      && var_idx <= int_data->max_reg_num)
  {
    ecma_value_t reg_value = int_data->regs_p[ var_idx - int_data->min_reg_num ];

    JERRY_ASSERT (!ecma_is_value_empty (reg_value));

    ret_value = ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
                                            ecma_copy_value (reg_value, true),
                                            ECMA_TARGET_ID_RESERVED);
  }
  else
  {
    ecma_reference_t ref;

    ecma_string_t *var_name_string_p = ecma_new_ecma_string_from_lit_index (var_idx);

    ref = ecma_op_get_identifier_reference (int_data->lex_env_p,
                                            var_name_string_p,
                                            int_data->is_strict);

    if (unlikely (do_eval_or_arguments_check
                  && do_strict_eval_arguments_check (ref)))
    {
      ret_value = ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_SYNTAX));
    }
    else
    {
      ret_value = ecma_op_get_value (ref);
    }

    ecma_deref_ecma_string (var_name_string_p);
    ecma_free_reference (ref);
  }

  return ret_value;
} /* get_variable_value */

/**
 * Set variable's value.
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
set_variable_value (struct __int_data *int_data, /**< interpreter context */
                    T_IDX var_idx, /**< variable identifier */
                    ecma_value_t value) /**< value to set */
{
  ecma_completion_value_t ret_value;

  if (var_idx >= int_data->min_reg_num
      && var_idx <= int_data->max_reg_num)
  {
    ecma_value_t reg_value = int_data->regs_p[ var_idx - int_data->min_reg_num ];

    if (!ecma_is_value_empty (reg_value))
    {
      ecma_free_value (reg_value, true);
    }

    int_data->regs_p[ var_idx - int_data->min_reg_num ] = ecma_copy_value (value, true);

    ret_value = ecma_make_empty_completion_value ();
  }
  else
  {
    ecma_reference_t ref;

    ecma_string_t *var_name_string_p = ecma_new_ecma_string_from_lit_index (var_idx);

    ref = ecma_op_get_identifier_reference (int_data->lex_env_p,
                                            var_name_string_p,
                                            int_data->is_strict);

    if (unlikely (do_strict_eval_arguments_check (ref)))
    {
      ret_value = ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_SYNTAX));
    }
    else
    {
      ret_value = ecma_op_put_value (ref, value);
    }

    ecma_deref_ecma_string (var_name_string_p);
    ecma_free_reference (ref);
  }

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
 * Number bitwise logic operations.
 */
typedef enum
{
  number_bitwise_logic_and, /**< bitwise AND calculation */
  number_bitwise_logic_or, /**< bitwise OR calculation */
  number_bitwise_logic_xor, /**< bitwise XOR calculation */
} number_bitwise_logic_op;

/**
 * Perform ECMA number arithmetic operation.
 *
 * The algorithm of the operation is following:
 *   leftNum = ToNumber (leftValue);
 *   rightNum = ToNumber (rightValue);
 *   result = leftNum ArithmeticOp rightNum;
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
do_number_arithmetic (struct __int_data *int_data, /**< interpreter context */
                      T_IDX dst_var_idx, /**< destination variable identifier */
                      number_arithmetic_op op, /**< number arithmetic operation */
                      ecma_value_t left_value, /**< left value */
                      ecma_value_t right_value) /** right value */
{
  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (num_left_value, ecma_op_to_number (left_value), ret_value);
  ECMA_TRY_CATCH (num_right_value, ecma_op_to_number (right_value), ret_value);

  ecma_number_t *left_p, *right_p, *res_p;
  left_p = (ecma_number_t*) ECMA_GET_POINTER (num_left_value.value.value);
  right_p = (ecma_number_t*) ECMA_GET_POINTER (num_right_value.value.value);

  res_p = ecma_alloc_number ();

  switch (op)
  {
    case number_arithmetic_addition:
    {
      *res_p = ecma_op_number_add (*left_p, *right_p);
      break;
    }
    case number_arithmetic_substraction:
    {
      *res_p = ecma_op_number_substract (*left_p, *right_p);
      break;
    }
    case number_arithmetic_multiplication:
    {
      *res_p = ecma_op_number_multiply (*left_p, *right_p);
      break;
    }
    case number_arithmetic_division:
    {
      *res_p = ecma_op_number_divide (*left_p, *right_p);
      break;
    }
    case number_arithmetic_remainder:
    {
      *res_p = ecma_op_number_remainder (*left_p, *right_p);
      break;
    }
  }

  ret_value = set_variable_value (int_data,
                                  dst_var_idx,
                                  ecma_make_number_value (res_p));

  ecma_dealloc_number (res_p);

  ECMA_FINALIZE (num_right_value);
  ECMA_FINALIZE (num_left_value);

  return ret_value;
} /* do_number_arithmetic */

/**
 * Perform ECMA number logic operation.
 *
 * The algorithm of the operation is following:
 *   leftNum = ToNumber (leftValue);
 *   rightNum = ToNumber (rightValue);
 *   result = leftNum BitwiseLogicOp rightNum;
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
do_number_bitwise_logic (struct __int_data *int_data, /**< interpreter context */
                         T_IDX dst_var_idx, /**< destination variable identifier */
                         number_bitwise_logic_op op, /**< number bitwise logic operation */
                         ecma_value_t left_value, /**< left value */
                         ecma_value_t right_value) /** right value */
{
  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (num_left_value, ecma_op_to_number (left_value), ret_value);
  ECMA_TRY_CATCH (num_right_value, ecma_op_to_number (right_value), ret_value);

  ecma_number_t *left_p, *right_p;
  left_p = (ecma_number_t*) ECMA_GET_POINTER (num_left_value.value.value);
  right_p = (ecma_number_t*) ECMA_GET_POINTER (num_right_value.value.value);

  uint32_t left_uint32, right_uint32, res = 0;

  left_uint32 = ecma_number_to_uint32 (*left_p);
  right_uint32 = ecma_number_to_uint32 (*right_p);

  switch (op)
  {
    case number_bitwise_logic_and:
    {
      res = left_uint32 & right_uint32;
      break;
    }
    case number_bitwise_logic_or:
    {
      res = left_uint32 | right_uint32;
      break;
    }
    case number_bitwise_logic_xor:
    {
      res = left_uint32 ^ right_uint32;
      break;
    }
  }

  ecma_number_t* res_p = ecma_alloc_number ();
  *res_p = ecma_uint32_to_number (res);

  ret_value = set_variable_value (int_data,
                                  dst_var_idx,
                                  ecma_make_number_value (res_p));

  ecma_dealloc_number (res_p);

  ECMA_FINALIZE (num_right_value);
  ECMA_FINALIZE (num_left_value);

  return ret_value;
} /* do_number_bitwise_logic */

#define OP_UNIMPLEMENTED_LIST(op) \
    op (call_n)                          \
    op (func_decl_n)                     \
    op (varg_list)                       \
    op (retval)                          \
    op (construct_decl)                  \
    op (array_decl)                      \
    op (prop)                            \
    op (prop_get_decl)                   \
    op (prop_set_decl)                   \
    op (obj_decl)                        \
    op (this)                            \
    op (delete)                          \
    op (typeof)                          \
    op (with)                            \
    op (end_with)                        \
    op (logical_not)                     \
    op (logical_and)                     \
    op (logical_or)                      \
    op (b_not)                           \
    op (b_shift_left)                    \
    op (b_shift_right)                   \
    op (b_shift_uright)                  \
    op (instanceof)                      \
    op (in)                              \
    static char __unused unimplemented_list_end

#define DEFINE_UNIMPLEMENTED_OP(op) \
  ecma_completion_value_t opfunc_ ## op (OPCODE opdata, struct __int_data *int_data) \
  { \
    JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (opdata, int_data); \
  }

OP_UNIMPLEMENTED_LIST (DEFINE_UNIMPLEMENTED_OP);
#undef DEFINE_UNIMPLEMENTED_OP

/**
 * 'Nop' opcode handler.
 */
ecma_completion_value_t
opfunc_nop (OPCODE opdata __unused, /**< operation data */
            struct __int_data *int_data) /**< interpreter context */
{
  int_data->pos++;

  return ecma_make_empty_completion_value ();
} /* opfunc_nop */

ecma_completion_value_t
opfunc_call_1 (OPCODE opdata __unused, struct __int_data *int_data)
{
  ecma_completion_value_t ret_value;
  ret_value = ecma_make_empty_completion_value ();

  int_data->pos++;

  ECMA_TRY_CATCH (arg_value, get_variable_value (int_data, opdata.data.call_1.arg1_lit_idx, false), ret_value);


  /**********************************************************/

  FIXME (/* Native call, i.e. opfunc_native_call */);
  bool is_native_call = true;

  string_literal_copy function_name;
  init_string_literal_copy (opdata.data.call_1.name_lit_idx, &function_name);

  if (!__strcmp ((const char*) function_name.str_p, "LEDToggle"))
  {
    JERRY_ASSERT (arg_value.value.value_type == ECMA_TYPE_NUMBER);
    ecma_number_t * num_p = (ecma_number_t*) ECMA_GET_POINTER (arg_value.value.value);
    uint32_t int_num = (uint32_t) * num_p;
    led_toggle (int_num);
    ret_value = ecma_make_empty_completion_value ();
  }
  else if (!__strcmp ((const char*) function_name.str_p, "LEDOn"))
  {
    JERRY_ASSERT (arg_value.value.value_type == ECMA_TYPE_NUMBER);
    ecma_number_t * num_p = (ecma_number_t*) ECMA_GET_POINTER (arg_value.value.value);
    uint32_t int_num = (uint32_t) * num_p;
    led_on (int_num);
    ret_value = ecma_make_empty_completion_value ();
  }
  else if (!__strcmp ((const char*) function_name.str_p, "LEDOff"))
  {
    JERRY_ASSERT (arg_value.value.value_type == ECMA_TYPE_NUMBER);
    ecma_number_t * num_p = (ecma_number_t*) ECMA_GET_POINTER (arg_value.value.value);
    uint32_t int_num = (uint32_t) * num_p;
    led_off (int_num);
    ret_value = ecma_make_empty_completion_value ();
  }
  else if (!__strcmp ((const char*) function_name.str_p, "LEDOnce"))
  {
    JERRY_ASSERT (arg_value.value.value_type == ECMA_TYPE_NUMBER);
    ecma_number_t * num_p = (ecma_number_t*) ECMA_GET_POINTER (arg_value.value.value);
    uint32_t int_num = (uint32_t) * num_p;
    led_blink_once (int_num);
    ret_value = ecma_make_empty_completion_value ();
  }
  else if (!__strcmp ((const char*) function_name.str_p, "wait"))
  {
    JERRY_ASSERT (arg_value.value.value_type == ECMA_TYPE_NUMBER);
    ecma_number_t * num_p = (ecma_number_t*) ECMA_GET_POINTER (arg_value.value.value);
    uint32_t int_num = (uint32_t) * num_p;
    wait_ms (int_num);
    ret_value = ecma_make_empty_completion_value ();
  }
  else if (!__strcmp ((const char *) function_name.str_p, "exit"))
  {
    JERRY_ASSERT (arg_value.value.value_type == ECMA_TYPE_NUMBER);
    ecma_number_t * num_p = (ecma_number_t*) ECMA_GET_POINTER (arg_value.value.value);
    uint32_t int_num = (uint32_t) * num_p;
    ecma_value_t exit_status = ecma_make_simple_value (int_num == 0 ? ECMA_SIMPLE_VALUE_TRUE
                                                       : ECMA_SIMPLE_VALUE_FALSE);
    ret_value = ecma_make_completion_value (ECMA_COMPLETION_TYPE_EXIT,
                                            exit_status,
                                            ECMA_TARGET_ID_RESERVED);
  }
  else
  {
    is_native_call = false;
  }

  if (is_native_call)
  {
#ifdef __TARGET_HOST_x64
    __printf ("%d::op_call_1:idx:%d:%d\t",
              int_data->pos,
              opdata.data.call_1.name_lit_idx,
              opdata.data.call_1.arg1_lit_idx);

    __printf ("%s\n", function_name.str_p);
#endif
  }

  free_string_literal_copy (&function_name);

  /**********************************************************/


  if (!is_native_call)
  {
    const T_IDX func_name_lit_idx = opdata.data.call_1.name_lit_idx;
    const T_IDX lhs_var_idx = opdata.data.call_1.lhs;

    ECMA_TRY_CATCH (func_value, get_variable_value (int_data, func_name_lit_idx, false), ret_value);

    if (!ecma_op_is_callable (func_value.value))
    {
      ret_value = ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
    else
    {
      ecma_object_t *func_obj_p = ECMA_GET_POINTER (func_value.value.value);

      ECMA_TRY_CATCH (this_value, ecma_op_implicit_this_value (int_data->lex_env_p), ret_value);

      ret_value = ecma_op_function_call (func_obj_p, this_value.value, &arg_value.value, 1);

      if (ret_value.type == ECMA_COMPLETION_TYPE_RETURN)
      {
        ecma_value_t returned_value = ret_value.value;

        ret_value = set_variable_value (int_data, lhs_var_idx, returned_value);

        ecma_free_value (returned_value, true);
      }

      ECMA_FINALIZE (this_value);
    }

    ECMA_FINALIZE (func_value);
  }

  ECMA_FINALIZE (arg_value);

  return ret_value;
}

/**
 * 'Jump if true' opcode handler.
 *
 * Note:
 *      the opcode changes current opcode position to specified opcode index
 *      if argument evaluates to true.
 */
ecma_completion_value_t
opfunc_is_true_jmp (OPCODE opdata, /**< operation data */
                    struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX cond_var_idx = opdata.data.is_true_jmp.value;
  const T_IDX dst_opcode_idx = opdata.data.is_true_jmp.opcode;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (cond_value, get_variable_value (int_data, cond_var_idx, false), ret_value);

  ecma_completion_value_t to_bool_completion = ecma_op_to_boolean (cond_value.value);
  JERRY_ASSERT (ecma_is_completion_value_normal (to_bool_completion));

  if (ecma_is_value_true (to_bool_completion.value))
  {
    int_data->pos = dst_opcode_idx;
  }
  else
  {
    int_data->pos++;
  }

  ret_value = ecma_make_empty_completion_value ();

  ECMA_FINALIZE (cond_value);

  return ret_value;
} /* opfunc_is_true_jmp */

/**
 * 'Jump if false' opcode handler.
 *
 * Note:
 *      the opcode changes current opcode position to specified opcode index
 *      if argument evaluates to false.
 */
ecma_completion_value_t
opfunc_is_false_jmp (OPCODE opdata, /**< operation data */
                     struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX cond_var_idx = opdata.data.is_false_jmp.value;
  const T_IDX dst_opcode_idx = opdata.data.is_false_jmp.opcode;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (cond_value, get_variable_value (int_data, cond_var_idx, false), ret_value);

  ecma_completion_value_t to_bool_completion = ecma_op_to_boolean (cond_value.value);
  JERRY_ASSERT (ecma_is_completion_value_normal (to_bool_completion));

  if (!ecma_is_value_true (to_bool_completion.value))
  {
    int_data->pos = dst_opcode_idx;
  }
  else
  {
    int_data->pos++;
  }

  ret_value = ecma_make_empty_completion_value ();

  ECMA_FINALIZE (cond_value);

  return ret_value;
} /* opfunc_is_false_jmp */

/**
 * 'Jump' opcode handler.
 *
 * Note:
 *      the opcode changes current opcode position to specified opcode index
 */
ecma_completion_value_t
opfunc_jmp (OPCODE opdata, /**< operation data */
            struct __int_data *int_data) /**< interpreter context */
{
  int_data->pos = opdata.data.jmp.opcode_idx;

  return ecma_make_empty_completion_value ();
} /* opfunc_jmp */

/**
 * 'Jump down' opcode handler.
 *
 * Note:
 *      the opcode changes adds specified value to current opcode position
 */
ecma_completion_value_t
opfunc_jmp_down (OPCODE opdata, /**< operation data */
                 struct __int_data *int_data) /**< interpreter context */
{
  JERRY_ASSERT (int_data->pos <= int_data->pos + opdata.data.jmp_up.opcode_count);

  int_data->pos = (opcode_counter_t) (int_data->pos + opdata.data.jmp_down.opcode_count);

  return ecma_make_empty_completion_value ();
} /* opfunc_jmp_down */

/**
 * 'Jump up' opcode handler.
 *
 * Note:
 *      the opcode changes substracts specified value from current opcode position
 */
ecma_completion_value_t
opfunc_jmp_up (OPCODE opdata, /**< operation data */
               struct __int_data *int_data) /**< interpreter context */
{
  JERRY_ASSERT (int_data->pos >= opdata.data.jmp_up.opcode_count);

  int_data->pos = (opcode_counter_t) (int_data->pos - opdata.data.jmp_down.opcode_count);

  return ecma_make_empty_completion_value ();
} /* opfunc_jmp_up */

/**
 * 'Assignment' opcode handler.
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
ecma_completion_value_t
opfunc_assignment (OPCODE opdata, /**< operation data */
                   struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.assignment.var_left;
  const opcode_arg_type_operand type_value_right = opdata.data.assignment.type_value_right;
  const T_IDX src_val_descr = opdata.data.assignment.value_right;

  int_data->pos++;

  ecma_completion_value_t get_value_completion;

  switch (type_value_right)
  {
    case OPCODE_ARG_TYPE_SIMPLE:
    {
      get_value_completion = ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
                                                         ecma_make_simple_value (src_val_descr),
                                                         ECMA_TARGET_ID_RESERVED);
      break;
    }
    case OPCODE_ARG_TYPE_STRING:
    {
      ecma_string_t *ecma_string_p = ecma_new_ecma_string_from_lit_index (src_val_descr);

      get_value_completion = ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
                                                         ecma_make_string_value (ecma_string_p),
                                                         ECMA_TARGET_ID_RESERVED);
      break;
    }
    case OPCODE_ARG_TYPE_VARIABLE:
    {
      get_value_completion = get_variable_value (int_data,
                                                 src_val_descr,
                                                 false);

      break;
    }
    case OPCODE_ARG_TYPE_NUMBER:
    {
      ecma_number_t *num_p = ecma_alloc_number ();
      *num_p = get_number_by_idx (src_val_descr);

      get_value_completion = ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
                                                         ecma_make_number_value (num_p),
                                                         ECMA_TARGET_ID_RESERVED);
      break;
    }
    case OPCODE_ARG_TYPE_SMALLINT:
    {
      ecma_number_t *num_p = ecma_alloc_number ();
      *num_p = src_val_descr;

      get_value_completion = ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
                                                         ecma_make_number_value (num_p),
                                                         ECMA_TARGET_ID_RESERVED);
      break;
    }
  }

  if (unlikely (ecma_is_completion_value_throw (get_value_completion)))
  {
    return get_value_completion;
  }
  else
  {
    JERRY_ASSERT (ecma_is_completion_value_normal (get_value_completion));

    ecma_completion_value_t assignment_completion_value = set_variable_value (int_data,
                                                                              dst_var_idx,
                                                                              get_value_completion.value);

    ecma_free_completion_value (get_value_completion);

    return assignment_completion_value;
  }
} /* opfunc_assignment */

/**
 * 'Addition' opcode handler.
 *
 * See also: ECMA-262 v5, 11.6.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_addition (OPCODE opdata, /**< operation data */
                 struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.addition.dst;
  const T_IDX left_var_idx = opdata.data.addition.var_left;
  const T_IDX right_var_idx = opdata.data.addition.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);
  ECMA_TRY_CATCH (prim_left_value, ecma_op_to_primitive (left_value.value, ECMA_PREFERRED_TYPE_NO), ret_value);
  ECMA_TRY_CATCH (prim_right_value, ecma_op_to_primitive (right_value.value, ECMA_PREFERRED_TYPE_NO), ret_value);

  if (prim_left_value.value.value_type == ECMA_TYPE_STRING
      || prim_right_value.value.value_type == ECMA_TYPE_STRING)
  {
    JERRY_UNIMPLEMENTED ();
  }
  else
  {
    ret_value = do_number_arithmetic (int_data,
                                      dst_var_idx,
                                      number_arithmetic_addition,
                                      prim_left_value.value,
                                      prim_right_value.value);
  }

  ECMA_FINALIZE (prim_right_value);
  ECMA_FINALIZE (prim_left_value);
  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_addition */

/**
 * 'Substraction' opcode handler.
 *
 * See also: ECMA-262 v5, 11.6.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_substraction (OPCODE opdata, /**< operation data */
                     struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.substraction.dst;
  const T_IDX left_var_idx = opdata.data.substraction.var_left;
  const T_IDX right_var_idx = opdata.data.substraction.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  ret_value = do_number_arithmetic (int_data,
                                    dst_var_idx,
                                    number_arithmetic_substraction,
                                    left_value.value,
                                    right_value.value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_substraction */

/**
 * 'Multiplication' opcode handler.
 *
 * See also: ECMA-262 v5, 11.5, 11.5.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_multiplication (OPCODE opdata, /**< operation data */
                       struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.multiplication.dst;
  const T_IDX left_var_idx = opdata.data.multiplication.var_left;
  const T_IDX right_var_idx = opdata.data.multiplication.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  ret_value = do_number_arithmetic (int_data,
                                    dst_var_idx,
                                    number_arithmetic_multiplication,
                                    left_value.value,
                                    right_value.value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_multiplication */

/**
 * 'Division' opcode handler.
 *
 * See also: ECMA-262 v5, 11.5, 11.5.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_division (OPCODE opdata, /**< operation data */
                 struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.division.dst;
  const T_IDX left_var_idx = opdata.data.division.var_left;
  const T_IDX right_var_idx = opdata.data.division.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  ret_value = do_number_arithmetic (int_data,
                                    dst_var_idx,
                                    number_arithmetic_division,
                                    left_value.value,
                                    right_value.value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_division */

/**
 * 'Remainder calculation' opcode handler.
 *
 * See also: ECMA-262 v5, 11.5, 11.5.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_remainder (OPCODE opdata, /**< operation data */
                  struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.remainder.dst;
  const T_IDX left_var_idx = opdata.data.remainder.var_left;
  const T_IDX right_var_idx = opdata.data.remainder.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  ret_value = do_number_arithmetic (int_data,
                                    dst_var_idx,
                                    number_arithmetic_remainder,
                                    left_value.value,
                                    right_value.value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_remainder */

/**
 * 'Bitwise AND' opcode handler.
 *
 * See also: ECMA-262 v5, 11.10
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_b_and (OPCODE opdata, /**< operation data */
              struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.b_and.dst;
  const T_IDX left_var_idx = opdata.data.b_and.var_left;
  const T_IDX right_var_idx = opdata.data.b_and.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  ret_value = do_number_bitwise_logic (int_data,
                                       dst_var_idx,
                                       number_bitwise_logic_and,
                                       left_value.value,
                                       right_value.value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_b_and */

/**
 * 'Bitwise OR' opcode handler.
 *
 * See also: ECMA-262 v5, 11.10
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_b_or (OPCODE opdata, /**< operation data */
             struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.b_or.dst;
  const T_IDX left_var_idx = opdata.data.b_or.var_left;
  const T_IDX right_var_idx = opdata.data.b_or.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  ret_value = do_number_bitwise_logic (int_data,
                                       dst_var_idx,
                                       number_bitwise_logic_or,
                                       left_value.value,
                                       right_value.value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_b_or */

/**
 * 'Bitwise XOR' opcode handler.
 *
 * See also: ECMA-262 v5, 11.10
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_b_xor (OPCODE opdata, /**< operation data */
              struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.b_xor.dst;
  const T_IDX left_var_idx = opdata.data.b_xor.var_left;
  const T_IDX right_var_idx = opdata.data.b_xor.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  ret_value = do_number_bitwise_logic (int_data,
                                       dst_var_idx,
                                       number_bitwise_logic_xor,
                                       left_value.value,
                                       right_value.value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_b_xor */

/**
 * 'Pre increment' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_pre_incr (OPCODE opdata, /**< operation data */
                 struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.pre_incr.dst;
  const T_IDX incr_var_idx = opdata.data.pre_incr.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  // 1., 2., 3.
  ECMA_TRY_CATCH (old_value, get_variable_value (int_data, incr_var_idx, true), ret_value);
  ECMA_TRY_CATCH (old_num_value, ecma_op_to_number (old_value.value), ret_value);

  // 4.
  ecma_number_t* new_num_p = ecma_alloc_number ();

  ecma_number_t* old_num_p = (ecma_number_t*) ECMA_GET_POINTER (old_num_value.value.value);
  *new_num_p = ecma_op_number_add (*old_num_p, ECMA_NUMBER_ONE);

  ecma_value_t new_num_value = ecma_make_number_value (new_num_p);

  // 5.
  ret_value = set_variable_value (int_data,
                                  incr_var_idx,
                                  new_num_value);

  // assignment of operator result to register variable
  ecma_completion_value_t reg_assignment_res = set_variable_value (int_data,
                                                                   dst_var_idx,
                                                                   new_num_value);
  JERRY_ASSERT (ecma_is_empty_completion_value (reg_assignment_res));

  ecma_dealloc_number (new_num_p);

  ECMA_FINALIZE (old_num_value);
  ECMA_FINALIZE (old_value);

  return ret_value;
} /* opfunc_pre_incr */

/**
 * 'Pre decrement' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_pre_decr (OPCODE opdata, /**< operation data */
                 struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.pre_decr.dst;
  const T_IDX decr_var_idx = opdata.data.pre_decr.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  // 1., 2., 3.
  ECMA_TRY_CATCH (old_value, get_variable_value (int_data, decr_var_idx, true), ret_value);
  ECMA_TRY_CATCH (old_num_value, ecma_op_to_number (old_value.value), ret_value);

  // 4.
  ecma_number_t* new_num_p = ecma_alloc_number ();

  ecma_number_t* old_num_p = (ecma_number_t*) ECMA_GET_POINTER (old_num_value.value.value);
  *new_num_p = ecma_op_number_substract (*old_num_p, ECMA_NUMBER_ONE);

  ecma_value_t new_num_value = ecma_make_number_value (new_num_p);

  // 5.
  ret_value = set_variable_value (int_data,
                                  decr_var_idx,
                                  new_num_value);

  // assignment of operator result to register variable
  ecma_completion_value_t reg_assignment_res = set_variable_value (int_data,
                                                                   dst_var_idx,
                                                                   new_num_value);
  JERRY_ASSERT (ecma_is_empty_completion_value (reg_assignment_res));

  ecma_dealloc_number (new_num_p);

  ECMA_FINALIZE (old_num_value);
  ECMA_FINALIZE (old_value);

  return ret_value;
} /* opfunc_pre_decr */

/**
 * 'Post increment' opcode handler.
 *
 * See also: ECMA-262 v5, 11.3.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_post_incr (OPCODE opdata, /**< operation data */
                  struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.post_incr.dst;
  const T_IDX incr_var_idx = opdata.data.post_incr.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  // 1., 2., 3.
  ECMA_TRY_CATCH (old_value, get_variable_value (int_data, incr_var_idx, true), ret_value);
  ECMA_TRY_CATCH (old_num_value, ecma_op_to_number (old_value.value), ret_value);

  // 4.
  ecma_number_t* new_num_p = ecma_alloc_number ();

  ecma_number_t* old_num_p = (ecma_number_t*) ECMA_GET_POINTER (old_num_value.value.value);
  *new_num_p = ecma_op_number_add (*old_num_p, ECMA_NUMBER_ONE);

  // 5.
  ret_value = set_variable_value (int_data,
                                  incr_var_idx,
                                  ecma_make_number_value (new_num_p));

  ecma_dealloc_number (new_num_p);

  // assignment of operator result to register variable
  ecma_completion_value_t reg_assignment_res = set_variable_value (int_data,
                                                                   dst_var_idx,
                                                                   old_value.value);
  JERRY_ASSERT (ecma_is_empty_completion_value (reg_assignment_res));

  ECMA_FINALIZE (old_num_value);
  ECMA_FINALIZE (old_value);

  return ret_value;
} /* opfunc_post_incr */

/**
 * 'Post decrement' opcode handler.
 *
 * See also: ECMA-262 v5, 11.3.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_post_decr (OPCODE opdata, /**< operation data */
                  struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.post_decr.dst;
  const T_IDX decr_var_idx = opdata.data.post_decr.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  // 1., 2., 3.
  ECMA_TRY_CATCH (old_value, get_variable_value (int_data, decr_var_idx, true), ret_value);
  ECMA_TRY_CATCH (old_num_value, ecma_op_to_number (old_value.value), ret_value);

  // 4.
  ecma_number_t* new_num_p = ecma_alloc_number ();

  ecma_number_t* old_num_p = (ecma_number_t*) ECMA_GET_POINTER (old_num_value.value.value);
  *new_num_p = ecma_op_number_substract (*old_num_p, ECMA_NUMBER_ONE);

  // 5.
  ret_value = set_variable_value (int_data,
                                  decr_var_idx,
                                  ecma_make_number_value (new_num_p));

  ecma_dealloc_number (new_num_p);

  // assignment of operator result to register variable
  ecma_completion_value_t reg_assignment_res = set_variable_value (int_data,
                                                                   dst_var_idx,
                                                                   old_value.value);
  JERRY_ASSERT (ecma_is_empty_completion_value (reg_assignment_res));

  ECMA_FINALIZE (old_num_value);
  ECMA_FINALIZE (old_value);

  return ret_value;
} /* opfunc_post_decr */

/**
 * 'Equals' opcode handler.
 *
 * See also: ECMA-262 v5, 11.9.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_equal_value (OPCODE opdata, /**< operation data */
                    struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.equal_value.dst;
  const T_IDX left_var_idx = opdata.data.equal_value.var_left;
  const T_IDX right_var_idx = opdata.data.equal_value.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  bool is_equal = ecma_op_abstract_equality_compare (left_value.value, right_value.value);

  ret_value = set_variable_value (int_data, dst_var_idx, ecma_make_simple_value (is_equal ? ECMA_SIMPLE_VALUE_TRUE
                                                                                 : ECMA_SIMPLE_VALUE_FALSE));

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_equal_value */

/**
 * 'Does-not-equals' opcode handler.
 *
 * See also: ECMA-262 v5, 11.9.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_not_equal_value (OPCODE opdata, /**< operation data */
                        struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.not_equal_value.dst;
  const T_IDX left_var_idx = opdata.data.not_equal_value.var_left;
  const T_IDX right_var_idx = opdata.data.not_equal_value.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  bool is_equal = ecma_op_abstract_equality_compare (left_value.value, right_value.value);

  ret_value = set_variable_value (int_data, dst_var_idx, ecma_make_simple_value (is_equal ? ECMA_SIMPLE_VALUE_FALSE
                                                                                 : ECMA_SIMPLE_VALUE_TRUE));


  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_not_equal_value */

/**
 * 'Strict Equals' opcode handler.
 *
 * See also: ECMA-262 v5, 11.9.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_equal_value_type (OPCODE opdata, /**< operation data */
                         struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.equal_value_type.dst;
  const T_IDX left_var_idx = opdata.data.equal_value_type.var_left;
  const T_IDX right_var_idx = opdata.data.equal_value_type.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  bool is_equal = ecma_op_strict_equality_compare (left_value.value, right_value.value);

  ret_value = set_variable_value (int_data, dst_var_idx, ecma_make_simple_value (is_equal ? ECMA_SIMPLE_VALUE_TRUE
                                                                                 : ECMA_SIMPLE_VALUE_FALSE));

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_equal_value_type */

/**
 * 'Strict Does-not-equals' opcode handler.
 *
 * See also: ECMA-262 v5, 11.9.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_not_equal_value_type (OPCODE opdata, /**< operation data */
                             struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.not_equal_value_type.dst;
  const T_IDX left_var_idx = opdata.data.not_equal_value_type.var_left;
  const T_IDX right_var_idx = opdata.data.not_equal_value_type.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  bool is_equal = ecma_op_strict_equality_compare (left_value.value, right_value.value);

  ret_value = set_variable_value (int_data, dst_var_idx, ecma_make_simple_value (is_equal ? ECMA_SIMPLE_VALUE_FALSE
                                                                                 : ECMA_SIMPLE_VALUE_TRUE));


  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_not_equal_value_type */

/**
 * 'Less-than' opcode handler.
 *
 * See also: ECMA-262 v5, 11.8.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_less_than (OPCODE opdata, /**< operation data */
                  struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.less_than.dst;
  const T_IDX left_var_idx = opdata.data.less_than.var_left;
  const T_IDX right_var_idx = opdata.data.less_than.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);
  ECMA_TRY_CATCH (compare_result,
                  ecma_op_abstract_relational_compare (left_value.value,
                                                       right_value.value,
                                                       true),
                  ret_value);

  ecma_simple_value_t res;

  if (ecma_is_value_undefined (compare_result.value))
  {
    res = ECMA_SIMPLE_VALUE_FALSE;
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (compare_result.value));

    res = compare_result.value.value;
  }

  ret_value = set_variable_value (int_data, dst_var_idx, ecma_make_simple_value (res));

  ECMA_FINALIZE (compare_result);
  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_less_than */

/**
 * 'Greater-than' opcode handler.
 *
 * See also: ECMA-262 v5, 11.8.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_greater_than (OPCODE opdata, /**< operation data */
                     struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.greater_than.dst;
  const T_IDX left_var_idx = opdata.data.greater_than.var_left;
  const T_IDX right_var_idx = opdata.data.greater_than.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);
  ECMA_TRY_CATCH (compare_result,
                  ecma_op_abstract_relational_compare (right_value.value,
                                                       left_value.value,
                                                       false),
                  ret_value);

  ecma_simple_value_t res;

  if (ecma_is_value_undefined (compare_result.value))
  {
    res = ECMA_SIMPLE_VALUE_FALSE;
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (compare_result.value));

    res = compare_result.value.value;
  }

  ret_value = set_variable_value (int_data, dst_var_idx, ecma_make_simple_value (res));

  ECMA_FINALIZE (compare_result);
  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_greater_than */

/**
 * 'Less-than-or-equal' opcode handler.
 *
 * See also: ECMA-262 v5, 11.8.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_less_or_equal_than (OPCODE opdata, /**< operation data */
                           struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.less_or_equal_than.dst;
  const T_IDX left_var_idx = opdata.data.less_or_equal_than.var_left;
  const T_IDX right_var_idx = opdata.data.less_or_equal_than.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);
  ECMA_TRY_CATCH (compare_result,
                  ecma_op_abstract_relational_compare (right_value.value,
                                                       left_value.value,
                                                       false),
                  ret_value);

  ecma_simple_value_t res;

  if (ecma_is_value_undefined (compare_result.value))
  {
    res = ECMA_SIMPLE_VALUE_FALSE;
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (compare_result.value));

    if (compare_result.value.value == ECMA_SIMPLE_VALUE_TRUE)
    {
      res = ECMA_SIMPLE_VALUE_FALSE;
    }
    else
    {
      res = ECMA_SIMPLE_VALUE_TRUE;
    }
  }

  ret_value = set_variable_value (int_data, dst_var_idx, ecma_make_simple_value (res));

  ECMA_FINALIZE (compare_result);
  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_less_or_equal_than */

/**
 * 'Greater-than-or-equal' opcode handler.
 *
 * See also: ECMA-262 v5, 11.8.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_greater_or_equal_than (OPCODE opdata, /**< operation data */
                              struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX dst_var_idx = opdata.data.greater_or_equal_than.dst;
  const T_IDX left_var_idx = opdata.data.greater_or_equal_than.var_left;
  const T_IDX right_var_idx = opdata.data.greater_or_equal_than.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);
  ECMA_TRY_CATCH (compare_result,
                  ecma_op_abstract_relational_compare (left_value.value,
                                                       right_value.value,
                                                       true),
                  ret_value);

  ecma_simple_value_t res;

  if (ecma_is_value_undefined (compare_result.value))
  {
    res = ECMA_SIMPLE_VALUE_FALSE;
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (compare_result.value));

    if (compare_result.value.value == ECMA_SIMPLE_VALUE_TRUE)
    {
      res = ECMA_SIMPLE_VALUE_FALSE;
    }
    else
    {
      res = ECMA_SIMPLE_VALUE_TRUE;
    }
  }

  ret_value = set_variable_value (int_data, dst_var_idx, ecma_make_simple_value (res));

  ECMA_FINALIZE (compare_result);
  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_greater_or_equal_than */

/**
 * 'Register variable declaration' opcode handler.
 *
 * The opcode is meta-opcode that is not supposed to be executed.
 */
ecma_completion_value_t
opfunc_reg_var_decl (OPCODE opdata __unused, /**< operation data */
                     struct __int_data *int_data __unused) /**< interpreter context */
{
  JERRY_UNREACHABLE ();
} /* opfunc_reg_var_decl */

/**
 * 'Variable declaration' opcode handler.
 *
 * See also: ECMA-262 v5, 10.5 - Declaration binding instantiation (block 8).
 *
 * @return completion value
 *         Returned value is simple and so need not be freed.
 *         However, ecma_free_completion_value may be called for it, but it is a no-op.
 */
ecma_completion_value_t
opfunc_var_decl (OPCODE opdata, /**< operation data */
                 struct __int_data *int_data) /**< interpreter context */
{
  ecma_string_t *var_name_string_p = ecma_new_ecma_string_from_lit_index (opdata.data.var_decl.variable_name);

  if (ecma_is_completion_value_normal_false (ecma_op_has_binding (int_data->lex_env_p,
                                                                  var_name_string_p)))
  {
    FIXME ("Pass configurableBindings that is true if and only if current code is eval code");

    ecma_completion_value_t completion = ecma_op_create_mutable_binding (int_data->lex_env_p,
                                                                         var_name_string_p,
                                                                         false);

    JERRY_ASSERT (ecma_is_empty_completion_value (completion));

    /* Skipping SetMutableBinding as we have already checked that there were not
     * any binding with specified name in current lexical environment
     * and CreateMutableBinding sets the created binding's value to undefined */
    JERRY_ASSERT (ecma_is_completion_value_normal_simple_value (ecma_op_get_binding_value (int_data->lex_env_p,
                                                                                           var_name_string_p,
                                                                                           true),
                                                                ECMA_SIMPLE_VALUE_UNDEFINED));
  }

  ecma_deref_ecma_string (var_name_string_p);

  int_data->pos++;

  return ecma_make_empty_completion_value ();
} /* opfunc_var_decl */

/**
 * Function declaration helper
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
function_declaration (struct __int_data *int_data, /**< interpreter context */
                      T_IDX function_name_lit_idx, /**< identifier of literal with function name */
                      ecma_string_t* args_names[], /**< names of arguments */
                      ecma_length_t args_number) /**< number of arguments */
{
  TODO ("Check if code of function itself is strict");

  const bool is_strict = int_data->is_strict;
  const bool is_configurable_bindings = int_data->is_eval_code;

  const opcode_counter_t jmp_down_opcode_idx = (opcode_counter_t) (int_data->pos);
  OPCODE jmp_down_opcode = read_opcode (jmp_down_opcode_idx);
  JERRY_ASSERT (jmp_down_opcode.op_idx == __op__idx_jmp_down);
  int_data->pos = (opcode_counter_t) (jmp_down_opcode_idx + jmp_down_opcode.data.jmp_down.opcode_count);

  const opcode_counter_t function_code_opcode_idx = (opcode_counter_t) (jmp_down_opcode_idx + 1);

  ecma_string_t *function_name_string_p = ecma_new_ecma_string_from_lit_index (function_name_lit_idx);

  ecma_completion_value_t ret_value = ecma_op_function_declaration (int_data->lex_env_p,
                                                                    function_name_string_p,
                                                                    function_code_opcode_idx,
                                                                    args_names,
                                                                    args_number,
                                                                    is_strict,
                                                                    is_configurable_bindings);
  ecma_deref_ecma_string (function_name_string_p);

  return ret_value;
} /* function_declaration */

/**
 * 'Function declaration with no parameters' opcode handler.
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_func_decl_0 (OPCODE opdata, /**< operation data */
                    struct __int_data *int_data) /**< interpreter context */
{
  int_data->pos++;

  return function_declaration (int_data,
                               opdata.data.func_decl_0.name_lit_idx,
                               NULL,
                               0);
} /* opfunc_func_decl_0 */

/**
 * 'Function declaration with one parameter' opcode handler.
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_func_decl_1 (OPCODE opdata, /**< operation data */
                    struct __int_data *int_data) /**< interpreter context */
{
  int_data->pos++;

  ecma_string_t *arg_name_string_p = ecma_new_ecma_string_from_lit_index (opdata.data.func_decl_1.arg1_lit_idx);

  ecma_completion_value_t ret_value = function_declaration (int_data,
                                                            opdata.data.func_decl_1.name_lit_idx,
                                                            &arg_name_string_p,
                                                            1);

  ecma_deref_ecma_string (arg_name_string_p);

  return ret_value;
} /* opfunc_func_decl_1 */

/**
 * 'Function declaration with two parameters' opcode handler.
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_func_decl_2 (OPCODE opdata, /**< operation data */
                    struct __int_data *int_data) /**< interpreter context */
{
  int_data->pos++;

  ecma_string_t* arg_names_strings[2] =
  {
    ecma_new_ecma_string_from_lit_index (opdata.data.func_decl_2.arg1_lit_idx),
    ecma_new_ecma_string_from_lit_index (opdata.data.func_decl_2.arg2_lit_idx)
  };

  ecma_completion_value_t ret_value = function_declaration (int_data,
                                                            opdata.data.func_decl_1.name_lit_idx,
                                                            arg_names_strings,
                                                            2);

  ecma_deref_ecma_string (arg_names_strings[0]);
  ecma_deref_ecma_string (arg_names_strings[1]);

  return ret_value;
} /* opfunc_func_decl_2 */

/**
 * Function call with no arguments' opcode handler.
 *
 * See also: ECMA-262 v5, 11.2.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_call_0 (OPCODE opdata, /**< operation data */
               struct __int_data *int_data) /**< interpreter context */
{
  const T_IDX func_name_lit_idx = opdata.data.call_0.name_lit_idx;
  const T_IDX lhs_var_idx = opdata.data.call_0.lhs;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (func_value, get_variable_value (int_data, func_name_lit_idx, false), ret_value);

  if (!ecma_op_is_callable (func_value.value))
  {
    ret_value = ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    ecma_object_t *func_obj_p = ECMA_GET_POINTER (func_value.value.value);

    ECMA_TRY_CATCH (this_value, ecma_op_implicit_this_value (int_data->lex_env_p), ret_value);

    ret_value = ecma_op_function_call (func_obj_p, this_value.value, NULL, 0);

    if (ret_value.type == ECMA_COMPLETION_TYPE_RETURN)
    {
      ecma_value_t returned_value = ret_value.value;

      ret_value = set_variable_value (int_data, lhs_var_idx, returned_value);

      ecma_free_value (returned_value, true);
    }

    ECMA_FINALIZE (this_value);
  }

  ECMA_FINALIZE (func_value);

  return ret_value;
} /* opfunc_call_0 */

/**
 * 'Return with no expression' opcode handler.
 *
 * See also: ECMA-262 v5, 12.9
 *
 * @return completion value
 *         Returned value is simple and so need not be freed.
 *         However, ecma_free_completion_value may be called for it, but it is a no-op.
 */
ecma_completion_value_t
opfunc_ret (OPCODE opdata __unused, /**< operation data */
            struct __int_data *int_data __unused) /**< interpreter context */
{
  return ecma_make_completion_value (ECMA_COMPLETION_TYPE_RETURN,
                                     ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED),
                                     ECMA_TARGET_ID_RESERVED);
} /* opfunc_ret */

/**
 * 'Property getter' opcode handler.
 *
 * See also: ECMA-262 v5, 11.2.1
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_prop_getter (OPCODE opdata __unused, /**< operation data */
                    struct __int_data *int_data __unused) /**< interpreter context */
{
  const T_IDX lhs_var_idx = opdata.data.prop_getter.lhs;
  const T_IDX base_var_idx = opdata.data.prop_getter.obj;
  const T_IDX prop_lit_idx = opdata.data.prop_getter.prop;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (base_value, get_variable_value (int_data, base_var_idx, false), ret_value);

  ecma_string_t *prop_name_string_p = ecma_new_ecma_string_from_lit_index (prop_lit_idx);
  ecma_reference_t ref = ecma_make_reference (base_value.value,
                                              prop_name_string_p,
                                              int_data->is_strict);
  ecma_deref_ecma_string (prop_name_string_p);

  ECMA_TRY_CATCH (prop_value, ecma_op_get_value (ref), ret_value);

  ret_value = set_variable_value (int_data, lhs_var_idx, prop_value.value);

  ECMA_FINALIZE (prop_value);

  ecma_free_reference (ref);

  ECMA_FINALIZE (base_value);

  return ret_value;
} /* opfunc_prop_getter */

/**
 * 'Property setter' opcode handler.
 *
 * See also: ECMA-262 v5, 11.2.1
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_prop_setter (OPCODE opdata __unused, /**< operation data */
                    struct __int_data *int_data __unused) /**< interpreter context */
{
  const T_IDX base_var_idx = opdata.data.prop_setter.obj;
  const T_IDX prop_lit_idx = opdata.data.prop_setter.prop;
  const T_IDX rhs_var_idx = opdata.data.prop_setter.rhs;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (rhs_value, get_variable_value (int_data, rhs_var_idx, false), ret_value);
  ECMA_TRY_CATCH (base_value, get_variable_value (int_data, base_var_idx, false), ret_value);

  ecma_string_t *prop_name_string_p = ecma_new_ecma_string_from_lit_index (prop_lit_idx);
  ecma_reference_t ref = ecma_make_reference (base_value.value,
                                              prop_name_string_p,
                                              int_data->is_strict);
  ecma_deref_ecma_string (prop_name_string_p);

  ret_value = ecma_op_put_value (ref, rhs_value.value);

  ecma_free_reference (ref);

  ECMA_FINALIZE (base_value);
  ECMA_FINALIZE (rhs_value);

  return ret_value;
} /* opfunc_prop_setter */

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
ecma_completion_value_t
opfunc_exitval (OPCODE opdata, /**< operation data */
                struct __int_data *int_data __unused) /**< interpreter context */
{
  JERRY_ASSERT (opdata.data.exitval.status_code == 0
                || opdata.data.exitval.status_code == 1);

  ecma_value_t exit_status = ecma_make_simple_value (opdata.data.exitval.status_code == 0 ? ECMA_SIMPLE_VALUE_TRUE
                                                     : ECMA_SIMPLE_VALUE_FALSE);
  return ecma_make_completion_value (ECMA_COMPLETION_TYPE_EXIT,
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
GETOP_IMPL_2 (post_incr, dst, var_right)
GETOP_IMPL_2 (post_decr, dst, var_right)
GETOP_IMPL_2 (pre_incr, dst, var_right)
GETOP_IMPL_2 (pre_decr, dst, var_right)
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
GETOP_IMPL_3 (varg_list, arg1_lit_idx, arg2_lit_idx, arg3_lit_idx)
GETOP_IMPL_1 (exitval, status_code)
GETOP_IMPL_1 (retval, ret_value)
GETOP_IMPL_0 (ret)
GETOP_IMPL_0 (nop)
GETOP_IMPL_1 (var_decl, variable_name)
GETOP_IMPL_2 (b_not, dst, var_right)
GETOP_IMPL_2 (logical_not, dst, var_right)
GETOP_IMPL_3 (instanceof, dst, var_left, var_right)
GETOP_IMPL_3 (in, dst, var_left, var_right)
GETOP_IMPL_3 (construct_decl, lhs, name_lit_idx, arg_list)
GETOP_IMPL_1 (func_decl_0, name_lit_idx)
GETOP_IMPL_2 (array_decl, lhs, list)
GETOP_IMPL_3 (prop, lhs, name, value)
GETOP_IMPL_3 (prop_getter, lhs, obj, prop)
GETOP_IMPL_3 (prop_setter, obj, prop, rhs)
GETOP_IMPL_2 (prop_get_decl, lhs, prop)
GETOP_IMPL_3 (prop_set_decl, lhs, prop, arg)
GETOP_IMPL_2 (obj_decl, lhs, list)
GETOP_IMPL_1 (this, lhs)
GETOP_IMPL_2 (delete, lhs, obj)
GETOP_IMPL_2 (typeof, lhs, obj)
GETOP_IMPL_1 (with, expr)
GETOP_IMPL_0 (end_with)
GETOP_IMPL_2 (reg_var_decl, min, max)
