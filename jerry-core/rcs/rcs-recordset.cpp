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
#include "rcs-recordset.h"

/** \addtogroup recordset Recordset
 * @{
 *
 * Non-contiguous container abstraction with iterator.
 *
 * @{
 */

/**
 * Get the record's type identifier
 *
 * @return record type identifier
 */
rcs_record_t::type_t
rcs_recordset_t::record_t::get_type (void)
const
{
  JERRY_STATIC_ASSERT (sizeof (type_t) * JERRY_BITSINBYTE >= _type_field_width);

  return (type_t) get_field (_type_field_pos, _type_field_width);
} /* rcs_recordset_t::record_t::get_type */

/**
 * Set the record's type identifier
 */
void
rcs_recordset_t::record_t::set_type (rcs_record_t::type_t type) /**< record type identifier */
{
  set_field (_type_field_pos, _type_field_width, type);
} /* rcs_recordset_t::record_t::set_type */

/**
 * Compress pointer to extended compressed pointer
 *
 * @return dynamic storage-specific extended compressed pointer
 */
rcs_cpointer_t
rcs_recordset_t::record_t::cpointer_t::compress (rcs_record_t *pointer) /**< pointer to compress */
{
  rcs_cpointer_t cpointer;

  cpointer.packed_value = 0;

  uintptr_t base_pointer = JERRY_ALIGNDOWN ((uintptr_t) pointer, MEM_ALIGNMENT);

  if ((void *) base_pointer == NULL)
  {
    cpointer.value.base_cp = MEM_CP_NULL;
  }
  else
  {
    cpointer.value.base_cp = mem_compress_pointer ((void *) base_pointer) & MEM_CP_MASK;
  }

#if MEM_ALIGNMENT_LOG > RCS_DYN_STORAGE_LENGTH_UNIT_LOG
  /*
   * If alignment of a unit in recordset storage is less than required by MEM_ALIGNMENT_LOG,
   * then mem_cpointer_t can't store pointer to the unit, and so, rcs_cpointer_t stores
   * mem_cpointer_t to block, aligned to MEM_ALIGNMENT, and also extension with difference
   * between positions of the MEM_ALIGNMENT-aligned block and the unit.
   */
  uintptr_t diff = (uintptr_t) pointer - base_pointer;

  JERRY_ASSERT (diff < MEM_ALIGNMENT);
  JERRY_ASSERT (jrt_extract_bit_field (diff, 0, RCS_DYN_STORAGE_LENGTH_UNIT_LOG) == 0);

  uintptr_t ext_part = (uintptr_t) jrt_extract_bit_field (diff,
                                                          RCS_DYN_STORAGE_LENGTH_UNIT_LOG,
                                                          MEM_ALIGNMENT_LOG - RCS_DYN_STORAGE_LENGTH_UNIT_LOG);

  cpointer.value.ext = ext_part & ((1ull << (MEM_ALIGNMENT_LOG - RCS_DYN_STORAGE_LENGTH_UNIT_LOG)) - 1);
#endif /* MEM_ALIGNMENT > RCS_DYN_STORAGE_LENGTH_UNIT_LOG */

  JERRY_ASSERT (decompress (cpointer) == pointer);

  return cpointer;
} /* rcs_recordset_t::record_t::cpointer_t::compress */

/**
 * Decompress extended compressed pointer
 *
 * @return decompressed pointer
 */
rcs_record_t*
rcs_recordset_t::record_t::cpointer_t::decompress (rcs_cpointer_t compressed_pointer) /**< recordset-specific
                                                                                       *   compressed pointer */
{
  uint8_t* base_pointer;
  if (compressed_pointer.value.base_cp == MEM_CP_NULL)
  {
    base_pointer = NULL;
  }
  else
  {
    base_pointer = (uint8_t*) mem_decompress_pointer (compressed_pointer.value.base_cp);
  }

  uintptr_t diff = 0;

#if MEM_ALIGNMENT_LOG > RCS_DYN_STORAGE_LENGTH_UNIT_LOG
  /*
   * See also:
   *          rcs_recordset_t::record_t::cpointer_t::compress
   */

  diff = (uintptr_t) compressed_pointer.value.ext << RCS_DYN_STORAGE_LENGTH_UNIT_LOG;
#endif /* MEM_ALIGNMENT_LOG > RCS_DYN_STORAGE_LENGTH_UNIT_LOG */

  rcs_record_t *rec_p = (rcs_record_t *) (base_pointer + diff);

  return rec_p;
} /* rcs_recordset_t::record_t::cpointer_t::decompress */

/**
 * Create NULL compressed pointer
 *
 * @return NULL compressed pointer
 */
rcs_cpointer_t
rcs_recordset_t::record_t::cpointer_t::null_cp ()
{
  rcs_cpointer_t cp;
  cp.packed_value = MEM_CP_NULL;
  return cp;
} /* rcs_recordset_t::record_t::cpointer_t::null_cp */

/**
 * Assert that 'this' value points to correct record
 */
void
rcs_recordset_t::record_t::check_this (void)
const
{
  JERRY_ASSERT (this != NULL);

  uintptr_t ptr = (uintptr_t) this;

  JERRY_ASSERT (JERRY_ALIGNUP (ptr, RCS_DYN_STORAGE_LENGTH_UNIT) == ptr);
} /* rcs_recordset_t::record_t::check_this */

/**
 * Get value of the record's field with specified offset and width
 *
 * @return field's 32-bit unsigned integer value
 */
uint32_t
rcs_recordset_t::record_t::get_field (uint32_t field_pos, /**< offset, in bits */
                                      uint32_t field_width) /**< width, in bits */
const
{
  check_this ();

  JERRY_ASSERT (sizeof (uint32_t) <= RCS_DYN_STORAGE_LENGTH_UNIT);
  JERRY_ASSERT (field_pos + field_width <= RCS_DYN_STORAGE_LENGTH_UNIT * JERRY_BITSINBYTE);

  uint32_t value = *reinterpret_cast<const uint32_t*> (this);
  return (uint32_t) jrt_extract_bit_field (value, field_pos, field_width);
} /* rcs_recordset_t::record_t::get_field */

/**
 * Set value of the record's field with specified offset and width
 */
void
rcs_recordset_t::record_t::set_field (uint32_t field_pos, /**< offset, in bits */
                                      uint32_t field_width, /**< width, in bits */
                                      size_t value) /**< 32-bit unsigned integer value */
{
  check_this ();

  JERRY_ASSERT (sizeof (uint32_t) <= RCS_DYN_STORAGE_LENGTH_UNIT);
  JERRY_ASSERT (field_pos + field_width <= RCS_DYN_STORAGE_LENGTH_UNIT * JERRY_BITSINBYTE);

  uint32_t prev_value = *reinterpret_cast<uint32_t*> (this);
  *reinterpret_cast<uint32_t*> (this) = (uint32_t) jrt_set_bit_field_value (prev_value,
                                                                            value,
                                                                            field_pos,
                                                                            field_width);
} /* rcs_recordset_t::record_t::set_field */

/**
 * Get value of the record's pointer field with specified offset and width
 *
 * @return pointer to record
 */
rcs_record_t*
rcs_recordset_t::record_t::get_pointer (uint32_t field_pos, /**< offset, in bits */
                                        uint32_t field_width) /**< width, in bits */
const
{
  cpointer_t cpointer;

  uint16_t value = (uint16_t) get_field (field_pos, field_width);

  JERRY_ASSERT (sizeof (cpointer) == sizeof (cpointer.value));
  JERRY_ASSERT (sizeof (value) == sizeof (cpointer.value));

  cpointer.packed_value = value;

  return cpointer_t::decompress (cpointer);
} /* rcs_recordset_t::record_t::get_pointer */

/**
 * Set value of the record's pointer field with specified offset and width
 */
void
rcs_recordset_t::record_t::set_pointer (uint32_t field_pos, /**< offset, in bits */
                                        uint32_t field_width, /**< width, in bits */
                                        rcs_record_t* pointer_p) /**< pointer to a record */
{
  cpointer_t cpointer = cpointer_t::compress (pointer_p);

  set_field (field_pos, field_width, cpointer.packed_value);
} /* rcs_recordset_t::record_t::set_pointer */

