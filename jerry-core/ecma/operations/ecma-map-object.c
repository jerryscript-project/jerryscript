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

#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-map-object.h"
#include "ecma-objects.h"

#ifndef CONFIG_DISABLE_ES2015_MAP_BUILTIN

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup \addtogroup ecmamaphelpers ECMA builtin map helper functions
 * @{
 */

JERRY_STATIC_ASSERT (ECMA_MAP_OBJECT_ITEM_COUNT == 3,
                     ecma_map_object_item_count_must_be_3);

/**
 * Handle calling [[Construct]] of built-in map like objects
 *
 * @return ecma value
 */
ecma_value_t
ecma_op_map_create (const ecma_value_t *arguments_list_p, /**< arguments list */
                    ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_MAP_PROTOTYPE);
  ecma_object_t *object_p = ecma_create_object (prototype_obj_p,
                                                sizeof (ecma_map_object_t),
                                                ECMA_OBJECT_TYPE_CLASS);

  ecma_deref_object (prototype_obj_p);

  ecma_map_object_t *map_object_p = (ecma_map_object_t *) object_p;
  map_object_p->header.u.class_prop.class_id = LIT_MAGIC_STRING_MAP_UL;
  map_object_p->header.u.class_prop.extra_info = 0;
  map_object_p->header.u.class_prop.u.length = 0;
  map_object_p->first_chunk_cp = ECMA_NULL_POINTER;
  map_object_p->last_chunk_cp = ECMA_NULL_POINTER;

  return ecma_make_object_value (object_p);
} /* ecma_op_map_create */

/**
 * Get map object pointer
 *
 * Note:
 *   If the function returns with NULL, the error object has
 *   already set, and the caller must return with ECMA_VALUE_ERROR
 *
 * @return pointer to the map if this_arg is a valid map object
 *         NULL otherwise
 */
static ecma_map_object_t *
ecma_op_map_get_object (ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_value_object (this_arg))
  {
    ecma_map_object_t *map_object_p = (ecma_map_object_t *) ecma_get_object_from_value (this_arg);

    if (ecma_get_object_type (&map_object_p->header.object) == ECMA_OBJECT_TYPE_CLASS
        && map_object_p->header.u.class_prop.class_id == LIT_MAGIC_STRING_MAP_UL)
    {
      return map_object_p;
    }
  }

  ecma_raise_type_error (ECMA_ERR_MSG ("Expected a Map object."));
  return NULL;
} /* ecma_op_map_get_object */

/**
 * Returns with the size of the map object.
 *
 * @return size of the map object as ecma-value.
 */
ecma_value_t
ecma_op_map_size (ecma_value_t this_arg) /**< this argument */
{
  ecma_map_object_t *map_object_p = ecma_op_map_get_object (this_arg);
  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  return ecma_make_uint32_value (map_object_p->header.u.class_prop.u.length);
} /* ecma_op_map_size */

/**
 * Linear search for the value in the map storage
 *
 * @return pointer to value if key is found
 *         NULL otherwise
 */
static ecma_value_t *
ecma_builtin_map_search (jmem_cpointer_t first_chunk_cp, /**< first chunk */
                         ecma_value_t key) /**< key to search */
{
  if (JERRY_UNLIKELY (first_chunk_cp == ECMA_NULL_POINTER))
  {
    return NULL;
  }

  ecma_map_object_chunk_t *chunk_p = ECMA_GET_NON_NULL_POINTER (ecma_map_object_chunk_t,
                                                                first_chunk_cp);

  bool is_direct = true;
  ecma_string_t *key_str_p = NULL;
  ecma_number_t key_float = 0;

  if (ecma_is_value_non_direct_string (key))
  {
    key_str_p = ecma_get_string_from_value (key);
    is_direct = false;
  }
  else if (ecma_is_value_float_number (key))
  {
    key_float = ecma_get_float_from_value (key);
    is_direct = false;
  }

  ecma_value_t *item_p = chunk_p->items;
  ecma_value_t last_key = ECMA_VALUE_ARRAY_HOLE;

  while (true)
  {
    ecma_value_t item = *item_p++;

    if (JERRY_UNLIKELY (item == ECMA_VALUE_ARRAY_HOLE))
    {
      JERRY_ASSERT (last_key == ECMA_VALUE_ARRAY_HOLE);
      continue;
    }

    if (JERRY_UNLIKELY (ecma_is_value_pointer (item)))
    {
      item_p = (ecma_value_t *) ecma_get_pointer_from_value (item);

      if (item_p == NULL)
      {
        JERRY_ASSERT (last_key == ECMA_VALUE_ARRAY_HOLE);
        return NULL;
      }

      JERRY_ASSERT (!ecma_is_value_pointer (*item_p));
      continue;
    }

    if (last_key == ECMA_VALUE_ARRAY_HOLE)
    {
      last_key = item;
    }
    else
    {
      if (JERRY_LIKELY (is_direct))
      {
        if (key == last_key)
        {
          return item_p - 1;
        }
      }
      else if (key_str_p != NULL)
      {
        if (ecma_is_value_non_direct_string (last_key)
            && ecma_compare_ecma_non_direct_strings (key_str_p, ecma_get_string_from_value (last_key)))
        {
          return item_p - 1;
        }
      }
      else if (ecma_is_value_float_number (last_key)
               && ecma_get_float_from_value (last_key) == key_float)
      {
        return item_p - 1;
      }

      last_key = ECMA_VALUE_ARRAY_HOLE;
    }
  }
} /* ecma_builtin_map_search */

