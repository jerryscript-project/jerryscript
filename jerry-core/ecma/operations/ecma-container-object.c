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
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-iterator-object.h"
#include "ecma-container-object.h"
#include "ecma-property-hashmap.h"
#include "ecma-objects.h"

#if ENABLED (JERRY_ES2015_BUILTIN_MAP)

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup \addtogroup ecmamaphelpers ECMA builtin map/set helper functions
 * @{
 */

/**
 * Creates an empty object for the map/set object's internal slot.
 *
 * Note: The created object is not registered to the GC.
 *
 * @return ecma value of the created object
 */
static ecma_value_t
ecma_op_container_create_internal_object (void)
{
  ecma_object_t *internal_object_p = ecma_alloc_object ();
  internal_object_p->type_flags_refs = (ECMA_OBJECT_TYPE_GENERAL | ECMA_OBJECT_FLAG_EXTENSIBLE | ECMA_OBJECT_REF_ONE);
  internal_object_p->property_list_or_bound_object_cp = JMEM_CP_NULL;
  internal_object_p->prototype_or_outer_reference_cp = JMEM_CP_NULL;

  return ecma_make_object_value (internal_object_p);
} /* ecma_op_container_create_internal_object */

/**
 * Handle calling [[Construct]] of built-in map/set like objects
 *
 * @return ecma value
 */
ecma_value_t
ecma_op_container_create (const ecma_value_t *arguments_list_p, /**< arguments list */
                          ecma_length_t arguments_list_len, /**< number of arguments */
                          bool is_set) /**< true - to perform Set operations
                                        *   false - to perform Map operations */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_object_t *proto_p = ecma_builtin_get (is_set ? ECMA_BUILTIN_ID_SET_PROTOTYPE : ECMA_BUILTIN_ID_MAP_PROTOTYPE);

  ecma_object_t *object_p = ecma_create_object (proto_p,
                                                sizeof (ecma_map_object_t),
                                                ECMA_OBJECT_TYPE_CLASS);

  ecma_map_object_t *map_obj_p = (ecma_map_object_t *) object_p;
  map_obj_p->header.u.class_prop.class_id = is_set ? LIT_MAGIC_STRING_SET_UL : LIT_MAGIC_STRING_MAP_UL;
  map_obj_p->header.u.class_prop.u.value = ecma_op_container_create_internal_object ();
  map_obj_p->size = 0;

  ecma_value_t set_value = ecma_make_object_value (object_p);

#if ENABLED (JERRY_ES2015_BUILTIN_SYMBOL)
  if (arguments_list_len == 0)
  {
    return set_value;
  }

  ecma_value_t iterable = arguments_list_p[0];

  if (ecma_is_value_undefined (iterable) || ecma_is_value_null (iterable))
  {
    return set_value;
  }

  ecma_value_t iter = ecma_op_get_iterator (iterable, ECMA_VALUE_EMPTY);

  if (ECMA_IS_VALUE_ERROR (iter))
  {
    ecma_deref_object (object_p);
    return iter;
  }

  while (true)
  {
    ecma_value_t next = ecma_op_iterator_step (iter);

    if (ECMA_IS_VALUE_ERROR (next))
    {
      ecma_free_value (iter);
      ecma_deref_object (object_p);
      return next;
    }

    if (ecma_is_value_false (next))
    {
      break;
    }

    ecma_value_t next_value = ecma_op_iterator_value (next);

    if (ECMA_IS_VALUE_ERROR (next_value))
    {
      ecma_free_value (next);
      ecma_free_value (iter);
      ecma_deref_object (object_p);
      return next_value;
    }

    ecma_value_t result;
    if (is_set)
    {
      result = ecma_op_container_set (set_value, next_value, next_value, is_set);
    }
    else
    {
      if (!ecma_is_value_object (next_value))
      {
        // TODO close the iterator when generator function will be supported
        ecma_free_value (next_value);
        ecma_free_value (next);
        ecma_free_value (iter);
        ecma_deref_object (object_p);
        return ecma_raise_type_error (ECMA_ERR_MSG ("Iterator value is not an object."));
      }

      ecma_object_t *next_object_p = ecma_get_object_from_value (next_value);

      ecma_value_t key = ecma_op_object_get (next_object_p, ecma_new_ecma_string_from_uint32 (0));

      if (ECMA_IS_VALUE_ERROR (key))
      {
        // TODO close the iterator when generator function will be supported
        ecma_free_value (next_value);
        ecma_free_value (next);
        ecma_free_value (iter);
        ecma_deref_object (object_p);
        return key;
      }

      ecma_value_t value = ecma_op_object_get (next_object_p, ecma_new_ecma_string_from_uint32 (1));

      if (ECMA_IS_VALUE_ERROR (value))
      {
        // TODO close the iterator when generator function will be supported
        ecma_free_value (next_value);
        ecma_free_value (key);
        ecma_free_value (next);
        ecma_free_value (iter);
        ecma_deref_object (object_p);
        return value;
      }

      result = ecma_op_container_set (set_value, key, value, is_set);

      ecma_free_value (key);
      ecma_free_value (value);
    }

    ecma_free_value (next_value);
    ecma_free_value (next);

    if (ECMA_IS_VALUE_ERROR (result))
    {
      // TODO close the iterator when generator function will be supported
      ecma_free_value (iter);
      ecma_deref_object (object_p);
      return result;
    }

    ecma_free_value (result);
  }

  ecma_free_value (iter);

#endif /* ENABLED (JERRY_ES2015_BUILTIN_SYMBOL) */

  return set_value;
} /* ecma_op_container_create */

