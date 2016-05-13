/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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

#include "ecma-alloc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "jrt.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

/**
 * Allocate a collection of ecma values.
 *
 * @return pointer to the collection's header
 */
ecma_collection_header_t *
ecma_new_values_collection (const ecma_value_t values_buffer[], /**< ecma values */
                            ecma_length_t values_number, /**< number of ecma values */
                            bool do_ref_if_object) /**< if the value is object value,
                                                        increase reference counter of the object */
{
  JERRY_ASSERT (values_buffer != NULL || values_number == 0);

  const size_t values_in_chunk = JERRY_SIZE_OF_STRUCT_MEMBER (ecma_collection_chunk_t, data) / sizeof (ecma_value_t);

  ecma_collection_header_t *header_p = ecma_alloc_collection_header ();

  header_p->unit_number = values_number;

  jmem_cpointer_t *next_chunk_cp_p = &header_p->first_chunk_cp;
  ecma_collection_chunk_t *last_chunk_p = NULL;
  ecma_value_t *cur_value_buf_iter_p = NULL;
  ecma_value_t *cur_value_buf_end_p = NULL;

  for (ecma_length_t value_index = 0;
       value_index < values_number;
       value_index++)
  {
    if (cur_value_buf_iter_p == cur_value_buf_end_p)
    {
      ecma_collection_chunk_t *chunk_p = ecma_alloc_collection_chunk ();
      ECMA_SET_POINTER (*next_chunk_cp_p, chunk_p);
      next_chunk_cp_p = &chunk_p->next_chunk_cp;

      cur_value_buf_iter_p = (ecma_value_t *) chunk_p->data;
      cur_value_buf_end_p = cur_value_buf_iter_p + values_in_chunk;

      last_chunk_p = chunk_p;
    }

    JERRY_ASSERT (cur_value_buf_iter_p + 1 <= cur_value_buf_end_p);

    if (do_ref_if_object)
    {
      *cur_value_buf_iter_p++ = ecma_copy_value (values_buffer[value_index]);
    }
    else
    {
      *cur_value_buf_iter_p++ = ecma_copy_value_if_not_object (values_buffer[value_index]);
    }
  }

  *next_chunk_cp_p = ECMA_NULL_POINTER;
  ECMA_SET_POINTER (header_p->last_chunk_cp, last_chunk_p);

  return header_p;
} /* ecma_new_values_collection */

/**
 * Free the collection of ecma values.
 */
void
ecma_free_values_collection (ecma_collection_header_t *header_p, /**< collection's header */
                             bool do_deref_if_object) /**< if the value is object value,
                                                           decrement reference counter of the object */
{
  JERRY_ASSERT (header_p != NULL);

  const size_t values_in_chunk = JERRY_SIZE_OF_STRUCT_MEMBER (ecma_collection_chunk_t, data) / sizeof (ecma_value_t);

  ecma_collection_chunk_t *chunk_p = ECMA_GET_POINTER (ecma_collection_chunk_t,
                                                       header_p->first_chunk_cp);
  ecma_length_t value_index = 0;

  while (chunk_p != NULL)
  {
    JERRY_ASSERT (value_index < header_p->unit_number);

    ecma_value_t *cur_value_buf_iter_p = (ecma_value_t *) chunk_p->data;
    ecma_value_t *cur_value_buf_end_p = cur_value_buf_iter_p + values_in_chunk;

    while (cur_value_buf_iter_p != cur_value_buf_end_p
           && value_index < header_p->unit_number)
    {
      JERRY_ASSERT (cur_value_buf_iter_p < cur_value_buf_end_p);

      if (do_deref_if_object)
      {
        ecma_free_value (*cur_value_buf_iter_p);
      }
      else
      {
        ecma_free_value_if_not_object (*cur_value_buf_iter_p);
      }

      cur_value_buf_iter_p++;
      value_index++;
    }

    ecma_collection_chunk_t *next_chunk_p = ECMA_GET_POINTER (ecma_collection_chunk_t,
                                                              chunk_p->next_chunk_cp);
    ecma_dealloc_collection_chunk (chunk_p);
    chunk_p = next_chunk_p;
  }

  ecma_dealloc_collection_header (header_p);
} /* ecma_free_values_collection */

/**
 * Append new value to ecma values collection
 */
