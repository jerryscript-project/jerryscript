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

#ifndef CONFIG_H
#define CONFIG_H

/**
 * Limit of data (system heap, engine's data except engine's own heap)
 */
#define CONFIG_MEM_DATA_LIMIT_MINUS_HEAP_SIZE 1024

/**
 * Limit of stack size
 */
#define CONFIG_MEM_STACK_LIMIT 4096

/**
 * Log2 of maximum number of chunks in a pool
 */
#define CONFIG_MEM_POOL_MAX_CHUNKS_NUMBER_LOG 16

/**
 * Size of pool chunk
 *
 * Should not be less than size of any of ECMA Object Model's data types.
 */
#define CONFIG_MEM_POOL_CHUNK_SIZE 16

/**
 * Size of heap chunk
 */
#define CONFIG_MEM_HEAP_CHUNK_SIZE 64

/**
 * Size of heap
 */
#define CONFIG_MEM_HEAP_AREA_SIZE (2 * 1024 + 512)

/**
 * Log2 of maximum possible offset in the heap
 *
 * The option affects size of compressed pointer that in turn
 * affects size of ECMA Object Model's data types.
 *
 * In any case size of any of the types should not exceed CONFIG_MEM_POOL_CHUNK_SIZE.
 *
 * On the other hand, value 2 ^ CONFIG_MEM_HEAP_OFFSET_LOG should not be less than CONFIG_MEM_HEAP_AREA_SIZE.
 */
#define CONFIG_MEM_HEAP_OFFSET_LOG 16

/**
 * Width of fields used for holding counter of references to ecma-strings and ecma-objects
 *
 * The option affects maximum number of simultaneously existing:
 *  - references to one string;
 *  - stack references to one object
 * The number is ((2 ^ CONFIG_ECMA_REFERENCE_COUNTER_WIDTH) - 1).
 *
 * Also the option affects size of ECMA Object Model's data types.
 * In any case size of any of the types should not exceed CONFIG_MEM_POOL_CHUNK_SIZE.
 */
#define CONFIG_ECMA_REFERENCE_COUNTER_WIDTH 10

/**
 * Use 32-bit/64-bit float for ecma-numbers
 */
#define CONFIG_ECMA_NUMBER_FLOAT32
// #define CONFIG_ECMA_NUMBER_FLOAT64

/**
 * Use ASCII/UTF-16 representation for ecma-characters
 */
#define CONFIG_ECMA_CHAR_ASCII
// #define CONFIG_ECMA_CHAR_UTF16

#endif /* !CONFIG_H */