/**
 * Get map/set object pointer
 *
 * Note:
 *   If the function returns with NULL, the error object has
 *   already set, and the caller must return with ECMA_VALUE_ERROR
 *
 * @return pointer to the map/set if this_arg is a valid map/set object
 *         NULL otherwise
 */
static ecma_map_object_t *
ecma_op_container_get_object (ecma_value_t this_arg, /**< this argument */
                              bool is_set) /**< true - to perform Set operations
                                            *   false - to perform Map operations */
{
  if (ecma_is_value_object (this_arg))
  {
    ecma_map_object_t *map_object_p = (ecma_map_object_t *) ecma_get_object_from_value (this_arg);

    if (ecma_get_object_type (&map_object_p->header.object) == ECMA_OBJECT_TYPE_CLASS
        && map_object_p->header.u.class_prop.class_id == (is_set ? LIT_MAGIC_STRING_SET_UL : LIT_MAGIC_STRING_MAP_UL))
    {
      return map_object_p;
    }
  }

  ecma_raise_type_error (ECMA_ERR_MSG (is_set ? "Expected a Set object." : "Expected a Map object."));
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

  if (ecma_is_value_object (key_arg))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (key_arg);
    ecma_string_t *key_string_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_MAP_KEY);
    ecma_property_t *property_p = ecma_find_named_property (obj_p, key_string_p);
    ecma_string_t *object_key_string;

    if (property_p == NULL)
    {
      object_key_string = ecma_new_map_key_string (key_arg);
      ecma_property_value_t *value_p = ecma_create_named_data_property (obj_p,
                                                                        key_string_p,
                                                                        ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                                        NULL);
      value_p->value = ecma_make_string_value (object_key_string);
    }
    else
    {
      object_key_string = ecma_get_string_from_value (ECMA_PROPERTY_VALUE_PTR (property_p)->value);

    }

    ecma_ref_ecma_string (object_key_string);
    return object_key_string;
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
 * Returns with the size of the map/set object.
 *
 * @return size of the map/set object as ecma-value.
 */
ecma_value_t
ecma_op_container_size (ecma_value_t this_arg, /**< this argument */
                        bool is_set) /**< true - to perform Set operations
                                      *   false - to perform Map operations */
{
  ecma_map_object_t *map_object_p = ecma_op_container_get_object (this_arg, is_set);

  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  return ecma_make_uint32_value (map_object_p->size);
} /* ecma_op_container_size */

