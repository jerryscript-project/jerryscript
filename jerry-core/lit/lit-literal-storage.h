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

#ifndef LIT_LITERAL_STORAGE_H
#define LIT_LITERAL_STORAGE_H

#include "ecma-globals.h"
#include "rcs-recordset.h"

class lit_literal_storage_t;
extern lit_literal_storage_t lit_storage;

/**
 * Charset record
 *
 * layout:
 * ------- header -----------------------
 * type (4 bits)
 * alignment (2 bits)
 * unused (2 bits)
 * hash (8 bits)
 * length (16 bits)
 * pointer to prev (16 bits)
 * ------- characters -------------------
 * ...
 * chars
 * ....
 * ------- alignment bytes --------------
 * unused bytes (their count is specified
 * by 'alignment' field in header)
 * --------------------------------------
 */
class lit_charset_record_t : public rcs_record_t
{
  friend class rcs_recordset_t;
  friend class lit_literal_storage_t;

public:
  /**
   * Calculate the size that record will occupy inside the storage
   *
   * @return size of the record in bytes
   */
  static size_t
  size (size_t size) /**< size of the charset buffer */
  {
    return JERRY_ALIGNUP (size + header_size (), RCS_DYN_STORAGE_LENGTH_UNIT);
  } /* size */

  /**
   * Get the size of the header part of the record
   *
   * @return size of the header in bytes
   */
  static size_t
  header_size ()
  {
    return _header_size;
  } /* header_size */

  /**
   * Get the count of the alignment bytes at the end of record.
   * These bytes are needed to align the record to RCS_DYN_STORAGE_ALIGNMENT.
   *
   * @return alignment bytes count (the value of the 'alignment' field in the header)
   */
  size_t
  get_alignment_bytes_count () const
  {
    return get_field (_alignment_field_pos, _alignment_field_width);
  } /* get_alignment_bytes_count */

  /**
   * Set the count of the alignment bytes at the end of record (the value of the 'alignment' field in the header)
   */
  void
  set_alignment_bytes_count (size_t count) /**< count of the alignment bytes */
  {
    JERRY_ASSERT (count <= RCS_DYN_STORAGE_LENGTH_UNIT);
    set_field (_alignment_field_pos, _alignment_field_width, count);
  } /* set_alignment_bytes_count */

  /**
   * Get hash value of the record's charset.
   *
   * @return hash value of the string (the value of the 'hash' field in the header)
   */
  lit_string_hash_t
  get_hash () const
  {
    return (lit_string_hash_t) get_field (_hash_field_pos, _hash_field_width);
  } /* get_hash */

  /**
   * Get the length of the string, which is contained inside the record
   *
   * @return length of the string (bytes count)
   */
  lit_utf8_size_t
  get_length () const
  {
    return (lit_utf8_size_t) (get_size () - header_size () - get_alignment_bytes_count ());
  } /* get_length */

  /**
   * Get the size of the record
   *
   * @return size of the record in bytes
   */
  size_t
  get_size () const
  {
    return get_field (_length_field_pos, _length_field_width) * RCS_DYN_STORAGE_LENGTH_UNIT;
  } /* get_size */

  rcs_record_t *get_prev () const;

  lit_utf8_size_t get_charset (lit_utf8_byte_t *, size_t);

  int compare_utf8 (const lit_utf8_byte_t *, lit_utf8_size_t);
  bool is_equal (lit_charset_record_t *);
  bool is_equal_utf8_string (const lit_utf8_byte_t *, lit_utf8_size_t);

  uint32_t dump_for_snapshot (uint8_t *, size_t, size_t *);
private:
  /**
   * Set record's size (the value of the 'length' field in the header)
   */
  void
  set_size (size_t size) /**< size in bytes */
  {
    JERRY_ASSERT (JERRY_ALIGNUP (size, RCS_DYN_STORAGE_LENGTH_UNIT) == size);

    set_field (_length_field_pos, _length_field_width, size >> RCS_DYN_STORAGE_LENGTH_UNIT_LOG);
  } /* set_size */

  /**
   * Set record's hash (the value of the 'hash' field in the header)
   */
  void
  set_hash (lit_string_hash_t hash) /**< hash value */
  {
    set_field (_hash_field_pos, _hash_field_width, hash);
  } /* set_hash */

  void set_prev (rcs_record_t *);

  void set_charset (const lit_utf8_byte_t *, lit_utf8_size_t);

  /**
   * Offset and length of 'alignment' field, in bits
   */
  static const uint32_t _alignment_field_pos = _fields_offset_begin;
  static const uint32_t _alignment_field_width = RCS_DYN_STORAGE_LENGTH_UNIT_LOG;

