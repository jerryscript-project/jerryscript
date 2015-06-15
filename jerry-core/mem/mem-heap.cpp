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

# define VALGRIND_NOACCESS_STRUCT(s)    (void)VALGRIND_MAKE_MEM_NOACCESS((s), sizeof (*(s)))
# define VALGRIND_UNDEFINED_STRUCT(s)   (void)VALGRIND_MAKE_MEM_UNDEFINED((s), sizeof (*(s)))
# define VALGRIND_DEFINED_STRUCT(s)     (void)VALGRIND_MAKE_MEM_DEFINED((s), sizeof (*(s)))
# define VALGRIND_NOACCESS_SPACE(p, s)  (void)VALGRIND_MAKE_MEM_NOACCESS((p), (s))
# define VALGRIND_UNDEFINED_SPACE(p, s) (void)VALGRIND_MAKE_MEM_UNDEFINED((p), (s))
# define VALGRIND_DEFINED_SPACE(p, s)   (void)VALGRIND_MAKE_MEM_DEFINED((p), (s))
#else /* JERRY_VALGRIND */
# define VALGRIND_NOACCESS_STRUCT(s)
# define VALGRIND_UNDEFINED_STRUCT(s)
# define VALGRIND_DEFINED_STRUCT(s)
# define VALGRIND_NOACCESS_SPACE(p, s)
# define VALGRIND_UNDEFINED_SPACE(p, s)
# define VALGRIND_DEFINED_SPACE(p, s)
#endif /* JERRY_VALGRIND */

/**
 * State of the block to initialize (argument of mem_init_block_header)
 *
 * @see mem_init_block_header
 */
typedef enum
{
  MEM_BLOCK_FREE,     /**< initializing free block */
  MEM_BLOCK_ALLOCATED /**< initializing allocated block */
} mem_block_state_t;

/**
 * Length type of the block
 *
 * @see mem_init_block_header
 */
typedef enum : uint8_t
{
  ONE_CHUNKED, /**< one-chunked block (See also: mem_heap_alloc_chunked_block) */
  GENERAL      /**< general (may be multi-chunk) block */
} mem_block_length_type_t;

/**
 * Linked list direction descriptors
 */
typedef enum
{
  MEM_DIRECTION_PREV = 0,  /**< direction from right to left */
  MEM_DIRECTION_NEXT = 1,  /**< direction from left to right */
  MEM_DIRECTION_COUNT = 2  /**< count of possible directions */
} mem_direction_t;

/**
 * Offset in the heap
 */
#if MEM_HEAP_OFFSET_LOG <= 16
typedef uint16_t mem_heap_offset_t;
#elif MEM_HEAP_OFFSET_LOG <= 32
typedef uint32_t mem_heap_offset_t;
#else /* MEM_HEAP_OFFSET_LOG > 32 */
# error "MEM_HEAP_OFFSET_LOG > 32 is not supported"
#endif /* MEM_HEAP_OFFSET_LOG > 32 */
JERRY_STATIC_ASSERT (sizeof (mem_heap_offset_t) * JERRY_BITSINBYTE >= MEM_HEAP_OFFSET_LOG);

/**
 * Description of heap memory block layout
 */
typedef struct __attribute__ ((aligned (MEM_ALIGNMENT))) mem_block_header_t
{
  mem_heap_offset_t allocated_bytes : MEM_HEAP_OFFSET_LOG; /**< allocated area size - for allocated blocks;
                                                                0 - for free blocks */
  mem_heap_offset_t prev_p : MEM_HEAP_OFFSET_LOG; /**< previous block's offset;
                                                   *   0 - if the block is the first block */
  mem_heap_offset_t next_p : MEM_HEAP_OFFSET_LOG; /**< next block's offset;
                                                   *   0 - if the block is the last block */
  mem_block_length_type_t length_type; /**< length type of the block (one-chunked or general) */
} mem_block_header_t;

#if MEM_HEAP_OFFSET_LOG <= 16
/**
 * Check that block header's size is not more than 8 bytes
 */
JERRY_STATIC_ASSERT (sizeof (mem_block_header_t) <= sizeof (uint64_t));
#endif /* MEM_HEAP_OFFSET_LOG <= 16 */

/**
 * Chunk should have enough space for block header
 */
JERRY_STATIC_ASSERT (MEM_HEAP_CHUNK_SIZE >= sizeof (mem_block_header_t));

/**
 * Chunk size should satisfy the required alignment value
 */
JERRY_STATIC_ASSERT (MEM_HEAP_CHUNK_SIZE % MEM_ALIGNMENT == 0);

/**
 * Description of heap state
 */
typedef struct
{
  uint8_t* heap_start; /**< first address of heap space */
  size_t heap_size; /**< heap space size */
  mem_block_header_t* first_block_p; /**< first block of the heap */
  mem_block_header_t* last_block_p;  /**< last block of the heap */
  size_t allocated_bytes; /**< total size of allocated heap space */
  size_t limit; /**< current limit of heap usage, that is upon being reached,
                 *   causes call of "try give memory back" callbacks */
} mem_heap_state_t;

