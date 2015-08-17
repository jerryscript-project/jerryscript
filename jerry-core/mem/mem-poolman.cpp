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
#include "mem-poolman.h"

/**
 * Size of a pool
 */
#define MEM_POOL_SIZE (mem_heap_get_chunked_block_data_size ())

/**
 * Number of chunks in a pool
 */
#define MEM_POOL_CHUNKS_NUMBER (MEM_POOL_SIZE / MEM_POOL_CHUNK_SIZE)

#ifndef JERRY_NDEBUG
size_t mem_free_chunks_number;
#endif /* !JERRY_NDEBUG */

/**
 * Index of chunk in a pool
 */
typedef uint8_t mem_pool_chunk_index_t;

/**
 * Pool chunk
 */
typedef struct mem_pool_chunk_t
{
  /**
   * Union of possible free chunk layouts
   *
   * Allocated chunk represents raw data of MEM_POOL_CHUNK_SIZE bytes,
   * and so, has no fixed layout.
   */
  union
  {
    /**
     * Structure of free pool chunks that are:
     *  - first in corresponding pool, while empty pool collector is not active;
     *  - not first in corresponding pool.
     */
    struct
    {
      mem_pool_chunk_t *next_p; /**< global list of free pool chunks */
    } free;

    /**
     * While empty pool collector is active, the following structure is used
     * for first chunks of pools, in which first chunks are free
     *
     * See also:
     *          mem_pools_collect_empty
     */
    struct
    {
      mem_cpointer_t next_first_cp; /**< list of first free chunks of
                                     *   pools with free first chunks */
      mem_cpointer_t free_list_cp; /**< list of free chunks
                                    *   in the pool containing this chunk */
      uint16_t hint_magic_num; /**< magic number that hints whether
                                *   there is a probability that the chunk
                                *   is an item (header) in a pool list */
      mem_pool_chunk_index_t free_chunks_num; /**< number of free chunks
                                               *   in the pool containing this chunk */
      uint8_t list_id; /**< identifier of a pool list */
    } pool_gc;
  } u;
} mem_pool_chunk_t;

/**
 * List of free pool chunks
 */
mem_pool_chunk_t *mem_free_chunk_p;

static void mem_check_pools (void);

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

/*
 * Valgrind-related options and headers
 */
#ifdef JERRY_VALGRIND
# include "memcheck.h"

# define VALGRIND_NOACCESS_SPACE(p, s)  (void)VALGRIND_MAKE_MEM_NOACCESS((p), (s))
# define VALGRIND_UNDEFINED_SPACE(p, s) (void)VALGRIND_MAKE_MEM_UNDEFINED((p), (s))
# define VALGRIND_DEFINED_SPACE(p, s)   (void)VALGRIND_MAKE_MEM_DEFINED((p), (s))
#else /* JERRY_VALGRIND */
# define VALGRIND_NOACCESS_SPACE(p, s)
# define VALGRIND_UNDEFINED_SPACE(p, s)
# define VALGRIND_DEFINED_SPACE(p, s)
#endif /* JERRY_VALGRIND */

/**
 * Initialize pool manager
 */
void
mem_pools_init (void)
{
#ifndef JERRY_NDEBUG
  mem_free_chunks_number = 0;
#endif /* !JERRY_NDEBUG */

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

#ifndef JERRY_NDEBUG
  JERRY_ASSERT (mem_free_chunks_number == 0);
#endif /* !JERRY_NDEBUG */
} /* mem_pools_finalize */

