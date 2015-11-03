/* Copyright 2015 Samsung Electronics Co., Ltd.
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

#include "jrt.h"
#include "jsp-mm.h"
#include "mem-allocator.h"
#include "mem-heap.h"

/** \addtogroup jsparser ECMAScript parser
 * @{
 *
 * \addtogroup managedmem Managed memory allocation
 * @{
 */

/**
 * Header of a managed block, allocated by parser
 */
typedef struct
{
  mem_cpointer_t prev_block_cp; /**< previous managed block */
  mem_cpointer_t next_block_cp; /**< next managed block */

  uint32_t padding; /**< padding for alignment */
} jsp_mm_header_t;

/**
 * Check that alignment of jsp_mm_header_t is MEM_ALIGNMENT
 */
JERRY_STATIC_ASSERT ((sizeof (jsp_mm_header_t) % MEM_ALIGNMENT) == 0);

/**
 * List used for tracking memory blocks
 */
jsp_mm_header_t *jsp_mm_blocks_p = NULL;

/**
 * Initialize managed memory allocator
 */
void
jsp_mm_init (void)
{
  JERRY_ASSERT (jsp_mm_blocks_p == NULL);
} /* jsp_mm_init */

/**
 * Finalize managed memory allocator
 */
void
jsp_mm_finalize (void)
{
  JERRY_ASSERT (jsp_mm_blocks_p == NULL);
} /* jsp_mm_finalize */

/**
 * Recommend allocation size
 *
 * Note:
 *      The interface is used by collection allocators
 *      for storage of data that takes the specified
 *      amount of bytes upon allocation, but probably
 *      would require more.
 *
 *      To reduce probability of reallocation in future,
 *      the allocators can request more space in first
 *      allocation request.
 *
 *      The interface helps to choose appropriate space
 *      to allocate, considering amount of heap space,
 *      that would be waste if allocation size
 *      would not be increased.
 *
 * @return recommended allocation size
 */
size_t
jsp_mm_recommend_size (size_t minimum_size) /**< minimum required size */
{
  size_t block_and_header_size = mem_heap_recommend_allocation_size (minimum_size + sizeof (jsp_mm_header_t));
  return block_and_header_size - sizeof (jsp_mm_header_t);
} /* jsp_mm_recommend_size */

/**
 * Allocate a managed memory block of specified size
 *
 * @return pointer to data space of allocated block
 */
void*
jsp_mm_alloc (size_t size) /**< size of block to allocate */
{
  void *ptr_p = mem_heap_alloc_block (size + sizeof (jsp_mm_header_t), MEM_HEAP_ALLOC_SHORT_TERM);

  jsp_mm_header_t *tmem_header_p = (jsp_mm_header_t*) ptr_p;

  tmem_header_p->prev_block_cp = MEM_CP_NULL;
  MEM_CP_SET_POINTER (tmem_header_p->next_block_cp, jsp_mm_blocks_p);

  if (jsp_mm_blocks_p != NULL)
  {
    MEM_CP_SET_POINTER (jsp_mm_blocks_p->prev_block_cp, tmem_header_p);
  }

  jsp_mm_blocks_p = tmem_header_p;

  return (void *) (tmem_header_p + 1);
} /* jsp_mm_alloc */

/**
 * Free a managed memory block
 */
void
jsp_mm_free (void *ptr) /**< pointer to data space of allocated block */
{
  jsp_mm_header_t *tmem_header_p = ((jsp_mm_header_t *) ptr) - 1;

  jsp_mm_header_t *prev_block_p = MEM_CP_GET_POINTER (jsp_mm_header_t,
                                                      tmem_header_p->prev_block_cp);
  jsp_mm_header_t *next_block_p = MEM_CP_GET_POINTER (jsp_mm_header_t,
                                                      tmem_header_p->next_block_cp);

  if (prev_block_p != NULL)
  {
    prev_block_p->next_block_cp = tmem_header_p->next_block_cp;
  }
  else
  {
    JERRY_ASSERT (jsp_mm_blocks_p == tmem_header_p);
    jsp_mm_blocks_p = next_block_p;
  }

  if (next_block_p != NULL)
  {
    next_block_p->prev_block_cp = tmem_header_p->prev_block_cp;
  }

  mem_heap_free_block (tmem_header_p);
} /* jsp_mm_free */

/**
 * Free all currently allocated managed memory blocks
 */
void
jsp_mm_free_all (void)
{
  while (jsp_mm_blocks_p != NULL)
  {
    jsp_mm_header_t *next_block_p = MEM_CP_GET_POINTER (jsp_mm_header_t,
                                                        jsp_mm_blocks_p->next_block_cp);

    mem_heap_free_block (jsp_mm_blocks_p);

    jsp_mm_blocks_p = next_block_p;
  }
} /* jsp_mm_free_all */

/**
 * @}
 * @}
 */