/**
 * The generic map prototype object's 'get' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_map_get (ecma_value_t this_arg, /**< this argument */
                 ecma_value_t key_arg) /**< key argument */
{
  ecma_map_object_t *map_object_p = ecma_op_map_get_object (this_arg);
  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t *value_p = ecma_builtin_map_search (map_object_p->first_chunk_cp, key_arg);

  if (value_p == NULL)
  {
    return ECMA_VALUE_UNDEFINED;
  }

  return ecma_copy_value (*value_p);
} /* ecma_op_map_get */

/**
 * The generic map prototype object's 'has' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_map_has (ecma_value_t this_arg, /**< this argument */
                 ecma_value_t key_arg) /**< key argument */
{
  ecma_map_object_t *map_object_p = ecma_op_map_get_object (this_arg);
  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  return ecma_make_boolean_value (ecma_builtin_map_search (map_object_p->first_chunk_cp, key_arg) != NULL);
} /* ecma_op_map_has */

/**
 * The generic map prototype object's 'set' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_map_set (ecma_value_t this_arg, /**< this argument */
                 ecma_value_t key_arg, /**< key argument */
                 ecma_value_t value_arg) /**< value argument */
{
  ecma_map_object_t *map_object_p = ecma_op_map_get_object (this_arg);
  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t *value_p = ecma_builtin_map_search (map_object_p->first_chunk_cp, key_arg);

  if (value_p == NULL)
  {
    ecma_value_t *key_p = NULL;
    ecma_map_object_chunk_t *last_chunk_p = ECMA_GET_POINTER (ecma_map_object_chunk_t,
                                                              map_object_p->last_chunk_cp);

    if (last_chunk_p != NULL)
    {
      if (last_chunk_p->items[2] == ECMA_VALUE_ARRAY_HOLE)
      {
        key_p = last_chunk_p->items + 2;

        if (last_chunk_p->items[1] == ECMA_VALUE_ARRAY_HOLE)
        {
          key_p = last_chunk_p->items + 1;
          value_p = last_chunk_p->items + 2;
        }
      }
    }

    if (key_p == NULL || value_p == NULL)
    {
      size_t size = sizeof (ecma_map_object_chunk_t);
      ecma_map_object_chunk_t *new_chunk_p = (ecma_map_object_chunk_t *) jmem_heap_alloc_block (size);

      new_chunk_p->items[ECMA_MAP_OBJECT_ITEM_COUNT] = ecma_make_pointer_value (NULL);

      for (int i = 0; i < ECMA_MAP_OBJECT_ITEM_COUNT; i++)
      {
        new_chunk_p->items[i] = ECMA_VALUE_ARRAY_HOLE;
      }

      ECMA_SET_NON_NULL_POINTER (map_object_p->last_chunk_cp, new_chunk_p);

      if (last_chunk_p == NULL)
      {
        map_object_p->first_chunk_cp = map_object_p->last_chunk_cp;
      }
      else
      {
        last_chunk_p->items[ECMA_MAP_OBJECT_ITEM_COUNT] = ecma_make_pointer_value (new_chunk_p);
      }

      if (key_p == NULL)
      {
        JERRY_ASSERT (value_p == NULL);
        key_p = new_chunk_p->items + 0;
        value_p = new_chunk_p->items + 1;
      }
      else
      {
        value_p = new_chunk_p->items + 0;
      }
    }

    *key_p = ecma_copy_value_if_not_object (key_arg);
    map_object_p->header.u.class_prop.u.length++;
  }
  else
  {
    ecma_free_value_if_not_object (*value_p);
  }

  *value_p = ecma_copy_value_if_not_object (value_arg);

  ecma_ref_object (&map_object_p->header.object);
  return this_arg;
} /* ecma_op_map_set */

