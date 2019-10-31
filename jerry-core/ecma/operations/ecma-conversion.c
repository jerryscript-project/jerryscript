/* Copyright JS Foundation and other contributors, http://js.foundation
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

#include <math.h>

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
#include "ecma-symbol-object.h"
#include "ecma-try-catch-macro.h"
#include "jrt-libc-includes.h"

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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_check_object_coercible (ecma_value_t value) /**< ecma value */
{
  ecma_check_value_type_is_spec_defined (value);

  if (ecma_is_value_undefined (value)
      || ecma_is_value_null (value))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument cannot be converted to an object."));
  }
  else
  {
    return ECMA_VALUE_EMPTY;
  }
} /* ecma_op_check_object_coercible */

/**
 * SameValue operation.
 *
 * See also:
 *          ECMA-262 v5, 9.12
 *
 * @return true - if the value are same according to ECMA-defined SameValue algorithm,
 *         false - otherwise
 */
bool
ecma_op_same_value (ecma_value_t x, /**< ecma value */
                    ecma_value_t y) /**< ecma value */
{
  const bool is_x_undefined = ecma_is_value_undefined (x);
  const bool is_x_null = ecma_is_value_null (x);
  const bool is_x_boolean = ecma_is_value_boolean (x);
  const bool is_x_number = ecma_is_value_number (x);
  const bool is_x_string = ecma_is_value_string (x);
  const bool is_x_object = ecma_is_value_object (x);

  const bool is_y_undefined = ecma_is_value_undefined (y);
  const bool is_y_null = ecma_is_value_null (y);
  const bool is_y_boolean = ecma_is_value_boolean (y);
  const bool is_y_number = ecma_is_value_number (y);
  const bool is_y_string = ecma_is_value_string (y);
  const bool is_y_object = ecma_is_value_object (y);

#if ENABLED (JERRY_ES2015)
  const bool is_x_symbol = ecma_is_value_symbol (x);
  const bool is_y_symbol = ecma_is_value_symbol (y);
#endif /* ENABLED (JERRY_ES2015) */

  const bool is_types_equal = ((is_x_undefined && is_y_undefined)
                               || (is_x_null && is_y_null)
                               || (is_x_boolean && is_y_boolean)
                               || (is_x_number && is_y_number)
                               || (is_x_string && is_y_string)
#if ENABLED (JERRY_ES2015)
                               || (is_x_symbol && is_y_symbol)
#endif /* ENABLED (JERRY_ES2015) */
                               || (is_x_object && is_y_object));

  if (!is_types_equal)
  {
    return false;
  }
  else if (is_x_undefined || is_x_null)
  {
    return true;
  }
  else if (is_x_number)
  {
    ecma_number_t x_num = ecma_get_number_from_value (x);
    ecma_number_t y_num = ecma_get_number_from_value (y);

    bool is_x_nan = ecma_number_is_nan (x_num);
    bool is_y_nan = ecma_number_is_nan (y_num);

    if (is_x_nan || is_y_nan)
    {
      /*
       * If both are NaN
       *   return true;
       * else
       *   one of the numbers is NaN, and another - is not
       *   return false;
       */
      return (is_x_nan && is_y_nan);
    }
    else if (ecma_number_is_zero (x_num)
             && ecma_number_is_zero (y_num)
             && ecma_number_is_negative (x_num) != ecma_number_is_negative (y_num))
    {
      return false;
    }
    else
    {
      return (x_num == y_num);
    }
  }
  else if (is_x_string)
  {
    ecma_string_t *x_str_p = ecma_get_string_from_value (x);
    ecma_string_t *y_str_p = ecma_get_string_from_value (y);

    return ecma_compare_ecma_strings (x_str_p, y_str_p);
  }
  else if (is_x_boolean)
  {
    return (ecma_is_value_true (x) == ecma_is_value_true (y));
  }
#if ENABLED (JERRY_ES2015)
  else if (is_x_symbol)
  {
    return x == y;
  }
#endif /* ENABLED (JERRY_ES2015) */
  else
  {
    JERRY_ASSERT (is_x_object);

    return (ecma_get_object_from_value (x) == ecma_get_object_from_value (y));
  }
} /* ecma_op_same_value */

