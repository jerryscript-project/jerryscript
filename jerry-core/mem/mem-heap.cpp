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
 * Length type of the block
 */
typedef enum : uint8_t
{
  GENERAL     = 0, /**< general (may be multi-chunk) block
                    *
                    *   Note:
                    *         As zero is used for initialization in mem_heap_init,
                    *         0 value for the GENERAL is necessary
                    */
  ONE_CHUNKED = 1  /**< one-chunked block (See also: mem_heap_alloc_chunked_block) */
} mem_block_length_type_t;

/**
 * Chunk size should satisfy the required alignment value
 */
JERRY_STATIC_ASSERT (MEM_HEAP_CHUNK_SIZE % MEM_ALIGNMENT == 0);

typedef enum
{
  MEM_HEAP_BITMAP_IS_ALLOCATED, /**< bitmap of 'chunk allocated' flags */
  MEM_HEAP_BITMAP_IS_FIRST_IN_BLOCK, /**< bitmap of 'chunk is first in allocated block' flags */

  MEM_HEAP_BITMAP__COUNT /**< number of bitmaps */
} mem_heap_bitmap_t;

/**
 * Type of bitmap storage item, used to store one or several bitmap blocks
 */
typedef size_t mem_heap_bitmap_storage_item_t;

/**
 * Mask of a single bit at specified offset in a bitmap storage item
 */
#define MEM_HEAP_BITMAP_ITEM_BIT(offset) (((mem_heap_bitmap_storage_item_t) 1u) << (offset))

/**
 * Number of bits in a bitmap storage item
 */
#define MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM (sizeof (mem_heap_bitmap_storage_item_t) * JERRY_BITSINBYTE)

/**
 * Full bit mask for a bitmap storage item
 */
#define MEM_HEAP_BITMAP_STORAGE_ALL_BITS_MASK ((mem_heap_bitmap_storage_item_t) -1)

/**
 * Number of chunks in heap
 *
 *                           bits_in_heap
 * ALIGN_DOWN (-----------------------------------------, bits_in_bitmap_storage_item)
 *               bitmap_bits_per_chunk + bits_in_chunk
 */
#define MEM_HEAP_CHUNKS_NUM JERRY_ALIGNDOWN (JERRY_BITSINBYTE * MEM_HEAP_SIZE / \
                                             (MEM_HEAP_BITMAP__COUNT + JERRY_BITSINBYTE * MEM_HEAP_CHUNK_SIZE), \
                                             MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM)

/**
 * Size of heap data area
 */
#define MEM_HEAP_AREA_SIZE (MEM_HEAP_CHUNKS_NUM * MEM_HEAP_CHUNK_SIZE)

/**
 * Number of bits in heap's bitmap
 */
#define MEM_HEAP_BITMAP_BITS (MEM_HEAP_CHUNKS_NUM * 1u)

/**
 * Overall number of bitmap bits is multiple of number of bits in a bitmap storage item
 */
JERRY_STATIC_ASSERT (MEM_HEAP_BITMAP_BITS % MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM == 0);

/**
 * Number of bitmap storage items
 */
#define MEM_HEAP_BITMAP_STORAGE_ITEMS (MEM_HEAP_BITMAP_BITS / MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM)

/**
 * Heap structure
 */