/**
 * Initialize record in specified place, and, if there is free space
 * before next record, initialize free record for the space
 */
void
rcs_recordset_t::alloc_record_in_place (rcs_record_t* place_p, /**< where to initialize record */
                                        size_t free_size, /**< size of free part between allocated record
                                                           *   and next allocated record */
                                        rcs_record_t* next_record_p) /**< next allocated record */
{
  const size_t node_data_space_size = get_node_data_space_size ();

  if (next_record_p != NULL)
  {
    if (free_size == 0)
    {
      set_prev (next_record_p, place_p);
    }
    else
    {
      rcs_chunked_list_t::node_t* node_p = _chunk_list.get_node_from_pointer (next_record_p);
      uint8_t* node_data_space_p = get_node_data_space (node_p);

      JERRY_ASSERT ((uint8_t*) next_record_p < node_data_space_p + node_data_space_size);

      rcs_record_t* free_rec_p;

      if ((uint8_t*) next_record_p >= node_data_space_p + free_size)
      {
        free_rec_p = (rcs_record_t*) ((uint8_t*) next_record_p - free_size);
      }
      else
      {
        size_t size_passed_back = (size_t) ((uint8_t*) next_record_p - node_data_space_p);
        JERRY_ASSERT (size_passed_back < free_size && size_passed_back + node_data_space_size > free_size);

        node_p = _chunk_list.get_prev (node_p);
        node_data_space_p = get_node_data_space (node_p);

        free_rec_p = (rcs_record_t*) (node_data_space_p + node_data_space_size - \
                                      (free_size - size_passed_back));
      }

      init_free_record (free_rec_p, free_size, place_p);
    }
  }
  else if (free_size != 0)
  {
    rcs_chunked_list_t::node_t* node_p = _chunk_list.get_node_from_pointer (place_p);
    JERRY_ASSERT (node_p != NULL);

    rcs_chunked_list_t::node_t* next_node_p = _chunk_list.get_next (node_p);

    while (next_node_p != NULL)
    {
      node_p = next_node_p;

      next_node_p = _chunk_list.get_next (node_p);
    }

    uint8_t* node_data_space_p = get_node_data_space (node_p);
    const size_t node_data_space_size = get_node_data_space_size ();

    rcs_record_t* free_rec_p = (rcs_record_t*) (node_data_space_p + node_data_space_size \
                                                - free_size);
    init_free_record (free_rec_p, free_size, place_p);
  }
} /* rcs_recordset_t::alloc_record_in_place */

/**
 * Initialize specified record as free record
 */
void
rcs_recordset_t::init_free_record (rcs_record_t *rec_p, /**< record to init as free record */
                                   size_t size, /**< size, including header */
                                   rcs_record_t *prev_rec_p) /**< previous record (or NULL) */
{
  rcs_free_record_t *free_rec_p = static_cast<rcs_free_record_t*> (rec_p);

  free_rec_p->set_type (_free_record_type_id);
  free_rec_p->set_size (size);
  free_rec_p->set_prev (prev_rec_p);
} /* rcs_recordset_t::init_free_record */

/**
 * Check if the record is free record
 */
bool
rcs_recordset_t::is_record_free (rcs_record_t *record_p) /**< a record */
{
  JERRY_ASSERT (record_p != NULL);

  return (record_p->get_type () == _free_record_type_id);
} /* rcs_recordset_t::is_record_free */

/**
 * Get the node's data space
 *
 * @return pointer to beginning of the node's data space
 */
uint8_t *
rcs_recordset_t::get_node_data_space (rcs_chunked_list_t::node_t *node_p) /**< the node */
const
{
  uintptr_t unaligned_data_space_beg = (uintptr_t) _chunk_list.get_node_data_space (node_p);
  uintptr_t aligned_data_space_beg = JERRY_ALIGNUP (unaligned_data_space_beg, RCS_DYN_STORAGE_LENGTH_UNIT);

  JERRY_ASSERT (unaligned_data_space_beg + rcs_chunked_list_t::get_node_data_space_size ()
                == aligned_data_space_beg + rcs_recordset_t::get_node_data_space_size ());

  return (uint8_t *) aligned_data_space_beg;
} /* rcs_recordset_t::get_node_data_space */

