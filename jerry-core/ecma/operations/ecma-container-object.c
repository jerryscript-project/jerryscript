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
#include "jcontext.h"
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
 * Create a new internal buffer.
 *
 * Note:
 *   The first element of the collection tracks the size of the buffer.
 *   ECMA_VALUE_EMPTY values are not calculated into the size.
 *
 * @return pointer to the internal buffer
 */
static inline ecma_collection_t *
ecma_op_create_internal_buffer (void)
{
  ecma_collection_t *collection_p = ecma_new_collection ();
  ecma_collection_push_back (collection_p, (ecma_value_t) 0);

  return collection_p;
} /* ecma_op_create_internal_buffer */

/**
 * Append values to the internal buffer.
 */
static void
ecma_op_internal_buffer_append (ecma_collection_t *container_p, /**< internal container pointer */
                                ecma_value_t key_arg, /**< key argument */
                                ecma_value_t value_arg, /**< value argument */
                                lit_magic_string_id_t lit_id) /**< class id */
{
  JERRY_ASSERT (container_p != NULL);

  ecma_collection_push_back (container_p, ecma_copy_value_if_not_object (key_arg));

  if (lit_id == LIT_MAGIC_STRING_WEAKMAP_UL || lit_id == LIT_MAGIC_STRING_MAP_UL)
  {
    ecma_collection_push_back (container_p, ecma_copy_value_if_not_object (value_arg));
  }

  ECMA_CONTAINER_SET_SIZE (container_p, ECMA_CONTAINER_GET_SIZE (container_p) + 1);
} /* ecma_op_internal_buffer_append */

/**
 * Update the value of a given entry.
 */
static inline void
ecma_op_internal_buffer_update (ecma_value_t *entry_p, /**< entry pointer */
                                ecma_value_t value_arg, /**< value argument */
                                lit_magic_string_id_t lit_id) /**< class id */
{
  JERRY_ASSERT (entry_p != NULL);

  if (lit_id == LIT_MAGIC_STRING_WEAKMAP_UL || lit_id == LIT_MAGIC_STRING_MAP_UL)
  {
    ecma_free_value_if_not_object (((ecma_container_pair_t *) entry_p)->value);

    ((ecma_container_pair_t *) entry_p)->value = ecma_copy_value_if_not_object (value_arg);
  }
} /* ecma_op_internal_buffer_update */

/**
 * Delete element from the internal buffer.
 */
static void
ecma_op_internal_buffer_delete (ecma_collection_t *container_p, /**< internal container pointer */
                                ecma_container_pair_t *entry_p, /**< entry pointer */
                                lit_magic_string_id_t lit_id) /**< class id */
{
  JERRY_ASSERT (container_p != NULL);
  JERRY_ASSERT (entry_p != NULL);

  ecma_free_value_if_not_object (entry_p->key);
  entry_p->key = ECMA_VALUE_EMPTY;

  if (lit_id == LIT_MAGIC_STRING_WEAKMAP_UL || lit_id == LIT_MAGIC_STRING_MAP_UL)
  {
    ecma_free_value_if_not_object (entry_p->value);
    entry_p->value = ECMA_VALUE_EMPTY;
  }

  ECMA_CONTAINER_SET_SIZE (container_p, ECMA_CONTAINER_GET_SIZE (container_p) - 1);
} /* ecma_op_internal_buffer_delete */

/**
 * Find an entry in the collection.
 *
 * @return pointer to the appropriate entry.
 */
static ecma_value_t *
ecma_op_internal_buffer_find (ecma_collection_t *container_p, /**< internal container pointer */
                              ecma_value_t key_arg, /**< key argument */
                              lit_magic_string_id_t lit_id) /**< class id */
{
  JERRY_ASSERT (container_p != NULL);

  uint8_t entry_size = ecma_op_container_entry_size (lit_id);
  uint32_t entry_count = ECMA_CONTAINER_ENTRY_COUNT (container_p);
  ecma_value_t *start_p = ECMA_CONTAINER_START (container_p);

  for (uint32_t i = 0; i < entry_count; i += entry_size)
  {
    ecma_value_t *entry_p = start_p + i;

    if (ecma_op_same_value_zero (*entry_p, key_arg))
    {
      return entry_p;
    }
  }

  return NULL;
} /* ecma_op_internal_buffer_find */

