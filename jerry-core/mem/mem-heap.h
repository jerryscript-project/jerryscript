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

/** \addtogroup mem Memory allocation
 * @{
 *
 * \addtogroup heap Heap
 * @{
 */

/**
 * Heap allocator interface
 */
#ifndef JERRY_MEM_HEAP_H
#define JERRY_MEM_HEAP_H

#include "jrt.h"

/**
 * Type of allocation (argument of mem_Alloc)
 *
 * @see mem_heap_alloc_block
 */
typedef enum
{
  MEM_HEAP_ALLOC_SHORT_TERM, /**< allocated region will be freed soon */
  MEM_HEAP_ALLOC_LONG_TERM /**< allocated region most likely will not be freed soon */
} mem_heap_alloc_term_t;

extern void mem_heap_init (void);
extern void mem_heap_finalize (void);
extern void *mem_heap_alloc_block (size_t, mem_heap_alloc_term_t);
extern void *mem_heap_alloc_chunked_block (mem_heap_alloc_term_t);
extern void mem_heap_free_block (void *);
extern void *mem_heap_get_chunked_block_start (void *);
extern size_t mem_heap_get_chunked_block_data_size (void);
extern uintptr_t mem_heap_compress_pointer (const void *);
extern void *mem_heap_decompress_pointer (uintptr_t);
extern bool mem_is_heap_pointer (const void *);
extern size_t __attr_pure___ mem_heap_recommend_allocation_size (size_t);
extern void mem_heap_print (bool, bool, bool);

#ifdef MEM_STATS
/**
 * Heap memory usage statistics
 */
typedef struct
{
  size_t size; /**< size */
  size_t blocks; /**< blocks count */

  size_t allocated_chunks; /**< currently allocated chunks */
  size_t peak_allocated_chunks; /**< peak allocated chunks */
  size_t global_peak_allocated_chunks; /**< non-resettable peak allocated chunks */

  size_t allocated_blocks; /**< currently allocated blocks */
  size_t peak_allocated_blocks; /**< peak allocated blocks */
  size_t global_peak_allocated_blocks; /**< non-resettable peak allocated blocks */

  size_t allocated_bytes; /**< currently allocated bytes */
  size_t peak_allocated_bytes; /**< peak allocated bytes */
  size_t global_peak_allocated_bytes; /**< non-resettable peak allocated bytes */

  size_t waste_bytes; /**< bytes waste due to blocks filled partially
                           and due to block headers */
  size_t peak_waste_bytes; /**< peak bytes waste */
  size_t global_peak_waste_bytes; /**< non-resettable peak bytes waste */
} mem_heap_stats_t;

extern void mem_heap_get_stats (mem_heap_stats_t *);
extern void mem_heap_stats_reset_peak (void);
#endif /* MEM_STATS */

#ifdef JERRY_VALGRIND_FREYA

#ifdef JERRY_VALGRIND
#error Valgrind and valgrind-freya modes are not compatible.
#endif

extern void mem_heap_valgrind_freya_mempool_request (void);

#define MEM_HEAP_VALGRIND_FREYA_MEMPOOL_REQUEST() mem_heap_valgrind_freya_mempool_request ()

#else /* JERRY_VALGRIND_FREYA */

#define MEM_HEAP_VALGRIND_FREYA_MEMPOOL_REQUEST()

#endif /* JERRY_VALGRIND_FREYA */

/**
 * Define a local array variable and allocate memory for the array on the heap.
 *
 * If requested number of elements is zero, assign NULL to the variable.
 *
 * Warning:
 *         if there is not enough memory on the heap, shutdown engine with ERR_OUT_OF_MEMORY.
 */
#define MEM_DEFINE_LOCAL_ARRAY(var_name, number, type) \
{ \
  size_t var_name ## ___size = (size_t) (number) * sizeof (type); \
  type *var_name = static_cast <type *> (mem_heap_alloc_block (var_name ## ___size, MEM_HEAP_ALLOC_SHORT_TERM));

/**
 * Free the previously defined local array variable, freeing corresponding block on the heap,
 * if it was allocated (i.e. if the array's size was non-zero).
 */
#define MEM_FINALIZE_LOCAL_ARRAY(var_name) \
  if (var_name != NULL) \
  { \
    JERRY_ASSERT (var_name ## ___size != 0); \
    \
    mem_heap_free_block (var_name); \
  } \
  else \
  { \
    JERRY_ASSERT (var_name ## ___size == 0); \
  } \
}

/**
 * @}
 * @}
 */

#endif /* !JERRY_MEM_HEAP_H */
