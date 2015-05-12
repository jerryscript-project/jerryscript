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
 * \addtogroup poolman Memory pool manager
 * @{
 */

/**
 * Memory pool manager implementation
 */

#define JERRY_MEM_POOL_INTERNAL

#include "jrt.h"
#include "jrt-libc-includes.h"
#include "mem-allocator.h"
#include "mem-heap.h"
#include "mem-pool.h"
#include "mem-poolman.h"

/**
 * Lists of pools
 */
mem_pool_state_t *mem_pools;

/**
 * Number of free chunks
 */
size_t mem_free_chunks_number;

#ifdef MEM_STATS
/**
 * Pools' memory usage statistics
 */
mem_pools_stats_t mem_pools_stats;

static void mem_pools_stat_init (void);
static void mem_pools_stat_alloc_pool (void);
static void mem_pools_stat_free_pool (void);
static void mem_pools_stat_alloc_chunk (void);
static void mem_pools_stat_free_chunk (void);

#  define MEM_POOLS_STAT_INIT() mem_pools_stat_init ()
#  define MEM_POOLS_STAT_ALLOC_POOL() mem_pools_stat_alloc_pool ()
#  define MEM_POOLS_STAT_FREE_POOL() mem_pools_stat_free_pool ()
#  define MEM_POOLS_STAT_ALLOC_CHUNK() mem_pools_stat_alloc_chunk ()
#  define MEM_POOLS_STAT_FREE_CHUNK() mem_pools_stat_free_chunk ()
#else /* !MEM_STATS */
#  define MEM_POOLS_STAT_INIT()
#  define MEM_POOLS_STAT_ALLOC_POOL()
#  define MEM_POOLS_STAT_FREE_POOL()
#  define MEM_POOLS_STAT_ALLOC_CHUNK()
#  define MEM_POOLS_STAT_FREE_CHUNK()
#endif /* !MEM_STATS */

/**
 * Initialize pool manager
 */
void
mem_pools_init (void)
{
  mem_pools = NULL;
  mem_free_chunks_number = 0;

  MEM_POOLS_STAT_INIT ();
} /* mem_pools_init */

/**
 * Finalize pool manager
 */
void
mem_pools_finalize (void)
{
  JERRY_ASSERT (mem_pools == NULL);
  JERRY_ASSERT (mem_free_chunks_number == 0);
} /* mem_pools_finalize */

/**
 * Long path for mem_pools_alloc
 *
 * @return true - if there is a free chunk in mem_pools,
 *         false - otherwise (not enough memory).
 */
static bool __attr_noinline___
mem_pools_alloc_longpath (void)
{
  /**
   * If there are no free chunks, allocate new pool.
   */
  if (mem_free_chunks_number == 0)
  {
    mem_pool_state_t *pool_state = (mem_pool_state_t*) mem_heap_alloc_block (MEM_POOL_SIZE, MEM_HEAP_ALLOC_LONG_TERM);

    JERRY_ASSERT (pool_state != NULL);

    mem_pool_init (pool_state, MEM_POOL_SIZE);

    MEM_CP_SET_POINTER (pool_state->next_pool_cp, mem_pools);

    mem_pools = pool_state;

    mem_free_chunks_number += MEM_POOL_CHUNKS_NUMBER;

    MEM_POOLS_STAT_ALLOC_POOL ();
  }
  else
  {
    /**
     * There is definitely at least one pool of specified type with at least one free chunk.
     *
     * Search for the pool.
     */
    mem_pool_state_t *pool_state = mem_pools, *prev_pool_state_p = NULL;

    while (pool_state->first_free_chunk == MEM_POOL_CHUNKS_NUMBER)
    {
      prev_pool_state_p = pool_state;
      pool_state = MEM_CP_GET_NON_NULL_POINTER (mem_pool_state_t, pool_state->next_pool_cp);
    }

    JERRY_ASSERT (prev_pool_state_p != NULL && pool_state != mem_pools);

    prev_pool_state_p->next_pool_cp = pool_state->next_pool_cp;
    MEM_CP_SET_NON_NULL_POINTER (pool_state->next_pool_cp, mem_pools);
    mem_pools = pool_state;
  }

  return true;
} /* mem_pools_alloc_longpath */

/**
 * Allocate a chunk of specified size
 *
 * @return pointer to allocated chunk, if allocation was successful,
 *         or NULL - if not enough memory.
 */
uint8_t*
mem_pools_alloc (void)
{
  if (mem_pools == NULL || mem_pools->first_free_chunk == MEM_POOL_CHUNKS_NUMBER)
  {
    if (!mem_pools_alloc_longpath ())
    {
      return NULL;
    }
  }

  JERRY_ASSERT (mem_pools != NULL && mem_pools->first_free_chunk != MEM_POOL_CHUNKS_NUMBER);

  /**
   * And allocate chunk within it.
   */
  mem_free_chunks_number--;

  MEM_POOLS_STAT_ALLOC_CHUNK ();

  return mem_pool_alloc_chunk (mem_pools);
} /* mem_pools_alloc */

/**
 * Free the chunk
 */
void
mem_pools_free (uint8_t *chunk_p) /**< pointer to the chunk */
{
  mem_pool_state_t *pool_state = mem_pools, *prev_pool_state_p = NULL;

  /**
   * Search for the pool containing specified chunk.
   */
  while (!mem_pool_is_chunk_inside (pool_state, chunk_p))
  {
    prev_pool_state_p = pool_state;
    pool_state = MEM_CP_GET_NON_NULL_POINTER (mem_pool_state_t, pool_state->next_pool_cp);
  }

  /**
   * Free the chunk
   */
  mem_pool_free_chunk (pool_state, chunk_p);
  mem_free_chunks_number++;

  MEM_POOLS_STAT_FREE_CHUNK ();

  /**
   * If all chunks of the pool are free, free the pool itself.
   */
  if (pool_state->free_chunks_number == MEM_POOL_CHUNKS_NUMBER)
  {
    if (prev_pool_state_p != NULL)
    {
      prev_pool_state_p->next_pool_cp = pool_state->next_pool_cp;
    }
    else
    {
      mem_pools = MEM_CP_GET_POINTER (mem_pool_state_t, pool_state->next_pool_cp);
    }

    mem_free_chunks_number -= MEM_POOL_CHUNKS_NUMBER;

    mem_heap_free_block ((uint8_t*) pool_state);

    MEM_POOLS_STAT_FREE_POOL ();
  }
  else if (mem_pools != pool_state)
  {
    JERRY_ASSERT (prev_pool_state_p != NULL);

    prev_pool_state_p->next_pool_cp = pool_state->next_pool_cp;
    MEM_CP_SET_NON_NULL_POINTER (pool_state->next_pool_cp, mem_pools);
    mem_pools = pool_state;
  }
} /* mem_pools_free */

#ifdef MEM_STATS
/**
 * Get pools memory usage statistics
 */
void
mem_pools_get_stats (mem_pools_stats_t *out_pools_stats_p) /**< out: pools' stats */
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
  mem_pools_stats.peak_allocated_chunks = mem_pools_stats.allocated_chunks;
} /* mem_pools_stats_reset_peak */

