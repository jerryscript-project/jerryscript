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

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-string-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID string_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup stringprototype ECMA String.prototype object built-in
 * @{
 */

/**
 * The String.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_to_string (ecma_value_t this) /**< this argument */
{
  if (this.value_type == ECMA_TYPE_STRING)
  {
    return ecma_make_normal_completion_value (ecma_copy_value (this, true));
  }
  else if (this.value_type == ECMA_TYPE_OBJECT)
  {
    ecma_object_t *obj_p = ECMA_GET_POINTER (this.value);

    ecma_property_t *class_prop_p = ecma_get_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_CLASS);

    if (class_prop_p->u.internal_property.value == ECMA_MAGIC_STRING_STRING_UL)
    {
      ecma_property_t *prim_value_prop_p = ecma_get_internal_property (obj_p,
                                                                       ECMA_INTERNAL_PROPERTY_PRIMITIVE_STRING_VALUE);

      ecma_string_t *prim_value_str_p = ECMA_GET_POINTER (prim_value_prop_p->u.internal_property.value);

      prim_value_str_p = ecma_copy_or_ref_ecma_string (prim_value_str_p);

      return ecma_make_normal_completion_value (ecma_make_string_value (prim_value_str_p));
    }
  }

  return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
} /* ecma_builtin_string_prototype_object_to_string */

/**
 * The String.prototype object's 'valueOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_value_of (ecma_value_t this) /**< this argument */
{
  return ecma_builtin_string_prototype_object_to_string (this);
} /* ecma_builtin_string_prototype_object_value_of */

/**
 * The String.prototype object's 'charAt' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_char_at (ecma_value_t this, /**< this argument */
                                              ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this, arg);
} /* ecma_builtin_string_prototype_object_char_at */

/**
 * The String.prototype object's 'charCodeAt' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_char_code_at (ecma_value_t this, /**< this argument */
                                                   ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this, arg);
} /* ecma_builtin_string_prototype_object_char_code_at */

/**
 * The String.prototype object's 'concat' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.6
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_concat (ecma_value_t this, /**< this argument */
                                             ecma_value_t* argument_list_p, /**< arguments list */
                                             ecma_length_t arguments_number) /**< number of arguments */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this, argument_list_p, arguments_number);
} /* ecma_builtin_string_prototype_object_concat */

/**
 * The String.prototype object's 'indexOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.7
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_index_of (ecma_value_t this, /**< this argument */
                                               ecma_value_t arg1, /**< routine's first argument */
                                               ecma_value_t arg2) /**< routine's second argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this, arg1, arg2);
} /* ecma_builtin_string_prototype_object_index_of */

/**
 * The String.prototype object's 'lastIndexOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.8
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_last_index_of (ecma_value_t this, /**< this argument */
                                                    ecma_value_t arg1, /**< routine's first argument */
                                                    ecma_value_t arg2) /**< routine's second argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this, arg1, arg2);
} /* ecma_builtin_string_prototype_object_last_index_of */

/**
 * The String.prototype object's 'localeCompare' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.9
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_locale_compare (ecma_value_t this, /**< this argument */
                                                     ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this, arg);
} /* ecma_builtin_string_prototype_object_locale_compare */

/**
 * The String.prototype object's 'match' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.10
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_match (ecma_value_t this, /**< this argument */
                                            ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this, arg);
} /* ecma_builtin_string_prototype_object_match */

/**
 * The String.prototype object's 'replace' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.11
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_replace (ecma_value_t this, /**< this argument */
                                              ecma_value_t arg1, /**< routine's first argument */
                                              ecma_value_t arg2) /**< routine's second argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this, arg1, arg2);
} /* ecma_builtin_string_prototype_object_replace */

/**
 * The String.prototype object's 'search' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.12
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_search (ecma_value_t this, /**< this argument */
                                             ecma_value_t arg) /**< routine's argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this, arg);
} /* ecma_builtin_string_prototype_object_search */

/**
 * The String.prototype object's 'slice' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.13
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_slice (ecma_value_t this, /**< this argument */
                                            ecma_value_t arg1, /**< routine's first argument */
                                            ecma_value_t arg2) /**< routine's second argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this, arg1, arg2);
} /* ecma_builtin_string_prototype_object_slice */

/**
 * The String.prototype object's 'split' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.14
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_split (ecma_value_t this, /**< this argument */
                                            ecma_value_t arg1, /**< routine's first argument */
                                            ecma_value_t arg2) /**< routine's second argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this, arg1, arg2);
} /* ecma_builtin_string_prototype_object_split */

/**
 * The String.prototype object's 'substring' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.15
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_substring (ecma_value_t this, /**< this argument */
                                                ecma_value_t arg1, /**< routine's first argument */
                                                ecma_value_t arg2) /**< routine's second argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this, arg1, arg2);
} /* ecma_builtin_string_prototype_object_substring */

/**
 * The String.prototype object's 'toLowerCase' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.16
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_to_lower_case (ecma_value_t this) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this);
} /* ecma_builtin_string_prototype_object_to_lower_case */

/**
 * The String.prototype object's 'toLocaleLowerCase' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.17
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_to_locale_lower_case (ecma_value_t this) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this);
} /* ecma_builtin_string_prototype_object_to_locale_lower_case */

/**
 * The String.prototype object's 'toUpperCase' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.18
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_to_upper_case (ecma_value_t this) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this);
} /* ecma_builtin_string_prototype_object_to_upper_case */

/**
 * The String.prototype object's 'toLocaleUpperCase' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.19
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_to_locale_upper_case (ecma_value_t this) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this);
} /* ecma_builtin_string_prototype_object_to_locale_upper_case */

/**
 * The String.prototype object's 'trim' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.20
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_prototype_object_trim (ecma_value_t this) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this);
} /* ecma_builtin_string_prototype_object_trim */

/**
 * @}
 * @}
 * @}
 */
