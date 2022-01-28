/* Copyright JS Foundation and other contributors, http://js.foundation
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
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"

#include "jcontext.h"
#include "jrt.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-error.inc.h"
#define BUILTIN_UNDERSCORED_ID  error
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
 * @return ecma value
 */
ecma_value_t
ecma_builtin_error_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                  uint32_t arguments_list_len) /**< number of arguments */
{
  return ecma_builtin_helper_error_dispatch_call (JERRY_ERROR_COMMON, arguments_list_p, arguments_list_len);
} /* ecma_builtin_error_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in Error object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_error_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                       uint32_t arguments_list_len) /**< number of arguments */
{
  ecma_object_t *proto_p =
    ecma_op_get_prototype_from_constructor (JERRY_CONTEXT (current_new_target_p), ECMA_BUILTIN_ID_ERROR_PROTOTYPE);

  if (proto_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t result = ecma_builtin_error_dispatch_call (arguments_list_p, arguments_list_len);

  if (!ECMA_IS_VALUE_ERROR (result))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (result);
    ECMA_SET_NON_NULL_POINTER (object_p->u2.prototype_cp, proto_p);
  }

  ecma_deref_object (proto_p);

  return result;
} /* ecma_builtin_error_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */
