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
#include "ecma-array-object.h"
#include "ecma-builtins.h"
#include "ecma-builtin-helpers.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-iterator-object.h"
#include "ecma-container-object.h"
#include "ecma-property-hashmap.h"
#include "ecma-objects.h"

#if ENABLED (JERRY_ES2015_BUILTIN_CONTAINER)

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup \addtogroup ecmamaphelpers ECMA builtin Map/Set helper functions
 * @{
 */

/**
 * Handle calling [[Construct]] of built-in Map/Set like objects
 *
 * @return ecma value
 */
ecma_value_t
ecma_op_container_create (const ecma_value_t *arguments_list_p, /**< arguments list */
                          ecma_length_t arguments_list_len, /**< number of arguments */
                          lit_magic_string_id_t lit_id, /**< internal class id */
                          ecma_builtin_id_t proto_id) /**< prototype builtin id */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);
  JERRY_ASSERT (lit_id == LIT_MAGIC_STRING_MAP_UL
                || lit_id == LIT_MAGIC_STRING_SET_UL
                || lit_id == LIT_MAGIC_STRING_WEAKMAP_UL
                || lit_id == LIT_MAGIC_STRING_WEAKSET_UL);

  ecma_object_t *internal_object_p = ecma_create_object (NULL,
                                                         sizeof (ecma_extended_object_t),
                                                         ECMA_OBJECT_TYPE_CLASS);

  ecma_extended_object_t *container_p = (ecma_extended_object_t *) internal_object_p;
  container_p->u.class_prop.class_id = LIT_INTERNAL_MAGIC_STRING_CONTAINER;
  container_p->u.class_prop.extra_info = ECMA_CONTAINER_FLAGS_EMPTY;
  container_p->u.class_prop.u.length = 0;

  if (lit_id == LIT_MAGIC_STRING_WEAKMAP_UL || lit_id == LIT_MAGIC_STRING_WEAKSET_UL)
  {
    container_p->u.class_prop.extra_info |= ECMA_CONTAINER_FLAGS_WEAK;
  }

  ecma_object_t *object_p = ecma_create_object (ecma_builtin_get (proto_id),
                                                sizeof (ecma_extended_object_t),
                                                ECMA_OBJECT_TYPE_CLASS);

  ecma_extended_object_t *map_obj_p = (ecma_extended_object_t *) object_p;
  map_obj_p->u.class_prop.class_id = (uint16_t) lit_id;
  ECMA_SET_INTERNAL_VALUE_POINTER (map_obj_p->u.class_prop.u.value, container_p);

  ecma_deref_object (internal_object_p);

  ecma_value_t set_value = ecma_make_object_value (object_p);
  ecma_value_t result = set_value;

