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
mem_PoolState_t *mem_Pools[ MEM_POOL_CHUNK_TYPE__COUNT ];

/**
 * Number of free chunks of possible chunk sizes
 */
size_t mem_FreeChunksNumber[ MEM_POOL_CHUNK_TYPE__COUNT ];

/**
 * Pool, containing pool headers
 */
mem_PoolState_t mem_PoolForPoolHeaders;

/**
 * Space for pool, containing pool headers
 */
uint8_t *mem_SpaceForPoolForPoolHeaders;

#ifdef MEM_STATS
/**
* Pools' memory usage statistics
 */
mem_PoolsStats_t mem_PoolsStats;

static void mem_PoolsStatInit( void);
static void mem_PoolsStatAllocPool( mem_PoolChunkType_t);
static void mem_PoolsStatFreePool( mem_PoolChunkType_t);
static void mem_PoolsStatAllocChunk( mem_PoolChunkType_t);
static void mem_PoolsStatFreeChunk( mem_PoolChunkType_t);
#else /* !MEM_STATS */
#  define mem_PoolsStatsInit()
#  define mem_PoolsStatAllocPool()
#  define mem_PoolsStatsFreePool()
#  define mem_PoolsStatAllocChunk()
#  define mem_PoolsStatFreeChunk()
#endif /* !MEM_STATS */

/**
 * Initialize pool manager
 */
void
mem_PoolsInit(void)
{
    for ( uint32_t i = 0; i < MEM_POOL_CHUNK_TYPE__COUNT; i++ )
    {
        mem_Pools[ i ] = NULL;
        mem_FreeChunksNumber[ i ] = 0;
    }

    /**
     * Space, at least for four pool headers and a bitmap entry.
     * 
     * TODO: Research.
     */
    size_t poolSpaceSize = mem_HeapRecommendAllocationSize( 4 * sizeof (mem_PoolState_t) + sizeof (mword_t) );

    mem_SpaceForPoolForPoolHeaders = mem_HeapAllocBlock(poolSpaceSize,
                                                        MEM_HEAP_ALLOC_LONG_TERM);

    /*
     * Get chunk type, checking that there is a type corresponding to specified size.
     */
    const mem_PoolChunkType_t chunkType = mem_SizeToPoolChunkType( sizeof(mem_PoolState_t));
    
    mem_PoolInit(&mem_PoolForPoolHeaders,
                 mem_GetChunkSize( chunkType),
                 mem_SpaceForPoolForPoolHeaders,
                 poolSpaceSize);

  mem_PoolsStatInit();
} /* mem_PoolsInit */

/**
 * Allocate a chunk of specified size
 * 
 * @return pointer to allocated chunk, if allocation was successful,
 *         or NULL - if not enough memory.
 */
uint8_t*
mem_PoolsAlloc( mem_PoolChunkType_t chunkType) /**< chunk type */
{
    size_t chunkSize = mem_GetChunkSize( chunkType);

    /**
     * If there are no free chunks, allocate new pool.
     */
    if ( mem_FreeChunksNumber[ chunkType ] == 0 )
    {
        mem_PoolState_t *poolState = (mem_PoolState_t*) mem_PoolAllocChunk( &mem_PoolForPoolHeaders);

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
        size_t poolSpaceSize = mem_HeapRecommendAllocationSize( 8 * chunkSize + sizeof (mword_t) );

        uint8_t *poolSpace = mem_HeapAllocBlock( poolSpaceSize,
                                                MEM_HEAP_ALLOC_LONG_TERM);

        if ( poolSpace == NULL )
        {
            /**
             * Not enough memory.
             */
            return NULL;
        }

        mem_PoolInit( poolState,
                     chunkSize,
                     poolSpace,
                     poolSpaceSize);

        poolState->m_pNextPool = mem_Pools[ chunkType ];
        mem_Pools[ chunkType ] = poolState;

        mem_FreeChunksNumber[ chunkType ] += poolState->m_FreeChunksNumber;
        
        mem_PoolsStatAllocPool( chunkType);
    }

    /**
     * Now there is definitely at least one pool of specified type with at least one free chunk.
     * 
     * Search for the pool.
     */
    mem_PoolState_t *poolState = mem_Pools[ chunkType ];

    while ( poolState->m_FreeChunksNumber == 0 )
    {
        poolState = poolState->m_pNextPool;

        JERRY_ASSERT( poolState != NULL );
    }

    /**
     * And allocate chunk within it.
     */
    mem_FreeChunksNumber[ chunkType ]--;

    mem_PoolsStatAllocChunk( chunkType);

    return mem_PoolAllocChunk( poolState);
} /* mem_PoolsAlloc */

/**
 * Free the chunk
 */
