/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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
 * Heap implementation
 */

#include "jrt.h"
#include "jrt-bit-fields.h"
#include "jrt-libc-includes.h"
#include "mem-allocator.h"
#include "mem-config.h"
#include "mem-heap.h"

#define MEM_ALLOCATOR_INTERNAL

#include "mem-allocator-internal.h"

/*
 * Valgrind-related options and headers
 */
#ifdef JERRY_VALGRIND
# include "memcheck.h"

# define VALGRIND_NOACCESS_SPACE(p, s)   VALGRIND_MAKE_MEM_NOACCESS((p), (s))
# define VALGRIND_UNDEFINED_SPACE(p, s)  VALGRIND_MAKE_MEM_UNDEFINED((p), (s))
# define VALGRIND_DEFINED_SPACE(p, s)    VALGRIND_MAKE_MEM_DEFINED((p), (s))

#else /* JERRY_VALGRIND */
# define VALGRIND_NOACCESS_SPACE(p, s)
# define VALGRIND_UNDEFINED_SPACE(p, s)
# define VALGRIND_DEFINED_SPACE(p, s)
#endif /* JERRY_VALGRIND */

#ifdef JERRY_VALGRIND_FREYA
# include "memcheck.h"

/**
 * Tells whether a pool manager allocator request is in progress.
 */
static bool valgrind_freya_mempool_request = false;

/**
 * Called by pool manager before a heap allocation or free.
 */
void mem_heap_valgrind_freya_mempool_request (void)
{
  valgrind_freya_mempool_request = true;
} /* mem_heap_valgrind_freya_mempool_request */

# define VALGRIND_FREYA_CHECK_MEMPOOL_REQUEST \
  bool mempool_request = valgrind_freya_mempool_request; \
  valgrind_freya_mempool_request = false

# define VALGRIND_FREYA_MALLOCLIKE_SPACE(p, s) \
  if (!mempool_request) \
  { \
    VALGRIND_MALLOCLIKE_BLOCK((p), (s), 0, 0); \
  }

# define VALGRIND_FREYA_FREELIKE_SPACE(p) \
  if (!mempool_request) \
  { \
    VALGRIND_FREELIKE_BLOCK((p), 0); \
  }

#else /* JERRY_VALGRIND_FREYA */
# define VALGRIND_FREYA_CHECK_MEMPOOL_REQUEST
# define VALGRIND_FREYA_MALLOCLIKE_SPACE(p, s)
# define VALGRIND_FREYA_FREELIKE_SPACE(p)
#endif /* JERRY_VALGRIND_FREYA */

/**
 * Chunk size should satisfy the required alignment value
 */
JERRY_STATIC_ASSERT (MEM_HEAP_CHUNK_SIZE % MEM_ALIGNMENT == 0,
                     MEM_HEAP_CHUNK_SIZE_must_be_multiple_of_MEM_ALIGNMENT);

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
#define MEM_HEAP_GET_OFFSET_FROM_ADDR(p) ((uint32_t) ((uint8_t *) (p) - (uint8_t *) mem_heap.area))
#define MEM_HEAP_GET_ADDR_FROM_OFFSET(u) ((mem_heap_free_t *) &mem_heap.area[u])
#else
/* In this case we simply store the pointer, since it fits anyway. */
#define MEM_HEAP_GET_OFFSET_FROM_ADDR(p) ((uint32_t) (p))
#define MEM_HEAP_GET_ADDR_FROM_OFFSET(u) ((mem_heap_free_t *)(u))
#endif

/**
 * Get end of region
 */
static mem_heap_free_t *  __attr_always_inline___ __attr_pure___
mem_heap_get_region_end (mem_heap_free_t *curr_p) /**< current region */
{
  return (mem_heap_free_t *)((uint8_t *) curr_p + curr_p->size);
} /* mem_heap_get_region_end */

/**
 * Heap structure
 */
typedef struct
{
  /** First node in free region list */
  mem_heap_free_t first;

  /**
   * Heap area
   */
  uint8_t area[MEM_HEAP_AREA_SIZE] __attribute__ ((aligned (MEM_ALIGNMENT)));
} mem_heap_t;