void
ecma_append_to_values_collection (ecma_collection_header_t *header_p, /**< collection's header */
                                  ecma_value_t v, /**< ecma value to append */
                                  bool do_ref_if_object) /**< if the value is object value,
                                                              increase reference counter of the object */
{
  const size_t values_in_chunk = JERRY_SIZE_OF_STRUCT_MEMBER (ecma_collection_chunk_t, data) / sizeof (ecma_value_t);

  size_t values_number = header_p->unit_number;
  size_t pos_of_new_value_in_chunk = values_number % values_in_chunk;

  values_number++;

  if ((ecma_length_t) values_number == values_number)
  {
    header_p->unit_number = (ecma_length_t) values_number;
  }
  else
  {
    jerry_fatal (ERR_OUT_OF_MEMORY);
  }

  ecma_collection_chunk_t *chunk_p = ECMA_GET_POINTER (ecma_collection_chunk_t,
                                                       header_p->last_chunk_cp);

  if (pos_of_new_value_in_chunk == 0)
  {
    /* all chunks are currently filled with values */

    chunk_p = ecma_alloc_collection_chunk ();
    chunk_p->next_chunk_cp = ECMA_NULL_POINTER;

    if (header_p->last_chunk_cp == ECMA_NULL_POINTER)
    {
      JERRY_ASSERT (header_p->first_chunk_cp == ECMA_NULL_POINTER);

      ECMA_SET_NON_NULL_POINTER (header_p->first_chunk_cp, chunk_p);
    }
    else
    {
      ecma_collection_chunk_t *last_chunk_p = ECMA_GET_NON_NULL_POINTER (ecma_collection_chunk_t,
                                                                         header_p->last_chunk_cp);

      JERRY_ASSERT (last_chunk_p->next_chunk_cp == ECMA_NULL_POINTER);

      ECMA_SET_NON_NULL_POINTER (last_chunk_p->next_chunk_cp, chunk_p);
    }

    ECMA_SET_NON_NULL_POINTER (header_p->last_chunk_cp, chunk_p);
  }
  else
  {
    /* last chunk can be appended with the new value */
    JERRY_ASSERT (chunk_p != NULL);
  }

  ecma_value_t *values_p = (ecma_value_t *) chunk_p->data;

  JERRY_ASSERT ((uint8_t *) (values_p + pos_of_new_value_in_chunk + 1) <= (uint8_t *) (chunk_p + 1));

  if (do_ref_if_object)
  {
    values_p[pos_of_new_value_in_chunk] = ecma_copy_value (v);
  }
  else
  {
    values_p[pos_of_new_value_in_chunk] = ecma_copy_value_if_not_object (v);
  }
} /* ecma_append_to_values_collection */

/**
 * Remove last element of the collection
 *
 * Warning:
 *         the function invalidates all iterators that are configured to access the passed collection
 */
void
ecma_remove_last_value_from_values_collection (ecma_collection_header_t *header_p) /**< collection's header */
{
  JERRY_ASSERT (header_p != NULL && header_p->unit_number > 0);

  const size_t values_in_chunk = JERRY_SIZE_OF_STRUCT_MEMBER (ecma_collection_chunk_t, data) / sizeof (ecma_value_t);
  size_t values_number = header_p->unit_number;
  size_t pos_of_value_to_remove_in_chunk = (values_number - 1u) % values_in_chunk;

  ecma_collection_chunk_t *last_chunk_p = ECMA_GET_NON_NULL_POINTER (ecma_collection_chunk_t,
                                                                     header_p->last_chunk_cp);

  ecma_value_t *values_p = (ecma_value_t *) last_chunk_p->data;
  JERRY_ASSERT ((uint8_t *) (values_p + pos_of_value_to_remove_in_chunk + 1) <= (uint8_t *) (last_chunk_p + 1));

  ecma_value_t value_to_remove = values_p[pos_of_value_to_remove_in_chunk];

  ecma_free_value (value_to_remove);

  header_p->unit_number--;

  if (pos_of_value_to_remove_in_chunk == 0)
  {
    ecma_collection_chunk_t *chunk_to_remove_p = last_chunk_p;

    /* free last chunk */
    if (header_p->first_chunk_cp == header_p->last_chunk_cp)
    {
      header_p->first_chunk_cp = ECMA_NULL_POINTER;
      header_p->last_chunk_cp = ECMA_NULL_POINTER;
    }
    else
    {
      ecma_collection_chunk_t *chunk_iter_p = ECMA_GET_NON_NULL_POINTER (ecma_collection_chunk_t,
                                                                         header_p->first_chunk_cp);

      while (chunk_iter_p->next_chunk_cp != header_p->last_chunk_cp)
      {
        chunk_iter_p = ECMA_GET_NON_NULL_POINTER (ecma_collection_chunk_t,
                                                  chunk_iter_p->next_chunk_cp);
      }

      ecma_collection_chunk_t *new_last_chunk_p = chunk_iter_p;

      JERRY_ASSERT (ECMA_GET_NON_NULL_POINTER (ecma_collection_chunk_t,
                                               new_last_chunk_p->next_chunk_cp) == chunk_to_remove_p);

      ECMA_SET_NON_NULL_POINTER (header_p->last_chunk_cp, new_last_chunk_p);
      new_last_chunk_p->next_chunk_cp = ECMA_NULL_POINTER;
    }

    ecma_dealloc_collection_chunk (chunk_to_remove_p);
  }
} /* ecma_remove_last_value_from_values_collection */

