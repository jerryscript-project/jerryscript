/* Copyright 2014 Samsung Electronics Co., Ltd.
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

#include "globals.h"
#include "jerry-libc.h"
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
#else /* !MEM_STATS */
#  define mem_pools_stat_init ()
#  define mem_pools_stat_alloc_pool ()
#  define mem_pools_stat_free_pool ()
#  define mem_pools_stat_alloc_chunk ()
#  define mem_pools_stat_free_chunk ()
#endif /* !MEM_STATS */

/**
 * Initialize pool manager
 */
void
mem_pools_init (void)
{
  mem_pools = NULL;
  mem_free_chunks_number = 0;

  mem_pools_stat_init ();
} /* mem_pools_init */

/**
 * Finalize pool manager
 */
void
mem_pools_finalize (void)
{
  JERRY_ASSERT(mem_pools == NULL);
  JERRY_ASSERT(mem_free_chunks_number == 0);
} /* mem_pools_finalize */

/**
 * Allocate a chunk of specified size
 *
 * @return pointer to allocated chunk, if allocation was successful,
 *         or NULL - if not enough memory.
 */
uint8_t*
mem_pools_alloc (void)
{
  /**
   * If there are no free chunks, allocate new pool.
   */
  if (mem_free_chunks_number == 0)
  {
    /**
     * Space, at least for header and eight chunks.
     *
     * TODO: Config.
     */
    size_t pool_size = mem_heap_recommend_allocation_size (sizeof (mem_pool_state_t) + 8 * MEM_POOL_CHUNK_SIZE);

    mem_pool_state_t *pool_state = (mem_pool_state_t*) mem_heap_alloc_block (pool_size, MEM_HEAP_ALLOC_LONG_TERM);

    if (pool_state == NULL)
    {
      /**
       * Not enough space for new pool.
       */
      return NULL;
    }

    mem_pool_init (pool_state, pool_size);

    if (mem_pools == NULL)
    {
      pool_state->next_pool_cp = MEM_COMPRESSED_POINTER_NULL;
    }
    else
    {
      pool_state->next_pool_cp = (uint16_t) mem_compress_pointer (mem_pools);
    }

    mem_pools = pool_state;

    mem_free_chunks_number += pool_state->chunks_number;

    mem_pools_stat_alloc_pool ();
  }

  /**
   * Now there is definitely at least one pool of specified type with at least one free chunk.
   *
   * Search for the pool.
   */
  mem_pool_state_t *pool_state = mem_pools;

  while (pool_state->first_free_chunk == pool_state->chunks_number)
  {
    pool_state = mem_decompress_pointer (pool_state->next_pool_cp);

    JERRY_ASSERT(pool_state != NULL);
  }

  /**
   * And allocate chunk within it.
   */
  mem_free_chunks_number--;

  mem_pools_stat_alloc_chunk ();

  return mem_pool_alloc_chunk (pool_state);
} /* mem_pools_alloc */

/**
 * Free the chunk
 */
void
mem_pools_free (uint8_t *chunk_p) /**< pointer to the chunk */
{
  mem_pool_state_t *pool_state = mem_pools, *prev_pool_state = NULL;

  /**
   * Search for the pool containing specified chunk.
   */
  while (!(chunk_p >= MEM_POOL_SPACE_START(pool_state)
           && chunk_p <= MEM_POOL_SPACE_START(pool_state) + pool_state->chunks_number * MEM_POOL_CHUNK_SIZE))
  {
    prev_pool_state = pool_state;
    pool_state = mem_decompress_pointer (pool_state->next_pool_cp);

    JERRY_ASSERT(pool_state != NULL);
  }

  /**
   * Free the chunk
   */
  mem_pool_free_chunk (pool_state, chunk_p);
  mem_free_chunks_number++;

  mem_pools_stat_free_chunk ();

  /**
   * If all chunks of the pool are free, free the pool itself.
   */
  if (pool_state->free_chunks_number == pool_state->chunks_number)
  {
    if (prev_pool_state != NULL)
    {
      prev_pool_state->next_pool_cp = pool_state->next_pool_cp;
    }
    else
    {
      if (pool_state->next_pool_cp == MEM_COMPRESSED_POINTER_NULL)
      {
        mem_pools = NULL;
      }
      else
      {
        mem_pools = mem_decompress_pointer (pool_state->next_pool_cp);
      }
    }

    mem_free_chunks_number -= pool_state->chunks_number;

    mem_heap_free_block ((uint8_t*)pool_state);

    mem_pools_stat_free_pool ();
  }
} /* mem_pools_free */

#ifdef MEM_STATS
/**
 * Get pools memory usage statistics
 */
void
mem_pools_get_stats (mem_pools_stats_t *out_pools_stats_p) /**< out: pools' stats */
{
  JERRY_ASSERT(out_pools_stats_p != NULL);

  *out_pools_stats_p = mem_pools_stats;
} /* mem_pools_get_stats */

/**
 * Initalize pools' memory usage statistics account structure
 */
static void
mem_pools_stat_init (void)
{
  __memset (&mem_pools_stats, 0, sizeof (mem_pools_stats));
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
} /* mem_pools_stat_alloc_pool */

/**
 * Account freeing of a pool
 */
static void
mem_pools_stat_free_pool (void)
{
  JERRY_ASSERT(mem_pools_stats.pools_count > 0);

  mem_pools_stats.pools_count--;
  mem_pools_stats.free_chunks = mem_free_chunks_number;
} /* mem_pools_stat_free_pool */

/**
 * Account allocation of chunk in a pool
 */
static void
mem_pools_stat_alloc_chunk (void)
{
  JERRY_ASSERT(mem_pools_stats.free_chunks > 0);

  mem_pools_stats.allocated_chunks++;
  mem_pools_stats.free_chunks--;

  if (mem_pools_stats.allocated_chunks > mem_pools_stats.peak_allocated_chunks)
  {
    mem_pools_stats.peak_allocated_chunks = mem_pools_stats.allocated_chunks;
  }
} /* mem_pools_stat_alloc_chunk */

/**
 * Account freeing of chunk in a pool
 */
static void
mem_pools_stat_free_chunk (void)
{
  JERRY_ASSERT(mem_pools_stats.allocated_chunks > 0);

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