typedef struct
{
  /**
   * Heap bitmaps
   *
   * The bitmaps consist of chunks with unique correspondence to the heap chunks
   */
  mem_heap_bitmap_storage_item_t bitmaps[MEM_HEAP_BITMAP__COUNT][MEM_HEAP_BITMAP_STORAGE_ITEMS];

  /**
   * Heap area
   */
  uint8_t area[MEM_HEAP_AREA_SIZE] __attribute__ ((aligned (JERRY_MAX (MEM_ALIGNMENT, MEM_HEAP_CHUNK_SIZE))));
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
JERRY_STATIC_ASSERT (sizeof (mem_heap) <= MEM_HEAP_SIZE);

/**
 * Bitmap of 'is allocated' flags
 */
#define MEM_HEAP_IS_ALLOCATED_BITMAP (mem_heap.bitmaps[MEM_HEAP_BITMAP_IS_ALLOCATED])

/**
 * Bitmap of 'is first in block' flags
 */
#define MEM_HEAP_IS_FIRST_IN_BLOCK_BITMAP (mem_heap.bitmaps[MEM_HEAP_BITMAP_IS_FIRST_IN_BLOCK])

/**
 * Total number of allocated heap chunks
 */
size_t mem_heap_allocated_chunks;

/**
 * Current limit of heap usage, that is upon being reached, causes call of "try give memory back" callbacks
 */
size_t mem_heap_limit;

#if defined (JERRY_VALGRIND) || defined (MEM_STATS) || !defined (JERRY_DISABLE_HEAVY_DEBUG)

# define MEM_HEAP_ENABLE_ALLOCATED_BYTES_ARRAY

/**
 * Number of bytes, allocated in heap block
 *
 * The array contains one entry per heap chunk with:
 *  - number of allocated bytes, if the chunk is at start of an allocated block;
 *  - 0, if the chunk is at start of free block;
 *  - -1, if the chunk is not at start of a block.
 */
ssize_t mem_heap_allocated_bytes[MEM_HEAP_CHUNKS_NUM];
#else /* JERRY_VALGRIND || MEM_STATS || !JERRY_DISABLE_HEAVY_DEBUG */
# ifdef MEM_HEAP_ENABLE_ALLOCATED_BYTES_ARRAY
#  error "!"
# endif /* MEM_HEAP_ENABLE_ALLOCATED_BYTES_ARRAY */
#endif /* !JERRY_VALGRIND && !MEM_STATS && JERRY_DISABLE_HEAVY_DEBUG */

#ifndef JERRY_NDEBUG
/**
 * Length types for allocated chunks
 *
 * The array contains one entry per heap chunk with:
 *  - length type of corresponding block, if the chunk is at start of an allocated block;
 *  - GENERAL length type for rest chunks.
 */
mem_block_length_type_t mem_heap_length_types[MEM_HEAP_CHUNKS_NUM];
#endif /* !JERRY_NDEBUG */

static size_t mem_get_block_chunks_count_from_data_size (size_t block_allocated_size);

static void mem_check_heap (void);

#ifdef MEM_STATS
/**
 * Heap's memory usage statistics
 */
static mem_heap_stats_t mem_heap_stats;

static void mem_heap_stat_init (void);
static void mem_heap_stat_alloc (size_t first_chunk_index, size_t chunks_num);
static void mem_heap_stat_free (size_t first_chunk_index, size_t chunks_num);

#  define MEM_HEAP_STAT_INIT() mem_heap_stat_init ()
#  define MEM_HEAP_STAT_ALLOC(v1, v2) mem_heap_stat_alloc (v1, v2)
#  define MEM_HEAP_STAT_FREE(v1, v2) mem_heap_stat_free (v1, v2)
#else /* !MEM_STATS */
#  define MEM_HEAP_STAT_INIT()
#  define MEM_HEAP_STAT_ALLOC(v1, v2)
#  define MEM_HEAP_STAT_FREE(v1, v2)
#endif /* !MEM_STATS */

/**
 * Calculate minimum chunks count needed for block with specified size of allocated data area.
 *
 * @return chunks count
 */
static size_t
mem_get_block_chunks_count_from_data_size (size_t block_allocated_size) /**< size of block's allocated area */
{
  return JERRY_ALIGNUP (block_allocated_size, MEM_HEAP_CHUNK_SIZE) / MEM_HEAP_CHUNK_SIZE;
} /* mem_get_block_chunks_count_from_data_size */

/**
 * Get index of a heap chunk from its starting address
 *
 * @return heap chunk index
 */
static size_t
mem_heap_get_chunk_from_address (const void *chunk_start_p) /**< address of a chunk's beginning */
{
  uintptr_t heap_start_uintptr = (uintptr_t) mem_heap.area;
  uintptr_t chunk_start_uintptr = (uintptr_t) chunk_start_p;

  uintptr_t chunk_offset = chunk_start_uintptr - heap_start_uintptr;

  JERRY_ASSERT (chunk_offset % MEM_HEAP_CHUNK_SIZE == 0);

  return (chunk_offset / MEM_HEAP_CHUNK_SIZE);
} /* mem_heap_get_chunk_from_address */

/**
 * Mark specified chunk allocated
 */
static void
mem_heap_mark_chunk_allocated (size_t chunk_index, /**< index of the heap chunk (bitmap's chunk index is the same) */
                               bool is_first_in_block) /**< is the chunk first in its block */
{
  JERRY_ASSERT (chunk_index < MEM_HEAP_CHUNKS_NUM);

  mem_heap_bitmap_storage_item_t bit = MEM_HEAP_BITMAP_ITEM_BIT (chunk_index % MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM);

  JERRY_ASSERT ((MEM_HEAP_IS_ALLOCATED_BITMAP[chunk_index / MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM] & bit) == 0);
  JERRY_ASSERT ((MEM_HEAP_IS_FIRST_IN_BLOCK_BITMAP[chunk_index / MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM] & bit) == 0);

  MEM_HEAP_IS_ALLOCATED_BITMAP[chunk_index / MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM] |= bit;

  if (is_first_in_block)
  {
    MEM_HEAP_IS_FIRST_IN_BLOCK_BITMAP[chunk_index / MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM] |= bit;
  }
} /* mem_heap_mark_chunk_allocated */

/**
 * Startup initialization of heap
 */
void
mem_heap_init (void)
{
  JERRY_STATIC_ASSERT ((MEM_HEAP_CHUNK_SIZE & (MEM_HEAP_CHUNK_SIZE - 1u)) == 0);
  JERRY_STATIC_ASSERT ((uintptr_t) mem_heap.area % MEM_ALIGNMENT == 0);
  JERRY_STATIC_ASSERT ((uintptr_t) mem_heap.area % MEM_HEAP_CHUNK_SIZE == 0);
  JERRY_STATIC_ASSERT (MEM_HEAP_AREA_SIZE % MEM_HEAP_CHUNK_SIZE == 0);

  JERRY_ASSERT (MEM_HEAP_AREA_SIZE <= (1u << MEM_HEAP_OFFSET_LOG));

  mem_heap_limit = CONFIG_MEM_HEAP_DESIRED_LIMIT;

  VALGRIND_NOACCESS_SPACE (mem_heap.area, MEM_HEAP_AREA_SIZE);

  memset (MEM_HEAP_IS_ALLOCATED_BITMAP, 0, sizeof (MEM_HEAP_IS_ALLOCATED_BITMAP));
  memset (MEM_HEAP_IS_FIRST_IN_BLOCK_BITMAP, 0, sizeof (MEM_HEAP_IS_FIRST_IN_BLOCK_BITMAP));

#ifdef MEM_HEAP_ENABLE_ALLOCATED_BYTES_ARRAY
  memset (mem_heap_allocated_bytes, -1, sizeof (mem_heap_allocated_bytes));

  for (size_t i = 0; i < MEM_HEAP_CHUNKS_NUM; i++)
  {
#ifndef JERRY_NDEBUG
    JERRY_ASSERT (mem_heap_length_types[i] == mem_block_length_type_t::GENERAL);
#endif /* !JERRY_NDEBUG */

    JERRY_ASSERT (mem_heap_allocated_bytes[i] == -1);
  }
#endif /* MEM_HEAP_ENABLE_ALLOCATED_BYTES_ARRAY */

  VALGRIND_NOACCESS_SPACE (&mem_heap, sizeof (mem_heap));

  MEM_HEAP_STAT_INIT ();
} /* mem_heap_init */

/**
 * Finalize heap
 */
void
mem_heap_finalize (void)
{
  JERRY_ASSERT (mem_heap_allocated_chunks == 0);

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
static
void* mem_heap_alloc_block_internal (size_t size_in_bytes, /**< size of region to allocate in bytes */
                                     mem_block_length_type_t length_type, /**< length type of the block
                                                                           *   (one-chunked or general) */
                                     mem_heap_alloc_term_t alloc_term) /**< expected allocation term */
{
  JERRY_ASSERT (size_in_bytes != 0);
  JERRY_ASSERT (length_type != mem_block_length_type_t::ONE_CHUNKED
                || size_in_bytes == mem_heap_get_chunked_block_data_size ());

  mem_check_heap ();

  bool is_direction_forward = (alloc_term == MEM_HEAP_ALLOC_LONG_TERM);

  /* searching for appropriate free area, considering requested direction */

  const size_t req_chunks_num = mem_get_block_chunks_count_from_data_size (size_in_bytes);
  JERRY_ASSERT (req_chunks_num > 0);

  size_t found_chunks_num = 0;
  size_t first_chunk = MEM_HEAP_CHUNKS_NUM;

  VALGRIND_DEFINED_SPACE (mem_heap.bitmaps, sizeof (mem_heap.bitmaps));

  for (size_t i = 0;
       i < MEM_HEAP_BITMAP_STORAGE_ITEMS && found_chunks_num != req_chunks_num;
       i++)
  {
    const size_t bitmap_item_index = (is_direction_forward ? i : MEM_HEAP_BITMAP_STORAGE_ITEMS - i - 1);

    mem_heap_bitmap_storage_item_t item = MEM_HEAP_IS_ALLOCATED_BITMAP[bitmap_item_index];

    if (item != MEM_HEAP_BITMAP_STORAGE_ALL_BITS_MASK)
    {
      for (size_t j = 0; j < MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM; j++)
      {
        const size_t bit_index = (is_direction_forward ? j : MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM - j - 1);
        mem_heap_bitmap_storage_item_t bit = MEM_HEAP_BITMAP_ITEM_BIT (bit_index);

        if ((item & bit) == 0)
        {
          found_chunks_num++;

          if (found_chunks_num == req_chunks_num)
          {
            first_chunk = bitmap_item_index * MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM + bit_index;

            if (is_direction_forward)
            {
              first_chunk -= req_chunks_num - 1u;
            }

            break;
          }
        }
        else
        {
          found_chunks_num = 0;
        }
      }
    }
    else
    {
      found_chunks_num = 0;
    }
  }

  VALGRIND_NOACCESS_SPACE (mem_heap.bitmaps, sizeof (mem_heap.bitmaps));

  if (found_chunks_num != req_chunks_num)
  {
    JERRY_ASSERT (found_chunks_num < req_chunks_num);

    /* not enough free space */
    return NULL;
  }

  JERRY_ASSERT (req_chunks_num <= found_chunks_num);

#ifdef MEM_HEAP_ENABLE_ALLOCATED_BYTES_ARRAY
  mem_heap_allocated_bytes[first_chunk] = (ssize_t) size_in_bytes;
#endif /* MEM_HEAP_ENABLE_ALLOCATED_BYTES_ARRAY */

  mem_heap_allocated_chunks += req_chunks_num;

  JERRY_ASSERT (mem_heap_allocated_chunks * MEM_HEAP_CHUNK_SIZE <= MEM_HEAP_AREA_SIZE);

  if (mem_heap_allocated_chunks * MEM_HEAP_CHUNK_SIZE >= mem_heap_limit)
  {
    mem_heap_limit = JERRY_MIN (MEM_HEAP_AREA_SIZE,
                                JERRY_MAX (mem_heap_limit + CONFIG_MEM_HEAP_DESIRED_LIMIT,
                                           mem_heap_allocated_chunks * MEM_HEAP_CHUNK_SIZE));
    JERRY_ASSERT (mem_heap_limit >= mem_heap_allocated_chunks * MEM_HEAP_CHUNK_SIZE);
  }

  VALGRIND_DEFINED_SPACE (mem_heap.bitmaps, sizeof (mem_heap.bitmaps));

  mem_heap_mark_chunk_allocated (first_chunk, true);
#ifndef JERRY_NDEBUG
  mem_heap_length_types[first_chunk] = length_type;
#endif /* !JERRY_NDEBUG */

  for (size_t chunk_index = first_chunk + 1u;
       chunk_index < first_chunk + req_chunks_num;
       chunk_index++)
  {
    mem_heap_mark_chunk_allocated (chunk_index, false);

#ifndef JERRY_NDEBUG
    JERRY_ASSERT (length_type == mem_block_length_type_t::GENERAL
                  && mem_heap_length_types[chunk_index] == length_type);
#endif /* !JERRY_NDEBUG */
  }

  VALGRIND_NOACCESS_SPACE (mem_heap.bitmaps, sizeof (mem_heap.bitmaps));

  MEM_HEAP_STAT_ALLOC (first_chunk, req_chunks_num);

  /* return data space beginning address */
  uint8_t *data_space_p = (uint8_t *) mem_heap.area + (first_chunk * MEM_HEAP_CHUNK_SIZE);
  JERRY_ASSERT ((uintptr_t) data_space_p % MEM_ALIGNMENT == 0);

  VALGRIND_UNDEFINED_SPACE (data_space_p, size_in_bytes);

  mem_check_heap ();

  return data_space_p;
} /* mem_heap_alloc_block_internal */

/**
 * Allocation of memory region, running 'try to give memory back' callbacks, if there is not enough memory.
 *
 * Note:
 *      if after running the callbacks, there is still not enough memory, engine is terminated with ERR_OUT_OF_MEMORY.
 *
 * Note:
 *      To reduce heap fragmentation there are two allocation modes - short-term and long-term.
 *
 *      If allocation is short-term then the beginning of the heap is preferred, else - the end of the heap.
 *
 *      It is supposed, that all short-term allocation is used during relatively short discrete sessions.
 *      After end of the session all short-term allocated regions are supposed to be freed.
 *
 * @return pointer to allocated memory block
 */
static void*
mem_heap_alloc_block_try_give_memory_back (size_t size_in_bytes, /**< size of region to allocate in bytes */
                                           mem_block_length_type_t length_type, /**< length type of the block
                                                                                 *   (one-chunked or general) */
                                           mem_heap_alloc_term_t alloc_term) /**< expected allocation term */
{
  VALGRIND_FREYA_CHECK_MEMPOOL_REQUEST;

#ifdef MEM_GC_BEFORE_EACH_ALLOC
  mem_run_try_to_give_memory_back_callbacks (MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_HIGH);
#endif /* MEM_GC_BEFORE_EACH_ALLOC */

  size_t chunks = mem_get_block_chunks_count_from_data_size (size_in_bytes);
  if ((mem_heap_allocated_chunks + chunks) * MEM_HEAP_CHUNK_SIZE >= mem_heap_limit)
  {
    mem_run_try_to_give_memory_back_callbacks (MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_LOW);
  }

  void *data_space_p = mem_heap_alloc_block_internal (size_in_bytes, length_type, alloc_term);

  if (likely (data_space_p != NULL))
  {
    VALGRIND_FREYA_MALLOCLIKE_SPACE (data_space_p, size_in_bytes);
    return data_space_p;
  }

  for (mem_try_give_memory_back_severity_t severity = MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_LOW;
       severity <= MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_HIGH;
       severity = (mem_try_give_memory_back_severity_t) (severity + 1))
  {
    mem_run_try_to_give_memory_back_callbacks (severity);

    data_space_p = mem_heap_alloc_block_internal (size_in_bytes, length_type, alloc_term);

    if (likely (data_space_p != NULL))
    {
      VALGRIND_FREYA_MALLOCLIKE_SPACE (data_space_p, size_in_bytes);
      return data_space_p;
    }
  }

  JERRY_ASSERT (data_space_p == NULL);

  jerry_fatal (ERR_OUT_OF_MEMORY);
} /* mem_heap_alloc_block_try_give_memory_back */

/**
 * Allocation of memory region.
 *
 * Note:
 *      Please look at mem_heap_alloc_block_try_give_memory_back
 *      for description of allocation term and out-of-memory handling.
 *
 * @return pointer to allocated memory block - if allocation is successful,
 *         NULL - if requested region size is zero.
 */
void*
mem_heap_alloc_block (size_t size_in_bytes,             /**< size of region to allocate in bytes */
                      mem_heap_alloc_term_t alloc_term) /**< expected allocation term */
{
  if (unlikely (size_in_bytes == 0))
  {
    return NULL;
  }
  else
  {
    return mem_heap_alloc_block_try_give_memory_back (size_in_bytes,
                                                      mem_block_length_type_t::GENERAL,
                                                      alloc_term);
  }
} /* mem_heap_alloc_block */

/**
 * Allocation of one-chunked memory region, i.e. memory block that exactly fits one heap chunk.
 *
 * Note:
 *      If there is any free space in the heap, it anyway can be allocated for one-chunked block.
 *
 *      Contrariwise, there are cases, when block, requiring more than one chunk,
 *      cannot be allocated, because of heap fragmentation.
 *
 * Note:
 *      Please look at mem_heap_alloc_block_try_give_memory_back
 *      for description of allocation term and out-of-memory handling.
 *
 * @return pointer to allocated memory block
 */
void*
mem_heap_alloc_chunked_block (mem_heap_alloc_term_t alloc_term) /**< expected allocation term */
{
  return mem_heap_alloc_block_try_give_memory_back (mem_heap_get_chunked_block_data_size (),
                                                    mem_block_length_type_t::ONE_CHUNKED,
                                                    alloc_term);
} /* mem_heap_alloc_chunked_block */

/**
 * Free the memory block.
 */
void
mem_heap_free_block (void *ptr) /**< pointer to beginning of data space of the block */
{
  VALGRIND_FREYA_CHECK_MEMPOOL_REQUEST;

  uint8_t *uint8_ptr = (uint8_t*) ptr;

  /* checking that uint8_ptr points to the heap */
  JERRY_ASSERT (uint8_ptr >= mem_heap.area && uint8_ptr <= (uint8_t *) mem_heap.area + MEM_HEAP_AREA_SIZE);

  mem_check_heap ();

  JERRY_ASSERT (mem_heap_limit >= mem_heap_allocated_chunks * MEM_HEAP_CHUNK_SIZE);

  size_t chunk_index = mem_heap_get_chunk_from_address (ptr);

  size_t chunks = 0;
  bool is_block_end_reached = false;

  VALGRIND_DEFINED_SPACE (mem_heap.bitmaps, sizeof (mem_heap.bitmaps));

  for (size_t bitmap_item_index = chunk_index / MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM;
       bitmap_item_index < MEM_HEAP_BITMAP_STORAGE_ITEMS && !is_block_end_reached;
       bitmap_item_index++)
  {
    mem_heap_bitmap_storage_item_t item_allocated = MEM_HEAP_IS_ALLOCATED_BITMAP[bitmap_item_index];
    mem_heap_bitmap_storage_item_t item_first_in_block = MEM_HEAP_IS_FIRST_IN_BLOCK_BITMAP[bitmap_item_index];

    if (item_first_in_block == 0
        && item_allocated == MEM_HEAP_BITMAP_STORAGE_ALL_BITS_MASK)
    {
      JERRY_ASSERT (chunks != 0);
      MEM_HEAP_IS_ALLOCATED_BITMAP[bitmap_item_index] = 0;

      chunks += MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM;
    }
    else
    {
      size_t bit_index;

      if (chunks == 0)
      {
        bit_index = chunk_index % MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM;
        mem_heap_bitmap_storage_item_t bit = MEM_HEAP_BITMAP_ITEM_BIT (bit_index);

        JERRY_ASSERT ((item_first_in_block & bit) != 0);
        item_first_in_block &= ~bit;

        MEM_HEAP_IS_FIRST_IN_BLOCK_BITMAP[bitmap_item_index] = item_first_in_block;
      }
      else
      {
        bit_index = 0;
      }

      while (bit_index < MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM)
      {
        mem_heap_bitmap_storage_item_t bit = MEM_HEAP_BITMAP_ITEM_BIT (bit_index);

        if ((item_allocated & bit) == 0
            || (item_first_in_block & bit) != 0)
        {
          is_block_end_reached = true;
          break;
        }
        else
        {
          JERRY_ASSERT ((item_allocated & bit) != 0);

          item_allocated &= ~bit;

          chunks++;
          bit_index++;
        }
      }

      MEM_HEAP_IS_ALLOCATED_BITMAP[bitmap_item_index] = item_allocated;
    }
  }

  VALGRIND_NOACCESS_SPACE (mem_heap.bitmaps, sizeof (mem_heap.bitmaps));

#ifdef JERRY_VALGRIND
  VALGRIND_CHECK_MEM_IS_ADDRESSABLE (ptr, mem_heap_allocated_bytes[chunk_index]);
#endif /* JERRY_VALGRIND */

  VALGRIND_FREYA_FREELIKE_SPACE (ptr);
  VALGRIND_NOACCESS_SPACE (ptr, chunks * MEM_HEAP_CHUNK_SIZE);

  JERRY_ASSERT (mem_heap_allocated_chunks >= chunks);
  mem_heap_allocated_chunks -= chunks;

  if (mem_heap_allocated_chunks * MEM_HEAP_CHUNK_SIZE * 3 <= mem_heap_limit)
  {
    mem_heap_limit /= 2;
  }
  else if (mem_heap_allocated_chunks * MEM_HEAP_CHUNK_SIZE + CONFIG_MEM_HEAP_DESIRED_LIMIT <= mem_heap_limit)
  {
    mem_heap_limit -= CONFIG_MEM_HEAP_DESIRED_LIMIT;
  }

  JERRY_ASSERT (mem_heap_limit >= mem_heap_allocated_chunks * MEM_HEAP_CHUNK_SIZE);

  MEM_HEAP_STAT_FREE (chunk_index, chunks);

#ifdef MEM_HEAP_ENABLE_ALLOCATED_BYTES_ARRAY
  mem_heap_allocated_bytes[chunk_index] = 0;
#endif /* MEM_HEAP_ENABLE_ALLOCATED_BYTES_ARRAY */

#ifndef JERRY_NDEBUG
  mem_heap_length_types[chunk_index] = mem_block_length_type_t::GENERAL;
#endif /* !JERRY_NDEBUG */

  mem_check_heap ();
} /* mem_heap_free_block */

/**
 * Find beginning of user data in a one-chunked block from pointer,
 * pointing into it, i.e. into [block_data_space_start; block_data_space_end) range.
 *
 * Note:
 *      Pointer must point to the one-chunked memory region which was previously allocated
 *      with mem_heap_alloc_chunked_block and is currently valid.
 *
 * Note:
 *      The interface should only be used for determining where the user space of heap-allocated block begins.
 *      Caller should never rely on some specific internals of heap implementation.
 *
 * @return beginning of user data space of block identified by the pointer
 */
void*
mem_heap_get_chunked_block_start (void *ptr) /**< pointer into a block */
{
  JERRY_STATIC_ASSERT ((MEM_HEAP_CHUNK_SIZE & (MEM_HEAP_CHUNK_SIZE - 1u)) == 0);
  JERRY_STATIC_ASSERT (((uintptr_t) mem_heap.area % MEM_HEAP_CHUNK_SIZE) == 0);

  JERRY_ASSERT (mem_heap.area <= ptr && ptr < (uint8_t *) mem_heap.area + MEM_HEAP_AREA_SIZE);

  uintptr_t uintptr = (uintptr_t) ptr;
  uintptr_t uintptr_chunk_aligned = JERRY_ALIGNDOWN (uintptr, MEM_HEAP_CHUNK_SIZE);

  JERRY_ASSERT (uintptr >= uintptr_chunk_aligned);

#ifndef JERRY_NDEBUG
  size_t chunk_index = mem_heap_get_chunk_from_address ((void*) uintptr_chunk_aligned);
  JERRY_ASSERT (mem_heap_length_types[chunk_index] == mem_block_length_type_t::ONE_CHUNKED);
#endif /* !JERRY_NDEBUG */

  return (void*) uintptr_chunk_aligned;
} /* mem_heap_get_chunked_block_start */

/**
 * Get size of one-chunked block data space
 */
size_t
mem_heap_get_chunked_block_data_size (void)
{
  return MEM_HEAP_CHUNK_SIZE;
} /* mem_heap_get_chunked_block_data_size */

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
uintptr_t
mem_heap_compress_pointer (const void *pointer_p) /**< pointer to compress */
{
  JERRY_ASSERT (pointer_p != NULL);

  uintptr_t int_ptr = (uintptr_t) pointer_p;
  uintptr_t heap_start = (uintptr_t) &mem_heap;

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
void*
mem_heap_decompress_pointer (uintptr_t compressed_pointer) /**< pointer to decompress */
{
  JERRY_ASSERT (compressed_pointer != MEM_CP_NULL);

  uintptr_t int_ptr = compressed_pointer;
  uintptr_t heap_start = (uintptr_t) &mem_heap;

  int_ptr <<= MEM_ALIGNMENT_LOG;
  int_ptr += heap_start;

  return (void*) int_ptr;
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
  uint8_t *uint8_pointer = (uint8_t*) pointer;

  return (uint8_pointer >= mem_heap.area && uint8_pointer <= ((uint8_t *) mem_heap.area + MEM_HEAP_AREA_SIZE));
} /* mem_is_heap_pointer */
#endif /* !JERRY_NDEBUG */

/**
 * Print heap block
 */
static void
mem_heap_print_block (bool dump_block_data, /**< print block with data (true)
                                             *   or print only block header (false) */
                      size_t start_chunk, /**< starting chunk of block */
                      size_t chunks_num, /**< number of chunks in the block */
                      bool is_free) /**< is the block free? */
{
  printf ("Block (%p): state=%s, size in chunks=%lu\n",
          (uint8_t *) mem_heap.area + start_chunk * MEM_HEAP_CHUNK_SIZE,
          is_free ? "free" : "allocated",
          (unsigned long) chunks_num);

  if (dump_block_data)
  {
    uint8_t *block_data_p = (uint8_t*) mem_heap.area;
    uint8_t *block_data_end_p = block_data_p + start_chunk * MEM_HEAP_CHUNK_SIZE;

#ifdef JERRY_VALGRIND
    VALGRIND_DISABLE_ERROR_REPORTING;
#endif /* JERRY_VALGRIND */

    while (block_data_p != block_data_end_p)
    {
      printf ("0x%02x ", *block_data_p++);
    }

#ifdef JERRY_VALGRIND
    VALGRIND_ENABLE_ERROR_REPORTING;
#endif /* JERRY_VALGRIND */

    printf ("\n");
  }
} /* mem_heap_print_block */

/**
 * Print heap
 */
void
mem_heap_print (bool dump_block_headers, /**< print block headers */
                bool dump_block_data, /**< print block with data (true)
                                           or print only block header (false) */
                bool dump_stats) /**< print heap stats */
{
  mem_check_heap ();

  JERRY_ASSERT (!dump_block_data || dump_block_headers);

  if (dump_block_headers)
  {
    VALGRIND_DEFINED_SPACE (mem_heap.bitmaps, sizeof (mem_heap.bitmaps));

    printf ("Heap: start=%p size=%lu\n",
            mem_heap.area,
            (unsigned long) MEM_HEAP_AREA_SIZE);

    bool is_free = true;
    size_t start_chunk = 0;
    size_t chunk_index;

    for (chunk_index = 0; chunk_index < MEM_HEAP_CHUNKS_NUM; chunk_index++)
    {
      size_t bitmap_item_index = chunk_index / MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM;
      size_t item_bit_index = chunk_index % MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM;
      mem_heap_bitmap_storage_item_t bit = MEM_HEAP_BITMAP_ITEM_BIT (item_bit_index);

      if ((MEM_HEAP_IS_FIRST_IN_BLOCK_BITMAP[bitmap_item_index] & bit) != 0
          || (((MEM_HEAP_IS_ALLOCATED_BITMAP[bitmap_item_index] & bit) == 0) != is_free))
      {
        mem_heap_print_block (dump_block_data, start_chunk, chunk_index - start_chunk, is_free);

        start_chunk = chunk_index;
        is_free = ((MEM_HEAP_IS_ALLOCATED_BITMAP[bitmap_item_index] & bit) == 0);
      }
    }

    VALGRIND_NOACCESS_SPACE (mem_heap.bitmaps, sizeof (mem_heap.bitmaps));

    mem_heap_print_block (dump_block_data, start_chunk, chunk_index - start_chunk, is_free);
  }

#ifdef MEM_STATS
  if (dump_stats)
  {
    printf ("Heap stats:\n");
    printf ("  Heap size = %zu bytes\n"
            "  Chunk size = %zu bytes\n"
            "  Allocated chunks count = %zu\n"
            "  Allocated = %zu bytes\n"
            "  Waste = %zu bytes\n"
            "  Peak allocated chunks count = %zu\n"
            "  Peak allocated = %zu bytes\n"
            "  Peak waste = %zu bytes\n",
            mem_heap_stats.size,
            MEM_HEAP_CHUNK_SIZE,
            mem_heap_stats.allocated_chunks,
            mem_heap_stats.allocated_bytes,
            mem_heap_stats.waste_bytes,
            mem_heap_stats.peak_allocated_chunks,
            mem_heap_stats.peak_allocated_bytes,
            mem_heap_stats.peak_waste_bytes);
  }
#else /* MEM_STATS */
  (void) dump_stats;
#endif /* !MEM_STATS */

  printf ("\n");
} /* mem_heap_print */

/**
 * Check heap consistency
 */
static void
mem_check_heap (void)
{
#ifndef JERRY_DISABLE_HEAVY_DEBUG
  size_t allocated_chunks_num = 0;

  VALGRIND_DEFINED_SPACE (mem_heap.bitmaps, sizeof (mem_heap.bitmaps));

  for (size_t chunk_index = 0; chunk_index < MEM_HEAP_CHUNKS_NUM; chunk_index++)
  {
    size_t bitmap_item_index = chunk_index / MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM;
    size_t item_bit_index = chunk_index % MEM_HEAP_BITMAP_BITS_IN_STORAGE_ITEM;
    mem_heap_bitmap_storage_item_t bit = MEM_HEAP_BITMAP_ITEM_BIT (item_bit_index);

    if ((MEM_HEAP_IS_FIRST_IN_BLOCK_BITMAP[bitmap_item_index] & bit) != 0)
    {
      JERRY_ASSERT ((MEM_HEAP_IS_ALLOCATED_BITMAP[bitmap_item_index] & bit) != 0);
    }

    if ((MEM_HEAP_IS_ALLOCATED_BITMAP[bitmap_item_index] & bit) != 0)
    {
      if (mem_heap_length_types[chunk_index] == mem_block_length_type_t::ONE_CHUNKED)
      {
        JERRY_ASSERT ((MEM_HEAP_IS_FIRST_IN_BLOCK_BITMAP[bitmap_item_index] & bit) != 0);
      }

      allocated_chunks_num++;
    }
  }

  VALGRIND_NOACCESS_SPACE (mem_heap.bitmaps, sizeof (mem_heap.bitmaps));

  JERRY_ASSERT (allocated_chunks_num == mem_heap_allocated_chunks);
#endif /* !JERRY_DISABLE_HEAVY_DEBUG */
} /* mem_check_heap */

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
  mem_heap_stats.peak_allocated_chunks = mem_heap_stats.allocated_chunks;
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
mem_heap_stat_alloc (size_t first_chunk_index, /**< first chunk of the allocated area */
                     size_t chunks_num) /**< number of chunks in the area */
{
  const size_t chunks = chunks_num;
  const size_t bytes = (size_t) mem_heap_allocated_bytes[first_chunk_index];
  const size_t waste_bytes = chunks * MEM_HEAP_CHUNK_SIZE - bytes;

  mem_heap_stats.allocated_chunks += chunks;
  mem_heap_stats.allocated_bytes += bytes;
  mem_heap_stats.waste_bytes += waste_bytes;

  if (mem_heap_stats.allocated_chunks > mem_heap_stats.peak_allocated_chunks)
  {
    mem_heap_stats.peak_allocated_chunks = mem_heap_stats.allocated_chunks;
  }
  if (mem_heap_stats.allocated_chunks > mem_heap_stats.global_peak_allocated_chunks)
  {
    mem_heap_stats.global_peak_allocated_chunks = mem_heap_stats.allocated_chunks;
  }

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

  JERRY_ASSERT (mem_heap_stats.allocated_bytes <= mem_heap_stats.size);
  JERRY_ASSERT (mem_heap_stats.allocated_chunks <= mem_heap_stats.size / MEM_HEAP_CHUNK_SIZE);
} /* mem_heap_stat_alloc */

/**
 * Account freeing
 */
static void
mem_heap_stat_free (size_t first_chunk_index, /**< first chunk of the freed area */
                    size_t chunks_num) /**< number of chunks in the area */
{
  const size_t chunks = chunks_num;
  const size_t bytes = (size_t) mem_heap_allocated_bytes[first_chunk_index];
  const size_t waste_bytes = chunks * MEM_HEAP_CHUNK_SIZE - bytes;

  JERRY_ASSERT (mem_heap_stats.allocated_bytes <= mem_heap_stats.size);
  JERRY_ASSERT (mem_heap_stats.allocated_chunks <= mem_heap_stats.size / MEM_HEAP_CHUNK_SIZE);

  JERRY_ASSERT (mem_heap_stats.allocated_chunks >= chunks);
  JERRY_ASSERT (mem_heap_stats.allocated_bytes >= bytes);
  JERRY_ASSERT (mem_heap_stats.waste_bytes >= waste_bytes);

  mem_heap_stats.allocated_chunks -= chunks;
  mem_heap_stats.allocated_bytes -= bytes;
  mem_heap_stats.waste_bytes -= waste_bytes;
} /* mem_heap_stat_free */
#endif /* MEM_STATS */

/**
 * @}
 * @}
 */
