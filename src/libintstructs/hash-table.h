/* Copyright 2014 Samsung Electronics Co., Ltd.
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

/**
  This file contains macros to create and manipulate hash-tables.
  In order to define a hash use HASH_TABLE or STATIC_HASH_TABLE macro.
  DO NOT FORGET to define KEY_TYPE##_hash and KEY_TYPE##_equal functions, they will be used
  for internal purposes.
  Before using the hash initialize it by calling HASH_INIT macro.
  To insert a key-value pair, use HASH_INSERT macro.
  To lookup a value by the key, use HASH_LOOKUP macro.
  After using the hash, delete it by calling HASH_FREE macro.
*/
#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "linked-list.h"
#include "mem-heap.h"

typedef void* hash_table;
#define null_hash NULL

hash_table hash_table_init (uint8_t, uint8_t, uint16_t, uint16_t (*hash) (void *), mem_heap_alloc_term_t);
void hash_table_free (hash_table);
void hash_table_insert (hash_table, void *, void *);
void *hash_table_lookup (hash_table, void *);

#endif /* HASH_TABLE_H */
