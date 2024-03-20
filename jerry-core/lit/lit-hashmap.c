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
 *
 *  The lit-hashmap library is based on the public domain software
 *  available from https://github.com/sheredom/hashmap.h
 */

#include <stdlib.h>
#include <string.h>

#include "ecma-globals.h"

#if JERRY_LIT_HASHMAP

#include "ecma-helpers.h"

#include "lit-hashmap-internal.h"

#define HASHMAP_CAST(type, x)     ((type) (x))
#define HASHMAP_PTR_CAST(type, x) ((type) (x))
#define HASHMAP_NULL              0

int
hashmap_create_ex (struct hashmap_create_options_s options, struct hashmap_s *const out_hashmap)
{
  if (2 > options.initial_capacity)
  {
    options.initial_capacity = 2;
  }
  else if (0 != (options.initial_capacity & (options.initial_capacity - 1)))
  {
    options.initial_capacity = 1u << (32 - hashmap_clz (options.initial_capacity));
  }

  out_hashmap->alloc_size =
    (options.initial_capacity + HASHMAP_LINEAR_PROBE_LENGTH) * sizeof (struct hashmap_element_s);
  out_hashmap->data = HASHMAP_CAST (struct hashmap_element_s *, jmem_heap_alloc_block (out_hashmap->alloc_size));
  memset (out_hashmap->data, 0, out_hashmap->alloc_size);

  out_hashmap->log2_capacity = 31 - hashmap_clz (options.initial_capacity);
  out_hashmap->size = 0;
  out_hashmap->initialized = 1;

  return 0;
} /* hashmap_create_ex */

/**
 * hashmap_put
 *
 */
int
hashmap_put (struct hashmap_s *const m, const ecma_string_t *const key)
{
  JERRY_ASSERT (m->initialized == 0 || m->initialized == 1);
  hashmap_uint32_t index;

  if ((HASHMAP_NULL == key))
  {
    return 1;
  }

  /* Find a place to put our value. */
  while (!hashmap_hash_helper (m, key, &index))
  {
    if (hashmap_rehash_helper (m))
    {
      return 1;
    }
  }

  if (0 == m->data[index].data)
    m->size++;

  /* Set the data. */
  m->data[index].data = key;

  return 0;
} /* hashmap_put */

/**
 * hashmap_init
 *
 */
void
hashmap_init (struct hashmap_s *m)
{
  if (m->initialized == 0)
  {
    m->initialized = 1;
    hashmap_create_options_t opts = { .initial_capacity = 32 };
    hashmap_create_ex (opts, m);
  }
  else if (m->initialized != 1)
  {
    JERRY_ASSERT (0);
  }
} /* hashmap_init */

/**
 * hashmap_get
 *
 */
const ecma_string_t *
hashmap_get (const struct hashmap_s *const m, const ecma_string_t *const key)
{
  JERRY_ASSERT (m->initialized == 0 || m->initialized == 1);
  hashmap_uint32_t i, curr;

  if ((HASHMAP_NULL == key))
  {
    JERRY_ASSERT (0);
    return HASHMAP_NULL;
  }

  curr = hashmap_hash_helper_int_helper (m, key);

  /* Linear probing, if necessary */
  for (i = 0; i < HASHMAP_LINEAR_PROBE_LENGTH; i++)
  {
    const hashmap_uint32_t index = curr + i;

    if (m->data[index].data)
    {
      if (ecma_compare_ecma_strings (m->data[index].data, key))
      {
        JERRY_ASSERT (m->initialized == 0 || m->initialized == 1);
        return m->data[index].data;
      }
    }
  }

  JERRY_ASSERT (m->initialized == 0 || m->initialized == 1);
  /* Not found */
  return HASHMAP_NULL;
} /* hashmap_get */

/**
 * hashmap_remove
 *
 */
int
hashmap_remove (struct hashmap_s *const m, const ecma_string_t *const key)
{
  hashmap_uint32_t i, curr;

  if ((HASHMAP_NULL == key))
  {
    return 1;
  }

  curr = hashmap_hash_helper_int_helper (m, key);

  /* Linear probing, if necessary */
  for (i = 0; i < HASHMAP_LINEAR_PROBE_LENGTH; i++)
  {
    const hashmap_uint32_t index = curr + i;

    if (m->data[index].data)
    {
      if (ecma_compare_ecma_strings (m->data[index].data, key))
      {
        /* Blank out the fields including in_use */
        memset (&m->data[index], 0, sizeof (struct hashmap_element_s));

        /* Reduce the size */
        m->size--;

        return 0;
      }
    }
  }

  return 1;
} /* hashmap_remove */

/**
 * hashmap_iterate_pairs
 *
 */