#if ENABLED (JERRY_ES2015_BUILTIN_MAP)
/**
 * SameValueZero operation.
 *
 * See also:
 *          ECMA-262 v6, 7.2.10
 *
 * @return true - if the value are same according to ECMA-defined SameValueZero algorithm,
 *         false - otherwise
 */
bool
ecma_op_same_value_zero (ecma_value_t x, /**< ecma value */
                         ecma_value_t y) /**< ecma value */
{
  if (ecma_is_value_number (x) && ecma_is_value_number (y))
  {
    ecma_number_t x_num = ecma_get_number_from_value (x);
    ecma_number_t y_num = ecma_get_number_from_value (y);

    bool is_x_nan = ecma_number_is_nan (x_num);
    bool is_y_nan = ecma_number_is_nan (y_num);

    if (is_x_nan || is_y_nan)
    {
      /*
       * If both are NaN
       *   return true;
       * else
       *   one of the numbers is NaN, and another - is not
       *   return false;
       */
      return (is_x_nan && is_y_nan);
    }

    if (ecma_number_is_zero (x_num)
        && ecma_number_is_zero (y_num)
        && ecma_number_is_negative (x_num) != ecma_number_is_negative (y_num))
    {
      return true;
    }

    return (x_num == y_num);
  }

  return ecma_op_same_value (x, y);
} /* ecma_op_same_value_zero */
#endif /* ENABLED (JERRY_ES2015_BUILTIN_MAP) */

/**
 * ToPrimitive operation.
 *
 * See also:
 *          ECMA-262 v5, 9.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_to_primitive (ecma_value_t value, /**< ecma value */
                      ecma_preferred_type_hint_t preferred_type) /**< preferred type hint */
{
  ecma_check_value_type_is_spec_defined (value);

  if (ecma_is_value_object (value))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (value);

    return ecma_op_object_default_value (obj_p, preferred_type);
  }
  else
  {
    return ecma_copy_value (value);
  }
} /* ecma_op_to_primitive */

/**
 * ToBoolean operation. Cannot throw an exception.
 *
 * See also:
 *          ECMA-262 v5, 9.2
 *
 * @return true - if the logical value is true
 *         false - otherwise
 */
bool
ecma_op_to_boolean (ecma_value_t value) /**< ecma value */
{
  ecma_check_value_type_is_spec_defined (value);

  if (ecma_is_value_simple (value))
  {
    JERRY_ASSERT (ecma_is_value_boolean (value)
                  || ecma_is_value_undefined (value)
                  || ecma_is_value_null (value));

    return ecma_is_value_true (value);
  }

  if (ecma_is_value_integer_number (value))
  {
    return (value != ecma_make_integer_value (0));
  }

  if (ecma_is_value_float_number (value))
  {
    ecma_number_t num = ecma_get_float_from_value (value);

    return (!ecma_number_is_nan (num) && !ecma_number_is_zero (num));
  }

  if (ecma_is_value_string (value))
  {
    ecma_string_t *str_p = ecma_get_string_from_value (value);

    return !ecma_string_is_empty (str_p);
  }

  JERRY_ASSERT (ecma_is_value_object (value) || ECMA_ASSERT_VALUE_IS_SYMBOL (value));

  return true;
} /* ecma_op_to_boolean */

/**
 * ToNumber operation.
 *
 * See also:
 *          ECMA-262 v5, 9.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_to_number (ecma_value_t value) /**< ecma value */
{
  ecma_check_value_type_is_spec_defined (value);

  if (ecma_is_value_integer_number (value))
  {
    return value;
  }

  if (ecma_is_value_float_number (value))
  {
    return ecma_copy_value (value);
  }

  if (ecma_is_value_string (value))
  {
    ecma_string_t *str_p = ecma_get_string_from_value (value);
    return ecma_make_number_value (ecma_string_to_number (str_p));
  }
#if ENABLED (JERRY_ES2015)
  if (ecma_is_value_symbol (value))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Cannot convert a Symbol value to a number."));
  }
