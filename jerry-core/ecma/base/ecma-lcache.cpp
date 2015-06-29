/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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
#include "jrt-libc-includes.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmalcache Property lookup cache
 * @{
 */

#ifndef CONFIG_ECMA_LCACHE_DISABLE
/**
 * Entry of LCache hash table
 */
typedef struct
{
  /** Compressed pointer to object (ECMA_NULL_POINTER marks record empty) */
  mem_cpointer_t object_cp;

  /** Compressed pointer to property's name */
  mem_cpointer_t prop_name_cp;

  /** Compressed pointer to a property of the object */
  mem_cpointer_t prop_cp;

  /** Padding structure to 8 bytes size */
  uint16_t padding;
} ecma_lcache_hash_entry_t;

JERRY_STATIC_ASSERT (sizeof (ecma_lcache_hash_entry_t) == sizeof (uint64_t));

/**
 * LCache hash value length, in bits
 */
#define ECMA_LCACHE_HASH_BITS (sizeof (lit_string_hash_t) * JERRY_BITSINBYTE)

/**
 * Number of rows in LCache's hash table
 */
#define ECMA_LCACHE_HASH_ROWS_COUNT (1ull << ECMA_LCACHE_HASH_BITS)

/**
 * Number of entries in a row of LCache's hash table
 */
#define ECMA_LCACHE_HASH_ROW_LENGTH (2)

/**
 * LCache's hash table
 */
static ecma_lcache_hash_entry_t ecma_lcache_hash_table[ ECMA_LCACHE_HASH_ROWS_COUNT ][ ECMA_LCACHE_HASH_ROW_LENGTH ];
#endif /* !CONFIG_ECMA_LCACHE_DISABLE */

/**
 * Initialize LCache
 */
void
ecma_lcache_init (void)
{
#ifndef CONFIG_ECMA_LCACHE_DISABLE
  memset (ecma_lcache_hash_table, 0, sizeof (ecma_lcache_hash_table));
#endif /* !CONFIG_ECMA_LCACHE_DISABLE */
} /* ecma_lcache_init */

#ifndef CONFIG_ECMA_LCACHE_DISABLE
/**
 * Invalidate specified LCache entry
 */
static void
ecma_lcache_invalidate_entry (ecma_lcache_hash_entry_t *entry_p) /**< entry to invalidate */
{
  JERRY_ASSERT (entry_p != NULL);
  JERRY_ASSERT (entry_p->object_cp != ECMA_NULL_POINTER);

  ecma_deref_object (ECMA_GET_NON_NULL_POINTER (ecma_object_t,
                                                entry_p->object_cp));

  entry_p->object_cp = ECMA_NULL_POINTER;
  ecma_deref_ecma_string (ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                     entry_p->prop_name_cp));

  if (entry_p->prop_cp != ECMA_NULL_POINTER)
  {
    ecma_set_property_lcached (ECMA_GET_NON_NULL_POINTER (ecma_property_t,
                                                          entry_p->prop_cp),
                               false);
  }
} /* ecma_lcache_invalidate_entry */
#endif /* !CONFIG_ECMA_LCACHE_DISABLE */

/**
 * Invalidate all entries in LCache
 */
void
ecma_lcache_invalidate_all (void)
{
#ifndef CONFIG_ECMA_LCACHE_DISABLE
  for (uint32_t row_index = 0; row_index < ECMA_LCACHE_HASH_ROWS_COUNT; row_index++)
  {
    for (uint32_t entry_index = 0; entry_index < ECMA_LCACHE_HASH_ROW_LENGTH; entry_index++)
    {
      if (ecma_lcache_hash_table[ row_index ][ entry_index ].object_cp != ECMA_NULL_POINTER)
      {
        ecma_lcache_invalidate_entry (&ecma_lcache_hash_table[ row_index ][ entry_index ]);
      }
    }
  }
#endif /* !CONFIG_ECMA_LCACHE_DISABLE */
} /* ecma_lcache_invalidate_all */

#ifndef CONFIG_ECMA_LCACHE_DISABLE
/**
 * Invalidate entries of LCache's row that correspond to given (object, property) pair
 */
