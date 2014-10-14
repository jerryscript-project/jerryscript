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

#define DEFINE_BACKET_TYPE(NAME, KEY_TYPE, VALUE_TYPE) \
typedef struct \
{ \
  KEY_TYPE key; \
  VALUE_TYPE value; \
} \
NAME##_backet;

TODO (/*Rewrite to NAME##_backet **backets when neccesary*/)
#define DEFINE_HASH_TYPE(NAME, KEY_TYPE, VALUE_TYPE) \
typedef struct \
{ \
  NAME##_backet *backets; \
  uint8_t *lens; \
  uint8_t size; \
  uint8_t max_lens; \
} \
NAME##_hash_table;

#define HASH_INIT(NAME, SIZE) \
do { \
  NAME.size = SIZE; \
  size_t backets_size = mem_heap_recommend_allocation_size (SIZE * sizeof (NAME##_backet)); \
  size_t lens_size = mem_heap_recommend_allocation_size (SIZE); \
  NAME.backets = (NAME##_backet *) mem_heap_alloc_block (backets_size, MEM_HEAP_ALLOC_SHORT_TERM); \
  NAME.lens = mem_heap_alloc_block (lens_size, MEM_HEAP_ALLOC_SHORT_TERM); \
  __memset (NAME.backets, 0, SIZE); \
  __memset (NAME.lens, 0, SIZE); \
  NAME.max_lens = 1; \
} while (0);

#define HASH_FREE(NAME) \
do { \
  mem_heap_free_block ((uint8_t *) NAME.backets); \
  mem_heap_free_block (NAME.lens); \
} while (0)

#define DEFINE_HASH_INSERT(NAME, KEY_TYPE, VALUE_TYPE) \
static void hash_insert_##NAME (KEY_TYPE key, VALUE_TYPE value) __unused; \
static void hash_insert_##NAME (KEY_TYPE key, VALUE_TYPE value) { \
  NAME##_backet backet = (NAME##_backet) { .key = key, .value = value }; \
  uint8_t hash = KEY_TYPE##_hash (key); \
  JERRY_ASSERT (hash < NAME.size); \
  JERRY_ASSERT (NAME.backets != NULL); \
  JERRY_ASSERT (NAME.lens[hash] == 0); \
  NAME.backets[hash] = backet; \
  NAME.lens[hash] = 1; \
}

#define HASH_INSERT(NAME, KEY, VALUE) \
do { \
  hash_insert_##NAME (KEY, VALUE); \
} while (0)

#define DEFINE_HASH_LOOKUP(NAME, KEY_TYPE, VALUE_TYPE) \
static VALUE_TYPE *lookup_##NAME (KEY_TYPE key) __unused; \
static VALUE_TYPE *lookup_##NAME (KEY_TYPE key) \
{ \
  uint8_t hash = KEY_TYPE##_hash (key); \
  JERRY_ASSERT (hash < NAME.size); \
  if (NAME.lens[hash] == 0) \
  { \
    return NULL; \
  } \
  if (KEY_TYPE##_equal (NAME.backets[hash].key, key)) \
  { \
    return &NAME.backets[hash].value; \
  } \
  return NULL; \
}

#define HASH_LOOKUP(NAME, KEY) \
lookup_##NAME (KEY)

#define HASH_TABLE(NAME, KEY_TYPE, VALUE_TYPE) \
DEFINE_BACKET_TYPE (NAME, KEY_TYPE, VALUE_TYPE) \
DEFINE_HASH_TYPE (NAME, KEY_TYPE, VALUE_TYPE) \
NAME##_hash_table NAME; \
DEFINE_HASH_INSERT (NAME, KEY_TYPE, VALUE_TYPE) \
DEFINE_HASH_LOOKUP (NAME, KEY_TYPE, VALUE_TYPE)

#define STATIC_HASH_TABLE(NAME, KEY_TYPE, VALUE_TYPE) \
DEFINE_BACKET_TYPE (NAME, KEY_TYPE, VALUE_TYPE) \
DEFINE_HASH_TYPE (NAME, KEY_TYPE, VALUE_TYPE) \
static NAME##_hash_table NAME; \
DEFINE_HASH_INSERT (NAME, KEY_TYPE, VALUE_TYPE) \
DEFINE_HASH_LOOKUP (NAME, KEY_TYPE, VALUE_TYPE)

#endif /* HASH_TABLE_H */
