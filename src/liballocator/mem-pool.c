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
static const uint8_t mem_pool_free_chunk_magic_num = 0x71;

/**
 * Number of bits in a single bitmap's bits' block.
 */
static const mword_t mem_bitmap_bits_in_block = sizeof (mword_t) * JERRY_BITSINBYTE;

static void mem_check_pool( mem_pool_state_t *pool_p);

/**
 * Initialization of memory pool.
 * 
 * Pool will be located in the segment [pool_start; pool_start + pool_size).
 * Part of pool space will be used for bitmap and the rest will store chunks.
 * 
 * Warning:
 *         it is incorrect to suppose, that chunk number = pool_size / chunk_size.
 */
void
mem_pool_init(mem_pool_state_t *pool_p, /**< pool */
             size_t chunk_size,       /**< size of one chunk */
             uint8_t *pool_start,     /**< start of pool space */
             size_t pool_size)        /**< pool space size */
{
    JERRY_ASSERT( pool_p != NULL );
    JERRY_ASSERT( (uintptr_t) pool_start % MEM_ALIGNMENT == 0);
    JERRY_ASSERT( chunk_size % MEM_ALIGNMENT == 0 );

    pool_p->pool_start_p = pool_start;
    pool_p->pool_size = pool_size;
    pool_p->chunk_size = chunk_size;

    const size_t bits_in_byte = JERRY_BITSINBYTE;
    const size_t bitmap_area_size_alignment = JERRY_MAX( sizeof (mword_t), MEM_ALIGNMENT);

    /*
     * Calculation chunks number
     */

    size_t bitmap_area_size = 0;
    size_t chunks_area_size = JERRY_ALIGNDOWN( pool_size - bitmap_area_size, chunk_size);
    size_t chunks_number = chunks_area_size / chunk_size;

    /* while there is not enough area to hold state of all chunks*/
    while ( bitmap_area_size * bits_in_byte < chunks_number )
    {
        JERRY_ASSERT( bitmap_area_size + chunks_area_size <= pool_size );

        /* correct bitmap area's size and, accordingly, chunks' area's size*/

        size_t new_bitmap_area_size = bitmap_area_size + bitmap_area_size_alignment;
        size_t new_chunks_area_size = JERRY_ALIGNDOWN( pool_size - new_bitmap_area_size, chunk_size);
        size_t new_chunks_number = new_chunks_area_size / chunk_size;

        bitmap_area_size = new_bitmap_area_size;
        chunks_area_size = new_chunks_area_size;
        chunks_number = new_chunks_number;
    }

    /*
     * Final calculation checks
     */
    JERRY_ASSERT( bitmap_area_size * bits_in_byte >= chunks_number );
    JERRY_ASSERT( chunks_area_size >= chunks_number * chunk_size );
    JERRY_ASSERT( bitmap_area_size + chunks_area_size <= pool_size );

    pool_p->bitmap_p = (mword_t*) pool_start;
    pool_p->chunks_p = pool_start + bitmap_area_size;

    JERRY_ASSERT( (uintptr_t) pool_p->chunks_p % MEM_ALIGNMENT == 0 );

    pool_p->chunks_number = chunks_number;

    /*
     * All chunks are free right after initialization
     */
    pool_p->free_chunks_number = chunks_number;
    __memset( pool_p->bitmap_p, 0, bitmap_area_size);

#ifndef JERRY_NDEBUG
    __memset( pool_p->chunks_p, mem_pool_free_chunk_magic_num, chunks_area_size);
#endif /* JERRY_NDEBUG */

    mem_check_pool( pool_p);
} /* mem_pool_init */

/**
 * Allocate a chunk in the pool
 */
