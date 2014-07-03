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
 * \addtogroup heap Heap
 * @{
 */

/**
 * Heap allocator interface
 */
#ifndef JERRY_MEM_HEAP_H
#define JERRY_MEM_HEAP_H

#include "globals.h"

/**
 * Type of allocation (argument of mem_Alloc)
 *
 * @see mem_HeapAllocBlock
 */
typedef enum {
    MEM_HEAP_ALLOC_SHORT_TERM, /**< allocated region will be freed soon */
    MEM_HEAP_ALLOC_LONG_TERM /**< allocated region most likely will not be freed soon */
} mem_HeapAllocTerm_t;

extern void mem_HeapInit(uint8_t *heapStart, size_t heapSize);
extern uint8_t* mem_HeapAllocBlock(size_t sizeInBytes, mem_HeapAllocTerm_t allocTerm);
extern void mem_HeapFreeBlock(uint8_t *ptr);
extern size_t mem_HeapRecommendAllocationSize(size_t minimumAllocationSize);
extern void mem_HeapPrint(bool dumpBlockData);

/**
 * @}
 * @}
 */

#endif /* !JERRY_MEM_HEAP_H */