#if ENABLED (JERRY_ES2015)
  if (arguments_list_len == 0)
  {
    return result;
  }

  ecma_value_t iterable = arguments_list_p[0];

  if (ecma_is_value_undefined (iterable) || ecma_is_value_null (iterable))
  {
    return result;
  }

  lit_magic_string_id_t adder_string_id;
  if (lit_id == LIT_MAGIC_STRING_MAP_UL || lit_id == LIT_MAGIC_STRING_WEAKMAP_UL)
  {
    adder_string_id = LIT_MAGIC_STRING_SET;
  }
  else
  {
    adder_string_id = LIT_MAGIC_STRING_ADD;
  }

  result = ecma_op_object_get_by_magic_id (object_p, adder_string_id);
  if (ECMA_IS_VALUE_ERROR (result))
  {
    goto cleanup_object;
  }

  if (!ecma_op_is_callable (result))
  {
    ecma_free_value (result);
    result = ecma_raise_type_error (ECMA_ERR_MSG ("add/set function is not callable."));
    goto cleanup_object;
  }

  ecma_object_t *adder_func_p = ecma_get_object_from_value (result);

  result = ecma_op_get_iterator (iterable, ECMA_VALUE_EMPTY);

  if (ECMA_IS_VALUE_ERROR (result))
  {
    goto cleanup_adder;
  }

  const ecma_value_t iter = result;

  while (true)
  {
    result = ecma_op_iterator_step (iter);

    if (ECMA_IS_VALUE_ERROR (result))
    {
      goto cleanup_iter;
    }

    if (ecma_is_value_false (result))
    {
      break;
    }

    const ecma_value_t next = result;
    result = ecma_op_iterator_value (next);
    ecma_free_value (next);

    if (ECMA_IS_VALUE_ERROR (result))
    {
      goto cleanup_iter;
    }

    if (lit_id == LIT_MAGIC_STRING_SET_UL || lit_id == LIT_MAGIC_STRING_WEAKSET_UL)
    {
      const ecma_value_t value = result;

      ecma_value_t arguments[] = { value };
      result = ecma_op_function_call (adder_func_p, set_value, arguments, 1);

      ecma_free_value (value);
    }
    else
    {
      if (!ecma_is_value_object (result))
      {
        // TODO close the iterator when generator function will be supported
        ecma_free_value (result);
        result = ecma_raise_type_error (ECMA_ERR_MSG ("Iterator value is not an object."));
        goto cleanup_iter;
      }

      ecma_object_t *next_object_p = ecma_get_object_from_value (result);

      result = ecma_op_object_get_by_uint32_index (next_object_p, 0);

      if (ECMA_IS_VALUE_ERROR (result))
      {
        // TODO close the iterator when generator function will be supported
        ecma_deref_object (next_object_p);
        goto cleanup_iter;
      }

      const ecma_value_t key = result;

      result = ecma_op_object_get_by_uint32_index (next_object_p, 1);

      if (ECMA_IS_VALUE_ERROR (result))
      {
        // TODO close the iterator when generator function will be supported
        ecma_deref_object (next_object_p);
        ecma_free_value (key);
        goto cleanup_iter;
      }

      const ecma_value_t value = result;
      ecma_value_t arguments[] = { key, value };
      result = ecma_op_function_call (adder_func_p, set_value, arguments, 2);

      ecma_free_value (key);
      ecma_free_value (value);
      ecma_deref_object (next_object_p);
    }


    if (ECMA_IS_VALUE_ERROR (result))
    {
      // TODO close the iterator when generator function will be supported
      goto cleanup_iter;
    }

    ecma_free_value (result);
  }

  ecma_free_value (iter);
  ecma_deref_object (adder_func_p);
  return ecma_make_object_value (object_p);

cleanup_iter:
  ecma_free_value (iter);
cleanup_adder:
  ecma_deref_object (adder_func_p);
cleanup_object:
  ecma_deref_object (object_p);
#endif /* ENABLED (JERRY_ES2015) */

  return result;
} /* ecma_op_container_create */

/**
 * Get Map/Set object pointer
 *
 * Note:
 *   If the function returns with NULL, the error object has
 *   already set, and the caller must return with ECMA_VALUE_ERROR
 *
 * @return pointer to the Map/Set if this_arg is a valid Map/Set object
 *         NULL otherwise
 */
static ecma_extended_object_t *
ecma_op_container_get_object (ecma_value_t this_arg, /**< this argument */
                              lit_magic_string_id_t lit_id) /**< internal class id */
{
  if (ecma_is_value_object (this_arg))
  {
    ecma_extended_object_t *map_object_p = (ecma_extended_object_t *) ecma_get_object_from_value (this_arg);

    if (ecma_get_object_type ((ecma_object_t *) map_object_p) == ECMA_OBJECT_TYPE_CLASS
        && map_object_p->u.class_prop.class_id == lit_id)
    {
      return map_object_p;
    }
  }

#if ENABLED (JERRY_ERROR_MESSAGES)
  ecma_raise_standard_error_with_format (ECMA_ERROR_TYPE,
                                         "Expected a % object.",
                                         ecma_make_string_value (ecma_get_magic_string (lit_id)));
#else /* !ENABLED (JERRY_ERROR_MESSAGES) */
  ecma_raise_type_error (NULL);
#endif /* ENABLED (JERRY_ERROR_MESSAGES) */

  return NULL;
} /* ecma_op_container_get_object */