/**
 * Allocate a collection of ecma-strings.
 *
 * @return pointer to the collection's header
 */
ecma_collection_header_t *
ecma_new_strings_collection (ecma_string_t *string_ptrs_buffer[], /**< pointers to ecma-strings */
                             ecma_length_t strings_number) /**< number of ecma-strings */
{
  JERRY_ASSERT (string_ptrs_buffer != NULL || strings_number == 0);

  ecma_collection_header_t *new_collection_p;

  new_collection_p = ecma_new_values_collection (NULL, 0, false);

  for (ecma_length_t string_index = 0;
       string_index < strings_number;
       string_index++)
  {
    ecma_append_to_values_collection (new_collection_p,
                                      ecma_make_string_value (string_ptrs_buffer[string_index]),
                                      false);
  }

  return new_collection_p;
} /* ecma_new_strings_collection */

/**
 * Initialize new collection iterator for the collection
 */
void
ecma_collection_iterator_init (ecma_collection_iterator_t *iterator_p, /**< context of iterator */
                               ecma_collection_header_t *collection_p) /**< header of collection */
{
  iterator_p->header_p = collection_p;
  iterator_p->next_chunk_cp = (collection_p != NULL ? collection_p->first_chunk_cp : JMEM_CP_NULL);
  iterator_p->current_index = 0;
  iterator_p->current_value_p = NULL;
  iterator_p->current_chunk_end_p = NULL;
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
  if (iterator_p->header_p == NULL
      || unlikely (iterator_p->header_p->unit_number == 0))
  {
    return false;
  }

  const size_t values_in_chunk = JERRY_SIZE_OF_STRUCT_MEMBER (ecma_collection_chunk_t, data) / sizeof (ecma_value_t);

  if (iterator_p->current_value_p == NULL)
  {
    JERRY_ASSERT (iterator_p->current_index == 0);

    ecma_collection_chunk_t *first_chunk_p = ECMA_GET_NON_NULL_POINTER (ecma_collection_chunk_t,
                                                                        iterator_p->header_p->first_chunk_cp);

    iterator_p->next_chunk_cp = first_chunk_p->next_chunk_cp;
    iterator_p->current_value_p = (ecma_value_t *) &first_chunk_p->data;
    iterator_p->current_chunk_end_p = (iterator_p->current_value_p + values_in_chunk);
  }
  else
  {
    if (iterator_p->current_index + 1 == iterator_p->header_p->unit_number)
    {
      return false;
    }

    JERRY_ASSERT (iterator_p->current_index + 1 < iterator_p->header_p->unit_number);

    iterator_p->current_index++;
    iterator_p->current_value_p++;
  }

  if (iterator_p->current_value_p == iterator_p->current_chunk_end_p)
  {
    ecma_collection_chunk_t *next_chunk_p = ECMA_GET_POINTER (ecma_collection_chunk_t,
                                                              iterator_p->next_chunk_cp);
    JERRY_ASSERT (next_chunk_p != NULL);

    iterator_p->next_chunk_cp = next_chunk_p->next_chunk_cp;
    iterator_p->current_value_p = (ecma_value_t *) &next_chunk_p->data;
    iterator_p->current_chunk_end_p = iterator_p->current_value_p + values_in_chunk;
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
