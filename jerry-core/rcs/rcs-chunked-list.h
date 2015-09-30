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

#ifndef RCS_CHUNKED_LIST_H
#define RCS_CHUNKED_LIST_H

#include "mem-allocator.h"

/** \addtogroup recordset Recordset
 * @{
 *
 * \addtogroup chunkedlist Chunked list
 *
 * List of nodes with size exactly fit to one memory heap's chunk.
 *
 * @{
 */

/**
 * Chunked list
 *
 * Note:
 *      Each node exactly fits size of one memory heap's chunk
 */
class rcs_chunked_list_t
{
public:
  /**
   * Constructor of chunked list
   */
  rcs_chunked_list_t ()
  {
    head_p = tail_p = NULL;
  } /* rcs_chunked_list_t */

  /**
   * List node
   */
  typedef struct
  {
    mem_cpointer_t prev_cp; /**< prev list's node */
    mem_cpointer_t next_cp; /**< next list's node */
  } node_t;

  void init (void);
  void cleanup (void);
  void free (void);

  node_t *get_first (void) const;
  node_t *get_last (void) const;

  node_t *get_prev (node_t *) const;
  node_t *get_next (node_t *) const;

  node_t *append_new (void);
  node_t *insert_new (node_t *);

  void remove (node_t *);

  node_t *get_node_from_pointer (void *) const;
  uint8_t *get_node_data_space (node_t *) const;

  static size_t get_node_data_space_size (void);

private:
  void set_prev (node_t *, node_t *);
  void set_next (node_t *, node_t *);

  static size_t get_node_size (void);

  void assert_list_is_correct (void) const;
  void assert_node_is_correct (const node_t *) const;

  node_t *head_p; /**< head node of list */
  node_t *tail_p; /**< tail node of list */
};

/**
 * @}
 * @}
 */

#endif /* RCS_CHUNKED_LIST_H */
