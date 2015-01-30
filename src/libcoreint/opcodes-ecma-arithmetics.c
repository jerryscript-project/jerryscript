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
#include "jerry-libc.h"

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
static void
do_number_arithmetic (ecma_completion_value_t &ret_value, /**< out: completion value */
                      int_data_t *int_data, /**< interpreter context */
                      idx_t dst_var_idx, /**< destination variable identifier */
                      number_arithmetic_op op, /**< number arithmetic operation */
                      const ecma_value_t& left_value, /**< left value */
                      const ecma_value_t& right_value) /** right value */
{
  ECMA_OP_TO_NUMBER_TRY_CATCH (num_left, left_value, ret_value);
  ECMA_OP_TO_NUMBER_TRY_CATCH (num_right, right_value, ret_value);

  ecma_number_t *res_p = int_data->tmp_num_p;

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

  set_variable_value (ret_value,
                      int_data, int_data->pos,
                      dst_var_idx,
                      ecma_value_t (res_p));

  ECMA_OP_TO_NUMBER_FINALIZE (num_right);
  ECMA_OP_TO_NUMBER_FINALIZE (num_left);
} /* do_number_arithmetic */

/**
 * 'Addition' opcode handler.
 *
 * See also: ECMA-262 v5, 11.6.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
void
opfunc_addition (ecma_completion_value_t &ret_value, /**< out: completion value */
                 opcode_t opdata, /**< operation data */
                 int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.addition.dst;
  const idx_t left_var_idx = opdata.data.addition.var_left;
  const idx_t right_var_idx = opdata.data.addition.var_right;

  ECMA_TRY_CATCH (ret_value, get_variable_value, left_value, int_data, left_var_idx, false);
  ECMA_TRY_CATCH (ret_value, get_variable_value, right_value, int_data, right_var_idx, false);
  ECMA_TRY_CATCH (ret_value, ecma_op_to_primitive, prim_left_value, left_value, ECMA_PREFERRED_TYPE_NO);
  ECMA_TRY_CATCH (ret_value, ecma_op_to_primitive, prim_right_value, right_value, ECMA_PREFERRED_TYPE_NO);

  if (ecma_is_value_string (prim_left_value)
      || ecma_is_value_string (prim_right_value))
  {
    ECMA_TRY_CATCH (ret_value, ecma_op_to_string, str_left_value, prim_left_value);
    ECMA_TRY_CATCH (ret_value, ecma_op_to_string, str_right_value, prim_right_value);

    ecma_string_t *string1_p = ecma_get_string_from_value (str_left_value);
    ecma_string_t *string2_p = ecma_get_string_from_value (str_right_value);

    ecma_string_t *concat_str_p = ecma_concat_ecma_strings (string1_p, string2_p);

    set_variable_value (ret_value, int_data, int_data->pos, dst_var_idx, ecma_value_t (concat_str_p));

    ecma_deref_ecma_string (concat_str_p);

    ECMA_FINALIZE (str_right_value);
    ECMA_FINALIZE (str_left_value);
  }
  else
  {
    do_number_arithmetic (ret_value,
                          int_data,
                          dst_var_idx,
                          number_arithmetic_addition,
                          prim_left_value,
                          prim_right_value);
  }

  ECMA_FINALIZE (prim_right_value);
  ECMA_FINALIZE (prim_left_value);
  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  int_data->pos++;
} /* opfunc_addition */

/**
 * 'Substraction' opcode handler.
 *
 * See also: ECMA-262 v5, 11.6.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
void
opfunc_substraction (ecma_completion_value_t &ret_value, /**< out: completion value */
                     opcode_t opdata, /**< operation data */
                     int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.substraction.dst;
  const idx_t left_var_idx = opdata.data.substraction.var_left;
  const idx_t right_var_idx = opdata.data.substraction.var_right;

  ECMA_TRY_CATCH (ret_value, get_variable_value, left_value, int_data, left_var_idx, false);
  ECMA_TRY_CATCH (ret_value, get_variable_value, right_value, int_data, right_var_idx, false);

  do_number_arithmetic (ret_value,
                        int_data,
                        dst_var_idx,
                        number_arithmetic_substraction,
                        left_value,
                        right_value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  int_data->pos++;
} /* opfunc_substraction */

