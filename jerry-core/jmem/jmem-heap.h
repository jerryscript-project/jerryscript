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

/**
 * Heap allocator interface
 */
#ifndef JMEM_HEAP_H
#define JMEM_HEAP_H

#include "jrt.h"

/** \addtogroup mem Memory allocation
 * @{
 *
 * \addtogroup heap Heap
 * @{
 */

extern void jmem_heap_init (void);
extern void jmem_heap_finalize (void);
extern void *jmem_heap_alloc_block (const size_t);
extern void *jmem_heap_alloc_block_null_on_error (const size_t);
extern void jmem_heap_free_block (void *, const size_t);
extern bool jmem_is_heap_pointer (const void *);

#ifdef JMEM_STATS
/**
 * Heap memory usage statistics
 */
typedef struct
{
  size_t size; /**< size */

  size_t allocated_bytes; /**< currently allocated bytes */
  size_t peak_allocated_bytes; /**< peak allocated bytes */
  size_t global_peak_allocated_bytes; /**< non-resettable peak allocated bytes */

  size_t waste_bytes; /**< bytes waste due to blocks filled partially
                           and due to block headers */
  size_t peak_waste_bytes; /**< peak bytes waste */
  size_t global_peak_waste_bytes; /**< non-resettable peak bytes waste */

  size_t skip_count;
  size_t nonskip_count;

  size_t alloc_count;
  size_t alloc_iter_count;

  size_t free_count;
  size_t free_iter_count;
} jmem_heap_stats_t;

extern void jmem_heap_get_stats (jmem_heap_stats_t *);
extern void jmem_heap_stats_reset_peak (void);
extern void jmem_heap_stats_print (void);
#endif /* JMEM_STATS */

#ifdef JERRY_VALGRIND_FREYA

#ifdef JERRY_VALGRIND
#error Valgrind and valgrind-freya modes are not compatible.
#endif /* JERRY_VALGRIND */

extern void jmem_heap_valgrind_freya_mempool_request (void);

#define JMEM_HEAP_VALGRIND_FREYA_MEMPOOL_REQUEST() jmem_heap_valgrind_freya_mempool_request ()

#else /* !JERRY_VALGRIND_FREYA */

#define JMEM_HEAP_VALGRIND_FREYA_MEMPOOL_REQUEST()

#endif /* JERRY_VALGRIND_FREYA */

/**
 * Define a local array variable and allocate memory for the array on the heap.
 *
 * If requested number of elements is zero, assign NULL to the variable.
 *
 * Warning:
 *         if there is not enough memory on the heap, shutdown engine with ERR_OUT_OF_MEMORY.
 */
#define JMEM_DEFINE_LOCAL_ARRAY(var_name, number, type) \
{ \
  size_t var_name ## ___size = (size_t) (number) * sizeof (type); \
  type *var_name = (type *) (jmem_heap_alloc_block (var_name ## ___size));

/**
 * Free the previously defined local array variable, freeing corresponding block on the heap,
 * if it was allocated (i.e. if the array's size was non-zero).
 */
#define JMEM_FINALIZE_LOCAL_ARRAY(var_name) \
  if (var_name != NULL) \
  { \
    JERRY_ASSERT (var_name ## ___size != 0); \
    \
    jmem_heap_free_block (var_name, var_name ## ___size); \
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

#endif /* !JMEM_HEAP_H */