/**
 * Get size of a node's data space
 *
 * @return size
 */
size_t
rcs_recordset_t::get_node_data_space_size (void)
{
  return JERRY_ALIGNDOWN (rcs_chunked_list_t::get_node_data_space_size (), RCS_DYN_STORAGE_LENGTH_UNIT);
} /* rcs_recordset_t::get_node_data_space_size */

/**
 * Allocate record of specified size
 *
 * @return record identifier
 */
rcs_record_t*
rcs_recordset_t::alloc_space_for_record (size_t bytes, /**< size */
                                         rcs_record_t** out_prev_rec_p) /**< out: pointer to record, previous
                                                                         *   to the allocated, or NULL if the allocated
                                                                         *   record is the first */
{
  assert_state_is_correct ();

  JERRY_ASSERT (JERRY_ALIGNUP (bytes, RCS_DYN_STORAGE_LENGTH_UNIT) == bytes);
  JERRY_ASSERT (out_prev_rec_p != NULL);

  const size_t node_data_space_size = get_node_data_space_size ();

  *out_prev_rec_p = NULL;

  for (rcs_record_t *rec_p = get_first ();
       rec_p != NULL;
       *out_prev_rec_p = rec_p, rec_p = get_next (rec_p))
  {
    if (is_record_free (rec_p))
    {
      size_t record_size = get_record_size (rec_p);

      rcs_record_t* next_rec_p = get_next (rec_p);

      if (record_size >= bytes)
      {
        /* record size is sufficient */
        alloc_record_in_place (rec_p, record_size - bytes, next_rec_p);

        return rec_p;
      }
      else
      {
        rcs_chunked_list_t::node_t* node_p = _chunk_list.get_node_from_pointer (rec_p);
        uint8_t* node_data_space_p = get_node_data_space (node_p);
        uint8_t* node_data_space_end_p = node_data_space_p + node_data_space_size;

        uint8_t* rec_space_p = (uint8_t*) rec_p;

        if (rec_space_p + record_size >= node_data_space_end_p)
        {
          /* record lies up to end of node's data space size,
           * and, so, can be extended up to necessary size */

          while (record_size < bytes)
          {
            node_p = _chunk_list.insert_new (node_p);

            record_size += node_data_space_size;
          }

          alloc_record_in_place (rec_p, record_size - bytes, next_rec_p);

          return rec_p;
        }
      }

      if (next_rec_p == NULL)
      {
        /* in the case, there are no more records in the storage,
         * so, we should append new record */
        break;
      }
      else
      {
        JERRY_ASSERT (!is_record_free (rec_p));
      }
    }
  }

  /* free record of sufficient size was not found */

  rcs_chunked_list_t::node_t *node_p = _chunk_list.append_new ();
  rcs_record_t* new_rec_p = (rcs_record_t*) get_node_data_space (node_p);

  size_t allocated_size = node_data_space_size;

  while (allocated_size < bytes)
  {
    allocated_size += node_data_space_size;
    _chunk_list.append_new ();
  }

  alloc_record_in_place (new_rec_p, allocated_size - bytes, NULL);

  return new_rec_p;
} /* rcs_recordset_t::alloc_space_for_record */

/**
 * Free specified record
 */