/**
 * Get the value that belongs to the key.
 *
 * Note: in case of Set containers, the values are the same as the keys.
 *
 * @return ecma value
 */
static ecma_value_t
ecma_op_container_get_value (ecma_value_t *entry_p, /**< entry (key) pointer */
                             lit_magic_string_id_t lit_id) /**< class id */
{
  JERRY_ASSERT (entry_p != NULL);

  if (lit_id == LIT_MAGIC_STRING_WEAKMAP_UL || lit_id == LIT_MAGIC_STRING_MAP_UL)
  {
    return ((ecma_container_pair_t *) entry_p)->value;
  }

  return *entry_p;
} /* ecma_op_container_get_value */

/**
 * Get the size (in ecma_value_t) of the stored entries.
 *
 * @return size of the entries.
 */
uint8_t
ecma_op_container_entry_size (lit_magic_string_id_t lit_id) /**< class id */
{
  if (lit_id == LIT_MAGIC_STRING_WEAKMAP_UL || lit_id == LIT_MAGIC_STRING_MAP_UL)
  {
    return ECMA_CONTAINER_PAIR_SIZE;
  }

  return ECMA_CONTAINER_VALUE_SIZE;
} /* ecma_op_container_entry_size */

#if ENABLED (JERRY_ES2015_BUILTIN_WEAKSET)
/**
 * Release the entries in the WeakSet container.
 */
static void
ecma_op_container_free_weakset_entries (ecma_object_t *object_p, /**< object pointer */
                                        ecma_collection_t *container_p) /** internal buffer pointer */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (container_p != NULL);

  uint32_t entry_count = ECMA_CONTAINER_ENTRY_COUNT (container_p);
  ecma_value_t *start_p = ECMA_CONTAINER_START (container_p);

  for (uint32_t i = 0; i < entry_count; i += ECMA_CONTAINER_VALUE_SIZE)
  {
    ecma_value_t *entry_p = start_p + i;

    if (ecma_is_value_empty (*entry_p))
    {
      continue;
    }

    ecma_op_container_unref_weak (ecma_get_object_from_value (*entry_p), ecma_make_object_value (object_p));
    ecma_op_container_remove_weak_entry (object_p, *entry_p);

    *entry_p = ECMA_VALUE_EMPTY;
  }
} /* ecma_op_container_free_weakset_entries */
#endif /* ENABLED (JERRY_ES2015_BUILTIN_WEAKSET) */

#if ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP)
/**
 * Release the entries in the WeakMap container.
 */
static void
ecma_op_container_free_weakmap_entries (ecma_object_t *object_p, /**< object pointer */
                                        ecma_collection_t *container_p) /**< internal buffer pointer */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (container_p != NULL);

  uint32_t entry_count = ECMA_CONTAINER_ENTRY_COUNT (container_p);
  ecma_value_t *start_p = ECMA_CONTAINER_START (container_p);

  for (uint32_t i = 0; i < entry_count; i += ECMA_CONTAINER_PAIR_SIZE)
  {
    ecma_container_pair_t *entry_p = (ecma_container_pair_t *) (start_p + i);

    if (ecma_is_value_empty (entry_p->key))
    {
      continue;
    }

    ecma_op_container_unref_weak (ecma_get_object_from_value (entry_p->key), ecma_make_object_value (object_p));
    ecma_op_container_remove_weak_entry (object_p, entry_p->key);

    ecma_free_value_if_not_object (entry_p->value);

    entry_p->key = ECMA_VALUE_EMPTY;
    entry_p->value = ECMA_VALUE_EMPTY;
  }
} /* ecma_op_container_free_weakmap_entries */
#endif /* ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP) */

#if ENABLED (JERRY_ES2015_BUILTIN_SET)
/**
 * Release the entries in the Set container.
 */
