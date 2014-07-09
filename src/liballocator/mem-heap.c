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

/**
 * Magic numbers for heap memory blocks
 */
typedef enum
{
  MEM_MAGIC_NUM_OF_FREE_BLOCK      = 0x31d7c809,
  MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK = 0x59d75b46
} mem_MagicNumOfBlock_t;

/**
 * State of the block to initialize (argument of mem_InitBlockHeader)
 *
 * @see mem_InitBlockHeader
 */
typedef enum
{
  MEM_BLOCK_FREE,     /**< initializing free block */
  MEM_BLOCK_ALLOCATED /**< initializing allocated block */
} mem_BlockState_t;

/**
 * Linked list direction descriptors
 */
typedef enum
{
  MEM_DIRECTION_PREV = 0,  /**< direction from right to left */
  MEM_DIRECTION_NEXT = 1,  /**< direction from left to right */
  MEM_DIRECTION_COUNT = 2  /**< count of possible directions */
} mem_Direction_t;

/**
 * Description of heap memory block layout
 */
typedef struct mem_BlockHeader_t
{
  mem_MagicNumOfBlock_t m_MagicNum;                              /**< magic number - MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK for allocated block
                                                                      and MEM_MAGIC_NUM_OF_FREE_BLOCK for free block */
  struct mem_BlockHeader_t *m_Neighbours[ MEM_DIRECTION_COUNT ]; /**< neighbour blocks */
  size_t allocated_bytes;                                        /**< allocated area size - for allocated blocks; 0 - for free blocks */
} mem_BlockHeader_t;

/**
 * Chunk should have enough space for block header
 */
JERRY_STATIC_ASSERT( MEM_HEAP_CHUNK_SIZE >= sizeof (mem_BlockHeader_t) );

/**
 * Chunk size should satisfy the required alignment value
 */
JERRY_STATIC_ASSERT( MEM_HEAP_CHUNK_SIZE % MEM_ALIGNMENT == 0 );

/**
 * Description of heap state
 */
typedef struct
{
  uint8_t* m_HeapStart;             /**< first address of heap space */
  size_t m_HeapSize;                /**< heap space size */
  mem_BlockHeader_t* m_pFirstBlock; /**< first block of the heap */
  mem_BlockHeader_t* m_pLastBlock;  /**< last block of the heap */
} mem_HeapState_t;

/**
 * Heap state
 */
mem_HeapState_t mem_Heap;

static size_t mem_get_block_chunks_count( const mem_BlockHeader_t *block_header_p);
static size_t mem_get_block_data_space_size( const mem_BlockHeader_t *block_header_p);
static size_t mem_get_block_chunks_count_from_data_size( size_t block_allocated_size);

static void mem_InitBlockHeader( uint8_t *pFirstChunk,
                                size_t sizeInChunks,
                                mem_BlockState_t blockState,
                                mem_BlockHeader_t *pPrevBlock,
                                mem_BlockHeader_t *pNextBlock);
static void mem_CheckHeap( void);

#ifdef MEM_STATS
/**
 * Heap's memory usage statistics
 */
static mem_HeapStats_t mem_HeapStats;

static void mem_HeapStatInit( void);
static void mem_HeapStatAllocBlock( mem_BlockHeader_t *block_header_p);
static void mem_HeapStatFreeBlock( mem_BlockHeader_t *block_header_p);
static void mem_HeapStatFreeBlockSplit( void);
static void mem_HeapStatFreeBlockMerge( void);
#else /* !MEM_STATS */
#  define mem_HeapStatInit()
#  define mem_HeapStatAllocBlock( v)
#  define mem_HeapStatFreeBlock( v)
#  define mem_HeapStatFreeBlockSplit()
#  define mem_HeapStatFreeBlockMerge()
#endif /* !MEM_STATS */

/**
 * get chunk count, used by the block.
 *
 * @return chunks count
 */
