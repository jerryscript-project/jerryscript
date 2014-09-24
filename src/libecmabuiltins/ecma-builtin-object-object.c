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
 * List of the Object object's built-in property names
 */
static const ecma_magic_string_id_t ecma_builtin_object_property_names[] =
{
  ECMA_MAGIC_STRING_PROTOTYPE,
  ECMA_MAGIC_STRING_LENGTH,
  ECMA_MAGIC_STRING_GET_PROTOTYPE_OF_UL,
  ECMA_MAGIC_STRING_GET_OWN_PROPERTY_DESCRIPTOR_UL,
  ECMA_MAGIC_STRING_GET_OWN_PROPERTY_NAMES_UL,
  ECMA_MAGIC_STRING_CREATE,
  ECMA_MAGIC_STRING_DEFINE_PROPERTY_UL,
  ECMA_MAGIC_STRING_DEFINE_PROPERTIES_UL,
  ECMA_MAGIC_STRING_SEAL,
  ECMA_MAGIC_STRING_FREEZE,
  ECMA_MAGIC_STRING_PREVENT_EXTENSIONS_UL,
  ECMA_MAGIC_STRING_IS_SEALED_UL,
  ECMA_MAGIC_STRING_IS_FROZEN_UL,
  ECMA_MAGIC_STRING_IS_EXTENSIBLE,
  ECMA_MAGIC_STRING_KEYS
};

/**
 * Number of the Object object's built-in properties
 */
const ecma_length_t ecma_builtin_object_property_number = (sizeof (ecma_builtin_object_property_names) /
                                                           sizeof (ecma_magic_string_id_t));
JERRY_STATIC_ASSERT (sizeof (ecma_builtin_object_property_names) > sizeof (void*));

/**
 * Get number of routine's parameters
 *
 * @return number of parameters
 */
ecma_length_t
ecma_builtin_object_get_routine_parameters_number (ecma_magic_string_id_t builtin_routine_id) /**< built-in routine's
                                                                                                   name */
{
  switch (builtin_routine_id)
  {
    case ECMA_MAGIC_STRING_GET_PROTOTYPE_OF_UL:
    case ECMA_MAGIC_STRING_GET_OWN_PROPERTY_NAMES_UL:
    case ECMA_MAGIC_STRING_SEAL:
    case ECMA_MAGIC_STRING_FREEZE:
    case ECMA_MAGIC_STRING_PREVENT_EXTENSIONS_UL:
    case ECMA_MAGIC_STRING_IS_SEALED_UL:
    case ECMA_MAGIC_STRING_IS_FROZEN_UL:
    case ECMA_MAGIC_STRING_IS_EXTENSIBLE:
    case ECMA_MAGIC_STRING_KEYS:
    {
      return 1;
    }

    case ECMA_MAGIC_STRING_GET_OWN_PROPERTY_DESCRIPTOR_UL:
    case ECMA_MAGIC_STRING_CREATE:
    case ECMA_MAGIC_STRING_DEFINE_PROPERTIES_UL:
    {
      return 2;
    }

    case ECMA_MAGIC_STRING_DEFINE_PROPERTY_UL:
    {
      return 3;
    }

    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* ecma_builtin_object_get_routine_parameters_number */

/**
 * Handle calling [[Call]] of built-in Object object
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_object_dispatch_call (ecma_value_t *arguments_list_p, /**< arguments list */
                                   ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_completion_value_t ret_value;

  if (arguments_list_len == 0
      || ecma_is_value_undefined (arguments_list_p[0])
      || ecma_is_value_null (arguments_list_p [0]))
  {
    ret_value = ecma_builtin_object_dispatch_construct (arguments_list_p, arguments_list_len);
  }
  else
  {
    ret_value = ecma_op_to_object (arguments_list_p [0]);
  }

  return ret_value;
} /* ecma_builtin_object_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in Object object
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_object_dispatch_construct (ecma_value_t *arguments_list_p, /**< arguments list */
                                        ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  if (arguments_list_len == 0)
  {
    ecma_object_t *obj_p = ecma_op_create_object_object_noarg ();

    return ecma_make_normal_completion_value (ecma_make_object_value (obj_p));
  }
  else
  {
    ecma_completion_value_t new_obj_value = ecma_op_create_object_object_arg (arguments_list_p [0]);

    if (!ecma_is_completion_value_normal (new_obj_value))
    {
      return new_obj_value;
    }
    else
    {
      return ecma_make_normal_completion_value (new_obj_value.u.value);
    }
  }
} /* ecma_builtin_object_dispatch_construct */

/**
 * The Object object's 'getPrototypeOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_object_get_prototype_of (ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (arg);
} /* ecma_builtin_object_object_get_prototype_of */

/**
 * The Object object's 'getOwnPropertyNames' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_object_get_own_property_names (ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (arg);
} /* ecma_builtin_object_object_get_own_property_names */

