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
 * Memory pool manager implementation
 */

#include "jcontext.h"
#include "jmem-allocator.h"
#include "jmem-heap.h"
#include "jmem-poolman.h"
#include "jrt-libc-includes.h"

#define JMEM_ALLOCATOR_INTERNAL
#include "jmem-allocator-internal.h"

/** \addtogroup mem Memory allocation
 * @{
 *
 * \addtogroup poolman Memory pool manager
 * @{
 */

#ifdef JMEM_STATS

static void jmem_pools_stat_free_pool (void);
static void jmem_pools_stat_new_alloc (void);
static void jmem_pools_stat_reuse (void);
static void jmem_pools_stat_dealloc (void);

#  define JMEM_POOLS_STAT_FREE_POOL() jmem_pools_stat_free_pool ()
#  define JMEM_POOLS_STAT_NEW_ALLOC() jmem_pools_stat_new_alloc ()
#  define JMEM_POOLS_STAT_REUSE() jmem_pools_stat_reuse ()
#  define JMEM_POOLS_STAT_DEALLOC() jmem_pools_stat_dealloc ()
#else /* !JMEM_STATS */
#  define JMEM_POOLS_STAT_FREE_POOL()
#  define JMEM_POOLS_STAT_NEW_ALLOC()
#  define JMEM_POOLS_STAT_REUSE()
#  define JMEM_POOLS_STAT_DEALLOC()
#endif /* JMEM_STATS */

/*
 * Valgrind-related options and headers
 */
#ifdef JERRY_VALGRIND
# include "memcheck.h"

# define VALGRIND_NOACCESS_SPACE(p, s)   VALGRIND_MAKE_MEM_NOACCESS((p), (s))
# define VALGRIND_UNDEFINED_SPACE(p, s)  VALGRIND_MAKE_MEM_UNDEFINED((p), (s))
# define VALGRIND_DEFINED_SPACE(p, s)    VALGRIND_MAKE_MEM_DEFINED((p), (s))
#else /* !JERRY_VALGRIND */
# define VALGRIND_NOACCESS_SPACE(p, s)
# define VALGRIND_UNDEFINED_SPACE(p, s)
# define VALGRIND_DEFINED_SPACE(p, s)
#endif /* JERRY_VALGRIND */

#ifdef JERRY_VALGRIND_FREYA
# include "memcheck.h"

# define VALGRIND_FREYA_MALLOCLIKE_SPACE(p, s) VALGRIND_MALLOCLIKE_BLOCK((p), (s), 0, 0)
# define VALGRIND_FREYA_FREELIKE_SPACE(p)      VALGRIND_FREELIKE_BLOCK((p), 0)
#else /* !JERRY_VALGRIND_FREYA */
# define VALGRIND_FREYA_MALLOCLIKE_SPACE(p, s)
# define VALGRIND_FREYA_FREELIKE_SPACE(p)
#endif /* JERRY_VALGRIND_FREYA */

/**
 * Finalize pool manager
 */
void
jmem_pools_finalize (void)
{
  jmem_pools_collect_empty ();

  JERRY_ASSERT (JERRY_CONTEXT (jmem_free_8_byte_chunk_p) == NULL);
#ifdef JERRY_CPOINTER_32_BIT
  JERRY_ASSERT (JERRY_CONTEXT (jmem_free_16_byte_chunk_p) == NULL);
#endif /* JERRY_CPOINTER_32_BIT */
} /* jmem_pools_finalize */

/**
 * Allocate a chunk of specified size
 *
 * @return pointer to allocated chunk, if allocation was successful,
 *         or NULL - if not enough memory.
 */
