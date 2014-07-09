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
 * \addtogroup pool Memory pool
 * @{
 */

/**
 * Memory pool implementation
 */

#define JERRY_MEM_POOL_INTERNAL

#include "globals.h"
#include "jerry-libc.h"
#include "mem-allocator.h"
#include "mem-pool.h"

/**
 * Magic number to fill free chunks in debug version
 */
static const uint8_t mem_PoolFreeChunkMagicNum = 0x71;

/**
 * Number of bits in a single bitmap's bits' block.
 */
static const mword_t mem_BitmapBitsInBlock = sizeof (mword_t) * JERRY_BITSINBYTE;

static void mem_CheckPool( mem_PoolState_t *pPool);

/**
 * Initialization of memory pool.
 * 
 * Pool will be located in the segment [poolStart; poolStart + poolSize).
 * Part of pool space will be used for bitmap and the rest will store chunks.
 * 
 * Warning:
 *         it is incorrect to suppose, that chunk number = poolSize / chunkSize.
 */
void
mem_PoolInit(mem_PoolState_t *pPool, /**< pool */
             size_t chunkSize,       /**< size of one chunk */
             uint8_t *poolStart,     /**< start of pool space */
             size_t poolSize)        /**< pool space size */
{
    JERRY_ASSERT( pPool != NULL );
    JERRY_ASSERT( (uintptr_t) poolStart % MEM_ALIGNMENT == 0);
    JERRY_ASSERT( chunkSize % MEM_ALIGNMENT == 0 );

    pPool->m_pPoolStart = poolStart;
    pPool->m_PoolSize = poolSize;
    pPool->m_ChunkSize = chunkSize;

    const size_t bitsInByte = JERRY_BITSINBYTE;
    const size_t bitmapAreaSizeAlignment = JERRY_MAX( sizeof (mword_t), MEM_ALIGNMENT);

    /*
     * Calculation chunks number
     */

    size_t bitmapAreaSize = 0;
    size_t chunksAreaSize = JERRY_ALIGNDOWN( poolSize - bitmapAreaSize, chunkSize);
    size_t chunksNumber = chunksAreaSize / chunkSize;

    /* while there is not enough area to hold state of all chunks*/
    while ( bitmapAreaSize * bitsInByte < chunksNumber )
    {
        JERRY_ASSERT( bitmapAreaSize + chunksAreaSize <= poolSize );

        /* correct bitmap area's size and, accordingly, chunks' area's size*/

        size_t newBitmapAreaSize = bitmapAreaSize + bitmapAreaSizeAlignment;
        size_t newChunksAreaSize = JERRY_ALIGNDOWN( poolSize - newBitmapAreaSize, chunkSize);
        size_t newChunksNumber = newChunksAreaSize / chunkSize;

        bitmapAreaSize = newBitmapAreaSize;
        chunksAreaSize = newChunksAreaSize;
        chunksNumber = newChunksNumber;
    }

    /*
     * Final calculation checks
     */
    JERRY_ASSERT( bitmapAreaSize * bitsInByte >= chunksNumber );
    JERRY_ASSERT( chunksAreaSize >= chunksNumber * chunkSize );
    JERRY_ASSERT( bitmapAreaSize + chunksAreaSize <= poolSize );

    pPool->m_pBitmap = (mword_t*) poolStart;
    pPool->m_pChunks = poolStart + bitmapAreaSize;

    JERRY_ASSERT( (uintptr_t) pPool->m_pChunks % MEM_ALIGNMENT == 0 );

    pPool->m_ChunksNumber = chunksNumber;

    /*
     * All chunks are free right after initialization
     */
    pPool->m_FreeChunksNumber = chunksNumber;
    __memset( pPool->m_pBitmap, 0, bitmapAreaSize);

#ifndef JERRY_NDEBUG
    __memset( pPool->m_pChunks, mem_PoolFreeChunkMagicNum, chunksAreaSize);
#endif /* JERRY_NDEBUG */

    mem_CheckPool( pPool);
} /* mem_PoolInit */

/**
 * Allocate a chunk in the pool
 */
uint8_t*
mem_PoolAllocChunk(mem_PoolState_t *pPool) /**< pool */
{
    mem_CheckPool( pPool);

    if ( pPool->m_FreeChunksNumber == 0 )
    {
        return NULL;
    }

    size_t chunkIndex = 0;
    size_t bitmapBlockIndex = 0;

    while ( chunkIndex < pPool->m_ChunksNumber )
    {
        if ( ~pPool->m_pBitmap[ bitmapBlockIndex ] != 0 )
        {
            break;
        } else
        {
            bitmapBlockIndex++;
            chunkIndex += mem_BitmapBitsInBlock;
        }
    }

    if ( chunkIndex >= pPool->m_ChunksNumber )
    {
        /* no free chunks */
        return NULL;
    }

    /* found bitmap block with a zero bit */

    mword_t bit = 1;
    for ( size_t bitIndex = 0;
          bitIndex < mem_BitmapBitsInBlock && chunkIndex < pPool->m_ChunksNumber;
          bitIndex++, chunkIndex++, bit <<= 1 )
    {
        if ( ~pPool->m_pBitmap[ bitmapBlockIndex ] & bit )
        {
            /* found free chunk */
            pPool->m_pBitmap[ bitmapBlockIndex ] |= bit;

            uint8_t *pChunk = &pPool->m_pChunks[ chunkIndex * pPool->m_ChunkSize ];
            pPool->m_FreeChunksNumber--;

            mem_CheckPool( pPool);

            return pChunk;
        }
    }

    /* that zero bit is at the end of the bitmap and doesn't correspond to any chunk */
    return NULL;
} /* mem_PoolAllocChunk */

/**
 * Free the chunk in the pool
 */
void
mem_PoolFreeChunk(mem_PoolState_t *pPool,  /**< pool */
                  uint8_t *pChunk)         /**< chunk pointer */
{
    JERRY_ASSERT( pPool->m_FreeChunksNumber < pPool->m_ChunksNumber );
    JERRY_ASSERT( pChunk >= pPool->m_pChunks && pChunk <= pPool->m_pChunks + pPool->m_ChunksNumber * pPool->m_ChunkSize );
    JERRY_ASSERT( ( (uintptr_t) pChunk - (uintptr_t) pPool->m_pChunks ) % pPool->m_ChunkSize == 0 );

    mem_CheckPool( pPool);

    size_t chunkIndex = (size_t) (pChunk - pPool->m_pChunks) / pPool->m_ChunkSize;
    size_t bitmapBlockIndex = chunkIndex / mem_BitmapBitsInBlock;
    size_t bitmapBitInBlock = chunkIndex % mem_BitmapBitsInBlock;
    mword_t bitMask = ( 1lu << bitmapBitInBlock );

#ifndef JERRY_NDEBUG
    __memset( (uint8_t*) pChunk, mem_PoolFreeChunkMagicNum, pPool->m_ChunkSize);
#endif /* JERRY_NDEBUG */
    JERRY_ASSERT( pPool->m_pBitmap[ bitmapBlockIndex ] & bitMask );

    pPool->m_pBitmap[ bitmapBlockIndex ] &= ~bitMask;
    pPool->m_FreeChunksNumber++;

    mem_CheckPool( pPool);
} /* mem_PoolFreeChunk */

/**
 * Check pool state consistency
 */
static void
mem_CheckPool( mem_PoolState_t __unused *pPool) /**< pool (unused #ifdef JERRY_NDEBUG) */
{
#ifndef JERRY_NDEBUG
    JERRY_ASSERT( pPool->m_ChunksNumber != 0 );
    JERRY_ASSERT( pPool->m_FreeChunksNumber <= pPool->m_ChunksNumber );
    JERRY_ASSERT( (uint8_t*) pPool->m_pBitmap == pPool->m_pPoolStart );
    JERRY_ASSERT( (uint8_t*) pPool->m_pChunks > pPool->m_pPoolStart );

    uint8_t freeChunkTemplate[ pPool->m_ChunkSize ];
    __memset( &freeChunkTemplate, mem_PoolFreeChunkMagicNum, sizeof (freeChunkTemplate));

    size_t metFreeChunksNumber = 0;

    for ( size_t chunkIndex = 0, bitmapBlockIndex = 0;
          chunkIndex < pPool->m_ChunksNumber;
          bitmapBlockIndex++ )
    {
        JERRY_ASSERT( (uint8_t*) & pPool->m_pBitmap[ bitmapBlockIndex ] < pPool->m_pChunks );

        mword_t bitmapBlock = pPool->m_pBitmap[ bitmapBlockIndex ];

        mword_t bitMask = 1;
        for ( size_t bitmapBitInBlock = 0;
              chunkIndex < pPool->m_ChunksNumber && bitmapBitInBlock < mem_BitmapBitsInBlock;
              bitmapBitInBlock++, bitMask <<= 1, chunkIndex++ )
        {
            if ( ~bitmapBlock & bitMask )
            {
                metFreeChunksNumber++;

                JERRY_ASSERT( __memcmp( &pPool->m_pChunks[ chunkIndex * pPool->m_ChunkSize ], freeChunkTemplate, pPool->m_ChunkSize) == 0 );
            }
        }
    }

    JERRY_ASSERT( metFreeChunksNumber == pPool->m_FreeChunksNumber );
#endif /* !JERRY_NDEBUG */
} /* mem_CheckPool */

/**
 * @}
 * @}
 */