static void
ecma_op_container_free_set_entries (ecma_collection_t *container_p)
{
  JERRY_ASSERT (container_p != NULL);

  uint32_t entry_count = ECMA_CONTAINER_ENTRY_COUNT (container_p);
  ecma_value_t *start_p = ECMA_CONTAINER_START (container_p);

  for (uint32_t i = 0; i < entry_count; i += ECMA_CONTAINER_VALUE_SIZE)
  {
    ecma_value_t *entry_p = start_p + i;

    if (ecma_is_value_empty (*entry_p))
    {
      continue;
    }

    ecma_free_value_if_not_object (*entry_p);
    *entry_p = ECMA_VALUE_EMPTY;
  }
} /* ecma_op_container_free_set_entries */
#endif /* ENABLED (JERRY_ES2015_BUILTIN_SET) */

#if ENABLED (JERRY_ES2015_BUILTIN_MAP)
/**
 * Release the entries in the Map container.
 */
static void
ecma_op_container_free_map_entries (ecma_collection_t *container_p)
{
  JERRY_ASSERT (container_p != NULL);

  uint32_t entry_count = ECMA_CONTAINER_ENTRY_COUNT (container_p);
  ecma_value_t *start_p = ECMA_CONTAINER_START (container_p);

  for (uint32_t i = 0; i < entry_count; i += ECMA_CONTAINER_PAIR_SIZE)
  {
    ecma_container_pair_t *entry_p = (ecma_container_pair_t *) (start_p + i);

    if (ecma_is_value_empty (entry_p->key))
    {
      continue;
    }

    ecma_free_value_if_not_object (entry_p->key);
    ecma_free_value_if_not_object (entry_p->value);

    entry_p->key = ECMA_VALUE_EMPTY;
    entry_p->value = ECMA_VALUE_EMPTY;
  }
} /* ecma_op_container_free_map_entries */
#endif /* ENABLED (JERRY_ES2015_BUILTIN_MAP) */

/**
 * Release the internal buffer and the stored entries.
 */
void
ecma_op_container_free_entries (ecma_object_t *object_p) /**< collection object pointer */
{
  JERRY_ASSERT (object_p != NULL);

  ecma_extended_object_t *map_object_p = (ecma_extended_object_t *) object_p;
  ecma_collection_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_collection_t,
                                                                    map_object_p->u.class_prop.u.value);

  switch (map_object_p->u.class_prop.class_id)
  {
#if ENABLED (JERRY_ES2015_BUILTIN_WEAKSET)
    case LIT_MAGIC_STRING_WEAKSET_UL:
    {
      ecma_op_container_free_weakset_entries (object_p, container_p);
      break;
    }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_WEAKSET) */
#if ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP)
    case LIT_MAGIC_STRING_WEAKMAP_UL:
    {
      ecma_op_container_free_weakmap_entries (object_p, container_p);
      break;
    }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP) */
#if ENABLED (JERRY_ES2015_BUILTIN_SET)
    case LIT_MAGIC_STRING_SET_UL:
    {
      ecma_op_container_free_set_entries (container_p);
      break;
    }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_SET) */
