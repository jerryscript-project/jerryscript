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

#include "linked-list.h"
#include "jrt-libc-includes.h"
#include "mem-heap.h"

typedef struct linked_list_header
{
  struct linked_list_header *next;
  uint16_t element_size;
} linked_list_header;

#define ASSERT_LIST(list) \
do { \
  linked_list_header *header = (linked_list_header *) list; \
  JERRY_ASSERT (header); \
} while (0);

static size_t linked_list_block_size (uint16_t element_size)
{
  return mem_heap_recommend_allocation_size (sizeof (linked_list_header) + element_size) - sizeof (linked_list_header);
}

linked_list
linked_list_init (uint16_t element_size)
{
  size_t size = sizeof (linked_list_header) + linked_list_block_size (element_size);
  linked_list list = (linked_list) mem_heap_alloc_block (size, MEM_HEAP_ALLOC_SHORT_TERM);
  if (list == null_list)
  {
    printf ("Out of memory");
    JERRY_UNREACHABLE ();
  }
  memset (list, 0, size);
  linked_list_header* header = (linked_list_header *) list;
  header->next = null_list;
  header->element_size = element_size;
  return list;
}

void
linked_list_free (linked_list list)
{
  ASSERT_LIST (list);
  linked_list_header *header = (linked_list_header *) list;
  if (header->next)
  {
    linked_list_free ((linked_list) header->next);
  }
  mem_heap_free_block (list);
}

void *
linked_list_element (linked_list list, size_t element_num)
{
  ASSERT_LIST (list);
  linked_list_header *header = (linked_list_header *) list;
  size_t block_size = linked_list_block_size (header->element_size);
  linked_list raw = list + sizeof (linked_list_header);
  if (block_size < header->element_size * (element_num + 1))
  {
    if (header->next)
    {
      return linked_list_element ((linked_list) header->next,
                                  element_num - (block_size / header->element_size));
    }
    else
    {
      return NULL;
    }
  }
  raw += header->element_size * element_num;
  return raw;
}

void
linked_list_set_element (linked_list list, size_t element_num, void *element)
{
  ASSERT_LIST (list);
  linked_list_header *header = (linked_list_header *) list;
  size_t block_size = linked_list_block_size (header->element_size);
  uint8_t *raw = (uint8_t *) (header + 1);
  if (block_size < header->element_size * (element_num + 1))
  {
    if (header->next == null_list)
    {
      header->next = (linked_list_header *) linked_list_init (header->element_size);
    }
    linked_list_set_element ((linked_list) header->next,
                             element_num - (block_size / header->element_size),
                             element);
    return;
  }
  if (element == NULL)
  {
    return;
  }
  memcpy (raw + element_num * header->element_size, element, header->element_size);
}
