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

#include "globals.h"
#include "mem-allocator.h"

extern void srand (unsigned int __seed);
extern int rand (void);
extern long int time (long int *__timer);
extern int printf (__const char *__restrict __format, ...);
extern void *memset (void *__s, int __c, size_t __n);

// Heap size is 8K
const size_t test_heap_size = 8 * 1024;

// Iterations count
const uint32_t test_iters = 1024 * 1024;

// Subiterations count
const uint32_t test_sub_iters = 3;

// Threshold size of block to allocate
const uint32_t test_threshold_block_size = 2048;

int
main( int __unused argc,
      char __unused **argv)
{
    uint8_t test_native_heap[test_heap_size];

    mem_heap_init( test_native_heap, sizeof (test_native_heap));

    srand((unsigned int) time(NULL));
    int k = rand();
    printf("seed=%d\n", k);
    srand((unsigned int) k);

    mem_heap_print( false, false, true);

    for ( uint32_t i = 0; i < test_iters; i++ )
    {
        const uint32_t subiters = test_sub_iters;
        uint8_t * ptrs[subiters];
        size_t sizes[subiters];

        for ( uint32_t j = 0; j < subiters; j++ )
        {
            size_t size = (unsigned int) rand() % ( test_threshold_block_size );
            ptrs[j] = mem_heap_alloc_block( size, ( rand() % 2 ) ? MEM_HEAP_ALLOC_SHORT_TERM : MEM_HEAP_ALLOC_SHORT_TERM);
            sizes[j] = size;
            if ( ptrs[j] != NULL )
            {
                memset(ptrs[j], 0, sizes[j]);
            }
            // JERRY_ASSERT(ptrs[j] != NULL);
        }

        // mem_heap_print( true);

        for ( uint32_t j = 0; j < subiters; j++ )
        {
            if ( ptrs[j] != NULL )
            {
                for( size_t k = 0; k < sizes[j]; k++ )
                {
                    JERRY_ASSERT( ptrs[j][k] == 0 );
                }
                mem_heap_free_block( ptrs[j]);
            }
        }
    }

    mem_heap_print( true, false, true);

    return 0;
} /* main */
