/* Copyright 2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged
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

#include "rcs-allocator.h"
#include "rcs-records.h"

/**
 * Assert that recordset state is correct.
 */
static void
rcs_assert_state_is_correct (rcs_record_set_t *rec_set_p __attr_unused___) /**< recordset */
{
#ifndef JERRY_DISABLE_HEAVY_DEBUG
  size_t node_size_sum = 0;
  size_t record_size_sum = 0;

  rcs_record_t *last_record_p = NULL;
  rcs_record_t *rec_p;
  rcs_record_t *next_rec_p;

  for (rec_p = rcs_record_get_first (rec_set_p);
       rec_p != NULL;
       last_record_p = rec_p, rec_p = next_rec_p)
  {
    JERRY_ASSERT (rcs_record_get_size (rec_p) > 0);
    record_size_sum += rcs_record_get_size (rec_p);

    rcs_chunked_list_t::node_t *node_p = rec_set_p->get_node_from_pointer (rec_p);
    next_rec_p = rcs_record_get_next (rec_set_p, rec_p);
    rcs_chunked_list_t::node_t *next_node_p = NULL;

    if (next_rec_p != NULL)
    {
      next_node_p = rec_set_p->get_node_from_pointer (next_rec_p);
    }

    while (node_p != next_node_p)
    {
      node_p = rec_set_p->get_next (node_p);
      node_size_sum += rcs_get_node_data_space_size ();
    }
  }

  JERRY_ASSERT (node_size_sum == record_size_sum);

  record_size_sum = 0;

  for (rec_p = last_record_p;
       rec_p != NULL;
       rec_p = rcs_record_get_prev (rec_set_p, rec_p))
  {
    record_size_sum += rcs_record_get_size (rec_p);
  }

  JERRY_ASSERT (node_size_sum == record_size_sum);
#endif /* !JERRY_DISABLE_HEAVY_DEBUG */
} /* rcs_assert_state_is_correct */

/**
 * Initialize specified record as free record.
 */
static void
rcs_init_free_record (rcs_record_set_t *rec_set_p, /**< recordset */
                      rcs_record_t *rec_p, /**< record to init as free record */
                      rcs_record_t *prev_rec_p, /**< previous record (or NULL) */
                      size_t size) /**< size, including header */
{
  rcs_record_set_type (rec_p, RCS_RECORD_TYPE_FREE);
  rcs_record_set_prev (rec_set_p, rec_p, prev_rec_p);
  rcs_record_set_size (rec_p, size);
} /* rcs_init_free_record */

/**
 * Check the alignment of the record.
 */
void
rcs_check_record_alignment (rcs_record_t *rec_p) /**< record */
{
  JERRY_ASSERT (rec_p != NULL);

  uintptr_t ptr = (uintptr_t) rec_p;

  JERRY_ASSERT (JERRY_ALIGNUP (ptr, RCS_DYN_STORAGE_LENGTH_UNIT) == ptr);
} /* rcs_check_record_alignment */

/**
 * Get size of a node's data space.
 *
 * @return size
 */
size_t
rcs_get_node_data_space_size (void)
{
  return JERRY_ALIGNDOWN (rcs_chunked_list_t::get_node_data_space_size (), RCS_DYN_STORAGE_LENGTH_UNIT);
} /* rcs_get_node_data_space_size */

/**
 * Get the node's data space.
 *
 * @return pointer to beginning of the node's data space
 */
uint8_t *
rcs_get_node_data_space (rcs_record_set_t *rec_set_p, /**< recordset */
                         rcs_chunked_list_t::node_t *node_p) /**< the node */
{
  uintptr_t unaligned_data_space_start = (uintptr_t) rec_set_p->get_node_data_space (node_p);
  uintptr_t aligned_data_space_start = JERRY_ALIGNUP (unaligned_data_space_start, RCS_DYN_STORAGE_LENGTH_UNIT);

  JERRY_ASSERT (unaligned_data_space_start + rcs_chunked_list_t::get_node_data_space_size ()
                == aligned_data_space_start + rcs_record_set_t::get_node_data_space_size ());

  return (uint8_t *) aligned_data_space_start;
} /* rcs_get_node_data_space */

/**
 * Initialize record in specified place, and, if there is free space
 * before next record, initialize free record for the space.
 */
