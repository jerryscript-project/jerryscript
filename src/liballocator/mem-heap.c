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
 * Heap implementation
 */

#include "globals.h"
#include "jerry-libc.h"
#include "mem-allocator.h"
#include "mem-heap.h"

/*
 * Valgrind-related options and headers
 */
#ifndef JERRY_NVALGRIND
# include "memcheck.h"

# define VALGRIND_NOACCESS_STRUCT( s)    (void)VALGRIND_MAKE_MEM_NOACCESS( ( s ), sizeof( *( s ) ) )
# define VALGRIND_UNDEFINED_STRUCT( s)   (void)VALGRIND_MAKE_MEM_UNDEFINED( ( s ), sizeof( *( s ) ) )
# define VALGRIND_DEFINED_STRUCT( s)     (void)VALGRIND_MAKE_MEM_DEFINED( ( s ), sizeof( *( s ) ) )
# define VALGRIND_NOACCESS_SPACE( p, s)  (void)VALGRIND_MAKE_MEM_NOACCESS( ( p ), ( s ) )
# define VALGRIND_UNDEFINED_SPACE( p, s) (void)VALGRIND_MAKE_MEM_UNDEFINED( ( p ), ( s ) )
# define VALGRIND_DEFINED_SPACE( p, s)   (void)VALGRIND_MAKE_MEM_DEFINED( ( p ), ( s ) )
#else /* !JERRRY_NVALGRIND */
# define VALGRIND_NOACCESS_STRUCT( s)
# define VALGRIND_UNDEFINED_STRUCT( s)
# define VALGRIND_DEFINED_STRUCT( s)
# define VALGRIND_NOACCESS_SPACE( p, s)
# define VALGRIND_UNDEFINED_SPACE( p, s)
# define VALGRIND_DEFINED_SPACE( p, s)
#endif /* !JERRY_NVALGRIND */

/**
 * Magic numbers for heap memory blocks
 */
typedef enum
{
  MEM_MAGIC_NUM_OF_FREE_BLOCK      = 0x31d7c809,
  MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK = 0x59d75b46
} mem_magic_num_of_block_t;

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
 * Linked list direction descriptors
 */
typedef enum
{
  MEM_DIRECTION_PREV = 0,  /**< direction from right to left */
  MEM_DIRECTION_NEXT = 1,  /**< direction from left to right */
  MEM_DIRECTION_COUNT = 2  /**< count of possible directions */
} mem_direction_t;

/**
 * Description of heap memory block layout
 */
typedef struct mem_block_header_t
{
  mem_magic_num_of_block_t magic_num;                              /**< magic number - MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK for allocated block
                                                                    and MEM_MAGIC_NUM_OF_FREE_BLOCK for free block */
  struct mem_block_header_t *neighbours[ MEM_DIRECTION_COUNT ]; /**< neighbour blocks */
  size_t allocated_bytes;                                      /**< allocated area size - for allocated blocks; 0 - for free blocks */
} mem_block_header_t;

/**
 * Chunk should have enough space for block header
 */
JERRY_STATIC_ASSERT( MEM_HEAP_CHUNK_SIZE >= sizeof (mem_block_header_t) );

/**
 * Chunk size should satisfy the required alignment value
 */
JERRY_STATIC_ASSERT( MEM_HEAP_CHUNK_SIZE % MEM_ALIGNMENT == 0 );

/**
 * Description of heap state
 */
typedef struct
{
  uint8_t* heap_start;             /**< first address of heap space */
  size_t heap_size;                /**< heap space size */
  mem_block_header_t* first_block_p; /**< first block of the heap */
  mem_block_header_t* last_block_p;  /**< last block of the heap */
} mem_heap_state_t;

/**
 * Heap state
 */
mem_heap_state_t mem_heap;

static size_t mem_get_block_chunks_count( const mem_block_header_t *block_header_p);
static size_t mem_get_block_data_space_size( const mem_block_header_t *block_header_p);
static size_t mem_get_block_chunks_count_from_data_size( size_t block_allocated_size);

