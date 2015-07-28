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

#include "opcodes.h"
#include "opcodes-ecma-support.h"
#include "ecma-number-arithmetic.h"
#include "jrt-libc-includes.h"

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
 *   leftNum = ToNumber (leftValue);
 *   rightNum = ToNumber (rightValue);
 *   result = leftNum ArithmeticOp rightNum;
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
do_number_arithmetic (vm_frame_ctx_t *frame_ctx_p, /**< interpreter context */
                      vm_idx_t dst_var_idx, /**< destination variable identifier */
                      number_arithmetic_op op, /**< number arithmetic operation */
                      ecma_value_t left_value, /**< left value */
                      ecma_value_t right_value) /** right value */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_OP_TO_NUMBER_TRY_CATCH (num_left, left_value, ret_value);
  ECMA_OP_TO_NUMBER_TRY_CATCH (num_right, right_value, ret_value);

  ecma_number_t *res_p = frame_ctx_p->tmp_num_p;

  switch (op)
  {
    case number_arithmetic_addition:
    {
      *res_p = ecma_number_add (num_left, num_right);
      break;
    }
    case number_arithmetic_substraction:
    {
      *res_p = ecma_number_substract (num_left, num_right);
      break;
    }
    case number_arithmetic_multiplication:
    {
      *res_p = ecma_number_multiply (num_left, num_right);
      break;
    }
    case number_arithmetic_division:
    {
      *res_p = ecma_number_divide (num_left, num_right);
      break;
    }
    case number_arithmetic_remainder:
    {
      *res_p = ecma_op_number_remainder (num_left, num_right);
      break;
    }
  }

  ret_value = set_variable_value (frame_ctx_p, frame_ctx_p->pos,
                                  dst_var_idx,
                                  ecma_make_number_value (res_p));

  ECMA_OP_TO_NUMBER_FINALIZE (num_right);
  ECMA_OP_TO_NUMBER_FINALIZE (num_left);

  return ret_value;
} /* do_number_arithmetic */

