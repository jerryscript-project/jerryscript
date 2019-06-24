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

#if ENABLED (JERRY_ES2015_BUILTIN_MAP)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-map-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID map_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup map ECMA Map object built-in
 * @{
 */

/**
 * The Map.prototype object's 'clear' routine
 *
 * See also:
 *          ECMA-262 v6, 23.1.3.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_map_prototype_object_clear (ecma_value_t this_arg) /**< this argument */
{
  return ecma_op_container_clear (this_arg, LIT_MAGIC_STRING_MAP_UL);
} /* ecma_builtin_map_prototype_object_clear */

/**
 * The Map.prototype object's 'delete' routine
 *
 * See also:
 *          ECMA-262 v6, 23.1.3.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_map_prototype_object_delete (ecma_value_t this_arg, /**< this argument */
                                          ecma_value_t key_arg) /**< key argument */
{
  return ecma_op_container_delete (this_arg, key_arg, LIT_MAGIC_STRING_MAP_UL);
} /* ecma_builtin_map_prototype_object_delete */

/**
 * The Map.prototype object's 'forEach' routine
 *
 * See also:
 *          ECMA-262 v6, 23.1.3.5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_map_prototype_object_foreach (ecma_value_t this_arg, /**< this argument */
                                           ecma_value_t predicate, /**< callback function */
                                           ecma_value_t predicate_this_arg) /**< this argument for
                                                                             *   invoke predicate */
{
  return ecma_op_container_foreach (this_arg, predicate, predicate_this_arg, LIT_MAGIC_STRING_MAP_UL);
} /* ecma_builtin_map_prototype_object_foreach */

/**
 * The Map.prototype object's 'get' routine
 *
 * See also:
 *          ECMA-262 v6, 23.1.3.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_map_prototype_object_get (ecma_value_t this_arg, /**< this argument */
                                       ecma_value_t key_arg) /**< key argument */
{
  return ecma_op_container_get (this_arg, key_arg);
} /* ecma_builtin_map_prototype_object_get */

/**
 * The Map.prototype object's 'has' routine
 *
 * See also:
 *          ECMA-262 v6, 23.1.3.7
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_map_prototype_object_has (ecma_value_t this_arg, /**< this argument */
                                       ecma_value_t key_arg) /**< key argument */
{
  return ecma_op_container_has (this_arg, key_arg, LIT_MAGIC_STRING_MAP_UL);
} /* ecma_builtin_map_prototype_object_has */

/**
 * The Map.prototype object's 'set' routine
 *
 * See also:
 *          ECMA-262 v6, 23.1.3.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_map_prototype_object_set (ecma_value_t this_arg, /**< this argument */
                                       ecma_value_t key_arg, /**< key argument */
                                       ecma_value_t value_arg) /**< value argument */
{
  return ecma_op_container_set (this_arg, key_arg, value_arg, LIT_MAGIC_STRING_MAP_UL);
} /* ecma_builtin_map_prototype_object_set */

/**
 * The Map.prototype object's 'size' getter
 *
 * See also:
 *          ECMA-262 v6, 23.1.3.10
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_map_prototype_object_size_getter (ecma_value_t this_arg) /**< this argument */
{
  return ecma_op_container_size (this_arg, LIT_MAGIC_STRING_MAP_UL);
} /* ecma_builtin_map_prototype_object_size_getter */

#if ENABLED (JERRY_ES2015_BUILTIN_ITERATOR)
/**
 * The Map.prototype object's 'entries' routine
 *
 * See also:
 *          ECMA-262 v6, 23.1.3.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_map_prototype_object_entries (ecma_value_t this_arg) /**< this argument */
{
  return ecma_op_container_create_iterator (this_arg,
                                            ECMA_ITERATOR_KEYS_VALUES,
                                            LIT_MAGIC_STRING_MAP_UL,
                                            ECMA_BUILTIN_ID_MAP_ITERATOR_PROTOTYPE,
                                            ECMA_PSEUDO_MAP_ITERATOR);
} /* ecma_builtin_map_prototype_object_entries */

/**
 * The Map.prototype object's 'keys' routine
 *
 * See also:
 *          ECMA-262 v6, 23.1.3.8
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_map_prototype_object_keys (ecma_value_t this_arg) /**< this argument */
{
  return ecma_op_container_create_iterator (this_arg,
                                            ECMA_ITERATOR_KEYS,
                                            LIT_MAGIC_STRING_MAP_UL,
                                            ECMA_BUILTIN_ID_MAP_ITERATOR_PROTOTYPE,
                                            ECMA_PSEUDO_MAP_ITERATOR);
} /* ecma_builtin_map_prototype_object_keys */

/**
 * The Map.prototype object's 'values' routine
 *
 * See also:
 *          ECMA-262 v6, 23.1.3.11
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_map_prototype_object_values (ecma_value_t this_arg) /**< this argument */
{
  return ecma_op_container_create_iterator (this_arg,
                                            ECMA_ITERATOR_VALUES,
                                            LIT_MAGIC_STRING_MAP_UL,
                                            ECMA_BUILTIN_ID_MAP_ITERATOR_PROTOTYPE,
                                            ECMA_PSEUDO_MAP_ITERATOR);
} /* ecma_builtin_map_prototype_object_values */

#endif /* ENABLED (JERRY_ES2015_BUILTIN_ITERATOR) */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_ES2015_BUILTIN_MAP) */
