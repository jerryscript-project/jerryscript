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

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-function-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID function_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup functionprototype ECMA Function.prototype object built-in
 * @{
 */

/**
 * The Function.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.3.4.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static void
ecma_builtin_function_prototype_object_to_string (ecma_completion_value_t &ret_value, /**< out: completion value */
                                                  const ecma_value_t& this_arg) /**< this argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (ret_value, this_arg);
} /* ecma_builtin_function_prototype_object_to_string */

/**
 * The Function.prototype object's 'apply' routine
 *
 * See also:
 *          ECMA-262 v5, 15.3.4.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static void
ecma_builtin_function_prototype_object_apply (ecma_completion_value_t &ret_value, /**< out: completion value */
                                              const ecma_value_t& this_arg, /**< this argument */
                                              const ecma_value_t& arg1, /**< first argument */
                                              const ecma_value_t& arg2) /**< second argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (ret_value, this_arg, arg1, arg2);
} /* ecma_builtin_function_prototype_object_apply */

/**
 * The Function.prototype object's 'call' routine
 *
 * See also:
 *          ECMA-262 v5, 15.3.4.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static void
ecma_builtin_function_prototype_object_call (ecma_completion_value_t &ret_value, /**< out: completion value */
                                             const ecma_value_t& this_arg, /**< this argument */
                                             const ecma_value_t* arguments_list_p, /**< list of arguments */
                                             ecma_length_t arguments_number) /**< number of arguments */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (ret_value, this_arg, arguments_list_p, arguments_number);
} /* ecma_builtin_function_prototype_object_call */

/**
 * The Function.prototype object's 'bind' routine
 *
 * See also:
 *          ECMA-262 v5, 15.3.4.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static void
ecma_builtin_function_prototype_object_bind (ecma_completion_value_t &ret_value, /**< out: completion value */
                                             const ecma_value_t& this_arg, /**< this argument */
                                             const ecma_value_t* arguments_list_p, /**< list of arguments */
                                             ecma_length_t arguments_number) /**< number of arguments */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (ret_value, this_arg, arguments_list_p, arguments_number);
} /* ecma_builtin_function_prototype_object_bind */

/**
 * Handle calling [[Call]] of built-in Function.prototype object
 *
 * @return completion-value
 */
void
ecma_builtin_function_prototype_dispatch_call (ecma_completion_value_t &ret_value, /**< out: completion value */
                                               const ecma_value_t* arguments_list_p, /**< arguments list */
                                               ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_make_simple_completion_value (ret_value, ECMA_SIMPLE_VALUE_UNDEFINED);
} /* ecma_builtin_function_prototype_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in Function.prototype object
 *
 * @return completion-value
 */
void
ecma_builtin_function_prototype_dispatch_construct (ecma_completion_value_t &ret_value, /**< out: completion value */
                                                    const ecma_value_t *arguments_list_p, /**< arguments list */
                                                    ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_object_ptr_t exception_obj_p;
  ecma_new_standard_error (exception_obj_p, ECMA_ERROR_TYPE);
  ecma_make_throw_obj_completion_value (ret_value, exception_obj_p);
} /* ecma_builtin_function_prototype_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */
