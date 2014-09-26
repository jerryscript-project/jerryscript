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
 * \addtogroup objectprototype ECMA Object.prototype object built-in
 * @{
 */

/**
 * List of the Object.prototype object built-in object value properties in format 'macro (name, value)'.
 */
#define ECMA_BUILTIN_OBJECT_PROTOTYPE_OBJECT_OBJECT_VALUES_PROPERTY_LIST(macro) \
  macro (ECMA_MAGIC_STRING_CONSTRUCTOR, ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT))

/**
 * List of the Object.prototype object built-in routine properties in format
 * 'macro (name, C function name, arguments number of the routine, length value of the routine)'.
 */
#define ECMA_BUILTIN_OBJECT_PROTOTYPE_OBJECT_ROUTINES_PROPERTY_LIST(macro) \
  macro (ECMA_MAGIC_STRING_TO_STRING_UL, \
         ecma_builtin_object_prototype_object_to_string, \
         0, \
         0) \
  macro (ECMA_MAGIC_STRING_VALUE_OF_UL, \
         ecma_builtin_object_prototype_object_value_of, \
         0, \
         0) \
  macro (ECMA_MAGIC_STRING_TO_LOCALE_STRING_UL, \
         ecma_builtin_object_prototype_object_to_locale_string, \
         0, \
         0) \
  macro (ECMA_MAGIC_STRING_HAS_OWN_PROPERTY_UL, \
         ecma_builtin_object_prototype_object_has_own_property, \
         1, \
         1) \
  macro (ECMA_MAGIC_STRING_IS_PROTOTYPE_OF_UL, \
         ecma_builtin_object_prototype_object_is_prototype_of, \
         1, \
         1) \
  macro (ECMA_MAGIC_STRING_PROPERTY_IS_ENUMERABLE_UL, \
         ecma_builtin_object_prototype_object_property_is_enumerable, \
         1, \
         1)

/**
 * List of the Object.prototype object's built-in property names
 */
static const ecma_magic_string_id_t ecma_builtin_property_names[] =
{
#define VALUE_PROP_LIST(name, value) name,
#define ROUTINE_PROP_LIST(name, c_function_name, args_number, length) name,
  ECMA_BUILTIN_OBJECT_PROTOTYPE_OBJECT_OBJECT_VALUES_PROPERTY_LIST (VALUE_PROP_LIST)
  ECMA_BUILTIN_OBJECT_PROTOTYPE_OBJECT_ROUTINES_PROPERTY_LIST (ROUTINE_PROP_LIST)
#undef VALUE_PROP_LIST
#undef ROUTINE_PROP_LIST
};

/**
 * Number of the Object.prototype object's built-in properties
 */
const ecma_length_t ecma_builtin_object_prototype_property_number = (sizeof (ecma_builtin_property_names) /
                                                                     sizeof (ecma_magic_string_id_t));

