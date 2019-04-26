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

#include "ecma-container-object.h"

#if ENABLED (JERRY_ES2015_BUILTIN_SET)

#if !ENABLED (JERRY_ES2015_BUILTIN_MAP)
#error "Set builtin requires ES2015 map builtin"
#endif /* !ENABLED (JERRY_ES2015_BUILTIN_MAP) */

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
                                       ecma_value_t value_arg) /**< value argument */
{
  return ecma_op_container_set (this_arg, value_arg, value_arg, true);
} /* ecma_builtin_set_prototype_object_add */

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
  return ecma_op_container_clear (this_arg, true);
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
                                          ecma_value_t value_arg) /**< value argument */
{
  return ecma_op_container_delete (this_arg, value_arg, true);
} /* ecma_builtin_set_prototype_object_delete */

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
ecma_builtin_set_prototype_object_foreach (ecma_value_t this_arg, /**< this argument */
                                           ecma_value_t predicate, /**< callback function */
                                           ecma_value_t predicate_this_arg) /**< this argument for
                                                                             *   invoke predicate */
{
  return ecma_op_container_foreach (this_arg, predicate, predicate_this_arg, true);
} /* ecma_builtin_set_prototype_object_foreach */

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
                                       ecma_value_t value_arg) /**< value argument */
{
  return ecma_op_container_has (this_arg, value_arg, true);
} /* ecma_builtin_set_prototype_object_has */

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
  return ecma_op_container_size (this_arg, true);
} /* ecma_builtin_set_prototype_object_size_getter */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_ES2015_BUILTIN_MAP) */