/**
 * 'Addition' opcode handler.
 *
 * See also: ECMA-262 v5, 11.6.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_addition (vm_instr_t instr, /**< instruction */
                 vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.addition.dst;
  const vm_idx_t left_var_idx = instr.data.addition.var_left;
  const vm_idx_t right_var_idx = instr.data.addition.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (left_value, get_variable_value (frame_ctx_p, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (frame_ctx_p, right_var_idx, false), ret_value);
  ECMA_TRY_CATCH (prim_left_value,
                  ecma_op_to_primitive (left_value,
                                        ECMA_PREFERRED_TYPE_NO),
                  ret_value);
  ECMA_TRY_CATCH (prim_right_value,
                  ecma_op_to_primitive (right_value,
                                        ECMA_PREFERRED_TYPE_NO),
                  ret_value);

  if (ecma_is_value_string (prim_left_value)
      || ecma_is_value_string (prim_right_value))
  {
    ECMA_TRY_CATCH (str_left_value, ecma_op_to_string (prim_left_value), ret_value);
    ECMA_TRY_CATCH (str_right_value, ecma_op_to_string (prim_right_value), ret_value);

    ecma_string_t *string1_p = ecma_get_string_from_value (str_left_value);
    ecma_string_t *string2_p = ecma_get_string_from_value (str_right_value);

    ecma_string_t *concat_str_p = ecma_concat_ecma_strings (string1_p, string2_p);

    ret_value = set_variable_value (frame_ctx_p, frame_ctx_p->pos, dst_var_idx, ecma_make_string_value (concat_str_p));

    ecma_deref_ecma_string (concat_str_p);

    ECMA_FINALIZE (str_right_value);
    ECMA_FINALIZE (str_left_value);
  }
  else
  {
    ret_value = do_number_arithmetic (frame_ctx_p,
                                      dst_var_idx,
                                      number_arithmetic_addition,
                                      prim_left_value,
                                      prim_right_value);
  }

  ECMA_FINALIZE (prim_right_value);
  ECMA_FINALIZE (prim_left_value);
  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  frame_ctx_p->pos++;

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
opfunc_substraction (vm_instr_t instr, /**< instruction */
                     vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.substraction.dst;
  const vm_idx_t left_var_idx = instr.data.substraction.var_left;
  const vm_idx_t right_var_idx = instr.data.substraction.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (left_value, get_variable_value (frame_ctx_p, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (frame_ctx_p, right_var_idx, false), ret_value);

  ret_value = do_number_arithmetic (frame_ctx_p,
                                    dst_var_idx,
                                    number_arithmetic_substraction,
                                    left_value,
                                    right_value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  frame_ctx_p->pos++;

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
opfunc_multiplication (vm_instr_t instr, /**< instruction */
                       vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.multiplication.dst;
  const vm_idx_t left_var_idx = instr.data.multiplication.var_left;
  const vm_idx_t right_var_idx = instr.data.multiplication.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (left_value, get_variable_value (frame_ctx_p, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (frame_ctx_p, right_var_idx, false), ret_value);

  ret_value = do_number_arithmetic (frame_ctx_p,
                                    dst_var_idx,
                                    number_arithmetic_multiplication,
                                    left_value,
                                    right_value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  frame_ctx_p->pos++;

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
opfunc_division (vm_instr_t instr, /**< instruction */
                 vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.division.dst;
  const vm_idx_t left_var_idx = instr.data.division.var_left;
  const vm_idx_t right_var_idx = instr.data.division.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (left_value, get_variable_value (frame_ctx_p, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (frame_ctx_p, right_var_idx, false), ret_value);

  ret_value = do_number_arithmetic (frame_ctx_p,
                                    dst_var_idx,
                                    number_arithmetic_division,
                                    left_value,
                                    right_value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  frame_ctx_p->pos++;

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
opfunc_remainder (vm_instr_t instr, /**< instruction */
                  vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.remainder.dst;
  const vm_idx_t left_var_idx = instr.data.remainder.var_left;
  const vm_idx_t right_var_idx = instr.data.remainder.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (left_value, get_variable_value (frame_ctx_p, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (frame_ctx_p, right_var_idx, false), ret_value);

  ret_value = do_number_arithmetic (frame_ctx_p,
                                    dst_var_idx,
                                    number_arithmetic_remainder,
                                    left_value,
                                    right_value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  frame_ctx_p->pos++;

  return ret_value;
} /* opfunc_remainder */

/**
 * 'Unary "+"' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4, 11.4.6
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_unary_plus (vm_instr_t instr, /**< instruction */
                   vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.remainder.dst;
  const vm_idx_t var_idx = instr.data.remainder.var_left;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (var_value, get_variable_value (frame_ctx_p, var_idx, false), ret_value);
  ECMA_OP_TO_NUMBER_TRY_CATCH (num_var_value,
                               var_value,
                               ret_value);

  ecma_number_t *tmp_p = frame_ctx_p->tmp_num_p;

  *tmp_p = num_var_value;
  ret_value = set_variable_value (frame_ctx_p, frame_ctx_p->pos,
                                  dst_var_idx,
                                  ecma_make_number_value (tmp_p));

  ECMA_OP_TO_NUMBER_FINALIZE (num_var_value);
  ECMA_FINALIZE (var_value);

  frame_ctx_p->pos++;

  return ret_value;
} /* opfunc_unary_plus */

/**
 * 'Unary "-"' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4, 11.4.7
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_unary_minus (vm_instr_t instr, /**< instruction */
                    vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.remainder.dst;
  const vm_idx_t var_idx = instr.data.remainder.var_left;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (var_value, get_variable_value (frame_ctx_p, var_idx, false), ret_value);
  ECMA_OP_TO_NUMBER_TRY_CATCH (num_var_value,
                               var_value,
                               ret_value);

  ecma_number_t *tmp_p = frame_ctx_p->tmp_num_p;

  *tmp_p = ecma_number_negate (num_var_value);
  ret_value = set_variable_value (frame_ctx_p, frame_ctx_p->pos,
                                  dst_var_idx,
                                  ecma_make_number_value (tmp_p));

  ECMA_OP_TO_NUMBER_FINALIZE (num_var_value);
  ECMA_FINALIZE (var_value);

  frame_ctx_p->pos++;

  return ret_value;
} /* opfunc_unary_minus */