/**
 * Heap state
 */
mem_heap_state_t mem_heap;

static size_t mem_get_block_chunks_count (const mem_block_header_t *block_header_p);
static size_t mem_get_block_data_space_size (const mem_block_header_t *block_header_p);
static size_t mem_get_block_chunks_count_from_data_size (size_t block_allocated_size);
static bool mem_is_block_free (const mem_block_header_t *block_header_p);

static void mem_init_block_header (uint8_t *first_chunk_p,
                                   size_t size_in_chunks,
                                   mem_block_state_t block_state,
                                   mem_block_length_type_t length_type,
                                   mem_block_header_t *prev_block_p,
                                   mem_block_header_t *next_block_p);
static void mem_check_heap (void);

#ifdef MEM_STATS
/**
 * Heap's memory usage statistics
 */
static mem_heap_stats_t mem_heap_stats;

static void mem_heap_stat_init (void);
static void mem_heap_stat_alloc_block (mem_block_header_t *block_header_p);
static void mem_heap_stat_free_block (mem_block_header_t *block_header_p);
static void mem_heap_stat_free_block_split (void);
static void mem_heap_stat_free_block_merge (void);

#  define MEM_HEAP_STAT_INIT() mem_heap_stat_init ()
#  define MEM_HEAP_STAT_ALLOC_BLOCK(v) mem_heap_stat_alloc_block (v)
#  define MEM_HEAP_STAT_FREE_BLOCK(v) mem_heap_stat_free_block (v)
#  define MEM_HEAP_STAT_FREE_BLOCK_SPLIT() mem_heap_stat_free_block_split ()
#  define MEM_HEAP_STAT_FREE_BLOCK_MERGE() mem_heap_stat_free_block_merge ()
#else /* !MEM_STATS */
#  define MEM_HEAP_STAT_INIT()
#  define MEM_HEAP_STAT_ALLOC_BLOCK(v)
#  define MEM_HEAP_STAT_FREE_BLOCK(v)
#  define MEM_HEAP_STAT_FREE_BLOCK_SPLIT()
#  define MEM_HEAP_STAT_FREE_BLOCK_MERGE()
#endif /* !MEM_STATS */

/**
 * Measure distance between blocks.
 *
 * Warning:
 *         another_block_p should be greater than block_p.
 *
 * @return size in bytes between beginning of two blocks
 */
static mem_heap_offset_t
mem_get_blocks_distance (const mem_block_header_t* block_p, /**< block to measure offset from */
                         const mem_block_header_t* another_block_p) /**< block offset is measured for */
{
  JERRY_ASSERT (another_block_p >= block_p);

  size_t distance = (size_t) ((uint8_t*) another_block_p - (uint8_t*)block_p);

  JERRY_ASSERT (distance == (mem_heap_offset_t) distance);

  return (mem_heap_offset_t) distance;
} /* mem_get_blocks_distance */

/**
 * Get value for neighbour field.
 *
 * Note:
 *      If second_block_p is next neighbour of first_block_p,
 *         then first_block_p->next_p = ret_val
 *              second_block_p->prev_p = ret_val
 *
 * @return offset value for prev_p / next_p field
 */
static mem_heap_offset_t
mem_get_block_neighbour_field (const mem_block_header_t* first_block_p, /**< first of the blocks
                                                                             in forward direction */
                               const mem_block_header_t* second_block_p) /**< second of the blocks
                                                                              in forward direction */
{
  JERRY_ASSERT (first_block_p != NULL
                || second_block_p != NULL);

  if (first_block_p == NULL
      || second_block_p == NULL)
  {
    return 0;
  }
  else
  {
    JERRY_ASSERT (first_block_p < second_block_p);

    return mem_get_blocks_distance (first_block_p, second_block_p);
  }
} /* mem_get_block_neighbour_field */

/**
 * Set previous for the block
 */
static void
mem_set_block_prev (mem_block_header_t *block_p, /**< block to set previous for */
                    mem_block_header_t *prev_block_p) /**< previous block (or NULL) */
{
  mem_heap_offset_t offset = mem_get_block_neighbour_field (prev_block_p, block_p);

  block_p->prev_p = (offset) & MEM_HEAP_OFFSET_MASK;
} /* mem_set_block_prev */

/**
 * Set next for the block
 */
static void
mem_set_block_next (mem_block_header_t *block_p, /**< block to set next for */
                    mem_block_header_t *next_block_p) /**< next block or NULL */
{
  mem_heap_offset_t offset = mem_get_block_neighbour_field (block_p, next_block_p);

  block_p->next_p = (offset) & MEM_HEAP_OFFSET_MASK;
} /* mem_set_block_next */

/**
 * Set allocated bytes for the block
 */