/**
 * Creates a property key for the internal object from the given argument
 *
 * Note:
 *      This operation does not increase the reference counter of strings and symbols
 *
 * @return property key
 */
static ecma_string_t *
ecma_op_container_to_key (ecma_value_t key_arg) /**< key argument */
{
  if (ecma_is_value_prop_name (key_arg))
  {
    ecma_string_t *prop_name_p = ecma_get_prop_name_from_value (key_arg);
    ecma_ref_ecma_string (prop_name_p);
    return prop_name_p;
  }

  if (ecma_is_value_integer_number (key_arg))
  {
    ecma_integer_value_t integer = ecma_get_integer_from_value (key_arg);

    if (JERRY_LIKELY (integer > 0 && integer <= ECMA_DIRECT_STRING_MAX_IMM))
    {
      return (ecma_string_t *) ECMA_CREATE_DIRECT_STRING (ECMA_DIRECT_STRING_ECMA_INTEGER, (uintptr_t) integer);
    }
  }

  return ecma_new_map_key_string (key_arg);
} /* ecma_op_container_to_key */

/**
 * Returns with the size of the Map/Set object.
 *
 * @return size of the Map/Set object as ecma-value.
 */
ecma_value_t
ecma_op_container_size (ecma_value_t this_arg, /**< this argument */
                        lit_magic_string_id_t lit_id) /**< internal class id */
{
  ecma_extended_object_t *map_object_p = ecma_op_container_get_object (this_arg, lit_id);

  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_extended_object_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_extended_object_t,
                                                                         map_object_p->u.class_prop.u.value);
  return ecma_make_uint32_value (container_p->u.class_prop.u.length);
} /* ecma_op_container_size */

/**
 * The generic Map/WeakMap prototype object's 'get' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_container_get (ecma_value_t this_arg, /**< this argument */
                       ecma_value_t key_arg, /**< key argument */
                       lit_magic_string_id_t lit_id) /**< internal class id */
{
  ecma_extended_object_t *map_object_p = ecma_op_container_get_object (this_arg, lit_id);

  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

#if ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP)
  if (lit_id == LIT_MAGIC_STRING_WEAKMAP_UL && !ecma_is_value_object (key_arg))
  {
    return ECMA_VALUE_UNDEFINED;
  }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP) */

  ecma_extended_object_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_extended_object_t,
                                                                         map_object_p->u.class_prop.u.value);

  if (container_p->u.class_prop.u.length == 0)
  {
    return ECMA_VALUE_UNDEFINED;
  }

  ecma_string_t *prop_name_p = ecma_op_container_to_key (key_arg);

  ecma_property_t *property_p = ecma_find_named_property ((ecma_object_t *) container_p, prop_name_p);

  ecma_deref_ecma_string (prop_name_p);

  if (property_p == NULL || ecma_is_value_empty (ECMA_PROPERTY_VALUE_PTR (property_p)->value))
  {
    return ECMA_VALUE_UNDEFINED;
  }

  return ecma_copy_value (ECMA_PROPERTY_VALUE_PTR (property_p)->value);
} /* ecma_op_container_get */

/**
 * The generic Map/Set prototype object's 'has' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_container_has (ecma_value_t this_arg, /**< this argument */
                       ecma_value_t key_arg, /**< key argument */
                       lit_magic_string_id_t lit_id) /**< internal class id */
{
  ecma_extended_object_t *map_object_p = ecma_op_container_get_object (this_arg, lit_id);

  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_extended_object_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_extended_object_t,
                                                                         map_object_p->u.class_prop.u.value);

