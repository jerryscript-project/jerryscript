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

#ifndef LIT_HASHMAP_H
#define LIT_HASHMAP_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ecma-globals.h"
typedef uint8_t hashmap_uint8_t;
typedef uint32_t hashmap_uint32_t;
typedef uint64_t hashmap_uint64_t;

/**
 * hashmap element
 */

typedef struct hashmap_element_s
{
  const ecma_string_t *data; /**< point to a literal */
} hashmap_element_t;

/**
 * hashmap structure
 */

typedef struct hashmap_s
{
  hashmap_uint32_t log2_capacity; /**< hashmap capacity */
  hashmap_uint32_t size; /**< hashmap size*/
  struct hashmap_element_s *data; /**< element array */
  size_t alloc_size; /**< allocated size */
  uint8_t initialized; /**< 0 if not initialized */
} hashmap_t;

/// @brief Initialize the hashmap.
/// @param hashmap The hashmap to initialize.
void hashmap_init (struct hashmap_s *m);

/// @brief Put an element into the hashmap.
/// @param hashmap The hashmap to insert into.
/// @param key The string key to use.
/// @param len The length of the string key.
/// @param value The value to insert.
/// @return On success 0 is returned.
///
/// The key string slice is not copied when creating the hashmap entry, and thus
/// must remain a valid pointer until the hashmap entry is removed or the
/// hashmap is destroyed.
int hashmap_put (struct hashmap_s *const hashmap, const ecma_string_t *const key);

/// @brief Get an element from the hashmap.
/// @param hashmap The hashmap to get from.
/// @param key The string key to use.
/// @param len The length of the string key.
/// @return The previously set element, or NULL if none exists.
const ecma_string_t *hashmap_get (const struct hashmap_s *const hashmap, const ecma_string_t *const key);

/// @brief Remove an element from the hashmap.
/// @param hashmap The hashmap to remove from.
/// @param key The string key to use.
/// @param len The length of the string key.
/// @return On success 0 is returned.
int hashmap_remove (struct hashmap_s *const hashmap, const ecma_string_t *const key);

/// @brief Destroy the hashmap.
/// @param hashmap The hashmap to destroy.
void hashmap_destroy (struct hashmap_s *const hashmap);

#endif // LIT_HASHMAP_H