  /**
   * Offset and length of 'hash' field, in bits
   */
  static const uint32_t _hash_field_pos = _alignment_field_pos + _alignment_field_width;
  static const uint32_t _hash_field_width = 8u;

  /**
   * Offset and length of 'alignment' field, in bits
   */
  static const uint32_t _length_field_pos = _hash_field_pos + _hash_field_width;
  static const uint32_t _length_field_width = 16u;

  /**
   * Offset and length of 'prev' field, in bits
   */
  static const uint32_t _prev_field_pos = _length_field_pos + _length_field_width;
  static const uint32_t _prev_field_width = rcs_cpointer_t::bit_field_width;

  static const size_t _header_size = RCS_DYN_STORAGE_LENGTH_UNIT + RCS_DYN_STORAGE_LENGTH_UNIT / 2;
}; /* lit_charset_record_t */

/**
 * Magic string record
 * Doesn't hold any characters. Corresponding string is identified by its id.
 *
 * Layout:
 * ------- header -----------------------
 * type (4 bits)
 * magic string id  (12 bits)
 * pointer to prev (16 bits)
 * --------------------------------------
 */
class lit_magic_record_t : public rcs_record_t
{
  friend class rcs_recordset_t;
  friend class lit_literal_storage_t;

public:
  /**
   * Calculate the size that record will occupy inside the storage
   *
   * @return size of the record in bytes
   */
  static size_t size ()
  {
    return _size;
  } /* size */

  /**
   * Get the size of the header part of the record
   *
   * @return size of the header in bytes
   */
  static size_t header_size ()
  {
    return _size;
  } /* header_size */

  /**
   * Get the size of the record
   *
   * @return size of the record in bytes
   */
  size_t get_size () const
  {
    return _size;
  } /* get_size */

  /**
   * Get magic string id which is held by the record
   *
   * @return magic string id
   */
  template <typename magic_string_id_t>
  magic_string_id_t get_magic_str_id () const
  {
    uint32_t id = get_field (magic_field_pos, magic_field_width);
    return (magic_string_id_t) id;
  } /* get_magic_str_id */

  /**
   * Dump magic string record to specified snapshot buffer
   *
   * @return number of bytes dumped,
   *         or 0 - upon dump failure
   */
  template <typename magic_string_id_t>
  uint32_t dump_for_snapshot (uint8_t *buffer_p, /**< buffer to dump to */
                              size_t buffer_size, /**< buffer size */
                              size_t *in_out_buffer_offset_p) const /**< in-out: buffer write offset */
  {
    magic_string_id_t id = get_magic_str_id<magic_string_id_t> ();
    if (!jrt_write_to_buffer_by_offset (buffer_p, buffer_size, in_out_buffer_offset_p, id))
    {
      return 0;
    }

    return ((uint32_t) sizeof (id));
  } /* dump_for_snapshot */
private:
  /**
   * Set record's size (the value of the 'length' field in the header)
   */
  void set_size (size_t size) /**< size in bytes */
  {
    JERRY_ASSERT (size == get_size ());
  } /* set_size */

  template <typename magic_string_id_t>
  void set_magic_str_id (magic_string_id_t id)
  {
    set_field (magic_field_pos, magic_field_width, id);
  } /* set_magic_str_id */

  /**
   * Get previous record in the literal storage
   *
   * @return  pointer to the previous record relative to this record, or NULL if this is the first record in the
   *          storage
   */
  rcs_record_t *get_prev () const
  {
    return get_pointer (prev_field_pos, prev_field_width);
  } /* get_prev */

  /**
   * Set the pointer to the previous record inside the literal storage (sets 'prev' field in the header)
   */
  void set_prev (rcs_record_t *prev_rec_p)
  {
    set_pointer (prev_field_pos, prev_field_width, prev_rec_p);
  } /* set_prev */


  /**
   * Offset and length of 'magic string id' field, in bits
   */
  static const uint32_t magic_field_pos = _fields_offset_begin;
  static const uint32_t magic_field_width = 12u;

  /**
   * Offset and length of 'prev' field, in bits
   */
  static const uint32_t prev_field_pos = magic_field_pos + magic_field_width;
  static const uint32_t prev_field_width = rcs_cpointer_t::bit_field_width;

  static const size_t _size = RCS_DYN_STORAGE_LENGTH_UNIT;
}; /* lit_magic_record_t */

/**
 * Number record
 * Doesn't hold any characters, holds a number. Numbers from source code are represented as number literals.
 *
 * Layout:
 * ------- header -----------------------
 * type (4 bits)
 * padding  (12 bits)
 * pointer to prev (16 bits)
 * --------------------------------------
 * ecma_number_t
 */
