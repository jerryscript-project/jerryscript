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

/**
 * Unit test for pool manager.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define JERRY_MEM_POOL_INTERNAL

#include "jrt.h"
#include "mem-allocator.h"
#include "mem-pool.h"
#include "mem-poolman.h"

// Iterations count
const uint32_t test_iters = 16384;

// Subiterations count
const uint32_t test_max_sub_iters = 32;

uint8_t *ptrs[test_max_sub_iters];

int
main (int __attr_unused___ argc,
      char __attr_unused___ **argv)
{
  mem_init ();

  srand ((unsigned int) time (NULL));
  unsigned int seed = (unsigned int) rand ();
  printf ("seed=%u\n", seed);
  srand (seed);

  for (uint32_t i = 0; i < test_iters; i++)
  {
    const size_t subiters = ((size_t) rand () % test_max_sub_iters) + 1;

    for (size_t j = 0; j < subiters; j++)
    {
      ptrs[j] = mem_pools_alloc ();
      // JERRY_ASSERT(ptrs[j] != NULL);

      if (ptrs[j] != NULL)
      {
        memset (ptrs[j], 0, MEM_POOL_CHUNK_SIZE);
      }
    }

    // mem_heap_print (false);

    for (size_t j = 0; j < subiters; j++)
    {
      if (ptrs[j] != NULL)
      {
        for (size_t k = 0; k < MEM_POOL_CHUNK_SIZE; k++)
        {
          JERRY_ASSERT(((uint8_t*) ptrs[j])[k] == 0);
        }

        mem_pools_free (ptrs[j]);
      }
    }
  }

#ifdef MEM_STATS
  mem_pools_stats_t stats;
  mem_pools_get_stats (&stats);

  printf ("Pools stats:\n");
  printf (" Chunk size: %u\n"
          "  Pools: %lu\n"
          "  Allocated chunks: %lu\n"
          "  Free chunks: %lu\n"
          "  Peak pools: %lu\n"
          "  Peak allocated chunks: %lu\n\n",
          MEM_POOL_CHUNK_SIZE,
          stats.pools_count,
          stats.allocated_chunks,
          stats.free_chunks,
          stats.peak_pools_count,
          stats.peak_allocated_chunks);
#endif /* MEM_STATS */

  return 0;
} /* main */