void
mem_PoolsFree( mem_PoolChunkType_t chunkType, /**< the chunk type */
              uint8_t *pChunk) /**< pointer to the chunk */
{
    mem_PoolState_t *poolState = mem_Pools[ chunkType ], *prevPoolState = NULL;

    /**
     * Search for the pool containing specified chunk.
     */
    while ( !( pChunk >= poolState->m_pChunks
              && pChunk <= poolState->m_pPoolStart + poolState->m_PoolSize ) )
    {
        prevPoolState = poolState;
        poolState = poolState->m_pNextPool;

        JERRY_ASSERT( poolState != NULL );
    }

    /**
     * Free the chunk
     */
    mem_PoolFreeChunk( poolState, pChunk);
    mem_FreeChunksNumber[ chunkType ]++;

    mem_PoolsStatFreeChunk( chunkType);

    /**
     * If all chunks of the pool are free, free the pool itself.
     */
    if ( poolState->m_FreeChunksNumber == poolState->m_ChunksNumber )
    {
        if ( prevPoolState != NULL )
        {
            prevPoolState->m_pNextPool = poolState->m_pNextPool;
        } else
        {
            mem_Pools[ chunkType ] = poolState->m_pNextPool;
        }

        mem_FreeChunksNumber[ chunkType ] -= poolState->m_ChunksNumber;

        mem_HeapFreeBlock( poolState->m_pPoolStart);

        mem_PoolFreeChunk( &mem_PoolForPoolHeaders, (uint8_t*) poolState);

        mem_PoolsStatFreePool( chunkType);
  }
} /* mem_PoolsFree */

#ifdef MEM_STATS
/**
 * Get pools memory usage statistics
 */
void
mem_PoolsGetStats( mem_PoolsStats_t *out_pools_stats_p) /**< out: pools' stats */
{
  JERRY_ASSERT( out_pools_stats_p != NULL );

  *out_pools_stats_p = mem_PoolsStats;
} /* mem_PoolsGetStats */

/**
 * Initalize pools' memory usage statistics account structure
 */
static void
mem_PoolsStatInit( void)
{
  libc_memset( &mem_PoolsStats, 0, sizeof (mem_PoolsStats));
} /* mem_PoolsStatInit */

/**
 * Account allocation of a pool
 */
static void
mem_PoolsStatAllocPool( mem_PoolChunkType_t chunkType) /**< chunk type */
{
  mem_PoolsStats.pools_count[ chunkType ]++;
  mem_PoolsStats.free_chunks[ chunkType ] = mem_FreeChunksNumber[ chunkType ];

  if ( mem_PoolsStats.pools_count[ chunkType ] > mem_PoolsStats.peak_pools_count[ chunkType ] )
  {
    mem_PoolsStats.peak_pools_count[ chunkType ] = mem_PoolsStats.pools_count[ chunkType ];
  }
} /* mem_PoolsStatAllocPool */

/**
 * Account freeing of a pool
 */
static void
mem_PoolsStatFreePool( mem_PoolChunkType_t chunkType) /**< chunk type */
{
  JERRY_ASSERT( mem_PoolsStats.pools_count[ chunkType ] > 0 );
  
  mem_PoolsStats.pools_count[ chunkType ]--;
  mem_PoolsStats.free_chunks[ chunkType ] = mem_FreeChunksNumber[ chunkType ];
} /* mem_PoolsStatFreePool */

/**
 * Account allocation of chunk in a pool
 */
static void
mem_PoolsStatAllocChunk( mem_PoolChunkType_t chunkType) /**< chunk type */
{
  JERRY_ASSERT( mem_PoolsStats.free_chunks[ chunkType ] > 0 );
  
  mem_PoolsStats.allocated_chunks[ chunkType ]++;
  mem_PoolsStats.free_chunks[ chunkType ]--;

  if ( mem_PoolsStats.allocated_chunks[ chunkType ] > mem_PoolsStats.peak_allocated_chunks[ chunkType ] )
  {
    mem_PoolsStats.peak_allocated_chunks[ chunkType ] = mem_PoolsStats.allocated_chunks[ chunkType ];
  }
} /* mem_PoolsStatAllocChunk */

/**
 * Account freeing of chunk in a pool
 */
static void
mem_PoolsStatFreeChunk( mem_PoolChunkType_t chunkType) /**< chunk type */
{
  JERRY_ASSERT( mem_PoolsStats.allocated_chunks[ chunkType ] > 0 );

  mem_PoolsStats.allocated_chunks[ chunkType ]--;
  mem_PoolsStats.free_chunks[ chunkType ]++;
} /* mem_PoolsStatFreeChunk */
#endif /* MEM_STATS */

/**
 * @}
 */
/**
 * @}
 */
