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

#include "ecma-alloc.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-try-catch-macro.h"
#include "globals.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup global ECMA Global object built-in
 * @{
 */

/**
 * List of the Global object's built-in property names
 *
 * Warning:
 *         values in the array should be sorted in ascending order
 *         that is checked in the ecma_builtin_init_global_object.
 */
static const ecma_magic_string_id_t ecma_builtin_global_property_names[] =
{
  ECMA_MAGIC_STRING_EVAL,
  ECMA_MAGIC_STRING_UNDEFINED,
  ECMA_MAGIC_STRING_NAN,
  ECMA_MAGIC_STRING_INFINITY_UL,
  ECMA_MAGIC_STRING_OBJECT_UL,
  ECMA_MAGIC_STRING_FUNCTION_UL,
  ECMA_MAGIC_STRING_ARRAY_UL,
  ECMA_MAGIC_STRING_STRING_UL,
  ECMA_MAGIC_STRING_BOOLEAN_UL,
  ECMA_MAGIC_STRING_NUMBER_UL,
  ECMA_MAGIC_STRING_DATE_UL,
  ECMA_MAGIC_STRING_REG_EXP_UL,
  ECMA_MAGIC_STRING_ERROR_UL,
  ECMA_MAGIC_STRING_EVAL_ERROR_UL,
  ECMA_MAGIC_STRING_RANGE_ERROR_UL,
  ECMA_MAGIC_STRING_REFERENCE_ERROR_UL,
  ECMA_MAGIC_STRING_SYNTAX_ERROR_UL,
  ECMA_MAGIC_STRING_TYPE_ERROR_UL,
  ECMA_MAGIC_STRING_URI_ERROR_UL,
  ECMA_MAGIC_STRING_MATH_UL,
  ECMA_MAGIC_STRING_JSON_U,
  ECMA_MAGIC_STRING_PARSE_INT,
  ECMA_MAGIC_STRING_PARSE_FLOAT,
  ECMA_MAGIC_STRING_IS_NAN,
  ECMA_MAGIC_STRING_IS_FINITE,
  ECMA_MAGIC_STRING_DECODE_URI,
  ECMA_MAGIC_STRING_DECODE_URI_COMPONENT,
  ECMA_MAGIC_STRING_ENCODE_URI,
  ECMA_MAGIC_STRING_ENCODE_URI_COMPONENT
};

/**
 * Number of the Global object's built-in properties
 */
static const ecma_length_t ecma_builtin_global_property_number = (sizeof (ecma_builtin_global_property_names) /
                                                                  sizeof (ecma_magic_string_id_t));
JERRY_STATIC_ASSERT (sizeof (ecma_builtin_global_property_names) > sizeof (void*));

/**
 * Global object
 */
static ecma_object_t* ecma_global_object_p = NULL;

/**
 * Get Global object
 *
 * @return pointer to the Global object
 *         caller should free the reference by calling ecma_deref_object
 */
ecma_object_t*
ecma_builtin_get_global_object (void)
{
  JERRY_ASSERT(ecma_global_object_p != NULL);

  ecma_ref_object (ecma_global_object_p);

  return ecma_global_object_p;
} /* ecma_builtin_get_global_object */

/**
 * Check if passed object is the Global object.
 *
 * @return true - if passed pointer points to the Global object,
 *         false - otherwise.
 */
bool
ecma_builtin_is_global_object (ecma_object_t *object_p) /**< an object */
{
  return (object_p == ecma_global_object_p);
} /* ecma_builtin_is_global_object */

/**
 * Initialize Global object.
 *
 * Warning:
 *         the routine should be called only from ecma_init_builtins
 */
void
ecma_builtin_init_global_object (void)
{
  JERRY_ASSERT(ecma_global_object_p == NULL);

  ecma_object_t *glob_obj_p = ecma_create_object (NULL, true, ECMA_OBJECT_TYPE_GENERAL);

  ecma_property_t *class_prop_p = ecma_create_internal_property (glob_obj_p,
                                                                 ECMA_INTERNAL_PROPERTY_CLASS);
  class_prop_p->u.internal_property.value = ECMA_OBJECT_CLASS_OBJECT;

  ecma_property_t *built_in_id_prop_p = ecma_create_internal_property (glob_obj_p,
                                                                       ECMA_INTERNAL_PROPERTY_BUILT_IN_ID);
  built_in_id_prop_p->u.internal_property.value = ECMA_BUILTIN_ID_GLOBAL;

  ecma_property_t *mask_0_31_prop_p;
  mask_0_31_prop_p = ecma_create_internal_property (glob_obj_p,
                                                    ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_0_31);

  JERRY_STATIC_ASSERT (ecma_builtin_global_property_number < sizeof (uint32_t) * JERRY_BITSINBYTE);
  uint32_t builtin_mask = ((uint32_t)1u << ecma_builtin_global_property_number) - 1;
  mask_0_31_prop_p->u.internal_property.value = builtin_mask;

  ecma_set_object_is_builtin (glob_obj_p, true);

  ecma_global_object_p = glob_obj_p;
} /* ecma_builtin_init_global_object */

/**
 * Remove global reference to the Global object.
 *
 * Warning:
 *         the routine should be called only from ecma_finalize_builtins
 */
void
ecma_builtin_finalize_global_object (void)
{
  JERRY_ASSERT (ecma_global_object_p != NULL);

  ecma_deref_object (ecma_global_object_p);
  ecma_global_object_p = NULL;
} /* ecma_builtin_finalize_global_object */

/**
 * The Global object's 'eval' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_eval (ecma_value_t x) /**< routine's first argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (x);
} /* ecma_builtin_global_object_eval */

/**
 * The Global object's 'parseInt' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_parse_int (ecma_value_t string, /**< routine's first argument */
                                      ecma_value_t radix) /**< routine's second argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (string, radix);
} /* ecma_builtin_global_object_parse_int */