inline void * __attr_hot___ __attr_always_inline___
jmem_pools_alloc (size_t size) /**< size of the chunk */
{
#ifdef JMEM_GC_BEFORE_EACH_ALLOC
  jmem_run_free_unused_memory_callbacks (JMEM_FREE_UNUSED_MEMORY_SEVERITY_HIGH);
#endif /* JMEM_GC_BEFORE_EACH_ALLOC */

  if (size <= 8)
  {
    if (JERRY_CONTEXT (jmem_free_8_byte_chunk_p) != NULL)
    {
      const jmem_pools_chunk_t *const chunk_p = JERRY_CONTEXT (jmem_free_8_byte_chunk_p);

      JMEM_POOLS_STAT_REUSE ();

      VALGRIND_DEFINED_SPACE (chunk_p, sizeof (jmem_pools_chunk_t));

      JERRY_CONTEXT (jmem_free_8_byte_chunk_p) = chunk_p->next_p;

      VALGRIND_UNDEFINED_SPACE (chunk_p, sizeof (jmem_pools_chunk_t));

      return (void *) chunk_p;
    }
    else
    {
      JMEM_POOLS_STAT_NEW_ALLOC ();
      return (void *) jmem_heap_alloc_block (8);
    }
  }

#ifdef JERRY_CPOINTER_32_BIT
  JERRY_ASSERT (size <= 16);

  if (JERRY_CONTEXT (jmem_free_16_byte_chunk_p) != NULL)
  {
    const jmem_pools_chunk_t *const chunk_p = JERRY_CONTEXT (jmem_free_16_byte_chunk_p);

    JMEM_POOLS_STAT_REUSE ();

    VALGRIND_DEFINED_SPACE (chunk_p, sizeof (jmem_pools_chunk_t));

    JERRY_CONTEXT (jmem_free_16_byte_chunk_p) = chunk_p->next_p;

    VALGRIND_UNDEFINED_SPACE (chunk_p, sizeof (jmem_pools_chunk_t));

    return (void *) chunk_p;
  }
  else
  {
    JMEM_POOLS_STAT_NEW_ALLOC ();
    return (void *) jmem_heap_alloc_block (16);
  }
#else /* !JERRY_CPOINTER_32_BIT */
  JERRY_UNREACHABLE ();
  return NULL;
#endif
} /* jmem_pools_alloc */

/**
 * Free the chunk
 */
inline void __attr_hot___ __attr_always_inline___
jmem_pools_free (void *chunk_p, /**< pointer to the chunk */
                 size_t size) /**< size of the chunk */
{
  JERRY_ASSERT (chunk_p != NULL);

  jmem_pools_chunk_t *const chunk_to_free_p = (jmem_pools_chunk_t *) chunk_p;

  VALGRIND_DEFINED_SPACE (chunk_to_free_p, size);

  if (size <= 8)
  {
    chunk_to_free_p->next_p = JERRY_CONTEXT (jmem_free_8_byte_chunk_p);
    JERRY_CONTEXT (jmem_free_8_byte_chunk_p) = chunk_to_free_p;
  }
  else
  {
#ifdef JERRY_CPOINTER_32_BIT
    JERRY_ASSERT (size <= 16);

    chunk_to_free_p->next_p = JERRY_CONTEXT (jmem_free_16_byte_chunk_p);
    JERRY_CONTEXT (jmem_free_16_byte_chunk_p) = chunk_to_free_p;
#else /* !JERRY_CPOINTER_32_BIT */
    JERRY_UNREACHABLE ();
#endif /* JERRY_CPOINTER_32_BIT */
  }

  VALGRIND_NOACCESS_SPACE (chunk_to_free_p, size);

  JMEM_POOLS_STAT_FREE_POOL ();
} /* jmem_pools_free */

/**
 *  Collect empty pool chunks
 */
void
jmem_pools_collect_empty ()
{
  jmem_pools_chunk_t *chunk_p = JERRY_CONTEXT (jmem_free_8_byte_chunk_p);
  JERRY_CONTEXT (jmem_free_8_byte_chunk_p) = NULL;

  while (chunk_p)
  {
    VALGRIND_DEFINED_SPACE (chunk_p, sizeof (jmem_pools_chunk_t));
    jmem_pools_chunk_t *const next_p = chunk_p->next_p;
    VALGRIND_NOACCESS_SPACE (chunk_p, sizeof (jmem_pools_chunk_t));

    jmem_heap_free_block (chunk_p, 8);
    JMEM_POOLS_STAT_DEALLOC ();
    chunk_p = next_p;
  }

#ifdef JERRY_CPOINTER_32_BIT
  chunk_p = JERRY_CONTEXT (jmem_free_16_byte_chunk_p);
  JERRY_CONTEXT (jmem_free_16_byte_chunk_p) = NULL;

  while (chunk_p)
  {
    VALGRIND_DEFINED_SPACE (chunk_p, sizeof (jmem_pools_chunk_t));
    jmem_pools_chunk_t *const next_p = chunk_p->next_p;
    VALGRIND_NOACCESS_SPACE (chunk_p, sizeof (jmem_pools_chunk_t));

    jmem_heap_free_block (chunk_p, 16);
    JMEM_POOLS_STAT_DEALLOC ();
    chunk_p = next_p;
  }
#endif /* JERRY_CPOINTER_32_BIT */
} /* jmem_pools_collect_empty */

