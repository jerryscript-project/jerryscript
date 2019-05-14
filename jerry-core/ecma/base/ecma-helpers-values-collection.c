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
JERRY_STATIC_ASSERT (ECMA_TYPE_ERROR == ECMA_TYPE_POINTER,
                     ecma_type_error_must_be_the_same_as_ecma_type_pointer);

/**
 * Allocate a collection of ecma values.
 *
 * @return pointer to the collection's header
 */
ecma_collection_header_t *
ecma_new_values_collection (void)
{
  ecma_collection_header_t *header_p;
  header_p = (ecma_collection_header_t *) jmem_pools_alloc (sizeof (ecma_collection_header_t));

  header_p->item_count = 0;
  header_p->first_chunk_cp = ECMA_NULL_POINTER;
  header_p->last_chunk_cp = ECMA_NULL_POINTER;

  return header_p;
} /* ecma_new_values_collection */

/**
 * Free the collection of ecma values.
 */
void
ecma_free_values_collection (ecma_collection_header_t *header_p, /**< collection's header */
                             uint32_t flags) /**< combination of ecma_collection_flag_t flags */
{
  ecma_collection_chunk_t *chunk_p = ECMA_GET_POINTER (ecma_collection_chunk_t,
                                                       header_p->first_chunk_cp);

  jmem_pools_free (header_p, sizeof (ecma_collection_header_t));

  if (chunk_p == NULL)
  {
    return;
  }

  do
  {
    ecma_value_t *item_p = chunk_p->items;

    JERRY_ASSERT (!ecma_is_value_pointer (*item_p));

    do
    {
      if (!(flags & ECMA_COLLECTION_NO_COPY)
          && (!ecma_is_value_object (*item_p)
              || !(flags & ECMA_COLLECTION_NO_REF_OBJECTS)))
      {
        ecma_free_value (*item_p);
      }

      item_p++;
    }
    while (!ecma_is_value_pointer (*item_p));

    ecma_collection_chunk_t *next_chunk_p = (ecma_collection_chunk_t *) ecma_get_pointer_from_value (*item_p);

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
                                  uint32_t flags) /**< combination of ecma_collection_flag_t flags */
{
  ecma_length_t item_index;
  ecma_collection_chunk_t *chunk_p;

  if (JERRY_UNLIKELY (header_p->item_count == 0))
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

    if (JERRY_UNLIKELY (item_index == 0))
    {
      JERRY_ASSERT (ecma_is_value_pointer (chunk_p->items[ECMA_COLLECTION_CHUNK_ITEMS])
                    && ecma_get_pointer_from_value (chunk_p->items[ECMA_COLLECTION_CHUNK_ITEMS]) == NULL);

      ecma_collection_chunk_t *next_chunk_p;
      next_chunk_p = (ecma_collection_chunk_t *) jmem_heap_alloc_block (sizeof (ecma_collection_chunk_t));

      chunk_p->items[ECMA_COLLECTION_CHUNK_ITEMS] = ecma_make_pointer_value ((void *) next_chunk_p);
      ECMA_SET_POINTER (header_p->last_chunk_cp, next_chunk_p);

      chunk_p = next_chunk_p;
    }
    else
    {
      JERRY_ASSERT (ecma_is_value_pointer (chunk_p->items[item_index])
                    && ecma_get_pointer_from_value (chunk_p->items[item_index]) == NULL);
    }
  }

  if (!(flags & ECMA_COLLECTION_NO_COPY)
      && (!ecma_is_value_object (value)
          || !(flags & ECMA_COLLECTION_NO_REF_OBJECTS)))
  {
    value = ecma_copy_value (value);
  }

  chunk_p->items[item_index] = value;
  chunk_p->items[item_index + 1] = ecma_make_pointer_value (NULL);
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
  if (JERRY_UNLIKELY (!header_p || header_p->item_count == 0))
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

  if (JERRY_UNLIKELY (ecma_is_value_pointer (*ecma_value_p)))
  {
    ecma_value_p = ((ecma_collection_chunk_t *) ecma_get_pointer_from_value (*ecma_value_p))->items;

    JERRY_ASSERT (ecma_value_p == NULL || !ecma_is_value_pointer (*ecma_value_p));
    return ecma_value_p;
  }

  return ecma_value_p;
} /* ecma_collection_iterator_next */

/**
 * @}
 * @}
 */