/**
 * The Global object's 'parseFloat' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_parse_float (ecma_value_t string) /**< routine's first argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (string);
} /* ecma_builtin_global_object_parse_float */

/**
 * The Global object's 'isNaN' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_is_nan (ecma_value_t arg) /**< routine's first argument */
{
  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (num_value, ecma_op_to_number (arg), ret_value);

  ecma_number_t *num_p = ECMA_GET_POINTER (num_value.u.value.value);

  bool is_nan = ecma_number_is_nan (*num_p);

  ret_value = ecma_make_return_completion_value (ecma_make_simple_value (is_nan ? ECMA_SIMPLE_VALUE_TRUE
                                                                                : ECMA_SIMPLE_VALUE_FALSE));

  ECMA_FINALIZE (num_value);

  return ret_value;
} /* ecma_builtin_global_object_is_nan */

/**
 * The Global object's 'isFinite' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_is_finite (ecma_value_t arg) /**< routine's first argument */
{
  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (num_value, ecma_op_to_number (arg), ret_value);

  ecma_number_t *num_p = ECMA_GET_POINTER (num_value.u.value.value);

  bool is_finite = !(ecma_number_is_nan (*num_p)
                     || ecma_number_is_infinity (*num_p));

  ret_value = ecma_make_return_completion_value (ecma_make_simple_value (is_finite ? ECMA_SIMPLE_VALUE_TRUE
                                                                                   : ECMA_SIMPLE_VALUE_FALSE));

  ECMA_FINALIZE (num_value);

  return ret_value;
} /* ecma_builtin_global_object_is_finite */

/**
 * The Global object's 'decodeURI' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.3.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_decode_uri (ecma_value_t encoded_uri) /**< routine's first argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (encoded_uri);
} /* ecma_builtin_global_object_decode_uri */

/**
 * The Global object's 'decodeURIComponent' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.3.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_decode_uri_component (ecma_value_t encoded_uri_component) /**< routine's first argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (encoded_uri_component);
} /* ecma_builtin_global_object_decode_uri_component */

/**
 * The Global object's 'encodeURI' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.3.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_encode_uri (ecma_value_t uri) /**< routine's first argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (uri);
} /* ecma_builtin_global_object_encode_uri */

/**
 * The Global object's 'encodeURIComponent' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.3.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_encode_uri_component (ecma_value_t uri_component) /**< routine's first argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (uri_component);
} /* ecma_builtin_global_object_encode_uri_component */

/**
 * Get number of routine's parameters
 *
 * @return number of parameters
 */