/**
 * Heap
 */
#ifndef JERRY_HEAP_SECTION_ATTR
mem_heap_t mem_heap;
#else
mem_heap_t mem_heap __attribute__ ((section (JERRY_HEAP_SECTION_ATTR)));
#endif

/**
 * Check size of heap is corresponding to configuration
 */
JERRY_STATIC_ASSERT (sizeof (mem_heap) <= MEM_HEAP_SIZE,
                     size_of_mem_heap_must_be_less_than_or_equal_to_MEM_HEAP_SIZE);

/**
 * Size of allocated regions
 */
size_t mem_heap_allocated_size;

/**
 * Current limit of heap usage, that is upon being reached, causes call of "try give memory back" callbacks
 */
size_t mem_heap_limit;

/* This is used to speed up deallocation. */
mem_heap_free_t *mem_heap_list_skip_p;

#ifdef MEM_STATS
/**
 * Heap's memory usage statistics
 */
static mem_heap_stats_t mem_heap_stats;

static void mem_heap_stat_init (void);
static void mem_heap_stat_alloc (size_t num);
static void mem_heap_stat_free (size_t num);
static void mem_heap_stat_skip ();
static void mem_heap_stat_nonskip ();
static void mem_heap_stat_alloc_iter ();
static void mem_heap_stat_free_iter ();

#  define MEM_HEAP_STAT_INIT() mem_heap_stat_init ()
#  define MEM_HEAP_STAT_ALLOC(v1) mem_heap_stat_alloc (v1)
#  define MEM_HEAP_STAT_FREE(v1) mem_heap_stat_free (v1)
#  define MEM_HEAP_STAT_SKIP() mem_heap_stat_skip ()
#  define MEM_HEAP_STAT_NONSKIP() mem_heap_stat_nonskip ()
#  define MEM_HEAP_STAT_ALLOC_ITER() mem_heap_stat_alloc_iter ()
#  define MEM_HEAP_STAT_FREE_ITER() mem_heap_stat_free_iter ()
#else /* !MEM_STATS */
#  define MEM_HEAP_STAT_INIT()
#  define MEM_HEAP_STAT_ALLOC(v1)
#  define MEM_HEAP_STAT_FREE(v1)
#  define MEM_HEAP_STAT_SKIP()
#  define MEM_HEAP_STAT_NONSKIP()
#  define MEM_HEAP_STAT_ALLOC_ITER()
#  define MEM_HEAP_STAT_FREE_ITER()
#endif /* !MEM_STATS */

/**
 * Startup initialization of heap
 */
void
mem_heap_init (void)
{
  JERRY_STATIC_ASSERT ((uintptr_t) mem_heap.area % MEM_ALIGNMENT == 0,
                       mem_heap_area_must_be_multiple_of_MEM_ALIGNMENT);

  mem_heap_allocated_size = 0;
  mem_heap_limit = CONFIG_MEM_HEAP_DESIRED_LIMIT;
  mem_heap.first.size = 0;
  mem_heap_free_t *const region_p = (mem_heap_free_t *) mem_heap.area;
  mem_heap.first.next_offset = MEM_HEAP_GET_OFFSET_FROM_ADDR (region_p);
  region_p->size = sizeof (mem_heap.area);
  region_p->next_offset = MEM_HEAP_GET_OFFSET_FROM_ADDR (MEM_HEAP_END_OF_LIST);

  mem_heap_list_skip_p = &mem_heap.first;

  VALGRIND_NOACCESS_SPACE (mem_heap.area, MEM_HEAP_AREA_SIZE);

  MEM_HEAP_STAT_INIT ();
} /* mem_heap_init */

/**
 * Finalize heap
 */
void mem_heap_finalize (void)
{
  JERRY_ASSERT (mem_heap_allocated_size == 0);
  VALGRIND_NOACCESS_SPACE (&mem_heap, sizeof (mem_heap));
} /* mem_heap_finalize */

/**
 * Allocation of memory region.
 *
 * See also:
 *          mem_heap_alloc_block
 *
 * @return pointer to allocated memory block - if allocation is successful,
 *         NULL - if there is not enough memory.
 */