void
rcs_recordset_t::free_record (rcs_record_t* record_p) /**< record to free */
{
  JERRY_ASSERT (record_p != NULL);

  assert_state_is_correct ();

  rcs_record_t *prev_rec_p = get_prev (record_p);

  // make record free
  init_free_record (record_p, get_record_size (record_p), prev_rec_p);

  // merge adjacent free records, if there are any,
  // and free nodes of chunked list that became unused
  rcs_record_t *rec_from_p = record_p, *rec_to_p = get_next (record_p);

  if (prev_rec_p != NULL
      && is_record_free (prev_rec_p))
  {
    rec_from_p = prev_rec_p;

    prev_rec_p = get_prev (rec_from_p);
  }

  if (rec_to_p != NULL
      && is_record_free (rec_to_p))
  {
    rec_to_p = get_next (rec_to_p);
  }

  JERRY_ASSERT (rec_from_p != NULL && is_record_free (rec_from_p));
  JERRY_ASSERT (rec_to_p == NULL || !is_record_free (rec_to_p));

  rcs_chunked_list_t::node_t *node_from_p = _chunk_list.get_node_from_pointer (rec_from_p);
  rcs_chunked_list_t::node_t *node_to_p;

  if (rec_to_p == NULL)
  {
    node_to_p = NULL;
  }
  else
  {
    node_to_p = _chunk_list.get_node_from_pointer (rec_to_p);
  }

  const size_t node_data_space_size = get_node_data_space_size ();

  uint8_t* rec_from_beg_p = (uint8_t*) rec_from_p;
  uint8_t* rec_to_beg_p = (uint8_t*) rec_to_p;
  size_t free_size;

  if (node_from_p == node_to_p)
  {
    JERRY_ASSERT (rec_from_beg_p + get_record_size (rec_from_p) <= rec_to_beg_p);

    free_size = (size_t) (rec_to_beg_p - rec_from_beg_p);
  }
  else
  {
    for (rcs_chunked_list_t::node_t *iter_node_p = _chunk_list.get_next (node_from_p), *iter_next_node_p;
         iter_node_p != node_to_p;
         iter_node_p = iter_next_node_p)
    {
      iter_next_node_p = _chunk_list.get_next (iter_node_p);

      _chunk_list.remove (iter_node_p);
    }

    JERRY_ASSERT (_chunk_list.get_next (node_from_p) == node_to_p);

    size_t node_from_space = (size_t) (get_node_data_space (node_from_p) +
                                       node_data_space_size - rec_from_beg_p);
    size_t node_to_space = (size_t) (node_to_p != NULL
                                     ? rec_to_beg_p - get_node_data_space (node_to_p)
                                     : 0);

    free_size = node_from_space + node_to_space;
  }

  init_free_record (rec_from_p, free_size, prev_rec_p);

  if (rec_to_p != NULL)
  {
    set_prev (rec_to_p, rec_from_p);
  }
  else if (prev_rec_p == NULL)
  {
    JERRY_ASSERT (node_to_p == NULL);

    _chunk_list.remove (node_from_p);

    JERRY_ASSERT (_chunk_list.get_first () == NULL);
  }

  assert_state_is_correct ();
} /* rcs_recordset_t::free_record */

/**
 * Get first record
 *
 * @return pointer of the first record of the recordset
 */
rcs_record_t*
rcs_recordset_t::get_first (void)
{
  rcs_chunked_list_t::node_t *first_node_p = _chunk_list.get_first ();

  if (first_node_p == NULL)
  {
    return NULL;
  }
  else
  {
    return (rcs_record_t*) get_node_data_space (first_node_p);
  }
} /* rcs_recordset_t::get_first */

/**
 * Get record, previous to the specified
 *
 * @return pointer to the previous record
 */
rcs_record_t*
rcs_recordset_t::get_prev (rcs_record_t* rec_p) /**< record */
{
  JERRY_ASSERT (rec_p->get_type () == _free_record_type_id);

  rcs_free_record_t *free_rec_p = static_cast<rcs_free_record_t*> (rec_p);

  return free_rec_p->get_prev ();
} /* rcs_recordset_t::get_prev */

/**
 * Get record, next to the specified
 *
 * @return pointer to the next record
 */
