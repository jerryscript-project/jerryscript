/* Copyright JS Foundation and other contributors, http://js.foundation
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
#include "jmem.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmagc Garbage collector
 * @{
 */

void ecma_init_gc_info (ecma_object_t *object_p);
void ecma_ref_object (ecma_object_t *object_p);
void ecma_deref_object (ecma_object_t *object_p);
void ecma_gc_run (jmem_free_unused_memory_severity_t severity);
void ecma_free_unused_memory (jmem_free_unused_memory_severity_t severity);

/**
 * @}
 * @}
 */

#endif /* !ECMA_GC_H */
