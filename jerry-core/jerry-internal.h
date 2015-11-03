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

#ifndef JERRY_INTERNAL
 # error "The header is for Jerry's internal interfaces"
#endif

#ifndef JERRY_INTERNAL_H
#define JERRY_INTERNAL_H

#include "ecma-globals.h"
#include "jerry-api.h"

extern ecma_completion_value_t
jerry_dispatch_external_function (ecma_object_t *, ecma_external_pointer_t, ecma_value_t, ecma_collection_header_t *);

extern void
jerry_dispatch_object_free_callback (ecma_external_pointer_t, ecma_external_pointer_t);

extern bool
jerry_is_abort_on_fail (void);

/**
 * Snapshot header
 */
typedef struct
{
  uint32_t lit_table_size; /**< size of literal table */
  uint32_t scopes_num; /**< number of saved bytecode pieces in the snapshot */
  uint32_t is_run_global : 1; /**< flag, indicating whether the snapshot
                               *   was dumped as 'Global scope'-mode code (true)
                               *   or as eval-mode code (false) */
} jerry_snapshot_header_t;

/**
 * Jerry snapshot format version
 */
#define JERRY_SNAPSHOT_VERSION (2u)

#endif /* !JERRY_INTERNAL_H */
