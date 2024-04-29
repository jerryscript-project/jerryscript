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

typedef struct hashmap_s *hashmap_t;

void hashmap_init (hashmap_t *);
void hashmap_put (hashmap_t hashmap, const ecma_string_t *const key);
const ecma_string_t *hashmap_get (hashmap_t hashmap, const ecma_string_t *const key);
void hashmap_remove (hashmap_t hashmap, const ecma_string_t *const key);
void hashmap_destroy (hashmap_t *hashmap);

#endif // LIT_HASHMAP_H