static void
mem_set_block_allocated_bytes (mem_block_header_t *block_p, /**< block to set allocated bytes field for */
                               size_t allocated_bytes) /**< number of bytes allocated in the block */
{
  block_p->allocated_bytes = allocated_bytes & MEM_HEAP_OFFSET_MASK;

  JERRY_ASSERT (block_p->allocated_bytes == allocated_bytes);
} /* mem_set_block_allocated_bytes */

/**
 * Set the block's length type
 */
static void
mem_set_block_set_length_type (mem_block_header_t *block_p, /**< block to set length type field for */
                               mem_block_length_type_t length_type) /**< length type of the block */
{
  block_p->length_type = length_type;
} /* mem_set_block_set_length_type */

/**
 * Get block located at specified offset from specified block.
 *
 * @return pointer to block header, located offset bytes after specified block (if dir is next),
 *         pointer to block header, located offset bytes before specified block (if dir is prev).
 */
static mem_block_header_t*
mem_get_block_by_offset (const mem_block_header_t* block_p, /**< block */
                         mem_heap_offset_t offset, /**< offset */
                         mem_direction_t dir) /**< direction of offset */
{
  const uint8_t* uint8_block_p = (uint8_t*) block_p;

  if (dir == MEM_DIRECTION_NEXT)
  {
    return (mem_block_header_t*) (uint8_block_p + offset);
  }
  else
  {
    return (mem_block_header_t*) (uint8_block_p - offset);
  }
} /* mem_get_block_by_offset */

/**
 * Get next block in specified direction.
 *
 * @return pointer to next block in direction specified by dir,
 *         or NULL - if the block is last in specified direction.
 */
static mem_block_header_t*
mem_get_next_block_by_direction (const mem_block_header_t* block_p, /**< block */
                                 mem_direction_t dir) /**< direction */
{
  mem_heap_offset_t offset = (dir == MEM_DIRECTION_NEXT ? block_p->next_p : block_p->prev_p);
  if (offset != 0)
  {
    return mem_get_block_by_offset (block_p,
                                    offset,
                                    dir);
  }
  else
  {
    return NULL;
  }
} /* mem_get_next_block_by_direction */

/**
 * get chunk count, used by the block.
 *
 * @return chunks count
 */
static size_t
mem_get_block_chunks_count (const mem_block_header_t *block_header_p) /**< block header */
{
  JERRY_ASSERT (block_header_p != NULL);

  const mem_block_header_t *next_block_p = mem_get_next_block_by_direction (block_header_p, MEM_DIRECTION_NEXT);
  size_t dist_till_block_end;

  if (next_block_p == NULL)
  {
    dist_till_block_end = (size_t) (mem_heap.heap_start + mem_heap.heap_size - (uint8_t*) block_header_p);
  }
  else
  {
    dist_till_block_end = (size_t) ((uint8_t*) next_block_p - (uint8_t*) block_header_p);
  }

  JERRY_ASSERT (dist_till_block_end <= mem_heap.heap_size);
  JERRY_ASSERT (dist_till_block_end % MEM_HEAP_CHUNK_SIZE == 0);

  return dist_till_block_end / MEM_HEAP_CHUNK_SIZE;
} /* mem_get_block_chunks_count */

/**
 * Calculate block's data space size
 *
 * @return size of block area that can be used to store data
 */
static size_t
mem_get_block_data_space_size (const mem_block_header_t *block_header_p) /**< block header */
{
  return mem_get_block_chunks_count (block_header_p) * MEM_HEAP_CHUNK_SIZE - sizeof (mem_block_header_t);
} /* mem_get_block_data_space_size */

/**
 * Calculate minimum chunks count needed for block with specified size of allocated data area.
 *
 * @return chunks count
 */
static size_t
mem_get_block_chunks_count_from_data_size (size_t block_allocated_size) /**< size of block's allocated area */
{
  return JERRY_ALIGNUP (sizeof (mem_block_header_t) + block_allocated_size, MEM_HEAP_CHUNK_SIZE) / MEM_HEAP_CHUNK_SIZE;
} /* mem_get_block_chunks_count_from_data_size */

/**
 * Check whether specified block is free.
 *
 * @return true - if block is free,
 *         false - otherwise
 */
static bool
mem_is_block_free (const mem_block_header_t *block_header_p) /**< block header */
{
  return (block_header_p->allocated_bytes == 0);
} /* mem_is_block_free */

/**
 * Startup initialization of heap
 *
 * Note:
 *      heap start and size should be aligned on MEM_HEAP_CHUNK_SIZE
 */