void
mem_pools_collect_empty (void)
{
  /*
   * Hint magic number in header of pools with free first chunks
   */
  const uint16_t hint_magic_num_value = 0x7e89;

  /*
   * At first pass collect pointers to those of free chunks that are first at their pools
   * to separate lists (collection-time pool lists) and change them to headers of corresponding pools
   */

  /*
   * Number of collection-time pool lists
   */
  constexpr uint32_t pool_lists_number = 8;

  /*
   * Collection-time pool lists
   */
  mem_pool_chunk_t *pool_lists_p[pool_lists_number];
  for (uint32_t i = 0; i < pool_lists_number; i++)
  {
    pool_lists_p[i] = NULL;
  }

  /*
   * Number of the pools, included into the lists
   */
  uint32_t pools_in_lists_number = 0;

  for (mem_pool_chunk_t *free_chunk_iter_p = mem_free_chunk_p, *prev_free_chunk_p = NULL, *next_free_chunk_p;
       free_chunk_iter_p != NULL;
       free_chunk_iter_p = next_free_chunk_p)
  {
    mem_pool_chunk_t *pool_start_p = (mem_pool_chunk_t *) mem_heap_get_chunked_block_start (free_chunk_iter_p);

    VALGRIND_DEFINED_SPACE (free_chunk_iter_p, MEM_POOL_CHUNK_SIZE);

    next_free_chunk_p = free_chunk_iter_p->u.free.next_p;

    if (pool_start_p == free_chunk_iter_p)
    {
      /*
       * The chunk is first at its pool
       *
       * Remove the chunk from common list of free chunks
       */
      if (prev_free_chunk_p == NULL)
      {
        JERRY_ASSERT (mem_free_chunk_p == free_chunk_iter_p);

        mem_free_chunk_p = next_free_chunk_p;
      }
      else
      {
        prev_free_chunk_p->u.free.next_p = next_free_chunk_p;
      }

      pools_in_lists_number++;

      uint8_t list_id = pools_in_lists_number % pool_lists_number;

      /*
       * Initialize pool header and insert the pool into one of lists
       */
      free_chunk_iter_p->u.pool_gc.free_list_cp = MEM_CP_NULL;
      free_chunk_iter_p->u.pool_gc.free_chunks_num = 1; /* the first chunk */
      free_chunk_iter_p->u.pool_gc.hint_magic_num = hint_magic_num_value;
      free_chunk_iter_p->u.pool_gc.list_id = list_id;

      MEM_CP_SET_POINTER (free_chunk_iter_p->u.pool_gc.next_first_cp, pool_lists_p[list_id]);
      pool_lists_p[list_id] = free_chunk_iter_p;
    }
    else
    {
      prev_free_chunk_p = free_chunk_iter_p;
    }
  }

  if (pools_in_lists_number == 0)
  {
    /* there are no empty pools */

    return;
  }

  /*
   * At second pass we check for all rest free chunks whether they are in pools that were included into
   * collection-time pool lists.
   *
   * For each of the chunk, try to find the corresponding pool through iterating the list.
   *
   * If pool is found in a list (so, first chunk of the pool is free) for a chunk, increment counter
   * of free chunks in the pools, and move the chunk from global free chunks list to collection-time
   * local list of corresponding pool's free chunks.
   */
  for (mem_pool_chunk_t *free_chunk_iter_p = mem_free_chunk_p, *prev_free_chunk_p = NULL, *next_free_chunk_p;
       free_chunk_iter_p != NULL;
       free_chunk_iter_p = next_free_chunk_p)
  {
    mem_pool_chunk_t *pool_start_p = (mem_pool_chunk_t *) mem_heap_get_chunked_block_start (free_chunk_iter_p);

    next_free_chunk_p = free_chunk_iter_p->u.free.next_p;

    bool is_chunk_moved_to_local_list = false;

    /*
     * The magic number doesn't guarantee that the chunk is actually a pool header,
     * so it is only optimization to reduce number of unnecessary iterations over
     * pool lists.
     */
    if (pool_start_p->u.pool_gc.hint_magic_num == hint_magic_num_value)
    {
      /*
       * Maybe, the first chunk is free.
       *
       * If it is so, it is included in the list of pool's first free chunks.
       */
      uint8_t id_to_search_in = pool_start_p->u.pool_gc.list_id;

      if (id_to_search_in < pool_lists_number)
      {
        for (mem_pool_chunk_t *pool_list_iter_p = pool_lists_p[id_to_search_in];
             pool_list_iter_p != NULL;
             pool_list_iter_p = MEM_CP_GET_POINTER (mem_pool_chunk_t,
                                                    pool_list_iter_p->u.pool_gc.next_first_cp))
        {
          if (pool_list_iter_p == pool_start_p)
          {
            /*
             * The first chunk is actually free.
             *
             * So, incrementing free chunks counter in it.
             */
            pool_start_p->u.pool_gc.free_chunks_num++;

            /*
             * It is possible that the corresponding pool is empty
             *
             * Moving current chunk from common list of free chunks to temporary list, local to the pool
             */
            if (prev_free_chunk_p == NULL)
            {
              JERRY_ASSERT (mem_free_chunk_p == free_chunk_iter_p);

              mem_free_chunk_p = next_free_chunk_p;
            }
            else
            {
              prev_free_chunk_p->u.free.next_p = next_free_chunk_p;
            }

            free_chunk_iter_p->u.free.next_p = MEM_CP_GET_POINTER (mem_pool_chunk_t,
                                                                   pool_start_p->u.pool_gc.free_list_cp);
            MEM_CP_SET_NON_NULL_POINTER (pool_start_p->u.pool_gc.free_list_cp, free_chunk_iter_p);

            is_chunk_moved_to_local_list = true;

            break;
          }
        }
      }
    }

    if (!is_chunk_moved_to_local_list)
    {
      prev_free_chunk_p = free_chunk_iter_p;
    }
  }

  /*
   * At third pass we check each pool in collection-time pool lists free for counted
   * number of free chunks in the pool.
   *
   * If the number is equal to number of chunks in the pool - then the pool is empty, and so is freed,
   * otherwise - free chunks of the pool are returned to common list of free chunks.
   */
  for (uint8_t list_id = 0; list_id < pool_lists_number; list_id++)
  {
    for (mem_pool_chunk_t *pool_list_iter_p = pool_lists_p[list_id], *next_p;
         pool_list_iter_p != NULL;
         pool_list_iter_p = next_p)
    {
      next_p = MEM_CP_GET_POINTER (mem_pool_chunk_t,
                                   pool_list_iter_p->u.pool_gc.next_first_cp);

      if (pool_list_iter_p->u.pool_gc.free_chunks_num == MEM_POOL_CHUNKS_NUMBER)
      {
#ifndef JERRY_NDEBUG
        mem_free_chunks_number -= MEM_POOL_CHUNKS_NUMBER;
#endif /* !JERRY_NDEBUG */

        mem_heap_free_block (pool_list_iter_p);

        MEM_POOLS_STAT_FREE_POOL ();
      }
      else
      {
        mem_pool_chunk_t *first_chunk_p = pool_list_iter_p;

        /*
         * Convert layout of first chunk from collection-time pool header to common free chunk
         */
        first_chunk_p->u.free.next_p = MEM_CP_GET_POINTER (mem_pool_chunk_t,
                                                           pool_list_iter_p->u.pool_gc.free_list_cp);

        /*
         * Link local pool's list of free chunks into global list of free chunks
         */
        for (mem_pool_chunk_t *pool_chunks_iter_p = first_chunk_p;
             ;
             pool_chunks_iter_p = pool_chunks_iter_p->u.free.next_p)
        {
          JERRY_ASSERT (pool_chunks_iter_p != NULL);

          if (pool_chunks_iter_p->u.free.next_p == NULL)
          {
            pool_chunks_iter_p->u.free.next_p = mem_free_chunk_p;

            break;
          }
        }

        mem_free_chunk_p = first_chunk_p;
      }
    }
  }

#ifdef JERRY_VALGRIND
  /*
   * Valgrind-mode specific pass that marks all free chunks inaccessible
   */
  for (mem_pool_chunk_t *free_chunk_iter_p = mem_free_chunk_p, *next_free_chunk_p;
       free_chunk_iter_p != NULL;
       free_chunk_iter_p = next_free_chunk_p)
  {
    next_free_chunk_p = free_chunk_iter_p->u.free.next_p;

    VALGRIND_NOACCESS_SPACE (free_chunk_iter_p, MEM_POOL_CHUNK_SIZE);
  }
#endif /* JERRY_VALGRIND */
} /* mem_pools_collect_empty */

