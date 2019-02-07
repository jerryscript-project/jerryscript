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

#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lcache.h"
#include "jcontext.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmalcache Property lookup cache
 * @{
 */

#ifndef CONFIG_ECMA_LCACHE_DISABLE

/**
 * Mask for hash bits
 */
#define ECMA_LCACHE_HASH_MASK (ECMA_LCACHE_HASH_ROWS_COUNT - 1)

/**
 * Invalidate specified LCache entry
 */
static inline void JERRY_ATTR_ALWAYS_INLINE
ecma_lcache_invalidate_entry (ecma_lcache_hash_entry_t *entry_p) /**< entry to invalidate */
{
  JERRY_ASSERT (entry_p != NULL);
  JERRY_ASSERT (entry_p->object_cp != ECMA_NULL_POINTER);
  JERRY_ASSERT (entry_p->prop_p != NULL);

  entry_p->object_cp = ECMA_NULL_POINTER;
  ecma_set_property_lcached (entry_p->prop_p, false);
} /* ecma_lcache_invalidate_entry */

/**
 * Compute the row index of object / property name pair
 *
 * @return row index
 */
static inline size_t JERRY_ATTR_ALWAYS_INLINE
ecma_lcache_row_index (jmem_cpointer_t object_cp, /**< compressed pointer to object */
                       lit_string_hash_t name_hash) /**< property name hash */
{
  /* Randomize the hash of the property name with the object pointer using a xor operation,
   * so properties of different objects with the same name can be cached effectively. */
  return (size_t) ((name_hash ^ object_cp) & ECMA_LCACHE_HASH_MASK);
} /* ecma_lcache_row_index */

/**
 * Insert an entry into LCache
 */
void
ecma_lcache_insert (ecma_object_t *object_p, /**< object */
                    jmem_cpointer_t name_cp, /**< property name */
                    ecma_property_t *prop_p) /**< property */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (prop_p != NULL && !ecma_is_property_lcached (prop_p));
  JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (*prop_p) == ECMA_PROPERTY_TYPE_NAMEDDATA
                || ECMA_PROPERTY_GET_TYPE (*prop_p) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR
                || ECMA_PROPERTY_GET_TYPE (*prop_p) == ECMA_PROPERTY_TYPE_INTERNAL);

  jmem_cpointer_t object_cp;

  ECMA_SET_NON_NULL_POINTER (object_cp, object_p);

  lit_string_hash_t name_hash = ecma_string_get_property_name_hash (*prop_p, name_cp);
  size_t row_index = ecma_lcache_row_index (object_cp, name_hash);
  ecma_lcache_hash_entry_t *entries_p = JERRY_CONTEXT (lcache) [row_index];

  uint32_t entry_index;
  for (entry_index = 0; entry_index < ECMA_LCACHE_HASH_ROW_LENGTH; entry_index++)
  {
    if (entries_p[entry_index].object_cp == ECMA_NULL_POINTER)
    {
      break;
    }
  }

  if (entry_index == ECMA_LCACHE_HASH_ROW_LENGTH)
  {
    /* Invalidate the last entry. */
    ecma_lcache_invalidate_entry (entries_p + ECMA_LCACHE_HASH_ROW_LENGTH - 1);

    /* Shift other entries towards the end. */
    for (uint32_t i = ECMA_LCACHE_HASH_ROW_LENGTH - 1; i > 0; i--)
    {
      entries_p[i] = entries_p[i - 1];
    }

    entry_index = 0;
  }

  ecma_lcache_hash_entry_t *entry_p = entries_p + entry_index;
  ECMA_SET_NON_NULL_POINTER (entry_p->object_cp, object_p);
  entry_p->prop_name_cp = name_cp;
  entry_p->prop_p = prop_p;

  ecma_set_property_lcached (entry_p->prop_p, true);
} /* ecma_lcache_insert */

/**
 * Lookup property in the LCache
 *
 * @return a pointer to an ecma_property_t if the lookup is successful
 *         NULL otherwise
 */