/**
 * 'Multiplication' opcode handler.
 *
 * See also: ECMA-262 v5, 11.5, 11.5.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
void
opfunc_multiplication (ecma_completion_value_t &ret_value, /**< out: completion value */
                       opcode_t opdata, /**< operation data */
                       int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.multiplication.dst;
  const idx_t left_var_idx = opdata.data.multiplication.var_left;
  const idx_t right_var_idx = opdata.data.multiplication.var_right;

  ECMA_TRY_CATCH (ret_value, get_variable_value, left_value, int_data, left_var_idx, false);
  ECMA_TRY_CATCH (ret_value, get_variable_value, right_value, int_data, right_var_idx, false);

  do_number_arithmetic (ret_value,
                        int_data,
                        dst_var_idx,
                        number_arithmetic_multiplication,
                        left_value,
                        right_value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  int_data->pos++;
} /* opfunc_multiplication */

/**
 * 'Division' opcode handler.
 *
 * See also: ECMA-262 v5, 11.5, 11.5.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
void
opfunc_division (ecma_completion_value_t &ret_value, /**< out: completion value */
                 opcode_t opdata, /**< operation data */
                 int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.division.dst;
  const idx_t left_var_idx = opdata.data.division.var_left;
  const idx_t right_var_idx = opdata.data.division.var_right;

  ECMA_TRY_CATCH (ret_value, get_variable_value, left_value, int_data, left_var_idx, false);
  ECMA_TRY_CATCH (ret_value, get_variable_value, right_value, int_data, right_var_idx, false);

  do_number_arithmetic (ret_value,
                        int_data,
                        dst_var_idx,
                        number_arithmetic_division,
                        left_value,
                        right_value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  int_data->pos++;
} /* opfunc_division */

/**
 * 'Remainder calculation' opcode handler.
 *
 * See also: ECMA-262 v5, 11.5, 11.5.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
void
opfunc_remainder (ecma_completion_value_t &ret_value, /**< out: completion value */
                  opcode_t opdata, /**< operation data */
                  int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.remainder.dst;
  const idx_t left_var_idx = opdata.data.remainder.var_left;
  const idx_t right_var_idx = opdata.data.remainder.var_right;

  ECMA_TRY_CATCH (ret_value, get_variable_value, left_value, int_data, left_var_idx, false);
  ECMA_TRY_CATCH (ret_value, get_variable_value, right_value, int_data, right_var_idx, false);

  do_number_arithmetic (ret_value,
                        int_data,
                        dst_var_idx,
                        number_arithmetic_remainder,
                        left_value,
                        right_value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  int_data->pos++;
} /* opfunc_remainder */

/**
 * 'Unary "+"' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4, 11.4.6
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
void
opfunc_unary_plus (ecma_completion_value_t &ret_value, /**< out: completion value */
                   opcode_t opdata, /**< operation data */
                   int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.remainder.dst;
  const idx_t var_idx = opdata.data.remainder.var_left;

  ECMA_TRY_CATCH (ret_value, get_variable_value, var_value, int_data, var_idx, false);
  ECMA_OP_TO_NUMBER_TRY_CATCH (num_var_value,
                               var_value,
                               ret_value);

  ecma_number_t *tmp_p = int_data->tmp_num_p;

  *tmp_p = num_var_value;
  set_variable_value (ret_value, int_data, int_data->pos, dst_var_idx, ecma_value_t (tmp_p));

  ECMA_OP_TO_NUMBER_FINALIZE (num_var_value);
  ECMA_FINALIZE (var_value);

  int_data->pos++;
} /* opfunc_unary_plus */

/**
 * 'Unary "-"' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4, 11.4.7
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
void
opfunc_unary_minus (ecma_completion_value_t &ret_value, /**< out: completion value */
                    opcode_t opdata, /**< operation data */
                    int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.remainder.dst;
  const idx_t var_idx = opdata.data.remainder.var_left;

  ECMA_TRY_CATCH (ret_value, get_variable_value, var_value, int_data, var_idx, false);
  ECMA_OP_TO_NUMBER_TRY_CATCH (num_var_value,
                               var_value,
                               ret_value);

  ecma_number_t *tmp_p = int_data->tmp_num_p;

  *tmp_p = ecma_number_negate (num_var_value);
  set_variable_value (ret_value, int_data, int_data->pos, dst_var_idx, ecma_value_t (tmp_p));

  ECMA_OP_TO_NUMBER_FINALIZE (num_var_value);
  ECMA_FINALIZE (var_value);

  int_data->pos++;
} /* opfunc_unary_minus */