static __attribute__((hot))
void *mem_heap_alloc_block_internal (const size_t size)
{
  // Align size
  const size_t required_size = ((size + MEM_ALIGNMENT - 1) / MEM_ALIGNMENT) * MEM_ALIGNMENT;
  mem_heap_free_t *data_space_p = NULL;

  VALGRIND_DEFINED_SPACE (&mem_heap.first, sizeof (mem_heap_free_t));

  // Fast path for 8 byte chunks, first region is guaranteed to be sufficient
  if (required_size == MEM_ALIGNMENT
      && likely (mem_heap.first.next_offset != MEM_HEAP_GET_OFFSET_FROM_ADDR (MEM_HEAP_END_OF_LIST)))
  {
    data_space_p = MEM_HEAP_GET_ADDR_FROM_OFFSET (mem_heap.first.next_offset);
    VALGRIND_DEFINED_SPACE (data_space_p, sizeof (mem_heap_free_t));
    mem_heap_allocated_size += MEM_ALIGNMENT;
    MEM_HEAP_STAT_ALLOC_ITER ();

    if (data_space_p->size == MEM_ALIGNMENT)
    {
      mem_heap.first.next_offset = data_space_p->next_offset;
    }
    else
    {
      JERRY_ASSERT (MEM_HEAP_GET_ADDR_FROM_OFFSET (mem_heap.first.next_offset)->size > MEM_ALIGNMENT);
      mem_heap_free_t *const remaining_p = MEM_HEAP_GET_ADDR_FROM_OFFSET (mem_heap.first.next_offset) + 1;

      VALGRIND_DEFINED_SPACE (remaining_p, sizeof (mem_heap_free_t));
      remaining_p->size = data_space_p->size - MEM_ALIGNMENT;
      remaining_p->next_offset = data_space_p->next_offset;
      VALGRIND_NOACCESS_SPACE (remaining_p, sizeof (mem_heap_free_t));

      mem_heap.first.next_offset = MEM_HEAP_GET_OFFSET_FROM_ADDR (remaining_p);
    }

    VALGRIND_UNDEFINED_SPACE (data_space_p, sizeof (mem_heap_free_t));

    if (unlikely (data_space_p == mem_heap_list_skip_p))
    {
      mem_heap_list_skip_p = MEM_HEAP_GET_ADDR_FROM_OFFSET (mem_heap.first.next_offset);
    }
  }
  // Slow path for larger regions
  else
  {
    mem_heap_free_t *current_p = MEM_HEAP_GET_ADDR_FROM_OFFSET (mem_heap.first.next_offset);
    mem_heap_free_t *prev_p = &mem_heap.first;
    while (current_p != MEM_HEAP_END_OF_LIST)
    {
      VALGRIND_DEFINED_SPACE (current_p, sizeof (mem_heap_free_t));
      MEM_HEAP_STAT_ALLOC_ITER ();
      const uint32_t next_offset = current_p->next_offset;

      if (current_p->size >= required_size)
      {
        // Region is sufficiently big, store address
        data_space_p = current_p;
        mem_heap_allocated_size += required_size;

        // Region was larger than necessary
        if (current_p->size > required_size)
        {
          // Get address of remaining space
          mem_heap_free_t *const remaining_p = (mem_heap_free_t *) ((uint8_t *) current_p + required_size);

          // Update metadata
          VALGRIND_DEFINED_SPACE (remaining_p, sizeof (mem_heap_free_t));
          remaining_p->size = current_p->size - (uint32_t) required_size;
          remaining_p->next_offset = next_offset;
          VALGRIND_NOACCESS_SPACE (remaining_p, sizeof (mem_heap_free_t));

          // Update list
          VALGRIND_DEFINED_SPACE (prev_p, sizeof (mem_heap_free_t));
          prev_p->next_offset = MEM_HEAP_GET_OFFSET_FROM_ADDR (remaining_p);
          VALGRIND_NOACCESS_SPACE (prev_p, sizeof (mem_heap_free_t));
        }
        // Block is an exact fit
        else
        {
          // Remove the region from the list
          VALGRIND_DEFINED_SPACE (prev_p, sizeof (mem_heap_free_t));
          prev_p->next_offset = next_offset;
          VALGRIND_NOACCESS_SPACE (prev_p, sizeof (mem_heap_free_t));
        }

        mem_heap_list_skip_p = prev_p;

        // Found enough space
        break;
      }

      VALGRIND_NOACCESS_SPACE (current_p, sizeof (mem_heap_free_t));
      // Next in list
      prev_p = current_p;
      current_p = MEM_HEAP_GET_ADDR_FROM_OFFSET (next_offset);
    }
  }

  while (mem_heap_allocated_size >= mem_heap_limit)
  {
    mem_heap_limit += CONFIG_MEM_HEAP_DESIRED_LIMIT;
  }

  VALGRIND_NOACCESS_SPACE (&mem_heap.first, sizeof (mem_heap_free_t));

  if (unlikely (!data_space_p))
  {
    return NULL;
  }

  JERRY_ASSERT ((uintptr_t) data_space_p % MEM_ALIGNMENT == 0);
  VALGRIND_UNDEFINED_SPACE (data_space_p, size);
  MEM_HEAP_STAT_ALLOC (size);

  return (void *) data_space_p;
} /* mem_heap_finalize */

