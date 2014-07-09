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

/**
 * Allocator implementation
 */

#include "globals.h"
#include "mem-allocator.h"
#include "mem-heap.h"
#include "mem-poolman.h"

/**
 * Area for heap
 */
static uint8_t mem_HeapArea[ MEM_HEAP_AREA_SIZE ] __attribute__((aligned(MEM_ALIGNMENT)));

/**
 * Check that heap area is less or equal than 64K.
 */
JERRY_STATIC_ASSERT( MEM_HEAP_AREA_SIZE <= 64 * 1024 );

/**
 * Initialize memory allocators.
 */
void
mem_Init( void)
{
    mem_HeapInit( mem_HeapArea, sizeof (mem_HeapArea));
    mem_PoolsInit();
} /* mem_Init */

/**
 * Get base pointer for allocation area.
 */
uintptr_t
mem_GetBasePointer( void)
{
    return (uintptr_t) mem_HeapArea;
} /* mem_GetBasePointer */