/**
 * The Object object's 'seal' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.8
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_object_seal (ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (arg);
} /* ecma_builtin_object_object_seal */

/**
 * The Object object's 'freeze' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.9
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_object_freeze (ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (arg);
} /* ecma_builtin_object_object_freeze */

/**
 * The Object object's 'preventExtensions' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.10
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_object_prevent_extensions (ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (arg);
} /* ecma_builtin_object_object_prevent_extensions */

/**
 * The Object object's 'isSealed' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.11
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_object_is_sealed (ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (arg);
} /* ecma_builtin_object_object_is_sealed */

/**
 * The Object object's 'isFrozen' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.12
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_object_is_frozen (ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (arg);
} /* ecma_builtin_object_object_is_frozen */

/**
 * The Object object's 'isExtensible' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.13
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_object_is_extensible (ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (arg);
} /* ecma_builtin_object_object_is_extensible */

/**
 * The Object object's 'keys' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.14
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_object_keys (ecma_value_t arg) /**< routine's argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (arg);
} /* ecma_builtin_object_object_keys */

/**
 * The Object object's 'getOwnPropertyDescriptor' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_object_get_own_property_descriptor (ecma_value_t arg1, /**< routine's first argument */
                                                        ecma_value_t arg2) /**< routine's second argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (arg1, arg2);
} /* ecma_builtin_object_object_get_own_property_descriptor */

/**
 * The Object object's 'create' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_object_create (ecma_value_t arg1, /**< routine's first argument */
                                   ecma_value_t arg2) /**< routine's second argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (arg1, arg2);
} /* ecma_builtin_object_object_create */

/**
 * The Object object's 'defineProperties' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.7
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_object_define_properties (ecma_value_t arg1, /**< routine's first argument */
                                              ecma_value_t arg2) /**< routine's second argument */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (arg1, arg2);
} /* ecma_builtin_object_object_define_properties */

/**
 * The Object object's 'defineProperty' routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.6
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_object_object_define_property (ecma_value_t arg1, /**< routine's first argument */
                                            ecma_value_t arg2, /**< routine's second argument */
                                            ecma_value_t arg3) /**< routine's third argument */
{
  ecma_completion_value_t ret_value;

  if (arg1.value_type != ECMA_TYPE_OBJECT)
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    ecma_object_t *obj_p = ECMA_GET_POINTER (arg1.value);

    ECMA_TRY_CATCH (name_str_value,
                    ecma_op_to_string (arg2),
                    ret_value);

    ecma_string_t *name_str_p = ECMA_GET_POINTER (name_str_value.u.value.value);

    ecma_property_descriptor_t prop_desc;

    ECMA_TRY_CATCH (conv_result,
                    ecma_op_to_property_descriptor (arg3, &prop_desc),
                    ret_value);

    ECMA_TRY_CATCH (define_own_prop_ret,
                    ecma_op_object_define_own_property (obj_p,
                                                        name_str_p,
                                                        prop_desc,
                                                        true),
                    ret_value);

    ret_value = ecma_make_return_completion_value (ecma_copy_value (arg1, true));

    ECMA_FINALIZE (define_own_prop_ret);
    ECMA_FINALIZE (conv_result);
    ECMA_FINALIZE (name_str_value);
  }

  return ret_value;
} /* ecma_builtin_object_object_define_property */