static size_t
mem_get_block_chunks_count( const mem_BlockHeader_t *block_header_p) /**< block header */
{
  JERRY_ASSERT( block_header_p != NULL );

  const mem_BlockHeader_t *next_block_p = block_header_p->m_Neighbours[ MEM_DIRECTION_NEXT ];
  size_t dist_till_block_end;

  if ( next_block_p == NULL )
  {
    dist_till_block_end = (size_t) ( mem_Heap.m_HeapStart + mem_Heap.m_HeapSize - (uint8_t*) block_header_p );
  } else
  {
    dist_till_block_end = (size_t) ( (uint8_t*) next_block_p - (uint8_t*) block_header_p );
  }

  JERRY_ASSERT( dist_till_block_end <= mem_Heap.m_HeapSize );
  JERRY_ASSERT( dist_till_block_end % MEM_HEAP_CHUNK_SIZE == 0 );

  return dist_till_block_end / MEM_HEAP_CHUNK_SIZE;
} /* mem_get_block_chunks_count */

/**
 * Calculate block's data space size
 *
 * @return size of block area that can be used to store data
 */
static size_t
mem_get_block_data_space_size( const mem_BlockHeader_t *block_header_p) /**< block header */
{
  return mem_get_block_chunks_count( block_header_p) * MEM_HEAP_CHUNK_SIZE - sizeof (mem_BlockHeader_t);
} /* mem_get_block_data_space_size */

/**
 * Calculate minimum chunks count needed for block with specified size of allocated data area.
 *
 * @return chunks count
 */
static size_t
mem_get_block_chunks_count_from_data_size( size_t block_allocated_size) /**< size of block's allocated area */
{
  return JERRY_ALIGNUP( sizeof (mem_BlockHeader_t) + block_allocated_size, MEM_HEAP_CHUNK_SIZE) / MEM_HEAP_CHUNK_SIZE;
} /* mem_get_block_chunks_count_from_data_size */

/**
 * Startup initialization of heap
 */
void
mem_HeapInit(uint8_t *heapStart, /**< first address of heap space */
             size_t heapSize)    /**< heap space size */
{
  JERRY_ASSERT( heapStart != NULL );
  JERRY_ASSERT( heapSize != 0 );
  JERRY_ASSERT( heapSize % MEM_HEAP_CHUNK_SIZE == 0 );
  JERRY_ASSERT( (uintptr_t) heapStart % MEM_ALIGNMENT == 0);

  mem_Heap.m_HeapStart = heapStart;
  mem_Heap.m_HeapSize = heapSize;

  mem_InitBlockHeader(mem_Heap.m_HeapStart,
                      0,
                      MEM_BLOCK_FREE,
                      NULL,
                      NULL);

  mem_Heap.m_pFirstBlock = (mem_BlockHeader_t*) mem_Heap.m_HeapStart;
  mem_Heap.m_pLastBlock = mem_Heap.m_pFirstBlock;

  mem_HeapStatInit();
} /* mem_HeapInit */

/**
 * Initialize block header
 */