void
mem_heap_init (uint8_t *heap_start, /**< first address of heap space */
               size_t heap_size)    /**< heap space size */
{
  JERRY_ASSERT (heap_start != NULL);
  JERRY_ASSERT (heap_size != 0);

  JERRY_STATIC_ASSERT ((MEM_HEAP_CHUNK_SIZE & (MEM_HEAP_CHUNK_SIZE - 1u)) == 0);
  JERRY_ASSERT ((uintptr_t) heap_start % MEM_ALIGNMENT == 0);
  JERRY_ASSERT ((uintptr_t) heap_start % MEM_HEAP_CHUNK_SIZE == 0);
  JERRY_ASSERT (heap_size % MEM_HEAP_CHUNK_SIZE == 0);

  JERRY_ASSERT (heap_size <= (1u << MEM_HEAP_OFFSET_LOG));

  mem_heap.heap_start = heap_start;
  mem_heap.heap_size = heap_size;
  mem_heap.limit = CONFIG_MEM_HEAP_DESIRED_LIMIT;

  VALGRIND_NOACCESS_SPACE (heap_start, heap_size);

  mem_init_block_header (mem_heap.heap_start,
                         0,
                         MEM_BLOCK_FREE,
                         mem_block_length_type_t::GENERAL,
                         NULL,
                         NULL);

  mem_heap.first_block_p = (mem_block_header_t*) mem_heap.heap_start;
  mem_heap.last_block_p = mem_heap.first_block_p;

  MEM_HEAP_STAT_INIT ();
} /* mem_heap_init */

/**
 * Finalize heap
 */
void
mem_heap_finalize (void)
{
  VALGRIND_DEFINED_SPACE (mem_heap.heap_start, mem_heap.heap_size);

  JERRY_ASSERT (mem_heap.first_block_p == mem_heap.last_block_p);
  JERRY_ASSERT (mem_is_block_free (mem_heap.first_block_p));

  VALGRIND_NOACCESS_SPACE (mem_heap.heap_start, mem_heap.heap_size);

  memset (&mem_heap, 0, sizeof (mem_heap));
} /* mem_heap_finalize */

/**
 * Initialize block header located in the specified first chunk of the block
 */
