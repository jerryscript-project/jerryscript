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

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-error.inc.h"
#define BUILTIN_UNDERSCORED_ID error
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup error ECMA Error object built-in
 * @{
 */

/**
 * Handle calling [[Call]] of built-in Error object
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_error_dispatch_call (ecma_value_t *arguments_list_p, /**< arguments list */
                                  ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_completion_value_t msg_to_str_completion = ecma_make_empty_completion_value ();;

  if (arguments_list_len != 0
      && !ecma_is_value_undefined (arguments_list_p [0]))
  {
    msg_to_str_completion = ecma_op_to_string (arguments_list_p[0]);

    if (!ecma_is_completion_value_normal (msg_to_str_completion))
    {
      return msg_to_str_completion;
    }
  }

  ecma_object_t *error_prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_ERROR_PROTOTYPE);

  ecma_object_t *new_error_object_p = ecma_create_object (error_prototype_obj_p,
                                                          true,
                                                          ECMA_OBJECT_TYPE_GENERAL);

  ecma_deref_object (error_prototype_obj_p);

  ecma_property_t *class_prop_p = ecma_create_internal_property (new_error_object_p,
                                                                 ECMA_INTERNAL_PROPERTY_CLASS);
  class_prop_p->u.internal_property.value = ECMA_MAGIC_STRING_ERROR_UL;

  if (!ecma_is_completion_value_empty (msg_to_str_completion))
  {
    ecma_string_t *message_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_MESSAGE);

    ecma_property_t *prop_p = ecma_create_named_data_property (new_error_object_p,
                                                               message_magic_string_p,
                                                               ECMA_PROPERTY_WRITABLE,
                                                               ECMA_PROPERTY_NOT_ENUMERABLE,
                                                               ECMA_PROPERTY_CONFIGURABLE);
    prop_p->u.named_data_property.value = ecma_copy_value (msg_to_str_completion.u.value, true);
    ecma_gc_update_may_ref_younger_object_flag_by_value (new_error_object_p, prop_p->u.named_data_property.value);
    
    ecma_deref_ecma_string (message_magic_string_p);

    ecma_free_completion_value (msg_to_str_completion);
  }

  return ecma_make_normal_completion_value (ecma_make_object_value (new_error_object_p));
} /* ecma_builtin_error_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in Error object
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_error_dispatch_construct (ecma_value_t *arguments_list_p, /**< arguments list */
                                       ecma_length_t arguments_list_len) /**< number of arguments */
{
  return ecma_builtin_error_dispatch_call (arguments_list_p, arguments_list_len);
} /* ecma_builtin_error_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */
