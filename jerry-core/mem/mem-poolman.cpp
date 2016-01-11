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

#include "jrt.h"
#include "jrt-libc-includes.h"
#include "mem-allocator.h"
#include "mem-heap.h"
#include "mem-poolman.h"

#define MEM_ALLOCATOR_INTERNAL
#include "mem-allocator-internal.h"

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
      uint8_t traversal_check_flag; /**< flag that is flipped between two non-first chunk lists traversals
                                     *   to determine whether the corresponding pool-first chunks are actually free */
    } pool_gc;

    /**
     * The field is added to make sizeof (mem_pool_chunk_t) equal to MEM_POOL_CHUNK_SIZE
     */
    uint8_t allocated_area[MEM_POOL_CHUNK_SIZE];
  } u;
} mem_pool_chunk_t;

/**
 * The condition is assumed when using pointer arithmetics on (mem_pool_chunk_t *) pointer type
 */
JERRY_STATIC_ASSERT (sizeof (mem_pool_chunk_t) == MEM_POOL_CHUNK_SIZE);

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

/**
 * Helper for reading magic number and traversal check flag fields of a pool-first chunk,
 * that suppresses valgrind's warnings about undefined values.
 *
 * A pool-first chunk can be either allocated or free.
 *
 * As chunks are marked as undefined upon allocation, some of chunks can still be
 * fully or partially marked as undefined.
 *
 * Nevertheless, the fields are read and their values are used to determine
 * whether the chunk is actually free pool-first chunk.
 *
 * See also:
 *          Description of collection algorithm in mem_pools_collect_empty
 */
static void __attr_always_inline___
mem_pools_collect_read_magic_num_and_flag (mem_pool_chunk_t *pool_first_chunk_p, /**< a pool-first chunk */
                                           uint16_t *out_magic_num_field_value_p, /**< out: value of magic num field,
                                                                                   *        read from the chunk */
                                           bool *out_traversal_check_flag_p) /**< out: value of traversal check flag
                                                                              *        field, read from the chunk */
{
  JERRY_ASSERT (pool_first_chunk_p != NULL);
  JERRY_ASSERT (out_magic_num_field_value_p != NULL);
  JERRY_ASSERT (out_traversal_check_flag_p != NULL);

#ifdef JERRY_VALGRIND
  /*
   * If the chunk is not free, there may be undefined bytes at hint_magic_num and traversal_check_flag fields.
   *
   * Although, it is correct for the routine, valgrind issues warning about using uninitialized data
   * in conditional expression. To suppress the false-positive warning, the chunk is temporarily marked
   * as defined, and after reading hint magic number and list identifier, valgrind state of the chunk is restored.
   */
  uint8_t vbits[MEM_POOL_CHUNK_SIZE];
  unsigned status;

  status = VALGRIND_GET_VBITS (pool_first_chunk_p, vbits, MEM_POOL_CHUNK_SIZE);
  JERRY_ASSERT (status == 0 || status == 1);

  VALGRIND_DEFINED_SPACE (pool_first_chunk_p, MEM_POOL_CHUNK_SIZE);
#endif /* JERRY_VALGRIND */

  uint16_t magic_num_field = pool_first_chunk_p->u.pool_gc.hint_magic_num;
  bool traversal_check_flag = pool_first_chunk_p->u.pool_gc.traversal_check_flag;

#ifdef JERRY_VALGRIND
  status = VALGRIND_SET_VBITS (pool_first_chunk_p, vbits, MEM_POOL_CHUNK_SIZE);
  JERRY_ASSERT (status == 0 || status == 1);
#endif /* JERRY_VALGRIND */

  *out_magic_num_field_value_p = magic_num_field;
  *out_traversal_check_flag_p = traversal_check_flag;
} /* mem_pools_collect_read_magic_num_and_flag */

/**
 * Collect chunks from empty pools and free the pools
 */
void
mem_pools_collect_empty (void)
{
  /*
   * Hint magic number in header of pools with free pool-first chunks
   */
  const uint16_t hint_magic_num_value = 0x7e89;

  /*
   * Collection-time chunk lists
   */
  mem_pool_chunk_t *first_chunks_list_p = NULL;
  mem_pool_chunk_t *non_first_chunks_list_p = NULL;

  /*
   * At first stage collect free pool-first chunks to separate collection-time lists
   * and change their layout from mem_pool_chunk_t::u::free to mem_pool_chunk_t::u::pool_gc
   */
  {
    mem_pool_chunk_t tmp_header;
    tmp_header.u.free.next_p = mem_free_chunk_p;

    for (mem_pool_chunk_t *free_chunk_iter_p = tmp_header.u.free.next_p,
                          *prev_free_chunk_p = &tmp_header,
                          *next_free_chunk_p;
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
        prev_free_chunk_p->u.free.next_p = next_free_chunk_p;

        /*
         * Initialize pool-first chunk as pool header and it insert into list of free pool-first chunks
         */
        free_chunk_iter_p->u.pool_gc.free_list_cp = MEM_CP_NULL;
        free_chunk_iter_p->u.pool_gc.free_chunks_num = 1; /* the first chunk */
        free_chunk_iter_p->u.pool_gc.hint_magic_num = hint_magic_num_value;
        free_chunk_iter_p->u.pool_gc.traversal_check_flag = false;

        MEM_CP_SET_POINTER (free_chunk_iter_p->u.pool_gc.next_first_cp, first_chunks_list_p);
        first_chunks_list_p = free_chunk_iter_p;
      }
      else
      {
        prev_free_chunk_p = free_chunk_iter_p;
      }
    }

    mem_free_chunk_p = tmp_header.u.free.next_p;
  }

  if (first_chunks_list_p == NULL)
  {
    /* there are no empty pools */

    return;
  }

  /*
   * At second stage we collect all free non-pool-first chunks, for which corresponding pool-first chunks are free,
   * and link them into the corresponding mem_pool_chunk_t::u::pool_gc::free_list_cp list, while also maintaining
   * the corresponding mem_pool_chunk_t::u::pool_gc::free_chunks_num:
   *  - at first, for each non-pool-first free chunk we check whether traversal check flag is cleared in corresponding
   *    first chunk in the same pool, and move those chunks, for which the condition is true,
   *    to separate temporary list.
   *
   *  - then, we flip the traversal check flags for each of free pool-first chunks.
   *
   *  - at last, we perform almost the same as at first step, but check only non-pool-first chunks from the temporary
   *    list, and send the chunks, for which the corresponding traversal check flag is cleared, back to the common list
   *    of free chunks, and the rest chunks from the temporary list are linked to corresponding pool-first chunks.
   *    Also, counter of the linked free chunks is maintained in every free pool-first chunk.
   */
  {
    {
      mem_pool_chunk_t tmp_header;
      tmp_header.u.free.next_p = mem_free_chunk_p;

      for (mem_pool_chunk_t *free_chunk_iter_p = tmp_header.u.free.next_p,
                            *prev_free_chunk_p = &tmp_header,
                            *next_free_chunk_p;
           free_chunk_iter_p != NULL;
           free_chunk_iter_p = next_free_chunk_p)
      {
        mem_pool_chunk_t *pool_start_p = (mem_pool_chunk_t *) mem_heap_get_chunked_block_start (free_chunk_iter_p);

        next_free_chunk_p = free_chunk_iter_p->u.free.next_p;

        /*
         * The magic number doesn't guarantee that the chunk is actually a free pool-first chunk,
         * so we test the traversal check flag after flipping values of the flags in every
         * free pool-first chunk.
         */
        uint16_t magic_num_field;
        bool traversal_check_flag;

        mem_pools_collect_read_magic_num_and_flag (pool_start_p, &magic_num_field, &traversal_check_flag);

        /*
         * During this traversal the flag in the free header chunks is in cleared state
         */
        if (!traversal_check_flag
            && magic_num_field == hint_magic_num_value)
        {
          free_chunk_iter_p->u.free.next_p = non_first_chunks_list_p;
          non_first_chunks_list_p = free_chunk_iter_p;

          prev_free_chunk_p->u.free.next_p = next_free_chunk_p;
        }
        else
        {
          prev_free_chunk_p = free_chunk_iter_p;
        }
      }

      mem_free_chunk_p = tmp_header.u.free.next_p;
    }

    {
      /*
       * Now, flip the traversal check flag in free pool-first chunks
       */
      for (mem_pool_chunk_t *first_chunks_iter_p = first_chunks_list_p;
           first_chunks_iter_p != NULL;
           first_chunks_iter_p = MEM_CP_GET_POINTER (mem_pool_chunk_t,
                                                     first_chunks_iter_p->u.pool_gc.next_first_cp))
      {
        JERRY_ASSERT (!first_chunks_iter_p->u.pool_gc.traversal_check_flag);

        first_chunks_iter_p->u.pool_gc.traversal_check_flag = true;
      }
    }

    {
      for (mem_pool_chunk_t *non_first_chunks_iter_p = non_first_chunks_list_p, *next_p;
           non_first_chunks_iter_p != NULL;
           non_first_chunks_iter_p = next_p)
      {
        next_p = non_first_chunks_iter_p->u.free.next_p;

        mem_pool_chunk_t *pool_start_p;
        pool_start_p = (mem_pool_chunk_t *) mem_heap_get_chunked_block_start (non_first_chunks_iter_p);

        uint16_t magic_num_field;
        bool traversal_check_flag;

        mem_pools_collect_read_magic_num_and_flag (pool_start_p, &magic_num_field, &traversal_check_flag);

        JERRY_ASSERT (magic_num_field == hint_magic_num_value);

#ifndef JERRY_DISABLE_HEAVY_DEBUG
        bool is_occured = false;

        for (mem_pool_chunk_t *first_chunks_iter_p = first_chunks_list_p;
             first_chunks_iter_p != NULL;
             first_chunks_iter_p = MEM_CP_GET_POINTER (mem_pool_chunk_t,
                                                       first_chunks_iter_p->u.pool_gc.next_first_cp))
        {
          if (pool_start_p == first_chunks_iter_p)
          {
            is_occured = true;
            break;
          }
        }

        JERRY_ASSERT (is_occured == traversal_check_flag);
#endif /* !JERRY_DISABLE_HEAVY_DEBUG */

        /*
         * During this traversal the flag in the free header chunks is in set state
         *
         * If the flag is set, it is guaranteed that the pool-first chunk,
         * from the same pool, as the current non-pool-first chunk, is free
         * and is placed in the corresponding list of free pool-first chunks.
         */
        if (traversal_check_flag)
        {
          pool_start_p->u.pool_gc.free_chunks_num++;

          non_first_chunks_iter_p->u.free.next_p = MEM_CP_GET_POINTER (mem_pool_chunk_t,
                                                                       pool_start_p->u.pool_gc.free_list_cp);
          MEM_CP_SET_NON_NULL_POINTER (pool_start_p->u.pool_gc.free_list_cp, non_first_chunks_iter_p);
        }
        else
        {
          non_first_chunks_iter_p->u.free.next_p = mem_free_chunk_p;
          mem_free_chunk_p = non_first_chunks_iter_p;
        }
      }
    }

    non_first_chunks_list_p = NULL;
  }

  /*
   * At third stage we check each free pool-first chunk in collection-time list for counted
   * number of free chunks in the pool, containing the chunk.
   *
   * If the number is equal to number of chunks in the pool - then the pool is empty, and so is freed,
   * otherwise - free chunks of the pool are returned to the common list of free chunks.
   */
  for (mem_pool_chunk_t *first_chunks_iter_p = first_chunks_list_p, *next_p;
       first_chunks_iter_p != NULL;
       first_chunks_iter_p = next_p)
  {
    next_p = MEM_CP_GET_POINTER (mem_pool_chunk_t,
                                 first_chunks_iter_p->u.pool_gc.next_first_cp);

    JERRY_ASSERT (first_chunks_iter_p->u.pool_gc.hint_magic_num == hint_magic_num_value);
    JERRY_ASSERT (first_chunks_iter_p->u.pool_gc.traversal_check_flag);
    JERRY_ASSERT (first_chunks_iter_p->u.pool_gc.free_chunks_num <= MEM_POOL_CHUNKS_NUMBER);

    if (first_chunks_iter_p->u.pool_gc.free_chunks_num == MEM_POOL_CHUNKS_NUMBER)
    {
#ifndef JERRY_NDEBUG
      mem_free_chunks_number -= MEM_POOL_CHUNKS_NUMBER;
#endif /* !JERRY_NDEBUG */

      MEM_HEAP_VALGRIND_FREYA_MEMPOOL_REQUEST ();
      mem_heap_free_block (first_chunks_iter_p);

      MEM_POOLS_STAT_FREE_POOL ();
    }
    else
    {
      mem_pool_chunk_t *first_chunk_p = first_chunks_iter_p;

      /*
       * Convert layout of first chunk from collection-time pool-first chunk's layout to the common free chunk layout
       */
      first_chunk_p->u.free.next_p = MEM_CP_GET_POINTER (mem_pool_chunk_t,
                                                         first_chunks_iter_p->u.pool_gc.free_list_cp);

      /*
       * Link local pool's list of free chunks into the common list of free chunks
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

  MEM_HEAP_VALGRIND_FREYA_MEMPOOL_REQUEST ();
  mem_pool_chunk_t *pool_start_p = (mem_pool_chunk_t*) mem_heap_alloc_chunked_block (MEM_HEAP_ALLOC_LONG_TERM);

  if (mem_free_chunk_p != NULL)
  {
    /* some chunks were freed due to GC invoked by heap allocator */
    MEM_HEAP_VALGRIND_FREYA_MEMPOOL_REQUEST ();
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
#ifdef MEM_GC_BEFORE_EACH_ALLOC
  mem_run_try_to_give_memory_back_callbacks (MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_HIGH);
#endif /* MEM_GC_BEFORE_EACH_ALLOC */

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

      VALGRIND_FREYA_MALLOCLIKE_SPACE (chunk_p, MEM_POOL_CHUNK_SIZE);
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

  VALGRIND_FREYA_FREELIKE_SPACE (chunk_to_free_p);
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
