/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_NUMBER_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-number-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID number_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

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
 * The Number.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.7.4.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static void
ecma_builtin_number_prototype_object_to_string (ecma_completion_value_t &ret_value, /**< out: completion value */
                                                const ecma_value_t& this_arg, /**< this argument */
                                                const ecma_value_t* arguments_list_p, /**< arguments list */
                                                ecma_length_t arguments_list_len) /**< number of arguments */
{
  ecma_number_t this_arg_number;

  if (ecma_is_value_number (this_arg))
  {
    ecma_number_t *this_arg_number_p = ecma_get_number_from_value (this_arg);

    this_arg_number = *this_arg_number_p;
  }
  else if (ecma_is_value_object (this_arg))
  {
    ecma_object_ptr_t obj_p;
    ecma_get_object_from_value (obj_p, this_arg);

    ecma_property_t *class_prop_p = ecma_get_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_CLASS);

    if (class_prop_p->u.internal_property.value == ECMA_MAGIC_STRING_NUMBER_UL)
    {
      ecma_property_t *prim_value_prop_p = ecma_get_internal_property (obj_p,
                                                                       ECMA_INTERNAL_PROPERTY_PRIMITIVE_NUMBER_VALUE);

      ecma_number_t *prim_value_num_p = ECMA_GET_NON_NULL_POINTER (ecma_number_t,
                                                                   prim_value_prop_p->u.internal_property.value);
      this_arg_number = *prim_value_num_p;
    }
    else
    {
      ecma_object_ptr_t exception_obj_p;
      ecma_new_standard_error (exception_obj_p, ECMA_ERROR_TYPE);
      ecma_make_throw_obj_completion_value (ret_value, exception_obj_p);
      return;
    }
  }
  else
  {
    ecma_object_ptr_t exception_obj_p;
    ecma_new_standard_error (exception_obj_p, ECMA_ERROR_TYPE);
    ecma_make_throw_obj_completion_value (ret_value, exception_obj_p);
    return;
  }

  if (arguments_list_len == 0)
  {
    ecma_string_t *ret_str_p = ecma_new_ecma_string_from_number (this_arg_number);

    ecma_make_normal_completion_value (ret_value, ecma_value_t (ret_str_p));
  }
  else
  {
    ECMA_BUILTIN_CP_UNIMPLEMENTED (ret_value, arguments_list_p);
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
static void
ecma_builtin_number_prototype_object_to_locale_string (ecma_completion_value_t &ret_value, /**< out: completion value */
                                                       const ecma_value_t& this_arg) /**< this argument */
{
  ecma_builtin_number_prototype_object_to_string (ret_value, this_arg, NULL, 0);
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
static void
ecma_builtin_number_prototype_object_value_of (ecma_completion_value_t &ret_value, /**< out: completion value */
                                               const ecma_value_t& this_arg) /**< this argument */
{
  if (ecma_is_value_number (this_arg))
  {
    ecma_value_t this_arg_copy;
    ecma_copy_value (this_arg_copy, this_arg, true);

    ecma_make_normal_completion_value (ret_value, this_arg_copy);
    return;
  }
  else if (ecma_is_value_object (this_arg))
  {
    ecma_object_ptr_t obj_p;
    ecma_get_object_from_value (obj_p, this_arg);

    ecma_property_t *class_prop_p = ecma_get_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_CLASS);

    if (class_prop_p->u.internal_property.value == ECMA_MAGIC_STRING_NUMBER_UL)
    {
      ecma_property_t *prim_value_prop_p = ecma_get_internal_property (obj_p,
                                                                       ECMA_INTERNAL_PROPERTY_PRIMITIVE_NUMBER_VALUE);

      ecma_number_t *prim_value_num_p = ECMA_GET_NON_NULL_POINTER (ecma_number_t,
                                                                   prim_value_prop_p->u.internal_property.value);

      ecma_number_t *ret_num_p = ecma_alloc_number ();
      *ret_num_p = *prim_value_num_p;

      ecma_make_normal_completion_value (ret_value, ecma_value_t (ret_num_p));
      return;
    }
  }

  ecma_object_ptr_t exception_obj_p;
  ecma_new_standard_error (exception_obj_p, ECMA_ERROR_TYPE);
  ecma_make_throw_obj_completion_value (ret_value, exception_obj_p);
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
static void
ecma_builtin_number_prototype_object_to_fixed (ecma_completion_value_t &ret_value, /**< out: completion value */
                                               const ecma_value_t& this_arg, /**< this argument */
                                               const ecma_value_t& arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (ret_value, this_arg, arg);
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
static void
ecma_builtin_number_prototype_object_to_exponential (ecma_completion_value_t &ret_value, /**< out: completion value */
                                                     const ecma_value_t& this_arg, /**< this argument */
                                                     const ecma_value_t& arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (ret_value, this_arg, arg);
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
static void
ecma_builtin_number_prototype_object_to_precision (ecma_completion_value_t &ret_value, /**< out: completion value */
                                                   const ecma_value_t& this_arg, /**< this argument */
                                                   const ecma_value_t& arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (ret_value, this_arg, arg);
} /* ecma_builtin_number_prototype_object_to_precision */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_NUMBER_BUILTIN */