static void
mem_InitBlockHeader( uint8_t *pFirstChunk,         /**< address of the first chunk to use for the block */
                    size_t allocated_bytes,        /**< size of block's allocated area */
                    mem_BlockState_t blockState,   /**< state of the block (allocated or free) */
                    mem_BlockHeader_t *pPrevBlock, /**< previous block */
                    mem_BlockHeader_t *pNextBlock) /**< next block */
{
  mem_BlockHeader_t *pBlockHeader = (mem_BlockHeader_t*) pFirstChunk;

  if ( blockState == MEM_BLOCK_FREE )
  {
    pBlockHeader->m_MagicNum = MEM_MAGIC_NUM_OF_FREE_BLOCK;

    JERRY_ASSERT( allocated_bytes == 0 );
  } else
  {
    pBlockHeader->m_MagicNum = MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK;
  }

  pBlockHeader->m_Neighbours[ MEM_DIRECTION_PREV ] = pPrevBlock;
  pBlockHeader->m_Neighbours[ MEM_DIRECTION_NEXT ] = pNextBlock;
  pBlockHeader->allocated_bytes = allocated_bytes;

  JERRY_ASSERT( allocated_bytes <= mem_get_block_data_space_size( pBlockHeader) );
} /* mem_InitBlockHeader */

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
mem_HeapAllocBlock( size_t sizeInBytes,           /**< size of region to allocate in bytes */
                   mem_HeapAllocTerm_t allocTerm) /**< expected allocation term */
{
  mem_BlockHeader_t *pBlock;
  mem_Direction_t direction;

  mem_CheckHeap();

  if ( allocTerm == MEM_HEAP_ALLOC_SHORT_TERM )
  {
    pBlock = mem_Heap.m_pFirstBlock;
    direction = MEM_DIRECTION_NEXT;
  } else
  {
    pBlock = mem_Heap.m_pLastBlock;
    direction = MEM_DIRECTION_PREV;
  }

  /* searching for appropriate block */
  while ( pBlock != NULL )
  {
    if ( pBlock->m_MagicNum == MEM_MAGIC_NUM_OF_FREE_BLOCK )
    {
      if ( mem_get_block_data_space_size( pBlock) >= sizeInBytes )
      {
        break;
      }
    } else
    {
      JERRY_ASSERT( pBlock->m_MagicNum == MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK );
    }

    pBlock = pBlock->m_Neighbours[ direction ];
  }

  if ( pBlock == NULL )
  {
    /* not enough free space */
    return NULL;
  }

  /* appropriate block found, allocating space */
  size_t newBlockSizeInChunks = mem_get_block_chunks_count_from_data_size( sizeInBytes);
  size_t foundBlockSizeInChunks = mem_get_block_chunks_count( pBlock);

  JERRY_ASSERT( newBlockSizeInChunks <= foundBlockSizeInChunks );

  mem_BlockHeader_t *pPrevBlock = pBlock->m_Neighbours[ MEM_DIRECTION_PREV ];
  mem_BlockHeader_t *pNextBlock = pBlock->m_Neighbours[ MEM_DIRECTION_NEXT ];

  if ( newBlockSizeInChunks < foundBlockSizeInChunks )
  {
    mem_HeapStatFreeBlockSplit();

    uint8_t *pNewFreeBlockFirstChunk = (uint8_t*) pBlock + newBlockSizeInChunks * MEM_HEAP_CHUNK_SIZE;
    mem_InitBlockHeader(pNewFreeBlockFirstChunk,
                        0,
                        MEM_BLOCK_FREE,
                        pBlock /* there we will place new allocated block */,
                        pNextBlock);

    mem_BlockHeader_t *pNewFreeBlock = (mem_BlockHeader_t*) pNewFreeBlockFirstChunk;

    if ( pNextBlock == NULL )
    {
      mem_Heap.m_pLastBlock = pNewFreeBlock;
    }

    pNextBlock = pNewFreeBlock;
  }

  mem_InitBlockHeader((uint8_t*) pBlock,
                      sizeInBytes,
                      MEM_BLOCK_ALLOCATED,
                      pPrevBlock,
                      pNextBlock);

  mem_HeapStatAllocBlock( pBlock);

  JERRY_ASSERT( mem_get_block_data_space_size( pBlock) >= sizeInBytes );

  mem_CheckHeap();

  /* return data space beginning address */
  uint8_t *pDataSpace = (uint8_t*) (pBlock + 1);
  JERRY_ASSERT( (uintptr_t) pDataSpace % MEM_ALIGNMENT == 0);

  return pDataSpace;
} /* mem_HeapAllocBlock */

/**
 * Free the memory block.
 */
