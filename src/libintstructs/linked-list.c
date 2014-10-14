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

#include "linked-list.h"
#include "jerry-libc.h"
#include "globals.h"
#include "mem-heap.h"
#include "lp-string.h"

#define ASSERT_LIST(list) \
do { \
  linked_list_header *header = (linked_list_header *) list; \
  JERRY_ASSERT (header); \
  JERRY_ASSERT (header->magic == LINKED_LIST_MAGIC); \
} while (0);

linked_list
linked_list_init (size_t element_size)
{
  size_t size = mem_heap_recommend_allocation_size (element_size);
  linked_list list = mem_heap_alloc_block (size, MEM_HEAP_ALLOC_SHORT_TERM);
  if (list == null_list)
  {
    __printf ("Out of memory");
    JERRY_UNREACHABLE ();
  }
  __memset (list, 0, size);
  linked_list_header* header = (linked_list_header *) list;
  header->magic = LINKED_LIST_MAGIC;
  header->prev = header->next = null_list;
  header->block_size = (uint8_t) (size - sizeof (linked_list_header));
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
linked_list_element (linked_list list, size_t element_size, size_t element_num)
{
  ASSERT_LIST (list);
  linked_list_header *header = (linked_list_header *) list;
  linked_list raw = list + sizeof (linked_list_header);
  if (header->block_size < element_size * (element_num + 1))
  {
    if (header->next)
    {
      return linked_list_element ((linked_list) header->next, element_size,
                                  element_num - (header->block_size / element_size));
    }
    else
    {
      return NULL;
    }
  }
  raw += element_size * element_num;
  return raw;
}

void
linked_list_set_element (linked_list list, size_t element_size, size_t element_num, void *element)
{
  ASSERT_LIST (list);
  linked_list_header *header = (linked_list_header *) list;
  linked_list raw = list + sizeof (linked_list_header);
  if (header->block_size < element_size * (element_num + 1))
  {
    if (header->next == null_list)
    {
      header->next = (linked_list_header *) linked_list_init (element_size);
      header->next->prev = header;
    }
    linked_list_set_element ((linked_list) header->next, element_size,
                             element_num - (header->block_size / element_size),
                             element);
    return;
  }
  if (element == NULL)
  {
    return;
  }
  switch (element_size)
  {
    case 1:
    {
      ((uint8_t *) raw)[element_num] = *((uint8_t *) element);
      break;
    }
    case 2:
    {
      ((uint16_t *) raw)[element_num] = *((uint16_t *) element);
      break;
    }
    case 4:
    {
      ((uint32_t *) raw)[element_num] = *((uint32_t *) element);
      break;
    }
    case 8:
    {
      ((uint64_t *) raw)[element_num] = *((uint64_t *) element);
      break;
    }
#ifdef __TARGET_HOST_x64
    case sizeof (lp_string):
    {
      ((lp_string *) raw)[element_num] = *((lp_string *) element);
      break;
    }
#endif
    default:
    {
      __printf ("Element_size %d is not supported\n", element_size);
      JERRY_UNIMPLEMENTED ();
    }
  }
}