#if ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP) || ENABLED (JERRY_ES2015_BUILTIN_WEAKSET)
  if ((container_p->u.class_prop.extra_info & ECMA_CONTAINER_FLAGS_WEAK) != 0
      && !ecma_is_value_object (key_arg))
  {
    return ECMA_VALUE_FALSE;
  }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP) ||  ENABLED (JERRY_ES2015_BUILTIN_WEAKSET) */

  if (container_p->u.class_prop.u.length == 0)
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_string_t *prop_name_p = ecma_op_container_to_key (key_arg);

  ecma_property_t *property_p = ecma_find_named_property ((ecma_object_t *) container_p, prop_name_p);

  ecma_deref_ecma_string (prop_name_p);

  return ecma_make_boolean_value (property_p != NULL
                                  && !ecma_is_value_empty (ECMA_PROPERTY_VALUE_PTR (property_p)->value));
} /* ecma_op_container_has */

/**
 * Set a weak reference from a container to a key object
 */
static void
ecma_op_container_set_weak (ecma_object_t *const key_p, /**< key object */
                            ecma_extended_object_t *const container_p) /**< container */
{
  if (JERRY_UNLIKELY (ecma_op_object_is_fast_array (key_p)))
  {
    ecma_fast_array_convert_to_normal (key_p);
  }

  ecma_string_t *weak_refs_string_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_WEAK_REFS);
  ecma_property_t *property_p = ecma_find_named_property (key_p, weak_refs_string_p);
  ecma_collection_t *refs_p;

  if (property_p == NULL)
  {
    ecma_property_value_t *value_p = ecma_create_named_data_property (key_p,
                                                                      weak_refs_string_p,
                                                                      ECMA_PROPERTY_CONFIGURABLE_WRITABLE,
                                                                      &property_p);
    ECMA_CONVERT_DATA_PROPERTY_TO_INTERNAL_PROPERTY (property_p);
    refs_p = ecma_new_collection ();
    ECMA_SET_INTERNAL_VALUE_POINTER (value_p->value, refs_p);
  }
  else
  {
    refs_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_collection_t, (ECMA_PROPERTY_VALUE_PTR (property_p)->value));
  }

  const ecma_value_t container_value = ecma_make_object_value ((ecma_object_t *) container_p);
  for (uint32_t i = 0; i < refs_p->item_count; i++)
  {
    if (ecma_is_value_empty (refs_p->buffer_p[i]))
    {
      refs_p->buffer_p[i] = container_value;
      return;
    }
  }

  ecma_collection_push_back (refs_p, container_value);
} /* ecma_op_container_set_weak */

/**
 * The generic Map prototype object's 'set' and Set prototype object's 'add' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_container_set (ecma_value_t this_arg, /**< this argument */
                       ecma_value_t key_arg, /**< key argument */
                       ecma_value_t value_arg, /**< value argument */
                       lit_magic_string_id_t lit_id) /**< internal class id */
{
  ecma_extended_object_t *map_object_p = ecma_op_container_get_object (this_arg, lit_id);

  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_extended_object_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_extended_object_t,
                                                                         map_object_p->u.class_prop.u.value);

#if ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP) ||  ENABLED (JERRY_ES2015_BUILTIN_WEAKSET)
  if ((container_p->u.class_prop.extra_info & ECMA_CONTAINER_FLAGS_WEAK) != 0
      && !ecma_is_value_object (key_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Key must be an object"));
  }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP) ||  ENABLED (JERRY_ES2015_BUILTIN_WEAKSET) */

  ecma_string_t *prop_name_p = ecma_op_container_to_key (key_arg);

  ecma_property_t *property_p = ecma_find_named_property ((ecma_object_t *) container_p, prop_name_p);

  if (property_p == NULL)
  {
    ecma_property_value_t *value_p = ecma_create_named_data_property ((ecma_object_t *) container_p,
                                                                      prop_name_p,
                                                                      ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                                      NULL);
    value_p->value = ecma_copy_value_if_not_object (value_arg);
    container_p->u.class_prop.u.length++;

#if ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP) ||  ENABLED (JERRY_ES2015_BUILTIN_WEAKSET)
    if ((container_p->u.class_prop.extra_info & ECMA_CONTAINER_FLAGS_WEAK) != 0)
    {
      ecma_object_t *key_p = ecma_get_object_from_value (key_arg);
      ecma_op_container_set_weak (key_p, container_p);
    }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP) ||  ENABLED (JERRY_ES2015_BUILTIN_WEAKSET) */

  }
  else
  {
    if (ecma_is_value_empty (ECMA_PROPERTY_VALUE_PTR (property_p)->value))
    {
      container_p->u.class_prop.u.length++;
    }

    ecma_named_data_property_assign_value ((ecma_object_t *) container_p,
                                           ECMA_PROPERTY_VALUE_PTR (property_p),
                                           value_arg);
  }

  ecma_deref_ecma_string (prop_name_p);
  ecma_ref_object ((ecma_object_t *) map_object_p);
  return this_arg;
} /* ecma_op_container_set */