void
mem_HeapFreeBlock( uint8_t *ptr) /**< pointer to beginning of data space of the block */
{
  /* checking that ptr points to the heap */
  JERRY_ASSERT( ptr >= mem_Heap.m_HeapStart
               && ptr <= mem_Heap.m_HeapStart + mem_Heap.m_HeapSize );

  mem_CheckHeap();

  mem_BlockHeader_t *pBlock = (mem_BlockHeader_t*) ptr - 1;
  mem_BlockHeader_t *pPrevBlock = pBlock->m_Neighbours[ MEM_DIRECTION_PREV ];
  mem_BlockHeader_t *pNextBlock = pBlock->m_Neighbours[ MEM_DIRECTION_NEXT ];

  mem_HeapStatFreeBlock( pBlock);

  /* checking magic nums that are neighbour to data space */
  JERRY_ASSERT( pBlock->m_MagicNum == MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK );
  if ( pNextBlock != NULL )
  {
    JERRY_ASSERT( pNextBlock->m_MagicNum == MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK
                 || pNextBlock->m_MagicNum == MEM_MAGIC_NUM_OF_FREE_BLOCK );
  }

  pBlock->m_MagicNum = MEM_MAGIC_NUM_OF_FREE_BLOCK;

  if ( pNextBlock != NULL
       && pNextBlock->m_MagicNum == MEM_MAGIC_NUM_OF_FREE_BLOCK )
  {
    /* merge with the next block */
    mem_HeapStatFreeBlockMerge();

    pNextBlock = pNextBlock->m_Neighbours[ MEM_DIRECTION_NEXT ];
    pBlock->m_Neighbours[ MEM_DIRECTION_NEXT ] = pNextBlock;
    if ( pNextBlock != NULL )
    {
      pNextBlock->m_Neighbours[ MEM_DIRECTION_PREV ] = pBlock;
    } else
    {
      mem_Heap.m_pLastBlock = pBlock;
    }
  }

  if ( pPrevBlock != NULL
       && pPrevBlock->m_MagicNum == MEM_MAGIC_NUM_OF_FREE_BLOCK )
  {
    /* merge with the previous block */
    mem_HeapStatFreeBlockMerge();

    pPrevBlock->m_Neighbours[ MEM_DIRECTION_NEXT ] = pNextBlock;
    if ( pNextBlock != NULL )
    {
      pNextBlock->m_Neighbours[ MEM_DIRECTION_PREV ] = pBlock->m_Neighbours[ MEM_DIRECTION_PREV ];
    } else
    {
      mem_Heap.m_pLastBlock = pPrevBlock;
    }
  }

  mem_CheckHeap();
} /* mem_HeapFreeBlock */

/**
 * Recommend allocation size based on chunk size.
 * 
 * @return recommended allocation size
 */
size_t
mem_HeapRecommendAllocationSize( size_t minimumAllocationSize) /**< minimum allocation size */
{
  size_t minimumAllocationSizeWithBlockHeader = minimumAllocationSize + sizeof (mem_BlockHeader_t);
  size_t heapChunkAlignedAllocationSize = JERRY_ALIGNUP( minimumAllocationSizeWithBlockHeader, MEM_HEAP_CHUNK_SIZE);

  return heapChunkAlignedAllocationSize - sizeof (mem_BlockHeader_t);
} /* mem_HeapRecommendAllocationSize */

/**
 * Print heap
 */
