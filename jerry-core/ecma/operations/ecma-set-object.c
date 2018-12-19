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
#include "ecma-set-object.h"
#include "ecma-objects.h"
#include "ecma-comparison.h"

#ifndef CONFIG_DISABLE_ES2015_SET_BUILTIN

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup \addtogroup ecmasethelpers ECMA builtin set helper functions
 * @{
 */

JERRY_STATIC_ASSERT (ECMA_SET_OBJECT_ITEM_COUNT == ECMA_COLLECTION_CHUNK_ITEMS,
                     ecma_set_object_item_count_must_be_3);

/**
 * Handle calling [[Construct]] of built-in set like objects
 *
 * @return ecma value
 */
ecma_value_t
ecma_op_set_create (const ecma_value_t *arguments_list_p, /**< arguments list */
                    ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_SET_PROTOTYPE);
  ecma_object_t *object_p = ecma_create_object (prototype_obj_p,
                                                sizeof (ecma_set_object_t),
                                                ECMA_OBJECT_TYPE_CLASS);

  ecma_set_object_t *set_object_p = (ecma_set_object_t *) object_p;
  set_object_p->header.u.class_prop.class_id = LIT_MAGIC_STRING_SET_UL;
  set_object_p->header.u.class_prop.extra_info = 0;
  set_object_p->header.u.class_prop.u.length = 0;
  set_object_p->first_chunk_cp = ECMA_NULL_POINTER;
  set_object_p->last_chunk_cp = ECMA_NULL_POINTER;

  return ecma_make_object_value (object_p);
} /* ecma_op_set_create */

/**
 * Get set object pointer
 *
 * Note:
 *   If the function returns with NULL, the error object has
 *   already set, and the caller must return with ECMA_VALUE_ERROR
 *
 * @return pointer to the set if this_arg is a valid set object
 *         NULL otherwise
 */
ecma_set_object_t *
ecma_op_set_get_object (ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_value_object (this_arg))
  {
    ecma_set_object_t *set_object_p = (ecma_set_object_t *) ecma_get_object_from_value (this_arg);

    if (ecma_get_object_type (&set_object_p->header.object) == ECMA_OBJECT_TYPE_CLASS
        && set_object_p->header.u.class_prop.class_id == LIT_MAGIC_STRING_SET_UL)
    {
      return set_object_p;
    }
  }

  ecma_raise_type_error (ECMA_ERR_MSG ("Expected a Set object."));
  return NULL;
} /* ecma_op_set_get_object */

/**
 * Returns with the size of the set object.
 *
 * @return size of the set object as ecma-value.
 */
