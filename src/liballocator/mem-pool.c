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

static void mem_check_pool( mem_pool_state_t *pool_p);

/**
 * Initialization of memory pool.
 * 
 * Pool will be located in the segment [pool_start; pool_start + pool_size).
 * Part of pool space will be used for bitmap and the rest will store chunks.
 */
void
mem_pool_init(mem_pool_state_t *pool_p, /**< pool */
              size_t chunk_size,       /**< size of pool's chunk */
              uint8_t *pool_start,     /**< start of pool space */
              size_t pool_size)        /**< pool space size */
{
    JERRY_ASSERT( pool_p != NULL );
    JERRY_ASSERT( (uintptr_t) pool_start % MEM_ALIGNMENT == 0);

    pool_p->pool_start_p = pool_start;
    pool_p->pool_size = pool_size;

    switch ( chunk_size )
      {
      case 4:
        pool_p->chunk_size_log = 2;
        break;

      case 8:
        pool_p->chunk_size_log = 3;
        break;

      case 16:
        pool_p->chunk_size_log = 4;
        break;

      case 32:
        pool_p->chunk_size_log = 5;
        break;

      case 64:
        pool_p->chunk_size_log = 6;
        break;

      default:
        JERRY_UNREACHABLE();
      }

    JERRY_ASSERT( chunk_size % MEM_ALIGNMENT == 0 );
    JERRY_ASSERT( chunk_size >= sizeof(mem_pool_chunk_offset_t) );

    size_t chunks_area_size = JERRY_ALIGNDOWN( pool_size, chunk_size);
    size_t chunks_number = chunks_area_size / chunk_size;

    JERRY_ASSERT( ( (mem_pool_chunk_offset_t) chunks_number ) == chunks_number );
    pool_p->chunks_number = (mem_pool_chunk_offset_t) chunks_number;

    pool_p->first_free_chunk = 0;

    /*
     * All chunks are free right after initialization
     */
    pool_p->free_chunks_number = pool_p->chunks_number;

    for ( uint32_t chunk_index = 0;
          chunk_index < chunks_number;
          chunk_index++ )
      {
        mem_pool_chunk_offset_t *next_free_chunk_offset_p = 
          (mem_pool_chunk_offset_t*) ( pool_p->pool_start_p + chunk_size * chunk_index );

        *next_free_chunk_offset_p = chunk_index + 1;
      }

    mem_check_pool( pool_p);
} /* mem_pool_init */

/**
 * Allocate a chunk in the pool
 */
uint8_t*
mem_pool_alloc_chunk(mem_pool_state_t *pool_p) /**< pool */
{
  mem_check_pool( pool_p);

  if ( unlikely( pool_p->free_chunks_number == 0 ) )
    {
      JERRY_ASSERT( pool_p->first_free_chunk == pool_p->chunks_number );

      return NULL;
    }

  JERRY_ASSERT( pool_p->first_free_chunk < pool_p->chunks_number );

  mem_pool_chunk_offset_t chunk_index = pool_p->first_free_chunk;
  uint8_t *chunk_p = pool_p->pool_start_p + ( chunk_index << pool_p->chunk_size_log );

  mem_pool_chunk_offset_t *next_free_chunk_offset_p = (mem_pool_chunk_offset_t*) chunk_p;
  pool_p->first_free_chunk = *next_free_chunk_offset_p;
  pool_p->free_chunks_number--;

  mem_check_pool( pool_p);

  return chunk_p;
} /* mem_pool_alloc_chunk */

/**
 * Free the chunk in the pool
 */
void
mem_pool_free_chunk(mem_pool_state_t *pool_p,  /**< pool */
                    uint8_t *chunk_p)         /**< chunk pointer */
{
    JERRY_ASSERT( pool_p->free_chunks_number < pool_p->chunks_number );
    JERRY_ASSERT( chunk_p >= pool_p->pool_start_p && chunk_p <= pool_p->pool_start_p + pool_p->chunks_number * ( 1u << pool_p->chunk_size_log ) );
    JERRY_ASSERT( ( (uintptr_t) chunk_p - (uintptr_t) pool_p->pool_start_p ) % ( 1u << pool_p->chunk_size_log ) == 0 );

    mem_check_pool( pool_p);

    const size_t chunk_byte_offset = (size_t) (chunk_p - pool_p->pool_start_p);
    const mem_pool_chunk_offset_t chunk_index = (mem_pool_chunk_offset_t) (chunk_byte_offset >> pool_p->chunk_size_log);
    mem_pool_chunk_offset_t *next_free_chunk_offset_p = (mem_pool_chunk_offset_t*) chunk_p;

    *next_free_chunk_offset_p = pool_p->first_free_chunk;

    pool_p->first_free_chunk = chunk_index;
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
    JERRY_ASSERT( pool_p->chunks_number * ( 1u << pool_p->chunk_size_log ) <= pool_p->pool_size );
    JERRY_ASSERT( pool_p->free_chunks_number <= pool_p->chunks_number );

    size_t met_free_chunks_number = 0;
    mem_pool_chunk_offset_t chunk_index = pool_p->first_free_chunk;

    while ( chunk_index != pool_p->chunks_number )
    {
      uint8_t *chunk_p = pool_p->pool_start_p + ( 1u << pool_p->chunk_size_log ) * chunk_index;
      mem_pool_chunk_offset_t *next_free_chunk_offset_p = (mem_pool_chunk_offset_t*) chunk_p;

      met_free_chunks_number++;

      chunk_index = *next_free_chunk_offset_p;
    }

    JERRY_ASSERT( met_free_chunks_number == pool_p->free_chunks_number );
#endif /* !JERRY_NDEBUG */
} /* mem_check_pool */

/**
 * @}
 * @}
 */
