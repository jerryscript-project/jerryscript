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

/** \addtogroup pool Memory pool
 * @{
 */

#ifndef JERRY_MEM_POOL_INTERNAL
#error "Please, use mem_poolman.h instead of mem_pool.h"
#endif

#ifndef JERRY_MEM_POOL_H
#define JERRY_MEM_POOL_H

typedef uint32_t mem_pool_chunk_offset_t;

/**
 * State of a memory pool
 *
 * TODO:
 *      Compact the struct
 */
typedef struct mem_pool_state_t {
    uint8_t *pool_start_p; /**< first address of pool space */
    size_t pool_size; /**< pool space size */

    size_t chunk_size_log; /**< log of size of one chunk */

    mem_pool_chunk_offset_t chunks_number; /**< number of chunks */
    mem_pool_chunk_offset_t free_chunks_number; /**< number of free chunks */

    mem_pool_chunk_offset_t first_free_chunk; /**< offset of first free chunk
                                                   from the beginning of the pool */

    struct mem_pool_state_t *next_pool_p; /**< pointer to the next pool with same chunk size */
} __attribute__((aligned(64))) mem_pool_state_t;

extern void mem_pool_init(mem_pool_state_t *pool_p, size_t chunk_size, uint8_t *pool_start, size_t pool_size);
extern uint8_t* mem_pool_alloc_chunk(mem_pool_state_t *pool_p);
extern void mem_pool_free_chunk(mem_pool_state_t *pool_p, uint8_t *chunk_p);

#endif /* JERRY_MEM_POOL_H */

/**
 * @}
 */
