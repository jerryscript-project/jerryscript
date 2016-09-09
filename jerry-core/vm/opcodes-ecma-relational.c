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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
opfunc_less_than (ecma_value_t left_value, /**< left value */
                  ecma_value_t right_value) /**< right value */
{
  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (left_value)
                && !ECMA_IS_VALUE_ERROR (right_value));

  ecma_value_t ret_value = ecma_op_abstract_relational_compare (left_value, right_value, true);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    return ret_value;
  }

  if (ecma_is_value_undefined (ret_value))
  {
    ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (ret_value));
  }

  return ret_value;
} /* opfunc_less_than */

/**
 * 'Greater-than' opcode handler.
 *
 * See also: ECMA-262 v5, 11.8.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
opfunc_greater_than (ecma_value_t left_value, /**< left value */
                     ecma_value_t right_value) /**< right value */
{
  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (left_value)
                && !ECMA_IS_VALUE_ERROR (right_value));

  ecma_value_t ret_value = ecma_op_abstract_relational_compare (left_value, right_value, false);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    return ret_value;
  }

  if (ecma_is_value_undefined (ret_value))
  {
    ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (ret_value));
  }

  return ret_value;
} /* opfunc_greater_than */

/**
 * 'Less-than-or-equal' opcode handler.
 *
 * See also: ECMA-262 v5, 11.8.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
opfunc_less_or_equal_than (ecma_value_t left_value, /**< left value */
                           ecma_value_t right_value) /**< right value */
{
  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (left_value)
                && !ECMA_IS_VALUE_ERROR (right_value));

  ecma_value_t ret_value = ecma_op_abstract_relational_compare (left_value, right_value, false);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    return ret_value;
  }

  if (ecma_is_value_undefined (ret_value))
  {
    ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (ret_value));

    ret_value = ecma_invert_boolean_value (ret_value);
  }

  return ret_value;
} /* opfunc_less_or_equal_than */

/**
 * 'Greater-than-or-equal' opcode handler.
 *
 * See also: ECMA-262 v5, 11.8.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
opfunc_greater_or_equal_than (ecma_value_t left_value, /**< left value */
                              ecma_value_t right_value) /**< right value */
{
  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (left_value)
                && !ECMA_IS_VALUE_ERROR (right_value));

  ecma_value_t ret_value = ecma_op_abstract_relational_compare (left_value, right_value, true);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    return ret_value;
  }

  if (ecma_is_value_undefined (ret_value))
  {
    ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (ret_value));

    ret_value = ecma_invert_boolean_value (ret_value);
  }

  return ret_value;
} /* opfunc_greater_or_equal_than */

/**
 * 'instanceof' opcode handler.
 *
 * See also: ECMA-262 v5, 11.8.6
 *
 * @return ecma value
 *         returned value must be freed with ecma_free_value.
 */
ecma_value_t
opfunc_instanceof (ecma_value_t left_value, /**< left value */
                   ecma_value_t right_value) /**< right value */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  if (!ecma_is_value_object (right_value))
  {
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG (""));
  }
  else
  {
    ecma_object_t *right_value_obj_p = ecma_get_object_from_value (right_value);

    ECMA_TRY_CATCH (is_instance_of,
                    ecma_op_object_has_instance (right_value_obj_p, left_value),
                    ret_value);

    ret_value = is_instance_of;

    ECMA_FINALIZE (is_instance_of);
  }

  return ret_value;
} /* opfunc_instanceof */

/**
 * 'in' opcode handler.
 *
 * See also: ECMA-262 v5, 11.8.7
 *
 * @return ecma value
 *         returned value must be freed with ecma_free_value.
 */
ecma_value_t
opfunc_in (ecma_value_t left_value, /**< left value */
           ecma_value_t right_value) /**< right value */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  if (!ecma_is_value_object (right_value))
  {
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG (""));
  }
  else
  {
    ECMA_TRY_CATCH (str_left_value, ecma_op_to_string (left_value), ret_value);

    ecma_string_t *left_value_prop_name_p = ecma_get_string_from_value (str_left_value);
    ecma_object_t *right_value_obj_p = ecma_get_object_from_value (right_value);

    if (ecma_op_object_has_property (right_value_obj_p, left_value_prop_name_p))
    {
      ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
    }
    else
    {
      ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
    }

    ECMA_FINALIZE (str_left_value);
  }

  return ret_value;
} /* opfunc_in */

/**
 * @}
 * @}
 */