/**
 * Long path for mem_pools_alloc
 */
static void __attr_noinline___
mem_pools_alloc_longpath (void)
{
  mem_check_pools ();

  JERRY_ASSERT (mem_free_chunk_p == NULL);

  JERRY_ASSERT (MEM_POOL_SIZE <= mem_heap_get_chunked_block_data_size ());
  JERRY_ASSERT (MEM_POOL_CHUNKS_NUMBER >= 1);

  mem_pool_chunk_t *pool_start_p = (mem_pool_chunk_t*) mem_heap_alloc_chunked_block (MEM_HEAP_ALLOC_LONG_TERM);

  if (mem_free_chunk_p != NULL)
  {
    /* some chunks were freed due to GC invoked by heap allocator */
    mem_heap_free_block (pool_start_p);

    return;
  }

#ifndef JERRY_NDEBUG
  mem_free_chunks_number += MEM_POOL_CHUNKS_NUMBER;
#endif /* !JERRY_NDEBUG */

  JERRY_STATIC_ASSERT (MEM_POOL_CHUNK_SIZE % MEM_ALIGNMENT == 0);
  JERRY_STATIC_ASSERT (sizeof (mem_pool_chunk_t) == MEM_POOL_CHUNK_SIZE);
  JERRY_STATIC_ASSERT (sizeof (mem_pool_chunk_index_t) <= MEM_POOL_CHUNK_SIZE);
  JERRY_ASSERT ((mem_pool_chunk_index_t) MEM_POOL_CHUNKS_NUMBER == MEM_POOL_CHUNKS_NUMBER);
  JERRY_ASSERT (MEM_POOL_SIZE == MEM_POOL_CHUNKS_NUMBER * MEM_POOL_CHUNK_SIZE);

  JERRY_ASSERT (((uintptr_t) pool_start_p) % MEM_ALIGNMENT == 0);

  mem_pool_chunk_t *prev_free_chunk_p = NULL;

  for (mem_pool_chunk_index_t chunk_index = 0;
       chunk_index < MEM_POOL_CHUNKS_NUMBER;
       chunk_index++)
  {
    mem_pool_chunk_t *chunk_p = pool_start_p + chunk_index;

    if (prev_free_chunk_p != NULL)
    {
      prev_free_chunk_p->u.free.next_p = chunk_p;
    }

    prev_free_chunk_p = chunk_p;
  }

  prev_free_chunk_p->u.free.next_p = NULL;

#ifdef JERRY_VALGRIND
  for (mem_pool_chunk_index_t chunk_index = 0;
       chunk_index < MEM_POOL_CHUNKS_NUMBER;
       chunk_index++)
  {
    mem_pool_chunk_t *chunk_p = pool_start_p + chunk_index;

    VALGRIND_NOACCESS_SPACE (chunk_p, MEM_POOL_CHUNK_SIZE);
  }
#endif /* JERRY_VALGRIND */

  mem_free_chunk_p = pool_start_p;

  MEM_POOLS_STAT_ALLOC_POOL ();

  mem_check_pools ();
} /* mem_pools_alloc_longpath */