/**
 * The generic map prototype object's 'get' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_container_get (ecma_value_t this_arg, /**< this argument */
                       ecma_value_t key_arg) /**< key argument */
{
  ecma_map_object_t *map_object_p = ecma_op_container_get_object (this_arg, false);

  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  if (map_object_p->size == 0)
  {
    return ECMA_VALUE_UNDEFINED;
  }

  ecma_object_t *internal_obj_p = ecma_get_object_from_value (map_object_p->header.u.class_prop.u.value);

  ecma_string_t *prop_name_p = ecma_op_container_to_key (key_arg);

  ecma_property_t *property_p = ecma_find_named_property (internal_obj_p, prop_name_p);

  ecma_deref_ecma_string (prop_name_p);

  if (property_p == NULL)
  {
    return ECMA_VALUE_UNDEFINED;
  }

  return ecma_copy_value (ECMA_PROPERTY_VALUE_PTR (property_p)->value);
} /* ecma_op_container_get */

/**
 * The generic map/set prototype object's 'has' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_container_has (ecma_value_t this_arg, /**< this argument */
                       ecma_value_t key_arg, /**< key argument */
                       bool is_set) /**< true - to perform Set operations
                                     *   false - to perform Map operations */
{
  ecma_map_object_t *map_object_p = ecma_op_container_get_object (this_arg, is_set);

  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  if (map_object_p->size == 0)
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_object_t *internal_obj_p = ecma_get_object_from_value (map_object_p->header.u.class_prop.u.value);

  ecma_string_t *prop_name_p = ecma_op_container_to_key (key_arg);

  ecma_property_t *property_p = ecma_find_named_property (internal_obj_p, prop_name_p);

  ecma_deref_ecma_string (prop_name_p);

  return ecma_make_boolean_value (property_p != NULL);
} /* ecma_op_container_has */

/**
 * The generic map prototype object's 'set' and set prototype object's 'add' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_container_set (ecma_value_t this_arg, /**< this argument */
                       ecma_value_t key_arg, /**< key argument */
                       ecma_value_t value_arg, /**< value argument */
                       bool is_set) /**< true - to perform Set operations
                                     *   false - to perform Map operations */
{
  ecma_map_object_t *map_object_p = ecma_op_container_get_object (this_arg, is_set);

  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_object_t *internal_obj_p = ecma_get_object_from_value (map_object_p->header.u.class_prop.u.value);

  ecma_string_t *prop_name_p = ecma_op_container_to_key (key_arg);

  ecma_property_t *property_p = ecma_find_named_property (internal_obj_p, prop_name_p);

  if (property_p == NULL)
  {
    ecma_property_value_t *value_p = ecma_create_named_data_property (internal_obj_p,
                                                                      prop_name_p,
                                                                      ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                                      NULL);
    value_p->value = ecma_copy_value_if_not_object (value_arg);
    map_object_p->size++;
  }
  else
  {
    ecma_named_data_property_assign_value (internal_obj_p, ECMA_PROPERTY_VALUE_PTR (property_p), value_arg);
  }

  ecma_deref_ecma_string (prop_name_p);
  ecma_ref_object ((ecma_object_t *) &map_object_p->header);
  return this_arg;
} /* ecma_op_container_set */

/**
 * Low-level function to clear all items from a map/set
 */
void
ecma_op_container_clear_map (ecma_map_object_t *map_object_p) /**< map object */
{
  ecma_object_t *object_p = ecma_get_object_from_value (map_object_p->header.u.class_prop.u.value);

  JERRY_ASSERT (object_p->type_flags_refs >= ECMA_OBJECT_REF_ONE);

  ecma_property_header_t *prop_iter_p = ecma_get_property_list (object_p);

  if (prop_iter_p != NULL && prop_iter_p->types[0] == ECMA_PROPERTY_TYPE_HASHMAP)
  {
    ecma_property_hashmap_free (object_p);
    prop_iter_p = ecma_get_property_list (object_p);
  }

  while (prop_iter_p != NULL)
  {
    JERRY_ASSERT (ECMA_PROPERTY_IS_PROPERTY_PAIR (prop_iter_p));

    /* Both cannot be deleted. */
    JERRY_ASSERT (prop_iter_p->types[0] != ECMA_PROPERTY_TYPE_DELETED
                  || prop_iter_p->types[1] != ECMA_PROPERTY_TYPE_DELETED);

    ecma_property_pair_t *prop_pair_p = (ecma_property_pair_t *) prop_iter_p;

    for (int i = 0; i < ECMA_PROPERTY_PAIR_ITEM_COUNT; i++)
    {
      ecma_property_t *property_p = (ecma_property_t *) (prop_iter_p->types + i);
      jmem_cpointer_t name_cp = prop_pair_p->names_cp[i];

      if (prop_iter_p->types[i] != ECMA_PROPERTY_TYPE_DELETED)
      {
        ecma_free_property (object_p, name_cp, property_p);
      }
    }

    prop_iter_p = ECMA_GET_POINTER (ecma_property_header_t,
                                    prop_iter_p->next_property_cp);

    ecma_dealloc_property_pair (prop_pair_p);
  }

  ecma_dealloc_object (object_p);
} /* ecma_op_container_clear_map */

