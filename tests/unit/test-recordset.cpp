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

#include "mem-allocator.h"
#include "rcs-recordset.h"

#include "test-common.h"

// Heap size is 32K
#define test_heap_size (32 * 1024)

// Iterations count
#define test_iters (64)

// Subiterations count
#define test_sub_iters 64

// Threshold size of block to allocate
#define test_threshold_block_size 8192

// Maximum number of elements in a type-one record
#define test_max_type_one_record_elements 64

class test_rcs_record_type_one_t : public rcs_record_t
{
public:
  static size_t size (uint32_t elements_count)
  {
    return JERRY_ALIGNUP (header_size + element_size * elements_count,
                          RCS_DYN_STORAGE_LENGTH_UNIT);
  }

  size_t get_size () const
  {
    return get_field (length_field_pos, length_field_width) * RCS_DYN_STORAGE_LENGTH_UNIT;
  }

  void set_size (size_t size)
  {
    JERRY_ASSERT (JERRY_ALIGNUP (size, RCS_DYN_STORAGE_LENGTH_UNIT) == size);

    set_field (length_field_pos, length_field_width, size >> RCS_DYN_STORAGE_LENGTH_UNIT_LOG);
  }

  rcs_record_t* get_prev () const
  {
    return get_pointer (prev_field_pos, prev_field_width);
  }

  void set_prev (rcs_record_t* prev_rec_p)
  {
    set_pointer (prev_field_pos, prev_field_width, prev_rec_p);
  }

  typedef uint16_t element_t;

  static const size_t header_size = 2 * RCS_DYN_STORAGE_LENGTH_UNIT;
  static const size_t element_size = sizeof (test_rcs_record_type_one_t::element_t);
private:
  static const uint32_t length_field_pos = _fields_offset_begin;
  static const uint32_t length_field_width = 12u;

  static const uint32_t prev_field_pos = length_field_pos + length_field_width;
  static const uint32_t prev_field_width = rcs_cpointer_t::bit_field_width;
};

class test_rcs_record_type_two_t : public rcs_record_t
{
public:
  static size_t size (void)
  {
    return JERRY_ALIGNUP (header_size, RCS_DYN_STORAGE_LENGTH_UNIT);
  }

  size_t get_size () const
  {
    return size ();
  }

  void set_size (size_t size)
  {
    JERRY_ASSERT (size == get_size ());
  }

  rcs_record_t* get_prev () const
  {
    return get_pointer (prev_field_pos, prev_field_width);
  }

  void set_prev (rcs_record_t* prev_rec_p)
  {
    set_pointer (prev_field_pos, prev_field_width, prev_rec_p);
  }

  static const size_t header_size = RCS_DYN_STORAGE_LENGTH_UNIT;
private:
  static const uint32_t prev_field_pos = _fields_offset_begin;
  static const uint32_t prev_field_width = rcs_cpointer_t::bit_field_width;
};

class test_rcs_recordset_t : public rcs_recordset_t
{
public:
  test_rcs_record_type_one_t*
  create_record_type_one (uint32_t elements_count)
  {
    return alloc_record<test_rcs_record_type_one_t, uint32_t> (_record_type_one_id,
                                                               elements_count);
  }

  void
  free_record_type_one (test_rcs_record_type_one_t* rec_p)
  {
    free_record (rec_p);
  }

  test_rcs_record_type_two_t*
  create_record_type_two (void)
  {
    return alloc_record<test_rcs_record_type_two_t> (_record_type_two_id);
  }

  void
  free_record_type_two (test_rcs_record_type_two_t* rec_p)
  {
    free_record (rec_p);
  }
private:
  static const int _record_type_one_id  = _first_type_id + 0;
  static const int _record_type_two_id  = _first_type_id + 1;

