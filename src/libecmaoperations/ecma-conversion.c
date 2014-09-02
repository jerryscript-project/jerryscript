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

/**
 * Implementation of ECMA-defined conversion routines
 */

#include "ecma-alloc.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-globals.h"
#include "ecma-magic-strings.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"
#include "jerry-libc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaconversion ECMA conversion routines
 * @{
 */

/**
 * CheckObjectCoercible operation.
 *
 * See also:
 *          ECMA-262 v5, 9.10
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_check_object_coercible (ecma_value_t value) /**< ecma-value */
{
  switch ((ecma_type_t)value.value_type)
  {
    case ECMA_TYPE_SIMPLE:
    {
      if (ecma_is_value_undefined (value)
          || ecma_is_value_null (value))
      {
        return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
      }
      else if (ecma_is_value_boolean (value))
      {
        break;
      }
      else
      {
        JERRY_UNREACHABLE();
      }

      break;
    }

    case ECMA_TYPE_NUMBER:
    case ECMA_TYPE_STRING:
    case ECMA_TYPE_OBJECT:
    {
      break;
    }
  }

  return ecma_make_empty_completion_value ();
} /* ecma_op_check_object_coercible */

/**
 * SameValue operation.
 *
 * See also:
 *          ECMA-262 v5, 9.12
 *
 * @return true - if the value are same according to ECMA-defined SameValue algorithm,
 *         false - otherwise.
 */
bool
ecma_op_same_value (ecma_value_t x, /**< ecma-value */
                    ecma_value_t y) /**< ecma-value */
{
  const bool is_x_undefined = ecma_is_value_undefined (x);
  const bool is_x_null = ecma_is_value_null (x);
  const bool is_x_boolean = ecma_is_value_boolean (x);
  const bool is_x_number = (x.value_type == ECMA_TYPE_NUMBER);
  const bool is_x_string = (x.value_type == ECMA_TYPE_STRING);
  const bool is_x_object = (x.value_type == ECMA_TYPE_OBJECT);

  const bool is_y_undefined = ecma_is_value_undefined (y);
  const bool is_y_null = ecma_is_value_null (y);
  const bool is_y_boolean = ecma_is_value_boolean (y);
  const bool is_y_number = (y.value_type == ECMA_TYPE_NUMBER);
  const bool is_y_string = (y.value_type == ECMA_TYPE_STRING);
  const bool is_y_object = (y.value_type == ECMA_TYPE_OBJECT);

  const bool is_types_equal = ((is_x_undefined && is_y_undefined)
                               || (is_x_null && is_y_null)
                               || (is_x_boolean && is_y_boolean)
                               || (is_x_number && is_y_number)
                               || (is_x_string && is_y_string)
                               || (is_x_object && is_y_object));

  if (!is_types_equal)
  {
    return false;
  }

  if (is_x_undefined
      || is_x_null)
  {
    return true;
  }

  if (is_x_number)
  {
    ecma_number_t *x_num_p = (ecma_number_t*)ECMA_GET_POINTER(x.value);
    ecma_number_t *y_num_p = (ecma_number_t*)ECMA_GET_POINTER(y.value);

    if (ecma_number_is_nan (*x_num_p)
        && ecma_number_is_nan (*y_num_p))
    {
      return true;
    }
    else if (ecma_number_is_zero (*x_num_p)
             && ecma_number_is_zero (*y_num_p)
             && ecma_number_is_negative (*x_num_p) != ecma_number_is_negative (*y_num_p))
    {
      return false;
    }

    return (*x_num_p == *y_num_p);
  }

  if (is_x_string)
  {
    ecma_string_t* x_str_p = ECMA_GET_POINTER(x.value);
    ecma_string_t* y_str_p = ECMA_GET_POINTER(y.value);

    return ecma_compare_ecma_string_to_ecma_string (x_str_p, y_str_p);
  }

  if (is_x_boolean)
  {
    return (ecma_is_value_true (x) == ecma_is_value_true (y));
  }

  JERRY_ASSERT(is_x_object);

  return (ECMA_GET_POINTER(x.value) == ECMA_GET_POINTER(y.value));
} /* ecma_op_same_value */

/**
 * ToPrimitive operation.
 *
 * See also:
 *          ECMA-262 v5, 9.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_to_primitive (ecma_value_t value, /**< ecma-value */
                      ecma_preferred_type_hint_t preferred_type) /**< preferred type hint */
{
  switch ((ecma_type_t)value.value_type)
  {
    case ECMA_TYPE_SIMPLE:
    case ECMA_TYPE_NUMBER:
    case ECMA_TYPE_STRING:
    {
      return ecma_make_normal_completion_value (ecma_copy_value (value, true));
    }

    case ECMA_TYPE_OBJECT:
    {
      ecma_object_t *obj_p = ECMA_GET_POINTER (value.value);

      return ecma_op_object_default_value (obj_p, preferred_type);
    }
  }

  JERRY_UNREACHABLE();
} /* ecma_op_to_primitive */

