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
#include "ecma-boolean-object.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-number-object.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"
#include "ecma-string-object.h"
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

    return ecma_compare_ecma_strings (x_str_p, y_str_p);
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
      ecma_completion_value_t ret_value;

      ECMA_TRY_CATCH (completion_to_primitive,
                      ecma_op_to_primitive (value, ECMA_PREFERRED_TYPE_NUMBER),
                      ret_value);

      ret_value = ecma_op_to_number (completion_to_primitive.u.value);

      ECMA_FINALIZE (completion_to_primitive);

      return ret_value;
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
      res_p = ecma_copy_or_ref_ecma_string (res_p);

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
          return ecma_op_create_boolean_object (value);
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
      return ecma_op_create_number_object (value);
    }
    case ECMA_TYPE_STRING:
    {
      return ecma_op_create_string_object (&value, 1);
    }
    case ECMA_TYPE_OBJECT:
    {
      return ecma_make_normal_completion_value (ecma_copy_value (value, true));
    }
  }

  JERRY_UNREACHABLE ();
} /* ecma_op_to_object */

/**
 * FromPropertyDescriptor operation.
 *
 * See also:
 *          ECMA-262 v5, 8.10.4
 *
 * @return constructed object
 */
ecma_object_t*
ecma_op_from_property_descriptor (const ecma_property_descriptor_t src_prop_desc) /**< property descriptor */
{
  // 2.
  ecma_object_t *obj_p = ecma_op_create_object_object_noarg ();

  ecma_completion_value_t completion;
  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();
  {
    prop_desc.is_value_defined = true;

    prop_desc.is_writable_defined = true;
    prop_desc.writable = ECMA_PROPERTY_WRITABLE;

    prop_desc.is_enumerable_defined = true;
    prop_desc.enumerable = ECMA_PROPERTY_ENUMERABLE;

    prop_desc.is_configurable_defined = true;
    prop_desc.configurable = ECMA_PROPERTY_CONFIGURABLE;
  }

  // 3.
  if (prop_desc.is_value_defined
      || prop_desc.is_writable_defined)
  {
    JERRY_ASSERT (prop_desc.is_value_defined && prop_desc.is_writable_defined);

    // a.
    prop_desc.value = src_prop_desc.value;

    ecma_string_t *value_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_VALUE);
    completion = ecma_op_object_define_own_property (obj_p,
                                                     value_magic_string_p,
                                                     prop_desc,
                                                     false);
    ecma_deref_ecma_string (value_magic_string_p);
    JERRY_ASSERT (ecma_is_completion_value_normal_true (completion));

    // b.
    const bool is_writable = (src_prop_desc.writable == ECMA_PROPERTY_WRITABLE);
    prop_desc.value = ecma_make_simple_value (is_writable ? ECMA_SIMPLE_VALUE_TRUE
                                                          : ECMA_SIMPLE_VALUE_FALSE);

    ecma_string_t *writable_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_WRITABLE);
    completion = ecma_op_object_define_own_property (obj_p,
                                                     writable_magic_string_p,
                                                     prop_desc,
                                                     false);
    ecma_deref_ecma_string (writable_magic_string_p);
    JERRY_ASSERT (ecma_is_completion_value_normal_true (completion));
  }
  else
  {
    // 4.
    JERRY_ASSERT (prop_desc.is_get_defined && prop_desc.is_set_defined);

    // a.
    if (src_prop_desc.get_p == NULL)
    {
      prop_desc.value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
    else
    {
      prop_desc.value = ecma_make_object_value (src_prop_desc.get_p);
    }

    ecma_string_t *get_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_GET);
    completion = ecma_op_object_define_own_property (obj_p,
                                                     get_magic_string_p,
                                                     prop_desc,
                                                     false);
    ecma_deref_ecma_string (get_magic_string_p);
    JERRY_ASSERT (ecma_is_completion_value_normal_true (completion));

    // b.
    if (src_prop_desc.set_p == NULL)
    {
      prop_desc.value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
    else
    {
      prop_desc.value = ecma_make_object_value (src_prop_desc.set_p);
    }

    ecma_string_t *set_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_SET);
    completion = ecma_op_object_define_own_property (obj_p,
                                                     set_magic_string_p,
                                                     prop_desc,
                                                     false);
    ecma_deref_ecma_string (set_magic_string_p);
    JERRY_ASSERT (ecma_is_completion_value_normal_true (completion));
  }

  const bool is_enumerable = (src_prop_desc.enumerable == ECMA_PROPERTY_ENUMERABLE);
  prop_desc.value = ecma_make_simple_value (is_enumerable ? ECMA_SIMPLE_VALUE_TRUE
                                            : ECMA_SIMPLE_VALUE_FALSE);

  ecma_string_t *enumerable_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_ENUMERABLE);
  completion = ecma_op_object_define_own_property (obj_p,
                                                   enumerable_magic_string_p,
                                                   prop_desc,
                                                   false);
  ecma_deref_ecma_string (enumerable_magic_string_p);
  JERRY_ASSERT (ecma_is_completion_value_normal_true (completion));

  const bool is_configurable = (src_prop_desc.configurable == ECMA_PROPERTY_CONFIGURABLE);
  prop_desc.value = ecma_make_simple_value (is_configurable ? ECMA_SIMPLE_VALUE_TRUE
                                            : ECMA_SIMPLE_VALUE_FALSE);

  ecma_string_t *configurable_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_CONFIGURABLE);
  completion = ecma_op_object_define_own_property (obj_p,
                                                   configurable_magic_string_p,
                                                   prop_desc,
                                                   false);
  ecma_deref_ecma_string (configurable_magic_string_p);
  JERRY_ASSERT (ecma_is_completion_value_normal_true (completion));

  return obj_p;
} /* ecma_op_from_property_descriptor */

