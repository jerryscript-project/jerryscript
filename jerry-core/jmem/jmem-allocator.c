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
 * Allocator implementation
 */

#include "jcontext.h"
#include "jmem-allocator.h"
#include "jmem-heap.h"
#include "jmem-poolman.h"
#include "jrt-libc-includes.h"

#define JMEM_ALLOCATOR_INTERNAL
#include "jmem-allocator-internal.h"

#ifdef JERRY_CPOINTER_32_BIT

/* This check will go away when we will support 64 bit compressed pointers. */
JERRY_STATIC_ASSERT (sizeof (uintptr_t) <= sizeof (jmem_cpointer_t),
                     size_of_uintpt_t_must_be_equal_to_jmem_cpointer_t);

#endif

/**
 * Initialize memory allocators.
 */
void
jmem_init (void)
{
  jmem_heap_init ();
} /* jmem_init */

/**
 * Finalize memory allocators.
 */
void
jmem_finalize ()
{
  jmem_pools_finalize ();

#ifdef JMEM_STATS
  if (JERRY_CONTEXT (jerry_init_flags) & JERRY_INIT_MEM_STATS)
  {
    jmem_stats_print ();
  }
#endif /* JMEM_STATS */

  jmem_heap_finalize ();
} /* jmem_finalize */

/**
 * Compress pointer
 *
 * @return packed pointer
 */
inline jmem_cpointer_t __attr_always_inline___
jmem_compress_pointer (const void *pointer_p) /**< pointer to compress */
{
  JERRY_ASSERT (pointer_p != NULL);
  JERRY_ASSERT (jmem_is_heap_pointer (pointer_p));

  uintptr_t uint_ptr = (uintptr_t) pointer_p;

  JERRY_ASSERT (uint_ptr % JMEM_ALIGNMENT == 0);

#ifdef JERRY_CPOINTER_32_BIT
  JERRY_ASSERT (((jmem_cpointer_t) uint_ptr) == uint_ptr);
#else /* !JERRY_CPOINTER_32_BIT */
  const uintptr_t heap_start = (uintptr_t) &JERRY_HEAP_CONTEXT (first);

  uint_ptr -= heap_start;
  uint_ptr >>= JMEM_ALIGNMENT_LOG;

  JERRY_ASSERT (uint_ptr <= UINT16_MAX);
  JERRY_ASSERT (uint_ptr != JMEM_CP_NULL);
#endif /* JERRY_CPOINTER_32_BIT */

  return (jmem_cpointer_t) uint_ptr;
} /* jmem_compress_pointer */

/**
 * Decompress pointer
 *
 * @return unpacked pointer
 */
inline void * __attr_always_inline___
jmem_decompress_pointer (uintptr_t compressed_pointer) /**< pointer to decompress */
{
  JERRY_ASSERT (compressed_pointer != JMEM_CP_NULL);

  uintptr_t uint_ptr = compressed_pointer;

  JERRY_ASSERT (((jmem_cpointer_t) uint_ptr) == uint_ptr);

#ifdef JERRY_CPOINTER_32_BIT
  JERRY_ASSERT (uint_ptr % JMEM_ALIGNMENT == 0);
#else /* !JERRY_CPOINTER_32_BIT */
  const uintptr_t heap_start = (uintptr_t) &JERRY_HEAP_CONTEXT (first);

  uint_ptr <<= JMEM_ALIGNMENT_LOG;
  uint_ptr += heap_start;

  JERRY_ASSERT (jmem_is_heap_pointer ((void *) uint_ptr));
#endif /* JERRY_CPOINTER_32_BIT */

  return (void *) uint_ptr;
} /* jmem_decompress_pointer */

/**
 * Register specified 'try to give memory back' callback routine
 */
void
jmem_register_free_unused_memory_callback (jmem_free_unused_memory_callback_t callback) /**< callback routine */
{
  /* Currently only one callback is supported */
  JERRY_ASSERT (JERRY_CONTEXT (jmem_free_unused_memory_callback) == NULL);

  JERRY_CONTEXT (jmem_free_unused_memory_callback) = callback;
} /* jmem_register_free_unused_memory_callback */

/**
 * Unregister specified 'try to give memory back' callback routine
 */
void
jmem_unregister_free_unused_memory_callback (jmem_free_unused_memory_callback_t callback) /**< callback routine */
{
  /* Currently only one callback is supported */
  JERRY_ASSERT (JERRY_CONTEXT (jmem_free_unused_memory_callback) == callback);

  JERRY_CONTEXT (jmem_free_unused_memory_callback) = NULL;
} /* jmem_unregister_free_unused_memory_callback */

/**
 * Run 'try to give memory back' callbacks with specified severity
 */
void
jmem_run_free_unused_memory_callbacks (jmem_free_unused_memory_severity_t severity) /**< severity of the request */
{
  if (JERRY_CONTEXT (jmem_free_unused_memory_callback) != NULL)
  {
    JERRY_CONTEXT (jmem_free_unused_memory_callback) (severity);
  }

  jmem_pools_collect_empty ();
} /* jmem_run_free_unused_memory_callbacks */

#ifdef JMEM_STATS
/**
 * Reset peak values in memory usage statistics
 */
void
jmem_stats_reset_peak (void)
{
  jmem_heap_stats_reset_peak ();
  jmem_pools_stats_reset_peak ();
} /* jmem_stats_reset_peak */

/**
 * Print memory usage statistics
 */
void
jmem_stats_print (void)
{
  jmem_heap_stats_print ();
  jmem_pools_stats_print ();
} /* jmem_stats_print */
#endif /* JMEM_STATS */
