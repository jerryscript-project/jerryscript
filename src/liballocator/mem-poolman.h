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
 */

/** \addtogroup poolman Memory pool manager
 * @{
 */

/**
 * Pool manager interface
 */
#ifndef JERRY_MEM_POOLMAN_H
#define JERRY_MEM_POOLMAN_H

#include "globals.h"

/**
 * Pool chunks's possible sizes
 */
typedef enum {
    MEM_POOL_CHUNK_TYPE_4, /**< 4-byte chunk */
    MEM_POOL_CHUNK_TYPE_8, /**< 8-byte chunk */
    MEM_POOL_CHUNK_TYPE_16, /**< 16-byte chunk */
    MEM_POOL_CHUNK_TYPE_32, /**< 32-byte chunk */
    MEM_POOL_CHUNK_TYPE_64, /**< 64-byte chunk */
    MEM_POOL_CHUNK_TYPE__COUNT /**< count of possible pool chunks' sizes */
} mem_pool_chunk_type_t;

/**
 * Convert size to pool chunk type.
 */
#define mem_size_to_pool_chunk_type( size) ((size) == 4 ? MEM_POOL_CHUNK_TYPE_4 : \
                                        ((size) == 8 ? MEM_POOL_CHUNK_TYPE_8 : \
                                        ((size) == 16 ? MEM_POOL_CHUNK_TYPE_16 : \
                                        ((size) == 32 ? MEM_POOL_CHUNK_TYPE_32 : \
                                        ((size) == 64 ? MEM_POOL_CHUNK_TYPE_64 : \
                                        jerry_UnreferencedExpression)))))

extern size_t mem_get_chunk_size( mem_pool_chunk_type_t chunkType);

extern void mem_pools_init(void);
extern uint8_t* mem_pools_alloc(mem_pool_chunk_type_t chunkType);
extern void mem_pools_free(mem_pool_chunk_type_t chunkType, uint8_t *pChunk);

#ifdef MEM_STATS
/**
 * Pools' memory usage statistics
 */
typedef struct
{
    /** pools' count, per type */
    size_t pools_count[ MEM_POOL_CHUNK_TYPE__COUNT ];
    
    /** peak pools' count, per type */
    size_t peak_pools_count[ MEM_POOL_CHUNK_TYPE__COUNT ];

    /** allocated chunks count, per type */
    size_t allocated_chunks[ MEM_POOL_CHUNK_TYPE__COUNT ];
    
    /** peak allocated chunks count, per type */
    size_t peak_allocated_chunks[ MEM_POOL_CHUNK_TYPE__COUNT ];
    
    /** free chunks count, per type */
    size_t free_chunks[ MEM_POOL_CHUNK_TYPE__COUNT ];
} mem_pools_stats_t;

extern void mem_pools_get_stats( mem_pools_stats_t *out_pools_stats_p);
#endif /* MEM_STATS */

#endif /* JERRY_MEM_POOLMAN_H */

/**
 * @}
 */

/**
 * @}
 */
