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
 * Lists of pools for possible chunk sizes
 */
mem_pool_state_t *mem_pools[ MEM_POOL_CHUNK_TYPE__COUNT ];

/**
 * Number of free chunks of possible chunk sizes
 */
size_t mem_free_chunks_number[ MEM_POOL_CHUNK_TYPE__COUNT ];

/**
 * Pool, containing pool headers
 */
mem_pool_state_t mem_pool_for_pool_headers;

/**
 * Space for pool, containing pool headers
 */
uint8_t *mem_space_for_pool_for_pool_headers;

#ifdef MEM_STATS
/**
* Pools' memory usage statistics
 */
mem_pools_stats_t mem_pools_stats;

static void mem_pools_stat_init( void);
static void mem_pools_stat_alloc_pool( mem_pool_chunk_type_t);
static void mem_pools_stat_free_pool( mem_pool_chunk_type_t);
static void mem_pools_stat_alloc_chunk( mem_pool_chunk_type_t);
static void mem_pools_stat_free_chunk( mem_pool_chunk_type_t);
#else /* !MEM_STATS */
#  define mem_pools_stat_init()
#  define mem_pools_stat_alloc_pool(v)
#  define mem_pools_stat_free_pool(v)
#  define mem_pools_stat_alloc_chunk(v)
#  define mem_pools_stat_free_chunk(v)
#endif /* !MEM_STATS */

/**
 * Get chunk size from chunk type.
 * 
 * @return size (in bytes) of chunk of specified type
 */
size_t
mem_get_chunk_size( mem_pool_chunk_type_t chunk_type) /**< chunk type */
{
    uint32_t chunk_type_id = (uint32_t) chunk_type;

    JERRY_ASSERT( chunk_type_id < MEM_POOL_CHUNK_TYPE__COUNT );

    return ( 1u << ( chunk_type_id + 2 ) );
} /* mem_get_chunk_size */

/**
 * Initialize pool manager
 */
void
mem_pools_init(void)
{
    for ( uint32_t i = 0; i < MEM_POOL_CHUNK_TYPE__COUNT; i++ )
    {
        mem_pools[ i ] = NULL;
        mem_free_chunks_number[ i ] = 0;
    }

    /**
     * Space, at least for four pool headers and a bitmap entry.
     * 
     * TODO: Research.
     */
    size_t pool_space_size = mem_heap_recommend_allocation_size( 4 * sizeof (mem_pool_state_t) + sizeof (mword_t) );

    mem_space_for_pool_for_pool_headers = mem_heap_alloc_block(pool_space_size,
                                                        MEM_HEAP_ALLOC_LONG_TERM);

    /*
     * Get chunk type, checking that there is a type corresponding to specified size.
     */
    const mem_pool_chunk_type_t chunk_type = mem_size_to_pool_chunk_type( sizeof(mem_pool_state_t));
    
    mem_pool_init(&mem_pool_for_pool_headers,
                 mem_get_chunk_size( chunk_type),
                 mem_space_for_pool_for_pool_headers,
                 pool_space_size);

  mem_pools_stat_init();
} /* mem_pools_init */

/**
 * Finalize pool manager
 */
void
mem_pools_finalize(void)
{
  for ( uint32_t i = 0; i < MEM_POOL_CHUNK_TYPE__COUNT; i++ )
    {
      JERRY_ASSERT( mem_pools[ i ] == NULL );
      JERRY_ASSERT( mem_free_chunks_number[ i ] == 0 );
    }

  JERRY_ASSERT( mem_pool_for_pool_headers.chunks_number == mem_pool_for_pool_headers.free_chunks_number );

  __memset( &mem_pool_for_pool_headers, 0, sizeof(mem_pool_for_pool_headers));

  mem_heap_free_block( mem_space_for_pool_for_pool_headers);
  mem_space_for_pool_for_pool_headers = NULL;
} /* mem_pools_finalize */

/**
 * Allocate a chunk of specified size
 * 
 * @return pointer to allocated chunk, if allocation was successful,
 *         or NULL - if not enough memory.
 */
uint8_t*
mem_pools_alloc( mem_pool_chunk_type_t chunk_type) /**< chunk type */
{
    size_t chunk_size = mem_get_chunk_size( chunk_type);

    /**
     * If there are no free chunks, allocate new pool.
     */
    if ( mem_free_chunks_number[ chunk_type ] == 0 )
    {
        mem_pool_state_t *pool_state = (mem_pool_state_t*) mem_pool_alloc_chunk( &mem_pool_for_pool_headers);

        if ( pool_state == NULL )
        {
            /**
             * Not enough space for new pool' header.
             */
            return NULL;
        }

        /**
         * Space, at least for eight chunks and a bitmap entry.
         * 
         * TODO: Research.
         */
        size_t pool_space_size = mem_heap_recommend_allocation_size( 8 * chunk_size + sizeof (mword_t) );

        uint8_t *pool_space = mem_heap_alloc_block( pool_space_size,
                                                MEM_HEAP_ALLOC_LONG_TERM);

        if ( pool_space == NULL )
          {
            /**
             * Not enough memory. Freeing pool header that was allocated above.
             */

            mem_pool_free_chunk( &mem_pool_for_pool_headers, (uint8_t*) pool_state);

            return NULL;
          }

        mem_pool_init( pool_state,
                     chunk_size,
                     pool_space,
                     pool_space_size);

        pool_state->next_pool_p = mem_pools[ chunk_type ];
        mem_pools[ chunk_type ] = pool_state;

        mem_free_chunks_number[ chunk_type ] += pool_state->free_chunks_number;
        
        mem_pools_stat_alloc_pool( chunk_type);
    }

    /**
     * Now there is definitely at least one pool of specified type with at least one free chunk.
     * 
     * Search for the pool.
     */
    mem_pool_state_t *pool_state = mem_pools[ chunk_type ];

    while ( pool_state->free_chunks_number == 0 )
    {
        pool_state = pool_state->next_pool_p;

        JERRY_ASSERT( pool_state != NULL );
    }

    /**
     * And allocate chunk within it.
     */
    mem_free_chunks_number[ chunk_type ]--;

    mem_pools_stat_alloc_chunk( chunk_type);

    return mem_pool_alloc_chunk( pool_state);
} /* mem_pools_alloc */

