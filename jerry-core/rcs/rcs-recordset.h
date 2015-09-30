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

#ifndef RCS_RECORDSET_H
#define RCS_RECORDSET_H

#include <string.h>

#include "jrt.h"
#include "jrt-bit-fields.h"
#include "mem-allocator.h"
#include "rcs-chunked-list.h"

/** \addtogroup recordset Recordset
 * @{
 *
 * Non-contiguous container abstraction with iterator.
 *
 * @{
 */

/**
 * Logarithm of a dynamic storage unit alignment
 */
#define RCS_DYN_STORAGE_LENGTH_UNIT_LOG (2u)

/**
 * Unit of length
 */
#define RCS_DYN_STORAGE_LENGTH_UNIT   ((size_t) (1ull << RCS_DYN_STORAGE_LENGTH_UNIT_LOG))

/**
 * Dynamic storage
 *
 * Note:
 *      Static C++ constructors / desctructors are supposed to be not supported.
 *      So, initialization / destruction is implemented through init / finalize
 *      static functions.
 */
class rcs_recordset_t
{
public:
  /* Constructor */
  void init (void)
  {
    _chunk_list.init ();

    JERRY_ASSERT (get_node_data_space_size () % RCS_DYN_STORAGE_LENGTH_UNIT == 0);
  } /* init */

  /* Destructor */
  void finalize (void)
  {
    _chunk_list.free ();
  } /* finalize */

  /* Free memory occupied by the dynamic storage */
  void cleanup (void)
  {
    _chunk_list.cleanup ();
  } /* cleanup */

  /**
   * Record type
   */
  class record_t
  {
  public:
    typedef uint8_t type_t;

    type_t get_type (void) const;
    void set_type (type_t);

    /**
     * Dynamic storage-specific extended compressed pointer
     *
     * Note:
     *      the pointer can represent addresses aligned by RCS_DYN_STORAGE_LENGTH_UNIT,
     *      while mem_cpointer_t can only represent addresses aligned by MEM_ALIGNMENT.
     */
    struct cpointer_t
    {
      static const uint32_t bit_field_width = MEM_CP_WIDTH + MEM_ALIGNMENT_LOG - RCS_DYN_STORAGE_LENGTH_UNIT_LOG;

      union
      {
        struct
        {
          mem_cpointer_t base_cp : MEM_CP_WIDTH; /**< pointer to base of addressed area */
#if MEM_ALIGNMENT_LOG > RCS_DYN_STORAGE_LENGTH_UNIT_LOG
          uint16_t ext : (MEM_ALIGNMENT_LOG - RCS_DYN_STORAGE_LENGTH_UNIT_LOG); /**< extension of the basic
                                                                                 *   compressed pointer
                                                                                 *   used for more detailed
                                                                                 *   addressing */
#endif /* MEM_ALIGNMENT_LOG > RCS_DYN_STORAGE_LENGTH_UNIT_LOG */
        } value;
        uint16_t packed_value;
      };

      static cpointer_t compress (record_t *);
      static record_t *decompress (cpointer_t);

      static cpointer_t null_cp ();

      static const int conval = 3;
    };

  private:
    /**
     * Offset of 'type' field, in bits
     */
    static constexpr uint32_t _type_field_pos = 0u;

    /**
     * Width of 'type' field, in bits
     */
    static constexpr uint32_t _type_field_width = 4u;

  protected:
    void check_this (void) const;

    uint32_t get_field (uint32_t, uint32_t) const;
    void set_field (uint32_t, uint32_t, size_t);

    record_t *get_pointer (uint32_t, uint32_t) const;
    void set_pointer (uint32_t, uint32_t, record_t *);

    /**
     * Offset of a derived record's fields, in bits
     */
    static constexpr uint32_t _fields_offset_begin = _type_field_pos + _type_field_width;
  };

  record_t *get_first (void);
  record_t *get_next (record_t *);

private:
  friend class rcs_record_iterator_t;

  /**
   * Type identifier for free record
   */
  static const record_t::type_t _free_record_type_id = 0;

  /**
   * Chunked list used for memory allocation
   */
  rcs_chunked_list_t _chunk_list;

  void alloc_record_in_place (record_t *, size_t, record_t *);

  void init_free_record (record_t *, size_t, record_t *);
  bool is_record_free (record_t *);

  uint8_t *get_node_data_space (rcs_chunked_list_t::node_t *) const;
  static size_t get_node_data_space_size (void);
protected:
  /**
   * First type identifier that can be used for storage-specific record types
   */
  static const record_t::type_t _first_type_id = _free_record_type_id + 1;

  /**
   * Allocate new record of specified type
   *
   * @return pointer to the new record
   */
  template<
    typename T, /**< type of record structure */
    typename ... SizeArgs> /**< type of arguments of T::size */
  T *alloc_record (record_t::type_t type, /**< record's type identifier */
                   SizeArgs ... size_args) /**< arguments of T::size */
  {
    JERRY_ASSERT (type >= _first_type_id);

    size_t size = T::size (size_args...);

    record_t *prev_rec_p;
    T *rec_p = static_cast<T*> (alloc_space_for_record (size, &prev_rec_p));

    rec_p->set_type (type);
    rec_p->set_size (size);
    rec_p->set_prev (prev_rec_p);

    assert_state_is_correct ();

    return rec_p;
  } /* alloc_record */

