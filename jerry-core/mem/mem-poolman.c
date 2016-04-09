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

#include "jrt.h"
#include "jrt-libc-includes.h"
#include "mem-allocator.h"
#include "mem-heap.h"
#include "mem-poolman.h"

#define MEM_ALLOCATOR_INTERNAL
#include "mem-allocator-internal.h"

/** \addtogroup mem Memory allocation
 * @{
 *
 * \addtogroup poolman Memory pool manager
 * @{
 */

/**
 * Node for free chunk list
 */
typedef struct mem_pools_chunk
{
  struct mem_pools_chunk *next_p; /* pointer to next pool chunk */
} mem_pools_chunk_t;

/**
 * List of free pool chunks
 */
mem_pools_chunk_t *mem_free_chunk_p;

#ifdef MEM_STATS

/**
 * Pools' memory usage statistics
 */
mem_pools_stats_t mem_pools_stats;

static void mem_pools_stat_init (void);
static void mem_pools_stat_free_pool (void);
static void mem_pools_stat_new_alloc (void);
static void mem_pools_stat_reuse (void);
static void mem_pools_stat_dealloc (void);

#  define MEM_POOLS_STAT_INIT() mem_pools_stat_init ()
#  define MEM_POOLS_STAT_FREE_POOL() mem_pools_stat_free_pool ()
#  define MEM_POOLS_STAT_NEW_ALLOC() mem_pools_stat_new_alloc ()
#  define MEM_POOLS_STAT_REUSE() mem_pools_stat_reuse ()
#  define MEM_POOLS_STAT_DEALLOC() mem_pools_stat_dealloc ()
#else /* !MEM_STATS */
#  define MEM_POOLS_STAT_INIT()
#  define MEM_POOLS_STAT_FREE_POOL()
#  define MEM_POOLS_STAT_NEW_ALLOC()
#  define MEM_POOLS_STAT_REUSE()
#  define MEM_POOLS_STAT_DEALLOC()
#endif /* !MEM_STATS */

/*
 * Valgrind-related options and headers
 */
#ifdef JERRY_VALGRIND
# include "memcheck.h"

# define VALGRIND_NOACCESS_SPACE(p, s)   VALGRIND_MAKE_MEM_NOACCESS((p), (s))
# define VALGRIND_UNDEFINED_SPACE(p, s)  VALGRIND_MAKE_MEM_UNDEFINED((p), (s))
# define VALGRIND_DEFINED_SPACE(p, s)    VALGRIND_MAKE_MEM_DEFINED((p), (s))
#else /* JERRY_VALGRIND */
# define VALGRIND_NOACCESS_SPACE(p, s)
# define VALGRIND_UNDEFINED_SPACE(p, s)
# define VALGRIND_DEFINED_SPACE(p, s)
#endif /* JERRY_VALGRIND */

#ifdef JERRY_VALGRIND_FREYA
# include "memcheck.h"

# define VALGRIND_FREYA_MALLOCLIKE_SPACE(p, s) VALGRIND_MALLOCLIKE_BLOCK((p), (s), 0, 0)
# define VALGRIND_FREYA_FREELIKE_SPACE(p)      VALGRIND_FREELIKE_BLOCK((p), 0)
#else /* JERRY_VALGRIND_FREYA */
# define VALGRIND_FREYA_MALLOCLIKE_SPACE(p, s)
# define VALGRIND_FREYA_FREELIKE_SPACE(p)
#endif /* JERRY_VALGRIND_FREYA */

/**
 * Initialize pool manager
 */
void
mem_pools_init (void)
{
  JERRY_STATIC_ASSERT (sizeof (mem_pools_chunk_t) <= MEM_POOL_CHUNK_SIZE,
                       size_of_mem_pools_chunk_t_must_be_less_than_or_equal_to_MEM_POOL_CHUNK_SIZE);

  mem_free_chunk_p = NULL;

  MEM_POOLS_STAT_INIT ();
} /* mem_pools_init */

/**
 * Finalize pool manager
 */
void
mem_pools_finalize (void)
{
  mem_pools_collect_empty ();

  JERRY_ASSERT (mem_free_chunk_p == NULL);
} /* mem_pools_finalize */

/**
 * Allocate a chunk of specified size
 *
 * @return pointer to allocated chunk, if allocation was successful,
 *         or NULL - if not enough memory.
 */
inline void * __attribute__((hot)) __attr_always_inline___
mem_pools_alloc (void)
{
#ifdef MEM_GC_BEFORE_EACH_ALLOC
  mem_run_try_to_give_memory_back_callbacks (MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_HIGH);
#endif /* MEM_GC_BEFORE_EACH_ALLOC */

  if (mem_free_chunk_p != NULL)
  {
    const mem_pools_chunk_t *const chunk_p = mem_free_chunk_p;

    MEM_POOLS_STAT_REUSE ();

    VALGRIND_DEFINED_SPACE (chunk_p, MEM_POOL_CHUNK_SIZE);

    mem_free_chunk_p = chunk_p->next_p;

    VALGRIND_UNDEFINED_SPACE (chunk_p, MEM_POOL_CHUNK_SIZE);

    return (void *) chunk_p;
  }
  else
  {
    MEM_POOLS_STAT_NEW_ALLOC ();
    return (void *) mem_heap_alloc_block (MEM_POOL_CHUNK_SIZE);
  }
} /* mem_pools_alloc */