/**
 * Free the chunk
 */
void
mem_pools_free(mem_pool_chunk_type_t chunk_type, /**< the chunk type */
               uint8_t *chunk_p) /**< pointer to the chunk */
{
    mem_pool_state_t *pool_state = mem_pools[ chunk_type ], *prev_pool_state = NULL;

    /**
     * Search for the pool containing specified chunk.
     */
    while ( !( chunk_p >= pool_state->chunks_p
               && chunk_p <= pool_state->pool_start_p + pool_state->pool_size ) )
    {
        prev_pool_state = pool_state;
        pool_state = pool_state->next_pool_p;

        JERRY_ASSERT( pool_state != NULL );
    }

    /**
     * Free the chunk
     */
    mem_pool_free_chunk( pool_state, chunk_p);
    mem_free_chunks_number[ chunk_type ]++;

    mem_pools_stat_free_chunk( chunk_type);

    /**
     * If all chunks of the pool are free, free the pool itself.
     */
    if ( pool_state->free_chunks_number == pool_state->chunks_number )
    {
        if ( prev_pool_state != NULL )
        {
            prev_pool_state->next_pool_p = pool_state->next_pool_p;
        } else
        {
            mem_pools[ chunk_type ] = pool_state->next_pool_p;
        }

        mem_free_chunks_number[ chunk_type ] -= pool_state->chunks_number;

        mem_heap_free_block( pool_state->pool_start_p);

        mem_pool_free_chunk( &mem_pool_for_pool_headers, (uint8_t*) pool_state);

        mem_pools_stat_free_pool( chunk_type);
  }
} /* mem_pools_free */

#ifdef MEM_STATS
/**
 * Get pools memory usage statistics
 */
void
mem_pools_get_stats( mem_pools_stats_t *out_pools_stats_p) /**< out: pools' stats */
{
  JERRY_ASSERT( out_pools_stats_p != NULL );

  *out_pools_stats_p = mem_pools_stats;
} /* mem_pools_get_stats */

/**
 * Initalize pools' memory usage statistics account structure
 */
static void
mem_pools_stat_init( void)
{
  __memset( &mem_pools_stats, 0, sizeof (mem_pools_stats));
} /* mem_pools_stat_init */

/**
 * Account allocation of a pool
 */
static void
mem_pools_stat_alloc_pool( mem_pool_chunk_type_t chunk_type) /**< chunk type */
{
  mem_pools_stats.pools_count[ chunk_type ]++;
  mem_pools_stats.free_chunks[ chunk_type ] = mem_free_chunks_number[ chunk_type ];

  if ( mem_pools_stats.pools_count[ chunk_type ] > mem_pools_stats.peak_pools_count[ chunk_type ] )
  {
    mem_pools_stats.peak_pools_count[ chunk_type ] = mem_pools_stats.pools_count[ chunk_type ];
  }
} /* mem_pools_stat_alloc_pool */

/**
 * Account freeing of a pool
 */
static void
mem_pools_stat_free_pool( mem_pool_chunk_type_t chunk_type) /**< chunk type */
{
  JERRY_ASSERT( mem_pools_stats.pools_count[ chunk_type ] > 0 );
  
  mem_pools_stats.pools_count[ chunk_type ]--;
  mem_pools_stats.free_chunks[ chunk_type ] = mem_free_chunks_number[ chunk_type ];
} /* mem_pools_stat_free_pool */

/**
 * Account allocation of chunk in a pool
 */
static void
mem_pools_stat_alloc_chunk( mem_pool_chunk_type_t chunk_type) /**< chunk type */
{
  JERRY_ASSERT( mem_pools_stats.free_chunks[ chunk_type ] > 0 );
  
  mem_pools_stats.allocated_chunks[ chunk_type ]++;
  mem_pools_stats.free_chunks[ chunk_type ]--;

  if ( mem_pools_stats.allocated_chunks[ chunk_type ] > mem_pools_stats.peak_allocated_chunks[ chunk_type ] )
  {
    mem_pools_stats.peak_allocated_chunks[ chunk_type ] = mem_pools_stats.allocated_chunks[ chunk_type ];
  }
} /* mem_pools_stat_alloc_chunk */

/**
 * Account freeing of chunk in a pool
 */
static void
mem_pools_stat_free_chunk( mem_pool_chunk_type_t chunk_type) /**< chunk type */
{
  JERRY_ASSERT( mem_pools_stats.allocated_chunks[ chunk_type ] > 0 );

  mem_pools_stats.allocated_chunks[ chunk_type ]--;
  mem_pools_stats.free_chunks[ chunk_type ]++;
} /* mem_pools_stat_free_chunk */
#endif /* MEM_STATS */

/**
 * @}
 */
/**
 * @}
 */