/**
 * Initalize pools' memory usage statistics account structure
 */
static void
mem_pools_stat_init (void)
{
  memset (&mem_pools_stats, 0, sizeof (mem_pools_stats));
} /* mem_pools_stat_init */

/**
 * Account allocation of a pool
 */
static void
mem_pools_stat_alloc_pool (void)
{
  mem_pools_stats.pools_count++;
  mem_pools_stats.free_chunks = mem_free_chunks_number;

  if (mem_pools_stats.pools_count > mem_pools_stats.peak_pools_count)
  {
    mem_pools_stats.peak_pools_count = mem_pools_stats.pools_count;
  }
  if (mem_pools_stats.pools_count > mem_pools_stats.global_peak_pools_count)
  {
    mem_pools_stats.global_peak_pools_count = mem_pools_stats.pools_count;
  }
} /* mem_pools_stat_alloc_pool */

/**
 * Account freeing of a pool
 */
static void
mem_pools_stat_free_pool (void)
{
  JERRY_ASSERT (mem_pools_stats.pools_count > 0);

  mem_pools_stats.pools_count--;
  mem_pools_stats.free_chunks = mem_free_chunks_number;
} /* mem_pools_stat_free_pool */

/**
 * Account allocation of chunk in a pool
 */
static void
mem_pools_stat_alloc_chunk (void)
{
  JERRY_ASSERT (mem_pools_stats.free_chunks > 0);

  mem_pools_stats.allocated_chunks++;
  mem_pools_stats.free_chunks--;

  if (mem_pools_stats.allocated_chunks > mem_pools_stats.peak_allocated_chunks)
  {
    mem_pools_stats.peak_allocated_chunks = mem_pools_stats.allocated_chunks;
  }
  if (mem_pools_stats.allocated_chunks > mem_pools_stats.global_peak_allocated_chunks)
  {
    mem_pools_stats.global_peak_allocated_chunks = mem_pools_stats.allocated_chunks;
  }
} /* mem_pools_stat_alloc_chunk */

/**
 * Account freeing of chunk in a pool
 */
static void
mem_pools_stat_free_chunk (void)
{
  JERRY_ASSERT (mem_pools_stats.allocated_chunks > 0);

  mem_pools_stats.allocated_chunks--;
  mem_pools_stats.free_chunks++;
} /* mem_pools_stat_free_chunk */
#endif /* MEM_STATS */

/**
 * @}
 */
/**
 * @}
 */
