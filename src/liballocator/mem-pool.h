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

/**
 * State of a memory pool
 *
 * TODO:
 *      Compact the struct
 */
typedef struct mem_pool_state_t {
    uint8_t *pPoolStart; /**< first address of pool space */
    size_t PoolSize; /**< pool space size */

    size_t ChunkSize; /**< size of one chunk */

    mword_t *pBitmap; /**< bitmap - pool chunks' state */
    uint8_t *pChunks; /**< chunks with data */

    size_t ChunksNumber; /**< number of chunks */
    size_t FreeChunksNumber; /**< number of free chunks */

    struct mem_pool_state_t *pNextPool; /**< pointer to the next pool with same chunk size */
} mem_pool_state_t;

extern void mem_pool_init(mem_pool_state_t *pPool, size_t chunkSize, uint8_t *poolStart, size_t poolSize);
extern uint8_t* mem_pool_alloc_chunk(mem_pool_state_t *pPool);
extern void mem_pool_free_chunk(mem_pool_state_t *pPool, uint8_t *pChunk);

#endif /* JERRY_MEM_POOL_H */

/**
 * @}
 */