ecma_value_t
ecma_op_set_size (ecma_value_t this_arg) /**< this argument */
{
  ecma_set_object_t *set_object_p = ecma_op_set_get_object (this_arg);
  if (set_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  return ecma_make_uint32_value (set_object_p->header.u.class_prop.u.length);
} /* ecma_op_set_size */

/**
 * Linear search for the value in the set storage
 *
 * @return pointer to value if key is found
 *         NULL otherwise
 */
static ecma_value_t *
ecma_builtin_set_search (jmem_cpointer_t first_chunk_cp, /**< first chunk */
                         ecma_value_t value) /**< value to search */
{
  if (JERRY_UNLIKELY (first_chunk_cp == ECMA_NULL_POINTER))
  {
    return NULL;
  }

  ecma_set_object_chunk_t *chunk_p = ECMA_GET_NON_NULL_POINTER (ecma_set_object_chunk_t,
                                                                first_chunk_cp);


  ecma_value_t *ecma_value_p = chunk_p->items;

  while (ecma_value_p != NULL)
  {
    /* TODO: Use SameValueZero */
    if (ecma_op_abstract_equality_compare (*ecma_value_p, value) == ECMA_VALUE_TRUE)
    {
      return ecma_value_p;
    }

    ecma_value_p = ecma_collection_iterator_next (ecma_value_p);
  }

  return NULL;
} /* ecma_builtin_set_search */

/**
 * The Set prototype object's 'has' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_set_has (ecma_value_t this_arg, /**< this argument */
                 ecma_value_t value) /**< value to check */
{
  ecma_set_object_t *set_object_p = ecma_op_set_get_object (this_arg);
  if (set_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  return ecma_make_boolean_value (ecma_builtin_set_search (set_object_p->first_chunk_cp, value) != NULL);
} /* ecma_op_set_has */

/**
 * The Set prototype object's 'add' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_set_add (ecma_value_t this_arg, /**< this argument */
                 ecma_value_t value) /**< value to add */
{
  ecma_set_object_t *set_object_p = ecma_op_set_get_object (this_arg);
  if (set_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t *value_p = ecma_builtin_set_search (set_object_p->first_chunk_cp, value);
  ecma_ref_object (&set_object_p->header.object);

  if (value_p == NULL)
  {
    ecma_collection_header_t header =
    {
      .item_count = set_object_p->header.u.class_prop.u.length,
      .first_chunk_cp = set_object_p->first_chunk_cp,
      .last_chunk_cp = set_object_p->last_chunk_cp
    };

    ecma_value_t *ecma_value_p = NULL;

    if (set_object_p->last_chunk_cp != JMEM_CP_NULL)
    {
      ecma_set_object_chunk_t *last_chunk_p = ECMA_GET_POINTER (ecma_set_object_chunk_t,
                                                                set_object_p->last_chunk_cp);

      ecma_value_p = last_chunk_p->items;

      while (ecma_value_p != NULL)
      {
        if (*ecma_value_p == ECMA_VALUE_ARRAY_HOLE)
        {
          *ecma_value_p = value;
          return this_arg;
        }

        ecma_value_p = ecma_collection_iterator_next (ecma_value_p);
      }
    }

    if (ecma_value_p == NULL)
    {
      ecma_append_to_values_collection (&header, value, ECMA_COLLECTION_NO_REF_OBJECTS);
      set_object_p->header.u.class_prop.u.length++;
      set_object_p->first_chunk_cp = header.first_chunk_cp;
      set_object_p->last_chunk_cp = header.last_chunk_cp;
    }
  }

  return this_arg;
} /* ecma_op_set_add */

/**
 * Low-level function to clear all items from a set
 */
void
ecma_op_set_clear_set (ecma_set_object_t *set_object_p) /**< set object */
{
  JERRY_ASSERT (ecma_get_object_type (&set_object_p->header.object) == ECMA_OBJECT_TYPE_CLASS
                && (set_object_p->header.u.class_prop.class_id == LIT_MAGIC_STRING_SET_UL));

  jmem_cpointer_t first_chunk_cp = set_object_p->first_chunk_cp;

  if (JERRY_UNLIKELY (first_chunk_cp == ECMA_NULL_POINTER))
  {
    return;
  }

  ecma_set_object_chunk_t *chunk_p = ECMA_GET_NON_NULL_POINTER (ecma_set_object_chunk_t,
                                                                first_chunk_cp);

  do
  {
    ecma_value_t *current_p = chunk_p->items;

    while (!ecma_is_value_pointer (*current_p))
    {
      ecma_free_value_if_not_object (*current_p++);
    }

    ecma_value_t next = *current_p;

    jmem_heap_free_block (chunk_p, sizeof (ecma_set_object_chunk_t));

    chunk_p = (ecma_set_object_chunk_t *) ecma_get_pointer_from_value (next);
  }
  while (chunk_p != NULL);

  set_object_p->header.u.class_prop.u.length = 0;
  set_object_p->first_chunk_cp = ECMA_NULL_POINTER;
  set_object_p->last_chunk_cp = ECMA_NULL_POINTER;
} /* ecma_op_set_clear_set */

/**
 * The Set prototype object's 'clear' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_set_clear (ecma_value_t this_arg) /**< this argument */
{
  ecma_set_object_t *set_object_p = ecma_op_set_get_object (this_arg);
  if (set_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_op_set_clear_set (set_object_p);
  return ECMA_VALUE_UNDEFINED;
} /* ecma_op_set_clear */

/**
 * Deletes the current chunk if it is filled with ECMA_VALUE_ARRAY_HOLE.
 *
 * @return next chunk if the chunk is deleted, NULL otherwise
 */
static ecma_set_object_chunk_t *
ecma_op_set_delete_chunk (ecma_set_object_t *set_object_p, /**< set object */
                          ecma_set_object_chunk_t *chunk_p, /**< current chunk */
                          ecma_set_object_chunk_t *prev_chunk_p) /**< previous chunk */
{
  for (int i = 0; i < ECMA_SET_OBJECT_ITEM_COUNT; i++)
  {
    JERRY_ASSERT (!ecma_is_value_pointer (chunk_p->items[i]));

    if (chunk_p->items[i] != ECMA_VALUE_ARRAY_HOLE)
    {
      return NULL;
    }
  }

  ecma_value_t next_chunk = chunk_p->items[ECMA_SET_OBJECT_ITEM_COUNT];
  ecma_set_object_chunk_t *next_chunk_p = (ecma_set_object_chunk_t *) ecma_get_pointer_from_value (next_chunk);

  jmem_heap_free_block (chunk_p, sizeof (ecma_set_object_chunk_t));

  if (prev_chunk_p != NULL)
  {
    prev_chunk_p->items[ECMA_SET_OBJECT_ITEM_COUNT] = ecma_make_pointer_value (next_chunk_p);

    if (next_chunk_p == NULL)
    {
      JERRY_ASSERT (set_object_p->first_chunk_cp != set_object_p->last_chunk_cp);
      JERRY_ASSERT (ECMA_GET_NON_NULL_POINTER (ecma_set_object_chunk_t, set_object_p->last_chunk_cp) == chunk_p);

      ECMA_SET_POINTER (set_object_p->last_chunk_cp, prev_chunk_p);
    }
    return next_chunk_p;
  }

  if (next_chunk_p == NULL)
  {
    JERRY_ASSERT (set_object_p->first_chunk_cp == set_object_p->last_chunk_cp);
    JERRY_ASSERT (ECMA_GET_NON_NULL_POINTER (ecma_set_object_chunk_t, set_object_p->last_chunk_cp) == chunk_p);

    set_object_p->first_chunk_cp = ECMA_NULL_POINTER;
    set_object_p->last_chunk_cp = ECMA_NULL_POINTER;
    return next_chunk_p;
  }

  ECMA_SET_POINTER (set_object_p->first_chunk_cp, next_chunk_p);
  return next_chunk_p;
} /* ecma_op_set_delete_chunk */

/**
 * The Set prototype object's 'delete' routine
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_set_delete (ecma_value_t this_arg, /**< this argument */
                    ecma_value_t value) /**< value to delete */
{
  ecma_set_object_t *set_object_p = ecma_op_set_get_object (this_arg);
  if (set_object_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  if (JERRY_UNLIKELY (set_object_p->first_chunk_cp == ECMA_NULL_POINTER))
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_set_object_chunk_t *chunk_p = ECMA_GET_NON_NULL_POINTER (ecma_set_object_chunk_t,
                                                                set_object_p->first_chunk_cp);

  ecma_set_object_chunk_t *prev_chunk_p = NULL;
  ecma_value_t *ecma_value_p = chunk_p->items;
  uint8_t array_hole_count = 0;

  while (ecma_value_p != NULL)
  {
    if (ecma_op_abstract_equality_compare (*ecma_value_p, value) == ECMA_VALUE_TRUE)
    {
      break;
    }
    else if (JERRY_UNLIKELY (*ecma_value_p == ECMA_VALUE_ARRAY_HOLE))
    {
      array_hole_count++;
    }

    ecma_value_p++;

    if (JERRY_UNLIKELY (ecma_is_value_pointer (*ecma_value_p)))
    {
      prev_chunk_p = chunk_p;
      chunk_p = (ecma_collection_chunk_t *) ecma_get_pointer_from_value (*ecma_value_p);
      ecma_value_p = chunk_p->items;
      array_hole_count = 0;

      JERRY_ASSERT (ecma_value_p == NULL || !ecma_is_value_pointer (*ecma_value_p));
    }
  }

  if (ecma_value_p == NULL)
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_free_value_if_not_object (*ecma_value_p);

  *ecma_value_p = ECMA_VALUE_ARRAY_HOLE;
  set_object_p->header.u.class_prop.u.length--;

  if (array_hole_count == ECMA_SET_OBJECT_ITEM_COUNT - 1)
  {
    ecma_op_set_delete_chunk (set_object_p, chunk_p, prev_chunk_p);
  }

  return ECMA_VALUE_TRUE;
} /* ecma_op_set_delete */

/**
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_ES2015_SET_BUILTIN */
