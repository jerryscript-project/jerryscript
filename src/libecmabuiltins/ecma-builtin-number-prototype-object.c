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
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-string-object.h"
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
 * \addtogroup numberprototype ECMA Number.prototype object built-in
 * @{
 */

/**
 * List of the Number.prototype object built-in object value properties in format 'macro (name, value)'.
 */
#define ECMA_BUILTIN_NUMBER_PROTOTYPE_OBJECT_OBJECT_VALUES_PROPERTY_LIST(macro) \
  macro (ECMA_MAGIC_STRING_CONSTRUCTOR, ecma_builtin_get (ECMA_BUILTIN_ID_NUMBER))

/**
 * List of the Number.prototype object built-in routine properties in format
 * 'macro (name, C function name, arguments number of the routine, length value of the routine)'.
 */
#define ECMA_BUILTIN_NUMBER_PROTOTYPE_OBJECT_ROUTINES_PROPERTY_LIST(macro) \
  macro (ECMA_MAGIC_STRING_TO_STRING_UL, \
         ecma_builtin_number_prototype_object_to_string, \
         NON_FIXED, \
         1) \
  macro (ECMA_MAGIC_STRING_VALUE_OF_UL, \
         ecma_builtin_number_prototype_object_value_of, \
         0, \
         0) \
  macro (ECMA_MAGIC_STRING_TO_LOCALE_STRING_UL, \
         ecma_builtin_number_prototype_object_to_locale_string, \
         0, \
         0) \
  macro (ECMA_MAGIC_STRING_TO_FIXED_UL, \
         ecma_builtin_number_prototype_object_to_fixed, \
         1, \
         1) \
  macro (ECMA_MAGIC_STRING_TO_EXPONENTIAL_UL, \
         ecma_builtin_number_prototype_object_to_exponential, \
         1, \
         1) \
  macro (ECMA_MAGIC_STRING_TO_PRECISION_UL, \
         ecma_builtin_number_prototype_object_to_precision, \
         1, \
         1)

/**
 * List of the Number.prototype object's built-in property names
 */
static const ecma_magic_string_id_t ecma_builtin_property_names[] =
{
#define VALUE_PROP_LIST(name, value) name,
#define ROUTINE_PROP_LIST(name, c_function_name, args_number, length) name,
  ECMA_BUILTIN_NUMBER_PROTOTYPE_OBJECT_OBJECT_VALUES_PROPERTY_LIST (VALUE_PROP_LIST)
  ECMA_BUILTIN_NUMBER_PROTOTYPE_OBJECT_ROUTINES_PROPERTY_LIST (ROUTINE_PROP_LIST)
#undef VALUE_PROP_LIST
#undef ROUTINE_PROP_LIST
};

/**
 * Number of the Number.prototype object's built-in properties
 */
static const ecma_length_t ecma_builtin_number_prototype_property_number = (sizeof (ecma_builtin_property_names) /
                                                                            sizeof (ecma_magic_string_id_t));

/**
 * The Number.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.7.4.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_number_prototype_object_to_string (ecma_value_t this, /**< this argument */
                                                ecma_value_t* arguments_list_p, /**< arguments list */
                                                ecma_length_t arguments_list_len) /**< number of arguments */
{
  ecma_number_t this_arg_number;

  if (this.value_type == ECMA_TYPE_NUMBER)
  {
    ecma_number_t *this_arg_number_p = ECMA_GET_POINTER (this.value);

    this_arg_number = *this_arg_number_p;
  }
  else if (this.value_type == ECMA_TYPE_OBJECT)
  {
    ecma_object_t *obj_p = ECMA_GET_POINTER (this.value);

    ecma_property_t *class_prop_p = ecma_get_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_CLASS);

    if (class_prop_p->u.internal_property.value == ECMA_MAGIC_STRING_NUMBER_UL)
    {
      ecma_property_t *prim_value_prop_p = ecma_get_internal_property (obj_p,
                                                                       ECMA_INTERNAL_PROPERTY_PRIMITIVE_NUMBER_VALUE);

      ecma_number_t *prim_value_num_p = ECMA_GET_POINTER (prim_value_prop_p->u.internal_property.value);
      this_arg_number = *prim_value_num_p;
    }
    else
    {
      return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
  }
  else
  {
    return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }

  if (arguments_list_len == 0)
  {
    ecma_string_t *ret_str_p = ecma_new_ecma_string_from_number (this_arg_number);

    return ecma_make_normal_completion_value (ecma_make_string_value (ret_str_p));
  }
  {
    ECMA_BUILTIN_CP_UNIMPLEMENTED (arguments_list_p);
  }
} /* ecma_builtin_number_prototype_object_to_string */