ecma_length_t
ecma_builtin_global_get_routine_parameters_number (ecma_magic_string_id_t builtin_routine_id) /**< built-in routine's
                                                                                                   name */
{
  switch (builtin_routine_id)
  {
    case ECMA_MAGIC_STRING_EVAL:
    case ECMA_MAGIC_STRING_PARSE_FLOAT:
    case ECMA_MAGIC_STRING_IS_NAN:
    case ECMA_MAGIC_STRING_IS_FINITE:
    case ECMA_MAGIC_STRING_DECODE_URI:
    case ECMA_MAGIC_STRING_DECODE_URI_COMPONENT:
    case ECMA_MAGIC_STRING_ENCODE_URI:
    case ECMA_MAGIC_STRING_ENCODE_URI_COMPONENT:
    {
      return 1;
    }

    case ECMA_MAGIC_STRING_PARSE_INT:
    {
      return 2;
    }

    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* ecma_builtin_global_get_routine_parameters_number */

/**
 * Dispatcher of the Global object's built-in routines
 *
 * @return completion-value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_builtin_global_dispatch_routine (ecma_magic_string_id_t builtin_routine_id, /**< Global object's
                                                                                      built-in routine's name */
                                      ecma_value_t arguments_list [], /**< list of arguments passed to routine */
                                      ecma_length_t arguments_number) /**< length of arguments' list */
{
  const ecma_value_t value_undefined = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

  switch (builtin_routine_id)
  {
    case ECMA_MAGIC_STRING_EVAL:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_global_object_eval (arg);
    }

    case ECMA_MAGIC_STRING_PARSE_INT:
    {
      ecma_value_t arg1 = (arguments_number >= 1 ? arguments_list[0] : value_undefined);
      ecma_value_t arg2 = (arguments_number >= 2 ? arguments_list[1] : value_undefined);

      return ecma_builtin_global_object_parse_int (arg1, arg2);
    }

    case ECMA_MAGIC_STRING_PARSE_FLOAT:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_global_object_parse_float (arg);
    }

    case ECMA_MAGIC_STRING_IS_NAN:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_global_object_is_nan (arg);
    }

    case ECMA_MAGIC_STRING_IS_FINITE:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_global_object_is_finite (arg);
    }

    case ECMA_MAGIC_STRING_DECODE_URI:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_global_object_decode_uri (arg);
    }

    case ECMA_MAGIC_STRING_DECODE_URI_COMPONENT:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_global_object_decode_uri_component (arg);
    }

    case ECMA_MAGIC_STRING_ENCODE_URI:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_global_object_encode_uri (arg);
    }

    case ECMA_MAGIC_STRING_ENCODE_URI_COMPONENT:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_global_object_encode_uri_component (arg);
    }

    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* ecma_builtin_global_dispatch_routine */

/**
 * If the property's name is one of built-in properties of the Global object
 * that is not instantiated yet, instantiate the property and
 * return pointer to the instantiated property.
 *
 * @return pointer property, if one was instantiated,
 *         NULL - otherwise.
 */