#endif /* ENABLED (JERRY_ES2015) */

  if (ecma_is_value_object (value))
  {
    ecma_value_t primitive_value = ecma_op_to_primitive (value, ECMA_PREFERRED_TYPE_NUMBER);

    if (ECMA_IS_VALUE_ERROR (primitive_value))
    {
      return primitive_value;
    }

    ecma_value_t ret_value = ecma_op_to_number (primitive_value);
    ecma_fast_free_value (primitive_value);
    return ret_value;
  }

  if (ecma_is_value_undefined (value))
  {
    return ecma_make_nan_value ();
  }

  ecma_integer_value_t num = 0;

  if (ecma_is_value_null (value))
  {
    num = 0;
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (value));

    num = ecma_is_value_true (value) ? 1 : 0;
  }

  return ecma_make_integer_value (num);
} /* ecma_op_to_number */

/**
 * Helper to get the number contained in an ecma value.
 *
 * See also:
 *          ECMA-262 v5, 9.3
 *
 * @return ECMA_VALUE_EMPTY if successful
 *         conversion error otherwise
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_get_number (ecma_value_t value, /**< ecma value*/
                 ecma_number_t *number_p) /**< [out] ecma number */
{
  if (ecma_is_value_integer_number (value))
  {
    *number_p = ecma_get_integer_from_value (value);
    return ECMA_VALUE_EMPTY;
  }

  if (ecma_is_value_float_number (value))
  {
    *number_p = ecma_get_float_from_value (value);
    return ECMA_VALUE_EMPTY;
  }

  if (ecma_is_value_string (value))
  {
    ecma_string_t *str_p = ecma_get_string_from_value (value);
    *number_p = ecma_string_to_number (str_p);
    return ECMA_VALUE_EMPTY;
  }

  if (ecma_is_value_object (value))
  {
    ecma_value_t primitive_value = ecma_op_to_primitive (value, ECMA_PREFERRED_TYPE_NUMBER);

    if (ECMA_IS_VALUE_ERROR (primitive_value))
    {
      return primitive_value;
    }

    ecma_value_t ret_value = ecma_get_number (primitive_value, number_p);
    ecma_fast_free_value (primitive_value);
    return ret_value;
  }

  if (ecma_is_value_undefined (value))
  {
    *number_p = ecma_number_make_nan ();
    return ECMA_VALUE_EMPTY;
  }

  if (ecma_is_value_null (value))
  {
    *number_p = 0;
    return ECMA_VALUE_EMPTY;
  }

#if ENABLED (JERRY_ES2015)
  if (ecma_is_value_symbol (value))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Cannot convert a Symbol value to a number."));
  }
#endif /* ENABLED (JERRY_ES2015) */

  JERRY_ASSERT (ecma_is_value_boolean (value));

  *number_p = ecma_is_value_true (value) ? 1 : 0;
  return ECMA_VALUE_EMPTY;
} /* ecma_get_number */

/**
 * ToString operation.
 *
 * See also:
 *          ECMA-262 v5, 9.8
 *
 * @return NULL - if the conversion fails
 *         pointer to the string descriptor - otherwise
 */
ecma_string_t *
ecma_op_to_string (ecma_value_t value) /**< ecma value */
{
  ecma_check_value_type_is_spec_defined (value);

  if (JERRY_UNLIKELY (ecma_is_value_object (value)))
  {
    ecma_value_t prim_value = ecma_op_to_primitive (value, ECMA_PREFERRED_TYPE_STRING);

    if (ECMA_IS_VALUE_ERROR (prim_value))
    {
      return NULL;
    }

    ecma_string_t *ret_string_p = ecma_op_to_string (prim_value);

    ecma_free_value (prim_value);

    return ret_string_p;
  }

  if (ecma_is_value_string (value))
  {
    ecma_string_t *res_p = ecma_get_string_from_value (value);
    ecma_ref_ecma_string (res_p);
    return res_p;
  }
  else if (ecma_is_value_integer_number (value))
  {
    ecma_integer_value_t num = ecma_get_integer_from_value (value);

    if (num < 0)
    {
      return ecma_new_ecma_string_from_number ((ecma_number_t) num);
    }
    else
    {
      return ecma_new_ecma_string_from_uint32 ((uint32_t) num);
    }
  }
  else if (ecma_is_value_float_number (value))
  {
    ecma_number_t num = ecma_get_float_from_value (value);
    return ecma_new_ecma_string_from_number (num);
  }
  else if (ecma_is_value_undefined (value))
  {
    return ecma_get_magic_string (LIT_MAGIC_STRING_UNDEFINED);
  }
  else if (ecma_is_value_null (value))
  {
    return ecma_get_magic_string (LIT_MAGIC_STRING_NULL);
  }
#if ENABLED (JERRY_ES2015)
  else if (ecma_is_value_symbol (value))
  {
    ecma_raise_type_error (ECMA_ERR_MSG ("Cannot convert a Symbol value to a string."));
    return NULL;
  }
#endif /* ENABLED (JERRY_ES2015) */
  JERRY_ASSERT (ecma_is_value_boolean (value));

  if (ecma_is_value_true (value))
  {
    return ecma_get_magic_string (LIT_MAGIC_STRING_TRUE);
  }

  return ecma_get_magic_string (LIT_MAGIC_STRING_FALSE);
} /* ecma_op_to_string */

