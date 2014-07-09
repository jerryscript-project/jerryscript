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
  mem_MagicNumOfBlock_t m_MagicNum;                                    /**< magic number - MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK for allocated block
                                                                         and MEM_MAGIC_NUM_OF_FREE_BLOCK for free block */
  struct mem_BlockHeader_t *m_Neighbours[ MEM_DIRECTION_COUNT ]; /**< neighbour blocks */
  size_t m_SizeInChunks;                                               /**< size of block with header in chunks */
} mem_BlockHeader_t;

/**
 * Calculate size in bytes of the block space, that can be used to store data
 */
#define mem_GetHeapBlockDataSpaceSizeInBytes( pBlockHeader) ( MEM_HEAP_CHUNK_SIZE * pBlockHeader->m_SizeInChunks - sizeof(mem_BlockHeader_t) )

/**
 * Calculate size in chunks of heap block from data space size in bytes
 */
#define mem_GetHeapBlockSizeInChunksFromDataSpaceSizeInBytes( size) ( ( sizeof(mem_BlockHeader_t) + size + MEM_HEAP_CHUNK_SIZE - 1 ) / MEM_HEAP_CHUNK_SIZE )

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
  uint8_t* m_HeapStart;                   /**< first address of heap space */
  size_t m_HeapSize;                      /**< heap space size */
  mem_BlockHeader_t* m_pFirstBlock; /**< first block of the heap */
  mem_BlockHeader_t* m_pLastBlock;  /**< last block of the heap */
} mem_HeapState_t;

/**
 * Heap state
 */
mem_HeapState_t mem_Heap;

static void mem_InitBlockHeader( uint8_t *pFirstChunk,
                                size_t sizeInChunks,
                                mem_BlockState_t blockState,
                                mem_BlockHeader_t *pPrevBlock,
                                mem_BlockHeader_t *pNextBlock);
static void mem_CheckHeap( void);

/**
 * Startup initialization of heap
 */
void
mem_HeapInit( uint8_t *heapStart, /**< first address of heap space */
             size_t heapSize)     /**< heap space size */
{
  JERRY_ASSERT( heapStart != NULL );
  JERRY_ASSERT( heapSize != 0 );
  JERRY_ASSERT( heapSize % MEM_HEAP_CHUNK_SIZE == 0 );
  JERRY_ASSERT( (uintptr_t) heapStart % MEM_ALIGNMENT == 0);

  mem_Heap.m_HeapStart = heapStart;
  mem_Heap.m_HeapSize = heapSize;

  mem_InitBlockHeader( mem_Heap.m_HeapStart,
                      heapSize / MEM_HEAP_CHUNK_SIZE,
                      MEM_BLOCK_FREE,
                      NULL,
                      NULL);

  mem_Heap.m_pFirstBlock = (mem_BlockHeader_t*) mem_Heap.m_HeapStart;
  mem_Heap.m_pLastBlock = mem_Heap.m_pFirstBlock;
} /* mem_HeapInit */

/**
 * Initialize block header
 */