#if ENABLED (JERRY_ES2015_BUILTIN_MAP)
    case LIT_MAGIC_STRING_MAP_UL:
    {
      ecma_op_container_free_map_entries (container_p);
      break;
    }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_MAP) */
    default:
    {
      break;
    }
  }

  ECMA_CONTAINER_SET_SIZE (container_p, 0);
} /* ecma_op_container_free_entries */

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
  JERRY_ASSERT (JERRY_CONTEXT (current_new_target) != NULL);

  ecma_object_t *proto_p = ecma_op_get_prototype_from_constructor (JERRY_CONTEXT (current_new_target), proto_id);

  if (JERRY_UNLIKELY (proto_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_collection_t *container_p = ecma_op_create_internal_buffer ();
  ecma_object_t *object_p  = ecma_create_object (proto_p,
                                                 sizeof (ecma_extended_object_t),
                                                 ECMA_OBJECT_TYPE_CLASS);
  ecma_deref_object (proto_p);
  ecma_extended_object_t *map_obj_p = (ecma_extended_object_t *) object_p;
  map_obj_p->u.class_prop.extra_info = ECMA_CONTAINER_FLAGS_EMPTY;
  map_obj_p->u.class_prop.class_id = (uint16_t) lit_id;

  if (lit_id == LIT_MAGIC_STRING_WEAKMAP_UL || lit_id == LIT_MAGIC_STRING_WEAKSET_UL)
  {
    map_obj_p->u.class_prop.extra_info |= ECMA_CONTAINER_FLAGS_WEAK;
  }

  ECMA_SET_INTERNAL_VALUE_POINTER (map_obj_p->u.class_prop.u.value, container_p);

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
        ecma_free_value (result);
        ecma_raise_type_error (ECMA_ERR_MSG ("Iterator value is not an object."));
        result = ecma_op_iterator_close (iter);
        JERRY_ASSERT (ECMA_IS_VALUE_ERROR (result));
        goto cleanup_iter;
      }

      ecma_object_t *next_object_p = ecma_get_object_from_value (result);

      result = ecma_op_object_get_by_uint32_index (next_object_p, 0);

      if (ECMA_IS_VALUE_ERROR (result))
      {
        ecma_deref_object (next_object_p);
        ecma_op_iterator_close (iter);
        goto cleanup_iter;
      }

      const ecma_value_t key = result;

      result = ecma_op_object_get_by_uint32_index (next_object_p, 1);

      if (ECMA_IS_VALUE_ERROR (result))
      {
        ecma_deref_object (next_object_p);
        ecma_free_value (key);
        ecma_op_iterator_close (iter);
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
      ecma_op_iterator_close (iter);
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

  ecma_collection_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_collection_t,
                                                                    map_object_p->u.class_prop.u.value);

  return ecma_make_uint32_value (ECMA_CONTAINER_GET_SIZE (container_p));
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

  ecma_collection_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_collection_t,
                                                                    map_object_p->u.class_prop.u.value);

  if (ECMA_CONTAINER_GET_SIZE (container_p) == 0)
  {
    return ECMA_VALUE_UNDEFINED;
  }

  ecma_value_t *entry_p = ecma_op_internal_buffer_find (container_p, key_arg, lit_id);

  if (entry_p == NULL)
  {
    return ECMA_VALUE_UNDEFINED;
  }

  return ecma_copy_value (((ecma_container_pair_t *) entry_p)->value);
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

  ecma_collection_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_collection_t,
                                                                    map_object_p->u.class_prop.u.value);

#if ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP) || ENABLED (JERRY_ES2015_BUILTIN_WEAKSET)
  if ((map_object_p->u.class_prop.extra_info & ECMA_CONTAINER_FLAGS_WEAK) != 0
      && !ecma_is_value_object (key_arg))
  {
    return ECMA_VALUE_FALSE;
  }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP) ||  ENABLED (JERRY_ES2015_BUILTIN_WEAKSET) */

  if (ECMA_CONTAINER_GET_SIZE (container_p) == 0)
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_value_t *entry_p = ecma_op_internal_buffer_find (container_p, key_arg, lit_id);

  return ecma_make_boolean_value (entry_p != NULL);
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
 * Helper method for the Map.prototype.set and Set.prototype.add methods to swap the sign of the given value if needed
 *
 * See also:
 *          ECMA-262 v6, 23.2.3.1 step 6
 *          ECMA-262 v6, 23.1.3.9 step 6
 *
 * @return ecma value
 */
static ecma_value_t
ecma_op_container_set_noramlize_zero (ecma_value_t this_arg) /*< this arg */
{
  if (ecma_is_value_number (this_arg))
  {
    ecma_number_t number_value = ecma_get_number_from_value (this_arg);

    if (JERRY_UNLIKELY (ecma_number_is_zero (number_value) && ecma_number_is_negative (number_value)))
    {
      return ecma_make_integer_value (0);
    }
  }

  return this_arg;
} /* ecma_op_container_set_noramlize_zero */

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

  ecma_collection_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_collection_t,
                                                                    map_object_p->u.class_prop.u.value);