/**
 * Allocate a chunk of specified size
 *
 * @return pointer to allocated chunk, if allocation was successful,
 *         or NULL - if not enough memory.
 */
uint8_t* __attr_always_inline___
mem_pools_alloc (void)
{
  mem_check_pools ();

  do
  {
    if (mem_free_chunk_p != NULL)
    {
      mem_pool_chunk_t *chunk_p = mem_free_chunk_p;

      MEM_POOLS_STAT_ALLOC_CHUNK ();

#ifndef JERRY_NDEBUG
      mem_free_chunks_number--;
#endif /* !JERRY_NDEBUG */

      VALGRIND_DEFINED_SPACE (chunk_p, MEM_POOL_CHUNK_SIZE);

      mem_free_chunk_p = chunk_p->u.free.next_p;

      VALGRIND_UNDEFINED_SPACE (chunk_p, MEM_POOL_CHUNK_SIZE);


      mem_check_pools ();

      return (uint8_t *) chunk_p;
    }
    else
    {
      mem_pools_alloc_longpath ();

      /* the assertion guarantees that there will be no more than two iterations */
      JERRY_ASSERT (mem_free_chunk_p != NULL);
    }
  } while (true);
} /* mem_pools_alloc */

/**
 * Free the chunk
 */
void __attr_always_inline___
mem_pools_free (uint8_t *chunk_p) /**< pointer to the chunk */
{
  mem_check_pools ();

  mem_pool_chunk_t *chunk_to_free_p = (mem_pool_chunk_t *) chunk_p;

  chunk_to_free_p->u.free.next_p = mem_free_chunk_p;
  mem_free_chunk_p = chunk_to_free_p;

  VALGRIND_NOACCESS_SPACE (chunk_to_free_p, MEM_POOL_CHUNK_SIZE);

#ifndef JERRY_NDEBUG
  mem_free_chunks_number++;
#endif /* !JERRY_NDEBUG */

  MEM_POOLS_STAT_FREE_CHUNK ();

  mem_check_pools ();
} /* mem_pools_free */

/**
 * Check correctness of pool allocator state
 */
static void
mem_check_pools (void)
{
#ifndef JERRY_DISABLE_HEAVY_DEBUG
  size_t free_chunks_met = 0;

  for (mem_pool_chunk_t *free_chunk_iter_p = mem_free_chunk_p, *next_free_chunk_p;
       free_chunk_iter_p != NULL;
       free_chunk_iter_p = next_free_chunk_p)
  {
    VALGRIND_DEFINED_SPACE (free_chunk_iter_p, MEM_POOL_CHUNK_SIZE);

    next_free_chunk_p = free_chunk_iter_p->u.free.next_p;

    VALGRIND_NOACCESS_SPACE (free_chunk_iter_p, MEM_POOL_CHUNK_SIZE);

    free_chunks_met++;
  }

  JERRY_ASSERT (free_chunks_met == mem_free_chunks_number);
#endif /* !JERRY_DISABLE_HEAVY_DEBUG */
} /* mem_check_pools */

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

  if (mem_pools_stats.pools_count > mem_pools_stats.peak_pools_count)
  {
    mem_pools_stats.peak_pools_count = mem_pools_stats.pools_count;
  }
  if (mem_pools_stats.pools_count > mem_pools_stats.global_peak_pools_count)
  {
    mem_pools_stats.global_peak_pools_count = mem_pools_stats.pools_count;
  }

  mem_pools_stats.free_chunks += MEM_POOL_CHUNKS_NUMBER;
} /* mem_pools_stat_alloc_pool */

/**
 * Account freeing of a pool
 */
static void
mem_pools_stat_free_pool (void)
{
  JERRY_ASSERT (mem_pools_stats.free_chunks >= MEM_POOL_CHUNKS_NUMBER);

  mem_pools_stats.free_chunks -= MEM_POOL_CHUNKS_NUMBER;

  JERRY_ASSERT (mem_pools_stats.pools_count > 0);

  mem_pools_stats.pools_count--;
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
