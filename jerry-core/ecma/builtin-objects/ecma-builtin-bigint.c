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

#include "ecma-bigint.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"

#if ENABLED (JERRY_BUILTIN_BIGINT)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-bigint.inc.h"
#define BUILTIN_UNDERSCORED_ID bigint
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup bigint ECMA BigInt object built-in
 * @{
 */

/**
 * Handle calling [[Call]] of built-in BigInt object
 *
 * See also:
 *          ECMA-262 v11, 20.2.1.1
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_bigint_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                   uint32_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_value_t value = (arguments_list_len == 0) ? ECMA_VALUE_UNDEFINED : arguments_list_p[0];
  bool free_value = false;

  if (ecma_is_value_object (value))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (value);
    value = ecma_op_object_default_value (obj_p, ECMA_PREFERRED_TYPE_NUMBER);
    free_value = true;

    if (ECMA_IS_VALUE_ERROR (value))
    {
      return value;
    }
  }

  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  if (ecma_is_value_number (value))
  {
    ret_value = ecma_bigint_number_to_bigint (ecma_get_number_from_value (value));
  }
  else
  {
    ret_value = ecma_bigint_to_bigint (value);
  }

  if (free_value)
  {
    ecma_free_value (value);
  }

  return ret_value;
} /* ecma_builtin_bigint_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in BigInt object
 *
 * See also:
 *          ECMA-262 v11, 20.2.1
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_bigint_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                        uint32_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_raise_type_error (ECMA_ERR_MSG ("BigInt function is not a constructor."));
} /* ecma_builtin_bigint_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_BUILTIN_BIGINT) */