inline ecma_property_t * JERRY_ATTR_ALWAYS_INLINE
ecma_lcache_lookup (ecma_object_t *object_p, /**< object */
                    const ecma_string_t *prop_name_p) /**< property's name */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (prop_name_p != NULL);

  jmem_cpointer_t object_cp;
  ECMA_SET_NON_NULL_POINTER (object_cp, object_p);

  ecma_property_t prop_name_type;
  jmem_cpointer_t prop_name_cp;
  lit_string_hash_t name_hash;

  if (ECMA_IS_DIRECT_STRING (prop_name_p))
  {
    prop_name_type = (ecma_property_t) ECMA_GET_DIRECT_STRING_TYPE (prop_name_p);

    uintptr_t value = ECMA_GET_DIRECT_STRING_VALUE (prop_name_p);
    prop_name_cp = (jmem_cpointer_t) value;
    name_hash = (lit_string_hash_t) value;

    if (prop_name_type == ECMA_DIRECT_STRING_MAGIC_EX)
    {
      name_hash = (lit_string_hash_t) (name_hash + LIT_MAGIC_STRING__COUNT);
    }
  }
  else
  {
    prop_name_type = ECMA_DIRECT_STRING_PTR;

    ECMA_SET_NON_NULL_POINTER (prop_name_cp, prop_name_p);
    name_hash = prop_name_p->hash;
  }

  size_t row_index = ecma_lcache_row_index (object_cp, name_hash);

  ecma_lcache_hash_entry_t *entry_p = JERRY_CONTEXT (lcache) [row_index];
  ecma_lcache_hash_entry_t *entry_end_p = entry_p + ECMA_LCACHE_HASH_ROW_LENGTH;

  while (entry_p < entry_end_p)
  {
    if (entry_p->object_cp == object_cp
        && entry_p->prop_name_cp == prop_name_cp)
    {
      ecma_property_t *prop_p = entry_p->prop_p;

      JERRY_ASSERT (prop_p != NULL && ecma_is_property_lcached (prop_p));

      if (ECMA_PROPERTY_GET_NAME_TYPE (*prop_p) == prop_name_type)
      {
        return prop_p;
      }
    }
    else
    {
      /* They can be equal, but generic string comparison is too costly. */
    }

    entry_p++;
  }

  return NULL;
} /* ecma_lcache_lookup */

/**
 * Invalidate LCache entries associated with given object and property name / property
 */
void
ecma_lcache_invalidate (ecma_object_t *object_p, /**< object */
                        jmem_cpointer_t name_cp, /**< property name */
                        ecma_property_t *prop_p) /**< property */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (prop_p != NULL && ecma_is_property_lcached (prop_p));
  JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (*prop_p) == ECMA_PROPERTY_TYPE_NAMEDDATA
                || ECMA_PROPERTY_GET_TYPE (*prop_p) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR
                || ECMA_PROPERTY_GET_TYPE (*prop_p) == ECMA_PROPERTY_TYPE_INTERNAL);

  jmem_cpointer_t object_cp;
  ECMA_SET_NON_NULL_POINTER (object_cp, object_p);

  lit_string_hash_t name_hash = ecma_string_get_property_name_hash (*prop_p, name_cp);
  size_t row_index = ecma_lcache_row_index (object_cp, name_hash);
  ecma_lcache_hash_entry_t *entry_p = JERRY_CONTEXT (lcache) [row_index];

  while (true)
  {
    /* The property must be present. */
    JERRY_ASSERT (entry_p - JERRY_CONTEXT (lcache) [row_index] < ECMA_LCACHE_HASH_ROW_LENGTH);

    if (entry_p->object_cp != ECMA_NULL_POINTER && entry_p->prop_p == prop_p)
    {
      JERRY_ASSERT (entry_p->object_cp == object_cp);

      ecma_lcache_invalidate_entry (entry_p);
      return;
    }
    entry_p++;
  }
} /* ecma_lcache_invalidate */

#endif /* !CONFIG_ECMA_LCACHE_DISABLE */

/**
 * @}
 * @}
 */