/**
 * ToPropertyName operation.
 *
 * @return NULL - if the conversion fails
 *         ecma-string - otherwise
 */
ecma_string_t *
ecma_op_to_prop_name (ecma_value_t value) /**< ecma value */
{
  ecma_check_value_type_is_spec_defined (value);

#if ENABLED (JERRY_ES2015)
  if (ecma_is_value_symbol (value))
  {
    ecma_string_t *symbol_p = ecma_get_symbol_from_value (value);
    ecma_ref_ecma_string (symbol_p);
    return symbol_p;
  }
#endif /* ENABLED (JERRY_ES2015) */

  return ecma_op_to_string (value);
} /* ecma_op_to_prop_name */

/**
 * ToObject operation.
 *
 * See also:
 *          ECMA-262 v5, 9.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_to_object (ecma_value_t value) /**< ecma value */
{
  ecma_check_value_type_is_spec_defined (value);

  if (ecma_is_value_number (value))
  {
    return ecma_op_create_number_object (value);
  }
  else if (ecma_is_value_string (value))
  {
    return ecma_op_create_string_object (&value, 1);
  }
  else if (ecma_is_value_object (value))
  {
    return ecma_copy_value (value);
  }
#if ENABLED (JERRY_ES2015)
  else if (ecma_is_value_symbol (value))
  {
    return ecma_op_create_symbol_object (value);
  }
#endif /* ENABLED (JERRY_ES2015) */
  else
  {
    if (ecma_is_value_undefined (value)
        || ecma_is_value_null (value))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("Argument cannot be converted to an object."));
    }
    else
    {
      JERRY_ASSERT (ecma_is_value_boolean (value));

      return ecma_op_create_boolean_object (value);
    }
  }
} /* ecma_op_to_object */

/**
 * FromPropertyDescriptor operation.
 *
 * See also:
 *          ECMA-262 v5, 8.10.4
 *
 * @return constructed object
 */