void
mem_HeapPrint( bool dumpBlockData) /**< print block with data (true)
                                     or print only block header (false) */
{
  mem_CheckHeap();

  __printf("Heap: start=%p size=%lu, first block->%p, last block->%p\n",
              mem_Heap.m_HeapStart,
              mem_Heap.m_HeapSize,
              (void*) mem_Heap.m_pFirstBlock,
              (void*) mem_Heap.m_pLastBlock);

  for ( mem_BlockHeader_t *pBlock = mem_Heap.m_pFirstBlock;
        pBlock != NULL;
        pBlock = pBlock->m_Neighbours[ MEM_DIRECTION_NEXT ] )
  {
    __printf("Block (%p): magic num=0x%08x, size in chunks=%lu, previous block->%p next block->%p\n",
                (void*) pBlock,
                pBlock->m_MagicNum,
                mem_get_block_chunks_count( pBlock),
                (void*) pBlock->m_Neighbours[ MEM_DIRECTION_PREV ],
                (void*) pBlock->m_Neighbours[ MEM_DIRECTION_NEXT ]);

    if ( dumpBlockData )
    {
      uint8_t *pBlockData = (uint8_t*) (pBlock + 1);
      for ( uint32_t offset = 0;
            offset < mem_get_block_data_space_size( pBlock);
            offset++ )
      {
        __printf("%02x ", pBlockData[ offset ]);
      }
      __printf("\n");
    }
  }

#ifdef MEM_STATS
  __printf("Heap stats:\n");
  __printf("  Size = %lu\n"
              "  Blocks count = %lu\n"
              "  Allocated blocks count = %lu\n"
              "  Allocated chunks count = %lu\n"
              "  Allocated bytes count = %lu\n"
              "  Waste bytes count = %lu\n"
              "  Peak allocated blocks count = %lu\n"
              "  Peak allocated chunks count = %lu\n"
              "  Peak allocated bytes count = %lu\n"
              "  Peak waste bytes count = %lu\n",
              mem_HeapStats.size,
              mem_HeapStats.blocks,
              mem_HeapStats.allocated_blocks,
              mem_HeapStats.allocated_chunks,
              mem_HeapStats.allocated_bytes,
              mem_HeapStats.waste_bytes,
              mem_HeapStats.peak_allocated_blocks,
              mem_HeapStats.peak_allocated_chunks,
              mem_HeapStats.peak_allocated_bytes,
              mem_HeapStats.peak_waste_bytes);
#endif /* MEM_STATS */

  __printf("\n");
} /* mem_PrintHeap */

/**
 * Check heap consistency
 */
static void
mem_CheckHeap( void)
{
#ifndef JERRY_NDEBUG
  JERRY_ASSERT( (uint8_t*) mem_Heap.m_pFirstBlock == mem_Heap.m_HeapStart );
  JERRY_ASSERT( mem_Heap.m_HeapSize % MEM_HEAP_CHUNK_SIZE == 0 );

  bool isLastBlockWasMet = false;
  for ( mem_BlockHeader_t *pBlock = mem_Heap.m_pFirstBlock;
        pBlock != NULL;
        pBlock = pBlock->m_Neighbours[ MEM_DIRECTION_NEXT ] )
  {
    JERRY_ASSERT( pBlock != NULL );
    JERRY_ASSERT( pBlock->m_MagicNum == MEM_MAGIC_NUM_OF_FREE_BLOCK
                 || pBlock->m_MagicNum == MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK );

    mem_BlockHeader_t *pNextBlock = pBlock->m_Neighbours[ MEM_DIRECTION_NEXT ];
    if ( pBlock == mem_Heap.m_pLastBlock )
    {
      isLastBlockWasMet = true;

      JERRY_ASSERT( pNextBlock == NULL );
    } else
    {
      JERRY_ASSERT( pNextBlock != NULL );
    }
  }

  JERRY_ASSERT( isLastBlockWasMet );
#endif /* !JERRY_NDEBUG */
} /* mem_CheckHeap */

#ifdef MEM_STATS
/**
 * Get heap memory usage statistics
 */
void
mem_HeapGetStats( mem_HeapStats_t *out_heap_stats_p) /**< out: heap stats */
{
  *out_heap_stats_p = mem_HeapStats;
} /* mem_HeapGetStats */

/**
 * Initalize heap memory usage statistics account structure
 */
static void
mem_HeapStatInit()
{
  __memset( &mem_HeapStats, 0, sizeof (mem_HeapStats));

  mem_HeapStats.size = mem_Heap.m_HeapSize;
  mem_HeapStats.blocks = 1;
} /* mem_InitStats */

/**
 * Account block allocation
 */
