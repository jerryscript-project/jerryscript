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
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "jrt.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

/**
 * The type of ecma error and ecma collection chunk must be the same.
 */
JERRY_STATIC_ASSERT (ECMA_TYPE_ERROR == ECMA_TYPE_COLLECTION_CHUNK,
                     ecma_type_error_must_be_the_same_as_ecma_type_collection_chunk);

/**
 * Allocate a collection of ecma values.
 *
 * @return pointer to the collection's header
 */
ecma_collection_header_t *
ecma_new_values_collection (const ecma_value_t values_buffer[], /**< ecma values */
                            ecma_length_t values_number, /**< number of ecma values */
                            bool do_ref_if_object) /**< if the value is object value,
                                                        increase reference counter of the object */
{
  JERRY_ASSERT (values_buffer != NULL || values_number == 0);

  ecma_collection_header_t *header_p;
  header_p = (ecma_collection_header_t *) jmem_pools_alloc (sizeof (ecma_collection_header_t));

  header_p->item_count = values_number;
  header_p->first_chunk_cp = ECMA_NULL_POINTER;
  header_p->last_chunk_cp = ECMA_NULL_POINTER;

  if (values_number == 0)
  {
    return header_p;
  }

  ecma_collection_chunk_t *current_chunk_p = NULL;
  int current_chunk_index = ECMA_COLLECTION_CHUNK_ITEMS;

  for (ecma_length_t value_index = 0;
       value_index < values_number;
       value_index++)
  {
    if (unlikely (current_chunk_index >= ECMA_COLLECTION_CHUNK_ITEMS))
    {
      ecma_collection_chunk_t *next_chunk_p;
      next_chunk_p = (ecma_collection_chunk_t *) jmem_pools_alloc (sizeof (ecma_collection_chunk_t));

      if (header_p->last_chunk_cp == ECMA_NULL_POINTER)
      {
        ECMA_SET_POINTER (header_p->first_chunk_cp, next_chunk_p);
        header_p->last_chunk_cp = header_p->first_chunk_cp;
      }
      else
      {
        current_chunk_p->items[ECMA_COLLECTION_CHUNK_ITEMS] = ecma_make_collection_chunk_value (next_chunk_p);
        ECMA_SET_POINTER (header_p->last_chunk_cp, next_chunk_p);
      }

      current_chunk_p = next_chunk_p;
      current_chunk_index = 0;
    }

    ecma_value_t value = values_buffer[value_index];

    if (do_ref_if_object || !ecma_is_value_object (value))
    {
      value = ecma_copy_value (value);
    }

    current_chunk_p->items[current_chunk_index++] = value;
  }

  current_chunk_p->items[current_chunk_index] = ecma_make_collection_chunk_value (NULL);
  return header_p;
} /* ecma_new_values_collection */

/**
 * Free the collection of ecma values.
 */
void
ecma_free_values_collection (ecma_collection_header_t *header_p, /**< collection's header */
                             bool do_deref_if_object) /**< if the value is object value,
                                                           decrement reference counter of the object */
{
  ecma_collection_chunk_t *chunk_p = ECMA_GET_POINTER (ecma_collection_chunk_t,
                                                       header_p->first_chunk_cp);

  jmem_heap_free_block (header_p, sizeof (ecma_collection_header_t));

  if (chunk_p == NULL)
  {
    return;
  }

  do
  {
    ecma_value_t *item_p = chunk_p->items;

    JERRY_ASSERT (!ecma_is_value_collection_chunk (*item_p));

    do
    {
      if (do_deref_if_object)
      {
        ecma_free_value (*item_p);
      }
      else
      {
        ecma_free_value_if_not_object (*item_p);
      }

      item_p++;
    }
    while (!ecma_is_value_collection_chunk (*item_p));

    ecma_collection_chunk_t *next_chunk_p = ecma_get_collection_chunk_from_value (*item_p);

    jmem_heap_free_block (chunk_p, sizeof (ecma_collection_chunk_t));

    chunk_p = next_chunk_p;
  }
  while (chunk_p != NULL);
} /* ecma_free_values_collection */