ecma_object_t *
ecma_op_from_property_descriptor (const ecma_property_descriptor_t *src_prop_desc_p) /**< property descriptor */
{
  /* 2. */
  ecma_object_t *obj_p = ecma_op_create_object_object_noarg ();

  ecma_value_t completion;
  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();
  {
    prop_desc.flags = (ECMA_PROP_IS_VALUE_DEFINED
                       | ECMA_PROP_IS_WRITABLE_DEFINED
                       | ECMA_PROP_IS_WRITABLE
                       | ECMA_PROP_IS_ENUMERABLE_DEFINED
                       | ECMA_PROP_IS_ENUMERABLE
                       | ECMA_PROP_IS_CONFIGURABLE_DEFINED
                       | ECMA_PROP_IS_CONFIGURABLE);
  }

  /* 3. */
  if (src_prop_desc_p->flags & (ECMA_PROP_IS_VALUE_DEFINED | ECMA_PROP_IS_WRITABLE_DEFINED))
  {
    JERRY_ASSERT ((prop_desc.flags & (ECMA_PROP_IS_VALUE_DEFINED | ECMA_PROP_IS_WRITABLE_DEFINED))
                   == (ECMA_PROP_IS_VALUE_DEFINED | ECMA_PROP_IS_WRITABLE_DEFINED));

    /* a. */
    prop_desc.value = src_prop_desc_p->value;

    completion = ecma_op_object_define_own_property (obj_p,
                                                     ecma_get_magic_string (LIT_MAGIC_STRING_VALUE),
                                                     &prop_desc);
    JERRY_ASSERT (ecma_is_value_true (completion));

    /* b. */
    prop_desc.value = ecma_make_boolean_value (src_prop_desc_p->flags & ECMA_PROP_IS_WRITABLE);

    completion = ecma_op_object_define_own_property (obj_p,
                                                     ecma_get_magic_string (LIT_MAGIC_STRING_WRITABLE),
                                                     &prop_desc);
    JERRY_ASSERT (ecma_is_value_true (completion));
  }
  else
  {
    /* 4. */
    JERRY_ASSERT (src_prop_desc_p->flags & (ECMA_PROP_IS_GET_DEFINED | ECMA_PROP_IS_SET_DEFINED));

    /* a. */
    if (src_prop_desc_p->get_p == NULL)
    {
      prop_desc.value = ECMA_VALUE_UNDEFINED;
    }
    else
    {
      prop_desc.value = ecma_make_object_value (src_prop_desc_p->get_p);
    }

    completion = ecma_op_object_define_own_property (obj_p,
                                                     ecma_get_magic_string (LIT_MAGIC_STRING_GET),
                                                     &prop_desc);
    JERRY_ASSERT (ecma_is_value_true (completion));

    /* b. */
    if (src_prop_desc_p->set_p == NULL)
    {
      prop_desc.value = ECMA_VALUE_UNDEFINED;
    }
    else
    {
      prop_desc.value = ecma_make_object_value (src_prop_desc_p->set_p);
    }

    completion = ecma_op_object_define_own_property (obj_p,
                                                     ecma_get_magic_string (LIT_MAGIC_STRING_SET),
                                                     &prop_desc);
    JERRY_ASSERT (ecma_is_value_true (completion));
  }

  prop_desc.value = ecma_make_boolean_value (src_prop_desc_p->flags & ECMA_PROP_IS_ENUMERABLE);

  completion = ecma_op_object_define_own_property (obj_p,
                                                   ecma_get_magic_string (LIT_MAGIC_STRING_ENUMERABLE),
                                                   &prop_desc);
  JERRY_ASSERT (ecma_is_value_true (completion));

  prop_desc.value = ecma_make_boolean_value (src_prop_desc_p->flags & ECMA_PROP_IS_CONFIGURABLE);

  completion = ecma_op_object_define_own_property (obj_p,
                                                   ecma_get_magic_string (LIT_MAGIC_STRING_CONFIGURABLE),
                                                   &prop_desc);
  JERRY_ASSERT (ecma_is_value_true (completion));

  return obj_p;
} /* ecma_op_from_property_descriptor */