/**
 * Allocation of memory block, running 'try to give memory back' callbacks, if there is not enough memory.
 *
 * Note:
 *      if after running the callbacks, there is still not enough memory, engine is terminated with ERR_OUT_OF_MEMORY.
 *
 * @return pointer to allocated memory block
 */
void * __attribute__((hot))
mem_heap_alloc_block (const size_t size)
{
  if (unlikely (size == 0))
  {
    return NULL;
  }

  VALGRIND_FREYA_CHECK_MEMPOOL_REQUEST;

#ifdef MEM_GC_BEFORE_EACH_ALLOC
  mem_run_try_to_give_memory_back_callbacks (MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_HIGH);
#endif /* MEM_GC_BEFORE_EACH_ALLOC */

  if (mem_heap_allocated_size + size >= mem_heap_limit)
  {
    mem_run_try_to_give_memory_back_callbacks (MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_LOW);
  }

  void *data_space_p = mem_heap_alloc_block_internal (size);

  if (likely (data_space_p != NULL))
  {
    VALGRIND_FREYA_MALLOCLIKE_SPACE (data_space_p, size);
    return data_space_p;
  }

  for (mem_try_give_memory_back_severity_t severity = MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_LOW;
       severity <= MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_HIGH;
       severity = (mem_try_give_memory_back_severity_t) (severity + 1))
  {
    mem_run_try_to_give_memory_back_callbacks (severity);

    data_space_p = mem_heap_alloc_block_internal (size);

    if (likely (data_space_p != NULL))
    {
      VALGRIND_FREYA_MALLOCLIKE_SPACE (data_space_p, size);
      return data_space_p;
    }
  }

  JERRY_ASSERT (data_space_p == NULL);

  jerry_fatal (ERR_OUT_OF_MEMORY);
} /* mem_heap_alloc_block */

/**
 *  Allocate block and store block size.
 *
 * Note: block will only be aligned to 4 bytes.
 */
void * __attr_always_inline___
mem_heap_alloc_block_store_size (size_t size) /**< required size */
{
  if (unlikely (size == 0))
  {
    return NULL;
  }

  size += sizeof (mem_heap_free_t);

  mem_heap_free_t *const data_space_p = (mem_heap_free_t *) mem_heap_alloc_block (size);
  data_space_p->size = (uint32_t) size;
  return (void *) (data_space_p + 1);
} /* mem_heap_alloc_block_store_size */

/**
 * Free the memory block.
 */
