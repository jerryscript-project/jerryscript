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

/** \addtogroup ecma ---TODO---
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

#include "ecma-alloc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "globals.h"

/**
 * Allocate a collection of ecma-values.
 *
 * @return pointer to the collection's header
 */
ecma_collection_header_t*
ecma_new_values_collection (ecma_value_t values_buffer[], /**< ecma-values */
                            ecma_length_t values_number, /**< number of ecma-values */
                            bool do_ref_if_object) /**< if the value is object value,
                                                        increase reference counter of the object */
{
  JERRY_ASSERT (values_buffer != NULL);
  JERRY_ASSERT (values_number > 0);

  ecma_collection_header_t* header_p = ecma_alloc_collection_header ();

  header_p->unit_number = values_number;

  uint16_t* next_chunk_cp_p = &header_p->next_chunk_cp;
  ecma_value_t* cur_value_buf_iter_p = (ecma_value_t*) header_p->data;
  ecma_value_t* cur_value_buf_end_p = cur_value_buf_iter_p + sizeof (header_p->data) / sizeof (ecma_value_t);

  for (ecma_length_t value_index = 0;
       value_index < values_number;
       value_index++)
  {
    if (unlikely (cur_value_buf_iter_p == cur_value_buf_end_p))
    {
      ecma_collection_chunk_t *chunk_p = ecma_alloc_collection_chunk ();
      ECMA_SET_POINTER (*next_chunk_cp_p, chunk_p);
      next_chunk_cp_p = &chunk_p->next_chunk_cp;

      cur_value_buf_iter_p = (ecma_value_t*) chunk_p->data;
      cur_value_buf_end_p = cur_value_buf_iter_p + sizeof (chunk_p->data) / sizeof (ecma_value_t);
    }

    JERRY_ASSERT (cur_value_buf_iter_p + 1 <= cur_value_buf_end_p);

    *cur_value_buf_iter_p++ = ecma_copy_value (values_buffer[value_index], do_ref_if_object);
  }

  *next_chunk_cp_p = ECMA_NULL_POINTER;

  return header_p;
} /* ecma_new_values_collection */

/**
 * Free the collection of ecma-values.
 */
void
ecma_free_values_collection (ecma_collection_header_t* header_p, /**< collection's header */
                             bool do_deref_if_object) /**< if the value is object value,
                                                           decrement reference counter of the object */
{
  JERRY_ASSERT (header_p != NULL);

  ecma_value_t* cur_value_buf_iter_p = (ecma_value_t*) header_p->data;
  ecma_value_t* cur_value_buf_end_p = cur_value_buf_iter_p + sizeof (header_p->data) / sizeof (ecma_value_t);

  ecma_length_t string_index = 0;
  while (cur_value_buf_iter_p != cur_value_buf_end_p
         && string_index < header_p->unit_number)
  {
    JERRY_ASSERT (cur_value_buf_iter_p < cur_value_buf_end_p);

    ecma_free_value (*cur_value_buf_iter_p, do_deref_if_object);

    cur_value_buf_iter_p++;
    string_index++;
  }

  ecma_collection_chunk_t *chunk_p = ECMA_GET_POINTER (header_p->next_chunk_cp);

  while (chunk_p != NULL)
  {
    JERRY_ASSERT (string_index < header_p->unit_number);

    cur_value_buf_iter_p = (ecma_value_t*) chunk_p->data;
    cur_value_buf_end_p = cur_value_buf_iter_p + sizeof (chunk_p->data) / sizeof (ecma_value_t);

    while (cur_value_buf_iter_p != cur_value_buf_end_p
           && string_index < header_p->unit_number)
    {
      JERRY_ASSERT (cur_value_buf_iter_p < cur_value_buf_end_p);

      ecma_free_value (*cur_value_buf_iter_p, do_deref_if_object);

      cur_value_buf_iter_p++;
      string_index++;
    }

    ecma_collection_chunk_t *next_chunk_p = ECMA_GET_POINTER (chunk_p->next_chunk_cp);
    ecma_dealloc_collection_chunk (chunk_p);
    chunk_p = next_chunk_p;
  }

  ecma_dealloc_collection_header (header_p);
} /* ecma_free_values_collection */

/**
 * Allocate a collection of ecma-strings.
 *
 * @return pointer to the collection's header
 */
ecma_collection_header_t*
ecma_new_strings_collection (ecma_string_t* string_ptrs_buffer[], /**< pointers to ecma-strings */
                             ecma_length_t strings_number) /**< number of ecma-strings */
{
  JERRY_ASSERT (string_ptrs_buffer != NULL);
  JERRY_ASSERT (strings_number > 0);

  ecma_value_t values_buffer[strings_number];

  for (ecma_length_t string_index = 0;
       string_index < strings_number;
       string_index++)
  {
    values_buffer[string_index] = ecma_make_string_value (string_ptrs_buffer[string_index]);
  }

  return ecma_new_values_collection (values_buffer, strings_number, false);
} /* ecma_new_strings_collection */

/**
 * Initialize new collection iterator for the collection
 */
void
ecma_collection_iterator_init (ecma_collection_iterator_t *iterator_p, /**< context of iterator */
                               ecma_collection_header_t *collection_p) /**< header of collection */
{
  iterator_p->header_p = collection_p;
  iterator_p->next_chunk_cp = collection_p->next_chunk_cp;
  iterator_p->current_index = 0;
  iterator_p->current_value_p = NULL;
  iterator_p->current_chunk_end_p = ((ecma_value_t*) iterator_p->header_p->data
                                     + sizeof (iterator_p->header_p->data) / sizeof (ecma_value_t));
} /* ecma_collection_iterator_init */

/**
 * Move collection iterator to next element if there is any.
 *
 * @return true - if iterator moved,
 *         false - otherwise (current element is last element in the collection)
 */
bool
ecma_collection_iterator_next (ecma_collection_iterator_t *iterator_p) /**< context of iterator */
{
  if (unlikely (iterator_p->header_p->unit_number == 0))
  {
    return false;
  }

  if (iterator_p->current_value_p == NULL)
  {
    JERRY_ASSERT (iterator_p->current_index == 0);
    iterator_p->current_value_p = (ecma_value_t*) iterator_p->header_p->data;

    return true;
  }

  if (iterator_p->current_index + 1 == iterator_p->header_p->unit_number)
  {
    return false;
  }

  JERRY_ASSERT (iterator_p->current_index + 1 < iterator_p->header_p->unit_number);

  iterator_p->current_index++;
  iterator_p->current_value_p++;

  if (iterator_p->current_value_p == iterator_p->current_chunk_end_p)
  {
    ecma_collection_chunk_t *next_chunk_p = ECMA_GET_POINTER (iterator_p->next_chunk_cp);
    JERRY_ASSERT (next_chunk_p != NULL);

    iterator_p->next_chunk_cp = next_chunk_p->next_chunk_cp;
    iterator_p->current_value_p = (ecma_value_t*) &next_chunk_p->data;
    iterator_p->current_chunk_end_p = iterator_p->current_value_p + sizeof (next_chunk_p->data) / sizeof (ecma_value_t);
  }
  else
  {
    JERRY_ASSERT (iterator_p->current_value_p < iterator_p->current_chunk_end_p);
  }

  return true;
} /* ecma_collection_iterator_next */

/**
 * @}
 * @}
 */
