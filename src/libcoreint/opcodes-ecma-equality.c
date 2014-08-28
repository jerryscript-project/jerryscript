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
 * 'Equals' opcode handler.
 *
 * See also: ECMA-262 v5, 11.9.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_equal_value (opcode_t opdata, /**< operation data */
                    int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.equal_value.dst;
  const idx_t left_var_idx = opdata.data.equal_value.var_left;
  const idx_t right_var_idx = opdata.data.equal_value.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  bool is_equal = ecma_op_abstract_equality_compare (left_value.u.value, right_value.u.value);

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
opfunc_not_equal_value (opcode_t opdata, /**< operation data */
                        int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.not_equal_value.dst;
  const idx_t left_var_idx = opdata.data.not_equal_value.var_left;
  const idx_t right_var_idx = opdata.data.not_equal_value.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  bool is_equal = ecma_op_abstract_equality_compare (left_value.u.value, right_value.u.value);

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
opfunc_equal_value_type (opcode_t opdata, /**< operation data */
                         int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.equal_value_type.dst;
  const idx_t left_var_idx = opdata.data.equal_value_type.var_left;
  const idx_t right_var_idx = opdata.data.equal_value_type.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  bool is_equal = ecma_op_strict_equality_compare (left_value.u.value, right_value.u.value);

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
opfunc_not_equal_value_type (opcode_t opdata, /**< operation data */
                             int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.not_equal_value_type.dst;
  const idx_t left_var_idx = opdata.data.not_equal_value_type.var_left;
  const idx_t right_var_idx = opdata.data.not_equal_value_type.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  bool is_equal = ecma_op_strict_equality_compare (left_value.u.value, right_value.u.value);

  ret_value = set_variable_value (int_data, dst_var_idx, ecma_make_simple_value (is_equal ? ECMA_SIMPLE_VALUE_FALSE
                                                                                 : ECMA_SIMPLE_VALUE_TRUE));


  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_not_equal_value_type */