static void
mem_init_block_header (uint8_t *first_chunk_p, /**< address of the first chunk to use for the block */
                       size_t allocated_bytes, /**< size of block's allocated area */
                       mem_block_state_t block_state, /**< state of the block (allocated or free) */
                       mem_block_length_type_t length_type, /**< length type of the block (one-chunked or general) */
                       mem_block_header_t *prev_block_p, /**< previous block */
                       mem_block_header_t *next_block_p) /**< next block */
{
  mem_block_header_t *block_header_p = (mem_block_header_t*) first_chunk_p;

  VALGRIND_UNDEFINED_STRUCT (block_header_p);

  mem_set_block_prev (block_header_p, prev_block_p);
  mem_set_block_next (block_header_p, next_block_p);
  mem_set_block_allocated_bytes (block_header_p, allocated_bytes);
  mem_set_block_set_length_type (block_header_p, length_type);

  JERRY_ASSERT (allocated_bytes <= mem_get_block_data_space_size (block_header_p));

  if (block_state == MEM_BLOCK_FREE)
  {
    JERRY_ASSERT (allocated_bytes == 0);
    JERRY_ASSERT (length_type == mem_block_length_type_t::GENERAL);
    JERRY_ASSERT (mem_is_block_free (block_header_p));
  }
  else
  {
    JERRY_ASSERT (allocated_bytes != 0);
    JERRY_ASSERT (!mem_is_block_free (block_header_p));
  }

  VALGRIND_NOACCESS_STRUCT (block_header_p);
} /* mem_init_block_header */

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
  mem_block_header_t *block_p;
  mem_direction_t direction;

  JERRY_ASSERT (size_in_bytes != 0);
  JERRY_ASSERT (length_type != mem_block_length_type_t::ONE_CHUNKED
                || size_in_bytes == mem_heap_get_chunked_block_data_size ());

  mem_check_heap ();

  if (alloc_term == MEM_HEAP_ALLOC_LONG_TERM)
  {
    block_p = mem_heap.first_block_p;
    direction = MEM_DIRECTION_NEXT;
  }
  else
  {
    JERRY_ASSERT (alloc_term == MEM_HEAP_ALLOC_SHORT_TERM);

    block_p = mem_heap.last_block_p;
    direction = MEM_DIRECTION_PREV;
  }

  /* searching for appropriate block */
  while (block_p != NULL)
  {
    VALGRIND_DEFINED_STRUCT (block_p);

    if (mem_is_block_free (block_p))
    {
      if (mem_get_block_data_space_size (block_p) >= size_in_bytes)
      {
        break;
      }
    }
    else
    {
      JERRY_ASSERT (!mem_is_block_free (block_p));
    }

    mem_block_header_t *next_block_p = mem_get_next_block_by_direction (block_p, direction);

    VALGRIND_NOACCESS_STRUCT (block_p);

    block_p = next_block_p;
  }

  if (block_p == NULL)
  {
    /* not enough free space */
    return NULL;
  }

  mem_heap.allocated_bytes += size_in_bytes;

  JERRY_ASSERT (mem_heap.allocated_bytes <= mem_heap.heap_size);

  if (mem_heap.allocated_bytes >= mem_heap.limit)
  {
    mem_heap.limit = JERRY_MIN (mem_heap.heap_size,
                                JERRY_MAX (mem_heap.limit + CONFIG_MEM_HEAP_DESIRED_LIMIT,
                                           mem_heap.allocated_bytes));
    JERRY_ASSERT (mem_heap.limit >= mem_heap.allocated_bytes);
  }

  /* appropriate block found, allocating space */
  size_t new_block_size_in_chunks = mem_get_block_chunks_count_from_data_size (size_in_bytes);
  size_t found_block_size_in_chunks = mem_get_block_chunks_count (block_p);

  JERRY_ASSERT (new_block_size_in_chunks <= found_block_size_in_chunks);

  mem_block_header_t *prev_block_p = mem_get_next_block_by_direction (block_p, MEM_DIRECTION_PREV);
  mem_block_header_t *next_block_p = mem_get_next_block_by_direction (block_p, MEM_DIRECTION_NEXT);

  if (new_block_size_in_chunks < found_block_size_in_chunks)
  {
    MEM_HEAP_STAT_FREE_BLOCK_SPLIT ();

    if (direction == MEM_DIRECTION_PREV)
    {
      prev_block_p = block_p;
      uint8_t *block_end_p = (uint8_t*) block_p + found_block_size_in_chunks * MEM_HEAP_CHUNK_SIZE;
      block_p = (mem_block_header_t*) (block_end_p - new_block_size_in_chunks * MEM_HEAP_CHUNK_SIZE);

      VALGRIND_DEFINED_STRUCT (prev_block_p);

      mem_set_block_next (prev_block_p, block_p);

      VALGRIND_NOACCESS_STRUCT (prev_block_p);

      if (next_block_p == NULL)
      {
        mem_heap.last_block_p = block_p;
      }
      else
      {
        VALGRIND_DEFINED_STRUCT (next_block_p);

        mem_set_block_prev (next_block_p, block_p);

        VALGRIND_NOACCESS_STRUCT (next_block_p);
      }
    }
    else
    {
      uint8_t *new_free_block_first_chunk_p = (uint8_t*) block_p + new_block_size_in_chunks * MEM_HEAP_CHUNK_SIZE;
      mem_init_block_header (new_free_block_first_chunk_p,
                             0,
                             MEM_BLOCK_FREE,
                             mem_block_length_type_t::GENERAL,
                             block_p,
                             next_block_p);

      mem_block_header_t *new_free_block_p = (mem_block_header_t*) new_free_block_first_chunk_p;

      if (next_block_p == NULL)
      {
        mem_heap.last_block_p = new_free_block_p;
      }
      else
      {
        VALGRIND_DEFINED_STRUCT (next_block_p);

        mem_block_header_t* new_free_block_p = (mem_block_header_t*) new_free_block_first_chunk_p;
        mem_set_block_prev (next_block_p, new_free_block_p);

        VALGRIND_NOACCESS_STRUCT (next_block_p);
      }

      next_block_p = new_free_block_p;
    }
  }

  mem_init_block_header ((uint8_t*) block_p,
                         size_in_bytes,
                         MEM_BLOCK_ALLOCATED,
                         length_type,
                         prev_block_p,
                         next_block_p);

  VALGRIND_DEFINED_STRUCT (block_p);

  MEM_HEAP_STAT_ALLOC_BLOCK (block_p);

  JERRY_ASSERT (mem_get_block_data_space_size (block_p) >= size_in_bytes);

  VALGRIND_NOACCESS_STRUCT (block_p);

  /* return data space beginning address */
  uint8_t *data_space_p = (uint8_t*) (block_p + 1);
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
  if (mem_heap.allocated_bytes + size_in_bytes >= mem_heap.limit)
  {
    mem_run_try_to_give_memory_back_callbacks (MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_LOW);
  }

  void *data_space_p = mem_heap_alloc_block_internal (size_in_bytes, length_type, alloc_term);

  if (likely (data_space_p != NULL))
  {
    return data_space_p;
  }

  for (mem_try_give_memory_back_severity_t severity = MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_LOW;
       severity <= MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_CRITICAL;
       severity = (mem_try_give_memory_back_severity_t) (severity + 1))
  {
    mem_run_try_to_give_memory_back_callbacks (severity);

    data_space_p = mem_heap_alloc_block_internal (size_in_bytes, length_type, alloc_term);

    if (data_space_p != NULL)
    {
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
  uint8_t *uint8_ptr = (uint8_t*) ptr;

  /* checking that uint8_ptr points to the heap */
  JERRY_ASSERT (uint8_ptr >= mem_heap.heap_start
                && uint8_ptr <= mem_heap.heap_start + mem_heap.heap_size);

  mem_check_heap ();

  mem_block_header_t *block_p = (mem_block_header_t*) uint8_ptr - 1;

  VALGRIND_DEFINED_STRUCT (block_p);

  mem_block_header_t *prev_block_p = mem_get_next_block_by_direction (block_p, MEM_DIRECTION_PREV);
  mem_block_header_t *next_block_p = mem_get_next_block_by_direction (block_p, MEM_DIRECTION_NEXT);

  JERRY_ASSERT (mem_heap.limit >= mem_heap.allocated_bytes);

  size_t bytes = block_p->allocated_bytes;
  JERRY_ASSERT (mem_heap.allocated_bytes >= bytes);
  mem_heap.allocated_bytes -= bytes;

  if (mem_heap.allocated_bytes * 3 <= mem_heap.limit)
  {
    mem_heap.limit /= 2;
  }
  else if (mem_heap.allocated_bytes + CONFIG_MEM_HEAP_DESIRED_LIMIT <= mem_heap.limit)
  {
    mem_heap.limit -= CONFIG_MEM_HEAP_DESIRED_LIMIT;
  }

  JERRY_ASSERT (mem_heap.limit >= mem_heap.allocated_bytes);

  MEM_HEAP_STAT_FREE_BLOCK (block_p);

  VALGRIND_NOACCESS_SPACE (uint8_ptr, block_p->allocated_bytes);

  JERRY_ASSERT (!mem_is_block_free (block_p));

  /* marking the block free */
  block_p->allocated_bytes = 0;
  block_p->length_type = mem_block_length_type_t::GENERAL;

  if (next_block_p != NULL)
  {
    VALGRIND_DEFINED_STRUCT (next_block_p);

    if (mem_is_block_free (next_block_p))
    {
      /* merge with the next block */
      MEM_HEAP_STAT_FREE_BLOCK_MERGE ();

      mem_block_header_t *next_next_block_p = mem_get_next_block_by_direction (next_block_p, MEM_DIRECTION_NEXT);

      VALGRIND_NOACCESS_STRUCT (next_block_p);

      next_block_p = next_next_block_p;

      VALGRIND_DEFINED_STRUCT (next_block_p);

      mem_set_block_next (block_p, next_block_p);
      if (next_block_p != NULL)
      {
        mem_set_block_prev (next_block_p, block_p);
      }
      else
      {
        mem_heap.last_block_p = block_p;
      }
    }

    VALGRIND_NOACCESS_STRUCT (next_block_p);
  }

  if (prev_block_p != NULL)
  {
    VALGRIND_DEFINED_STRUCT (prev_block_p);

    if (mem_is_block_free (prev_block_p))
    {
      /* merge with the previous block */
      MEM_HEAP_STAT_FREE_BLOCK_MERGE ();

      mem_set_block_next (prev_block_p, next_block_p);
      if (next_block_p != NULL)
      {
        VALGRIND_DEFINED_STRUCT (next_block_p);

        mem_block_header_t* prev_block_p = mem_get_next_block_by_direction (block_p, MEM_DIRECTION_PREV);
        mem_set_block_prev (next_block_p, prev_block_p);

        VALGRIND_NOACCESS_STRUCT (next_block_p);
      }
      else
      {
        mem_heap.last_block_p = prev_block_p;
      }
    }

    VALGRIND_NOACCESS_STRUCT (prev_block_p);
  }

  VALGRIND_NOACCESS_STRUCT (block_p);

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
  JERRY_ASSERT (((uintptr_t) mem_heap.heap_start % MEM_HEAP_CHUNK_SIZE) == 0);

  JERRY_ASSERT (mem_heap.heap_start <= ptr
                && ptr < mem_heap.heap_start + mem_heap.heap_size);

  uintptr_t uintptr = (uintptr_t) ptr;
  uintptr_t uintptr_chunk_aligned = JERRY_ALIGNDOWN (uintptr, MEM_HEAP_CHUNK_SIZE);

  JERRY_ASSERT (uintptr > uintptr_chunk_aligned);

  mem_block_header_t *block_p = (mem_block_header_t *) uintptr_chunk_aligned;
  JERRY_ASSERT (block_p->length_type == mem_block_length_type_t::ONE_CHUNKED);

#ifndef JERRY_NDEBUG
  const mem_block_header_t *block_iter_p = mem_heap.first_block_p;
  bool is_found = false;

  /* searching for corresponding block */
  while (block_iter_p != NULL)
  {
    VALGRIND_DEFINED_STRUCT (block_iter_p);

    const mem_block_header_t *next_block_p = mem_get_next_block_by_direction (block_iter_p,
                                                                              MEM_DIRECTION_NEXT);
    is_found = (ptr > block_iter_p
                && (ptr < next_block_p
                    || next_block_p == NULL));

    if (is_found)
    {
      JERRY_ASSERT (!mem_is_block_free (block_iter_p));
      JERRY_ASSERT (block_iter_p + 1 <= ptr);
      JERRY_ASSERT (ptr < ((uint8_t*) (block_iter_p + 1) + block_iter_p->allocated_bytes));
    }

    VALGRIND_NOACCESS_STRUCT (block_iter_p);

    if (is_found)
    {
      break;
    }

    block_iter_p = next_block_p;
  }

  JERRY_ASSERT (is_found && block_p == block_iter_p);
#endif /* !JERRY_NDEBUG */

  return (void*) (block_p + 1);
} /* mem_heap_get_chunked_block_start */

/**
 * Get size of one-chunked block data space
 */
size_t
mem_heap_get_chunked_block_data_size (void)
{
  return (MEM_HEAP_CHUNK_SIZE - sizeof (mem_block_header_t));
} /* mem_heap_get_chunked_block_data_size */

/**
 * Recommend allocation size based on chunk size.
 *
 * @return recommended allocation size
 */
size_t __attr_pure___
mem_heap_recommend_allocation_size (size_t minimum_allocation_size) /**< minimum allocation size */
{
  size_t minimum_allocation_size_with_block_header = minimum_allocation_size + sizeof (mem_block_header_t);
  size_t heap_chunk_aligned_allocation_size = JERRY_ALIGNUP (minimum_allocation_size_with_block_header,
                                                             MEM_HEAP_CHUNK_SIZE);

  return heap_chunk_aligned_allocation_size - sizeof (mem_block_header_t);
} /* mem_heap_recommend_allocation_size */

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
    printf ("Heap: start=%p size=%lu, first block->%p, last block->%p\n",
            mem_heap.heap_start,
            (unsigned long) mem_heap.heap_size,
            (void*) mem_heap.first_block_p,
            (void*) mem_heap.last_block_p);

    for (mem_block_header_t *block_p = mem_heap.first_block_p, *next_block_p;
         block_p != NULL;
         block_p = next_block_p)
    {
      VALGRIND_DEFINED_STRUCT (block_p);

      printf ("Block (%p): state=%s, size in chunks=%lu, previous block->%p next block->%p\n",
              (void*) block_p,
              mem_is_block_free (block_p) ? "free" : "allocated",
              (unsigned long) mem_get_block_chunks_count (block_p),
              (void*) mem_get_next_block_by_direction (block_p, MEM_DIRECTION_PREV),
              (void*) mem_get_next_block_by_direction (block_p, MEM_DIRECTION_NEXT));

      if (dump_block_data)
      {
        uint8_t *block_data_p = (uint8_t*) (block_p + 1);
        for (uint32_t offset = 0;
             offset < mem_get_block_data_space_size (block_p);
             offset++)
        {
          printf ("%02x ", block_data_p[ offset ]);
        }
        printf ("\n");
      }

      next_block_p = mem_get_next_block_by_direction (block_p, MEM_DIRECTION_NEXT);

      VALGRIND_NOACCESS_STRUCT (block_p);
    }
  }