static void
mem_HeapStatAllocBlock( mem_BlockHeader_t *block_header_p) /**< allocated block */
{
  JERRY_ASSERT( block_header_p->m_MagicNum == MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK );

  const size_t chunks = mem_get_block_chunks_count( block_header_p);
  const size_t bytes = block_header_p->allocated_bytes;
  const size_t waste_bytes = chunks * MEM_HEAP_CHUNK_SIZE - bytes;

  mem_HeapStats.allocated_blocks++;
  mem_HeapStats.allocated_chunks += chunks;
  mem_HeapStats.allocated_bytes += bytes;
  mem_HeapStats.waste_bytes += waste_bytes;

  if ( mem_HeapStats.allocated_blocks > mem_HeapStats.peak_allocated_blocks )
  {
    mem_HeapStats.peak_allocated_blocks = mem_HeapStats.allocated_blocks;
  }

  if ( mem_HeapStats.allocated_chunks > mem_HeapStats.peak_allocated_chunks )
  {
    mem_HeapStats.peak_allocated_chunks = mem_HeapStats.allocated_chunks;
  }

  if ( mem_HeapStats.allocated_bytes > mem_HeapStats.peak_allocated_bytes )
  {
    mem_HeapStats.peak_allocated_bytes = mem_HeapStats.allocated_bytes;
  }

  if ( mem_HeapStats.waste_bytes > mem_HeapStats.peak_waste_bytes )
  {
    mem_HeapStats.peak_waste_bytes = mem_HeapStats.waste_bytes;
  }

  JERRY_ASSERT( mem_HeapStats.allocated_blocks <= mem_HeapStats.blocks );
  JERRY_ASSERT( mem_HeapStats.allocated_bytes <= mem_HeapStats.size );
  JERRY_ASSERT( mem_HeapStats.allocated_chunks <= mem_HeapStats.size / MEM_HEAP_CHUNK_SIZE );
} /* mem_HeapStatAllocBlock */

/**
 * Account block freeing
 */
static void
mem_HeapStatFreeBlock( mem_BlockHeader_t *block_header_p) /**< block to be freed */
{
  JERRY_ASSERT( block_header_p->m_MagicNum == MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK );

  const size_t chunks = mem_get_block_chunks_count( block_header_p);
  const size_t bytes = block_header_p->allocated_bytes;
  const size_t waste_bytes = chunks * MEM_HEAP_CHUNK_SIZE - bytes;

  JERRY_ASSERT( mem_HeapStats.allocated_blocks <= mem_HeapStats.blocks );
  JERRY_ASSERT( mem_HeapStats.allocated_bytes <= mem_HeapStats.size );
  JERRY_ASSERT( mem_HeapStats.allocated_chunks <= mem_HeapStats.size / MEM_HEAP_CHUNK_SIZE );

  JERRY_ASSERT( mem_HeapStats.allocated_blocks >= 1 );
  JERRY_ASSERT( mem_HeapStats.allocated_chunks >= chunks );
  JERRY_ASSERT( mem_HeapStats.allocated_bytes >= bytes );
  JERRY_ASSERT( mem_HeapStats.waste_bytes >= waste_bytes );

  mem_HeapStats.allocated_blocks--;
  mem_HeapStats.allocated_chunks -= chunks;
  mem_HeapStats.allocated_bytes -= bytes;
  mem_HeapStats.waste_bytes -= waste_bytes;
} /* mem_HeapStatFreeBlock */

/**
 * Account free block split
 */
static void
mem_HeapStatFreeBlockSplit( void)
{
  mem_HeapStats.blocks++;
} /* mem_HeapStatFreeBlockSplit */

/**
 * Account free block merge
 */
static void
mem_HeapStatFreeBlockMerge( void)
{
  mem_HeapStats.blocks--;
} /* mem_HeapStatFreeBlockMerge */
#endif /* MEM_STATS */

/**
 * @}
 * @}
 */