  record_t *alloc_space_for_record (size_t, record_t **);
  void free_record (record_t *);

  virtual record_t *get_prev (record_t *);

  virtual void set_prev (record_t *, record_t *);

  virtual size_t get_record_size (record_t *);

  void assert_state_is_correct (void);
}; /* rcs_recordset_t */

/**
 * Record type
 */
typedef rcs_recordset_t::record_t rcs_record_t;

/**
 * Recordset-specific compressed pointer type
 */
typedef rcs_record_t::cpointer_t rcs_cpointer_t;

/**
 * Record iterator
 */
class rcs_record_iterator_t
{
public:
  /**
   * Constructor
   */
  rcs_record_iterator_t (rcs_recordset_t *rcs_p, /**< recordset */
                         rcs_record_t *rec_p) /**< record which should belong to the recordset */
  {
    _record_start_p = rec_p;
    _recordset_p = rcs_p;

    reset ();
  } /* rcs_record_iterator_t */

  /**
   * Constructor
   */
  rcs_record_iterator_t (rcs_recordset_t *rcs_p, /**< recordset */
                         rcs_cpointer_t rec_ext_cp) /**< compressed pointer to the record */
  {
    _record_start_p = rcs_cpointer_t::decompress (rec_ext_cp);
    _recordset_p = rcs_p;

    reset ();
  } /* rcs_record_iterator_t */

protected:
  /**
   * Types of access
   */
  typedef enum
  {
    ACCESS_WRITE, /**< If access_type == ACCESS_WRITE,
                   * write 'size' bytes from 'data' buffer to the record. */
    ACCESS_READ,  /**< If access_type == ACCESS_READ,
                   * read 'size' bytes from the record and write to the 'data' buffer. */
    ACCESS_SKIP   /**< If access_type == ACCESS_SKIP,
                   * increment current position so that 'size' bytes would be skipped. */
  } access_t;

  void access (access_t, void *, size_t);

public:
  /**
   * Read value of type T from the record.
   * After reading iterator doesn't change its position.
   *
   * @return read value
   */
  template<typename T> T read (void)
  {
    T data;
    access (ACCESS_READ, &data, sizeof (T));
    return data;
  } /* read */

  /**
   * Write value of type T to the record.
   * After writing iterator doesn't change its position.
   */
  template<typename T> void write (T value) /**< value to write */
  {
    access (ACCESS_WRITE, &value, sizeof (T));
  } /* write */

  /**
   * Increment current position to skip T value in the record.
   */
  template<typename T> void skip ()
  {
    access (ACCESS_SKIP, NULL, sizeof (T));
  } /* skip */

  /**
   * Increment current position to skip 'size' bytes.
   */
  void skip (size_t size) /**< number of bytes to skip */
  {
    if (size)
    {
      access (ACCESS_SKIP, NULL, size);
    }
  } /* skip */

  /**
   * Check if the end of the record was reached.
   *
   * @return true if the whole record was iterated
   *         false otherwise
   */
  bool finished ()
  {
    return _current_pos_p == NULL;
  } /* finished */

  /**
   * Reset the iterator, so that it points to the beginning of the record
   */
  void reset ()
  {
    _current_pos_p = (uint8_t *)_record_start_p;
    _current_offset = 0;
  } /* reset */

private:
  rcs_record_t *_record_start_p; /**< start of current record */
  uint8_t *_current_pos_p; /**< pointer to current offset in current record */
  size_t _current_offset; /**< current offset */
  rcs_recordset_t *_recordset_p; /**< recordset containing the record */
}; /* rcs_record_iterator_t */

/**
 * Free record layout description
 */
class rcs_free_record_t : public rcs_record_t
{
public:
  size_t get_size (void) const;
  void set_size (size_t);

  rcs_record_t *get_prev (void) const;
  void set_prev (rcs_record_t *);
private:
  /**
   * Offset of 'length' field, in bits
   */
  static constexpr uint32_t _length_field_pos = _fields_offset_begin;

  /**
   * Width of 'length' field, in bits
   */
  static constexpr uint32_t _length_field_width = 14u - RCS_DYN_STORAGE_LENGTH_UNIT_LOG;

  /**
   * Offset of 'previous record' field, in bits
   */
  static constexpr uint32_t _prev_field_pos = _length_field_pos + _length_field_width;

  /**
   * Width of 'previous record' field, in bits
   */
  static constexpr uint32_t _prev_field_width = rcs_cpointer_t::bit_field_width;

  /**
   * Free record should be be placeable at any free space unit of recordset,
   * and so its size should be less than minimal size of a free space unit
   * that is RCS_DYN_STORAGE_LENGTH_UNIT bytes.
   */
  JERRY_STATIC_ASSERT (_prev_field_pos + _prev_field_width <= RCS_DYN_STORAGE_LENGTH_UNIT * JERRY_BITSINBYTE);
};

/**
 * @}
 */

#endif /* RCS_RECORDSET_H */