/**
 * The generic Map/Set prototype object's 'forEach' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_container_foreach (ecma_value_t this_arg, /**< this argument */
                           ecma_value_t predicate, /**< callback function */
                           ecma_value_t predicate_this_arg, /**< this argument for
                                                             *   invoke predicate */
                           lit_magic_string_id_t lit_id) /**< internal class id */
{
  ecma_extended_object_t *map_object_p = ecma_op_container_get_object (this_arg, lit_id);

  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  if (!ecma_op_is_callable (predicate))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
  }

  JERRY_ASSERT (ecma_is_value_object (predicate));
  ecma_object_t *func_object_p = ecma_get_object_from_value (predicate);

  ecma_object_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                                map_object_p->u.class_prop.u.value);

  ecma_collection_t *props_p = ecma_op_object_get_property_names (container_p, ECMA_LIST_NO_OPTS);

  ecma_value_t *buffer_p = props_p->buffer_p;

  ecma_value_t ret_value = ECMA_VALUE_UNDEFINED;

  ecma_ref_object (container_p);

  for (uint32_t i = 0; i < props_p->item_count; i++)
  {
    ecma_string_t *prop_name_p = ecma_get_prop_name_from_value (buffer_p[i]);
    ecma_property_t *property_p = ecma_find_named_property (container_p, prop_name_p);
    JERRY_ASSERT (property_p != NULL);

    if (ecma_is_value_empty (ECMA_PROPERTY_VALUE_PTR (property_p)->value))
    {
      continue;
    }

    ecma_value_t value = ecma_copy_value (ECMA_PROPERTY_VALUE_PTR (property_p)->value);
    ecma_value_t key_arg;

    if (lit_id == LIT_MAGIC_STRING_SET_UL)
    {
      key_arg = value;
    }
    else if (ecma_prop_name_is_map_key (prop_name_p))
    {
      key_arg = ((ecma_extended_string_t *) prop_name_p)->u.value;
    }
    else
    {
      if (ECMA_IS_DIRECT_STRING (prop_name_p)
          && ECMA_GET_DIRECT_STRING_TYPE (prop_name_p) == ECMA_DIRECT_STRING_ECMA_INTEGER)
      {
        key_arg = ecma_make_uint32_value ((uint32_t) ECMA_GET_DIRECT_STRING_VALUE (prop_name_p));
      }
      else
      {
        key_arg = buffer_p[i];
      }
    }

    ecma_value_t call_args[] = { value, key_arg };

    ecma_value_t call_value = ecma_op_function_call (func_object_p, predicate_this_arg, call_args, 2);

    ecma_free_value (value);

    if (ECMA_IS_VALUE_ERROR (call_value))
    {
      ret_value = call_value;
      break;
    }

    ecma_free_value (call_value);
  }

  ecma_deref_object (container_p);
  ecma_collection_free (props_p);

  return ret_value;
} /* ecma_op_container_foreach */

