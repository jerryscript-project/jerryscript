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
 * 'Equals' opcode handler.
 *
 * See also: ECMA-262 v5, 11.9.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_equal_value (vm_instr_t instr, /**< instruction */
                    vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.equal_value.dst;
  const vm_idx_t left_var_idx = instr.data.equal_value.var_left;
  const vm_idx_t right_var_idx = instr.data.equal_value.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (left_value, get_variable_value (frame_ctx_p, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (frame_ctx_p, right_var_idx, false), ret_value);
  ECMA_TRY_CATCH (compare_result,
                  ecma_op_abstract_equality_compare (left_value,
                                                     right_value),
                  ret_value);

  JERRY_ASSERT (ecma_is_value_boolean (compare_result));

  ret_value = set_variable_value (frame_ctx_p, frame_ctx_p->pos, dst_var_idx,
                                  compare_result);

  ECMA_FINALIZE (compare_result);
  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  frame_ctx_p->pos++;

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
opfunc_not_equal_value (vm_instr_t instr, /**< instruction */
                        vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.not_equal_value.dst;
  const vm_idx_t left_var_idx = instr.data.not_equal_value.var_left;
  const vm_idx_t right_var_idx = instr.data.not_equal_value.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (left_value, get_variable_value (frame_ctx_p, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (frame_ctx_p, right_var_idx, false), ret_value);
  ECMA_TRY_CATCH (compare_result,
                  ecma_op_abstract_equality_compare (left_value, right_value),
                  ret_value);

  JERRY_ASSERT (ecma_is_value_boolean (compare_result));

  bool is_equal = ecma_is_value_true (compare_result);

  ret_value = set_variable_value (frame_ctx_p, frame_ctx_p->pos, dst_var_idx,
                                  ecma_make_simple_value (is_equal ? ECMA_SIMPLE_VALUE_FALSE
                                                          : ECMA_SIMPLE_VALUE_TRUE));

  ECMA_FINALIZE (compare_result);
  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  frame_ctx_p->pos++;

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
opfunc_equal_value_type (vm_instr_t instr, /**< instruction */
                         vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.equal_value_type.dst;
  const vm_idx_t left_var_idx = instr.data.equal_value_type.var_left;
  const vm_idx_t right_var_idx = instr.data.equal_value_type.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (left_value, get_variable_value (frame_ctx_p, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (frame_ctx_p, right_var_idx, false), ret_value);

  bool is_equal = ecma_op_strict_equality_compare (left_value, right_value);

  ret_value = set_variable_value (frame_ctx_p, frame_ctx_p->pos, dst_var_idx,
                                  ecma_make_simple_value (is_equal ? ECMA_SIMPLE_VALUE_TRUE
                                                          : ECMA_SIMPLE_VALUE_FALSE));

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  frame_ctx_p->pos++;

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
opfunc_not_equal_value_type (vm_instr_t instr, /**< instruction */
                             vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t dst_var_idx = instr.data.not_equal_value_type.dst;
  const vm_idx_t left_var_idx = instr.data.not_equal_value_type.var_left;
  const vm_idx_t right_var_idx = instr.data.not_equal_value_type.var_right;

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (left_value, get_variable_value (frame_ctx_p, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (frame_ctx_p, right_var_idx, false), ret_value);

  bool is_equal = ecma_op_strict_equality_compare (left_value, right_value);

  ret_value = set_variable_value (frame_ctx_p, frame_ctx_p->pos, dst_var_idx,
                                  ecma_make_simple_value (is_equal ? ECMA_SIMPLE_VALUE_FALSE
                                                          : ECMA_SIMPLE_VALUE_TRUE));


  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  frame_ctx_p->pos++;

  return ret_value;
} /* opfunc_not_equal_value_type */