  virtual rcs_record_t* get_prev (rcs_record_t* rec_p)
  {
    switch (rec_p->get_type ())
    {
      case _record_type_one_id:
      {
        return (static_cast<test_rcs_record_type_one_t*> (rec_p))->get_prev ();
      }
      case _record_type_two_id:
      {
        return (static_cast<test_rcs_record_type_two_t*> (rec_p))->get_prev ();
      }
      default:
      {
        JERRY_ASSERT (rec_p->get_type () < _first_type_id);

        return rcs_recordset_t::get_prev (rec_p);
      }
    }
  }

  virtual void set_prev (rcs_record_t* rec_p,
                         rcs_record_t *prev_rec_p)
  {
    switch (rec_p->get_type ())
    {
      case _record_type_one_id:
      {
        return (static_cast<test_rcs_record_type_one_t*> (rec_p))->set_prev (prev_rec_p);
      }
      case _record_type_two_id:
      {
        return (static_cast<test_rcs_record_type_two_t*> (rec_p))->set_prev (prev_rec_p);
      }
      default:
      {
        JERRY_ASSERT (rec_p->get_type () < _first_type_id);

        return rcs_recordset_t::set_prev (rec_p, prev_rec_p);
      }
    }
  }

  virtual size_t get_record_size (rcs_record_t* rec_p)
  {
    switch (rec_p->get_type ())
    {
      case _record_type_one_id:
      {
        return (static_cast<test_rcs_record_type_one_t*> (rec_p))->get_size ();
      }
      case _record_type_two_id:
      {
        return (static_cast<test_rcs_record_type_two_t*> (rec_p))->get_size ();
      }
      default:
      {
        JERRY_ASSERT (rec_p->get_type () < _first_type_id);

        return rcs_recordset_t::get_record_size (rec_p);
      }
    }
  }
};