/**
 * Low-level function to clear all items from a map
 */
void
ecma_op_map_clear_map (ecma_map_object_t *map_object_p) /**< map object */
{
  JERRY_ASSERT (ecma_get_object_type (&map_object_p->header.object) == ECMA_OBJECT_TYPE_CLASS
                && (map_object_p->header.u.class_prop.class_id == LIT_MAGIC_STRING_MAP_UL));

  jmem_cpointer_t first_chunk_cp = map_object_p->first_chunk_cp;

  if (JERRY_UNLIKELY (first_chunk_cp == ECMA_NULL_POINTER))
  {
    return;
  }

  ecma_map_object_chunk_t *chunk_p = ECMA_GET_NON_NULL_POINTER (ecma_map_object_chunk_t,
                                                                first_chunk_cp);

  do
  {
    ecma_value_t *current_p = chunk_p->items;
    ecma_value_t *last_p = current_p + ECMA_MAP_OBJECT_ITEM_COUNT;

    do
    {
      ecma_free_value_if_not_object (*current_p++);
    }
    while (current_p < last_p);

    ecma_value_t next = *current_p;

    jmem_heap_free_block (chunk_p, sizeof (ecma_map_object_chunk_t));

    chunk_p = (ecma_map_object_chunk_t *) ecma_get_pointer_from_value (next);
  }
  while (chunk_p != NULL);

  map_object_p->header.u.class_prop.u.length = 0;
  map_object_p->first_chunk_cp = ECMA_NULL_POINTER;
  map_object_p->last_chunk_cp = ECMA_NULL_POINTER;
} /* ecma_op_map_clear_map */

/**
 * The Map prototype object's 'clear' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_map_clear (ecma_value_t this_arg) /**< this argument */
{
  /* WeakMap does not have a clear method. */
  ecma_map_object_t *map_object_p = ecma_op_map_get_object (this_arg);
  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_op_map_clear_map (map_object_p);
  return ECMA_VALUE_UNDEFINED;
} /* ecma_op_map_clear */

/**
 * Deletes the current chunk if it is filled with ECMA_VALUE_ARRAY_HOLE.
 *
 * @return next chunk if the chunk is deleted, NULL otherwise
 */
static ecma_map_object_chunk_t *
ecma_op_map_delete_chunk (ecma_map_object_t *map_object_p, /**< map object */
                          ecma_map_object_chunk_t *chunk_p, /**< current chunk */
                          ecma_map_object_chunk_t *prev_chunk_p) /**< previous chunk */
{
  for (int i = 0; i < ECMA_MAP_OBJECT_ITEM_COUNT; i++)
  {
    JERRY_ASSERT (!ecma_is_value_pointer (chunk_p->items[i]));

    if (chunk_p->items[i] != ECMA_VALUE_ARRAY_HOLE)
    {
      return NULL;
    }
  }

  ecma_value_t next_chunk = chunk_p->items[ECMA_MAP_OBJECT_ITEM_COUNT];
  ecma_map_object_chunk_t *next_chunk_p = (ecma_map_object_chunk_t *) ecma_get_pointer_from_value (next_chunk);

  jmem_heap_free_block (chunk_p, sizeof (ecma_map_object_chunk_t));

  if (prev_chunk_p != NULL)
  {
    prev_chunk_p->items[ECMA_MAP_OBJECT_ITEM_COUNT] = ecma_make_pointer_value (next_chunk_p);

    if (next_chunk_p == NULL)
    {
      JERRY_ASSERT (map_object_p->first_chunk_cp != map_object_p->last_chunk_cp);
      JERRY_ASSERT (ECMA_GET_NON_NULL_POINTER (ecma_map_object_chunk_t, map_object_p->last_chunk_cp) == chunk_p);

      ECMA_SET_POINTER (map_object_p->last_chunk_cp, prev_chunk_p);
    }
    return next_chunk_p;
  }

  if (next_chunk_p == NULL)
  {
    JERRY_ASSERT (map_object_p->first_chunk_cp == map_object_p->last_chunk_cp);
    JERRY_ASSERT (ECMA_GET_NON_NULL_POINTER (ecma_map_object_chunk_t, map_object_p->last_chunk_cp) == chunk_p);

    map_object_p->first_chunk_cp = ECMA_NULL_POINTER;
    map_object_p->last_chunk_cp = ECMA_NULL_POINTER;
    return next_chunk_p;
  }

  ECMA_SET_POINTER (map_object_p->first_chunk_cp, next_chunk_p);
  return next_chunk_p;
} /* ecma_op_map_delete_chunk */

