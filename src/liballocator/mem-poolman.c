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
mem_get_chunk_size( mem_pool_chunk_type_t chunkType) /**< chunk type */
{
    uint32_t chunkTypeId = (uint32_t) chunkType;

    JERRY_ASSERT( chunkTypeId < MEM_POOL_CHUNK_TYPE__COUNT );

    return ( 1u << ( chunkTypeId + 2 ) );
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
    size_t poolSpaceSize = mem_heap_recommend_allocation_size( 4 * sizeof (mem_pool_state_t) + sizeof (mword_t) );

    mem_space_for_pool_for_pool_headers = mem_heap_alloc_block(poolSpaceSize,
                                                        MEM_HEAP_ALLOC_LONG_TERM);

    /*
     * Get chunk type, checking that there is a type corresponding to specified size.
     */
    const mem_pool_chunk_type_t chunkType = mem_size_to_pool_chunk_type( sizeof(mem_pool_state_t));
    
    mem_pool_init(&mem_pool_for_pool_headers,
                 mem_get_chunk_size( chunkType),
                 mem_space_for_pool_for_pool_headers,
                 poolSpaceSize);

  mem_pools_stat_init();
} /* mem_pools_init */

/**
 * Allocate a chunk of specified size
 * 
 * @return pointer to allocated chunk, if allocation was successful,
 *         or NULL - if not enough memory.
 */
uint8_t*
mem_pools_alloc( mem_pool_chunk_type_t chunkType) /**< chunk type */
{
    size_t chunkSize = mem_get_chunk_size( chunkType);

    /**
     * If there are no free chunks, allocate new pool.
     */
    if ( mem_free_chunks_number[ chunkType ] == 0 )
    {
        mem_pool_state_t *poolState = (mem_pool_state_t*) mem_pool_alloc_chunk( &mem_pool_for_pool_headers);

        if ( poolState == NULL )
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
        size_t poolSpaceSize = mem_heap_recommend_allocation_size( 8 * chunkSize + sizeof (mword_t) );

        uint8_t *poolSpace = mem_heap_alloc_block( poolSpaceSize,
                                                MEM_HEAP_ALLOC_LONG_TERM);

        if ( poolSpace == NULL )
        {
            /**
             * Not enough memory.
             */
            return NULL;
        }

        mem_pool_init( poolState,
                     chunkSize,
                     poolSpace,
                     poolSpaceSize);

        poolState->pNextPool = mem_pools[ chunkType ];
        mem_pools[ chunkType ] = poolState;

        mem_free_chunks_number[ chunkType ] += poolState->FreeChunksNumber;
        
        mem_pools_stat_alloc_pool( chunkType);
    }

    /**
     * Now there is definitely at least one pool of specified type with at least one free chunk.
     * 
     * Search for the pool.
     */
    mem_pool_state_t *poolState = mem_pools[ chunkType ];

    while ( poolState->FreeChunksNumber == 0 )
    {
        poolState = poolState->pNextPool;

        JERRY_ASSERT( poolState != NULL );
    }

    /**
     * And allocate chunk within it.
     */
    mem_free_chunks_number[ chunkType ]--;

    mem_pools_stat_alloc_chunk( chunkType);

    return mem_pool_alloc_chunk( poolState);
} /* mem_pools_alloc */

/**
 * Free the chunk
 */
void
mem_pools_free( mem_pool_chunk_type_t chunkType, /**< the chunk type */
              uint8_t *pChunk) /**< pointer to the chunk */
{
    mem_pool_state_t *poolState = mem_pools[ chunkType ], *prevPoolState = NULL;

    /**
     * Search for the pool containing specified chunk.
     */
    while ( !( pChunk >= poolState->pChunks
               && pChunk <= poolState->pPoolStart + poolState->PoolSize ) )
    {
        prevPoolState = poolState;
        poolState = poolState->pNextPool;

        JERRY_ASSERT( poolState != NULL );
    }

    /**
     * Free the chunk
     */
    mem_pool_free_chunk( poolState, pChunk);
    mem_free_chunks_number[ chunkType ]++;

    mem_pools_stat_free_chunk( chunkType);

    /**
     * If all chunks of the pool are free, free the pool itself.
     */
    if ( poolState->FreeChunksNumber == poolState->ChunksNumber )
    {
        if ( prevPoolState != NULL )
        {
            prevPoolState->pNextPool = poolState->pNextPool;
        } else
        {
            mem_pools[ chunkType ] = poolState->pNextPool;
        }

        mem_free_chunks_number[ chunkType ] -= poolState->ChunksNumber;

        mem_heap_free_block( poolState->pPoolStart);

        mem_pool_free_chunk( &mem_pool_for_pool_headers, (uint8_t*) poolState);

        mem_pools_stat_free_pool( chunkType);
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
mem_pools_stat_alloc_pool( mem_pool_chunk_type_t chunkType) /**< chunk type */
{
  mem_pools_stats.pools_count[ chunkType ]++;
  mem_pools_stats.free_chunks[ chunkType ] = mem_free_chunks_number[ chunkType ];

  if ( mem_pools_stats.pools_count[ chunkType ] > mem_pools_stats.peak_pools_count[ chunkType ] )
  {
    mem_pools_stats.peak_pools_count[ chunkType ] = mem_pools_stats.pools_count[ chunkType ];
  }
} /* mem_pools_stat_alloc_pool */

/**
 * Account freeing of a pool
 */
static void
mem_pools_stat_free_pool( mem_pool_chunk_type_t chunkType) /**< chunk type */
{
  JERRY_ASSERT( mem_pools_stats.pools_count[ chunkType ] > 0 );
  
  mem_pools_stats.pools_count[ chunkType ]--;
  mem_pools_stats.free_chunks[ chunkType ] = mem_free_chunks_number[ chunkType ];
} /* mem_pools_stat_free_pool */

/**
 * Account allocation of chunk in a pool
 */
static void
mem_pools_stat_alloc_chunk( mem_pool_chunk_type_t chunkType) /**< chunk type */
{
  JERRY_ASSERT( mem_pools_stats.free_chunks[ chunkType ] > 0 );
  
  mem_pools_stats.allocated_chunks[ chunkType ]++;
  mem_pools_stats.free_chunks[ chunkType ]--;

  if ( mem_pools_stats.allocated_chunks[ chunkType ] > mem_pools_stats.peak_allocated_chunks[ chunkType ] )
  {
    mem_pools_stats.peak_allocated_chunks[ chunkType ] = mem_pools_stats.allocated_chunks[ chunkType ];
  }
} /* mem_pools_stat_alloc_chunk */

/**
 * Account freeing of chunk in a pool
 */
static void
mem_pools_stat_free_chunk( mem_pool_chunk_type_t chunkType) /**< chunk type */
{
  JERRY_ASSERT( mem_pools_stats.allocated_chunks[ chunkType ] > 0 );

  mem_pools_stats.allocated_chunks[ chunkType ]--;
  mem_pools_stats.free_chunks[ chunkType ]++;
} /* mem_pools_stat_free_chunk */
#endif /* MEM_STATS */

/**
 * @}
 */
/**
 * @}
 */