static void mem_init_block_header( uint8_t *first_chunk_p,
                                size_t size_in_chunks,
                                mem_block_state_t block_state,
                                mem_block_header_t *prev_block_p,
                                mem_block_header_t *next_block_p);
static void mem_check_heap( void);

#ifdef MEM_STATS
/**
 * Heap's memory usage statistics
 */
static mem_heap_stats_t mem_heap_stats;

static void mem_heap_stat_init( void);
static void mem_heap_stat_alloc_block( mem_block_header_t *block_header_p);
static void mem_heap_stat_free_block( mem_block_header_t *block_header_p);
static void mem_heap_stat_free_block_split( void);
static void mem_heap_stat_free_block_merge( void);
#else /* !MEM_STATS */
#  define mem_heap_stat_init()
#  define mem_heap_stat_alloc_block( v)
#  define mem_heap_stat_free_block( v)
#  define mem_heap_stat_free_block_split()
#  define mem_heap_stat_free_block_merge()
#endif /* !MEM_STATS */

/**
 * get chunk count, used by the block.
 *
 * @return chunks count
 */
static size_t
mem_get_block_chunks_count( const mem_block_header_t *block_header_p) /**< block header */
{
  JERRY_ASSERT( block_header_p != NULL );

  const mem_block_header_t *next_block_p = block_header_p->neighbours[ MEM_DIRECTION_NEXT ];
  size_t dist_till_block_end;

  if ( next_block_p == NULL )
  {
    dist_till_block_end = (size_t) ( mem_heap.heap_start + mem_heap.heap_size - (uint8_t*) block_header_p );
  } else
  {
    dist_till_block_end = (size_t) ( (uint8_t*) next_block_p - (uint8_t*) block_header_p );
  }

  JERRY_ASSERT( dist_till_block_end <= mem_heap.heap_size );
  JERRY_ASSERT( dist_till_block_end % MEM_HEAP_CHUNK_SIZE == 0 );

  return dist_till_block_end / MEM_HEAP_CHUNK_SIZE;
} /* mem_get_block_chunks_count */

/**
 * Calculate block's data space size
 *
 * @return size of block area that can be used to store data
 */
static size_t
mem_get_block_data_space_size( const mem_block_header_t *block_header_p) /**< block header */
{
  return mem_get_block_chunks_count( block_header_p) * MEM_HEAP_CHUNK_SIZE - sizeof (mem_block_header_t);
} /* mem_get_block_data_space_size */

/**
 * Calculate minimum chunks count needed for block with specified size of allocated data area.
 *
 * @return chunks count
 */
static size_t
mem_get_block_chunks_count_from_data_size( size_t block_allocated_size) /**< size of block's allocated area */
{
  return JERRY_ALIGNUP( sizeof (mem_block_header_t) + block_allocated_size, MEM_HEAP_CHUNK_SIZE) / MEM_HEAP_CHUNK_SIZE;
} /* mem_get_block_chunks_count_from_data_size */

/**
 * Startup initialization of heap
 */
void
mem_heap_init(uint8_t *heap_start, /**< first address of heap space */
             size_t heap_size)    /**< heap space size */
{
  JERRY_ASSERT( heap_start != NULL );
  JERRY_ASSERT( heap_size != 0 );
  JERRY_ASSERT( heap_size % MEM_HEAP_CHUNK_SIZE == 0 );
  JERRY_ASSERT( (uintptr_t) heap_start % MEM_ALIGNMENT == 0);

  mem_heap.heap_start = heap_start;
  mem_heap.heap_size = heap_size;

  VALGRIND_NOACCESS_SPACE( heap_start, heap_size);

  mem_init_block_header(mem_heap.heap_start,
                      0,
                      MEM_BLOCK_FREE,
                      NULL,
                      NULL);

  mem_heap.first_block_p = (mem_block_header_t*) mem_heap.heap_start;
  mem_heap.last_block_p = mem_heap.first_block_p;

  mem_heap_stat_init();
} /* mem_heap_init */

