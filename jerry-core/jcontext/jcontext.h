/* Copyright 2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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

/**
 * Memory context for JerryScript
 */
#ifndef JCONTEXT_H
#define JCONTEXT_H

#include "jrt.h"
#include "jmem-allocator.h"
#include "jmem-config.h"

/** \addtogroup context Jerry context
 * @{
 *
 * \addtogroup context Context
 * @{
 */

/**
 * Calculate heap area size, leaving space for a pointer to the free list
 */
#define JMEM_HEAP_AREA_SIZE (JMEM_HEAP_SIZE - JMEM_ALIGNMENT)

/**
 * Heap structure
 *
 * Memory blocks returned by the allocator must not start from the
 * beginning of the heap area because offset 0 is reserved for
 * JMEM_CP_NULL. This special constant is used in several places,
 * e.g. it marks the end of the property chain list, so it cannot
 * be eliminated from the project. Although the allocator cannot
 * use the first 8 bytes of the heap, nothing prevents to use it
 * for other purposes. Currently the free region start is stored
 * there.
 */
typedef struct
{
  jmem_heap_free_t first; /**< first node in free region list */
  uint8_t area[JMEM_HEAP_AREA_SIZE]; /**< heap area */
} jmem_heap_t;

/**
 * JerryScript context
 *
 * The purpose of this header is storing
 * all global variables for Jerry
 */
typedef struct
{
  /**
   * Memory manager part.
   */
  size_t jmem_heap_allocated_size; /**< size of allocated regions */
  size_t jmem_heap_limit; /**< current limit of heap usage, that is upon being reached,
                           *   causes call of "try give memory back" callbacks */
  jmem_heap_free_t *jmem_heap_list_skip_p; /**< This is used to speed up deallocation. */
  jmem_pools_chunk_t *jmem_free_chunk_p; /**< list of free pool chunks */
  jmem_free_unused_memory_callback_t jmem_free_unused_memory_callback; /**< Callback for freeing up memory. */

#ifdef JMEM_STATS
  jmem_heap_stats_t jmem_heap_stats; /**< heap's memory usage statistics */
  jmem_pools_stats_t jmem_pools_stats; /**< pools' memory usage statistics */
#endif /* MEM_STATS */

#ifdef JERRY_VALGRIND_FREYA
  bool valgrind_freya_mempool_request; /**< Tells whether a pool manager
                                        *   allocator request is in progress */
#endif /* JERRY_VALGRIND_FREYA */
} jerry_context_t;

/**
 * Jerry global context.
 */
extern jerry_context_t jerry_global_context;

/**
 * Jerry global heap.
 */
extern jmem_heap_t jerry_global_heap;

/**
 * Provides a reference to a field in the current context.
 */
#define JERRY_CONTEXT(field) (jerry_global_context.field)

/**
 * Provides a reference to the area field of the heap.
 */
#define JERRY_HEAP_CONTEXT(field) (jerry_global_heap.field)

/**
 * @}
 * @}
 */

#endif /* !JCONTEXT_H */