/**
 * ToBoolean operation.
 *
 * See also:
 *          ECMA-262 v5, 9.2
 *
 * @return completion value
 *         Returned value is simple and so need not be freed.
 *         However, ecma_free_completion_value may be called for it, but it is a no-op.
 */
ecma_completion_value_t
ecma_op_to_boolean (ecma_value_t value) /**< ecma-value */
{
  switch ((ecma_type_t)value.value_type)
  {
    case ECMA_TYPE_NUMBER:
    {
      ecma_number_t *num_p = ECMA_GET_POINTER(value.value);

      if (ecma_number_is_nan (*num_p)
          || ecma_number_is_zero (*num_p))
      {
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE);
      }
      else
      {
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
      }

      break;
    }
    case ECMA_TYPE_SIMPLE:
    {
      if (ecma_is_value_boolean (value))
      {
        return ecma_make_simple_completion_value (value.value);
      }
      else if (ecma_is_value_undefined (value)
                 || ecma_is_value_null (value))
      {
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE);
      }
      else
      {
        JERRY_UNREACHABLE();
      }

      break;
    }
    case ECMA_TYPE_STRING:
    {
      ecma_string_t *str_p = ECMA_GET_POINTER(value.value);

      return ecma_make_simple_completion_value ((ecma_string_get_length (str_p) == 0) ? ECMA_SIMPLE_VALUE_FALSE
                                                : ECMA_SIMPLE_VALUE_TRUE);

      break;
    }
    case ECMA_TYPE_OBJECT:
    {
      return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);

      break;
    }
  }

  JERRY_UNREACHABLE();
} /* ecma_op_to_boolean */

/**
 * ToNumber operation.
 *
 * See also:
 *          ECMA-262 v5, 9.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_to_number (ecma_value_t value) /**< ecma-value */
{
  switch ((ecma_type_t)value.value_type)
  {
    case ECMA_TYPE_NUMBER:
    {
      return ecma_make_normal_completion_value (ecma_copy_value (value, true));
    }
    case ECMA_TYPE_SIMPLE:
    {
      ecma_number_t *num_p = ecma_alloc_number ();

      switch ((ecma_simple_value_t)value.value)
      {
        case ECMA_SIMPLE_VALUE_UNDEFINED:
        {
          *num_p = ecma_number_make_nan ();

          break;
        }
        case ECMA_SIMPLE_VALUE_NULL:
        case ECMA_SIMPLE_VALUE_FALSE:
        {
          *num_p = ECMA_NUMBER_ZERO;

          break;
        }
        case ECMA_SIMPLE_VALUE_TRUE:
        {
          *num_p = ECMA_NUMBER_ONE;

          break;
        }
        case ECMA_SIMPLE_VALUE_EMPTY:
        case ECMA_SIMPLE_VALUE_ARRAY_REDIRECT:
        case ECMA_SIMPLE_VALUE__COUNT:
        {
          JERRY_UNREACHABLE ();
        }
      }

      return ecma_make_normal_completion_value (ecma_make_number_value (num_p));
    }
    case ECMA_TYPE_STRING:
    {
      ecma_string_t *str_p = ECMA_GET_POINTER (value.value);

      ecma_number_t *num_p = ecma_alloc_number ();
      *num_p = ecma_string_to_number (str_p);

      return ecma_make_normal_completion_value (ecma_make_number_value (num_p));
    }
    case ECMA_TYPE_OBJECT:
    {
      ecma_completion_value_t completion_to_primitive = ecma_op_to_primitive (value, ECMA_PREFERRED_TYPE_NUMBER);
      JERRY_ASSERT(ecma_is_completion_value_normal (completion_to_primitive));

      ecma_completion_value_t completion_to_number = ecma_op_to_number (completion_to_primitive.u.value);
      ecma_free_completion_value (completion_to_primitive);

      return completion_to_number;
    }
  }

  JERRY_UNREACHABLE();
} /* ecma_op_to_number */