void __attribute__((hot))
mem_heap_free_block (void *ptr, /**< pointer to beginning of data space of the block */
                     const size_t size) /**< size of allocated region */
{
  VALGRIND_FREYA_CHECK_MEMPOOL_REQUEST;

  /* checking that ptr points to the heap */
  JERRY_ASSERT ((uint8_t *) ptr >= mem_heap.area && (uint8_t *) ptr <= mem_heap.area + MEM_HEAP_AREA_SIZE);
  JERRY_ASSERT (size > 0);
  JERRY_ASSERT (mem_heap_limit >= mem_heap_allocated_size);

  VALGRIND_FREYA_FREELIKE_SPACE (ptr);
  VALGRIND_NOACCESS_SPACE (ptr, size);
  MEM_HEAP_STAT_FREE_ITER ();

  mem_heap_free_t *block_p = (mem_heap_free_t *) ptr;
  mem_heap_free_t *prev_p;
  mem_heap_free_t *next_p;

  VALGRIND_DEFINED_SPACE (&mem_heap.first, sizeof (mem_heap_free_t));

  if (block_p > mem_heap_list_skip_p)
  {
    prev_p = mem_heap_list_skip_p;
    MEM_HEAP_STAT_SKIP ();
  }
  else
  {
    prev_p = &mem_heap.first;
    MEM_HEAP_STAT_NONSKIP ();
  }

  const uint32_t block_offset = MEM_HEAP_GET_OFFSET_FROM_ADDR (block_p);
  VALGRIND_DEFINED_SPACE (prev_p, sizeof (mem_heap_free_t));
  // Find position of region in the list
  while (prev_p->next_offset < block_offset)
  {
    mem_heap_free_t *const next_p = MEM_HEAP_GET_ADDR_FROM_OFFSET (prev_p->next_offset);
    VALGRIND_DEFINED_SPACE (next_p, sizeof (mem_heap_free_t));
    VALGRIND_NOACCESS_SPACE (prev_p, sizeof (mem_heap_free_t));
    prev_p = next_p;
    MEM_HEAP_STAT_FREE_ITER ();
  }
  next_p = MEM_HEAP_GET_ADDR_FROM_OFFSET (prev_p->next_offset);
  VALGRIND_DEFINED_SPACE (next_p, sizeof (mem_heap_free_t));

  /* Realign size */
  const size_t aligned_size = (size + MEM_ALIGNMENT - 1) / MEM_ALIGNMENT * MEM_ALIGNMENT;

  VALGRIND_DEFINED_SPACE (block_p, sizeof (mem_heap_free_t));
  VALGRIND_DEFINED_SPACE (prev_p, sizeof (mem_heap_free_t));
  // Update prev
  if (mem_heap_get_region_end (prev_p) == block_p)
  {
    // Can be merged
    prev_p->size += (uint32_t) aligned_size;
    VALGRIND_NOACCESS_SPACE (block_p, sizeof (mem_heap_free_t));
    block_p = prev_p;
  }
  else
  {
    block_p->size = (uint32_t) aligned_size;
    prev_p->next_offset = block_offset;
  }

  VALGRIND_DEFINED_SPACE (next_p, sizeof (mem_heap_free_t));
  // Update next
  if (mem_heap_get_region_end (block_p) == next_p)
  {
    if (unlikely (next_p == mem_heap_list_skip_p))
    {
      mem_heap_list_skip_p = block_p;
    }

    // Can be merged
    block_p->size += next_p->size;
    block_p->next_offset = next_p->next_offset;

  }
  else
  {
    block_p->next_offset = MEM_HEAP_GET_OFFSET_FROM_ADDR (next_p);
  }

  mem_heap_list_skip_p = prev_p;

  VALGRIND_NOACCESS_SPACE (prev_p, sizeof (mem_heap_free_t));
  VALGRIND_NOACCESS_SPACE (block_p, size);
  VALGRIND_NOACCESS_SPACE (next_p, sizeof (mem_heap_free_t));

  JERRY_ASSERT (mem_heap_allocated_size > 0);
  mem_heap_allocated_size -= aligned_size;

  while (mem_heap_allocated_size + CONFIG_MEM_HEAP_DESIRED_LIMIT <= mem_heap_limit)
  {
    mem_heap_limit -= CONFIG_MEM_HEAP_DESIRED_LIMIT;
  }

  VALGRIND_NOACCESS_SPACE (&mem_heap.first, sizeof (mem_heap_free_t));
  JERRY_ASSERT (mem_heap_limit >= mem_heap_allocated_size);
  MEM_HEAP_STAT_FREE (size);
} /* mem_heap_free_block */