#ifdef MEM_STATS
  if (dump_stats)
  {
    printf ("Heap stats:\n");
    printf ("  Heap size = %zu bytes\n"
            "  Chunk size = %zu bytes\n"
            "  Blocks count = %zu\n"
            "  Allocated blocks count = %zu\n"
            "  Allocated chunks count = %zu\n"
            "  Allocated = %zu bytes\n"
            "  Waste = %zu bytes\n"
            "  Peak allocated blocks count = %zu\n"
            "  Peak allocated chunks count = %zu\n"
            "  Peak allocated= %zu bytes\n"
            "  Peak waste = %zu bytes\n",
            mem_heap_stats.size,
            MEM_HEAP_CHUNK_SIZE,
            mem_heap_stats.blocks,
            mem_heap_stats.allocated_blocks,
            mem_heap_stats.allocated_chunks,
            mem_heap_stats.allocated_bytes,
            mem_heap_stats.waste_bytes,
            mem_heap_stats.peak_allocated_blocks,
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
  JERRY_ASSERT ((uint8_t*) mem_heap.first_block_p == mem_heap.heap_start);
  JERRY_ASSERT (mem_heap.heap_size % MEM_HEAP_CHUNK_SIZE == 0);

  bool is_last_block_was_met = false;
  size_t chunk_sizes_sum = 0;
  size_t allocated_sum = 0;

  for (mem_block_header_t *block_p = mem_heap.first_block_p, *next_block_p;
       block_p != NULL;
       block_p = next_block_p)
  {
    VALGRIND_DEFINED_STRUCT (block_p);

    JERRY_ASSERT (block_p->length_type != mem_block_length_type_t::ONE_CHUNKED
                  || block_p->allocated_bytes == mem_heap_get_chunked_block_data_size ());

    chunk_sizes_sum += mem_get_block_chunks_count (block_p);

    if (!mem_is_block_free (block_p))
    {
      allocated_sum += block_p->allocated_bytes;
    }

    next_block_p = mem_get_next_block_by_direction (block_p, MEM_DIRECTION_NEXT);

    if (block_p == mem_heap.last_block_p)
    {
      is_last_block_was_met = true;

      JERRY_ASSERT (next_block_p == NULL);
    }
    else
    {
      JERRY_ASSERT (next_block_p != NULL);
    }

    VALGRIND_NOACCESS_STRUCT (block_p);
  }

  JERRY_ASSERT (chunk_sizes_sum * MEM_HEAP_CHUNK_SIZE == mem_heap.heap_size);
  JERRY_ASSERT (allocated_sum == mem_heap.allocated_bytes);
  JERRY_ASSERT (is_last_block_was_met);

  bool is_first_block_was_met = false;
  chunk_sizes_sum = 0;

  for (mem_block_header_t *block_p = mem_heap.last_block_p, *prev_block_p;
       block_p != NULL;
       block_p = prev_block_p)
  {
    VALGRIND_DEFINED_STRUCT (block_p);

    chunk_sizes_sum += mem_get_block_chunks_count (block_p);

    prev_block_p = mem_get_next_block_by_direction (block_p, MEM_DIRECTION_PREV);

    if (block_p == mem_heap.first_block_p)
    {
      is_first_block_was_met = true;

      JERRY_ASSERT (prev_block_p == NULL);
    }
    else
    {
      JERRY_ASSERT (prev_block_p != NULL);
    }

    VALGRIND_NOACCESS_STRUCT (block_p);
  }

  JERRY_ASSERT (chunk_sizes_sum * MEM_HEAP_CHUNK_SIZE == mem_heap.heap_size);
  JERRY_ASSERT (is_first_block_was_met);
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
  mem_heap_stats.peak_allocated_blocks = mem_heap_stats.allocated_blocks;
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

  mem_heap_stats.size = mem_heap.heap_size;
  mem_heap_stats.blocks = 1;
} /* mem_heap_stat_init */

/**
 * Account block allocation
 */
static void
mem_heap_stat_alloc_block (mem_block_header_t *block_header_p) /**< allocated block */
{
  JERRY_ASSERT (!mem_is_block_free (block_header_p));

  const size_t chunks = mem_get_block_chunks_count (block_header_p);
  const size_t bytes = block_header_p->allocated_bytes;
  const size_t waste_bytes = chunks * MEM_HEAP_CHUNK_SIZE - bytes;

  mem_heap_stats.allocated_blocks++;
  mem_heap_stats.allocated_chunks += chunks;
  mem_heap_stats.allocated_bytes += bytes;
  mem_heap_stats.waste_bytes += waste_bytes;

  if (mem_heap_stats.allocated_blocks > mem_heap_stats.peak_allocated_blocks)
  {
    mem_heap_stats.peak_allocated_blocks = mem_heap_stats.allocated_blocks;
  }
  if (mem_heap_stats.allocated_blocks > mem_heap_stats.global_peak_allocated_blocks)
  {
    mem_heap_stats.global_peak_allocated_blocks = mem_heap_stats.allocated_blocks;
  }

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

  JERRY_ASSERT (mem_heap_stats.allocated_blocks <= mem_heap_stats.blocks);
  JERRY_ASSERT (mem_heap_stats.allocated_bytes <= mem_heap_stats.size);
  JERRY_ASSERT (mem_heap_stats.allocated_chunks <= mem_heap_stats.size / MEM_HEAP_CHUNK_SIZE);
} /* mem_heap_stat_alloc_block */

/**
 * Account block freeing
 */
static void
mem_heap_stat_free_block (mem_block_header_t *block_header_p) /**< block to be freed */
{
  JERRY_ASSERT (!mem_is_block_free (block_header_p));

  const size_t chunks = mem_get_block_chunks_count (block_header_p);
  const size_t bytes = block_header_p->allocated_bytes;
  const size_t waste_bytes = chunks * MEM_HEAP_CHUNK_SIZE - bytes;

  JERRY_ASSERT (mem_heap_stats.allocated_blocks <= mem_heap_stats.blocks);
  JERRY_ASSERT (mem_heap_stats.allocated_bytes <= mem_heap_stats.size);
  JERRY_ASSERT (mem_heap_stats.allocated_chunks <= mem_heap_stats.size / MEM_HEAP_CHUNK_SIZE);

  JERRY_ASSERT (mem_heap_stats.allocated_blocks >= 1);
  JERRY_ASSERT (mem_heap_stats.allocated_chunks >= chunks);
  JERRY_ASSERT (mem_heap_stats.allocated_bytes >= bytes);
  JERRY_ASSERT (mem_heap_stats.waste_bytes >= waste_bytes);

  mem_heap_stats.allocated_blocks--;
  mem_heap_stats.allocated_chunks -= chunks;
  mem_heap_stats.allocated_bytes -= bytes;
  mem_heap_stats.waste_bytes -= waste_bytes;
} /* mem_heap_stat_free_block */

/**
 * Account free block split
 */
static void
mem_heap_stat_free_block_split (void)
{
  mem_heap_stats.blocks++;
} /* mem_heap_stat_free_block_split */

/**
 * Account free block merge
 */
static void
mem_heap_stat_free_block_merge (void)
{
  mem_heap_stats.blocks--;
} /* mem_heap_stat_free_block_merge */
#endif /* MEM_STATS */

/**
 * @}
 * @}
 */