#if ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP) ||  ENABLED (JERRY_ES2015_BUILTIN_WEAKSET)
  if ((map_object_p->u.class_prop.extra_info & ECMA_CONTAINER_FLAGS_WEAK) != 0
      && !ecma_is_value_object (key_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Key must be an object"));
  }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP) ||  ENABLED (JERRY_ES2015_BUILTIN_WEAKSET) */

  ecma_value_t *entry_p = ecma_op_internal_buffer_find (container_p, key_arg, lit_id);

  if (entry_p == NULL)
  {
    ecma_op_internal_buffer_append (container_p,
                                    ecma_op_container_set_noramlize_zero (key_arg),
                                    value_arg,
                                    lit_id);

#if ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP) ||  ENABLED (JERRY_ES2015_BUILTIN_WEAKSET)
    if ((map_object_p->u.class_prop.extra_info & ECMA_CONTAINER_FLAGS_WEAK) != 0)
    {
      ecma_object_t *key_p = ecma_get_object_from_value (key_arg);
      ecma_op_container_set_weak (key_p, map_object_p);
    }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP) ||  ENABLED (JERRY_ES2015_BUILTIN_WEAKSET) */
  }
  else
  {
    ecma_op_internal_buffer_update (entry_p, ecma_op_container_set_noramlize_zero (value_arg), lit_id);
  }

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
  ecma_value_t ret_value = ECMA_VALUE_UNDEFINED;

  ecma_collection_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_collection_t,
                                                                    map_object_p->u.class_prop.u.value);

  uint8_t entry_size = ecma_op_container_entry_size (lit_id);

  for (uint32_t i = 0; i < ECMA_CONTAINER_ENTRY_COUNT (container_p); i += entry_size)
  {
    ecma_value_t *entry_p = ECMA_CONTAINER_START (container_p) + i;

    if (ecma_is_value_empty (*entry_p))
    {
      continue;
    }

    ecma_value_t key_arg = *entry_p;
    ecma_value_t value_arg = ecma_op_container_get_value (entry_p, lit_id);

    ecma_value_t call_args[] = { value_arg, key_arg, this_arg };
    ecma_value_t call_value = ecma_op_function_call (func_object_p, predicate_this_arg, call_args, 3);

    if (ECMA_IS_VALUE_ERROR (call_value))
    {
      ret_value = call_value;
      break;
    }

    ecma_free_value (call_value);
  }

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

  ecma_op_container_free_entries ((ecma_object_t *) map_object_p);

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

  ecma_collection_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_collection_t,
                                                                    map_object_p->u.class_prop.u.value);

  ecma_value_t *entry_p = ecma_op_internal_buffer_find (container_p, key_arg, lit_id);

  if (entry_p == NULL)
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_op_internal_buffer_delete (container_p, (ecma_container_pair_t *) entry_p, lit_id);
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

  ecma_collection_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_collection_t,
                                                                    map_object_p->u.class_prop.u.value);

  ecma_value_t *entry_p = ecma_op_internal_buffer_find (container_p, key_arg, lit_id);

  if (entry_p == NULL)
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_op_internal_buffer_delete (container_p, (ecma_container_pair_t *) entry_p, lit_id);

  ecma_object_t *key_object_p = ecma_get_object_from_value (key_arg);
  ecma_op_container_unref_weak (key_object_p, ecma_make_object_value ((ecma_object_t *) map_object_p));

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
ecma_op_container_remove_weak_entry (ecma_object_t *object_p, /**< internal container object */
                                     ecma_value_t key_arg) /**< key */
{
  ecma_extended_object_t *map_object_p = (ecma_extended_object_t *) object_p;

  ecma_collection_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_collection_t,
                                                                    map_object_p->u.class_prop.u.value);

  ecma_value_t *entry_p = ecma_op_internal_buffer_find (container_p, key_arg, map_object_p->u.class_prop.class_id);

  JERRY_ASSERT (entry_p != NULL);

  ecma_op_internal_buffer_delete (container_p, (ecma_container_pair_t *) entry_p, map_object_p->u.class_prop.class_id);
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
 * Get the index of the iterator object.
 *
 * @return index of the iterator.
 */
static uint32_t
ecma_op_iterator_get_index (ecma_object_t *iter_obj_p)  /**< iterator object pointer */
{
  uint32_t index = ((ecma_extended_object_t *) iter_obj_p)->u.pseudo_array.u1.iterator_index;

  if (JERRY_UNLIKELY (index == ECMA_ITERATOR_INDEX_LIMIT))
  {
    ecma_string_t *prop_name_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_ITERATOR_NEXT_INDEX);
    ecma_property_t *property_p = ecma_find_named_property (iter_obj_p, prop_name_p);
    ecma_property_value_t *value_p = ECMA_PROPERTY_VALUE_PTR (property_p);

    return (uint32_t) (ecma_get_number_from_value (value_p->value));
  }

  return index;
} /* ecma_op_iterator_get_index */