/**
 * Finalize heap
 */
void
mem_heap_finalize(void)
{
  VALGRIND_DEFINED_SPACE( mem_heap.heap_start, mem_heap.heap_size);

  JERRY_ASSERT( mem_heap.first_block_p == mem_heap.last_block_p );
  JERRY_ASSERT( mem_heap.first_block_p->magic_num == MEM_MAGIC_NUM_OF_FREE_BLOCK );

  VALGRIND_NOACCESS_SPACE( mem_heap.heap_start, mem_heap.heap_size);

  __memset( &mem_heap, 0, sizeof(mem_heap));
} /* mem_heap_finalize */

/**
 * Initialize block header
 */
static void
mem_init_block_header( uint8_t *first_chunk_p,         /**< address of the first chunk to use for the block */
                    size_t allocated_bytes,        /**< size of block's allocated area */
                    mem_block_state_t block_state,   /**< state of the block (allocated or free) */
                    mem_block_header_t *prev_block_p, /**< previous block */
                    mem_block_header_t *next_block_p) /**< next block */
{
  mem_block_header_t *block_header_p = (mem_block_header_t*) first_chunk_p;

  VALGRIND_UNDEFINED_STRUCT( block_header_p);

  if ( block_state == MEM_BLOCK_FREE )
  {
    block_header_p->magic_num = MEM_MAGIC_NUM_OF_FREE_BLOCK;

    JERRY_ASSERT( allocated_bytes == 0 );
  } else
  {
    block_header_p->magic_num = MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK;
  }

  block_header_p->neighbours[ MEM_DIRECTION_PREV ] = prev_block_p;
  block_header_p->neighbours[ MEM_DIRECTION_NEXT ] = next_block_p;
  block_header_p->allocated_bytes = allocated_bytes;

  JERRY_ASSERT( allocated_bytes <= mem_get_block_data_space_size( block_header_p) );

  VALGRIND_NOACCESS_STRUCT( block_header_p);
} /* mem_init_block_header */

/**
 * Allocation of memory region.
 *
 * To reduce heap fragmentation there are two allocation modes - short-term and long-term.
 *
 * If allocation is short-term then the beginning of the heap is preferred, else - the end of the heap.
 *
 * It is supposed, that all short-term allocation is used during relatively short discrete sessions.
 * After end of the session all short-term allocated regions are supposed to be freed.
 *
 * @return pointer to allocated memory block - if allocation is successful,\n
 *         NULL - if there is not enough memory.
 */
