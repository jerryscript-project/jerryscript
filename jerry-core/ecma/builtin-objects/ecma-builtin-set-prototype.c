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

#include "ecma-set-object.h"
#include "ecma-function-object.h"
#include "ecma-exceptions.h"


#ifndef CONFIG_DISABLE_ES2015_SET_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-set-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID set_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup set ECMA Set object built-in
 * @{
 */

/**
 * The Set.prototype object's 'clear' routine
 *
 * See also:
 *          ECMA-262 v6, 23.2.3.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_set_prototype_object_clear (ecma_value_t this_arg) /**< this argument */
{
  return ecma_op_set_clear (this_arg);
} /* ecma_builtin_set_prototype_object_clear */

/**
 * The Set.prototype object's 'delete' routine
 *
 * See also:
 *          ECMA-262 v6, 23.2.3.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_set_prototype_object_delete (ecma_value_t this_arg, /**< this argument */
                                          ecma_value_t value) /**< value to delete */
{
  return ecma_op_set_delete (this_arg, value);
} /* ecma_builtin_set_prototype_object_delete */

/**
 * The Set.prototype object's 'has' routine
 *
 * See also:
 *          ECMA-262 v6, 23.2.3.7
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_set_prototype_object_has (ecma_value_t this_arg, /**< this argument */
                                       ecma_value_t value) /**< value to check */
{
  return ecma_op_set_has (this_arg, value);
} /* ecma_builtin_set_prototype_object_has */

/**
 * The Set.prototype object's 'add' routine
 *
 * See also:
 *          ECMA-262 v6, 23.2.3.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_set_prototype_object_add (ecma_value_t this_arg, /**< this argument */
                                       ecma_value_t value) /**< value to add */
{
  return ecma_op_set_add (this_arg, value);
} /* ecma_builtin_set_prototype_object_add */

/**
 * The Set.prototype object's 'size' getter
 *
 * See also:
 *          ECMA-262 v6, 23.2.3.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_set_prototype_object_size_getter (ecma_value_t this_arg) /**< this argument */
{
  return ecma_op_set_size (this_arg);
} /* ecma_builtin_set_prototype_object_size_getter */

/**
 * The Set.prototype object's 'forEach' routine
 *
 * See also:
 *          ECMA-262 v6, 23.2.3.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_set_prototype_object_for_each (ecma_value_t this_arg, /**< this argument */
                                            ecma_value_t callbackfn, /**< callbackfn */
                                            ecma_value_t callbackfn_this_args) /**< thisArg */
{
  ecma_set_object_t *set_object_p = ecma_op_set_get_object (this_arg);
  if (set_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  if (!ecma_op_is_callable (callbackfn))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
  }

  JERRY_ASSERT (ecma_is_value_object (callbackfn));
  ecma_object_t *func_object_p = ecma_get_object_from_value (callbackfn);

  if (JERRY_UNLIKELY (set_object_p->first_chunk_cp == ECMA_NULL_POINTER))
  {
    return ECMA_VALUE_UNDEFINED;
  }

  ecma_set_object_chunk_t *chunk_p = ECMA_GET_NON_NULL_POINTER (ecma_set_object_chunk_t,
                                                                set_object_p->first_chunk_cp);


  ecma_value_t *ecma_value_p = chunk_p->items;

  while (ecma_value_p != NULL)
  {
    ecma_value_t call_args[] = { *ecma_value_p, *ecma_value_p, this_arg };

    ecma_value_t call_value = ecma_op_function_call (func_object_p, callbackfn_this_args, call_args, 3);

    if (ECMA_IS_VALUE_ERROR (call_value))
    {
      return call_value;
    }

    ecma_value_p = ecma_collection_iterator_next (ecma_value_p);
  }

  return ECMA_VALUE_UNDEFINED;
} /* ecma_builtin_array_prototype_object_for_each */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_ES2015_SET_BUILTIN */