/**
 * ToString operation.
 *
 * See also:
 *          ECMA-262 v5, 9.8
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_to_string (ecma_value_t value) /**< ecma-value */
{
  if (unlikely (value.value_type == ECMA_TYPE_OBJECT))
  {
    ecma_completion_value_t ret_value;

    ECMA_TRY_CATCH (prim_value,
                    ecma_op_to_primitive (value, ECMA_PREFERRED_TYPE_STRING),
                    ret_value);

    ret_value = ecma_op_to_string (prim_value.u.value);

    ECMA_FINALIZE (prim_value);

    return ret_value;
  }

  ecma_string_t *res_p = NULL;

  switch ((ecma_type_t) value.value_type)
  {
    case ECMA_TYPE_SIMPLE:
    {
      switch ((ecma_simple_value_t) value.value)
      {
        case ECMA_SIMPLE_VALUE_UNDEFINED:
        {
          res_p = ecma_get_magic_string (ECMA_MAGIC_STRING_UNDEFINED);
          break;
        }
        case ECMA_SIMPLE_VALUE_NULL:
        {
          res_p = ecma_get_magic_string (ECMA_MAGIC_STRING_NULL);
          break;
        }
        case ECMA_SIMPLE_VALUE_FALSE:
        {
          res_p = ecma_get_magic_string (ECMA_MAGIC_STRING_FALSE);
          break;
        }
        case ECMA_SIMPLE_VALUE_TRUE:
        {
          res_p = ecma_get_magic_string (ECMA_MAGIC_STRING_TRUE);
          break;
        }

        case ECMA_SIMPLE_VALUE_EMPTY:
        case ECMA_SIMPLE_VALUE_ARRAY_REDIRECT:
        case ECMA_SIMPLE_VALUE__COUNT:
        {
          JERRY_UNREACHABLE ();
        }
      }

      break;
    }

    case ECMA_TYPE_NUMBER:
    {
      ecma_number_t *num_p = ECMA_GET_POINTER (value.value);
      res_p = ecma_new_ecma_string_from_number (*num_p);

      break;
    }

    case ECMA_TYPE_STRING:
    {
      res_p = ECMA_GET_POINTER (value.value);
      ecma_ref_ecma_string (res_p);

      break;
    }

    case ECMA_TYPE_OBJECT:
    {
      JERRY_UNREACHABLE ();
    }
  }

  return ecma_make_normal_completion_value (ecma_make_string_value (res_p));
} /* ecma_op_to_string */

/**
 * ToObject operation.
 *
 * See also:
 *          ECMA-262 v5, 9.9
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_to_object (ecma_value_t value) /**< ecma-value */
{
  switch ((ecma_type_t)value.value_type)
  {
    case ECMA_TYPE_SIMPLE:
    {
      switch ((ecma_simple_value_t)value.value)
      {
        case ECMA_SIMPLE_VALUE_UNDEFINED:
        case ECMA_SIMPLE_VALUE_NULL:
        {
          return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
        }

        case ECMA_SIMPLE_VALUE_FALSE:
        case ECMA_SIMPLE_VALUE_TRUE:
        {
          /* return Boolean object with [[PrimitiveValue]] set to the value */
          JERRY_UNIMPLEMENTED ();
        }
        case ECMA_SIMPLE_VALUE_EMPTY:
        case ECMA_SIMPLE_VALUE_ARRAY_REDIRECT:
        case ECMA_SIMPLE_VALUE__COUNT:
        {
          JERRY_UNREACHABLE ();
        }
      }

      JERRY_UNREACHABLE ();
    }
    case ECMA_TYPE_NUMBER:
    {
      /* return Number object with [[PrimitiveValue]] set to the value */
      JERRY_UNIMPLEMENTED ();
    }
    case ECMA_TYPE_STRING:
    {
      /* return String object with [[PrimitiveValue]] set to the value */
      JERRY_UNIMPLEMENTED ();
    }
    case ECMA_TYPE_OBJECT:
    {
      return ecma_make_normal_completion_value (ecma_copy_value (value, true));
    }
  }

  JERRY_UNREACHABLE ();
} /* ecma_op_to_object */

/**
 * @}
 * @}
 */