/**
 * Free the chunk
 */
void __attribute__((hot))
mem_pools_free (void *chunk_p) /**< pointer to the chunk */
{
  mem_pools_chunk_t *const chunk_to_free_p = (mem_pools_chunk_t *) chunk_p;

  VALGRIND_DEFINED_SPACE (chunk_to_free_p, MEM_POOL_CHUNK_SIZE);

  chunk_to_free_p->next_p = mem_free_chunk_p;
  mem_free_chunk_p = chunk_to_free_p;

  VALGRIND_NOACCESS_SPACE (chunk_to_free_p, MEM_POOL_CHUNK_SIZE);

  MEM_POOLS_STAT_FREE_POOL ();
} /* mem_pools_free */

/**
 *  Collect empty pool chunks
 */
void
mem_pools_collect_empty ()
{
  while (mem_free_chunk_p)
  {
    VALGRIND_DEFINED_SPACE (mem_free_chunk_p, sizeof (mem_pools_chunk_t));
    mem_pools_chunk_t *const next_p = mem_free_chunk_p->next_p;
    VALGRIND_NOACCESS_SPACE (mem_free_chunk_p, sizeof (mem_pools_chunk_t));

    mem_heap_free_block (mem_free_chunk_p, MEM_POOL_CHUNK_SIZE);
    MEM_POOLS_STAT_DEALLOC ();
    mem_free_chunk_p = next_p;
  }
} /* mem_pools_collect_empty */

#ifdef MEM_STATS
/**
 * Get pools memory usage statistics
 */
void
mem_pools_get_stats (mem_pools_stats_t *out_pools_stats_p) /**< [out] pools' stats */
{
  JERRY_ASSERT (out_pools_stats_p != NULL);

  *out_pools_stats_p = mem_pools_stats;
} /* mem_pools_get_stats */

/**
 * Reset peak values in memory usage statistics
 */
void
mem_pools_stats_reset_peak (void)
{
  mem_pools_stats.peak_pools_count = mem_pools_stats.pools_count;
} /* mem_pools_stats_reset_peak */

/**
 * Print pools memory usage statistics
 */
void
mem_pools_stats_print (void)
{
  printf ("Pools stats:\n"
          "  Chunk size: %zu\n"
          "  Pool chunks: %zu\n"
          "  Peak pool chunks: %zu\n"
          "  Free chunks: %zu\n"
          "  Pool reuse ratio: %zu.%04zu\n",
          MEM_POOL_CHUNK_SIZE,
          mem_pools_stats.pools_count,
          mem_pools_stats.peak_pools_count,
          mem_pools_stats.free_chunks,
          mem_pools_stats.reused_count / mem_pools_stats.new_alloc_count,
          mem_pools_stats.reused_count % mem_pools_stats.new_alloc_count * 10000 / mem_pools_stats.new_alloc_count);
} /* mem_pools_stats_print */

/**
 * Initalize pools' memory usage statistics account structure
 */
static void
mem_pools_stat_init (void)
{
  memset (&mem_pools_stats, 0, sizeof (mem_pools_stats));
} /* mem_pools_stat_init */

/**
 * Account for allocation of new pool chunk
 */
static void
mem_pools_stat_new_alloc (void)
{
  mem_pools_stats.pools_count++;
  mem_pools_stats.new_alloc_count++;

  if (mem_pools_stats.pools_count > mem_pools_stats.peak_pools_count)
  {
    mem_pools_stats.peak_pools_count = mem_pools_stats.pools_count;
  }
  if (mem_pools_stats.pools_count > mem_pools_stats.global_peak_pools_count)
  {
    mem_pools_stats.global_peak_pools_count = mem_pools_stats.pools_count;
  }
} /* mem_pools_stat_new_alloc */


/**
 * Account for reuse of pool chunk
 */
static void
mem_pools_stat_reuse (void)
{
  mem_pools_stats.pools_count++;
  mem_pools_stats.free_chunks--;
  mem_pools_stats.reused_count++;

  if (mem_pools_stats.pools_count > mem_pools_stats.peak_pools_count)
  {
    mem_pools_stats.peak_pools_count = mem_pools_stats.pools_count;
  }
  if (mem_pools_stats.pools_count > mem_pools_stats.global_peak_pools_count)
  {
    mem_pools_stats.global_peak_pools_count = mem_pools_stats.pools_count;
  }
} /* mem_pools_stat_reuse */


/**
 * Account for freeing a chunk
 */
static void
mem_pools_stat_free_pool (void)
{
  JERRY_ASSERT (mem_pools_stats.pools_count > 0);

  mem_pools_stats.pools_count--;
  mem_pools_stats.free_chunks++;
} /* mem_pools_stat_free_pool */

/**
 * Account for freeing a chunk
 */
static void
mem_pools_stat_dealloc (void)
{
  mem_pools_stats.free_chunks--;
} /* mem_pools_stat_dealloc */
#endif /* MEM_STATS */

/**
 * @}
 * @}
 */
