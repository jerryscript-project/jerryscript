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
 * \addtogroup booleanprototype ECMA Boolean.prototype object built-in
 * @{
 */

#define ROUTINE_ARG_LIST_0 ecma_value_t this
#define ROUTINE(name, c_function_name, args_number, length_prop_value) \
  static ecma_completion_value_t c_function_name (ROUTINE_ARG_LIST_ ## args_number);
#include "ecma-builtin-boolean-prototype.inc.h"
#undef ROUTINE_ARG_LIST_0

/**
 * If the property's name is one of built-in properties of the Boolean.prototype object
 * that is not instantiated yet, instantiate the property and
 * return pointer to the instantiated property.
 *
 * @return pointer property, if one was instantiated,
 *         NULL - otherwise.
 */
ecma_property_t*
ecma_builtin_boolean_prototype_try_to_instantiate_property (ecma_object_t *obj_p, /**< object */
                                                            ecma_string_t *prop_name_p) /**< property's name */
{
  JERRY_ASSERT (ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_BOOLEAN_PROTOTYPE));
  JERRY_ASSERT (ecma_find_named_property (obj_p, prop_name_p) == NULL);

  ecma_magic_string_id_t id;

  if (!ecma_is_string_magic (prop_name_p, &id))
  {
    return NULL;
  }

  const ecma_magic_string_id_t ecma_builtin_property_names[] =
  {
#define OBJECT_VALUE(name, obj_getter, prop_writable, prop_enumerable, prop_configurable) name,
#define ROUTINE(name, c_function_name, args_number, length_prop_value) name,
#include "ecma-builtin-boolean-prototype.inc.h"
  };

  int32_t index;
  index = ecma_builtin_bin_search_for_magic_string_id_in_array (ecma_builtin_property_names,
                                                                sizeof (ecma_builtin_property_names) /
                                                                sizeof (ecma_builtin_property_names [0]),
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
  ecma_property_writable_value_t writable;
  ecma_property_enumerable_value_t enumerable;
  ecma_property_configurable_value_t configurable;

  switch (id)
  {
#define ROUTINE(name, c_function_name, args_number, length_prop_value) case name: \
    { \
      ecma_object_t *func_obj_p = ecma_builtin_make_function_object_for_routine (ECMA_BUILTIN_ID_BOOLEAN_PROTOTYPE, \
                                                                                 id, \
                                                                                 length_prop_value); \
      \
      writable = ECMA_PROPERTY_WRITABLE; \
      enumerable = ECMA_PROPERTY_NOT_ENUMERABLE; \
      configurable = ECMA_PROPERTY_CONFIGURABLE; \
      \
      value = ecma_make_object_value (func_obj_p); \
      \
      break; \
    }
#define OBJECT_VALUE(name, obj_getter, prop_writable, prop_enumerable, prop_configurable) case name: \
    { \
      value = ecma_make_object_value (obj_getter); \
      writable = prop_writable; \
      enumerable = prop_enumerable; \
      configurable = prop_configurable; \
      break; \
    }
#include "ecma-builtin-boolean-prototype.inc.h"

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
} /* ecma_builtin_boolean_prototype_try_to_instantiate_property */

/**
 * Dispatcher of the Boolean.prototype object's built-in routines
 *
 * @return completion-value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_builtin_boolean_prototype_dispatch_routine (ecma_magic_string_id_t builtin_routine_id, /**< Boolean.prototype
                                                                                                object's built-in
                                                                                                routine's name */
                                                 ecma_value_t this_arg_value __unused, /**< 'this' argument value */
                                                 ecma_value_t arguments_list [] __unused, /**< list of arguments
                                                                                               passed to routine */
                                                 ecma_length_t arguments_number __unused) /**< length of
                                                                                              arguments' list */
{
  switch (builtin_routine_id)
  {
#define ROUTINE_ARG(n) (arguments_number >= n ? arguments_list[n - 1] : value_undefined)
#define ROUTINE_ARG_LIST_0
#define ROUTINE_ARG_LIST_1 , ROUTINE_ARG(1)
#define ROUTINE_ARG_LIST_2 ROUTINE_ARG_LIST_1, ROUTINE_ARG(2)
#define ROUTINE_ARG_LIST_3 ROUTINE_ARG_LIST_2, ROUTINE_ARG(3)
#define ROUTINE_ARG_LIST_NON_FIXED , arguments_list, arguments_number
#define ROUTINE(name, c_function_name, args_number, length_prop_value) \
       case name: \
       { \
         return c_function_name (this_arg_value ROUTINE_ARG_LIST_ ## args_number); \
       }
#include "ecma-builtin-boolean-prototype.inc.h"
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
} /* ecma_builtin_boolean_prototype_dispatch_routine */

/**
 * The Boolean.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.6.4.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_boolean_prototype_object_to_string (ecma_value_t this) /**< this argument */
{
  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (completion_value_of,
                  ecma_builtin_boolean_prototype_object_value_of (this),
                  ret_value);

  ecma_string_t *ret_str_p;

  if (ecma_is_completion_value_normal_true (completion_value_of))
  {
    ret_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING_TRUE);
  }
  else
  {
    JERRY_ASSERT (ecma_is_completion_value_normal_false (completion_value_of));

    ret_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING_FALSE);
  }

  ret_value = ecma_make_normal_completion_value (ecma_make_string_value (ret_str_p));

  ECMA_FINALIZE (completion_value_of);

  return ret_value;
} /* ecma_builtin_boolean_prototype_object_to_string */

/**
 * The Boolean.prototype object's 'valueOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.6.4.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_boolean_prototype_object_value_of (ecma_value_t this) /**< this argument */
{
  if (ecma_is_value_boolean (this))
  {
    return ecma_make_normal_completion_value (this);
  }
  else if (this.value_type == ECMA_TYPE_OBJECT)
  {
    ecma_object_t *obj_p = ECMA_GET_POINTER (this.value);

    ecma_property_t *class_prop_p = ecma_get_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_CLASS);

    if (class_prop_p->u.internal_property.value == ECMA_MAGIC_STRING_BOOLEAN_UL)
    {
      ecma_property_t *prim_value_prop_p = ecma_get_internal_property (obj_p,
                                                                       ECMA_INTERNAL_PROPERTY_PRIMITIVE_BOOLEAN_VALUE);

      JERRY_ASSERT (prim_value_prop_p->u.internal_property.value < ECMA_SIMPLE_VALUE__COUNT);

      ecma_simple_value_t prim_simple_value = prim_value_prop_p->u.internal_property.value;

      ecma_value_t ret_boolean_value = ecma_make_simple_value (prim_simple_value);

      JERRY_ASSERT (ecma_is_value_boolean (ret_boolean_value));

      return ecma_make_normal_completion_value (ret_boolean_value);
    }
  }

  return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
} /* ecma_builtin_boolean_prototype_object_value_of */

/**
 * @}
 * @}
 * @}
 */