/**
 * Set the index of the iterator object.
 */
static void
ecma_op_iterator_set_index (ecma_object_t *iter_obj_p, /**< iterator object pointer */
                            uint32_t index) /* iterator index to set */
{
  if (JERRY_UNLIKELY (index >= ECMA_ITERATOR_INDEX_LIMIT))
  {
    /* After the ECMA_ITERATOR_INDEX_LIMIT limit is reached the [[%Iterator%NextIndex]]
       property is stored as an internal property */
    ecma_string_t *prop_name_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_ITERATOR_NEXT_INDEX);
    ecma_property_t *property_p = ecma_find_named_property (iter_obj_p, prop_name_p);
    ecma_property_value_t *value_p;

    if (property_p == NULL)
    {
      value_p = ecma_create_named_data_property (iter_obj_p, prop_name_p, ECMA_PROPERTY_FLAG_WRITABLE, &property_p);
      value_p->value = ecma_make_uint32_value (index);
    }
    else
    {
      value_p = ECMA_PROPERTY_VALUE_PTR (property_p);
      value_p->value = ecma_make_uint32_value (index);
    }
  }
  else
  {
    ((ecma_extended_object_t *) iter_obj_p)->u.pseudo_array.u1.iterator_index = (uint16_t) index;
  }
} /* ecma_op_iterator_set_index */

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
  lit_magic_string_id_t lit_id = map_object_p->u.class_prop.class_id;

  ecma_collection_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_collection_t,
                                                                    map_object_p->u.class_prop.u.value);
  uint32_t entry_count = ECMA_CONTAINER_ENTRY_COUNT (container_p);
  uint32_t index = ecma_op_iterator_get_index (obj_p);

  if (index == entry_count)
  {
    ext_obj_p->u.pseudo_array.u2.iterated_value = ECMA_VALUE_EMPTY;

    return ecma_create_iter_result_object (ECMA_VALUE_UNDEFINED, ECMA_VALUE_TRUE);
  }

  uint8_t entry_size = ecma_op_container_entry_size (lit_id);
  uint8_t iterator_kind = ext_obj_p->u.pseudo_array.extra_info;
  ecma_value_t *start_p = ECMA_CONTAINER_START (container_p);
  ecma_value_t ret_value = ECMA_VALUE_UNDEFINED;

  for (uint32_t i = index; i < entry_count; i += entry_size)
  {
    ecma_value_t *entry_p = start_p + i;

    if (ecma_is_value_empty (*entry_p))
    {
      if (i == (entry_count - entry_size))
      {
        ret_value = ecma_create_iter_result_object (ECMA_VALUE_UNDEFINED, ECMA_VALUE_TRUE);
        break;
      }

      continue;
    }

    ecma_op_iterator_set_index (obj_p, i + entry_size);

    ecma_value_t key_arg = *entry_p;
    ecma_value_t value_arg = ecma_op_container_get_value (entry_p, lit_id);

    if (iterator_kind == ECMA_ITERATOR_KEYS)
    {
      ret_value = ecma_create_iter_result_object (key_arg, ECMA_VALUE_FALSE);
    }
    else if (iterator_kind == ECMA_ITERATOR_VALUES)
    {
      ret_value = ecma_create_iter_result_object (value_arg, ECMA_VALUE_FALSE);
    }
    else
    {
      JERRY_ASSERT (iterator_kind == ECMA_ITERATOR_KEYS_VALUES);

      ecma_value_t entry_array_value;
      entry_array_value = ecma_create_array_from_iter_element (value_arg, key_arg);

      ret_value = ecma_create_iter_result_object (entry_array_value, ECMA_VALUE_FALSE);
      ecma_free_value (entry_array_value);
    }

    break;
  }

  return ret_value;
} /* ecma_op_container_iterator_next */

#endif /* ENABLED (JERRY_ES2015) */

/**
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_ES2015_BUILTIN_CONTAINER) */
