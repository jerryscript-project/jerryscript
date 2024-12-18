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

#include "lit-hashmap.h"

#include <stdlib.h>
#include <string.h>

#include "ecma-globals.h"

#if JERRY_LIT_HASHMAP

#include "lit-hashmap-impl.h"
#include "lit-hashmap.h"

struct hashmap_s
{
  hashmap_impl map;
};

void
hashmap_init (hashmap_t *map)
{
  struct hashmap_s *hashmap = jmem_heap_alloc_block (sizeof (struct hashmap_s));
  hashmap_impl_init (8, &(hashmap->map));
  (*map) = hashmap;
} /* hashmap_init */

void
hashmap_put (hashmap_t hashmap, const ecma_string_t *const literal)
{
  hashmap_impl_insert (&hashmap->map, literal);
} /* hashmap_put */

const ecma_string_t *
hashmap_get (hashmap_t hashmap, const ecma_string_t *const literal)
{
  hashmap_impl_iter it = hashmap_impl_find (&hashmap->map, literal->u.hash);

  for (const ecma_string_t *p = hashmap_impl_iter_get (&it); p != NULL; p = hashmap_impl_iter_next (&it))
  {
    if (ecma_compare_ecma_strings (p, literal))
    {
      return p;
    }
  }
  return NULL;
} /* hashmap_get */

void
hashmap_destroy (hashmap_t *hashmap)
{
  if (*hashmap)
  {
    hashmap_impl_destroy (&((*hashmap)->map));
    jmem_heap_free_block (&((*hashmap)->map), sizeof (struct hashmap_s));
    *hashmap = (hashmap_t) 0;
  }
} /* hashmap_destroy */

#endif /* JERRY_LIT_HASHMAP */
