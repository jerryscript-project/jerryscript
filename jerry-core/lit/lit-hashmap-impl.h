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
#include "ecma-helpers.h"

/* ------------------------------------------------------------------------------------------------ */
// Configuration

#define HASHMAP_LINEAR_PROBE_LENGTH ((size_t) 8)

/* ------------------------------------------------------------------------------------------------ */

#define HASHMAP_CAST(type, x)     ((type) (x))
#define HASHMAP_PTR_CAST(type, x) ((type) (x))
#define HASHMAP_NULL              0

#define HASHMAP_IMPL_INLINE __attribute__ ((always_inline)) inline

/* ------------------------------------------------------------------------------------------------ */

/**
 * HashMap object, internal representation
 */
typedef struct
{
  size_t log2_capacity; /**< hashmap   capacity */
  uint32_t used_entries; /**< hashmap size*/
  size_t array_size; /**< allocated size */
  const ecma_string_t **literals; /**< element array */
} hashmap_impl;

/**
 * HashMap iterator object, internal representation
 */
typedef struct
{
  hashmap_impl *hmap; /**< pointer to the hashmap */
  uint32_t index; /**< iterator start index */
  uint32_t slot; /**< iterator slot, from 0 to HASHMAP_LINEAR_PROBE_LENGTH */
  lit_string_hash_t key; /**< key beeing looked for */
} hashmap_impl_iter;

/* ------------------------------------------------------------------------------------------------ */

void hashmap_impl_rehash (hashmap_impl *const hashmap);

/* ------------------------------------------------------------------------------------------------ */

HASHMAP_IMPL_INLINE size_t
hashmap_impl_log2 (size_t v)
{
  int n = 31 - __builtin_clz ((uint32_t) v);
  return (size_t) n;
} /* hashmap_round_log2 */

HASHMAP_IMPL_INLINE uint32_t
hashmap_impl_lithash_to_index (const hashmap_impl *const m, lit_string_hash_t hash)
{
  uint32_t v = (hash * 2654435769u) >> (32u - m->log2_capacity);
  return v;
} /* hashmap_hash_to_index */

/* ------------------------------------------------------------------------------------------------ */

HASHMAP_IMPL_INLINE void
hashmap_impl_init (size_t initial_capacity, hashmap_impl *hashmap)
{
  initial_capacity = ((size_t) 1) << hashmap_impl_log2 (initial_capacity);
  size_t array_size = (initial_capacity + HASHMAP_LINEAR_PROBE_LENGTH);

  hashmap->literals = jmem_heap_alloc_block (array_size * sizeof (const ecma_string_t *));
  memset (hashmap->literals, 0, array_size * sizeof (const ecma_string_t *));
  hashmap->log2_capacity = hashmap_impl_log2 (initial_capacity);
  hashmap->used_entries = 0;
} /* hashmap_new */

/* ------------------------------------------------------------------------------------------------ */

HASHMAP_IMPL_INLINE int
hashmap_impl_find_empty_slot (const hashmap_impl *const hashmap, const lit_string_hash_t key, uint32_t *const out_index)
{
  uint32_t start_index = hashmap_impl_lithash_to_index (hashmap, key);

  for (uint32_t offset = 0; offset < HASHMAP_LINEAR_PROBE_LENGTH; offset++)
  {
    const uint32_t index = start_index + offset;

    if (hashmap->literals[index] == NULL)
    {
      *out_index = index;
      return 1;
    }
  }

  return 0;
} /* hashmap_impl_find_empty_slot */

/* ------------------------------------------------------------------------------------------------ */

HASHMAP_IMPL_INLINE void
hashmap_impl_insert (hashmap_impl *hashmap, const ecma_string_t *literal)
{
  /* Find a place to put our value. */
  uint32_t index;
  uint32_t lithash = literal->u.hash;
  while (!hashmap_impl_find_empty_slot (hashmap, lithash, &index))
  {
    hashmap_impl_rehash (hashmap);
  }

  hashmap->literals[index] = literal;
  hashmap->used_entries++;
} /* hashmap_impl_insert */

/* ------------------------------------------------------------------------------------------------ */

HASHMAP_IMPL_INLINE hashmap_impl_iter
hashmap_impl_find (hashmap_impl *hashmap, lit_string_hash_t key)
{
  uint32_t start_index = hashmap_impl_lithash_to_index (hashmap, key);
  return (hashmap_impl_iter){
    .hmap = hashmap,
    .index = start_index,
    .slot = 0,
    .key = key,
  };
} /* hashmap_impl_find */

HASHMAP_IMPL_INLINE const ecma_string_t *
hashmap_impl_iter_get (hashmap_impl_iter *iter)
{
  const ecma_string_t **literals = iter->hmap->literals;
  while (iter->slot < HASHMAP_LINEAR_PROBE_LENGTH)
  {
    uint32_t index = iter->index + iter->slot;
    const ecma_string_t *literal = literals[index];
    if (literal && literal->u.hash == iter->key)
    {
      return literals[index];
    }

    iter->slot++;
  }
  return NULL;
} /* hashmap_impl_iter_get */

HASHMAP_IMPL_INLINE const ecma_string_t *
hashmap_impl_iter_next (hashmap_impl_iter *iter)
{
  iter->slot++;
  return hashmap_impl_iter_get (iter);
} /* hashmap_impl_iter_next */

HASHMAP_IMPL_INLINE void
hashmap_impl_destroy (hashmap_impl *hashmap)
{
  size_t l = ((size_t) (1 << hashmap->log2_capacity)) + HASHMAP_LINEAR_PROBE_LENGTH;
  jmem_heap_free_block (hashmap->literals, l * sizeof (const ecma_string_t *));
} /* hashmap_impl_destroy */

/* ------------------------------------------------------------------------------------------------ */

void
hashmap_impl_rehash (hashmap_impl *const hashmap)
{
  size_t clen = ((size_t) (1 << hashmap->log2_capacity)) + HASHMAP_LINEAR_PROBE_LENGTH;

  hashmap_impl new_hashmap;
  hashmap_impl_init (clen * 2, &new_hashmap);

  size_t l = clen + HASHMAP_LINEAR_PROBE_LENGTH;
  for (size_t i = 0; i < l; i++)
  {
    const ecma_string_t *p = hashmap->literals[i];
    if (!p)
      continue;
    hashmap_impl_insert (&new_hashmap, p);
  }

  jmem_heap_free_block (hashmap->literals, l * sizeof (const ecma_string_t *));

  memcpy (hashmap, &new_hashmap, sizeof (hashmap_impl));
} /* hashmap_impl_rehash */