ecma_property_t*
ecma_builtin_global_try_to_instantiate_property (ecma_object_t *obj_p, /**< object */
                                                 ecma_string_t *prop_name_p) /**< property's name */
{
  JERRY_ASSERT (ecma_builtin_is_global_object (obj_p));
  JERRY_ASSERT (ecma_find_named_property (obj_p, prop_name_p) == NULL);

  ecma_magic_string_id_t id;

  if (!ecma_is_string_magic (prop_name_p, &id))
  {
    return NULL;
  }

  int32_t index = ecma_builtin_bin_search_for_magic_string_id_in_array (ecma_builtin_global_property_names,
                                                                        ecma_builtin_global_property_number,
                                                                        id);

  if (index == -1)
  {
    return NULL;
  }

  JERRY_ASSERT (index >= 0 && (uint32_t) index < sizeof (uint32_t) * JERRY_BITSINBYTE);

  uint32_t bit = (uint32_t) 1u << index;

  ecma_property_t *mask_0_31_prop_p;
  mask_0_31_prop_p = ecma_get_internal_property (obj_p,
                                                 ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_0_31);
  uint32_t bit_mask = mask_0_31_prop_p->u.internal_property.value;

  if (!(bit_mask & bit))
  {
    return NULL;
  }

  bit_mask &= ~bit;

  mask_0_31_prop_p->u.internal_property.value = bit_mask;

  ecma_value_t value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  ecma_property_writable_value_t writable = ECMA_PROPERTY_WRITABLE;
  ecma_property_enumerable_value_t enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;
  ecma_property_configurable_value_t configurable = ECMA_PROPERTY_CONFIGURABLE;

  switch (id)
  {
    case ECMA_MAGIC_STRING_EVAL:
    case ECMA_MAGIC_STRING_PARSE_INT:
    case ECMA_MAGIC_STRING_PARSE_FLOAT:
    case ECMA_MAGIC_STRING_IS_NAN:
    case ECMA_MAGIC_STRING_IS_FINITE:
    case ECMA_MAGIC_STRING_DECODE_URI:
    case ECMA_MAGIC_STRING_DECODE_URI_COMPONENT:
    case ECMA_MAGIC_STRING_ENCODE_URI:
    case ECMA_MAGIC_STRING_ENCODE_URI_COMPONENT:
    {
      ecma_object_t *func_obj_p = ecma_builtin_make_function_object_for_routine (ECMA_BUILTIN_ID_GLOBAL,
                                                                                 id);

      value = ecma_make_object_value (func_obj_p);

      writable = ECMA_PROPERTY_NOT_WRITABLE;
      enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;
      configurable = ECMA_PROPERTY_NOT_CONFIGURABLE;

      break;
    }
    case ECMA_MAGIC_STRING_UNDEFINED:
    {
      value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

      writable = ECMA_PROPERTY_NOT_WRITABLE;
      enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;
      configurable = ECMA_PROPERTY_NOT_CONFIGURABLE;

      break;
    }
    case ECMA_MAGIC_STRING_NAN:
    {
      ecma_number_t *num_p = ecma_alloc_number ();
      *num_p = ecma_number_make_nan ();

      value = ecma_make_number_value (num_p);

      writable = ECMA_PROPERTY_NOT_WRITABLE;
      enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;
      configurable = ECMA_PROPERTY_NOT_CONFIGURABLE;

      break;
    }
    case ECMA_MAGIC_STRING_INFINITY_UL:
    {
      ecma_number_t *num_p = ecma_alloc_number ();
      *num_p = ecma_number_make_infinity (false);

      value = ecma_make_number_value (num_p);

      writable = ECMA_PROPERTY_NOT_WRITABLE;
      enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;
      configurable = ECMA_PROPERTY_NOT_CONFIGURABLE;

      break;
    }
    case ECMA_MAGIC_STRING_OBJECT_UL:
    case ECMA_MAGIC_STRING_FUNCTION_UL:
    case ECMA_MAGIC_STRING_ARRAY_UL:
    case ECMA_MAGIC_STRING_STRING_UL:
    case ECMA_MAGIC_STRING_BOOLEAN_UL:
    case ECMA_MAGIC_STRING_NUMBER_UL:
    case ECMA_MAGIC_STRING_DATE_UL:
    case ECMA_MAGIC_STRING_REG_EXP_UL:
    case ECMA_MAGIC_STRING_ERROR_UL:
    case ECMA_MAGIC_STRING_EVAL_ERROR_UL:
    case ECMA_MAGIC_STRING_RANGE_ERROR_UL:
    case ECMA_MAGIC_STRING_REFERENCE_ERROR_UL:
    case ECMA_MAGIC_STRING_SYNTAX_ERROR_UL:
    case ECMA_MAGIC_STRING_TYPE_ERROR_UL:
    case ECMA_MAGIC_STRING_URI_ERROR_UL:
    case ECMA_MAGIC_STRING_MATH_UL:
    case ECMA_MAGIC_STRING_JSON_U:
    {
      JERRY_UNIMPLEMENTED ();
    }

    default:
    {
      JERRY_UNREACHABLE ();
    }
  }

  ecma_property_t *prop_p = ecma_create_named_data_property (obj_p,
                                                             prop_name_p,
                                                             writable,
                                                             enumerable,
                                                             configurable);

  prop_p->u.named_data_property.value = ecma_copy_value (value, false);
  ecma_gc_update_may_ref_younger_object_flag_by_value (obj_p,
                                                       prop_p->u.named_data_property.value);

  ecma_free_value (value, true);

  return prop_p;
} /* ecma_builtin_global_try_to_instantiate_property */

/**
 * @}
 * @}
 * @}
 */
