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

/*
   The lit-hashmap library is based on the public domain software
   available from https://github.com/sheredom/hashmap.h
*/

#ifndef LIT_HASHMAP_INTERNAL_H
#define LIT_HASHMAP_INTERNAL_H

#include "lit-hashmap.h"

#define HASHMAP_ALWAYS_INLINE       __attribute__ ((always_inline)) inline
#define HASHMAP_LINEAR_PROBE_LENGTH (8)

/**
 * hashmap creation options.
 */

typedef struct hashmap_create_options_s
{
  hashmap_uint32_t initial_capacity; /**< initial hashmap capacity */
} hashmap_create_options_t;

/// @brief Create a hashmap.
/// @param options The options to create the hashmap with.
/// @param out_hashmap The storage for the created hashmap.
/// @return On success 0 is returned.
///
/// The options members work as follows:
/// - initial_capacity The initial capacity of the hashmap.
/// - hasher Which hashing function to use with the hashmap (by default the
//    crc32 with Robert Jenkins' mix is used).
int hashmap_create_ex (struct hashmap_create_options_s options, struct hashmap_s *const out_hashmap);

/// @brief Iterate over all the elements in a hashmap.
/// @param hashmap The hashmap to iterate over.
/// @param iterator The function pointer to call on each element.
/// @param context The context to pass as the first argument to f.
/// @return If the entire hashmap was iterated then 0 is returned.
/// Otherwise if the callback function f returned positive then the positive
/// value is returned.  If the callback function returns -1, the current item
/// is removed and iteration continues.
int hashmap_iterate_pairs (struct hashmap_s *const hashmap,
                           int (*iterator) (void *const, struct hashmap_element_s *const),
                           void *const context);

/// @brief Get the size of the hashmap.
/// @param hashmap The hashmap to get the size of.
/// @return The size of the hashmap.
HASHMAP_ALWAYS_INLINE hashmap_uint32_t hashmap_num_entries (const struct hashmap_s *const hashmap);

/// @brief Get the capacity of the hashmap.
/// @param hashmap The hashmap to get the size of.
/// @return The capacity of the hashmap.
HASHMAP_ALWAYS_INLINE hashmap_uint32_t hashmap_capacity (const struct hashmap_s *const hashmap);

HASHMAP_ALWAYS_INLINE hashmap_uint32_t hashmap_hash_helper_int_helper (const struct hashmap_s *const m,
                                                                       const ecma_string_t *const key);
HASHMAP_ALWAYS_INLINE int hashmap_hash_helper (const struct hashmap_s *const m,
                                               const ecma_string_t *const key,
                                               hashmap_uint32_t *const out_index);
int hashmap_rehash_iterator (void *const new_hash, struct hashmap_element_s *const e);
HASHMAP_ALWAYS_INLINE int hashmap_rehash_helper (struct hashmap_s *const m);
HASHMAP_ALWAYS_INLINE hashmap_uint32_t hashmap_clz (const hashmap_uint32_t x);

#endif // LIT_HASHMAP_INTERNAL_H