rcs_record_t*
rcs_recordset_t::get_next (rcs_record_t* rec_p) /**< record */
{
  rcs_chunked_list_t::node_t* node_p = _chunk_list.get_node_from_pointer (rec_p);

  const uint8_t* data_space_begin_p = get_node_data_space (node_p);
  const size_t data_space_size = get_node_data_space_size ();

  const uint8_t* record_start_p = (const uint8_t*) rec_p;
  size_t record_size = get_record_size (rec_p);

  size_t record_offset_in_node = (size_t) (record_start_p - data_space_begin_p);
  size_t node_size_left = data_space_size - record_offset_in_node;

  if (node_size_left > record_size)
  {
    return (rcs_record_t*) (record_start_p + record_size);
  }
  else
  {
    node_p = _chunk_list.get_next (node_p);
    JERRY_ASSERT (node_p != NULL || record_size == node_size_left);

    size_t record_size_left = record_size - node_size_left;

    while (record_size_left >= data_space_size)
    {
      JERRY_ASSERT (node_p != NULL);
      node_p = _chunk_list.get_next (node_p);

      record_size_left -= data_space_size;
    }

    if (node_p == NULL)
    {
      JERRY_ASSERT (record_size_left == 0);

      return NULL;
    }
    else
    {
      return (rcs_record_t*) (get_node_data_space (node_p) + record_size_left);
    }
  }
} /* rcs_recordset_t::get_next */

/**
 * Set previous record
 */
void
rcs_recordset_t::set_prev (rcs_record_t* rec_p, /**< record to set previous for */
                           rcs_record_t *prev_rec_p) /**< previous record */
{
  JERRY_ASSERT (rec_p->get_type () == _free_record_type_id);

  rcs_free_record_t *free_rec_p = static_cast<rcs_free_record_t*> (rec_p);

  free_rec_p->set_prev (prev_rec_p);
} /* rcs_recordset_t::set_prev */

/**
 * Get size of the record
 */
size_t
rcs_recordset_t::get_record_size (rcs_record_t* rec_p) /**< record */
{
  JERRY_ASSERT (rec_p->get_type () == _free_record_type_id);

  rcs_free_record_t *free_rec_p = static_cast<rcs_free_record_t*> (rec_p);

  return free_rec_p->get_size ();
} /* rcs_recordset_t::get_record_size */

/**
 * Assert that recordset state is correct
 */
void
rcs_recordset_t::assert_state_is_correct (void)
{
#ifndef JERRY_DISABLE_HEAVY_DEBUG
  size_t node_size_sum = 0;
  size_t record_size_sum = 0;

  rcs_record_t* last_record_p = NULL;

  for (rcs_record_t* rec_p = get_first (), *next_rec_p;
       rec_p != NULL;
       last_record_p = rec_p, rec_p = next_rec_p)
  {
    JERRY_ASSERT (get_record_size (rec_p) > 0);
    record_size_sum += get_record_size (rec_p);

    rcs_chunked_list_t::node_t *node_p = _chunk_list.get_node_from_pointer (rec_p);

    next_rec_p = get_next (rec_p);

    rcs_chunked_list_t::node_t *next_node_p;
    if (next_rec_p == NULL)
    {
      next_node_p = NULL;
    }
    else
    {
      next_node_p = _chunk_list.get_node_from_pointer (next_rec_p);
    }

    while (node_p != next_node_p)
    {
      node_p = _chunk_list.get_next (node_p);
      node_size_sum += get_node_data_space_size ();
    }
  }

  JERRY_ASSERT (node_size_sum == record_size_sum);

  record_size_sum = 0;
  for (rcs_record_t* rec_p = last_record_p;
       rec_p != NULL;
       rec_p = get_prev (rec_p))
  {
    record_size_sum += get_record_size (rec_p);
  }

  JERRY_ASSERT (node_size_sum == record_size_sum);
#endif /* !JERRY_DISABLE_HEAVY_DEBUG */
} /* rcs_recordset_t::assert_state_is_correct */

/**
 * Perform general access to the record
 *
 * Warning: This function is implemented in assumption that `size` is not more than `2 * node_data_space_size`.
 */
