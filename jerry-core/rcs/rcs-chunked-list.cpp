/* Copyright 2015 Samsung Electronics Co., Ltd.
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

#include "rcs-chunked-list.h"

/**
 * Constructor
 */
void
rcs_chunked_list_t::init (void)
{
  head_p = NULL;
  tail_p = NULL;
} /* rcs_chunked_list_t::init */

/**
 * Destructor
 */
void
rcs_chunked_list_t::free (void)
{
  JERRY_ASSERT (head_p == NULL);
  JERRY_ASSERT (tail_p == NULL);
} /* rcs_chunked_list_t::free */

void
rcs_chunked_list_t::cleanup (void)
{
  while (head_p)
  {
    remove (head_p);
  }
} /* rcs_chunked_list_t::cleanup */

/**
 * Get first node of the list
 *
 * @return pointer to the first node
 */
rcs_chunked_list_t::node_t*
rcs_chunked_list_t::get_first (void)
const
{
  return head_p;
} /* rcs_chunked_list_t::get_first */

/**
 * Get last node of the list
 *
 * @return pointer to the last node
 */
rcs_chunked_list_t::node_t*
rcs_chunked_list_t::get_last (void)
const
{
  return tail_p;
} /* rcs_chunked_list_t::get_last */

/**
 * Get node, previous to specified
 *
 * @return pointer to previous node
 */
rcs_chunked_list_t::node_t*
rcs_chunked_list_t::get_prev (rcs_chunked_list_t::node_t *node_p) /**< node to get previous for */
const
{
  JERRY_ASSERT (node_p != NULL);

  return MEM_CP_GET_POINTER (node_t, node_p->prev_cp);
} /* rcs_chunked_list_t::get_prev */

/**
 * Get node, next to specified
 *
 * @return pointer to next node
 */
rcs_chunked_list_t::node_t*
rcs_chunked_list_t::get_next (rcs_chunked_list_t::node_t *node_p) /**< node to get next for */
const
{
  JERRY_ASSERT (node_p != NULL);

  return MEM_CP_GET_POINTER (node_t, node_p->next_cp);
} /* rcs_chunked_list_t::get_next */

/**
 * Append new node to end of the list
 *
 * @return pointer to the new node
 */
rcs_chunked_list_t::node_t*
rcs_chunked_list_t::append_new (void)
{
  assert_list_is_correct ();

  node_t *node_p = (node_t*) mem_heap_alloc_chunked_block (MEM_HEAP_ALLOC_LONG_TERM);

  set_prev (node_p, tail_p);
  set_next (node_p, NULL);

  if (head_p == NULL)
  {
    JERRY_ASSERT (tail_p == NULL);

    head_p = node_p;
    tail_p = node_p;
  }
  else
  {
    JERRY_ASSERT (tail_p != NULL);

    set_next (tail_p, node_p);

    tail_p = node_p;
  }

  assert_node_is_correct (node_p);

  return node_p;
} /* rcs_chunked_list_t::append_new */

/**
 * Insert new node after the specified node
 *
 * @return pointer to the new node
 */
rcs_chunked_list_t::node_t*
rcs_chunked_list_t::insert_new (rcs_chunked_list_t::node_t* after_p) /**< the node to insert the new node after */
{
  assert_list_is_correct ();

  node_t *node_p = (node_t*) mem_heap_alloc_chunked_block (MEM_HEAP_ALLOC_LONG_TERM);

  JERRY_ASSERT (head_p != NULL);
  JERRY_ASSERT (tail_p != NULL);
  assert_node_is_correct (after_p);

  set_next (after_p, node_p);
  if (tail_p == after_p)
  {
    tail_p = node_p;
  }

  set_prev (node_p, after_p);
  set_next (node_p, NULL);

  assert_node_is_correct (node_p);

  return node_p;
} /* rcs_chunked_list_t::insert_new */

/**
 * Remove specified node
 */
void
rcs_chunked_list_t::remove (rcs_chunked_list_t::node_t* node_p) /**< node to remove */
{
  JERRY_ASSERT (head_p != NULL);
  JERRY_ASSERT (tail_p != NULL);

  assert_node_is_correct (node_p);

  node_t *prev_node_p, *next_node_p;
  prev_node_p = get_prev (node_p);
  next_node_p = get_next (node_p);

  if (prev_node_p == NULL)
  {
    JERRY_ASSERT (head_p == node_p);
    head_p = next_node_p;
  }
  else
  {
    set_next (prev_node_p, next_node_p);
  }

  if (next_node_p == NULL)
  {
    JERRY_ASSERT (tail_p == node_p);
    tail_p = prev_node_p;
  }
  else
  {
    set_prev (next_node_p, prev_node_p);
  }

  mem_heap_free_block (node_p);

  assert_list_is_correct ();
} /* rcs_chunked_list_t::remove */

