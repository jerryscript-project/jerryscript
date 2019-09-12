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
#include "ecma-gc.h"
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
 * Allocate a collection of ecma values.
 *
 * @return pointer to the collection
 */
ecma_collection_t *
ecma_new_collection (void)
{
  ecma_collection_t *collection_p;
  collection_p = (ecma_collection_t *) jmem_heap_alloc_block (sizeof (ecma_collection_t));

  collection_p->item_count = 0;
  collection_p->capacity = ECMA_COLLECTION_INITIAL_CAPACITY;
  const uint32_t size = ECMA_COLLECTION_ALLOCATED_SIZE (ECMA_COLLECTION_INITIAL_CAPACITY);
  collection_p->buffer_p = (ecma_value_t *) jmem_heap_alloc_block (size);

  return collection_p;
} /* ecma_new_collection */

/**
 * Deallocate a collection of ecma values without freeing it's values
 */
inline void JERRY_ATTR_ALWAYS_INLINE
ecma_collection_destroy (ecma_collection_t *collection_p) /**< value collection */
{
  JERRY_ASSERT (collection_p != NULL);

  jmem_heap_free_block (collection_p->buffer_p, ECMA_COLLECTION_ALLOCATED_SIZE (collection_p->capacity));
  jmem_heap_free_block (collection_p, sizeof (ecma_collection_t));
} /* ecma_collection_destroy */

/**
 * Free the object collection elements and deallocate the collection
 */
void
ecma_collection_free_objects (ecma_collection_t *collection_p) /**< value collection */
{
  JERRY_ASSERT (collection_p != NULL);

  ecma_value_t *buffer_p = collection_p->buffer_p;

  for (uint32_t i = 0; i < collection_p->item_count; i++)
  {
    if (ecma_is_value_object (buffer_p[i]))
    {
      ecma_deref_object (ecma_get_object_from_value (buffer_p[i]));
    }
  }

  ecma_collection_destroy (collection_p);
} /* ecma_collection_free_objects */

/**
 * Free the non-object collection elements and deallocate the collection
 */
void
ecma_collection_free_if_not_object (ecma_collection_t *collection_p) /**< value collection */
{
  JERRY_ASSERT (collection_p != NULL);

  ecma_value_t *buffer_p = collection_p->buffer_p;

  for (uint32_t i = 0; i < collection_p->item_count; i++)
  {
    ecma_free_value_if_not_object (buffer_p[i]);
  }

  ecma_collection_destroy (collection_p);
} /* ecma_collection_free_if_not_object */

/**
 * Free the collection elements and deallocate the collection
 */
void
ecma_collection_free (ecma_collection_t *collection_p) /**< value collection */
{
  JERRY_ASSERT (collection_p != NULL);

  ecma_value_t *buffer_p = collection_p->buffer_p;

  for (uint32_t i = 0; i < collection_p->item_count; i++)
  {
    ecma_free_value (buffer_p[i]);
  }

  ecma_collection_destroy (collection_p);
} /* ecma_collection_free */

/**
 * Append new value to ecma values collection
 *
 * Note: The reference count of the values are not increased
 */
void
ecma_collection_push_back (ecma_collection_t *collection_p, /**< value collection */
                           ecma_value_t value) /**< ecma value to append */
{
  JERRY_ASSERT (collection_p != NULL);

  ecma_value_t *buffer_p = collection_p->buffer_p;

  if (JERRY_LIKELY (collection_p->item_count < collection_p->capacity))
  {
    buffer_p[collection_p->item_count++] = value;
    return;
  }

  const uint32_t new_capacity = collection_p->capacity + ECMA_COLLECTION_GROW_FACTOR;
  const uint32_t old_size = ECMA_COLLECTION_ALLOCATED_SIZE (collection_p->capacity);
  const uint32_t new_size = ECMA_COLLECTION_ALLOCATED_SIZE (new_capacity);

  buffer_p = jmem_heap_realloc_block (buffer_p, old_size, new_size);
  buffer_p[collection_p->item_count++] = value;
  collection_p->capacity = new_capacity;

  collection_p->buffer_p = buffer_p;
} /* ecma_collection_push_back */

/**
 * @}
 * @}
 */