static void
rcs_alloc_record_in_place (rcs_record_set_t *rec_set_p, /**< recordset */
                           rcs_record_t *place_p, /**< where to initialize the record */
                           rcs_record_t *next_record_p, /**< next allocated record */
                           size_t free_size) /**< size of free part between the allocated record
                                                  and the next allocated record */
{
  const size_t node_data_space_size = rcs_get_node_data_space_size ();

  if (next_record_p != NULL)
  {
    if (free_size == 0)
    {
      rcs_record_set_prev (rec_set_p, next_record_p, place_p);
    }
    else
    {
      rcs_chunked_list_t::node_t *node_p = rec_set_p->get_node_from_pointer (next_record_p);
      uint8_t *node_data_space_p = rcs_get_node_data_space (rec_set_p, node_p);

      JERRY_ASSERT ((uint8_t *) next_record_p < node_data_space_p + node_data_space_size);

      rcs_record_t *free_rec_p;

      if ((uint8_t *) next_record_p >= node_data_space_p + free_size)
      {
        free_rec_p = (rcs_record_t *) ((uint8_t *) next_record_p - free_size);
      }
      else
      {
        size_t size_passed_back = (size_t) ((uint8_t *) next_record_p - node_data_space_p);
        JERRY_ASSERT (size_passed_back < free_size && size_passed_back + node_data_space_size > free_size);

        node_p = rec_set_p->get_prev (node_p);
        node_data_space_p = rcs_get_node_data_space (rec_set_p, node_p);

        free_rec_p = (rcs_record_t *) (node_data_space_p + node_data_space_size - \
                                      (free_size - size_passed_back));
      }

      rcs_init_free_record (rec_set_p, free_rec_p, place_p, free_size);
    }
  }
  else if (free_size != 0)
  {
    rcs_chunked_list_t::node_t *node_p = rec_set_p->get_node_from_pointer (place_p);
    JERRY_ASSERT (node_p != NULL);

    rcs_chunked_list_t::node_t *next_node_p = rec_set_p->get_next (node_p);

    while (next_node_p != NULL)
    {
      node_p = next_node_p;
      next_node_p = rec_set_p->get_next (node_p);
    }

    uint8_t *node_data_space_p = rcs_get_node_data_space (rec_set_p, node_p);
    const size_t node_data_space_size = rcs_get_node_data_space_size ();

    rcs_record_t *free_rec_p = (rcs_record_t *) (node_data_space_p + node_data_space_size - free_size);
    rcs_init_free_record (rec_set_p, free_rec_p, place_p, free_size);
  }
} /* rcs_alloc_record_in_place */

/**
 * Allocate record of specified size.
 *
 * @return record identifier
 */
static rcs_record_t *
rcs_alloc_space_for_record (rcs_record_set_t *rec_set_p, /**< recordset */
                            rcs_record_t **out_prev_rec_p, /**< out: pointer to record, previous to the allocated,
                                                        *   or NULL if the allocated record is the first */
                            size_t bytes) /**< size */
{
  rcs_assert_state_is_correct (rec_set_p);

  JERRY_ASSERT (JERRY_ALIGNUP (bytes, RCS_DYN_STORAGE_LENGTH_UNIT) == bytes);
  JERRY_ASSERT (out_prev_rec_p != NULL);

  rcs_record_t *rec_p = NULL;
  *out_prev_rec_p = NULL;

  const size_t node_data_space_size = rcs_get_node_data_space_size ();

  for (rec_p = rcs_record_get_first (rec_set_p);
       rec_p != NULL;
       *out_prev_rec_p = rec_p, rec_p = rcs_record_get_next (rec_set_p, rec_p))
  {
    if (RCS_RECORD_IS_FREE (rec_p))
    {
      rcs_record_t *next_rec_p = rcs_record_get_next (rec_set_p, rec_p);
      size_t record_size = rcs_record_get_size (rec_p);

      if (record_size >= bytes)
      {
        /* Record size is sufficient. */
        rcs_alloc_record_in_place (rec_set_p, rec_p, next_rec_p, record_size - bytes);
        return rec_p;
      }

      rcs_chunked_list_t::node_t *node_p = rec_set_p->get_node_from_pointer (rec_p);
      uint8_t *node_data_space_p = rcs_get_node_data_space (rec_set_p, node_p);
      uint8_t *node_data_space_end_p = node_data_space_p + node_data_space_size;
      uint8_t *rec_space_p = (uint8_t *) rec_p;

      if (rec_space_p + record_size >= node_data_space_end_p)
      {
        /* Record lies up to end of node's data space size,
         * thus it can be extended up to necessary size. */
        while (record_size < bytes)
        {
          node_p = rec_set_p->insert_new (node_p);
          record_size += node_data_space_size;
        }

        rcs_alloc_record_in_place (rec_set_p, rec_p, next_rec_p, record_size - bytes);
        return rec_p;
      }

      if (next_rec_p == NULL)
      {
        /* There are no more records in the storage,
         * so we should append a new record. */
        break;
      }

      JERRY_ASSERT (!RCS_RECORD_IS_FREE (rec_p));
    }
  }

  /* Free record of sufficient size was not found. */
  rcs_chunked_list_t::node_t *node_p = rec_set_p->append_new ();
  rcs_record_t *new_rec_p = (rcs_record_t *) rcs_get_node_data_space (rec_set_p, node_p);

  size_t allocated_size = node_data_space_size;

  while (allocated_size < bytes)
  {
    allocated_size += node_data_space_size;
    rec_set_p->append_new ();
  }

  rcs_alloc_record_in_place (rec_set_p, new_rec_p, NULL, allocated_size - bytes);

  return new_rec_p;
} /* rcs_alloc_space_for_record */

