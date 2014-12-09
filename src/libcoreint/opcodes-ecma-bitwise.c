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

#include "opcodes.h"
#include "opcodes-ecma-support.h"

/**
 * Number bitwise logic operations.
 */
typedef enum
{
  number_bitwise_logic_and, /**< bitwise AND calculation */
  number_bitwise_logic_or, /**< bitwise OR calculation */
  number_bitwise_logic_xor, /**< bitwise XOR calculation */
  number_bitwise_shift_left, /**< bitwise LEFT SHIFT calculation */
  number_bitwise_shift_right, /**< bitwise RIGHT_SHIFT calculation */
  number_bitwise_shift_uright, /**< bitwise UNSIGNED RIGHT SHIFT calculation */
  number_bitwise_not, /**< bitwise NOT calculation */
} number_bitwise_logic_op;

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
do_number_bitwise_logic (int_data_t *int_data, /**< interpreter context */
                         idx_t dst_var_idx, /**< destination variable identifier */
                         number_bitwise_logic_op op, /**< number bitwise logic operation */
                         ecma_value_t left_value, /**< left value */
                         ecma_value_t right_value) /** right value */
{
  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (num_left_value, ecma_op_to_number (left_value), ret_value);
  ECMA_TRY_CATCH (num_right_value, ecma_op_to_number (right_value), ret_value);

  ecma_number_t *left_p, *right_p;
  left_p = ecma_get_number_from_completion_value (num_left_value);
  right_p = ecma_get_number_from_completion_value (num_right_value);

  ecma_number_t* res_p = int_data->tmp_num_p;

  int32_t left_int32 = ecma_number_to_int32 (*left_p);
  // int32_t right_int32 = ecma_number_to_int32 (*right_p);

  uint32_t left_uint32 = ecma_number_to_uint32 (*left_p);
  uint32_t right_uint32 = ecma_number_to_uint32 (*right_p);

  switch (op)
  {
    case number_bitwise_logic_and:
    {
      *res_p = ecma_int32_to_number ((int32_t) (left_uint32 & right_uint32));
      break;
    }
    case number_bitwise_logic_or:
    {
      *res_p = ecma_int32_to_number ((int32_t) (left_uint32 | right_uint32));
      break;
    }
    case number_bitwise_logic_xor:
    {
      *res_p = ecma_int32_to_number ((int32_t) (left_uint32 ^ right_uint32));
      break;
    }
    case number_bitwise_shift_left:
    {
      *res_p = ecma_int32_to_number (left_int32 << (right_uint32 & 0x1F));
      break;
    }
    case number_bitwise_shift_right:
    {
      *res_p = ecma_int32_to_number (left_int32 >> (right_uint32 & 0x1F));
      break;
    }
    case number_bitwise_shift_uright:
    {
      *res_p = ecma_uint32_to_number (left_uint32 >> (right_uint32 & 0x1F));
      break;
    }
    case number_bitwise_not:
    {
      *res_p = ecma_int32_to_number ((int32_t) ~right_uint32);
      break;
    }
  }

  ret_value = set_variable_value (int_data,
                                  dst_var_idx,
                                  ecma_make_number_value (res_p));

  ECMA_FINALIZE (num_right_value);
  ECMA_FINALIZE (num_left_value);

  return ret_value;
} /* do_number_bitwise_logic */

/**
 * 'Bitwise AND' opcode handler.
 *
 * See also: ECMA-262 v5, 11.10
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_b_and (opcode_t opdata, /**< operation data */
              int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.b_and.dst;
  const idx_t left_var_idx = opdata.data.b_and.var_left;
  const idx_t right_var_idx = opdata.data.b_and.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  ret_value = do_number_bitwise_logic (int_data,
                                       dst_var_idx,
                                       number_bitwise_logic_and,
                                       ecma_get_completion_value_value (left_value),
                                       ecma_get_completion_value_value (right_value));

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
opfunc_b_or (opcode_t opdata, /**< operation data */
             int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.b_or.dst;
  const idx_t left_var_idx = opdata.data.b_or.var_left;
  const idx_t right_var_idx = opdata.data.b_or.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  ret_value = do_number_bitwise_logic (int_data,
                                       dst_var_idx,
                                       number_bitwise_logic_or,
                                       ecma_get_completion_value_value (left_value),
                                       ecma_get_completion_value_value (right_value));

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
opfunc_b_xor (opcode_t opdata, /**< operation data */
              int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.b_xor.dst;
  const idx_t left_var_idx = opdata.data.b_xor.var_left;
  const idx_t right_var_idx = opdata.data.b_xor.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  ret_value = do_number_bitwise_logic (int_data,
                                       dst_var_idx,
                                       number_bitwise_logic_xor,
                                       ecma_get_completion_value_value (left_value),
                                       ecma_get_completion_value_value (right_value));

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_b_xor */

/**
 * 'Left Shift Operator' opcode handler.
 *
 * See also: ECMA-262 v5, 11.7.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_b_shift_left (opcode_t opdata, /**< operation data */
                     int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.b_shift_left.dst;
  const idx_t left_var_idx = opdata.data.b_shift_left.var_left;
  const idx_t right_var_idx = opdata.data.b_shift_left.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  ret_value = do_number_bitwise_logic (int_data,
                                       dst_var_idx,
                                       number_bitwise_shift_left,
                                       ecma_get_completion_value_value (left_value),
                                       ecma_get_completion_value_value (right_value));

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_b_shift_left */

/**
 * 'Right Shift Operator' opcode handler.
 *
 * See also: ECMA-262 v5, 11.7.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_b_shift_right (opcode_t opdata, /**< operation data */
                      int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.b_shift_right.dst;
  const idx_t left_var_idx = opdata.data.b_shift_right.var_left;
  const idx_t right_var_idx = opdata.data.b_shift_right.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  ret_value = do_number_bitwise_logic (int_data,
                                       dst_var_idx,
                                       number_bitwise_shift_right,
                                       ecma_get_completion_value_value (left_value),
                                       ecma_get_completion_value_value (right_value));

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_b_shift_right */

/**
 * 'Unsigned Right Shift Operator' opcode handler.
 *
 * See also: ECMA-262 v5, 11.7.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_b_shift_uright (opcode_t opdata, /**< operation data */
                      int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.b_shift_uright.dst;
  const idx_t left_var_idx = opdata.data.b_shift_uright.var_left;
  const idx_t right_var_idx = opdata.data.b_shift_uright.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  ret_value = do_number_bitwise_logic (int_data,
                                       dst_var_idx,
                                       number_bitwise_shift_uright,
                                       ecma_get_completion_value_value (left_value),
                                       ecma_get_completion_value_value (right_value));

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_b_shift_uright */

/**
 * 'Bitwise NOT Operator' opcode handler.
 *
 * See also: ECMA-262 v5, 10.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_b_not (opcode_t opdata, /**< operation data */
              int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.b_not.dst;
  const idx_t right_var_idx = opdata.data.b_not.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  ret_value = do_number_bitwise_logic (int_data,
                                       dst_var_idx,
                                       number_bitwise_not,
                                       ecma_get_completion_value_value (right_value),
                                       ecma_get_completion_value_value (right_value));

  ECMA_FINALIZE (right_value);

  return ret_value;
} /* opfunc_b_not */