int
main (int __attr_unused___ argc,
      char __attr_unused___ **argv)
{
  TEST_INIT ();

  mem_init ();

  test_rcs_recordset_t storage;
  storage.init ();

  for (uint32_t i = 0; i < test_iters; i++)
  {
    test_rcs_record_type_one_t *type_one_records[test_sub_iters];
    uint32_t type_one_record_element_counts[test_sub_iters];
    test_rcs_record_type_one_t::element_t type_one_record_elements[test_sub_iters][test_max_type_one_record_elements];
    int type_one_records_number = 0;

    test_rcs_record_type_two_t *type_two_records[test_sub_iters];
    int type_two_records_number = 0;

    for (uint32_t j = 0; j < test_sub_iters; j++)
    {
      if (rand () % 2)
      {
        JERRY_ASSERT (type_one_records_number < test_sub_iters);

        uint32_t elements_count = ((uint32_t) rand ()) % test_max_type_one_record_elements;
        type_one_record_element_counts[type_one_records_number] = elements_count;
        type_one_records[type_one_records_number] = storage.create_record_type_one (elements_count);

        JERRY_ASSERT (type_one_records[type_one_records_number] != NULL);

        rcs_record_iterator_t it (&storage, type_one_records[type_one_records_number]);
        it.skip (test_rcs_record_type_one_t::header_size); // skip header
        for (uint32_t i = 0;
             i < type_one_record_element_counts[type_one_records_number];
             it.skip<test_rcs_record_type_one_t::element_t>(), i++)
        {
          test_rcs_record_type_one_t::element_t val = (test_rcs_record_type_one_t::element_t) rand ();
          type_one_record_elements[type_one_records_number][i] = val;
          it.write (val);
        }

        JERRY_ASSERT (type_one_records[type_one_records_number] != NULL);

        it.reset ();
        it.skip (test_rcs_record_type_one_t::header_size); // skip header
        for (uint32_t i = 0;
             i < type_one_record_element_counts[type_one_records_number];
             it.skip<test_rcs_record_type_one_t::element_t>(), i++)
        {
          test_rcs_record_type_one_t::element_t val = type_one_record_elements[type_one_records_number][i];
          JERRY_ASSERT (val == it.read<test_rcs_record_type_one_t::element_t> ());
        }

        type_one_records_number++;
      }
      else
      {
        JERRY_ASSERT (type_two_records_number < test_sub_iters);

        type_two_records[type_two_records_number] = storage.create_record_type_two ();

        JERRY_ASSERT (type_two_records[type_two_records_number] != NULL);

        type_two_records_number++;
      }
    }

    while (type_one_records_number + type_two_records_number != 0)
    {
      // Read test
      for (int index_to_free = 0; index_to_free < type_one_records_number; index_to_free++)
      {
        JERRY_ASSERT (type_one_records_number > 0);

        JERRY_ASSERT (index_to_free >= 0 && index_to_free < type_one_records_number);

        rcs_record_iterator_t it (&storage, type_one_records[index_to_free]);
        it.skip (test_rcs_record_type_one_t::header_size); // skip header
        for (uint32_t i = 0;
             i < type_one_record_element_counts[index_to_free];
             it.skip<test_rcs_record_type_one_t::element_t>(), i++)
        {
          test_rcs_record_type_one_t::element_t val = type_one_record_elements[index_to_free][i];
          JERRY_ASSERT (it.read <test_rcs_record_type_one_t::element_t> () == val);
        }

        JERRY_ASSERT (JERRY_ALIGNUP (type_one_record_element_counts[index_to_free]
                                     * sizeof (test_rcs_record_type_one_t::element_t)
                                     + test_rcs_record_type_one_t::header_size,
                                     RCS_DYN_STORAGE_LENGTH_UNIT)
                      == type_one_records[index_to_free]->get_size ());
      }

      bool free_type_one;

      if (type_one_records_number == 0)
      {
        JERRY_ASSERT (type_two_records_number != 0);

        free_type_one = false;
      }
      else if (type_two_records_number == 0)
      {
        JERRY_ASSERT (type_one_records_number != 0);

        free_type_one = true;
      }
      else
      {
        free_type_one = ((rand () % 2) != 0);
      }

      if (free_type_one)
      {
        JERRY_ASSERT (type_one_records_number > 0);
        int index_to_free = (rand () % type_one_records_number);

        JERRY_ASSERT (index_to_free >= 0 && index_to_free < type_one_records_number);

        rcs_record_iterator_t it (&storage, type_one_records[index_to_free]);
        it.skip (test_rcs_record_type_one_t::header_size); // skip header
        for (uint32_t i = 0;
             i < type_one_record_element_counts[index_to_free];
             it.skip<test_rcs_record_type_one_t::element_t>(), i++)
        {
          test_rcs_record_type_one_t::element_t val = type_one_record_elements[index_to_free][i];
          JERRY_ASSERT (it.read <test_rcs_record_type_one_t::element_t> () == val);
        }

        // free the record
        storage.free_record_type_one (type_one_records[index_to_free]);

        type_one_records_number--;
        while (index_to_free < type_one_records_number)
        {
          type_one_records[index_to_free] = type_one_records[index_to_free + 1];
          type_one_record_element_counts[index_to_free] = type_one_record_element_counts[index_to_free + 1];
          memcpy (type_one_record_elements[index_to_free], type_one_record_elements[index_to_free + 1],
                  test_max_type_one_record_elements * sizeof (test_rcs_record_type_one_t::element_t));

          index_to_free++;
        }
      }
      else
      {
        JERRY_ASSERT (type_two_records_number > 0);
        int index_to_free = (rand () % type_two_records_number);

        JERRY_ASSERT (index_to_free >= 0 && index_to_free < type_two_records_number);

        storage.free_record_type_two (type_two_records[index_to_free]);

        type_two_records_number--;
        while (index_to_free < type_two_records_number)
        {
          type_two_records[index_to_free] = type_two_records[index_to_free + 1];

          index_to_free++;
        }
      }
    }
  }

  storage.finalize ();

  mem_finalize (true);

  return 0;
} /* main */

template test_rcs_record_type_one_t *
rcs_recordset_t::alloc_record<test_rcs_record_type_one_t> (rcs_record_t::type_t, uint32_t);

template test_rcs_record_type_two_t *
rcs_recordset_t::alloc_record<test_rcs_record_type_two_t> (rcs_record_t::type_t);
