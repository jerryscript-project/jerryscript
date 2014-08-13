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

  ecma_collection_header_t* header_p = ecma_alloc_collection_header ();

  header_p->unit_number = strings_number;

  uint16_t* next_chunk_cp_p = &header_p->next_chunk_cp;
  uint16_t* cur_strcp_buf_iter_p = (uint16_t*) header_p->data;
  uint16_t* cur_strcp_buf_end_p = cur_strcp_buf_iter_p + sizeof (header_p->data) / sizeof (uint16_t);

  for (ecma_length_t string_index = 0;
       string_index < strings_number;
       string_index++)
  {
    if (unlikely (cur_strcp_buf_iter_p == cur_strcp_buf_end_p))
    {
      ecma_collection_chunk_t *chunk_p = ecma_alloc_collection_chunk ();
      ECMA_SET_POINTER (*next_chunk_cp_p, chunk_p);
      next_chunk_cp_p = &chunk_p->next_chunk_cp;

      cur_strcp_buf_iter_p = (uint16_t*) chunk_p->data;
      cur_strcp_buf_end_p = cur_strcp_buf_iter_p + sizeof (chunk_p->data) / sizeof (uint16_t);
    }

    JERRY_ASSERT (cur_strcp_buf_iter_p + 1 <= cur_strcp_buf_end_p);

    ecma_ref_ecma_string (string_ptrs_buffer[string_index]);
    ECMA_SET_POINTER (*cur_strcp_buf_iter_p, string_ptrs_buffer[string_index]);
    cur_strcp_buf_iter_p++;
  }

  return header_p;
} /* ecma_new_strings_collection */

/**
 * Free the collection of ecma-strings.
 */
void
ecma_free_strings_collection (ecma_collection_header_t* header_p) /**< collection's header */
{
  JERRY_ASSERT (header_p != NULL);

  uint16_t* cur_strcp_buf_iter_p = (uint16_t*) header_p->data;
  uint16_t* cur_strcp_buf_end_p = cur_strcp_buf_iter_p + sizeof (header_p->data) / sizeof (uint16_t);

  ecma_length_t string_index = 0;
  while (cur_strcp_buf_iter_p != cur_strcp_buf_end_p
         && string_index < header_p->unit_number)
  {
    JERRY_ASSERT (cur_strcp_buf_iter_p < cur_strcp_buf_end_p);

    ecma_free_string (ECMA_GET_POINTER (*cur_strcp_buf_iter_p));

    cur_strcp_buf_iter_p++;
    string_index++;
  }

  ecma_collection_chunk_t *chunk_p = ECMA_GET_POINTER (header_p->next_chunk_cp);

  while (chunk_p != NULL)
  {
    JERRY_ASSERT (string_index < header_p->unit_number);

    cur_strcp_buf_iter_p = (uint16_t*) chunk_p->data;
    cur_strcp_buf_end_p = cur_strcp_buf_iter_p + sizeof (chunk_p->data) / sizeof (uint16_t);

    while (cur_strcp_buf_iter_p != cur_strcp_buf_end_p
           && string_index < header_p->unit_number)
    {
      JERRY_ASSERT (cur_strcp_buf_iter_p < cur_strcp_buf_end_p);

      ecma_free_string (ECMA_GET_POINTER (*cur_strcp_buf_iter_p));

      cur_strcp_buf_iter_p++;
      string_index++;
    }

    ecma_collection_chunk_t *next_chunk_p = ECMA_GET_POINTER (chunk_p->next_chunk_cp);
    ecma_dealloc_collection_chunk (chunk_p);
    chunk_p = next_chunk_p;
  }

  ecma_dealloc_collection_header (header_p);
} /* ecma_free_strings_collection */

/**
 * @}
 * @}
 */
