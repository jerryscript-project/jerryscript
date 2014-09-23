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
#include "ecma-objects-general.h"
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
 * \addtogroup object ECMA Object object built-in
 * @{
 */

/**
 * List of the Math object built-in value properties in format 'macro (name, value)'.
 */
#define ECMA_BUILTIN_MATH_OBJECT_VALUES_PROPERTY_LIST(macro) \
  macro (ECMA_MAGIC_STRING_E_U,       2.7182818284590452354) \
  macro (ECMA_MAGIC_STRING_LN10_U,    2.302585092994046) \
  macro (ECMA_MAGIC_STRING_LN2_U,     0.6931471805599453) \
  macro (ECMA_MAGIC_STRING_LOG2E_U,   1.4426950408889634) \
  macro (ECMA_MAGIC_STRING_LOG10E_U,  0.4342944819032518) \
  macro (ECMA_MAGIC_STRING_PI_U,      3.1415926535897932) \
  macro (ECMA_MAGIC_STRING_SQRT1_2_U, 0.7071067811865476) \
  macro (ECMA_MAGIC_STRING_SQRT2_U,   1.4142135623730951)

/**
 * List of the Math object built-in routine properties in format 'macro (name, length value of the routine)'.
 */
#define ECMA_BUILTIN_MATH_OBJECT_ROUTINES_PROPERTY_LIST(macro) \
  macro (ECMA_MAGIC_STRING_ABS, 1) \
  macro (ECMA_MAGIC_STRING_ACOS, 1) \
  macro (ECMA_MAGIC_STRING_ASIN, 1) \
  macro (ECMA_MAGIC_STRING_ATAN, 1) \
  macro (ECMA_MAGIC_STRING_ATAN2, 2) \
  macro (ECMA_MAGIC_STRING_CEIL, 1) \
  macro (ECMA_MAGIC_STRING_COS, 1) \
  macro (ECMA_MAGIC_STRING_EXP, 1) \
  macro (ECMA_MAGIC_STRING_FLOOR, 1) \
  macro (ECMA_MAGIC_STRING_LOG, 1) \
  macro (ECMA_MAGIC_STRING_MAX, 2) \
  macro (ECMA_MAGIC_STRING_MIN, 2) \
  macro (ECMA_MAGIC_STRING_POW, 2) \
  macro (ECMA_MAGIC_STRING_RANDOM, 0) \
  macro (ECMA_MAGIC_STRING_ROUND, 1) \
  macro (ECMA_MAGIC_STRING_SIN, 1) \
  macro (ECMA_MAGIC_STRING_SQRT, 1) \
  macro (ECMA_MAGIC_STRING_TAN, 1)

/**
 * List of the Math object's built-in property names
 */
static const ecma_magic_string_id_t ecma_builtin_math_property_names[] =
{
#define VALUE_PROP_LIST(name, value) name,
#define ROUTINE_PROP_LIST(name, length) name,
  ECMA_BUILTIN_MATH_OBJECT_VALUES_PROPERTY_LIST (VALUE_PROP_LIST)
  ECMA_BUILTIN_MATH_OBJECT_ROUTINES_PROPERTY_LIST (ROUTINE_PROP_LIST)
#undef VALUE_PROP_LIST
#undef ROUTINE_PROP_LIST
};

/**
 * Number of the Math object's built-in properties
 */
const ecma_length_t ecma_builtin_math_property_number = (sizeof (ecma_builtin_math_property_names) /
                                                         sizeof (ecma_magic_string_id_t));
JERRY_STATIC_ASSERT (sizeof (ecma_builtin_math_property_names) > sizeof (void*));

/**
 * If the property's name is one of built-in properties of the Math object
 * that is not instantiated yet, instantiate the property and
 * return pointer to the instantiated property.
 *
 * @return pointer property, if one was instantiated,
 *         NULL - otherwise.
 */
ecma_property_t*
ecma_builtin_math_try_to_instantiate_property (ecma_object_t *obj_p, /**< object */
                                               ecma_string_t *prop_name_p) /**< property's name */
{
  JERRY_ASSERT (ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_MATH));
  JERRY_ASSERT (ecma_find_named_property (obj_p, prop_name_p) == NULL);

  ecma_magic_string_id_t id;

  if (!ecma_is_string_magic (prop_name_p, &id))
  {
    return NULL;
  }

  int32_t index = ecma_builtin_bin_search_for_magic_string_id_in_array (ecma_builtin_math_property_names,
                                                                        ecma_builtin_math_property_number,
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

  ecma_property_t *mask_prop_p = ecma_get_internal_property (obj_p, mask_prop_id);
  uint32_t bit_mask = mask_prop_p->u.internal_property.value;

  if (!(bit_mask & bit))
  {
    return NULL;
  }

  bit_mask &= ~bit;

  mask_prop_p->u.internal_property.value = bit_mask;

  ecma_value_t value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  ecma_property_writable_value_t writable = ECMA_PROPERTY_WRITABLE;
  ecma_property_enumerable_value_t enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;
  ecma_property_configurable_value_t configurable = ECMA_PROPERTY_CONFIGURABLE;

  switch (id)
  {
#define CASE_ROUTINE_PROP_LIST(name, length) case name:
    ECMA_BUILTIN_MATH_OBJECT_ROUTINES_PROPERTY_LIST (CASE_ROUTINE_PROP_LIST)
#undef CASE_ROUTINE_PROP_LIST
    {
      ecma_object_t *func_obj_p = ecma_builtin_make_function_object_for_routine (ECMA_BUILTIN_ID_MATH, id);

      value = ecma_make_object_value (func_obj_p);

      break;
    }
#define CASE_VALUE_PROP_LIST(name, value) case name:
    ECMA_BUILTIN_MATH_OBJECT_VALUES_PROPERTY_LIST (CASE_VALUE_PROP_LIST)
#undef CASE_VALUE_PROP_LIST
    {
      writable = ECMA_PROPERTY_NOT_WRITABLE;
      enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;
      configurable = ECMA_PROPERTY_NOT_CONFIGURABLE;

      ecma_number_t *num_p = ecma_alloc_number ();
      value = ecma_make_number_value (num_p);

      switch (id)
      {
#define CASE_VALUE_PROP_LIST(name, value) case name: { *num_p = (ecma_number_t) value; break; }
        ECMA_BUILTIN_MATH_OBJECT_VALUES_PROPERTY_LIST (CASE_VALUE_PROP_LIST)
#undef CASE_VALUE_PROP_LIST
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
} /* ecma_builtin_math_try_to_instantiate_property */

/**
 * @}
 * @}
 * @}
 */
