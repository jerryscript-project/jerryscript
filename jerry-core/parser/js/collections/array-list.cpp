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
#include "jrt-libc-includes.h"
#include "jsp-mm.h"

typedef struct
{
  uint8_t element_size;
  size_t len;
  size_t size;
} array_list_header;

static array_list_header *
extract_header (array_list al)
{
  JERRY_ASSERT (al != null_list);
  array_list_header *header = (array_list_header *) al;
  return header;
}

static uint8_t *
data (array_list al)
{
  return (uint8_t *) al + sizeof (array_list_header);
}

array_list
array_list_append (array_list al, void *element)
{
  array_list_header *h = extract_header (al);
  if ((h->len + 1) * h->element_size + sizeof (array_list_header) > h->size)
  {
    size_t size = jsp_mm_recommend_size (h->size + h->element_size);
    JERRY_ASSERT (size > h->size);

    uint8_t *new_block_p = (uint8_t *) jsp_mm_alloc (size);
    memcpy (new_block_p, h, h->size);
    memset (new_block_p + h->size, 0, size - h->size);

    jsp_mm_free (h);

    h = (array_list_header *) new_block_p;
    h->size = size;
    al = (array_list) h;
  }
  memcpy (data (al) + (h->len * h->element_size), element, h->element_size);
  h->len++;
  return al;
}

void
array_list_drop_last (array_list al)
{
  array_list_header *h = extract_header (al);
  JERRY_ASSERT (h->len > 0);
  h->len--;
}

void *
array_list_element (array_list al, size_t index)
{
  array_list_header *h = extract_header (al);
  if (h->len <= index)
  {
    return NULL;
  }
  return data (al) + (index * h->element_size);
}

void
array_list_set_element (array_list al, size_t index, void *elem)
{
  array_list_header *h = extract_header (al);
  JERRY_ASSERT (index < h->len);
  memcpy (data (al) + (index * h->element_size), elem, h->element_size);
}

void *
array_list_last_element (array_list al, size_t index)
{
  array_list_header *h = extract_header (al);
  if (index == 0 || index > h->len)
  {
    return NULL;
  }
  return array_list_element (al, (size_t) (h->len - index));
}

void
array_list_set_last_element (array_list al, size_t index, void *elem)
{
  array_list_header *h = extract_header (al);
  JERRY_ASSERT (index != 0 && index <= h->len);
  array_list_set_element (al, (size_t) (h->len - index), elem);
}

array_list
array_list_init (uint8_t element_size)
{
  size_t size = jsp_mm_recommend_size (sizeof (array_list_header));
  array_list_header *header = (array_list_header *) jsp_mm_alloc (size);
  memset (header, 0, size);
  header->element_size = element_size;
  header->len = 0;
  header->size = size;
  return (array_list) header;
}

size_t
array_list_len (array_list al)
{
  array_list_header *h = extract_header (al);
  return h->len;
}

void
array_list_free (array_list al)
{
  array_list_header *h = extract_header (al);
  jsp_mm_free (h);
}
