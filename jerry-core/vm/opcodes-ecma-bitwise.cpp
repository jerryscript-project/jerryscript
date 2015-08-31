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
do_number_bitwise_logic (vm_frame_ctx_t *frame_ctx_p, /**< interpreter context */
                         vm_idx_t dst_var_idx, /**< destination variable identifier */
                         number_bitwise_logic_op op, /**< number bitwise logic operation */
                         ecma_value_t left_value, /**< left value */
                         ecma_value_t right_value) /** right value */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_OP_TO_NUMBER_TRY_CATCH (num_left, left_value, ret_value);
  ECMA_OP_TO_NUMBER_TRY_CATCH (num_right, right_value, ret_value);

  ecma_number_t* res_p = frame_ctx_p->tmp_num_p;

  int32_t left_int32 = ecma_number_to_int32 (num_left);
  // int32_t right_int32 = ecma_number_to_int32 (num_right);

  uint32_t left_uint32 = ecma_number_to_uint32 (num_left);
  uint32_t right_uint32 = ecma_number_to_uint32 (num_right);

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

  ret_value = set_variable_value (frame_ctx_p, frame_ctx_p->pos,
                                  dst_var_idx,
                                  ecma_make_number_value (res_p));

  ECMA_OP_TO_NUMBER_FINALIZE (num_right);
  ECMA_OP_TO_NUMBER_FINALIZE (num_left);

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
opfunc_b_and (vm_instr_t instr, /**< instruction */
              vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.b_and.dst;
  const vm_idx_t left_var_idx = instr.data.b_and.var_left;
  const vm_idx_t right_var_idx = instr.data.b_and.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (left_value, get_variable_value (frame_ctx_p, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (frame_ctx_p, right_var_idx, false), ret_value);

  ret_value = do_number_bitwise_logic (frame_ctx_p,
                                       dst_var_idx,
                                       number_bitwise_logic_and,
                                       left_value,
                                       right_value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  frame_ctx_p->pos++;

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
opfunc_b_or (vm_instr_t instr, /**< instruction */
             vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.b_or.dst;
  const vm_idx_t left_var_idx = instr.data.b_or.var_left;
  const vm_idx_t right_var_idx = instr.data.b_or.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (left_value, get_variable_value (frame_ctx_p, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (frame_ctx_p, right_var_idx, false), ret_value);

  ret_value = do_number_bitwise_logic (frame_ctx_p,
                                       dst_var_idx,
                                       number_bitwise_logic_or,
                                       left_value,
                                       right_value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  frame_ctx_p->pos++;

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
opfunc_b_xor (vm_instr_t instr, /**< instruction */
              vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.b_xor.dst;
  const vm_idx_t left_var_idx = instr.data.b_xor.var_left;
  const vm_idx_t right_var_idx = instr.data.b_xor.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (left_value, get_variable_value (frame_ctx_p, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (frame_ctx_p, right_var_idx, false), ret_value);

  ret_value = do_number_bitwise_logic (frame_ctx_p,
                                       dst_var_idx,
                                       number_bitwise_logic_xor,
                                       left_value,
                                       right_value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  frame_ctx_p->pos++;

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
opfunc_b_shift_left (vm_instr_t instr, /**< instruction */
                     vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.b_shift_left.dst;
  const vm_idx_t left_var_idx = instr.data.b_shift_left.var_left;
  const vm_idx_t right_var_idx = instr.data.b_shift_left.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (left_value, get_variable_value (frame_ctx_p, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (frame_ctx_p, right_var_idx, false), ret_value);

  ret_value = do_number_bitwise_logic (frame_ctx_p,
                                       dst_var_idx,
                                       number_bitwise_shift_left,
                                       left_value,
                                       right_value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  frame_ctx_p->pos++;

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
opfunc_b_shift_right (vm_instr_t instr, /**< instruction */
                      vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.b_shift_right.dst;
  const vm_idx_t left_var_idx = instr.data.b_shift_right.var_left;
  const vm_idx_t right_var_idx = instr.data.b_shift_right.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (left_value, get_variable_value (frame_ctx_p, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (frame_ctx_p, right_var_idx, false), ret_value);

  ret_value = do_number_bitwise_logic (frame_ctx_p,
                                       dst_var_idx,
                                       number_bitwise_shift_right,
                                       left_value,
                                       right_value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  frame_ctx_p->pos++;

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
opfunc_b_shift_uright (vm_instr_t instr, /**< instruction */
                      vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.b_shift_uright.dst;
  const vm_idx_t left_var_idx = instr.data.b_shift_uright.var_left;
  const vm_idx_t right_var_idx = instr.data.b_shift_uright.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (left_value, get_variable_value (frame_ctx_p, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (frame_ctx_p, right_var_idx, false), ret_value);

  ret_value = do_number_bitwise_logic (frame_ctx_p,
                                       dst_var_idx,
                                       number_bitwise_shift_uright,
                                       left_value,
                                       right_value);

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  frame_ctx_p->pos++;

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
opfunc_b_not (vm_instr_t instr, /**< instruction */
              vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.b_not.dst;
  const vm_idx_t right_var_idx = instr.data.b_not.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (right_value, get_variable_value (frame_ctx_p, right_var_idx, false), ret_value);

  ret_value = do_number_bitwise_logic (frame_ctx_p,
                                       dst_var_idx,
                                       number_bitwise_not,
                                       right_value,
                                       right_value);

  ECMA_FINALIZE (right_value);

  frame_ctx_p->pos++;

  return ret_value;
} /* opfunc_b_not */