uint8_t*
mem_pool_alloc_chunk(mem_pool_state_t *pool_p) /**< pool */
{
    mem_check_pool( pool_p);

    if ( pool_p->free_chunks_number == 0 )
    {
        return NULL;
    }

    size_t chunk_index = 0;
    size_t bitmap_block_index = 0;

    while ( chunk_index < pool_p->chunks_number )
    {
        if ( ~pool_p->bitmap_p[ bitmap_block_index ] != 0 )
        {
            break;
        } else
        {
            bitmap_block_index++;
            chunk_index += mem_bitmap_bits_in_block;
        }
    }

    if ( chunk_index >= pool_p->chunks_number )
    {
        /* no free chunks */
        return NULL;
    }

    /* found bitmap block with a zero bit */

    mword_t bit = 1;
    for ( size_t bit_index = 0;
          bit_index < mem_bitmap_bits_in_block && chunk_index < pool_p->chunks_number;
          bit_index++, chunk_index++, bit <<= 1 )
    {
        if ( ~pool_p->bitmap_p[ bitmap_block_index ] & bit )
        {
            /* found free chunk */
            pool_p->bitmap_p[ bitmap_block_index ] |= bit;

            uint8_t *chunk_p = &pool_p->chunks_p[ chunk_index * pool_p->chunk_size ];
            pool_p->free_chunks_number--;

            mem_check_pool( pool_p);

            return chunk_p;
        }
    }

    /* that zero bit is at the end of the bitmap and doesn't correspond to any chunk */
    return NULL;
} /* mem_pool_alloc_chunk */

/**
 * Free the chunk in the pool
 */
void
mem_pool_free_chunk(mem_pool_state_t *pool_p,  /**< pool */
                  uint8_t *chunk_p)         /**< chunk pointer */
{
    JERRY_ASSERT( pool_p->free_chunks_number < pool_p->chunks_number );
    JERRY_ASSERT( chunk_p >= pool_p->chunks_p && chunk_p <= pool_p->chunks_p + pool_p->chunks_number * pool_p->chunk_size );
    JERRY_ASSERT( ( (uintptr_t) chunk_p - (uintptr_t) pool_p->chunks_p ) % pool_p->chunk_size == 0 );

    mem_check_pool( pool_p);

    size_t chunk_index = (size_t) (chunk_p - pool_p->chunks_p) / pool_p->chunk_size;
    size_t bitmap_block_index = chunk_index / mem_bitmap_bits_in_block;
    size_t bitmap_bit_in_block = chunk_index % mem_bitmap_bits_in_block;
    mword_t bit_mask = ( 1lu << bitmap_bit_in_block );

#ifndef JERRY_NDEBUG
    __memset( (uint8_t*) chunk_p, mem_pool_free_chunk_magic_num, pool_p->chunk_size);
#endif /* JERRY_NDEBUG */
    JERRY_ASSERT( pool_p->bitmap_p[ bitmap_block_index ] & bit_mask );

    pool_p->bitmap_p[ bitmap_block_index ] &= ~bit_mask;
    pool_p->free_chunks_number++;

    mem_check_pool( pool_p);
} /* mem_pool_free_chunk */

/**
 * Check pool state consistency
 */
static void
mem_check_pool( mem_pool_state_t __unused *pool_p) /**< pool (unused #ifdef JERRY_NDEBUG) */
{
#ifndef JERRY_NDEBUG
    JERRY_ASSERT( pool_p->chunks_number != 0 );
    JERRY_ASSERT( pool_p->free_chunks_number <= pool_p->chunks_number );
    JERRY_ASSERT( (uint8_t*) pool_p->bitmap_p == pool_p->pool_start_p );
    JERRY_ASSERT( (uint8_t*) pool_p->chunks_p > pool_p->pool_start_p );

    uint8_t free_chunk_template[ pool_p->chunk_size ];
    __memset( &free_chunk_template, mem_pool_free_chunk_magic_num, sizeof (free_chunk_template));

    size_t met_free_chunks_number = 0;

    for ( size_t chunk_index = 0, bitmap_block_index = 0;
          chunk_index < pool_p->chunks_number;
          bitmap_block_index++ )
    {
        JERRY_ASSERT( (uint8_t*) & pool_p->bitmap_p[ bitmap_block_index ] < pool_p->chunks_p );

        mword_t bitmap_block = pool_p->bitmap_p[ bitmap_block_index ];

        mword_t bit_mask = 1;
        for ( size_t bitmap_bit_in_block = 0;
              chunk_index < pool_p->chunks_number && bitmap_bit_in_block < mem_bitmap_bits_in_block;
              bitmap_bit_in_block++, bit_mask <<= 1, chunk_index++ )
        {
            if ( ~bitmap_block & bit_mask )
            {
                met_free_chunks_number++;

                JERRY_ASSERT( __memcmp( &pool_p->chunks_p[ chunk_index * pool_p->chunk_size ], free_chunk_template, pool_p->chunk_size) == 0 );
            }
        }
    }

    JERRY_ASSERT( met_free_chunks_number == pool_p->free_chunks_number );
#endif /* !JERRY_NDEBUG */
} /* mem_check_pool */

/**
 * @}
 * @}
 */