/**
 * The generic map prototype object's 'delete' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_map_delete (ecma_value_t this_arg, /**< this argument */
                    ecma_value_t key_arg) /**< key argument */
{
  ecma_map_object_t *map_object_p = ecma_op_map_get_object (this_arg);
  if (map_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  if (JERRY_UNLIKELY (map_object_p->first_chunk_cp == ECMA_NULL_POINTER))
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_map_object_chunk_t *chunk_p = ECMA_GET_NON_NULL_POINTER (ecma_map_object_chunk_t,
                                                                map_object_p->first_chunk_cp);

  bool is_direct = true;
  ecma_string_t *key_str_p = NULL;
  ecma_number_t key_float = 0;

  if (ecma_is_value_non_direct_string (key_arg))
  {
    key_str_p = ecma_get_string_from_value (key_arg);
    is_direct = false;
  }
  else if (ecma_is_value_float_number (key_arg))
  {
    key_float = ecma_get_float_from_value (key_arg);
    is_direct = false;
  }

  ecma_map_object_chunk_t *prev_chunk_p = NULL;
  ecma_value_t *item_p = chunk_p->items;
  bool is_key = true;

  while (true)
  {
    ecma_value_t item = *item_p++;

    if (JERRY_UNLIKELY (item == ECMA_VALUE_ARRAY_HOLE))
    {
      JERRY_ASSERT (is_key);
      continue;
    }

    if (JERRY_UNLIKELY (ecma_is_value_pointer (item)))
    {
      prev_chunk_p = chunk_p;
      chunk_p = (ecma_map_object_chunk_t *) ecma_get_pointer_from_value (item);

      if (chunk_p == NULL)
      {
        JERRY_ASSERT (is_key);
        return ECMA_VALUE_FALSE;
      }

      item_p = chunk_p->items;

      JERRY_ASSERT (!ecma_is_value_pointer (*item_p));
      continue;
    }

    if (is_key)
    {
      if (JERRY_LIKELY (is_direct))
      {
        if (key_arg == item)
        {
          break;
        }
      }
      else if (key_str_p != NULL)
      {
        if (ecma_is_value_non_direct_string (item)
            && ecma_compare_ecma_non_direct_strings (key_str_p, ecma_get_string_from_value (item)))
        {
          break;
        }
      }
      else if (ecma_is_value_float_number (item)
               && ecma_get_float_from_value (item) == key_float)
      {
        break;
      }
    }

    is_key = !is_key;
  }

  map_object_p->header.u.class_prop.u.length--;

  item_p -= 1;
  ecma_free_value_if_not_object (item_p[0]);
  item_p[0] = ECMA_VALUE_ARRAY_HOLE;

  if ((item_p - chunk_p->items) < ECMA_MAP_OBJECT_ITEM_COUNT - 1)
  {
    JERRY_ASSERT (!ecma_is_value_pointer (item_p[1]));

    ecma_free_value_if_not_object (item_p[1]);
    item_p[1] = ECMA_VALUE_ARRAY_HOLE;

    ecma_op_map_delete_chunk (map_object_p, chunk_p, prev_chunk_p);
    return ECMA_VALUE_TRUE;
  }

  ecma_map_object_chunk_t *next_chunk_p = ecma_op_map_delete_chunk (map_object_p, chunk_p, prev_chunk_p);

  if (next_chunk_p == NULL)
  {
    prev_chunk_p = chunk_p;

    ecma_value_t next_chunk = chunk_p->items[ECMA_MAP_OBJECT_ITEM_COUNT];
    next_chunk_p = (ecma_map_object_chunk_t *) ecma_get_pointer_from_value (next_chunk);
  }

  ecma_free_value_if_not_object (next_chunk_p->items[0]);
  next_chunk_p->items[0] = ECMA_VALUE_ARRAY_HOLE;

  ecma_op_map_delete_chunk (map_object_p, next_chunk_p, prev_chunk_p);

  return ECMA_VALUE_TRUE;
} /* ecma_op_map_delete */

/**
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_ES2015_MAP_BUILTIN */
