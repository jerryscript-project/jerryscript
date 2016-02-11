/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2015-2016 University of Szeged.
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

#include "ecma-comparison.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"
#include "opcodes.h"

/** \addtogroup vm Virtual machine
 * @{
 *
 * \addtogroup vm_opcodes Opcodes
 * @{
 */

/**
 * 'Less-than' opcode handler.
 *
 * See also: ECMA-262 v5, 11.8.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_less_than (ecma_value_t left_value, /**< left value */
                  ecma_value_t right_value) /**< right value */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (compare_result,
                  ecma_op_abstract_relational_compare (left_value, right_value, true),
                  ret_value);

  ecma_simple_value_t res;

  if (ecma_is_value_undefined (compare_result))
  {
    res = ECMA_SIMPLE_VALUE_FALSE;
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (compare_result));

    res = (ecma_is_value_true (compare_result) ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE);
  }

  ret_value = ecma_make_simple_completion_value (res);

  ECMA_FINALIZE (compare_result);

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
opfunc_greater_than (ecma_value_t left_value, /**< left value */
                     ecma_value_t right_value) /**< right value */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (compare_result,
                  ecma_op_abstract_relational_compare (right_value, left_value, false),
                  ret_value);

  ecma_simple_value_t res;

  if (ecma_is_value_undefined (compare_result))
  {
    res = ECMA_SIMPLE_VALUE_FALSE;
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (compare_result));

    res = (ecma_is_value_true (compare_result) ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE);
  }

  ret_value = ecma_make_simple_completion_value (res);

  ECMA_FINALIZE (compare_result);

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
opfunc_less_or_equal_than (ecma_value_t left_value, /**< left value */
                           ecma_value_t right_value) /**< right value */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (compare_result,
                  ecma_op_abstract_relational_compare (right_value, left_value, false),
                  ret_value);

  ecma_simple_value_t res;

  if (ecma_is_value_undefined (compare_result))
  {
    res = ECMA_SIMPLE_VALUE_FALSE;
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (compare_result));

    if (ecma_is_value_true (compare_result))
    {
      res = ECMA_SIMPLE_VALUE_FALSE;
    }
    else
    {
      res = ECMA_SIMPLE_VALUE_TRUE;
    }
  }

  ret_value = ecma_make_simple_completion_value (res);

  ECMA_FINALIZE (compare_result);

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
opfunc_greater_or_equal_than (ecma_value_t left_value, /**< left value */
                              ecma_value_t right_value) /**< right value */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (compare_result,
                  ecma_op_abstract_relational_compare (left_value, right_value, true),
                  ret_value);

  ecma_simple_value_t res;

  if (ecma_is_value_undefined (compare_result))
  {
    res = ECMA_SIMPLE_VALUE_FALSE;
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (compare_result));

    if (ecma_is_value_true (compare_result))
    {
      res = ECMA_SIMPLE_VALUE_FALSE;
    }
    else
    {
      res = ECMA_SIMPLE_VALUE_TRUE;
    }
  }

  ret_value = ecma_make_simple_completion_value (res);

  ECMA_FINALIZE (compare_result);

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
opfunc_instanceof (ecma_value_t left_value, /**< left value */
                   ecma_value_t right_value) /**< right value */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (!ecma_is_value_object (right_value))
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    ecma_object_t *right_value_obj_p = ecma_get_object_from_value (right_value);

    ECMA_TRY_CATCH (is_instance_of,
                    ecma_op_object_has_instance (right_value_obj_p, left_value),
                    ret_value);

    ret_value = ecma_make_normal_completion_value (is_instance_of);

    ECMA_FINALIZE (is_instance_of);
  }

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
opfunc_in (ecma_value_t left_value, /**< left value */
           ecma_value_t right_value) /**< right value */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (!ecma_is_value_object (right_value))
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    ECMA_TRY_CATCH (str_left_value, ecma_op_to_string (left_value), ret_value);

    ecma_simple_value_t is_in = ECMA_SIMPLE_VALUE_UNDEFINED;
    ecma_string_t *left_value_prop_name_p = ecma_get_string_from_value (str_left_value);
    ecma_object_t *right_value_obj_p = ecma_get_object_from_value (right_value);

    if (ecma_op_object_get_property (right_value_obj_p, left_value_prop_name_p) != NULL)
    {
      is_in = ECMA_SIMPLE_VALUE_TRUE;
    }
    else
    {
      is_in = ECMA_SIMPLE_VALUE_FALSE;
    }

    ret_value = ecma_make_simple_completion_value (is_in);

    ECMA_FINALIZE (str_left_value);
  }

  return ret_value;
} /* opfunc_in */

/**
 * @}
 * @}
 */
