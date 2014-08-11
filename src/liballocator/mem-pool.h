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

#ifndef JERRY_MEM_POOL_INTERNAL
#error "Please, use mem_poolman.h instead of mem_pool.h"
#endif

#ifndef JERRY_MEM_POOL_H
#define JERRY_MEM_POOL_H

#include "mem-config.h"

/** \addtogroup pool Memory pool
 * @{
 */

/**
 * Get pool's space size
 */
#define MEM_POOL_SPACE_START(pool_header_p) ((uint8_t*) ((mem_pool_state_t*) pool_header_p + 1))

/**
 * Index of chunk in a pool
 */
typedef uint16_t mem_pool_chunk_index_t;

/**
 * State of a memory pool
 */
typedef struct mem_pool_state_t {
    unsigned int chunks_number : MEM_POOL_MAX_CHUNKS_NUMBER_LOG; /**< number of chunks (mem_pool_chunk_index_t) */
    unsigned int free_chunks_number : MEM_POOL_MAX_CHUNKS_NUMBER_LOG; /**< number of free chunks (mem_pool_chunk_index_t) */

    unsigned int first_free_chunk : MEM_POOL_MAX_CHUNKS_NUMBER_LOG; /**< offset of first free chunk
                                                                         from the beginning of the pool
                                                                         (mem_pool_chunk_index_t) */

    unsigned int next_pool_cp : MEM_HEAP_OFFSET_LOG; /**< pointer to the next pool with same chunk size */
} mem_pool_state_t;

extern void mem_pool_init (mem_pool_state_t *pool_p, size_t pool_size);
extern uint8_t* mem_pool_alloc_chunk (mem_pool_state_t *pool_p);
extern void mem_pool_free_chunk (mem_pool_state_t *pool_p, uint8_t *chunk_p);

/**
 * @}
 */

#endif /* JERRY_MEM_POOL_H */
