/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#include "array-list.h"
#include "hash-table.h"
#include "jrt-libc-includes.h"
#include "jsp-mm.h"

typedef struct
{
  uint16_t (*hash) (void *);
  array_list *data;
  uint16_t size;
  uint8_t key_size;
  uint8_t value_size;
} hash_table_int;

static hash_table_int *
extract_header (hash_table ht)
{
  JERRY_ASSERT (ht != null_hash);
  hash_table_int *hti = (hash_table_int *) ht;
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
  uint8_t *bucket = (uint8_t *) jsp_mm_alloc (bucket_size (hti));
  memcpy (bucket, key, hti->key_size);
  memcpy (bucket + hti->key_size, value, hti->value_size);
  list = array_list_append (list, bucket);
  hti->data[index] = list;
  jsp_mm_free (bucket);
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
    uint8_t *bucket = (uint8_t*) array_list_element (al, i);
    JERRY_ASSERT (bucket != NULL);
    if (!memcmp (bucket, key, h->key_size))
    {
      return bucket + h->key_size;
    }
  }
  return NULL;
}

hash_table
hash_table_init (uint8_t key_size, uint8_t value_size, uint16_t size,
                 uint16_t (*hash) (void *))
{
  hash_table_int *res = (hash_table_int *) jsp_mm_alloc (sizeof (hash_table_int));
  memset (res, 0, sizeof (hash_table_int));
  res->key_size = key_size;
  res->value_size = value_size;
  res->size = size;
  res->data = (array_list *) jsp_mm_alloc (size * sizeof (array_list));
  memset (res->data, 0, size * sizeof (array_list));
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
  jsp_mm_free (h->data);
  jsp_mm_free (h);
}