/**
 * ToPropertyDescriptor operation.
 *
 * See also:
 *          ECMA-262 v5, 8.10.5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_to_property_descriptor (ecma_value_t obj_value, /**< object value */
                                ecma_property_descriptor_t *out_prop_desc_p) /**< [out] filled property descriptor
                                                                                  if return value is normal
                                                                                  empty completion value */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  /* 1. */
  if (!ecma_is_value_object (obj_value))
  {
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Expected an object."));
  }
  else
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (obj_value);

    /* 2. */
    ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();

    /* 3. */
    ECMA_TRY_CATCH (enumerable_prop_value,
                    ecma_op_object_find (obj_p, ecma_get_magic_string (LIT_MAGIC_STRING_ENUMERABLE)),
                    ret_value);

    if (ecma_is_value_found (enumerable_prop_value))
    {
      uint32_t is_enumerable = (ecma_op_to_boolean (enumerable_prop_value) ? ECMA_PROP_IS_ENUMERABLE
                                                                           : ECMA_PROP_NO_OPTS);

      prop_desc.flags |= (uint16_t) (ECMA_PROP_IS_ENUMERABLE_DEFINED | is_enumerable);
    }

    ECMA_FINALIZE (enumerable_prop_value);

    if (!ECMA_IS_VALUE_ERROR (ret_value))
    {
      JERRY_ASSERT (ecma_is_value_empty (ret_value));

      /* 4. */
      ECMA_TRY_CATCH (configurable_prop_value,
                      ecma_op_object_find (obj_p, ecma_get_magic_string (LIT_MAGIC_STRING_CONFIGURABLE)),
                      ret_value);

      if (ecma_is_value_found (configurable_prop_value))
      {
        uint32_t is_configurable = (ecma_op_to_boolean (configurable_prop_value) ? ECMA_PROP_IS_CONFIGURABLE
                                                                                 : ECMA_PROP_NO_OPTS);

        prop_desc.flags |= (uint16_t) (ECMA_PROP_IS_CONFIGURABLE_DEFINED | is_configurable);
      }

      ECMA_FINALIZE (configurable_prop_value);
    }

    if (!ECMA_IS_VALUE_ERROR (ret_value))
    {
      JERRY_ASSERT (ecma_is_value_empty (ret_value));

      /* 5. */
      ECMA_TRY_CATCH (value_prop_value,
                      ecma_op_object_find (obj_p, ecma_get_magic_string (LIT_MAGIC_STRING_VALUE)),
                      ret_value);

      if (ecma_is_value_found (value_prop_value))
      {
        prop_desc.flags |= ECMA_PROP_IS_VALUE_DEFINED;
        prop_desc.value = ecma_copy_value (value_prop_value);
      }

      ECMA_FINALIZE (value_prop_value);
    }

    if (!ECMA_IS_VALUE_ERROR (ret_value))
    {
      JERRY_ASSERT (ecma_is_value_empty (ret_value));

      /* 6. */
      ECMA_TRY_CATCH (writable_prop_value,
                      ecma_op_object_find (obj_p, ecma_get_magic_string (LIT_MAGIC_STRING_WRITABLE)),
                      ret_value);

      if (ecma_is_value_found (writable_prop_value))
      {
        uint32_t is_writable = (ecma_op_to_boolean (writable_prop_value) ? ECMA_PROP_IS_WRITABLE
                                                                         : ECMA_PROP_NO_OPTS);

        prop_desc.flags |= (uint16_t) (ECMA_PROP_IS_WRITABLE_DEFINED | is_writable);
      }

      ECMA_FINALIZE (writable_prop_value);
    }

    if (!ECMA_IS_VALUE_ERROR (ret_value))
    {
      JERRY_ASSERT (ecma_is_value_empty (ret_value));

      /* 7. */
      ECMA_TRY_CATCH (get_prop_value,
                      ecma_op_object_find (obj_p, ecma_get_magic_string (LIT_MAGIC_STRING_GET)),
                      ret_value);

      if (ecma_is_value_found (get_prop_value))
      {
        if (!ecma_op_is_callable (get_prop_value)
            && !ecma_is_value_undefined (get_prop_value))
        {
          ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Expected a function."));
        }
        else
        {
          prop_desc.flags |= ECMA_PROP_IS_GET_DEFINED;

          if (ecma_is_value_undefined (get_prop_value))
          {
            prop_desc.get_p = NULL;
          }
          else
          {
            JERRY_ASSERT (ecma_is_value_object (get_prop_value));

            ecma_object_t *get_p = ecma_get_object_from_value (get_prop_value);
            ecma_ref_object (get_p);

            prop_desc.get_p = get_p;
          }
        }
      }

      ECMA_FINALIZE (get_prop_value);
    }

    if (!ECMA_IS_VALUE_ERROR (ret_value))
    {
      JERRY_ASSERT (ecma_is_value_empty (ret_value));

      /* 8. */
      ECMA_TRY_CATCH (set_prop_value,
                      ecma_op_object_find (obj_p, ecma_get_magic_string (LIT_MAGIC_STRING_SET)),
                      ret_value);

      if (ecma_is_value_found (set_prop_value))
      {
        if (!ecma_op_is_callable (set_prop_value)
            && !ecma_is_value_undefined (set_prop_value))
        {
          ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Expected a function."));
        }
        else
        {
          prop_desc.flags |= ECMA_PROP_IS_SET_DEFINED;

          if (ecma_is_value_undefined (set_prop_value))
          {
            prop_desc.set_p = NULL;
          }
          else
          {
            JERRY_ASSERT (ecma_is_value_object (set_prop_value));

            ecma_object_t *set_p = ecma_get_object_from_value (set_prop_value);
            ecma_ref_object (set_p);

            prop_desc.set_p = set_p;
          }
        }
      }

      ECMA_FINALIZE (set_prop_value);
    }

    if (!ECMA_IS_VALUE_ERROR (ret_value))
    {
      JERRY_ASSERT (ecma_is_value_empty (ret_value));

      /* 9. */
      if ((prop_desc.flags & (ECMA_PROP_IS_VALUE_DEFINED | ECMA_PROP_IS_WRITABLE_DEFINED))
           && (prop_desc.flags & (ECMA_PROP_IS_GET_DEFINED | ECMA_PROP_IS_SET_DEFINED)))
      {
        ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Accessors cannot be writable."));
      }
    }

    if (!ECMA_IS_VALUE_ERROR (ret_value))
    {
      JERRY_ASSERT (ecma_is_value_empty (ret_value));
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
 * ToInteger operation.
 *
 * See also:
 *          ECMA-262 v5, 9.4
 *          ECMA-262 v6, 7.1.4
 *
 * @return ECMA_VALUE_EMPTY if successful
 *         conversion error otherwise
 */
ecma_value_t
ecma_op_to_integer (ecma_value_t value, /**< ecma value*/
                    ecma_number_t *number_p) /**< [out] ecma number */
{
  if (ECMA_IS_VALUE_ERROR (value))
  {
    return value;
  }

  /* 1 */
  ecma_value_t to_number = ecma_get_number (value, number_p);

  /* 2 */
  if (ECMA_IS_VALUE_ERROR (to_number))
  {
    return to_number;
  }

  ecma_number_t number = *number_p;

  /* 3 */
  if (ecma_number_is_nan (number))
  {
    *number_p = ECMA_NUMBER_ZERO;
    return ECMA_VALUE_EMPTY;
  }

  /* 4 */
  if (ecma_number_is_zero (number) || ecma_number_is_infinity (number))
  {
    return ECMA_VALUE_EMPTY;
  }

  ecma_number_t floor_fabs = floor (fabs (number));

  /* 5 */
  *number_p = ecma_number_is_negative (number) ? -floor_fabs : floor_fabs;
  return ECMA_VALUE_EMPTY;
} /* ecma_op_to_integer */

/**
 * ToLength operation.
 *
 * See also:
 *          ECMA-262 v6, 7.1.15
 *
 * @return ECMA_VALUE_EMPTY if successful
 *         conversion error otherwise
 */
ecma_value_t
ecma_op_to_length (ecma_value_t value, /**< ecma value*/
                   uint32_t *length) /**< [out] ecma number */
{
  /* 1 */
  if (ECMA_IS_VALUE_ERROR (value))
  {
    return value;
  }

#if ENABLED (JERRY_ES2015)
  /* 2 */
  ecma_number_t num;
  ecma_value_t length_num = ecma_op_to_integer (value, &num);

  /* 3 */
  if (ECMA_IS_VALUE_ERROR (length_num))
  {
    return length_num;
  }

  /* 4 */
  if (num <= 0.0f)
  {
    *length = 0;
    return ECMA_VALUE_EMPTY;
  }

  /* 5 */
  if (num >= (ecma_number_t) UINT32_MAX)
  {
    *length = UINT32_MAX;
    return ECMA_VALUE_EMPTY;
  }

  /* 6 */
  *length = (uint32_t) num;
  return ECMA_VALUE_EMPTY;
#else /* !ENABLED (JERRY_ES2015) */
  /* In the case of ES5, ToLength(ES6) operation is the same as ToUint32(ES5) */
  ecma_number_t num;
  ecma_value_t to_number = ecma_get_number (value, &num);

  /* 2 */
  if (ECMA_IS_VALUE_ERROR (to_number))
  {
    return to_number;
  }

  *length = ecma_number_to_uint32 (num);
  return ECMA_VALUE_EMPTY;
#endif /* ENABLED (JERRY_ES2015) */
} /* ecma_op_to_length */

/**
 * @}
 * @}
 */