static void
ecma_lcache_invalidate_row_for_object_property_pair (uint32_t row_index, /**< index of the row */
                                                     unsigned int object_cp, /**< compressed pointer
                                                                              *   to an object */
                                                     unsigned property_cp) /**< compressed pointer
                                                                            *   to the object's
                                                                            *   property */
{
  for (uint32_t entry_index = 0; entry_index < ECMA_LCACHE_HASH_ROW_LENGTH; entry_index++)
  {
    if (ecma_lcache_hash_table[ row_index ][ entry_index ].object_cp == object_cp
        && ecma_lcache_hash_table[ row_index ][ entry_index ].prop_cp == property_cp)
    {
      ecma_lcache_invalidate_entry (&ecma_lcache_hash_table[ row_index ][ entry_index ]);
    }
  }
} /* ecma_lcache_invalidate_row_for_object_property_pair */
#endif /* !CONFIG_ECMA_LCACHE_DISABLE */

/**
 * Insert an entry into LCache
 */
void
ecma_lcache_insert (ecma_object_t *object_p, /**< object */
                    ecma_string_t *prop_name_p, /**< property's name */
                    ecma_property_t *prop_p) /**< pointer to associated property or NULL
                                              *   (NULL indicates that the object doesn't have property
                                              *   with the name specified) */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (prop_name_p != NULL);

#ifndef CONFIG_ECMA_LCACHE_DISABLE
  prop_name_p = ecma_copy_or_ref_ecma_string (prop_name_p);

  lit_string_hash_t hash_key = ecma_string_hash (prop_name_p);

  if (prop_p != NULL)
  {
    if (unlikely (ecma_is_property_lcached (prop_p)))
    {
      mem_cpointer_t prop_cp;
      ECMA_SET_NON_NULL_POINTER (prop_cp, prop_p);

      int32_t entry_index;
      for (entry_index = 0; entry_index < ECMA_LCACHE_HASH_ROW_LENGTH; entry_index++)
      {
        if (ecma_lcache_hash_table[hash_key][entry_index].object_cp != ECMA_NULL_POINTER
            && ecma_lcache_hash_table[hash_key][entry_index].prop_cp == prop_cp)
        {
#ifndef JERRY_NDEBUG
          ecma_object_t* obj_in_entry_p;
          obj_in_entry_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t,
                                                      ecma_lcache_hash_table[hash_key][entry_index].object_cp);
          JERRY_ASSERT (obj_in_entry_p == object_p);
#endif /* !JERRY_NDEBUG */
          break;
        }
      }

      JERRY_ASSERT (entry_index != ECMA_LCACHE_HASH_ROW_LENGTH);
      ecma_lcache_invalidate_entry (&ecma_lcache_hash_table[hash_key][entry_index]);
    }

    JERRY_ASSERT (!ecma_is_property_lcached (prop_p));
    ecma_set_property_lcached (prop_p, true);
  }

  int32_t entry_index;
  for (entry_index = 0; entry_index < ECMA_LCACHE_HASH_ROW_LENGTH; entry_index++)
  {
    if (ecma_lcache_hash_table[hash_key][entry_index].object_cp == ECMA_NULL_POINTER)
    {
      break;
    }
  }

  if (entry_index == ECMA_LCACHE_HASH_ROW_LENGTH)
  {
    /* No empty entry was found, invalidating the whole row */
    for (uint32_t i = 0; i < ECMA_LCACHE_HASH_ROW_LENGTH; i++)
    {
      ecma_lcache_invalidate_entry (&ecma_lcache_hash_table[hash_key][i]);
    }

    entry_index = 0;
  }

  ecma_ref_object (object_p);
  ECMA_SET_NON_NULL_POINTER (ecma_lcache_hash_table[ hash_key ][ entry_index ].object_cp, object_p);
  ECMA_SET_NON_NULL_POINTER (ecma_lcache_hash_table[ hash_key ][ entry_index ].prop_name_cp, prop_name_p);
  ECMA_SET_POINTER (ecma_lcache_hash_table[ hash_key ][ entry_index ].prop_cp, prop_p);
#else /* CONFIG_ECMA_LCACHE_DISABLE */
  (void) prop_p;
