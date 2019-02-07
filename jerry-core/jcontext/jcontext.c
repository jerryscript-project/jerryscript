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

#include "jcontext.h"

/** \addtogroup context Context
 * @{
 */

#ifndef JERRY_ENABLE_EXTERNAL_CONTEXT

/**
 * Global context.
 */
jerry_context_t jerry_global_context;

#ifndef JERRY_SYSTEM_ALLOCATOR

/**
 * Check size of heap is corresponding to configuration
 */
JERRY_STATIC_ASSERT (sizeof (jmem_heap_t) <= JMEM_HEAP_SIZE,
                     size_of_mem_heap_must_be_less_than_or_equal_to_JMEM_HEAP_SIZE);

/**
 * Jerry global heap section attribute.
 */
#ifndef JERRY_HEAP_SECTION_ATTR
#define JERRY_GLOBAL_HEAP_SECTION
#else /* JERRY_HEAP_SECTION_ATTR */
#define JERRY_GLOBAL_HEAP_SECTION JERRY_ATTR_SECTION (JERRY_HEAP_SECTION_ATTR)
#endif /* !JERRY_HEAP_SECTION_ATTR */

/**
 * Global heap.
 */
jmem_heap_t jerry_global_heap JERRY_ATTR_ALIGNED (JMEM_ALIGNMENT) JERRY_GLOBAL_HEAP_SECTION;

#endif /* !JERRY_SYSTEM_ALLOCATOR */

#endif /* !JERRY_ENABLE_EXTERNAL_CONTEXT */

/**
 * @}
 */