/**
 * ToPropertyDescriptor operation.
 *
 * See also:
 *          ECMA-262 v5, 8.10.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_to_property_descriptor (ecma_value_t obj_value, /**< object value */
                                ecma_property_descriptor_t *out_prop_desc_p) /**< out: filled property descriptor
                                                                                  if return value is normal
                                                                                  empty completion value */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  // 1.
  if (obj_value.value_type != ECMA_TYPE_OBJECT)
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    ecma_object_t *obj_p = ECMA_GET_POINTER (obj_value.value);

    // 2.
    ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();

    // 3.
    ecma_string_t *enumerable_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_ENUMERABLE);

    if (ecma_op_object_has_property (obj_p, enumerable_magic_string_p))
    {
      ECMA_TRY_CATCH (enumerable_prop_value,
                      ecma_op_object_get (obj_p, enumerable_magic_string_p),
                      ret_value);
      ECMA_TRY_CATCH (boolean_enumerable_prop_value,
                      ecma_op_to_boolean (enumerable_prop_value.u.value),
                      ret_value);

      prop_desc.is_enumerable_defined = true;
      if (ecma_is_completion_value_normal_true (boolean_enumerable_prop_value))
      {
        prop_desc.enumerable = ECMA_PROPERTY_ENUMERABLE;
      }
      else
      {
        JERRY_ASSERT (ecma_is_completion_value_normal_false (boolean_enumerable_prop_value));

        prop_desc.enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;
      }

      ECMA_FINALIZE (boolean_enumerable_prop_value);
      ECMA_FINALIZE (enumerable_prop_value);
    }

    ecma_deref_ecma_string (enumerable_magic_string_p);

    if (!ecma_is_completion_value_throw (ret_value))
    {
      JERRY_ASSERT (ecma_is_completion_value_empty (ret_value));

      // 4.
      ecma_string_t *configurable_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_CONFIGURABLE);

      if (ecma_op_object_has_property (obj_p, configurable_magic_string_p))
      {
        ECMA_TRY_CATCH (configurable_prop_value,
                        ecma_op_object_get (obj_p, configurable_magic_string_p),
                        ret_value);
        ECMA_TRY_CATCH (boolean_configurable_prop_value,
                        ecma_op_to_boolean (configurable_prop_value.u.value),
                        ret_value);

        prop_desc.is_configurable_defined = true;
        if (ecma_is_completion_value_normal_true (boolean_configurable_prop_value))
        {
          prop_desc.configurable = ECMA_PROPERTY_CONFIGURABLE;
        }
        else
        {
          JERRY_ASSERT (ecma_is_completion_value_normal_false (boolean_configurable_prop_value));

          prop_desc.configurable = ECMA_PROPERTY_NOT_CONFIGURABLE;
        }

        ECMA_FINALIZE (boolean_configurable_prop_value);
        ECMA_FINALIZE (configurable_prop_value);
      }

      ecma_deref_ecma_string (configurable_magic_string_p);
    }

    if (!ecma_is_completion_value_throw (ret_value))
    {
      JERRY_ASSERT (ecma_is_completion_value_empty (ret_value));

      // 5.
      ecma_string_t *value_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_VALUE);

      if (ecma_op_object_has_property (obj_p, value_magic_string_p))
      {
        ECMA_TRY_CATCH (value_prop_value,
                        ecma_op_object_get (obj_p, value_magic_string_p),
                        ret_value);

        prop_desc.is_value_defined = true;
        prop_desc.value = ecma_copy_value (value_prop_value.u.value, true);

        ECMA_FINALIZE (value_prop_value);
      }

      ecma_deref_ecma_string (value_magic_string_p);
    }

    if (!ecma_is_completion_value_throw (ret_value))
    {
      JERRY_ASSERT (ecma_is_completion_value_empty (ret_value));

      // 6.
      ecma_string_t *writable_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_WRITABLE);

      if (ecma_op_object_has_property (obj_p, writable_magic_string_p))
      {
        ECMA_TRY_CATCH (writable_prop_value,
                        ecma_op_object_get (obj_p, writable_magic_string_p),
                        ret_value);
        ECMA_TRY_CATCH (boolean_writable_prop_value,
                        ecma_op_to_boolean (writable_prop_value.u.value),
                        ret_value);

        prop_desc.is_writable_defined = true;
        if (ecma_is_completion_value_normal_true (boolean_writable_prop_value))
        {
          prop_desc.writable = ECMA_PROPERTY_WRITABLE;
        }
        else
        {
          JERRY_ASSERT (ecma_is_completion_value_normal_false (boolean_writable_prop_value));

          prop_desc.writable = ECMA_PROPERTY_NOT_WRITABLE;
        }

        ECMA_FINALIZE (boolean_writable_prop_value);
        ECMA_FINALIZE (writable_prop_value);
      }

      ecma_deref_ecma_string (writable_magic_string_p);
    }

    if (!ecma_is_completion_value_throw (ret_value))
    {
      JERRY_ASSERT (ecma_is_completion_value_empty (ret_value));

      // 7.
      ecma_string_t *get_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_GET);

      if (ecma_op_object_has_property (obj_p, get_magic_string_p))
      {
        ECMA_TRY_CATCH (get_prop_value,
                        ecma_op_object_get (obj_p, get_magic_string_p),
                        ret_value);

        if (!ecma_op_is_callable (get_prop_value.u.value)
            && !ecma_is_value_undefined (get_prop_value.u.value))
        {
          ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
        }
        else
        {
          prop_desc.is_get_defined = true;

          if (ecma_is_value_undefined (get_prop_value.u.value))
          {
            prop_desc.get_p = NULL;
          }
          else
          {
            JERRY_ASSERT (get_prop_value.u.value.value_type == ECMA_TYPE_OBJECT);

            ecma_object_t *get_p = ECMA_GET_POINTER (get_prop_value.u.value.value);
            ecma_ref_object (get_p);

            prop_desc.get_p = get_p;
          }
        }

        ECMA_FINALIZE (get_prop_value);
      }

      ecma_deref_ecma_string (get_magic_string_p);
    }

    if (!ecma_is_completion_value_throw (ret_value))
    {
      JERRY_ASSERT (ecma_is_completion_value_empty (ret_value));

      // 8.

      ecma_string_t *set_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_SET);

      if (ecma_op_object_has_property (obj_p, set_magic_string_p))
      {
        ECMA_TRY_CATCH (set_prop_value,
                        ecma_op_object_get (obj_p, set_magic_string_p),
                        ret_value);

        if (!ecma_op_is_callable (set_prop_value.u.value)
            && !ecma_is_value_undefined (set_prop_value.u.value))
        {
          ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
        }
        else
        {
          prop_desc.is_set_defined = true;

          if (ecma_is_value_undefined (set_prop_value.u.value))
          {
            prop_desc.set_p = NULL;
          }
          else
          {
            JERRY_ASSERT (set_prop_value.u.value.value_type == ECMA_TYPE_OBJECT);

            ecma_object_t *set_p = ECMA_GET_POINTER (set_prop_value.u.value.value);
            ecma_ref_object (set_p);

            prop_desc.set_p = set_p;
          }
        }

        ECMA_FINALIZE (set_prop_value);
      }

      ecma_deref_ecma_string (set_magic_string_p);
    }

    if (!ecma_is_completion_value_throw (ret_value))
    {
      JERRY_ASSERT (ecma_is_completion_value_empty (ret_value));

      // 9.
      if (prop_desc.is_get_defined
          || prop_desc.is_set_defined)
      {
        if (prop_desc.is_value_defined
            || prop_desc.is_writable_defined)
        {
          ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
        }
      }
    }

    if (!ecma_is_completion_value_throw (ret_value))
    {
      JERRY_ASSERT (ecma_is_completion_value_empty (ret_value));
    }
    else
    {
      ecma_free_property_descriptor (&prop_desc);
    }

    *out_prop_desc_p = prop_desc;
  }

  return ret_value;
} /* ecma_op_to_property_descriptor */

/**
 * @}
 * @}
 */
