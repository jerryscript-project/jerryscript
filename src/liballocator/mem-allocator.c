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
static uint8_t mem_heap_area[ MEM_HEAP_AREA_SIZE ] __attribute__((aligned(MEM_ALIGNMENT)));

/**
 * Check that heap area is less or equal than 64K.
 */
JERRY_STATIC_ASSERT( MEM_HEAP_AREA_SIZE <= 64 * 1024 );

/**
 * Initialize memory allocators.
 */
void
mem_init( void)
{
  mem_heap_init( mem_heap_area, sizeof (mem_heap_area));
  mem_pools_init();
} /* mem_init */

/**
 * Finalize memory allocators.
 */
void
mem_finalize( bool is_show_mem_stats) /**< show heap memory stats
                                           before finalization? */
{
  mem_pools_finalize();

  if (is_show_mem_stats)
    {
      mem_heap_print( false, false, true);
    }

  mem_heap_finalize();
} /* mem_finalize */

/**
 * Get base pointer for allocation area.
 */
uintptr_t
mem_get_base_pointer( void)
{
  return (uintptr_t) mem_heap_area;
} /* mem_get_base_pointer */