/**
 * The Map/Set prototype object's 'clear' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_container_clear (ecma_value_t this_arg, /**< this argument */
                         lit_magic_string_id_t lit_id) /**< internal class id */
{
  ecma_extended_object_t *map_object_p = ecma_op_container_get_object (this_arg, lit_id);

  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_object_t *internal_object_p = ecma_create_object (NULL,
                                                         sizeof (ecma_extended_object_t),
                                                         ECMA_OBJECT_TYPE_CLASS);

  ecma_extended_object_t *container_p = (ecma_extended_object_t *) internal_object_p;
  container_p->u.class_prop.class_id = LIT_INTERNAL_MAGIC_STRING_CONTAINER;
  container_p->u.class_prop.extra_info = ECMA_CONTAINER_FLAGS_EMPTY;
  container_p->u.class_prop.u.length = 0;

  ECMA_SET_INTERNAL_VALUE_POINTER (map_object_p->u.class_prop.u.value, internal_object_p);

  ecma_deref_object (internal_object_p);

  return ECMA_VALUE_UNDEFINED;
} /* ecma_op_container_clear */

/**
 * The generic Map/Set prototype object's 'delete' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_container_delete (ecma_value_t this_arg, /**< this argument */
                          ecma_value_t key_arg, /**< key argument */
                          lit_magic_string_id_t lit_id) /**< internal class id */
{
  ecma_extended_object_t *map_object_p = ecma_op_container_get_object (this_arg, lit_id);

  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_extended_object_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_extended_object_t,
                                                                         map_object_p->u.class_prop.u.value);

  ecma_string_t *prop_name_p = ecma_op_container_to_key (key_arg);

  ecma_property_t *property_p = ecma_find_named_property ((ecma_object_t *) container_p, prop_name_p);

  ecma_deref_ecma_string (prop_name_p);

  if (property_p == NULL || ecma_is_value_empty (ECMA_PROPERTY_VALUE_PTR (property_p)->value))
  {
    return ECMA_VALUE_FALSE;
  }


  ecma_named_data_property_assign_value ((ecma_object_t *) container_p,
                                         ECMA_PROPERTY_VALUE_PTR (property_p),
                                         ECMA_VALUE_EMPTY);
  container_p->u.class_prop.u.length--;

  return ECMA_VALUE_TRUE;
} /* ecma_op_container_delete */

/**
 * The generic WeakMap/WeakSet prototype object's 'delete' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_container_delete_weak (ecma_value_t this_arg, /**< this argument */
                               ecma_value_t key_arg, /**< key argument */
                               lit_magic_string_id_t lit_id) /**< internal class id */
{
  ecma_extended_object_t *map_object_p = ecma_op_container_get_object (this_arg, lit_id);

  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  if (!ecma_is_value_object (key_arg))
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_extended_object_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_extended_object_t,
                                                                         map_object_p->u.class_prop.u.value);

  ecma_string_t *prop_name_p = ecma_op_container_to_key (key_arg);

  ecma_property_t *property_p = ecma_find_named_property ((ecma_object_t *) container_p, prop_name_p);

  ecma_deref_ecma_string (prop_name_p);

  if (property_p == NULL)
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_delete_property ((ecma_object_t *) container_p, ECMA_PROPERTY_VALUE_PTR (property_p));

  ecma_object_t *internal_obj_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                                   map_object_p->u.class_prop.u.value);
  ecma_object_t *key_object_p = ecma_get_object_from_value (key_arg);
  ecma_op_container_unref_weak (key_object_p, ecma_make_object_value (internal_obj_p));

  container_p->u.class_prop.u.length--;
  return ECMA_VALUE_TRUE;
} /* ecma_op_container_delete_weak */

/**
 * Helper function to remove a weak reference to an object.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
void
ecma_op_container_unref_weak (ecma_object_t *object_p, /**< this argument */
                              ecma_value_t ref_holder) /**< key argument */
{
  ecma_string_t *weak_refs_string_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_WEAK_REFS);

  ecma_property_t *property_p = ecma_find_named_property (object_p, weak_refs_string_p);
  JERRY_ASSERT (property_p != NULL);

  ecma_collection_t *refs_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_collection_t,
                                                               ECMA_PROPERTY_VALUE_PTR (property_p)->value);
  for (uint32_t i = 0; i < refs_p->item_count; i++)
  {
    if (refs_p->buffer_p[i] == ref_holder)
    {
      refs_p->buffer_p[i] = ECMA_VALUE_EMPTY;
      break;
    }
  }
} /* ecma_op_container_unref_weak */

