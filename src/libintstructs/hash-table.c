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

#include "hash-table.h"
#include "array-list.h"
#include "mem-heap.h"
#include "jerry-libc.h"

#define HASH_MAP_MAGIC 0x67

typedef struct
{
  uint16_t (*hash) (void *);
  array_list *data;
  uint16_t size;
  uint8_t magic;
  uint8_t key_size;
  uint8_t value_size;
  mem_heap_alloc_term_t alloc_term;
}
hash_table_int;

static hash_table_int *
extract_header (hash_table ht)
{
  JERRY_ASSERT (ht != null_hash);
  hash_table_int *hti = (hash_table_int *) ht;
  JERRY_ASSERT (hti->magic == HASH_MAP_MAGIC);
  return hti;
}

static uint8_t
bucket_size (hash_table_int *hti)
{
  return (uint8_t) (hti->key_size + hti->value_size);
}

static array_list
get_list (hash_table_int *h, uint16_t i)
{
  return h->data[i];
}

static void
set_list (hash_table_int *h, uint16_t i, array_list al)
{
  h->data[i] = al;
}

void
hash_table_insert (hash_table ht, void *key, void *value)
{
  hash_table_int *hti = extract_header (ht);
  uint16_t index = hti->hash (key);
  JERRY_ASSERT (index < hti->size);
  array_list list = get_list (hti, index);
  if (list == null_list)
  {
    list = array_list_init (bucket_size (hti));
  }
  uint8_t *bucket = mem_heap_alloc_block (bucket_size (hti), hti->alloc_term);
  __memcpy (bucket, key, hti->key_size);
  __memcpy (bucket + hti->key_size, value, hti->value_size);
  list = array_list_append (list, bucket);
  hti->data[index] = list;
  mem_heap_free_block (bucket);
}

void *
hash_table_lookup (hash_table ht, void *key)
{
  JERRY_ASSERT (key != NULL);
  hash_table_int *h = extract_header (ht);
  uint16_t index = h->hash (key);
  array_list al = get_list (h, index);
  if (al == null_list)
  {
    return NULL;
  }
  for (uint16_t i = 0; i < array_list_len (al); i++)
  {
    uint8_t *bucket = array_list_element (al, i);
    JERRY_ASSERT (bucket != NULL);
    if (!__memcmp (bucket, key, h->key_size))
    {
      return bucket + h->key_size;
    }
  }
  return NULL;
}

hash_table
hash_table_init (uint8_t key_size, uint8_t value_size, uint16_t size,
                 uint16_t (*hash) (void *), mem_heap_alloc_term_t alloc_term)
{
  hash_table_int *res = (hash_table_int *) mem_heap_alloc_block (sizeof (hash_table_int), alloc_term);
  __memset (res, 0, sizeof (hash_table_int));
  res->magic = HASH_MAP_MAGIC;
  res->key_size = key_size;
  res->value_size = value_size;
  res->size = size;
  res->alloc_term = alloc_term;
  res->data = (array_list *) mem_heap_alloc_block (size * sizeof (array_list), alloc_term);
  __memset (res->data, 0, size * sizeof (array_list));
  res->hash = hash;
  return res;
}

void
hash_table_free (hash_table ht)
{
  hash_table_int *h = extract_header (ht);
  for (uint16_t i = 0; i < h->size; i++)
  {
    array_list al = get_list (h, i);
    if (al != null_list)
    {
      array_list_free (al);
      set_list (h, i, null_list);
    }
  }
  mem_heap_free_block ((uint8_t *) h->data);
  mem_heap_free_block ((uint8_t *) h);
}
