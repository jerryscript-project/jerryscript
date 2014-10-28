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
 * \addtogroup compact_profile_error ECMA CompactProfileError object built-in
 * @{
 */

/**
 * Number of the Boolean object's built-in properties
 */
const ecma_length_t ecma_builtin_compact_profile_error_property_number = 0;

/**
 * If the property's name is one of built-in properties of the CompactProfileError object
 * that is not instantiated yet, instantiate the property and
 * return pointer to the instantiated property.
 *
 * @return pointer property, if one was instantiated,
 *         NULL - otherwise.
 */
ecma_property_t*
ecma_builtin_compact_profile_error_try_to_instantiate_property (ecma_object_t *obj_p, /**< object */
                                                              ecma_string_t *prop_name_p) /**< property's name */
{
  JERRY_ASSERT (ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_COMPACT_PROFILE_ERROR));
  JERRY_ASSERT (ecma_find_named_property (obj_p, prop_name_p) == NULL);

  return NULL;
} /* ecma_builtin_compact_profile_error_try_to_instantiate_property */

/**
 * Dispatcher of the CompactProfileError object's built-in routines
 *
 * @return completion-value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_builtin_compact_profile_error_dispatch_routine (ecma_magic_string_id_t builtin_routine_id __unused,
                                                   ecma_value_t this_arg_value __unused,
                                                   ecma_value_t arguments_list [] __unused,
                                                   ecma_length_t arguments_compact_profile_error __unused)
{
  JERRY_UNREACHABLE ();
} /* ecma_builtin_compact_profile_error_dispatch_routine */

/**
 * Handle calling [[Call]] of built-in CompactProfileError object
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_compact_profile_error_dispatch_call (ecma_value_t *arguments_list_p, /**< arguments list */
                                                ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_make_throw_obj_completion_value (ecma_builtin_get (ECMA_BUILTIN_ID_COMPACT_PROFILE_ERROR));
} /* ecma_builtin_compact_profile_error_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in CompactProfileError object
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_compact_profile_error_dispatch_construct (ecma_value_t *arguments_list_p, /**< arguments list */
                                                     ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_make_throw_obj_completion_value (ecma_builtin_get (ECMA_BUILTIN_ID_COMPACT_PROFILE_ERROR));
} /* ecma_builtin_compact_profile_error_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */
