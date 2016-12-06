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

#ifndef ECMA_PROPERTY_HASHMAP_H
#define ECMA_PROPERTY_HASHMAP_H

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmapropertyhashmap Property hashmap
 * @{
 */

/**
 * Recommended minimum number of items in a property cache.
 */
#define ECMA_PROPERTY_HASMAP_MINIMUM_SIZE 32

/**
 * Property hash.
 */
typedef struct
{
  ecma_property_header_t header; /**< header of the property */
  uint32_t max_property_count; /**< maximum property count (power of 2) */
  uint32_t null_count; /**< number of NULLs in the map */

  /*
   * The hash is followed by max_property_count ecma_cpointer_t
   * compressed pointers and (max_property_count + 7) / 8 bytes
   * which stores a flag for each compressed pointer.
   *
   * If the compressed pointer is equal to ECMA_NULL_POINTER
   *   - flag is cleared if the entry is NULL
   *   - flag is set if the entry is deleted
   *
   * If the compressed pointer is not equal to ECMA_NULL_POINTER
   *   - flag is cleared if the first entry of a property pair is referenced
   *   - flag is set if the second entry of a property pair is referenced
   */
} ecma_property_hashmap_t;

extern void ecma_property_hashmap_create (ecma_object_t *);
extern void ecma_property_hashmap_free (ecma_object_t *);
extern void ecma_property_hashmap_insert (ecma_object_t *, ecma_string_t *, ecma_property_pair_t *, int);
extern void ecma_property_hashmap_delete (ecma_object_t *, jmem_cpointer_t, ecma_property_t *);

#ifndef CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE
extern ecma_property_t *ecma_property_hashmap_find (ecma_property_hashmap_t *, ecma_string_t *, jmem_cpointer_t *);
#endif /* !CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE */

/**
 * @}
 * @}
 */

#endif /* !ECMA_PROPERTY_HASHMAP_H */