/**
 * Free block with stored size
 */
void __attr_always_inline___
mem_heap_free_block_size_stored (void *ptr) /**< pointer to the memory block */
{
  mem_heap_free_t *const original_p = ((mem_heap_free_t *) ptr) - 1;
  JERRY_ASSERT (original_p + 1 == ptr);
  mem_heap_free_block (original_p, original_p->size);
} /* mem_heap_free_block_size_stored */

/**
 * Recommend allocation size based on chunk size.
 *
 * @return recommended allocation size
 */
size_t __attr_pure___
mem_heap_recommend_allocation_size (size_t minimum_allocation_size) /**< minimum allocation size */
{
  return JERRY_ALIGNUP (minimum_allocation_size, MEM_HEAP_CHUNK_SIZE);
} /* mem_heap_recommend_allocation_size */

/**
 * Compress pointer
 *
 * @return packed heap pointer
 */
uintptr_t __attr_pure___ __attribute__((hot))
mem_heap_compress_pointer (const void *pointer_p) /**< pointer to compress */
{
  JERRY_ASSERT (pointer_p != NULL);

  uintptr_t int_ptr = (uintptr_t) pointer_p;
  const uintptr_t heap_start = (uintptr_t) &mem_heap;

  JERRY_ASSERT (int_ptr % MEM_ALIGNMENT == 0);

  int_ptr -= heap_start;
  int_ptr >>= MEM_ALIGNMENT_LOG;

  JERRY_ASSERT ((int_ptr & ~((1u << MEM_HEAP_OFFSET_LOG) - 1)) == 0);

  JERRY_ASSERT (int_ptr != MEM_CP_NULL);

  return int_ptr;
} /* mem_heap_compress_pointer */

/**
 * Decompress pointer
 *
 * @return unpacked heap pointer
 */
void *  __attr_pure___ __attribute__((hot))
mem_heap_decompress_pointer (uintptr_t compressed_pointer) /**< pointer to decompress */
{
  JERRY_ASSERT (compressed_pointer != MEM_CP_NULL);

  uintptr_t int_ptr = compressed_pointer;
  const uintptr_t heap_start = (uintptr_t) &mem_heap;

  int_ptr <<= MEM_ALIGNMENT_LOG;
  int_ptr += heap_start;

  return (void *) int_ptr;
} /* mem_heap_decompress_pointer */

#ifndef JERRY_NDEBUG
/**
 * Check whether the pointer points to the heap
 *
 * Note:
 *      the routine should be used only for assertion checks
 *
 * @return true - if pointer points to the heap,
 *         false - otherwise
 */
bool
mem_is_heap_pointer (const void *pointer) /**< pointer */
{
  return ((uint8_t *) pointer >= mem_heap.area
          && (uint8_t *) pointer <= ((uint8_t *) mem_heap.area + MEM_HEAP_AREA_SIZE));
} /* mem_is_heap_pointer */
#endif /* !JERRY_NDEBUG */

/**
 * Print heap
 */
void
mem_heap_print ()
{
#ifdef MEM_STATS
  printf ("Heap stats:\n");
  printf ("  Heap size = %zu bytes\n"
          "  Allocated = %zu bytes\n"
          "  Waste = %zu bytes\n"
          "  Peak allocated = %zu bytes\n"
          "  Peak waste = %zu bytes\n"
          "  Skip-ahead ratio = %zu.%04zu\n"
          "  Average alloc iteration = %zu.%04zu\n"
          "  Average free iteration = %zu.%04zu\n",
          mem_heap_stats.size,
          mem_heap_stats.allocated_bytes,
          mem_heap_stats.waste_bytes,
          mem_heap_stats.peak_allocated_bytes,
          mem_heap_stats.peak_waste_bytes,
          mem_heap_stats.skip_count / mem_heap_stats.nonskip_count,
          mem_heap_stats.skip_count % mem_heap_stats.nonskip_count * 10000 / mem_heap_stats.nonskip_count,
          mem_heap_stats.alloc_iter_count / mem_heap_stats.alloc_count,
          mem_heap_stats.alloc_iter_count % mem_heap_stats.alloc_count * 10000 / mem_heap_stats.alloc_count,
          mem_heap_stats.free_iter_count / mem_heap_stats.free_count,
          mem_heap_stats.free_iter_count % mem_heap_stats.free_count * 10000 / mem_heap_stats.free_count);
  printf ("\n");
#endif /* !MEM_STATS */
} /* mem_heap_print */