/**
 * The Number.prototype object's 'toLocaleString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.7.4.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_number_prototype_object_to_locale_string (ecma_value_t this) /**< this argument */
{
  return ecma_builtin_number_prototype_object_to_string (this, NULL, 0);
} /* ecma_builtin_number_prototype_object_to_locale_string */

/**
 * The Number.prototype object's 'valueOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.7.4.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_number_prototype_object_value_of (ecma_value_t this) /**< this argument */
{
  if (this.value_type == ECMA_TYPE_NUMBER)
  {
    return ecma_make_normal_completion_value (ecma_copy_value (this, true));
  }
  else if (this.value_type == ECMA_TYPE_OBJECT)
  {
    ecma_object_t *obj_p = ECMA_GET_POINTER (this.value);

    ecma_property_t *class_prop_p = ecma_get_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_CLASS);

    if (class_prop_p->u.internal_property.value == ECMA_MAGIC_STRING_NUMBER_UL)
    {
      ecma_property_t *prim_value_prop_p = ecma_get_internal_property (obj_p,
                                                                       ECMA_INTERNAL_PROPERTY_PRIMITIVE_NUMBER_VALUE);

      ecma_number_t *prim_value_num_p = ECMA_GET_POINTER (prim_value_prop_p->u.internal_property.value);

      ecma_number_t *ret_num_p = ecma_alloc_number ();
      *ret_num_p = *prim_value_num_p;

      return ecma_make_normal_completion_value (ecma_make_number_value (ret_num_p));
    }
  }

  return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
} /* ecma_builtin_number_prototype_object_value_of */

/**
 * The Number.prototype object's 'toFixed' routine
 *
 * See also:
 *          ECMA-262 v5, 15.7.4.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_number_prototype_object_to_fixed (ecma_value_t this, /**< this argument */
                                               ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this, arg);
} /* ecma_builtin_number_prototype_object_to_fixed */

/**
 * The Number.prototype object's 'toExponential' routine
 *
 * See also:
 *          ECMA-262 v5, 15.7.4.6
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_number_prototype_object_to_exponential (ecma_value_t this, /**< this argument */
                                                     ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this, arg);
} /* ecma_builtin_number_prototype_object_to_exponential */

/**
 * The Number.prototype object's 'toPrecision' routine
 *
 * See also:
 *          ECMA-262 v5, 15.7.4.7
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_number_prototype_object_to_precision (ecma_value_t this, /**< this argument */
                                                   ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this, arg);
} /* ecma_builtin_number_prototype_object_to_precision */

/**
 * If the property's name is one of built-in properties of the Number.prototype object
 * that is not instantiated yet, instantiate the property and
 * return pointer to the instantiated property.
 *
 * @return pointer property, if one was instantiated,
 *         NULL - otherwise.
 */
