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
 * \addtogroup pool Memory pool
 * @{
 */

/**
 * Memory pool implementation
 */

#define JERRY_MEM_POOL_INTERNAL

#include "jrt.h"
#include "jrt-libc-includes.h"
#include "mem-allocator.h"
#include "mem-pool.h"

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

static void mem_check_pool (mem_pool_state_t *pool_p);

/**
 * Get address of pool chunk with specified index
 */
#define MEM_POOL_CHUNK_ADDRESS(pool_header_p, chunk_index) ((uint8_t*) (MEM_POOL_SPACE_START(pool_p) + \
                                                                        MEM_POOL_CHUNK_SIZE * chunk_index))

/**
 * Is the chunk is inside of the pool?
 *
 * @return true / false
 */
bool __attr_const___
mem_pool_is_chunk_inside (mem_pool_state_t *pool_p, /**< pool */
                          uint8_t *chunk_p) /**< chunk */
{
  if (chunk_p >= (uint8_t*) pool_p && chunk_p < (uint8_t*) pool_p + MEM_POOL_SIZE)
  {
    JERRY_ASSERT (chunk_p >= MEM_POOL_SPACE_START(pool_p)
                  && chunk_p <= MEM_POOL_SPACE_START(pool_p) + MEM_POOL_CHUNKS_NUMBER * MEM_POOL_CHUNK_SIZE);

    return true;
  }

  return false;
} /* mem_pool_is_chunk_inside */

/**
 * Initialization of memory pool.
 *
 * Pool will be located in the segment [pool_start; pool_start + pool_size).
 * Part of pool space will be used for bitmap and the rest will store chunks.
 */
void
mem_pool_init (mem_pool_state_t *pool_p, /**< pool */
               size_t pool_size)         /**< pool size */
{
  JERRY_ASSERT(pool_p != NULL);
  JERRY_ASSERT((size_t)MEM_POOL_SPACE_START(pool_p) % MEM_ALIGNMENT == 0);

  JERRY_STATIC_ASSERT(MEM_POOL_CHUNK_SIZE % MEM_ALIGNMENT == 0);
  JERRY_STATIC_ASSERT(MEM_POOL_MAX_CHUNKS_NUMBER_LOG <= sizeof (mem_pool_chunk_index_t) * JERRY_BITSINBYTE);
  JERRY_ASSERT(sizeof (mem_pool_chunk_index_t) <= MEM_POOL_CHUNK_SIZE);

  JERRY_ASSERT (MEM_POOL_SIZE == sizeof (mem_pool_state_t) + MEM_POOL_CHUNKS_NUMBER * MEM_POOL_CHUNK_SIZE);
  JERRY_ASSERT (MEM_POOL_CHUNKS_NUMBER >= CONFIG_MEM_LEAST_CHUNK_NUMBER_IN_POOL);

  JERRY_ASSERT (pool_size == MEM_POOL_SIZE);

  /*
   * All chunks are free right after initialization
   */
  pool_p->free_chunks_number = (mem_pool_chunk_index_t) MEM_POOL_CHUNKS_NUMBER;
  JERRY_ASSERT (pool_p->free_chunks_number == MEM_POOL_CHUNKS_NUMBER);

  /*
   * Chunk with zero index is first free chunk in the pool now
   */
  pool_p->first_free_chunk = 0;

  for (mem_pool_chunk_index_t chunk_index = 0;
       chunk_index < MEM_POOL_CHUNKS_NUMBER;
       chunk_index++)
  {
    mem_pool_chunk_index_t *next_free_chunk_index_p = (mem_pool_chunk_index_t*) MEM_POOL_CHUNK_ADDRESS(pool_p,
                                                                                                       chunk_index);

    *next_free_chunk_index_p = (mem_pool_chunk_index_t) (chunk_index + 1u);

    VALGRIND_NOACCESS_SPACE (next_free_chunk_index_p, MEM_POOL_CHUNK_SIZE);
  }

  mem_check_pool (pool_p);
} /* mem_pool_init */