#ifdef JMEM_STATS
/**
 * Get pools memory usage statistics
 */
void
jmem_pools_get_stats (jmem_pools_stats_t *out_pools_stats_p) /**< [out] pools' stats */
{
  JERRY_ASSERT (out_pools_stats_p != NULL);

  *out_pools_stats_p = JERRY_CONTEXT (jmem_pools_stats);
} /* jmem_pools_get_stats */

/**
 * Reset peak values in memory usage statistics
 */
void
jmem_pools_stats_reset_peak (void)
{
  JERRY_CONTEXT (jmem_pools_stats).peak_pools_count = JERRY_CONTEXT (jmem_pools_stats.pools_count);
} /* jmem_pools_stats_reset_peak */

/**
 * Print pools memory usage statistics
 */
void
jmem_pools_stats_print (void)
{
  jmem_pools_stats_t *pools_stats = &JERRY_CONTEXT (jmem_pools_stats);

  JERRY_DEBUG_MSG ("Pools stats:\n"
                   "  Pool chunks: %zu\n"
                   "  Peak pool chunks: %zu\n"
                   "  Free chunks: %zu\n"
                   "  Pool reuse ratio: %zu.%04zu\n",
                   pools_stats->pools_count,
                   pools_stats->peak_pools_count,
                   pools_stats->free_chunks,
                   pools_stats->reused_count / pools_stats->new_alloc_count,
                   pools_stats->reused_count % pools_stats->new_alloc_count * 10000 / pools_stats->new_alloc_count);
} /* jmem_pools_stats_print */

/**
 * Account for allocation of new pool chunk
 */
static void
jmem_pools_stat_new_alloc (void)
{
  jmem_pools_stats_t *pools_stats = &JERRY_CONTEXT (jmem_pools_stats);

  pools_stats->pools_count++;
  pools_stats->new_alloc_count++;

  if (pools_stats->pools_count > pools_stats->peak_pools_count)
  {
    pools_stats->peak_pools_count = pools_stats->pools_count;
  }
  if (pools_stats->pools_count > pools_stats->global_peak_pools_count)
  {
    pools_stats->global_peak_pools_count = pools_stats->pools_count;
  }
} /* jmem_pools_stat_new_alloc */


/**
 * Account for reuse of pool chunk
 */
static void
jmem_pools_stat_reuse (void)
{
  jmem_pools_stats_t *pools_stats = &JERRY_CONTEXT (jmem_pools_stats);

  pools_stats->pools_count++;
  pools_stats->free_chunks--;
  pools_stats->reused_count++;

  if (pools_stats->pools_count > pools_stats->peak_pools_count)
  {
    pools_stats->peak_pools_count = pools_stats->pools_count;
  }
  if (pools_stats->pools_count > pools_stats->global_peak_pools_count)
  {
    pools_stats->global_peak_pools_count = pools_stats->pools_count;
  }
} /* jmem_pools_stat_reuse */


/**
 * Account for freeing a chunk
 */
static void
jmem_pools_stat_free_pool (void)
{
  jmem_pools_stats_t *pools_stats = &JERRY_CONTEXT (jmem_pools_stats);

  JERRY_ASSERT (pools_stats->pools_count > 0);

  pools_stats->pools_count--;
  pools_stats->free_chunks++;
} /* jmem_pools_stat_free_pool */

/**
 * Account for freeing a chunk
 */
static void
jmem_pools_stat_dealloc (void)
{
  JERRY_CONTEXT (jmem_pools_stats).free_chunks--;
} /* jmem_pools_stat_dealloc */
#endif /* JMEM_STATS */

/**
 * @}
 * @}
 */
