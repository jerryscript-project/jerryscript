/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
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
 * Allocator implementation
 */

#include "jcontext.h"
#include "jmem-allocator.h"
#include "jmem-heap.h"
#include "jmem-poolman.h"
#include "jrt-libc-includes.h"

#define JMEM_ALLOCATOR_INTERNAL
#include "jmem-allocator-internal.h"

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
uintptr_t
jmem_compress_pointer (const void *pointer_p) /**< pointer to compress */
{
  JERRY_ASSERT (jmem_is_heap_pointer (pointer_p));

  return jmem_heap_compress_pointer (pointer_p);
} /* jmem_compress_pointer */

/**
 * Decompress pointer
 *
 * @return unpacked pointer
 */
void *
jmem_decompress_pointer (uintptr_t compressed_pointer) /**< pointer to decompress */
{
  return jmem_heap_decompress_pointer (compressed_pointer);
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
