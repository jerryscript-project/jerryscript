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

#ifndef MEM_HEAP_INTERNAL_H
#define MEM_HEAP_INTERNAL_H

#ifndef MEM_HEAP_INTERNAL
# error "The header is for internal routines of memory heap component. Please, don't use the routines directly."
#endif /* !MEM_HEAP_INTERNAL */

#include "mem-allocator.h"
#include "mem-config.h"

/** \addtogroup mem Memory allocation
 * @{
 */

/* Calculate heap area size, leaving space for a pointer to the free list */
#define MEM_HEAP_AREA_SIZE (MEM_HEAP_SIZE - MEM_ALIGNMENT)
#define MEM_HEAP_END_OF_LIST ((mem_heap_free_t *const) ~((uint32_t) 0x0))

/**
 *  Free region node
 */
typedef struct
{
  uint32_t next_offset; /* Offset of next region in list */
  uint32_t size; /* Size of region */
} mem_heap_free_t;

#ifdef MEM_HEAP_PTR_64
#define MEM_HEAP_GET_OFFSET_FROM_ADDR(h, p) ((uint32_t) ((uint8_t *) (p) - (uint8_t *) (h)->area))
#define MEM_HEAP_GET_ADDR_FROM_OFFSET(h, u) ((mem_heap_free_t *) &(h)->area[u])
#else
/* In this case we simply store the pointer, since it fits anyway. */
#define MEM_HEAP_GET_OFFSET_FROM_ADDR(h, p) ((uint32_t) (p))
#define MEM_HEAP_GET_ADDR_FROM_OFFSET(h, u) ((mem_heap_free_t *)(u))
#endif

/**
 * Heap structure
 */
struct mem_heap
{
  mem_heap_free_t first;
  /* This is used to speed up deallocation. */
  mem_heap_free_t *list_skip_p;
  /**
   * Size of allocated regions
   */
  size_t allocated_size;

  /**
   * Current limit of heap usage, that is upon being reached, causes call of "try give memory back" callbacks
   */
  size_t limit;

  /**
   * Heap area
   */
  uint8_t area[MEM_HEAP_AREA_SIZE] __attribute__ ((aligned (MEM_ALIGNMENT)));
};


/**
 * @}
 */

#endif /* MEM_HEAP_INTERNAL_H */