ecma_property_t*
ecma_builtin_number_prototype_try_to_instantiate_property (ecma_object_t *obj_p, /**< object */
                                                           ecma_string_t *prop_name_p) /**< property's name */
{
  JERRY_ASSERT (ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_NUMBER_PROTOTYPE));
  JERRY_ASSERT (ecma_find_named_property (obj_p, prop_name_p) == NULL);

  ecma_magic_string_id_t id;

  if (!ecma_is_string_magic (prop_name_p, &id))
  {
    return NULL;
  }

  int32_t index = ecma_builtin_bin_search_for_magic_string_id_in_array (ecma_builtin_property_names,
                                                                        ecma_builtin_number_prototype_property_number,
                                                                        id);

  if (index == -1)
  {
    return NULL;
  }

  JERRY_ASSERT (index >= 0 && (uint32_t) index < sizeof (uint64_t) * JERRY_BITSINBYTE);

  uint32_t bit;
  ecma_internal_property_id_t mask_prop_id;

  if (index >= 32)
  {
    mask_prop_id = ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_32_63;
    bit = (uint32_t) 1u << (index - 32);
  }
  else
  {
    mask_prop_id = ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_0_31;
    bit = (uint32_t) 1u << index;
  }

  ecma_property_t *mask_prop_p = ecma_find_internal_property (obj_p, mask_prop_id);
  if (mask_prop_p == NULL)
  {
    mask_prop_p = ecma_create_internal_property (obj_p, mask_prop_id);
    mask_prop_p->u.internal_property.value = 0;
  }

  uint32_t bit_mask = mask_prop_p->u.internal_property.value;

  if (bit_mask & bit)
  {
    return NULL;
  }

  bit_mask |= bit;

  mask_prop_p->u.internal_property.value = bit_mask;

  ecma_value_t value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  ecma_property_writable_value_t writable = ECMA_PROPERTY_WRITABLE;
  ecma_property_enumerable_value_t enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;
  ecma_property_configurable_value_t configurable = ECMA_PROPERTY_CONFIGURABLE;

  switch (id)
  {
#define CASE_ROUTINE_PROP_LIST(name, c_function_name, args_number, length) case name: \
    { \
      ecma_object_t *func_obj_p = ecma_builtin_make_function_object_for_routine (ECMA_BUILTIN_ID_NUMBER_PROTOTYPE, \
                                                                                 id, \
                                                                                 length); \
      \
      value = ecma_make_object_value (func_obj_p); \
      \
      break; \
    }
    ECMA_BUILTIN_NUMBER_PROTOTYPE_OBJECT_ROUTINES_PROPERTY_LIST (CASE_ROUTINE_PROP_LIST)
#undef CASE_ROUTINE_PROP_LIST
#define CASE_VALUE_PROP_LIST(name, value) case name:
    ECMA_BUILTIN_NUMBER_PROTOTYPE_OBJECT_OBJECT_VALUES_PROPERTY_LIST (CASE_VALUE_PROP_LIST)
#undef CASE_VALUE_PROP_LIST
    {
      writable = ECMA_PROPERTY_NOT_WRITABLE;
      enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;
      configurable = ECMA_PROPERTY_NOT_CONFIGURABLE;

      switch (id)
      {
#define CASE_OBJECT_VALUE_PROP_LIST(name, value_obj) case name: { value = ecma_make_object_value (value_obj); break; }
        ECMA_BUILTIN_NUMBER_PROTOTYPE_OBJECT_OBJECT_VALUES_PROPERTY_LIST (CASE_OBJECT_VALUE_PROP_LIST)
#undef CASE_OBJECT_VALUE_PROP_LIST
        default:
        {
          JERRY_UNREACHABLE ();
        }
      }

      break;
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
} /* ecma_builtin_number_prototype_try_to_instantiate_property */

/**
 * Dispatcher of the Number.prototype object's built-in routines
 *
 * @return completion-value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_builtin_number_prototype_dispatch_routine (ecma_magic_string_id_t builtin_routine_id, /**< Number.prototype
                                                                                                object's built-in
                                                                                                routine's name */
                                                ecma_value_t this_arg_value __unused, /**< 'this' argument value */
                                                ecma_value_t arguments_list [], /**< list of arguments
                                                                                     passed to routine */
                                                ecma_length_t arguments_number) /**< length of arguments' list */
{
  const ecma_value_t value_undefined = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

  switch (builtin_routine_id)
  {
#define ROUTINE_ARG(n) (arguments_number >= n ? arguments_list[n - 1] : value_undefined)
#define ROUTINE_ARG_LIST_0
#define ROUTINE_ARG_LIST_1 , ROUTINE_ARG(1)
#define ROUTINE_ARG_LIST_2 ROUTINE_ARG_LIST_1, ROUTINE_ARG(2)
#define ROUTINE_ARG_LIST_3 ROUTINE_ARG_LIST_2, ROUTINE_ARG(3)
#define ROUTINE_ARG_LIST_NON_FIXED , arguments_list, arguments_number
#define CASE_ROUTINE_PROP_LIST(name, c_function_name, args_number, length) \
       case name: \
       { \
         return c_function_name (this_arg_value ROUTINE_ARG_LIST_ ## args_number); \
       }
    ECMA_BUILTIN_NUMBER_PROTOTYPE_OBJECT_ROUTINES_PROPERTY_LIST (CASE_ROUTINE_PROP_LIST)
#undef CASE_ROUTINE_PROP_LIST
#undef ROUTINE_ARG_LIST_0
#undef ROUTINE_ARG_LIST_1
#undef ROUTINE_ARG_LIST_2
#undef ROUTINE_ARG_LIST_3
#undef ROUTINE_ARG_LIST_NON_FIXED

    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* ecma_builtin_number_prototype_dispatch_routine */

/**
 * @}
 * @}
 * @}
 */