/**
 * Helper function to remove a key/value pair from a weak container object
 */
void
ecma_op_container_remove_weak_entry (ecma_object_t *container_p, /**< internal container object */
                                     ecma_value_t key_arg) /**< key */
{
  ecma_string_t *prop_name_p = ecma_new_map_key_string (key_arg);

  ecma_property_t *property_p = ecma_find_named_property (container_p, prop_name_p);
  JERRY_ASSERT (property_p != NULL);

  ecma_deref_ecma_string (prop_name_p);

  ecma_delete_property (container_p, ECMA_PROPERTY_VALUE_PTR (property_p));
  ((ecma_extended_object_t *) container_p)->u.class_prop.u.length--;
} /* ecma_op_container_remove_weak_entry */

#if ENABLED (JERRY_ES2015)

/**
 * The Create{Set, Map}Iterator Abstract operation
 *
 * See also:
 *          ECMA-262 v6, 23.1.5.1
 *          ECMA-262 v6, 23.2.5.1
 *
 * Note:
 *     Returned value must be freed with ecma_free_value.
 *
 * @return Map/Set iterator object, if success
 *         error - otherwise
 */
ecma_value_t
ecma_op_container_create_iterator (ecma_value_t this_arg, /**< this argument */
                                   uint8_t type, /**< any combination of
                                                  *   ecma_iterator_type_t bits */
                                   lit_magic_string_id_t lit_id, /**< internal class id */
                                   ecma_builtin_id_t proto_id, /**< prototype builtin id */
                                   ecma_pseudo_array_type_t iterator_type) /**< type of the iterator */
{
  ecma_extended_object_t *map_object_p = ecma_op_container_get_object (this_arg, lit_id);

  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  return ecma_op_create_iterator_object (this_arg,
                                         ecma_builtin_get (proto_id),
                                         (uint8_t) iterator_type,
                                         type);
} /* ecma_op_container_create_iterator */

/**
 * The %{Set, Map}IteratorPrototype% object's 'next' routine
 *
 * See also:
 *          ECMA-262 v6, 23.1.5.2.1
 *          ECMA-262 v6, 23.2.5.2.1
 *
 * Note:
 *     Returned value must be freed with ecma_free_value.
 *
 * @return iterator result object, if success
 *         error - otherwise
 */