uint8_t*
mem_heap_alloc_block( size_t size_in_bytes,           /**< size of region to allocate in bytes */
                   mem_heap_alloc_term_t alloc_term) /**< expected allocation term */
{
  mem_block_header_t *block_p;
  mem_direction_t direction;

  mem_check_heap();

  if ( alloc_term == MEM_HEAP_ALLOC_SHORT_TERM )
  {
    block_p = mem_heap.first_block_p;
    direction = MEM_DIRECTION_NEXT;
  } else
  {
    block_p = mem_heap.last_block_p;
    direction = MEM_DIRECTION_PREV;
  }

  /* searching for appropriate block */
  while ( block_p != NULL )
  {
    VALGRIND_DEFINED_STRUCT( block_p);

    if ( block_p->magic_num == MEM_MAGIC_NUM_OF_FREE_BLOCK )
    {
      if ( mem_get_block_data_space_size( block_p) >= size_in_bytes )
      {
        break;
      }
    } else
    {
      JERRY_ASSERT( block_p->magic_num == MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK );
    }

    mem_block_header_t *next_block_p = block_p->neighbours[ direction ];

    VALGRIND_NOACCESS_STRUCT( block_p);

    block_p = next_block_p;
  }

  if ( block_p == NULL )
  {
    /* not enough free space */
    return NULL;
  }

  /* appropriate block found, allocating space */
  size_t new_block_size_in_chunks = mem_get_block_chunks_count_from_data_size( size_in_bytes);
  size_t found_block_size_in_chunks = mem_get_block_chunks_count( block_p);

  JERRY_ASSERT( new_block_size_in_chunks <= found_block_size_in_chunks );

  mem_block_header_t *prev_block_p = block_p->neighbours[ MEM_DIRECTION_PREV ];
  mem_block_header_t *next_block_p = block_p->neighbours[ MEM_DIRECTION_NEXT ];

  if ( new_block_size_in_chunks < found_block_size_in_chunks )
  {
    mem_heap_stat_free_block_split();

    uint8_t *new_free_block_first_chunk_p = (uint8_t*) block_p + new_block_size_in_chunks * MEM_HEAP_CHUNK_SIZE;
    mem_init_block_header(new_free_block_first_chunk_p,
                          0,
                          MEM_BLOCK_FREE,
                          block_p /* there we will place new allocated block */,
                          next_block_p);

    mem_block_header_t *new_free_block_p = (mem_block_header_t*) new_free_block_first_chunk_p;

    if ( next_block_p == NULL )
      {
        mem_heap.last_block_p = new_free_block_p;
      }
    else
      {
        next_block_p->neighbours[ MEM_DIRECTION_PREV ] = (mem_block_header_t*) new_free_block_first_chunk_p;
      }

    next_block_p = new_free_block_p;
  }

  mem_init_block_header((uint8_t*) block_p,
                      size_in_bytes,
                      MEM_BLOCK_ALLOCATED,
                      prev_block_p,
                      next_block_p);

  VALGRIND_DEFINED_STRUCT( block_p);

  mem_heap_stat_alloc_block( block_p);

  JERRY_ASSERT( mem_get_block_data_space_size( block_p) >= size_in_bytes );

  VALGRIND_NOACCESS_STRUCT( block_p);

  mem_check_heap();

  /* return data space beginning address */
  uint8_t *data_space_p = (uint8_t*) (block_p + 1);
  JERRY_ASSERT( (uintptr_t) data_space_p % MEM_ALIGNMENT == 0);

  VALGRIND_UNDEFINED_SPACE( data_space_p, size_in_bytes);

  return data_space_p;
} /* mem_heap_alloc_block */

/**
 * Free the memory block.
 */
void
mem_heap_free_block( uint8_t *ptr) /**< pointer to beginning of data space of the block */
{
  /* checking that ptr points to the heap */
  JERRY_ASSERT( ptr >= mem_heap.heap_start
                && ptr <= mem_heap.heap_start + mem_heap.heap_size );

  mem_check_heap();

  mem_block_header_t *block_p = (mem_block_header_t*) ptr - 1;

  VALGRIND_DEFINED_STRUCT( block_p);

  mem_block_header_t *prev_block_p = block_p->neighbours[ MEM_DIRECTION_PREV ];
  mem_block_header_t *next_block_p = block_p->neighbours[ MEM_DIRECTION_NEXT ];

  mem_heap_stat_free_block( block_p);

  VALGRIND_NOACCESS_SPACE( ptr, block_p->allocated_bytes);

  /* checking magic nums that are neighbour to data space */
  JERRY_ASSERT( block_p->magic_num == MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK );
  if ( next_block_p != NULL )
  {
    VALGRIND_DEFINED_STRUCT( next_block_p);

    JERRY_ASSERT( next_block_p->magic_num == MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK
                 || next_block_p->magic_num == MEM_MAGIC_NUM_OF_FREE_BLOCK );

    VALGRIND_NOACCESS_STRUCT( next_block_p);
  }

  block_p->magic_num = MEM_MAGIC_NUM_OF_FREE_BLOCK;

  if ( next_block_p != NULL )
    {
      VALGRIND_DEFINED_STRUCT( next_block_p);

      if (next_block_p->magic_num == MEM_MAGIC_NUM_OF_FREE_BLOCK )
        {
          /* merge with the next block */
          mem_heap_stat_free_block_merge();

          mem_block_header_t *next_next_block_p = next_block_p->neighbours[ MEM_DIRECTION_NEXT ];

          VALGRIND_NOACCESS_STRUCT( next_block_p);

          next_block_p = next_next_block_p;

          VALGRIND_DEFINED_STRUCT( next_block_p);

          block_p->neighbours[ MEM_DIRECTION_NEXT ] = next_block_p;
          if ( next_block_p != NULL )
            {
              next_block_p->neighbours[ MEM_DIRECTION_PREV ] = block_p;
            }
          else
            {
              mem_heap.last_block_p = block_p;
            }
        }

      VALGRIND_NOACCESS_STRUCT( next_block_p);
    }

  if ( prev_block_p != NULL )
    {
      VALGRIND_DEFINED_STRUCT( prev_block_p);

      if ( prev_block_p->magic_num == MEM_MAGIC_NUM_OF_FREE_BLOCK )
        {
          /* merge with the previous block */
          mem_heap_stat_free_block_merge();

          prev_block_p->neighbours[ MEM_DIRECTION_NEXT ] = next_block_p;
          if ( next_block_p != NULL )
            {
              VALGRIND_DEFINED_STRUCT( next_block_p);

              next_block_p->neighbours[ MEM_DIRECTION_PREV ] = block_p->neighbours[ MEM_DIRECTION_PREV ];

              VALGRIND_NOACCESS_STRUCT( next_block_p);
            }
          else
            {
              mem_heap.last_block_p = prev_block_p;
            }
        }

      VALGRIND_NOACCESS_STRUCT( prev_block_p);
    }

  VALGRIND_NOACCESS_STRUCT( block_p);

  mem_check_heap();
} /* mem_heap_free_block */