class lit_number_record_t : public rcs_record_t
{
  friend class rcs_recordset_t;
  friend class lit_literal_storage_t;

public:

  static size_t
  size ()
  {
    return _size;
  } /* size */

  /**
   * Get the size of the header part of the record
   *
   * @return size of the header in bytes
   */
  static size_t
  header_size ()
  {
    return _header_size;
  } /* header_size */

  /**
   * Get the size of the record
   *
   * @return size of the record in bytes
   */
  size_t
  get_size () const
  {
    return _size;
  } /* get_size */

  /**
   * Get previous record in the literal storage
   *
   * @return  pointer to the previous record relative to this record, or NULL if this is the first record in the
   *          storage
   */
  rcs_record_t *
  get_prev () const
  {
    return get_pointer (prev_field_pos, prev_field_width);
  } /* get_prev */

  /**
   * Set the pointer to the previous record inside the literal storage (sets 'prev' field in the header)
   */
  void
  set_prev (rcs_record_t *prev_rec_p)
  {
    set_pointer (prev_field_pos, prev_field_width, prev_rec_p);
  } /* set_prev */

  /**
   * Get the number which is held by the record
   *
   * @return number
   */
  ecma_number_t
  get_number () const
  {
    rcs_record_iterator_t it ((rcs_recordset_t *)&lit_storage,
                              (rcs_record_t *)this);
    it.skip (header_size ());

    return it.read<ecma_number_t> ();
  } /* get_number */

  uint32_t dump_for_snapshot (uint8_t *, size_t, size_t *) const;
private:
  /**
   * Set record's size (the value of the 'length' field in the header)
   */
  void
  set_size (size_t size) /**< size in bytes */
  {
    JERRY_ASSERT (size == get_size ());
  } /* set_size */

  /**
   * Offset and length of 'prev' field, in bits
   */
  static const uint32_t prev_field_pos = _fields_offset_begin + 12u;
  static const uint32_t prev_field_width = rcs_cpointer_t::bit_field_width;

  static const size_t _header_size = RCS_DYN_STORAGE_LENGTH_UNIT;
  static const size_t _size = _header_size + sizeof (ecma_number_t);
}; /* lit_number_record_t */

/**
 * Literal storage
 *
 * Represents flexible storage for the literals. The following records could be created inside the storage:
 * - charset literal (lit_charset_record_t)
 * - magic string literal (lit_magic_record_t)
 * - number literal (lit_number_record_t)
 */
class lit_literal_storage_t : public rcs_recordset_t
{
public:
  enum
  {
    LIT_TYPE_FIRST = _first_type_id,

    LIT_STR = LIT_TYPE_FIRST,
    LIT_MAGIC_STR,
    LIT_MAGIC_STR_EX,
    LIT_NUMBER,

    LIT_TYPE_LAST = LIT_NUMBER
  };

  lit_charset_record_t *create_charset_record (const lit_utf8_byte_t *, lit_utf8_size_t);
  lit_magic_record_t *create_magic_record (lit_magic_string_id_t);
  lit_magic_record_t *create_magic_record_ex (lit_magic_string_ex_id_t);
  lit_number_record_t *create_number_record (ecma_number_t);

  uint32_t count_literals (void);

  void dump ();

private:
  virtual rcs_record_t *get_prev (rcs_record_t *);
  virtual void set_prev (rcs_record_t *, rcs_record_t *);
  virtual size_t get_record_size (rcs_record_t *);
}; /* lit_literal_storage_t */

#define LIT_STR_T (lit_literal_storage_t::LIT_STR)
#define LIT_MAGIC_STR_T (lit_literal_storage_t::LIT_MAGIC_STR)
#define LIT_MAGIC_STR_EX_T (lit_literal_storage_t::LIT_MAGIC_STR_EX)
#define LIT_NUMBER_T (lit_literal_storage_t::LIT_NUMBER)

#ifdef JERRY_ENABLE_SNAPSHOT
/**
 * Map from literal identifiers to the literal offsets in snapshot (or reverse)
 */
typedef struct
{
  lit_cpointer_t literal_id; /**< identifier of literal in literal storage */
  uint32_t literal_offset; /**< offset of the literal in snapshot */
} lit_mem_to_snapshot_id_map_entry_t;

extern bool
lit_dump_literals_for_snapshot (uint8_t *, size_t, size_t *, lit_mem_to_snapshot_id_map_entry_t **,
                                uint32_t *, uint32_t *);
extern bool lit_load_literals_from_snapshot (const uint8_t *, uint32_t, lit_mem_to_snapshot_id_map_entry_t **,
                                             uint32_t *, bool);
#endif /* JERRY_ENABLE_SNAPSHOT */


#endif /* LIT_LITERAL_STORAGE_H */
