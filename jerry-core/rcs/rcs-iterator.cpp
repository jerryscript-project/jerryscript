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

#include "rcs-iterator.h"
#include "rcs-allocator.h"
#include "rcs-records.h"

/**
 * Represents the memory access on the literal storage.
 */
typedef enum
{
  RCS_ITERATOR_ACCESS_WRITE = 0, /**< Write 'size' bytes from 'data' buffer to the record. */
  RCS_ITERATOR_ACCESS_READ = 1, /**< Read 'size' bytes from the record and write to the 'data' buffer. */
  RCS_ITERATOR_ACCESS_SKIP = 2 /**< Increment current position so that 'size' bytes would be skipped. */
} rcs_access_t;

/**
 * Create an iterator context.
 *
 * @return an initialized iterator context
 */
rcs_iterator_t
rcs_iterator_create (rcs_record_set_t *recordset_p, /**< recordset */
                     rcs_record_t *record_p) /**< start record */
{
  rcs_iterator_t ctx;
  {
    ctx.recordset_p = recordset_p;
    ctx.record_start_p = record_p;

    rcs_iterator_reset (&ctx);
  }

  return ctx;
} /* rcs_iterator_create */

/**
 * Perform general access to the record
 *
 * Warning: This function is implemented in assumption that `size` is not more than `2 * node_data_space_size`.
 */
static void
rcs_iterator_access (rcs_iterator_t *ctx_p, /**< iterator context */
                     void *data, /**< iterator context */
                     size_t size, /**< iterator context */
                     rcs_access_t access_type) /**< access type */
{
  const size_t node_data_space_size = rcs_get_node_data_space_size ();
  JERRY_ASSERT (2 * node_data_space_size >= size);
  const size_t record_size = rcs_record_get_size (ctx_p->record_start_p);

  JERRY_ASSERT (!rcs_iterator_finished (ctx_p));

  rcs_chunked_list_t::node_t *current_node_p = ctx_p->recordset_p->get_node_from_pointer (ctx_p->current_pos_p);
  uint8_t *current_node_data_space_p = rcs_get_node_data_space (ctx_p->recordset_p, current_node_p);
  size_t left_in_node = node_data_space_size - (size_t) (ctx_p->current_pos_p - current_node_data_space_p);

  JERRY_ASSERT (ctx_p->current_offset + size <= record_size);

  /*
   * Read the data and increase the current position pointer.
   */
  if (left_in_node >= size)
  {
    /* all data is placed inside single node */
    if (access_type == RCS_ITERATOR_ACCESS_READ)
    {
      memcpy (data, ctx_p->current_pos_p, size);
    }
    else if (access_type == RCS_ITERATOR_ACCESS_WRITE)
    {
      memcpy (ctx_p->current_pos_p, data, size);
    }
    else
    {
      JERRY_ASSERT (access_type == RCS_ITERATOR_ACCESS_SKIP);

      if (left_in_node > size)
      {
        ctx_p->current_pos_p += size;
      }
      else if (ctx_p->current_offset + size < record_size)
      {
        current_node_p = ctx_p->recordset_p->get_next (current_node_p);
        JERRY_ASSERT (current_node_p);
        ctx_p->current_pos_p = rcs_get_node_data_space (ctx_p->recordset_p, current_node_p);
      }
      else
      {
        JERRY_ASSERT (ctx_p->current_offset + size == record_size);
      }
    }
  }
  else
  {
    /* Data is distributed between two nodes. */
    const size_t first_chunk_size = node_data_space_size - (size_t) (ctx_p->current_pos_p - current_node_data_space_p);

    if (access_type == RCS_ITERATOR_ACCESS_READ)
    {
      memcpy (data, ctx_p->current_pos_p, first_chunk_size);
    }
    else if (access_type == RCS_ITERATOR_ACCESS_WRITE)
    {
      memcpy (ctx_p->current_pos_p, data, first_chunk_size);
    }

    rcs_chunked_list_t::node_t *next_node_p = ctx_p->recordset_p->get_next (current_node_p);
    JERRY_ASSERT (next_node_p != NULL);
    uint8_t *next_node_data_space_p = rcs_get_node_data_space (ctx_p->recordset_p, next_node_p);

    if (access_type == RCS_ITERATOR_ACCESS_READ)
    {
      memcpy ((uint8_t *)data + first_chunk_size, next_node_data_space_p, size - first_chunk_size);
    }
    else if (access_type == RCS_ITERATOR_ACCESS_WRITE)
    {
      memcpy (next_node_data_space_p, (uint8_t *)data + first_chunk_size, size - first_chunk_size);
    }
    else
    {
      JERRY_ASSERT (access_type == RCS_ITERATOR_ACCESS_SKIP);
      ctx_p->current_pos_p = next_node_data_space_p + size - first_chunk_size;
    }
  }

  /* Check if we reached the end. */
  if (access_type == RCS_ITERATOR_ACCESS_SKIP)
  {
    ctx_p->current_offset += size;
    JERRY_ASSERT (ctx_p->current_offset <= record_size);

    if (ctx_p->current_offset == record_size)
    {
      ctx_p->current_pos_p = NULL;
      ctx_p->current_offset = 0;
    }
  }
} /* rcs_iterator_access */

/**
 * Read a value from the record.
 * After reading iterator doesn't change its position.
 *
 * @return read value
 */
void
rcs_iterator_read (rcs_iterator_t *ctx_p, /**< iterator context */
                   void *out_data, /**< value to read */
                   size_t size) /**< size to read */
{
  rcs_iterator_access (ctx_p, out_data, size, RCS_ITERATOR_ACCESS_READ);
} /* rcs_iterator_read */

/**
 * Write a value to the record.
 * After writing, iterator doesn't change its position.
 */
void
rcs_iterator_write (rcs_iterator_t *ctx_p, /**< iterator context */
                    void *value, /**< value to write */
                    size_t size) /**< size to write */
{
  rcs_iterator_access (ctx_p, value, size, RCS_ITERATOR_ACCESS_WRITE);
} /* rcs_iterator_write */

/**
 * Increment current position to skip 'size' bytes.
 */
void
rcs_iterator_skip (rcs_iterator_t *ctx_p, /**< iterator context */
                   size_t size) /**< size to skip */
{
  if (size)
  {
    rcs_iterator_access (ctx_p, NULL, size, RCS_ITERATOR_ACCESS_SKIP);
  }
} /* rcs_iterator_skip */

/**
 * Reset the iterator, so that it points to the beginning of the record.
 */
void
rcs_iterator_reset (rcs_iterator_t *ctx_p) /**< iterator context */
{
  ctx_p->current_pos_p = ctx_p->record_start_p;
  ctx_p->current_offset = 0;
} /* rcs_iterator_reset */

/**
 * Check if the end of the record was reached.
 *
 * @return true if the whole record was iterated
 *         false otherwise
 */
bool
rcs_iterator_finished (rcs_iterator_t *ctx_p) /**< iterator context */
{
  return ctx_p->current_pos_p == NULL;
} /* rcs_iterator_finished */