ecma_value_t
ecma_op_container_iterator_next (ecma_value_t this_val, /**< this argument */
                                 ecma_pseudo_array_type_t iterator_type) /**< type of the iterator */
{
  if (!ecma_is_value_object (this_val))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not an object."));
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (this_val);
  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

  if (ecma_get_object_type (obj_p) != ECMA_OBJECT_TYPE_PSEUDO_ARRAY
      || ext_obj_p->u.pseudo_array.type != iterator_type)
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not an iterator."));
  }

  ecma_value_t iterated_value = ext_obj_p->u.pseudo_array.u2.iterated_value;

  if (ecma_is_value_empty (iterated_value))
  {
    return ecma_create_iter_result_object (ECMA_VALUE_UNDEFINED, ECMA_VALUE_TRUE);
  }

  ecma_extended_object_t *map_object_p = (ecma_extended_object_t *) (ecma_get_object_from_value (iterated_value));

  ecma_object_t *internal_obj_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                                   map_object_p->u.class_prop.u.value);
  ecma_collection_t *props_p = ecma_op_object_get_property_names (internal_obj_p, ECMA_LIST_NO_OPTS);

  uint32_t length = props_p->item_count;
  uint32_t index = ext_obj_p->u.pseudo_array.u1.iterator_index;

  if (JERRY_UNLIKELY (index == ECMA_ITERATOR_INDEX_LIMIT))
  {
    /* After the ECMA_ITERATOR_INDEX_LIMIT limit is reached the [[%Iterator%NextIndex]]
       property is stored as an internal property */
    ecma_string_t *prop_name_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_ITERATOR_NEXT_INDEX);

    ecma_property_t *property_p = ecma_find_named_property (obj_p, prop_name_p);
    ecma_property_value_t *value_p;

    if (property_p == NULL)
    {
      value_p = ecma_create_named_data_property (obj_p, prop_name_p, ECMA_PROPERTY_FLAG_WRITABLE, &property_p);
      value_p->value = ecma_make_uint32_value (index);
    }
    else
    {
      value_p = ECMA_PROPERTY_VALUE_PTR (property_p);
      index = (uint32_t) (ecma_get_number_from_value (value_p->value) + 1);
      value_p->value = ecma_make_uint32_value (index);
    }
  }
  else
  {
    ext_obj_p->u.pseudo_array.u1.iterator_index++;
  }

  if (index >= length)
  {
    ext_obj_p->u.pseudo_array.u2.iterated_value = ECMA_VALUE_EMPTY;
    ecma_collection_free (props_p);
    return ecma_create_iter_result_object (ECMA_VALUE_UNDEFINED, ECMA_VALUE_TRUE);
  }

  uint8_t iterator_kind = ext_obj_p->u.pseudo_array.extra_info;

  ecma_value_t *buffer_p = props_p->buffer_p;

  ecma_value_t ret_value = ECMA_VALUE_UNDEFINED;

  for (uint32_t i = 0; i < props_p->item_count; i++)
  {
    if (index > 0)
    {
      index--;
      continue;
    }

    ecma_string_t *prop_name_p = ecma_get_prop_name_from_value (buffer_p[i]);
    ecma_property_t *property_p = ecma_find_named_property (internal_obj_p, prop_name_p);
    JERRY_ASSERT (property_p != NULL);

    if (ecma_is_value_empty (ECMA_PROPERTY_VALUE_PTR (property_p)->value))
    {
      if (i == props_p->item_count - 1)
      {
        ret_value = ecma_create_iter_result_object (ECMA_VALUE_UNDEFINED, ECMA_VALUE_TRUE);
      }
      continue;
    }

    ecma_value_t value = ecma_copy_value (ECMA_PROPERTY_VALUE_PTR (property_p)->value);
    ecma_value_t key_arg;

    if (iterator_type == ECMA_PSEUDO_SET_ITERATOR)
    {
      key_arg = value;
    }
    else if (ecma_prop_name_is_map_key (prop_name_p))
    {
      key_arg = ((ecma_extended_string_t *) prop_name_p)->u.value;
    }
    else
    {
      if (ECMA_IS_DIRECT_STRING (prop_name_p)
          && ECMA_GET_DIRECT_STRING_TYPE (prop_name_p) == ECMA_DIRECT_STRING_ECMA_INTEGER)
      {
        key_arg = ecma_make_uint32_value ((uint32_t) ECMA_GET_DIRECT_STRING_VALUE (prop_name_p));
      }
      else
      {
        key_arg = buffer_p[i];
      }
    }

    if (iterator_kind == ECMA_ITERATOR_KEYS)
    {
      ret_value = ecma_create_iter_result_object (key_arg, ECMA_VALUE_FALSE);
    }
    else if (iterator_kind == ECMA_ITERATOR_VALUES)
    {
      ret_value = ecma_create_iter_result_object (value, ECMA_VALUE_FALSE);
    }
    else
    {
      JERRY_ASSERT (iterator_kind == ECMA_ITERATOR_KEYS_VALUES);

      ecma_value_t entry_array_value;
      entry_array_value = ecma_create_array_from_iter_element (value, key_arg);

      ret_value = ecma_create_iter_result_object (entry_array_value, ECMA_VALUE_FALSE);
      ecma_free_value (entry_array_value);
    }

    ecma_free_value (value);
    break;
  }

  ecma_collection_free (props_p);

  return ret_value;
} /* ecma_op_container_iterator_next */

#endif /* ENABLED (JERRY_ES2015) */

/**
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_ES2015_BUILTIN_CONTAINER) */