/**
 * The Object.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_prototype_object_to_string (ecma_value_t this) /**< this argument */
{
  ecma_magic_string_id_t type_string;

  if (ecma_is_value_undefined (this))
  {
    type_string = ECMA_MAGIC_STRING_UNDEFINED_UL;
  }
  else if (ecma_is_value_null (this))
  {
    type_string = ECMA_MAGIC_STRING_NULL_UL;
  }
  else
  {
    ecma_completion_value_t obj_this = ecma_op_to_object (this);

    if (!ecma_is_completion_value_normal (obj_this))
    {
      return obj_this;
    }

    JERRY_ASSERT (obj_this.u.value.value_type == ECMA_TYPE_OBJECT);

    ecma_object_t *obj_p = ECMA_GET_POINTER (obj_this.u.value.value);

    ecma_property_t *class_prop_p = ecma_get_internal_property (obj_p,
                                                                ECMA_INTERNAL_PROPERTY_CLASS);
    type_string = (ecma_magic_string_id_t) class_prop_p->u.internal_property.value;

    ecma_free_completion_value (obj_this);
  }

  /* Building string "[object #type#]" where type is 'Undefined',
     'Null' or one of possible object's classes.
     The string with null character is maximum 19 characters long. */
  const ssize_t buffer_size = 19;
  ecma_char_t str_buffer[buffer_size];

  const ecma_char_t *left_square_zt_str_p = ecma_get_magic_string_zt (ECMA_MAGIC_STRING_LEFT_SQUARE_CHAR);
  const ecma_char_t *object_zt_str_p = ecma_get_magic_string_zt (ECMA_MAGIC_STRING_OBJECT);
  const ecma_char_t *space_zt_str_p = ecma_get_magic_string_zt (ECMA_MAGIC_STRING_SPACE_CHAR);
  const ecma_char_t *type_name_zt_str_p = ecma_get_magic_string_zt (type_string);
  const ecma_char_t *right_square_zt_str_p = ecma_get_magic_string_zt (ECMA_MAGIC_STRING_RIGHT_SQUARE_CHAR);

  ecma_char_t *buffer_ptr = str_buffer;
  ssize_t buffer_size_left = buffer_size;
  buffer_ptr = ecma_copy_zt_string_to_buffer (left_square_zt_str_p,
                                              buffer_ptr,
                                              buffer_size_left);
  buffer_size_left = buffer_size - (buffer_ptr - str_buffer) * (ssize_t) sizeof (ecma_char_t);
  buffer_ptr = ecma_copy_zt_string_to_buffer (object_zt_str_p,
                                              buffer_ptr,
                                              buffer_size_left);
  buffer_size_left = buffer_size - (buffer_ptr - str_buffer) * (ssize_t) sizeof (ecma_char_t);
  buffer_ptr = ecma_copy_zt_string_to_buffer (space_zt_str_p,
                                              buffer_ptr,
                                              buffer_size_left);
  buffer_size_left = buffer_size - (buffer_ptr - str_buffer) * (ssize_t) sizeof (ecma_char_t);
  buffer_ptr = ecma_copy_zt_string_to_buffer (type_name_zt_str_p,
                                              buffer_ptr,
                                              buffer_size_left);
  buffer_size_left = buffer_size - (buffer_ptr - str_buffer) * (ssize_t) sizeof (ecma_char_t);
  buffer_ptr = ecma_copy_zt_string_to_buffer (right_square_zt_str_p,
                                              buffer_ptr,
                                              buffer_size_left);
  buffer_size_left = buffer_size - (buffer_ptr - str_buffer) * (ssize_t) sizeof (ecma_char_t);

  JERRY_ASSERT (buffer_size_left >= 0);

  ecma_string_t *ret_string_p = ecma_new_ecma_string (str_buffer);

  return ecma_make_normal_completion_value (ecma_make_string_value (ret_string_p));
} /* ecma_builtin_object_prototype_object_to_string */

/**
 * The Object.prototype object's 'valueOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_prototype_object_value_of (ecma_value_t this) /**< this argument */
{
  return ecma_op_to_object (this);
} /* ecma_builtin_object_prototype_object_value_of */

/**
 * The Object.prototype object's 'toLocaleString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_prototype_object_to_locale_string (ecma_value_t this) /**< this argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (this);
} /* ecma_builtin_object_prototype_object_to_locale_string */

/**
 * The Object.prototype object's 'hasOwnProperty' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_prototype_object_has_own_property (ecma_value_t this, /**< this argument */
                                                       ecma_value_t arg) /**< first argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (this, arg);
} /* ecma_builtin_object_prototype_object_has_own_property */

/**
 * The Object.prototype object's 'isPrototypeOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.6
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_prototype_object_is_prototype_of (ecma_value_t this, /**< this argument */
                                                      ecma_value_t arg) /**< routine's first argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (this, arg);
} /* ecma_builtin_object_prototype_object_is_prototype_of */