/**
 * Find node containing space, pointed by specified pointer
 *
 * @return pointer to the node that contains the pointed area
 */
rcs_chunked_list_t::node_t*
rcs_chunked_list_t::get_node_from_pointer (void* ptr) /**< the pointer value */
const
{
  node_t *node_p = (node_t*) mem_heap_get_chunked_block_start (ptr);

  assert_node_is_correct (node_p);

  return node_p;
} /* rcs_chunked_list_t::get_node_from_pointer */

/**
 * Get the node's data space
 *
 * @return pointer to beginning of the node's data space
 */
uint8_t *
rcs_chunked_list_t::get_node_data_space (rcs_chunked_list_t::node_t *node_p) /**< the node */
const
{
  assert_node_is_correct (node_p);

  return (uint8_t *) (node_p + 1);
} /* rcs_chunked_list_t::get_node_data_space */

/**
 * Get size of a node's data space
 *
 * @return size
 */
size_t
rcs_chunked_list_t::get_node_data_space_size (void)
{
  return rcs_chunked_list_t::get_node_size () - sizeof (node_t);
} /* rcs_chunked_list_t::get_node_data_space_size */

/**
 * Set previous node for the specified node
 */
void
rcs_chunked_list_t::set_prev (rcs_chunked_list_t::node_t *node_p, /**< node to set previous for */
                              rcs_chunked_list_t::node_t *prev_node_p) /**< the previous node */
{
  JERRY_ASSERT (node_p != NULL);

  MEM_CP_SET_POINTER (node_p->prev_cp, prev_node_p);
} /* rcs_chunked_list_t::set_prev */

/**
 * Set next node for the specified node
 */
void
rcs_chunked_list_t::set_next (rcs_chunked_list_t::node_t *node_p, /**< node to set next for */
                              rcs_chunked_list_t::node_t *next_node_p) /**< the next node */
{
  JERRY_ASSERT (node_p != NULL);

  MEM_CP_SET_POINTER (node_p->next_cp, next_node_p);
} /* rcs_chunked_list_t::set_next */

/**
 * Get size of the node
 *
 * @return size of node, including header and data space
 */
size_t
rcs_chunked_list_t::get_node_size (void)
{
  size_t size = mem_heap_recommend_allocation_size (sizeof (node_t) + 1u);

  JERRY_ASSERT (size != 0 && size >= sizeof (node_t));

  return size;
} /* rcs_chunked_list_t::get_node_size */

/**
 * Assert that the list state is correct
 */
void
rcs_chunked_list_t::assert_list_is_correct (void)
const
{
#ifndef JERRY_DISABLE_HEAVY_DEBUG
  for (node_t *node_iter_p = get_first ();
       node_iter_p != NULL;
       node_iter_p = get_next (node_iter_p))
  {
    node_t *prev_node_p = get_prev (node_iter_p);
    node_t *next_node_p = get_next (node_iter_p);

    JERRY_ASSERT ((node_iter_p == head_p
                   && prev_node_p == NULL)
                  || (node_iter_p != head_p
                      && prev_node_p != NULL
                      && get_next (prev_node_p) == node_iter_p));
    JERRY_ASSERT ((node_iter_p == tail_p
                   && next_node_p == NULL)
                  || (node_iter_p != tail_p
                      && next_node_p != NULL
                      && get_prev (next_node_p) == node_iter_p));
  }
#endif /* !JERRY_DISABLE_HEAVY_DEBUG */
} /* rcs_chunked_list_t::assert_list_is_correct */

/**
 * Assert that state of specified node is correct
 */
void
rcs_chunked_list_t::assert_node_is_correct (const rcs_chunked_list_t::node_t* node_p) /**< the node */
const
{
#ifndef JERRY_DISABLE_HEAVY_DEBUG
  JERRY_ASSERT (node_p != NULL);

  assert_list_is_correct ();

  bool is_in_list = false;
  for (node_t *node_iter_p = get_first ();
       node_iter_p != NULL;
       node_iter_p = get_next (node_iter_p))
  {
    if (node_iter_p == node_p)
    {
      is_in_list = true;

      break;
    }
  }

  JERRY_ASSERT (is_in_list);
#else /* !JERRY_DISABLE_HEAVY_DEBUG */
  (void) node_p;
#endif /* JERRY_DISABLE_HEAVY_DEBUG */
} /* rcs_chunked_list_t::assert_node_is_correct */