/**
 * Dispatcher of the Object object's built-in routines
 *
 * @return completion-value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_builtin_object_dispatch_routine (ecma_magic_string_id_t builtin_routine_id, /**< Object object's
                                                                                      built-in routine's name */
                                      ecma_value_t this_arg_value __unused, /**< 'this' argument value */
                                      ecma_value_t arguments_list [], /**< list of arguments passed to routine */
                                      ecma_length_t arguments_number) /**< length of arguments' list */
{
  const ecma_value_t value_undefined = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

  switch (builtin_routine_id)
  {
    case ECMA_MAGIC_STRING_GET_PROTOTYPE_OF_UL:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_object_object_get_prototype_of (arg);
    }

    case ECMA_MAGIC_STRING_GET_OWN_PROPERTY_NAMES_UL:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_object_object_get_own_property_names (arg);
    }

    case ECMA_MAGIC_STRING_SEAL:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_object_object_seal (arg);
    }

    case ECMA_MAGIC_STRING_FREEZE:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_object_object_freeze (arg);
    }

    case ECMA_MAGIC_STRING_PREVENT_EXTENSIONS_UL:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_object_object_prevent_extensions (arg);
    }

    case ECMA_MAGIC_STRING_IS_SEALED_UL:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_object_object_is_sealed (arg);
    }

    case ECMA_MAGIC_STRING_IS_FROZEN_UL:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_object_object_is_frozen (arg);
    }

    case ECMA_MAGIC_STRING_IS_EXTENSIBLE:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_object_object_is_extensible (arg);
    }

    case ECMA_MAGIC_STRING_KEYS:
    {
      ecma_value_t arg = (arguments_number >= 1 ? arguments_list[0] : value_undefined);

      return ecma_builtin_object_object_keys (arg);
    }

    case ECMA_MAGIC_STRING_GET_OWN_PROPERTY_DESCRIPTOR_UL:
    {
      ecma_value_t arg1 = (arguments_number >= 1 ? arguments_list[0] : value_undefined);
      ecma_value_t arg2 = (arguments_number >= 2 ? arguments_list[1] : value_undefined);

      return ecma_builtin_object_object_get_own_property_descriptor (arg1, arg2);
    }

    case ECMA_MAGIC_STRING_CREATE:
    {
      ecma_value_t arg1 = (arguments_number >= 1 ? arguments_list[0] : value_undefined);
      ecma_value_t arg2 = (arguments_number >= 2 ? arguments_list[1] : value_undefined);

      return ecma_builtin_object_object_create (arg1, arg2);
    }

    case ECMA_MAGIC_STRING_DEFINE_PROPERTIES_UL:
    {
      ecma_value_t arg1 = (arguments_number >= 1 ? arguments_list[0] : value_undefined);
      ecma_value_t arg2 = (arguments_number >= 2 ? arguments_list[1] : value_undefined);

      return ecma_builtin_object_object_define_properties (arg1, arg2);
    }

    case ECMA_MAGIC_STRING_DEFINE_PROPERTY_UL:
    {
      ecma_value_t arg1 = (arguments_number >= 1 ? arguments_list[0] : value_undefined);
      ecma_value_t arg2 = (arguments_number >= 2 ? arguments_list[1] : value_undefined);
      ecma_value_t arg3 = (arguments_number >= 3 ? arguments_list[2] : value_undefined);

      return ecma_builtin_object_object_define_property (arg1, arg2, arg3);
    }

    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* ecma_builtin_object_dispatch_routine */

/**
 * If the property's name is one of built-in properties of the Object object
 * that is not instantiated yet, instantiate the property and
 * return pointer to the instantiated property.
 *
 * @return pointer property, if one was instantiated,
 *         NULL - otherwise.
 */
ecma_property_t*
ecma_builtin_object_try_to_instantiate_property (ecma_object_t *obj_p, /**< object */
                                                 ecma_string_t *prop_name_p) /**< property's name */
{
  JERRY_ASSERT (ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_OBJECT));
  JERRY_ASSERT (ecma_find_named_property (obj_p, prop_name_p) == NULL);

  ecma_magic_string_id_t id;

  if (!ecma_is_string_magic (prop_name_p, &id))
  {
    return NULL;
  }

  int32_t index = ecma_builtin_bin_search_for_magic_string_id_in_array (ecma_builtin_object_property_names,
                                                                        ecma_builtin_object_property_number,
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
    case ECMA_MAGIC_STRING_GET_PROTOTYPE_OF_UL:
    case ECMA_MAGIC_STRING_GET_OWN_PROPERTY_NAMES_UL:
    case ECMA_MAGIC_STRING_SEAL:
    case ECMA_MAGIC_STRING_FREEZE:
    case ECMA_MAGIC_STRING_PREVENT_EXTENSIONS_UL:
    case ECMA_MAGIC_STRING_IS_SEALED_UL:
    case ECMA_MAGIC_STRING_IS_FROZEN_UL:
    case ECMA_MAGIC_STRING_IS_EXTENSIBLE:
    case ECMA_MAGIC_STRING_KEYS:
    case ECMA_MAGIC_STRING_GET_OWN_PROPERTY_DESCRIPTOR_UL:
    case ECMA_MAGIC_STRING_CREATE:
    case ECMA_MAGIC_STRING_DEFINE_PROPERTY_UL:
    case ECMA_MAGIC_STRING_DEFINE_PROPERTIES_UL:
    {
      ecma_object_t *func_obj_p = ecma_builtin_make_function_object_for_routine (ECMA_BUILTIN_ID_OBJECT,
                                                                                 id);

      value = ecma_make_object_value (func_obj_p);

      break;
    }
    case ECMA_MAGIC_STRING_PROTOTYPE:
    {
      JERRY_UNIMPLEMENTED ();

      writable = ECMA_PROPERTY_NOT_WRITABLE;
      enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;
      configurable = ECMA_PROPERTY_NOT_CONFIGURABLE;

      break;
    }
    case ECMA_MAGIC_STRING_LENGTH:
    {
      ecma_number_t *num_p = ecma_alloc_number ();
      *num_p = ECMA_NUMBER_ONE;

      value = ecma_make_number_value (num_p);

      writable = ECMA_PROPERTY_NOT_WRITABLE;
      enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;
      configurable = ECMA_PROPERTY_NOT_CONFIGURABLE;

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
} /* ecma_builtin_object_try_to_instantiate_property */

/**
 * @}
 * @}
 * @}
 */