/**
 * Append new value to ecma values collection
 */
void
ecma_append_to_values_collection (ecma_collection_header_t *header_p, /**< collection's header */
                                  ecma_value_t value, /**< ecma value to append */
                                  bool do_ref_if_object) /**< if the value is object value,
                                                              increase reference counter of the object */
{
  ecma_length_t item_index;
  ecma_collection_chunk_t *chunk_p;

  if (unlikely (header_p->item_count == 0))
  {
    item_index = 0;
    chunk_p = (ecma_collection_chunk_t *) jmem_heap_alloc_block (sizeof (ecma_collection_chunk_t));

    ECMA_SET_POINTER (header_p->first_chunk_cp, chunk_p);
    header_p->last_chunk_cp = header_p->first_chunk_cp;
  }
  else
  {
    item_index = header_p->item_count % ECMA_COLLECTION_CHUNK_ITEMS;

    chunk_p = ECMA_GET_NON_NULL_POINTER (ecma_collection_chunk_t,
                                         header_p->last_chunk_cp);

    if (unlikely (item_index == 0))
    {
      JERRY_ASSERT (ecma_is_value_collection_chunk (chunk_p->items[ECMA_COLLECTION_CHUNK_ITEMS])
                    && ecma_get_collection_chunk_from_value (chunk_p->items[ECMA_COLLECTION_CHUNK_ITEMS]) == NULL);

      ecma_collection_chunk_t *next_chunk_p;
      next_chunk_p = (ecma_collection_chunk_t *) jmem_heap_alloc_block (sizeof (ecma_collection_chunk_t));

      chunk_p->items[ECMA_COLLECTION_CHUNK_ITEMS] = ecma_make_collection_chunk_value (next_chunk_p);
      ECMA_SET_POINTER (header_p->last_chunk_cp, next_chunk_p);

      chunk_p = next_chunk_p;
    }
    else
    {
      JERRY_ASSERT (ecma_is_value_collection_chunk (chunk_p->items[item_index])
                    && ecma_get_collection_chunk_from_value (chunk_p->items[item_index]) == NULL);
    }
  }

  if (do_ref_if_object || !ecma_is_value_object (value))
  {
    value = ecma_copy_value (value);
  }

  chunk_p->items[item_index] = value;
  chunk_p->items[item_index + 1] = ecma_make_collection_chunk_value (NULL);
  header_p->item_count++;
} /* ecma_append_to_values_collection */

/**
 * Initialize new collection iterator for the collection
 *
 * @return pointer to the first item
 */
ecma_value_t *
ecma_collection_iterator_init (ecma_collection_header_t *header_p) /**< header of collection */
{
  if (unlikely (!header_p || header_p->item_count == 0))
  {
    return NULL;
  }

  ecma_collection_chunk_t *chunk_p = ECMA_GET_NON_NULL_POINTER (ecma_collection_chunk_t,
                                                                header_p->first_chunk_cp);

  return chunk_p->items;
} /* ecma_collection_iterator_init */

/**
 * Move collection iterator to next element if there is any.
 *
 * @return pointer to the next item
 */
ecma_value_t *
ecma_collection_iterator_next (ecma_value_t *ecma_value_p) /**< current value */
{
  JERRY_ASSERT (ecma_value_p != NULL);

  ecma_value_p++;

  if (unlikely (ecma_is_value_collection_chunk (*ecma_value_p)))
  {
    ecma_value_p = ecma_get_collection_chunk_from_value (*ecma_value_p)->items;

    JERRY_ASSERT (ecma_value_p == NULL || !ecma_is_value_collection_chunk (*ecma_value_p));
    return ecma_value_p;
  }

  return ecma_value_p;
} /* ecma_collection_iterator_next */

/**
 * @}
 * @}
 */
