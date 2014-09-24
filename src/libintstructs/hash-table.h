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

#define DEFINE_BACKET_TYPE(NAME, KEY_TYPE, VALUE_TYPE) \
typedef struct \
{ \
  KEY_TYPE key; \
  VALUE_TYPE value; \
} \
__packed \
NAME##_backet;

#define DEFINE_HASH_TYPE(NAME, KEY_TYPE, VALUE_TYPE) \
typedef struct \
{ \
  uint8_t size; \
  NAME##_backet_stack *backets; \
} \
__packed \
NAME##_hash_table;

#define HASH_INIT(NAME, SIZE) \
do { \
  NAME.size = SIZE; \
  size_t size = mem_heap_recommend_allocation_size (SIZE * sizeof (NAME##_backet_stack)); \
  NAME.backets = (NAME##_backet_stack *) mem_heap_alloc_block (size, MEM_HEAP_ALLOC_SHORT_TERM); \
  __memset (NAME.backets, 0, size); \
} while (0);

#define HASH_FREE(NAME) \
do { \
  for (uint8_t i = 0; i < NAME.size; i++) { \
    if (NAME.backets[i].length != 0) { \
      mem_heap_free_block ((uint8_t *) NAME.backets[i].data); \
    } \
  } \
  mem_heap_free_block ((uint8_t *) NAME.backets); \
} while (0)

#define DEFINE_HASH_INSERT(NAME, KEY_TYPE, VALUE_TYPE) \
static void hash_insert_##NAME (KEY_TYPE key, VALUE_TYPE value) __unused; \
static void hash_insert_##NAME (KEY_TYPE key, VALUE_TYPE value) { \
  NAME##_backet backet = (NAME##_backet) { .key = key, .value = value }; \
  uint8_t hash = KEY_TYPE##_hash (key); \
  JERRY_ASSERT (hash < NAME.size); \
  JERRY_ASSERT (NAME.backets != NULL); \
  if (NAME.backets[hash].length == 0) \
  { \
    size_t stack_size = mem_heap_recommend_allocation_size (0); \
    NAME.backets[hash].data = (NAME##_backet *) mem_heap_alloc_block \
                      (stack_size, MEM_HEAP_ALLOC_SHORT_TERM); \
    NAME.backets[hash].current = 0; \
    NAME.backets[hash].length = (__typeof__ (NAME.backets[hash].length)) \
                                (stack_size / sizeof (NAME##_backet)); \
  } \
  else if (NAME.backets[hash].current == NAME.backets[hash].length) \
  { \
    size_t old_size = NAME.backets[hash].length * sizeof (NAME##_backet); \
    size_t temp1_size = mem_heap_recommend_allocation_size ( \
                        (size_t) (sizeof (NAME##_backet))); \
    size_t new_size = mem_heap_recommend_allocation_size ( \
                      (size_t) (temp1_size + old_size)); \
    NAME##_backet *temp1 = (NAME##_backet *) mem_heap_alloc_block \
                           (temp1_size, MEM_HEAP_ALLOC_SHORT_TERM); \
    NAME##_backet *temp2 = (NAME##_backet *) mem_heap_alloc_block \
                           (old_size, MEM_HEAP_ALLOC_SHORT_TERM); \
    if (temp2 == NULL) \
    { \
      mem_heap_print (true, false, true); \
      JERRY_UNREACHABLE (); \
    } \
    __memcpy (temp2, NAME.backets[hash].data, old_size); \
    mem_heap_free_block ((uint8_t *) NAME.backets[hash].data); \
    mem_heap_free_block ((uint8_t *) temp1); \
    NAME.backets[hash].data = (NAME##_backet *) mem_heap_alloc_block \
                              (new_size, MEM_HEAP_ALLOC_SHORT_TERM); \
    __memcpy (NAME.backets[hash].data, temp2, old_size); \
    mem_heap_free_block ((uint8_t *) temp2); \
    NAME.backets[hash].length = (__typeof__ (NAME.backets[hash].length)) \
                                (new_size / sizeof (NAME##_backet)); \
  } \
  NAME.backets[hash].data[NAME.backets[hash].current++] = backet; \
}

#define HASH_INSERT(NAME, KEY, VALUE) \
do { \
  hash_insert_##NAME (KEY, VALUE); \
} while (0)

#define DEFINE_HASH_LOOKUP(NAME, KEY_TYPE, VALUE_TYPE) \
static VALUE_TYPE *lookup_##NAME (KEY_TYPE key) __unused; \
static VALUE_TYPE *lookup_##NAME (KEY_TYPE key) { \
  uint8_t hash = KEY_TYPE##_hash (key); \
  JERRY_ASSERT (hash < NAME.size); \
  if (NAME.backets[hash].length == 0) { \
    return NULL; \
  } \
  for (size_t i = 0; i < NAME.backets[hash].current; i++) { \
    if (KEY_TYPE##_equal (NAME.backets[hash].data[i].key, key)) { \
      return &NAME.backets[hash].data[i].value; \
    } \
  } \
  return NULL; \
}

#define HASH_LOOKUP(NAME, KEY) \
lookup_##NAME (KEY)

#define HASH_TABLE(NAME, KEY_TYPE, VALUE_TYPE) \
DEFINE_BACKET_TYPE (NAME, KEY_TYPE, VALUE_TYPE) \
DEFINE_STACK_TYPE (NAME##_backet, uint8_t, NAME##_backet) \
DEFINE_HASH_TYPE (NAME, KEY_TYPE, VALUE_TYPE) \
NAME##_hash_table NAME; \
DEFINE_HASH_INSERT (NAME, KEY_TYPE, VALUE_TYPE) \
DEFINE_HASH_LOOKUP (NAME, KEY_TYPE, VALUE_TYPE)

#define STATIC_HASH_TABLE(NAME, KEY_TYPE, VALUE_TYPE) \
DEFINE_BACKET_TYPE (NAME, KEY_TYPE, VALUE_TYPE) \
DEFINE_STACK_TYPE (NAME##_backet, uint8_t, NAME##_backet) \
DEFINE_HASH_TYPE (NAME, KEY_TYPE, VALUE_TYPE) \
static NAME##_hash_table NAME; \
DEFINE_HASH_INSERT (NAME, KEY_TYPE, VALUE_TYPE) \
DEFINE_HASH_LOOKUP (NAME, KEY_TYPE, VALUE_TYPE)

#endif /* HASH_TABLE_H */
