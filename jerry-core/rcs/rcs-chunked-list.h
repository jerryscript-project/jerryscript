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
  * List node
  */
typedef struct
{
  mem_cpointer_t prev_cp; /**< prev list's node */
  mem_cpointer_t next_cp; /**< next list's node */
} rcs_chunked_list_node_t;

/**
 * Chunked list
 */
typedef struct
{
  rcs_chunked_list_node_t *head_p; /**< head node of list */
  rcs_chunked_list_node_t *tail_p; /**< tail node of list */
} rcs_chunked_list_t;

extern void rcs_chunked_list_init (rcs_chunked_list_t *);
extern void rcs_chunked_list_free (rcs_chunked_list_t *);
extern void rcs_chunked_list_cleanup (rcs_chunked_list_t *);
extern rcs_chunked_list_node_t *rcs_chunked_list_get_first (rcs_chunked_list_t *);
extern rcs_chunked_list_node_t *rcs_chunked_list_get_last (rcs_chunked_list_t *);
extern rcs_chunked_list_node_t *rcs_chunked_list_get_prev (rcs_chunked_list_node_t *);
extern rcs_chunked_list_node_t *rcs_chunked_list_get_next (rcs_chunked_list_node_t *);
extern rcs_chunked_list_node_t *rcs_chunked_list_append_new (rcs_chunked_list_t *);
extern rcs_chunked_list_node_t *rcs_chunked_list_insert_new (rcs_chunked_list_t *, rcs_chunked_list_node_t *);
extern void rcs_chunked_list_remove (rcs_chunked_list_t *, rcs_chunked_list_node_t *);
extern rcs_chunked_list_node_t *rcs_chunked_list_get_node_from_pointer (rcs_chunked_list_t *, void *);
extern uint8_t *rcs_chunked_list_get_node_data_space (rcs_chunked_list_t *, rcs_chunked_list_node_t *);
extern size_t rcs_chunked_list_get_node_data_space_size (void);


#endif /* RCS_CHUNKED_LIST_H */