#endif /* CONFIG_ECMA_LCACHE_DISABLE */
} /* ecma_lcache_insert */

/**
 * Lookup property in the LCache
 *
 * @return true - if (object, property name) pair is registered in LCache,
 *         false - probably, not registered.
 */
bool __attr_always_inline___
ecma_lcache_lookup (ecma_object_t *object_p, /**< object */
                    const ecma_string_t *prop_name_p, /**< property's name */
                    ecma_property_t **prop_p_p) /**< out: if return value is true,
                                                 *         then here will be pointer to property,
                                                 *         if the object contains property with specified name,
                                                 *         or, otherwise - NULL;
                                                 *        if return value is false,
                                                 *         then the output parameter is not set */
{
#ifndef CONFIG_ECMA_LCACHE_DISABLE
  lit_string_hash_t hash_key = ecma_string_hash (prop_name_p);

  unsigned int object_cp;
  ECMA_SET_NON_NULL_POINTER (object_cp, object_p);

  for (uint32_t i = 0; i < ECMA_LCACHE_HASH_ROW_LENGTH; i++)
  {
    if (ecma_lcache_hash_table[hash_key][i].object_cp == object_cp)
    {
      ecma_string_t *entry_prop_name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                                    ecma_lcache_hash_table[hash_key][i].prop_name_cp);

      if (ecma_compare_ecma_strings_equal_hashes (prop_name_p, entry_prop_name_p))
      {
        ecma_property_t *prop_p = ECMA_GET_POINTER (ecma_property_t, ecma_lcache_hash_table[hash_key][i].prop_cp);
        JERRY_ASSERT (prop_p == NULL || ecma_is_property_lcached (prop_p));

        *prop_p_p = prop_p;

        return true;
      }
      else
      {
        /* may be equal but it is long to compare it here */
      }
    }
  }
#else /* CONFIG_ECMA_LCACHE_DISABLE */
  (void) object_p;
  (void) prop_name_p;
  (void) prop_p_p;
#endif /* CONFIG_ECMA_LCACHE_DISABLE */

  return false;
} /* ecma_lcache_lookup */

/**
 * Invalidate LCache entries associated with given object and property name / property
 *
 * Note:
 *      Either property name argument or property argument should be NULL,
 *      and another should be non-NULL.
 *      In case property name argument is NULL, property's name is taken
 *      from property's description.
 */
void
ecma_lcache_invalidate (ecma_object_t *object_p, /**< object */
                        ecma_string_t *prop_name_arg_p, /**< property's name (See also: Note) */
                        ecma_property_t *prop_p) /**< property (See also: Note) */
{
  JERRY_ASSERT (object_p != NULL);
  JERRY_ASSERT (prop_p != NULL || prop_name_arg_p != NULL);

#ifndef CONFIG_ECMA_LCACHE_DISABLE
  ecma_string_t *prop_name_p = NULL;

  if (prop_p != NULL)
  {
    JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDDATA
                  || prop_p->type == ECMA_PROPERTY_NAMEDACCESSOR);

    bool is_cached = ecma_is_property_lcached (prop_p);

    if (!is_cached)
    {
      return;
    }

    ecma_set_property_lcached (prop_p, false);

    if (prop_p->type == ECMA_PROPERTY_NAMEDDATA)
    {
      prop_name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                               prop_p->u.named_data_property.name_p);
    }
    else
    {
      prop_name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                               prop_p->u.named_accessor_property.name_p);
    }
  }
  else
  {
    prop_name_p = prop_name_arg_p;
  }

  unsigned int object_cp, prop_cp;
  ECMA_SET_NON_NULL_POINTER (object_cp, object_p);
  ECMA_SET_POINTER (prop_cp, prop_p);

  lit_string_hash_t hash_key = ecma_string_hash (prop_name_p);

  /* Property's name has was computed.
   * Given (object, property name) pair should be in the row corresponding to computed hash.
   */
  ecma_lcache_invalidate_row_for_object_property_pair (hash_key, object_cp, prop_cp);
#endif /* !CONFIG_ECMA_LCACHE_DISABLE */
} /* ecma_lcache_invalidate */

/**
 * @}
 * @}
 */