/**
 * Recommend allocation size based on chunk size.
 * 
 * @return recommended allocation size
 */
size_t
mem_heap_recommend_allocation_size( size_t minimum_allocation_size) /**< minimum allocation size */
{
  size_t minimum_allocation_size_with_block_header = minimum_allocation_size + sizeof (mem_block_header_t);
  size_t heap_chunk_aligned_allocation_size = JERRY_ALIGNUP( minimum_allocation_size_with_block_header, MEM_HEAP_CHUNK_SIZE);

  return heap_chunk_aligned_allocation_size - sizeof (mem_block_header_t);
} /* mem_heap_recommend_allocation_size */

/**
 * Print heap
 */
void
mem_heap_print( bool dump_block_headers, /**< print block headers */
                bool dump_block_data, /**< print block with data (true)
                                        or print only block header (false) */
                bool dump_stats) /**< print heap stats */
{
  mem_check_heap();

  JERRY_ASSERT( !dump_block_data || dump_block_headers );

  if ( dump_block_headers )
    {
      __printf("Heap: start=%p size=%lu, first block->%p, last block->%p\n",
               mem_heap.heap_start,
               mem_heap.heap_size,
               (void*) mem_heap.first_block_p,
               (void*) mem_heap.last_block_p);

      for ( mem_block_header_t *block_p = mem_heap.first_block_p, *next_block_p;
           block_p != NULL;
           block_p = next_block_p )
        {
          VALGRIND_DEFINED_STRUCT( block_p);

          __printf("Block (%p): magic num=0x%08x, size in chunks=%lu, previous block->%p next block->%p\n",
                   (void*) block_p,
                   block_p->magic_num,
                   mem_get_block_chunks_count( block_p),
                   (void*) block_p->neighbours[ MEM_DIRECTION_PREV ],
                   (void*) block_p->neighbours[ MEM_DIRECTION_NEXT ]);

          if ( dump_block_data )
            {
              uint8_t *block_data_p = (uint8_t*) (block_p + 1);
              for ( uint32_t offset = 0;
                   offset < mem_get_block_data_space_size( block_p);
                   offset++ )
                {
                  __printf("%02x ", block_data_p[ offset ]);
                }
              __printf("\n");
            }

          next_block_p = block_p->neighbours[ MEM_DIRECTION_NEXT ];

          VALGRIND_NOACCESS_STRUCT( block_p);
        }
    }

#ifdef MEM_STATS
  if ( dump_stats )
    {
      __printf("Heap stats:\n");
      __printf("  Heap size = %lu bytes\n"
               "  Chunk size = %lu bytes\n"
               "  Blocks count = %lu\n"
               "  Allocated blocks count = %lu\n"
               "  Allocated chunks count = %lu\n"
               "  Allocated = %lu bytes\n"
               "  Waste = %lu bytes\n"
               "  Peak allocated blocks count = %lu\n"
               "  Peak allocated chunks count = %lu\n"
               "  Peak allocated= %lu bytes\n"
               "  Peak waste = %lu bytes\n",
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
#endif /* MEM_STATS */

  __printf("\n");
} /* mem_heap_print */

/**
 * Check heap consistency
 */
static void
mem_check_heap( void)
{
#ifndef JERRY_NDEBUG
  JERRY_ASSERT( (uint8_t*) mem_heap.first_block_p == mem_heap.heap_start );
  JERRY_ASSERT( mem_heap.heap_size % MEM_HEAP_CHUNK_SIZE == 0 );

  bool is_last_block_was_met = false;
  size_t chunk_sizes_sum = 0;

  for ( mem_block_header_t *block_p = mem_heap.first_block_p, *next_block_p;
        block_p != NULL;
        block_p = next_block_p )
  {
    VALGRIND_DEFINED_STRUCT( block_p);

    JERRY_ASSERT( block_p->magic_num == MEM_MAGIC_NUM_OF_FREE_BLOCK
                 || block_p->magic_num == MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK );
    chunk_sizes_sum += mem_get_block_chunks_count( block_p);

    next_block_p = block_p->neighbours[ MEM_DIRECTION_NEXT ];

    if ( block_p == mem_heap.last_block_p )
    {
      is_last_block_was_met = true;

      JERRY_ASSERT( next_block_p == NULL );
    } else
    {
      JERRY_ASSERT( next_block_p != NULL );
    }

    VALGRIND_NOACCESS_STRUCT( block_p);
 }

  JERRY_ASSERT( chunk_sizes_sum * MEM_HEAP_CHUNK_SIZE == mem_heap.heap_size );
  JERRY_ASSERT( is_last_block_was_met );

  bool is_first_block_was_met = false;
  chunk_sizes_sum = 0;

  for ( mem_block_header_t *block_p = mem_heap.last_block_p, *prev_block_p;
        block_p != NULL;
        block_p = prev_block_p )
  {
    VALGRIND_DEFINED_STRUCT( block_p);

    JERRY_ASSERT( block_p->magic_num == MEM_MAGIC_NUM_OF_FREE_BLOCK
                 || block_p->magic_num == MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK );
    chunk_sizes_sum += mem_get_block_chunks_count( block_p);

    prev_block_p = block_p->neighbours[ MEM_DIRECTION_PREV ];

    if ( block_p == mem_heap.first_block_p )
    {
      is_first_block_was_met = true;

      JERRY_ASSERT( prev_block_p == NULL );
    } else
    {
      JERRY_ASSERT( prev_block_p != NULL );
    }

    VALGRIND_NOACCESS_STRUCT( block_p);
 }

  JERRY_ASSERT( chunk_sizes_sum * MEM_HEAP_CHUNK_SIZE == mem_heap.heap_size );
  JERRY_ASSERT( is_first_block_was_met );
#endif /* !JERRY_NDEBUG */
} /* mem_check_heap */

#ifdef MEM_STATS
/**
 * Get heap memory usage statistics
 */
void
mem_heap_get_stats( mem_heap_stats_t *out_heap_stats_p) /**< out: heap stats */
{
  *out_heap_stats_p = mem_heap_stats;
} /* mem_heap_get_stats */

/**
 * Initalize heap memory usage statistics account structure
 */
static void
mem_heap_stat_init()
{
  __memset( &mem_heap_stats, 0, sizeof (mem_heap_stats));

  mem_heap_stats.size = mem_heap.heap_size;
  mem_heap_stats.blocks = 1;
} /* mem_heap_stat_init */

/**
 * Account block allocation
 */
static void
mem_heap_stat_alloc_block( mem_block_header_t *block_header_p) /**< allocated block */
{
  JERRY_ASSERT( block_header_p->magic_num == MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK );

  const size_t chunks = mem_get_block_chunks_count( block_header_p);
  const size_t bytes = block_header_p->allocated_bytes;
  const size_t waste_bytes = chunks * MEM_HEAP_CHUNK_SIZE - bytes;

  mem_heap_stats.allocated_blocks++;
  mem_heap_stats.allocated_chunks += chunks;
  mem_heap_stats.allocated_bytes += bytes;
  mem_heap_stats.waste_bytes += waste_bytes;

  if ( mem_heap_stats.allocated_blocks > mem_heap_stats.peak_allocated_blocks )
  {
    mem_heap_stats.peak_allocated_blocks = mem_heap_stats.allocated_blocks;
  }

  if ( mem_heap_stats.allocated_chunks > mem_heap_stats.peak_allocated_chunks )
  {
    mem_heap_stats.peak_allocated_chunks = mem_heap_stats.allocated_chunks;
  }

  if ( mem_heap_stats.allocated_bytes > mem_heap_stats.peak_allocated_bytes )
  {
    mem_heap_stats.peak_allocated_bytes = mem_heap_stats.allocated_bytes;
  }

  if ( mem_heap_stats.waste_bytes > mem_heap_stats.peak_waste_bytes )
  {
    mem_heap_stats.peak_waste_bytes = mem_heap_stats.waste_bytes;
  }

  JERRY_ASSERT( mem_heap_stats.allocated_blocks <= mem_heap_stats.blocks );
  JERRY_ASSERT( mem_heap_stats.allocated_bytes <= mem_heap_stats.size );
  JERRY_ASSERT( mem_heap_stats.allocated_chunks <= mem_heap_stats.size / MEM_HEAP_CHUNK_SIZE );
} /* mem_heap_stat_alloc_block */

/**
 * Account block freeing
 */
static void
mem_heap_stat_free_block( mem_block_header_t *block_header_p) /**< block to be freed */
{
  JERRY_ASSERT( block_header_p->magic_num == MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK );

  const size_t chunks = mem_get_block_chunks_count( block_header_p);
  const size_t bytes = block_header_p->allocated_bytes;
  const size_t waste_bytes = chunks * MEM_HEAP_CHUNK_SIZE - bytes;

  JERRY_ASSERT( mem_heap_stats.allocated_blocks <= mem_heap_stats.blocks );
  JERRY_ASSERT( mem_heap_stats.allocated_bytes <= mem_heap_stats.size );
  JERRY_ASSERT( mem_heap_stats.allocated_chunks <= mem_heap_stats.size / MEM_HEAP_CHUNK_SIZE );

  JERRY_ASSERT( mem_heap_stats.allocated_blocks >= 1 );
  JERRY_ASSERT( mem_heap_stats.allocated_chunks >= chunks );
  JERRY_ASSERT( mem_heap_stats.allocated_bytes >= bytes );
  JERRY_ASSERT( mem_heap_stats.waste_bytes >= waste_bytes );

  mem_heap_stats.allocated_blocks--;
  mem_heap_stats.allocated_chunks -= chunks;
  mem_heap_stats.allocated_bytes -= bytes;
  mem_heap_stats.waste_bytes -= waste_bytes;
} /* mem_heap_stat_free_block */

/**
 * Account free block split
 */
static void
mem_heap_stat_free_block_split( void)
{
  mem_heap_stats.blocks++;
} /* mem_heap_stat_free_block_split */

/**
 * Account free block merge
 */
static void
mem_heap_stat_free_block_merge( void)
{
  mem_heap_stats.blocks--;
} /* mem_heap_stat_free_block_merge */
#endif /* MEM_STATS */

/**
 * @}
 * @}
 */