/**
 * The Object.prototype object's 'propertyIsEnumerable' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.7
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_prototype_object_property_is_enumerable (ecma_value_t this, /**< this argument */
                                                             ecma_value_t arg) /**< routine's first argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (this, arg);
} /* ecma_builtin_object_prototype_object_property_is_enumerable */

/**
 * If the property's name is one of built-in properties of the Object.prototype object
 * that is not instantiated yet, instantiate the property and
 * return pointer to the instantiated property.
 *
 * @return pointer property, if one was instantiated,
 *         NULL - otherwise.
 */
ecma_property_t*
ecma_builtin_object_prototype_try_to_instantiate_property (ecma_object_t *obj_p, /**< object */
                                                           ecma_string_t *prop_name_p) /**< property's name */
{
  JERRY_ASSERT (ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_OBJECT_PROTOTYPE));
  JERRY_ASSERT (ecma_find_named_property (obj_p, prop_name_p) == NULL);

  ecma_magic_string_id_t id;

  if (!ecma_is_string_magic (prop_name_p, &id))
  {
    return NULL;
  }

  int32_t index = ecma_builtin_bin_search_for_magic_string_id_in_array (ecma_builtin_property_names,
                                                                        ecma_builtin_object_prototype_property_number,
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
#define CASE_ROUTINE_PROP_LIST(name, c_function_name, args_number, length) case name:
    ECMA_BUILTIN_OBJECT_PROTOTYPE_OBJECT_ROUTINES_PROPERTY_LIST (CASE_ROUTINE_PROP_LIST)
#undef CASE_ROUTINE_PROP_LIST
    {
      ecma_object_t *func_obj_p = ecma_builtin_make_function_object_for_routine (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
                                                                                 id);

      value = ecma_make_object_value (func_obj_p);

      break;
    }
#define CASE_VALUE_PROP_LIST(name, value) case name:
    ECMA_BUILTIN_OBJECT_PROTOTYPE_OBJECT_OBJECT_VALUES_PROPERTY_LIST (CASE_VALUE_PROP_LIST)
#undef CASE_VALUE_PROP_LIST
    {
      writable = ECMA_PROPERTY_NOT_WRITABLE;
      enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;
      configurable = ECMA_PROPERTY_NOT_CONFIGURABLE;

      switch (id)
      {
#define CASE_OBJECT_VALUE_PROP_LIST(name, value_obj) case name: { value = ecma_make_object_value (value_obj); break; }
        ECMA_BUILTIN_OBJECT_PROTOTYPE_OBJECT_OBJECT_VALUES_PROPERTY_LIST (CASE_OBJECT_VALUE_PROP_LIST)
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
} /* ecma_builtin_object_prototype_try_to_instantiate_property */

/**
 * Dispatcher of the Object.prototype object's built-in routines
 *
 * @return completion-value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_builtin_object_prototype_dispatch_routine (ecma_magic_string_id_t builtin_routine_id, /**< Object.prototype
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
    ECMA_BUILTIN_OBJECT_PROTOTYPE_OBJECT_ROUTINES_PROPERTY_LIST (CASE_ROUTINE_PROP_LIST)
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
} /* ecma_builtin_object_prototype_dispatch_routine */

/**
 * Get number of routine's parameters
 *
 * @return number of parameters
 */
ecma_length_t
ecma_builtin_object_prototype_get_routine_parameters_number (ecma_magic_string_id_t builtin_routine_id) /**< built-in
                                                                                                             routine's
                                                                                                             name */
{
  switch (builtin_routine_id)
  {
#define CASE_ROUTINE_PROP_LIST(name, c_function_name, args_number, length) case name: return length;
    ECMA_BUILTIN_OBJECT_PROTOTYPE_OBJECT_ROUTINES_PROPERTY_LIST (CASE_ROUTINE_PROP_LIST)
#undef CASE_ROUTINE_PROP_LIST

    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* ecma_builtin_object_prototype_get_routine_parameters_number */

/**
 * @}
 * @}
 * @}
 */