static void
mem_InitBlockHeader( uint8_t *pFirstChunk,               /**< address of the first chunk to use for the block */
                    size_t sizeInChunks,                 /**< size of block with header in chunks */
                    mem_BlockState_t blockState,   /**< state of the block (allocated or free) */
                    mem_BlockHeader_t *pPrevBlock, /**< previous block */
                    mem_BlockHeader_t *pNextBlock) /**< next block */
{
  mem_BlockHeader_t *pBlockHeader = (mem_BlockHeader_t*) pFirstChunk;

  if ( blockState == MEM_BLOCK_FREE )
    {
      pBlockHeader->m_MagicNum = MEM_MAGIC_NUM_OF_FREE_BLOCK;
    } else
      {
        pBlockHeader->m_MagicNum = MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK;
      }

    pBlockHeader->m_Neighbours[ MEM_DIRECTION_PREV ] = pPrevBlock;
    pBlockHeader->m_Neighbours[ MEM_DIRECTION_NEXT ] = pNextBlock;
    pBlockHeader->m_SizeInChunks = sizeInChunks;
} /* mem_InitFreeBlock */

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
mem_HeapAllocBlock( size_t sizeInBytes,              /**< size of region to allocate in bytes */
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
            if ( mem_GetHeapBlockDataSpaceSizeInBytes( pBlock) >= sizeInBytes )
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
    size_t newBlockSizeInChunks = mem_GetHeapBlockSizeInChunksFromDataSpaceSizeInBytes( sizeInBytes);
    size_t foundBlockSizeInChunks = pBlock->m_SizeInChunks;

    JERRY_ASSERT( newBlockSizeInChunks <= foundBlockSizeInChunks );

    mem_BlockHeader_t *pPrevBlock = pBlock->m_Neighbours[ MEM_DIRECTION_PREV ];
    mem_BlockHeader_t *pNextBlock = pBlock->m_Neighbours[ MEM_DIRECTION_NEXT ];

    if ( newBlockSizeInChunks < foundBlockSizeInChunks )
      {
        uint8_t *pNewFreeBlockFirstChunk = (uint8_t*) pBlock + newBlockSizeInChunks * MEM_HEAP_CHUNK_SIZE;
        mem_InitBlockHeader( pNewFreeBlockFirstChunk,
                            foundBlockSizeInChunks - newBlockSizeInChunks,
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

    mem_InitBlockHeader( (uint8_t*) pBlock,
                        newBlockSizeInChunks,
                        MEM_BLOCK_ALLOCATED,
                        pPrevBlock,
                        pNextBlock);

    JERRY_ASSERT( mem_GetHeapBlockDataSpaceSizeInBytes( pBlock) >= sizeInBytes );

    mem_CheckHeap();

    /* return data space beginning address */
    uint8_t *pDataSpace = (uint8_t*) (pBlock + 1);
    JERRY_ASSERT( (uintptr_t) pDataSpace % MEM_ALIGNMENT == 0);

    return pDataSpace;
} /* mem_Alloc */

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
      pBlock->m_SizeInChunks += pNextBlock->m_SizeInChunks;
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
      pPrevBlock->m_SizeInChunks += pBlock->m_SizeInChunks;
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
} /* mem_Free */

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
                  pBlock->m_SizeInChunks,
                  (void*) pBlock->m_Neighbours[ MEM_DIRECTION_PREV ],
                  (void*) pBlock->m_Neighbours[ MEM_DIRECTION_NEXT ]);

      if ( dumpBlockData )
        {
          uint8_t *pBlockData = (uint8_t*) (pBlock + 1);
          for ( uint32_t offset = 0;
               offset < mem_GetHeapBlockDataSpaceSizeInBytes( pBlock);
               offset++ )
            {
              __printf("%02x ", pBlockData[ offset ]);
            }
          __printf("\n");
        }
    }

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

  size_t chunksCount = 0;
  bool isLastBlockWasMet = false;
  for ( mem_BlockHeader_t *pBlock = mem_Heap.m_pFirstBlock;
       pBlock != NULL;
       pBlock = pBlock->m_Neighbours[ MEM_DIRECTION_NEXT ] )
    {
      JERRY_ASSERT( pBlock != NULL );
      JERRY_ASSERT( pBlock->m_MagicNum == MEM_MAGIC_NUM_OF_FREE_BLOCK
                   || pBlock->m_MagicNum == MEM_MAGIC_NUM_OF_ALLOCATED_BLOCK );

      chunksCount += pBlock->m_SizeInChunks;

      mem_BlockHeader_t *pNextBlock = pBlock->m_Neighbours[ MEM_DIRECTION_NEXT ];
      if ( pBlock == mem_Heap.m_pLastBlock )
        {
          isLastBlockWasMet = true;

          JERRY_ASSERT( pNextBlock == NULL );
          JERRY_ASSERT( mem_Heap.m_HeapStart + mem_Heap.m_HeapSize
                       == (uint8_t*) pBlock + pBlock->m_SizeInChunks * MEM_HEAP_CHUNK_SIZE );
        } else
          {
            JERRY_ASSERT( pNextBlock != NULL );
            JERRY_ASSERT( (uint8_t*) pNextBlock == (uint8_t*) pBlock + pBlock->m_SizeInChunks * MEM_HEAP_CHUNK_SIZE );
          }
    }

  JERRY_ASSERT( isLastBlockWasMet );
  JERRY_ASSERT( chunksCount == mem_Heap.m_HeapSize / MEM_HEAP_CHUNK_SIZE );
#endif /* !JERRY_NDEBUG */
} /* mem_CheckHeap */

/**
 * @}
 * @}
 */