#ifdef MEM_STATS
/**
 * Get heap memory usage statistics
 */
void
mem_heap_get_stats (mem_heap_stats_t *out_heap_stats_p) /**< out: heap stats */
{
  *out_heap_stats_p = mem_heap_stats;
} /* mem_heap_get_stats */

/**
 * Reset peak values in memory usage statistics
 */
void
mem_heap_stats_reset_peak (void)
{
  mem_heap_stats.peak_allocated_bytes = mem_heap_stats.allocated_bytes;
  mem_heap_stats.peak_waste_bytes = mem_heap_stats.waste_bytes;
} /* mem_heap_stats_reset_peak */

/**
 * Initalize heap memory usage statistics account structure
 */
static void
mem_heap_stat_init ()
{
  memset (&mem_heap_stats, 0, sizeof (mem_heap_stats));

  mem_heap_stats.size = MEM_HEAP_AREA_SIZE;
} /* mem_heap_stat_init */

/**
 * Account allocation
 */
static void
mem_heap_stat_alloc (size_t size) /**< Size of allocated block */
{
  const size_t aligned_size = (size + MEM_ALIGNMENT - 1) / MEM_ALIGNMENT * MEM_ALIGNMENT;
  const size_t waste_bytes = aligned_size - size;

  mem_heap_stats.allocated_bytes += aligned_size;
  mem_heap_stats.waste_bytes += waste_bytes;
  mem_heap_stats.alloc_count++;


  if (mem_heap_stats.allocated_bytes > mem_heap_stats.peak_allocated_bytes)
  {
    mem_heap_stats.peak_allocated_bytes = mem_heap_stats.allocated_bytes;
  }
  if (mem_heap_stats.allocated_bytes > mem_heap_stats.global_peak_allocated_bytes)
  {
    mem_heap_stats.global_peak_allocated_bytes = mem_heap_stats.allocated_bytes;
  }

  if (mem_heap_stats.waste_bytes > mem_heap_stats.peak_waste_bytes)
  {
    mem_heap_stats.peak_waste_bytes = mem_heap_stats.waste_bytes;
  }
  if (mem_heap_stats.waste_bytes > mem_heap_stats.global_peak_waste_bytes)
  {
    mem_heap_stats.global_peak_waste_bytes = mem_heap_stats.waste_bytes;
  }
} /* mem_heap_stat_alloc */

/**
 * Account freeing
 */
static void
mem_heap_stat_free (size_t size) /**< Size of freed block */
{
  const size_t aligned_size = (size + MEM_ALIGNMENT - 1) / MEM_ALIGNMENT * MEM_ALIGNMENT;
  const size_t waste_bytes = aligned_size - size;

  mem_heap_stats.free_count++;
  mem_heap_stats.allocated_bytes -= aligned_size;
  mem_heap_stats.waste_bytes -= waste_bytes;
} /* mem_heap_stat_free */

/**
 * Counts number of skip-aheads during insertion of free block
 */
static void
mem_heap_stat_skip ()
{
  mem_heap_stats.skip_count++;
} /* mem_heap_stat_skip  */

/**
 * Counts number of times we could not skip ahead during free block insertion
 */
static void
mem_heap_stat_nonskip ()
{
  mem_heap_stats.nonskip_count++;
} /* mem_heap_stat_nonskip */

/**
 * Count number of iterations required for allocations
 */
static void
mem_heap_stat_alloc_iter ()
{
  mem_heap_stats.alloc_iter_count++;
} /* mem_heap_stat_alloc_iter */

/**
 * Counts number of iterations required for inserting free blocks
 */
static void
mem_heap_stat_free_iter ()
{
  mem_heap_stats.free_iter_count++;
} /* mem_heap_stat_free_iter */
#endif /* MEM_STATS */

/**
 * @}
 * @}
 */
