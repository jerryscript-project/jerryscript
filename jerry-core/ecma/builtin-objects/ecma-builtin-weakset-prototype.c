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

#if ENABLED (JERRY_ES2015_BUILTIN_WEAKSET)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-weakset-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID weakset_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup weakset ECMA WeakSet object built-in
 * @{
 */

/**
 * The WeakSet.prototype object's 'add' routine
 *
 * See also:
 *          ECMA-262 v6, 23.4.3.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_weakset_prototype_object_add (ecma_value_t this_arg, /**< this argument */
                                           ecma_value_t value_arg) /**< value argument */
{
  return ecma_op_container_set (this_arg, value_arg, ECMA_VALUE_UNDEFINED, LIT_MAGIC_STRING_WEAKSET_UL);
} /* ecma_builtin_weakset_prototype_object_add */

/**
 * The WeakSet.prototype object's 'delete' routine
 *
 * See also:
 *          ECMA-262 v6, 23.4.3.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_weakset_prototype_object_delete (ecma_value_t this_arg, /**< this argument */
                                              ecma_value_t value_arg) /**< value argument */
{
  return ecma_op_container_delete_weak (this_arg, value_arg, LIT_MAGIC_STRING_WEAKSET_UL);
} /* ecma_builtin_weakset_prototype_object_delete */

/**
 * The WeakSet.prototype object's 'has' routine
 *
 * See also:
 *          ECMA-262 v6, 23.4.3.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_weakset_prototype_object_has (ecma_value_t this_arg, /**< this argument */
                                           ecma_value_t value_arg) /**< value argument */
{
  return ecma_op_container_has (this_arg, value_arg, LIT_MAGIC_STRING_WEAKSET_UL);
} /* ecma_builtin_weakset_prototype_object_has */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_ES2015_BUILTIN_WEAKSET) */
