/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged.
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
#include "jrt.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-object.inc.h"
#define BUILTIN_UNDERSCORED_ID object
#include "ecma-builtin-internal-routines-template.inc.h"

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
 * Handle calling [[Call]] of built-in Object object
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_object_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                   ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

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
ecma_builtin_object_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
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
      return ecma_make_normal_completion_value (ecma_get_completion_value_value (new_obj_value));
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
ecma_builtin_object_object_get_prototype_of (ecma_value_t this_arg, /**< 'this' argument */
                                             ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
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
ecma_builtin_object_object_get_own_property_names (ecma_value_t this_arg, /**< 'this' argument */
                                                   ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
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
ecma_builtin_object_object_seal (ecma_value_t this_arg, /**< 'this' argument */
                                 ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
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
ecma_builtin_object_object_freeze (ecma_value_t this_arg, /**< 'this' argument */
                                   ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
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
ecma_builtin_object_object_prevent_extensions (ecma_value_t this_arg, /**< 'this' argument */
                                               ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
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
ecma_builtin_object_object_is_sealed (ecma_value_t this_arg, /**< 'this' argument */
                                      ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
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
ecma_builtin_object_object_is_frozen (ecma_value_t this_arg, /**< 'this' argument */
                                      ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
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
ecma_builtin_object_object_is_extensible (ecma_value_t this_arg, /**< 'this' argument */
                                          ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
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
ecma_builtin_object_object_keys (ecma_value_t this_arg, /**< 'this' argument */
                                 ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
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
ecma_builtin_object_object_get_own_property_descriptor (ecma_value_t this_arg, /**< 'this' argument */
                                                        ecma_value_t arg1, /**< routine's first argument */
                                                        ecma_value_t arg2) /**< routine's second argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg1, arg2);
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
ecma_builtin_object_object_create (ecma_value_t this_arg, /**< 'this' argument */
                                   ecma_value_t arg1, /**< routine's first argument */
                                   ecma_value_t arg2) /**< routine's second argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg1, arg2);
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
ecma_builtin_object_object_define_properties (ecma_value_t this_arg __attr_unused___, /**< 'this' argument */
                                              ecma_value_t arg1, /**< routine's first argument */
                                              ecma_value_t arg2) /**< routine's second argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (!ecma_is_value_object (arg1))
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    return ret_value;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (arg1);
  ecma_object_t *props_p = ecma_get_object_from_value (arg2);

  ecma_property_t *property_p;

  for (property_p = ecma_get_property_list (props_p);
       property_p != NULL;
       property_p = ECMA_GET_POINTER (ecma_property_t, property_p->next_property_p))
  {
    ecma_string_t *property_name_p;

    if (property_p->type == ECMA_PROPERTY_NAMEDDATA)
    {
      property_name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                   property_p->u.named_data_property.name_p);
    }
    else if (property_p->type == ECMA_PROPERTY_NAMEDACCESSOR)
    {
      property_name_p = ECMA_GET_NON_NULL_POINTER(ecma_string_t,
                                                  property_p->u.named_accessor_property.name_p);
    }
    else
    {
      continue;
    }
    ecma_property_descriptor_t prop_desc;

    ECMA_TRY_CATCH (descObj,
                    ecma_op_general_object_get (props_p, property_name_p),
                    ret_value);

    ECMA_TRY_CATCH (conv_result,
                    ecma_op_to_property_descriptor (ecma_get_completion_value_value (descObj),
                                                    &prop_desc),
                    ret_value);

    ECMA_TRY_CATCH (define_own_prop_ret,
                    ecma_op_object_define_own_property (obj_p,
                                                        property_name_p,
                                                        &prop_desc,
                                                        true),
                    ret_value);

    ECMA_FINALIZE (define_own_prop_ret);
    ecma_free_property_descriptor (&prop_desc);
    ECMA_FINALIZE (conv_result);
    ECMA_FINALIZE (descObj);

    if (ecma_is_completion_value_throw (ret_value))
    {
      return ret_value;
    }
  }

  ret_value = ecma_make_normal_completion_value (ecma_copy_value (arg1, true));

  return ret_value;
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
ecma_builtin_object_object_define_property (ecma_value_t this_arg __attr_unused___, /**< 'this' argument */
                                            ecma_value_t arg1, /**< routine's first argument */
                                            ecma_value_t arg2, /**< routine's second argument */
                                            ecma_value_t arg3) /**< routine's third argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (!ecma_is_value_object (arg1))
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (arg1);

    ECMA_TRY_CATCH (name_str_value,
                    ecma_op_to_string (arg2),
                    ret_value);

    ecma_string_t *name_str_p = ecma_get_string_from_value (name_str_value);

    ecma_property_descriptor_t prop_desc;

    ECMA_TRY_CATCH (conv_result,
                    ecma_op_to_property_descriptor (arg3, &prop_desc),
                    ret_value);

    ECMA_TRY_CATCH (define_own_prop_ret,
                    ecma_op_object_define_own_property (obj_p,
                                                        name_str_p,
                                                        &prop_desc,
                                                        true),
                    ret_value);

    ret_value = ecma_make_normal_completion_value (ecma_copy_value (arg1, true));

    ECMA_FINALIZE (define_own_prop_ret);
    ecma_free_property_descriptor (&prop_desc);
    ECMA_FINALIZE (conv_result);
    ECMA_FINALIZE (name_str_value);
  }

  return ret_value;
} /* ecma_builtin_object_object_define_property */

/**
 * @}
 * @}
 * @}
 */
