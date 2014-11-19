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
 * 'Less-than' opcode handler.
 *
 * See also: ECMA-262 v5, 11.8.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_less_than (opcode_t opdata, /**< operation data */
                  int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.less_than.dst;
  const idx_t left_var_idx = opdata.data.less_than.var_left;
  const idx_t right_var_idx = opdata.data.less_than.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);
  ECMA_TRY_CATCH (compare_result,
                  ecma_op_abstract_relational_compare (left_value.u.value,
                                                       right_value.u.value,
                                                       true),
                  ret_value);

  ecma_simple_value_t res;

  if (ecma_is_value_undefined (compare_result.u.value))
  {
    res = ECMA_SIMPLE_VALUE_FALSE;
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (compare_result.u.value));

    res = compare_result.u.value.value;
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
opfunc_greater_than (opcode_t opdata, /**< operation data */
                     int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.greater_than.dst;
  const idx_t left_var_idx = opdata.data.greater_than.var_left;
  const idx_t right_var_idx = opdata.data.greater_than.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);
  ECMA_TRY_CATCH (compare_result,
                  ecma_op_abstract_relational_compare (right_value.u.value,
                                                       left_value.u.value,
                                                       false),
                  ret_value);

  ecma_simple_value_t res;

  if (ecma_is_value_undefined (compare_result.u.value))
  {
    res = ECMA_SIMPLE_VALUE_FALSE;
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (compare_result.u.value));

    res = compare_result.u.value.value;
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
opfunc_less_or_equal_than (opcode_t opdata, /**< operation data */
                           int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.less_or_equal_than.dst;
  const idx_t left_var_idx = opdata.data.less_or_equal_than.var_left;
  const idx_t right_var_idx = opdata.data.less_or_equal_than.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);
  ECMA_TRY_CATCH (compare_result,
                  ecma_op_abstract_relational_compare (right_value.u.value,
                                                       left_value.u.value,
                                                       false),
                  ret_value);

  ecma_simple_value_t res;

  if (ecma_is_value_undefined (compare_result.u.value))
  {
    res = ECMA_SIMPLE_VALUE_FALSE;
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (compare_result.u.value));

    if (compare_result.u.value.value == ECMA_SIMPLE_VALUE_TRUE)
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
opfunc_greater_or_equal_than (opcode_t opdata, /**< operation data */
                              int_data_t *int_data) /**< interpreter context */
{
  const idx_t dst_var_idx = opdata.data.greater_or_equal_than.dst;
  const idx_t left_var_idx = opdata.data.greater_or_equal_than.var_left;
  const idx_t right_var_idx = opdata.data.greater_or_equal_than.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);
  ECMA_TRY_CATCH (compare_result,
                  ecma_op_abstract_relational_compare (left_value.u.value,
                                                       right_value.u.value,
                                                       true),
                  ret_value);

  ecma_simple_value_t res;

  if (ecma_is_value_undefined (compare_result.u.value))
  {
    res = ECMA_SIMPLE_VALUE_FALSE;
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (compare_result.u.value));

    if (compare_result.u.value.value == ECMA_SIMPLE_VALUE_TRUE)
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
 * 'instanceof' opcode handler.
 *
 * See also: ECMA-262 v5, 11.8.6
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_instanceof (opcode_t opdata __unused, /**< operation data */
                   int_data_t *int_data __unused) /**< interpreter context */
{
  const idx_t dst_idx = opdata.data.instanceof.dst;
  const idx_t left_var_idx = opdata.data.instanceof.var_left;
  const idx_t right_var_idx = opdata.data.instanceof.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  if (!ecma_is_value_object (right_value.u.value))
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    ecma_object_t *right_value_obj_p = ECMA_GET_NON_NULL_POINTER (right_value.u.value.value);

    ECMA_TRY_CATCH (is_instance_of,
                    ecma_op_object_has_instance (right_value_obj_p,
                                                 left_value.u.value),
                    ret_value);

    ret_value = set_variable_value (int_data, dst_idx, is_instance_of.u.value);

    ECMA_FINALIZE (is_instance_of);
  }

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_instanceof */

/**
 * 'in' opcode handler.
 *
 * See also: ECMA-262 v5, 11.8.7
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_in (opcode_t opdata __unused, /**< operation data */
           int_data_t *int_data __unused) /**< interpreter context */
{
  const idx_t dst_idx = opdata.data.in.dst;
  const idx_t left_var_idx = opdata.data.in.var_left;
  const idx_t right_var_idx = opdata.data.in.var_right;

  int_data->pos++;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (left_value, get_variable_value (int_data, left_var_idx, false), ret_value);
  ECMA_TRY_CATCH (right_value, get_variable_value (int_data, right_var_idx, false), ret_value);

  if (!ecma_is_value_object (right_value.u.value))
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    ECMA_TRY_CATCH (str_left_value, ecma_op_to_string (left_value.u.value), ret_value);

    ecma_simple_value_t is_in = ECMA_SIMPLE_VALUE_UNDEFINED;
    ecma_string_t *left_value_prop_name_p = ECMA_GET_NON_NULL_POINTER (str_left_value.u.value.value);
    ecma_object_t *right_value_obj_p = ECMA_GET_NON_NULL_POINTER (right_value.u.value.value);

    if (ecma_op_object_has_property (right_value_obj_p, left_value_prop_name_p))
    {
      is_in = ECMA_SIMPLE_VALUE_TRUE;
    }
    else
    {
      is_in = ECMA_SIMPLE_VALUE_FALSE;
    }

    ret_value = set_variable_value (int_data,
                                    dst_idx,
                                    ecma_make_simple_value (is_in));

    ECMA_FINALIZE (str_left_value);
  }

  ECMA_FINALIZE (right_value);
  ECMA_FINALIZE (left_value);

  return ret_value;
} /* opfunc_in */
