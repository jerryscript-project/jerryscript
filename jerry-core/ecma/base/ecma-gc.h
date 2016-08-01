/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
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

#ifndef ECMA_GC_H
#define ECMA_GC_H

#include "ecma-globals.h"
#include "jmem-allocator.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmagc Garbage collector
 * @{
 */

extern void ecma_init_gc_info (ecma_object_t *);
extern void ecma_ref_object (ecma_object_t *);
extern void ecma_deref_object (ecma_object_t *);
extern void ecma_gc_run (jmem_free_unused_memory_severity_t);
extern void ecma_free_unused_memory (jmem_free_unused_memory_severity_t);

/**
 * @}
 * @}
 */

#endif /* !ECMA_GC_H */