void
rcs_record_iterator_t::access (access_t access_type, /**< type of access: read, write or skip */
                               void *data, /**< in/out data to read or write */
                               size_t size) /**< size of the data in bytes */
{
  const size_t node_data_space_size = _recordset_p->get_node_data_space_size ();
  JERRY_ASSERT (2 * node_data_space_size >= size);
  const size_t record_size = _recordset_p->get_record_size (_record_start_p);

  JERRY_ASSERT (!finished ());

  rcs_chunked_list_t::node_t *current_node_p = _recordset_p->_chunk_list.get_node_from_pointer (_current_pos_p);
  uint8_t *current_node_data_space_p = _recordset_p->get_node_data_space (current_node_p);
  size_t left_in_node = node_data_space_size - (size_t)(_current_pos_p - current_node_data_space_p);

  JERRY_ASSERT (_current_offset + size <= record_size);

  /*
   * Read the data and increase the current position pointer.
   */
  if (left_in_node >= size)
  {
    /* all data is placed inside single node */
    if (access_type == ACCESS_READ)
    {
      memcpy (data, _current_pos_p, size);
    }
    else if (access_type == ACCESS_WRITE)
    {
      memcpy (_current_pos_p, data, size);
    }
    else
    {
      JERRY_ASSERT (access_type == ACCESS_SKIP);

      if (left_in_node > size)
      {
        _current_pos_p += size;
      }
      else if (_current_offset + size < record_size)
      {
        current_node_p = _recordset_p->_chunk_list.get_next (current_node_p);
        JERRY_ASSERT (current_node_p);
        _current_pos_p = _recordset_p->get_node_data_space (current_node_p);
      }
      else
      {
        JERRY_ASSERT (_current_offset + size == record_size);
      }
    }
  }
  else
  {
    /* data is distributed between two nodes */
    size_t first_chunk_size = node_data_space_size - (size_t) (_current_pos_p - current_node_data_space_p);

    if (access_type == ACCESS_READ)
    {
      memcpy (data, _current_pos_p, first_chunk_size);
    }
    else if (access_type == ACCESS_WRITE)
    {
      memcpy (_current_pos_p, data, first_chunk_size);
    }

    rcs_chunked_list_t::node_t *next_node_p = _recordset_p->_chunk_list.get_next (current_node_p);
    JERRY_ASSERT (next_node_p != NULL);
    uint8_t *next_node_data_space_p = _recordset_p->get_node_data_space (next_node_p);

    if (access_type == ACCESS_READ)
    {
      memcpy ((uint8_t *)data + first_chunk_size, next_node_data_space_p, size - first_chunk_size);
    }
    else if (access_type == ACCESS_WRITE)
    {
      memcpy (next_node_data_space_p, (uint8_t *)data + first_chunk_size, size - first_chunk_size);
    }
    else
    {
      JERRY_ASSERT (access_type == ACCESS_SKIP);

      _current_pos_p = next_node_data_space_p + size - first_chunk_size;
    }
  }

  /* check if we reached the end */
  if (access_type == ACCESS_SKIP)
  {
    _current_offset += size;
    JERRY_ASSERT (_current_offset <= record_size);

    if (_current_offset == record_size)
    {
      _current_pos_p = NULL;
      _current_offset = 0;
    }
  }
} /* rcs_record_iterator_t::access */

/**
 * Get size of the free record
 */
size_t
rcs_free_record_t::get_size (void)
const
{
  return get_field (_length_field_pos, _length_field_width) * RCS_DYN_STORAGE_LENGTH_UNIT;
} /* rcs_free_record_t::get_size */

/**
 * Set size of the free record
 */
void
rcs_free_record_t::set_size (size_t size) /**< size to set */
{
  JERRY_ASSERT (JERRY_ALIGNUP (size, RCS_DYN_STORAGE_LENGTH_UNIT) == size);

  set_field (_length_field_pos, _length_field_width, size >> RCS_DYN_STORAGE_LENGTH_UNIT_LOG);
} /* rcs_free_record_t::set_size */

/**
 * Get previous record for the free record
 */
rcs_record_t*
rcs_free_record_t::get_prev (void)
const
{
  return get_pointer (_prev_field_pos, _prev_field_width);
} /* rcs_free_record_t::get_prev */

/**
 * Set previous record for the free record
 */
void
rcs_free_record_t::set_prev (rcs_record_t* prev_rec_p) /**< previous record to set */
{
  set_pointer (_prev_field_pos, _prev_field_width, prev_rec_p);
} /* rcs_free_record_t::set_prev */

/**
 * @}
 */