/**
 * Allocate a chunk in the pool
 */
uint8_t*
mem_pool_alloc_chunk (mem_pool_state_t *pool_p) /**< pool */
{
  mem_check_pool (pool_p);

  JERRY_ASSERT (pool_p->free_chunks_number != 0);
  JERRY_ASSERT (pool_p->first_free_chunk < MEM_POOL_CHUNKS_NUMBER);

  mem_pool_chunk_index_t chunk_index = pool_p->first_free_chunk;
  uint8_t *chunk_p = MEM_POOL_CHUNK_ADDRESS(pool_p, chunk_index);

  VALGRIND_DEFINED_SPACE (chunk_p, MEM_POOL_CHUNK_SIZE);

  mem_pool_chunk_index_t *next_free_chunk_index_p = (mem_pool_chunk_index_t*) chunk_p;
  pool_p->first_free_chunk = *next_free_chunk_index_p;
  pool_p->free_chunks_number--;

  VALGRIND_UNDEFINED_SPACE (chunk_p, MEM_POOL_CHUNK_SIZE);

  mem_check_pool (pool_p);

  return chunk_p;
} /* mem_pool_alloc_chunk */

/**
 * Free the chunk in the pool
 */
void
mem_pool_free_chunk (mem_pool_state_t *pool_p,  /**< pool */
                     uint8_t *chunk_p)         /**< chunk pointer */
{
  JERRY_ASSERT(pool_p->free_chunks_number < MEM_POOL_CHUNKS_NUMBER);
  JERRY_ASSERT(mem_pool_is_chunk_inside (pool_p, chunk_p));
  JERRY_ASSERT(((uintptr_t) chunk_p - (uintptr_t) MEM_POOL_SPACE_START(pool_p)) % MEM_POOL_CHUNK_SIZE == 0);

  mem_check_pool (pool_p);

  const size_t chunk_byte_offset = (size_t) (chunk_p - MEM_POOL_SPACE_START(pool_p));
  const mem_pool_chunk_index_t chunk_index = (mem_pool_chunk_index_t) (chunk_byte_offset / MEM_POOL_CHUNK_SIZE);

  mem_pool_chunk_index_t *next_free_chunk_index_p = (mem_pool_chunk_index_t*) chunk_p;

  *next_free_chunk_index_p = pool_p->first_free_chunk;

  pool_p->first_free_chunk = chunk_index;
  pool_p->free_chunks_number++;

  VALGRIND_NOACCESS_SPACE (next_free_chunk_index_p, MEM_POOL_CHUNK_SIZE);

  mem_check_pool (pool_p);
} /* mem_pool_free_chunk */

/**
 * Check pool state consistency
 */
static void
mem_check_pool (mem_pool_state_t __attr_unused___ *pool_p) /**< pool (unused #ifdef JERRY_NDEBUG) */
{
#ifndef JERRY_NDEBUG
  JERRY_ASSERT(pool_p->free_chunks_number <= MEM_POOL_CHUNKS_NUMBER);

  size_t met_free_chunks_number = 0;
  mem_pool_chunk_index_t chunk_index = pool_p->first_free_chunk;

  while (chunk_index != MEM_POOL_CHUNKS_NUMBER)
  {
    uint8_t *chunk_p = MEM_POOL_CHUNK_ADDRESS(pool_p, chunk_index);
    mem_pool_chunk_index_t *next_free_chunk_index_p = (mem_pool_chunk_index_t*) chunk_p;

    met_free_chunks_number++;

    VALGRIND_DEFINED_SPACE (next_free_chunk_index_p, MEM_POOL_CHUNK_SIZE);

    chunk_index = *next_free_chunk_index_p;

    VALGRIND_NOACCESS_SPACE (next_free_chunk_index_p, MEM_POOL_CHUNK_SIZE);
  }

  JERRY_ASSERT(met_free_chunks_number == pool_p->free_chunks_number);
#endif /* !JERRY_NDEBUG */
} /* mem_check_pool */

/**
 * @}
 * @}
 */
