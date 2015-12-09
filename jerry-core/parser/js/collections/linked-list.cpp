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

#include "jrt-libc-includes.h"
#include "jsp-mm.h"
#include "linked-list.h"

/**
 * Header of a linked list's chunk
 */
typedef struct linked_list_chunk_header
{
  struct linked_list_chunk_header *next_p; /**< pointer to next chunk of the list */
} linked_list_chunk_header;

/**
 * Header of a linked list
 */
typedef struct
{
  uint16_t list_length; /**< number of elements */
  uint16_t element_size; /**< size of an element */
} linked_list_header;

#define ASSERT_LIST(list) \
do { \
  linked_list_header *header = (linked_list_header *) list; \
  JERRY_ASSERT (header); \
} while (0);

/**
 * Calculate size of a linked list's chunk
 *
 * @return size of the chunk's data space
 */
static size_t
linked_list_block_size (bool is_first_chunk) /**< is it first chunk (chunk, containing header)? */
{
  if (is_first_chunk)
  {
    return (jsp_mm_recommend_size (sizeof (linked_list_header) + sizeof (linked_list_chunk_header) + 1u)
            - sizeof (linked_list_header) - sizeof (linked_list_chunk_header));
  }
  else
  {
    return (jsp_mm_recommend_size (sizeof (linked_list_chunk_header) + 1u) - sizeof (linked_list_chunk_header));
  }
} /* linked_list_block_size */

/**
 * Initialize linked list
 *
 * @return linked list's identifier
 */
linked_list
linked_list_init (size_t element_size) /**< size of a linked list's element */
{
  JERRY_ASSERT (element_size <= linked_list_block_size (true));
  size_t size = sizeof (linked_list_header) + sizeof (linked_list_chunk_header) + linked_list_block_size (true);

  linked_list list = (linked_list) jsp_mm_alloc (size);
  JERRY_ASSERT (list != null_list);

  linked_list_header *header_p = (linked_list_header *) list;

  header_p->element_size = (uint16_t) element_size;
  JERRY_ASSERT (header_p->element_size == element_size);

  header_p->list_length = 0;

  linked_list_chunk_header* chunk_header_p = (linked_list_chunk_header *) (header_p + 1u);

  chunk_header_p->next_p = NULL;

  return list;
} /* linked_list_init */

/**
 * Create and append new chunk to list
 *
 * @return pointer to the new chunk
 */
static linked_list_chunk_header *
linked_list_append_new_chunk (linked_list_header *header_p, /**< linked list's header */
                              linked_list_chunk_header *last_chunk_header_p) /**< last chunk of the list */
{
  JERRY_ASSERT (header_p != NULL && last_chunk_header_p != NULL);

  JERRY_ASSERT (header_p->element_size <= linked_list_block_size (false));
  size_t size = sizeof (linked_list_chunk_header) + linked_list_block_size (false);

  linked_list_chunk_header *new_chunk_header_p = (linked_list_chunk_header *) jsp_mm_alloc (size);
  JERRY_ASSERT (new_chunk_header_p != NULL);

  new_chunk_header_p->next_p = NULL;

  JERRY_ASSERT (last_chunk_header_p->next_p == NULL);
  last_chunk_header_p->next_p = new_chunk_header_p;

  return new_chunk_header_p;
} /* linked_list_append_new_chunk */

/**
 * Free the linked list
 */
void
linked_list_free (linked_list list) /**< linked list's identifier */
{
  ASSERT_LIST (list);

  linked_list_header *header_p = (linked_list_header *) list;
  linked_list_chunk_header *first_chunk_header_p = (linked_list_chunk_header *) (header_p + 1u);

  linked_list_chunk_header *iter_p = first_chunk_header_p->next_p;
  while (iter_p != NULL)
  {
    linked_list_chunk_header *iter_next_p = iter_p->next_p;
    jsp_mm_free (iter_p);

    iter_p = iter_next_p;
  }

  jsp_mm_free (header_p);
} /* linked_list_free */

/**
 * Get pointer to next element of the list
 *
 * @return pointer to the next element's area,
 *         or NULL - in case end of list was reached.
 */
static uint8_t*
linked_list_switch_to_next_elem (linked_list_header *header_p, /**< list header */
                                 linked_list_chunk_header **in_out_chunk_header_p, /**< list iterator (in case end
                                                                                    *   of list was reached,
                                                                                    *   the iterator points
                                                                                    *   to last chunk of the list) */
                                 uint8_t *raw_elem_ptr_p) /**< element to get the next element for */
{
  linked_list_chunk_header *chunk_header_p = *in_out_chunk_header_p;

  const size_t element_size = header_p->element_size;
  const bool is_first_chunk = ((linked_list_chunk_header *) (header_p + 1u) == chunk_header_p);

  JERRY_ASSERT (raw_elem_ptr_p + element_size
                <= (uint8_t *) (chunk_header_p + 1u) + linked_list_block_size (is_first_chunk));

  const size_t elements_in_chunk = linked_list_block_size (is_first_chunk) / element_size;

  uint8_t *raw_start_p = (uint8_t *) (chunk_header_p + 1u);

  JERRY_ASSERT (raw_elem_ptr_p >= raw_start_p);
  size_t element_offset = (size_t) (raw_elem_ptr_p - raw_start_p) / element_size;

  if (element_offset == elements_in_chunk - 1)
  {
    linked_list_chunk_header *next_chunk_header_p = chunk_header_p->next_p;

    if (next_chunk_header_p == NULL)
    {
      return NULL;
    }
    else
    {
      *in_out_chunk_header_p = next_chunk_header_p;
      return (uint8_t *) (next_chunk_header_p + 1u);
    }
  }
  else
  {
    JERRY_ASSERT (element_offset < elements_in_chunk - 1u);

    return (raw_elem_ptr_p + element_size);
  }
} /* linked_list_switch_to_next_elem */