/**
 * The generic map/set prototype object's 'forEach' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_container_foreach (ecma_value_t this_arg, /**< this argument */
                           ecma_value_t predicate, /**< callback function */
                           ecma_value_t predicate_this_arg, /**< this argument for
                                                             *   invoke predicate */
                           bool is_set) /**< true - to perform Set operations
                                         *   false - to perform Map operations */
{
  ecma_map_object_t *map_object_p = ecma_op_container_get_object (this_arg, is_set);

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

  ecma_object_t *internal_obj_p = ecma_get_object_from_value (map_object_p->header.u.class_prop.u.value);

  ecma_collection_header_t *props_p = ecma_op_object_get_property_names (internal_obj_p, ECMA_LIST_NO_OPTS);

  ecma_value_t *ecma_value_p = ecma_collection_iterator_init (props_p);

  ecma_value_t ret_value = ECMA_VALUE_UNDEFINED;

  while (ecma_value_p != NULL)
  {
    ecma_string_t *prop_name_p = ecma_get_prop_name_from_value (*ecma_value_p);
    ecma_property_t *property_p = ecma_find_named_property (internal_obj_p, prop_name_p);

    if (property_p == NULL)
    {
      ecma_value_p = ecma_collection_iterator_next (ecma_value_p);
      continue;
    }

    ecma_value_t value = ecma_copy_value (ECMA_PROPERTY_VALUE_PTR (property_p)->value);
    ecma_value_t key_arg;

    if (ecma_prop_name_is_map_key (prop_name_p))
    {
      key_arg = prop_name_p->u.value;
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
        key_arg = *ecma_value_p;
      }
    }

    ecma_value_t call_args[] = { value, is_set ? value : key_arg };

    ecma_value_t call_value = ecma_op_function_call (func_object_p, predicate_this_arg, call_args, 2);

    ecma_free_value (value);

    if (ECMA_IS_VALUE_ERROR (call_value))
    {
      ret_value = call_value;
      break;
    }

    ecma_free_value (call_value);

    ecma_value_p = ecma_collection_iterator_next (ecma_value_p);
  }

  ecma_free_values_collection (props_p, 0);

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
                         bool is_set) /**< true - to perform Set operations
                                       *   false - to perform Map operations */
{
  ecma_map_object_t *map_object_p = ecma_op_container_get_object (this_arg, is_set);

  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_op_container_clear_map (map_object_p);

  map_object_p->header.u.class_prop.u.value = ecma_op_container_create_internal_object ();
  map_object_p->size = 0;

  return ECMA_VALUE_UNDEFINED;
} /* ecma_op_container_clear */

/**
 * The generic map/set prototype object's 'delete' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_container_delete (ecma_value_t this_arg, /**< this argument */
                          ecma_value_t key_arg, /**< key argument */
                          bool is_set) /**< true - to perform Set operations
                                        *   false - to perform Map operations */
{
  ecma_map_object_t *map_object_p = ecma_op_container_get_object (this_arg, is_set);

  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_object_t *internal_obj_p = ecma_get_object_from_value (map_object_p->header.u.class_prop.u.value);

  ecma_string_t *prop_name_p = ecma_op_container_to_key (key_arg);

  ecma_property_t *property_p = ecma_find_named_property (internal_obj_p, prop_name_p);

  ecma_deref_ecma_string (prop_name_p);

  if (property_p == NULL)
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_delete_property (internal_obj_p, ECMA_PROPERTY_VALUE_PTR (property_p));
  map_object_p->size--;

  return ECMA_VALUE_TRUE;
} /* ecma_op_container_delete */

/**
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_ES2015_BUILTIN_MAP) */
