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

#ifndef RCS_ITERATOR_H
#define RCS_ITERATOR_H

#include "ecma-globals.h"

/**
 * Represents a context for the iterator.
 */
typedef struct
{
  rcs_record_set_t *recordset_p; /**< recordset, containing the records */
  rcs_record_t *record_start_p; /**< start of current record */
  uint8_t *current_pos_p; /**< pointer to current offset in current record */
  size_t current_offset; /**< current offset */
} rcs_iterator_t;

extern rcs_iterator_t rcs_iterator_create (rcs_record_set_t *, rcs_record_t *);

extern void rcs_iterator_write (rcs_iterator_t *, void *, size_t);
extern void rcs_iterator_read (rcs_iterator_t *, void *, size_t);
extern void rcs_iterator_skip (rcs_iterator_t *, size_t);

extern void rcs_iterator_reset (rcs_iterator_t *);
extern bool rcs_iterator_finished (rcs_iterator_t *);

#endif /* !RCS_ITERATOR_H */
