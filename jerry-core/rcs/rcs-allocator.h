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

#ifndef RCS_ALLOCATOR_H
#define RCS_ALLOCATOR_H

#include "rcs-globals.h"

extern void rcs_check_record_alignment (rcs_record_t *);
extern void rcs_free_record (rcs_record_set_t *, rcs_record_t *);
extern size_t rcs_get_node_data_space_size (void);
extern uint8_t *rcs_get_node_data_space (rcs_record_set_t *, rcs_chunked_list_t::node_t *);
extern rcs_record_t *rcs_alloc_record (rcs_record_set_t *, rcs_record_type_t, size_t);

#endif /* !RCS_ALLOCATOR_H */