int
hashmap_iterate_pairs (struct hashmap_s *const m,
                       int (*f) (void *const, struct hashmap_element_s *const),
                       void *const context)
{
  hashmap_uint32_t i;
  int r;

  for (i = 0; i < (hashmap_capacity (m) + HASHMAP_LINEAR_PROBE_LENGTH); i++)
  {
    struct hashmap_element_s *p;
    p = &m->data[i];
    if (p->data)
    {
      r = f (context, p);
      switch (r)
      {
        case -1: /* remove item */
          memset (p, 0, sizeof (struct hashmap_element_s));
          m->size--;
          break;
        case 0: /* continue iterating */
          break;
        default: /* early exit */
          return 1;
      }
    }
  }
  return 0;
} /* hashmap_iterate_pairs */

/**
 * hashmap_destroy
 *
 */
void
hashmap_destroy (struct hashmap_s *const m)
{
  if (m->data)
    jmem_heap_free_block (m->data, m->alloc_size);
  memset (m, 0, sizeof (struct hashmap_s));
} /* hashmap_destroy */

/**
 * hashmap_num_entries
 *
 */
HASHMAP_ALWAYS_INLINE hashmap_uint32_t
hashmap_num_entries (const struct hashmap_s *const m)
{
  return m->size;
} /* hashmap_num_entries */

/**
 * hashmap_capacity
 *
 */
HASHMAP_ALWAYS_INLINE hashmap_uint32_t
hashmap_capacity (const struct hashmap_s *const m)
{
  return 1u << m->log2_capacity;
} /* hashmap_capacity */

/**
 * hashmap_hash_helper_int_helper
 *
 */
HASHMAP_ALWAYS_INLINE hashmap_uint32_t
hashmap_hash_helper_int_helper (const struct hashmap_s *const m, const ecma_string_t *const k)
{
  return (k->u.hash * 2654435769u) >> (32u - m->log2_capacity);
} /* hashmap_hash_helper_int_helper */

/**
 * hashmap_hash_helper
 *
 */
HASHMAP_ALWAYS_INLINE int
hashmap_hash_helper (const struct hashmap_s *const m, const ecma_string_t *const key, hashmap_uint32_t *const out_index)
{
  hashmap_uint32_t curr;
  hashmap_uint32_t i;
  hashmap_uint32_t first_free;

  /* If full, return immediately */
  if (hashmap_num_entries (m) == hashmap_capacity (m))
  {
    return 0;
  }

  /* Find the best index */
  curr = hashmap_hash_helper_int_helper (m, key);
  first_free = ~0u;

  for (i = 0; i < HASHMAP_LINEAR_PROBE_LENGTH; i++)
  {
    const hashmap_uint32_t index = curr + i;

    if (!m->data[index].data)
    {
      first_free = (first_free < index) ? first_free : index;
    }
    else if (ecma_compare_ecma_strings (m->data[index].data, key))
    {
      *out_index = index;
      return 1;
    }
  }

  // Couldn't find a free element in the linear probe.
  if (~0u == first_free)
  {
    return 0;
  }

  *out_index = first_free;
  return 1;
} /* hashmap_hash_helper */

/**
 * hashmap_rehash_iterator
 *
 */
int
hashmap_rehash_iterator (void *const new_hash, struct hashmap_element_s *const e)
{
  int temp = hashmap_put (HASHMAP_PTR_CAST (struct hashmap_s *, new_hash), e->data);

  if (0 < temp)
  {
    return 1;
  }

  /* clear old value to avoid stale pointers */
  return -1;
} /* hashmap_rehash_iterator */

/**
 * hashmap_rehash_helper
 *
 * Doubles the size of the hashmap, and rehashes all the elements
 */
HASHMAP_ALWAYS_INLINE int
hashmap_rehash_helper (struct hashmap_s *const m)
{
  struct hashmap_create_options_s options;
  struct hashmap_s new_m;
  int flag;

  memset (&options, 0, sizeof (options));
  options.initial_capacity = hashmap_capacity (m) * 2;

  if (0 == options.initial_capacity)
  {
    return 1;
  }

  flag = hashmap_create_ex (options, &new_m);

  if (0 != flag)
  {
    JERRY_ASSERT (0);
    return flag;
  }

  /* copy the old elements to the new table */
  flag = hashmap_iterate_pairs (m, hashmap_rehash_iterator, HASHMAP_PTR_CAST (void *, &new_m));

  if (0 != flag)
  {
    return flag;
  }

  hashmap_destroy (m);

  /* put new hash into old hash structure by copying */
  memcpy (m, &new_m, sizeof (struct hashmap_s));

  return 0;
} /* hashmap_rehash_helper */

/**
 * hashmap_clz
 *
 */
HASHMAP_ALWAYS_INLINE hashmap_uint32_t
hashmap_clz (const hashmap_uint32_t x)
{
#if defined(_MSC_VER)
  unsigned long result;
  _BitScanReverse (&result, x);
  return 31 - HASHMAP_CAST (hashmap_uint32_t, result);
#else /* _MSC_VER */
  return HASHMAP_CAST (hashmap_uint32_t, __builtin_clz (x));
#endif /* _MSC_VER */
} /* hashmap_clz */

#endif /* JERRY_LIT_HASHMAP */
