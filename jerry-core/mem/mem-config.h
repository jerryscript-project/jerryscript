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

#ifndef MEM_CONFIG_H
#define MEM_CONFIG_H

#include "config.h"

/**
 * Log2 of maximum possible offset in the heap
 */
#define MEM_HEAP_OFFSET_LOG (CONFIG_MEM_HEAP_OFFSET_LOG)

/**
 * Size of heap
 */
#define MEM_HEAP_SIZE ((size_t) (CONFIG_MEM_HEAP_AREA_SIZE))

/**
 * Size of heap chunk
 */
#define MEM_HEAP_CHUNK_SIZE ((size_t) (CONFIG_MEM_HEAP_CHUNK_SIZE))

/**
 * Size of pool chunk
 */
#define MEM_POOL_CHUNK_SIZE ((size_t) (CONFIG_MEM_POOL_CHUNK_SIZE))

/**
 * Logarithm of required alignment for allocated units/blocks
 */
#define MEM_ALIGNMENT_LOG   3

#endif /* MEM_CONFIG_H */