/**
 * Allocate and initialize a new record.
 *
 * @return pointer to the new record
 */
rcs_record_t *
rcs_alloc_record (rcs_record_set_t *rec_set_p, /**< recordset */
                  rcs_record_type_t type, /**< type for the new record */
                  size_t size) /**< allocation size */
{
  JERRY_ASSERT (type >= RCS_RECORD_TYPE_FIRST && type <= RCS_RECORD_TYPE_LAST);

  rcs_record_t *prev_rec_p;
  rcs_record_t *rec_p = (rcs_record_t *) rcs_alloc_space_for_record (rec_set_p, &prev_rec_p, size);

  rcs_record_set_type (rec_p, type);
  rcs_record_set_size (rec_p, size);
  rcs_record_set_prev (rec_set_p, rec_p, prev_rec_p);

  rcs_assert_state_is_correct (rec_set_p);

  return rec_p;
} /* alloc_record */

/**
 * Free the specified record.
 */
void
rcs_free_record (rcs_record_set_t *rec_set_p, /**< recordset */
                 rcs_record_t *record_p) /**< record to free */
{
  JERRY_ASSERT (record_p != NULL);

  rcs_assert_state_is_correct (rec_set_p);

  rcs_record_t *prev_rec_p = rcs_record_get_prev (rec_set_p, record_p);

  rcs_init_free_record (rec_set_p, record_p, prev_rec_p, rcs_record_get_size (record_p));

  /* Merge adjacent free records, if there are any,
   * and free nodes of chunked list that became unused. */
  rcs_record_t *rec_from_p = record_p;
  rcs_record_t *rec_to_p = rcs_record_get_next (rec_set_p, record_p);

  if (prev_rec_p != NULL && RCS_RECORD_IS_FREE (prev_rec_p))
  {
    rec_from_p = prev_rec_p;
    prev_rec_p = rcs_record_get_prev (rec_set_p, rec_from_p);
  }

  if (rec_to_p != NULL && RCS_RECORD_IS_FREE (rec_to_p))
  {
    rec_to_p = rcs_record_get_next (rec_set_p, rec_to_p);
  }

  JERRY_ASSERT (rec_from_p != NULL && RCS_RECORD_IS_FREE (rec_from_p));
  JERRY_ASSERT (rec_to_p == NULL || !RCS_RECORD_IS_FREE (rec_to_p));

  rcs_chunked_list_t::node_t *node_from_p = rec_set_p->get_node_from_pointer (rec_from_p);
  rcs_chunked_list_t::node_t *node_to_p = NULL;

  if (rec_to_p != NULL)
  {
    node_to_p = rec_set_p->get_node_from_pointer (rec_to_p);
  }

  const size_t node_data_space_size = rcs_get_node_data_space_size ();

  uint8_t *rec_from_beg_p = (uint8_t *) rec_from_p;
  uint8_t *rec_to_beg_p = (uint8_t *) rec_to_p;
  size_t free_size;

  if (node_from_p == node_to_p)
  {
    JERRY_ASSERT (rec_from_beg_p + rcs_record_get_size (rec_from_p) <= rec_to_beg_p);
    free_size = (size_t) (rec_to_beg_p - rec_from_beg_p);
  }
  else
  {
    rcs_chunked_list_t::node_t *iter_node_p;
    rcs_chunked_list_t::node_t *iter_next_node_p;

    for (iter_node_p = rec_set_p->get_next (node_from_p);
         iter_node_p != node_to_p;
         iter_node_p = iter_next_node_p)
    {
      iter_next_node_p = rec_set_p->get_next (iter_node_p);
      rec_set_p->remove (iter_node_p);
    }

    JERRY_ASSERT (rec_set_p->get_next (node_from_p) == node_to_p);

    size_t node_from_space = (size_t) (rcs_get_node_data_space (rec_set_p, node_from_p) \
                                       + node_data_space_size - rec_from_beg_p);
    size_t node_to_space = (size_t) (node_to_p != NULL ? \
                   (rec_to_beg_p - rcs_get_node_data_space (rec_set_p, node_to_p)) : 0);

    free_size = node_from_space + node_to_space;
  }

  rcs_init_free_record (rec_set_p, rec_from_p, prev_rec_p, free_size);

  if (rec_to_p != NULL)
  {
    rcs_record_set_prev (rec_set_p, rec_to_p, rec_from_p);
  }
  else if (prev_rec_p == NULL)
  {
    rec_set_p->remove (node_from_p);

    JERRY_ASSERT (node_to_p == NULL);
    JERRY_ASSERT (rec_set_p->get_first () == NULL);
  }

  rcs_assert_state_is_correct (rec_set_p);
} /* rcs_free_record */
