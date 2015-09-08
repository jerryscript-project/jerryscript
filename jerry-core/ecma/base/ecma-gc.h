/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmagc Garbage collector
 * @{
 */

#ifndef ECMA_GC_H
#define ECMA_GC_H

#include "ecma-globals.h"
#include "mem-allocator.h"

extern void ecma_gc_init (void);
extern void ecma_init_gc_info (ecma_object_t *);
extern void ecma_ref_object (ecma_object_t *);
extern void ecma_deref_object (ecma_object_t *);
extern void ecma_gc_run (void);
extern void ecma_try_to_give_back_some_memory (mem_try_give_memory_back_severity_t);

#endif /* !ECMA_GC_H */

/**
 * @}
 * @}
 */