/**
 * Get pointer to the linked list's element
 */
void *
linked_list_element (linked_list list, /**< linked list's identifier */
                     size_t element_num) /**< index of the element */
{
  ASSERT_LIST (list);

  linked_list_header *header_p = (linked_list_header *) list;

  if (element_num >= header_p->list_length)
  {
    return NULL;
  }

  linked_list_chunk_header *list_chunk_iter_p = (linked_list_chunk_header *) (header_p + 1u);

  uint8_t *element_iter_p = (uint8_t *) (list_chunk_iter_p + 1);

  for (size_t i = 0; i < element_num; i++)
  {
    element_iter_p = linked_list_switch_to_next_elem (header_p, &list_chunk_iter_p, element_iter_p);

    if (element_iter_p == NULL)
    {
      return NULL;
    }
  }

  return element_iter_p;
} /* linked_list_element */

/**
 * Set linked list's element
 */
void
linked_list_set_element (linked_list list, /**< linked list's identifier */
                         size_t element_num, /**< index of element to set */
                         void *element_p) /**< pointer to new value of the element */
{
  if (element_p == NULL)
  {
    return;
  }

  ASSERT_LIST (list);

  linked_list_header *header_p = (linked_list_header *) list;
  linked_list_chunk_header *list_chunk_iter_p = (linked_list_chunk_header *) (header_p + 1u);

  uint8_t *element_iter_p = (uint8_t *) (list_chunk_iter_p + 1u);

  for (size_t i = 0; i < element_num; i++)
  {
    element_iter_p = linked_list_switch_to_next_elem (header_p, &list_chunk_iter_p, element_iter_p);

    if (element_iter_p == NULL)
    {
      JERRY_ASSERT (element_num >= header_p->list_length);

      linked_list_append_new_chunk (header_p, list_chunk_iter_p);
      list_chunk_iter_p = list_chunk_iter_p->next_p;

      element_iter_p = (uint8_t *) (list_chunk_iter_p + 1u);
    }
  }

  if (element_num + 1 > header_p->list_length)
  {
    header_p->list_length = (uint16_t) (element_num + 1u);
    JERRY_ASSERT (header_p->list_length == element_num + 1u);
  }

  JERRY_ASSERT (element_iter_p != NULL);
  memcpy (element_iter_p, element_p, header_p->element_size);
} /* linked_list_set_element */

/**
 * Remove specified element from the linked list
 */
void
linked_list_remove_element (linked_list list, /**< linked list's identifier */
                            size_t element_num) /**< position of the element */
{
  ASSERT_LIST (list);

  linked_list_header *header_p = (linked_list_header *) list;

  linked_list_chunk_header *list_chunk_iter_p = (linked_list_chunk_header *) (header_p + 1u);
  linked_list_chunk_header *chunk_prev_to_chunk_with_last_elem_p = list_chunk_iter_p;

  const size_t list_length = header_p->list_length;
  const size_t element_size = header_p->element_size;

  JERRY_ASSERT (element_num < list_length);

  uint8_t *element_iter_p = (uint8_t *) (list_chunk_iter_p + 1u);

  for (size_t i = 0; i < element_num; i++)
  {
    chunk_prev_to_chunk_with_last_elem_p = list_chunk_iter_p;
    element_iter_p = linked_list_switch_to_next_elem (header_p, &list_chunk_iter_p, element_iter_p);
    JERRY_ASSERT (element_iter_p != NULL);
  }

  uint8_t *next_elem_iter_p = linked_list_switch_to_next_elem (header_p, &list_chunk_iter_p, element_iter_p);

  JERRY_ASSERT ((element_num + 1 == list_length && next_elem_iter_p == NULL)
                || (next_elem_iter_p != NULL));

  for (size_t i = element_num + 1; i < list_length; i++)
  {
    JERRY_ASSERT (next_elem_iter_p != NULL);
    memcpy (element_iter_p, next_elem_iter_p, element_size);

    chunk_prev_to_chunk_with_last_elem_p = list_chunk_iter_p;

    element_iter_p = next_elem_iter_p;
    next_elem_iter_p = linked_list_switch_to_next_elem (header_p, &list_chunk_iter_p, next_elem_iter_p);
  }

  if (list_chunk_iter_p != chunk_prev_to_chunk_with_last_elem_p)
  {
    JERRY_ASSERT (chunk_prev_to_chunk_with_last_elem_p->next_p == list_chunk_iter_p);

    jsp_mm_free (list_chunk_iter_p);
    chunk_prev_to_chunk_with_last_elem_p->next_p = NULL;
  }

  header_p->list_length--;
} /* linked_list_remove_element */

/**
 * Get length of the linked list
 *
 * @return length
 */
uint16_t
linked_list_get_length (linked_list list) /**< linked list's identifier */
{
  ASSERT_LIST (list);

  linked_list_header *header_p = (linked_list_header *) list;
  return header_p->list_length;
} /* linked_list_get_length */
