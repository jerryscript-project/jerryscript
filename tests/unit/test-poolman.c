/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

/**
 * Unit test for pool manager.
 */

#define JERRY_MEM_POOL_INTERNAL

#include "mem-allocator.h"
#include "mem-poolman.h"

#include "test-common.h"

// Iterations count
const uint32_t test_iters = 1024;

// Subiterations count
#define TEST_MAX_SUB_ITERS  1024

uint8_t *ptrs[TEST_MAX_SUB_ITERS];
uint8_t data[TEST_MAX_SUB_ITERS][MEM_POOL_CHUNK_SIZE];

int
main (int __attr_unused___ argc,
      char __attr_unused___ **argv)
{
  TEST_INIT ();

  mem_init ();

  for (uint32_t i = 0; i < test_iters; i++)
  {
    const size_t subiters = ((size_t) rand () % TEST_MAX_SUB_ITERS) + 1;

    for (size_t j = 0; j < subiters; j++)
    {
      ptrs[j] = (uint8_t *) mem_pools_alloc ();

      if (ptrs[j] != NULL)
      {
        for (size_t k = 0; k < MEM_POOL_CHUNK_SIZE; k++)
        {
          ptrs[j][k] = (uint8_t) (rand () % 256);
        }

        memcpy (data[j], ptrs[j], MEM_POOL_CHUNK_SIZE);
      }
    }

    // mem_heap_print (false);

    for (size_t j = 0; j < subiters; j++)
    {
      if (rand () % 256 == 0)
      {
        mem_pools_collect_empty ();
      }

      if (ptrs[j] != NULL)
      {
        JERRY_ASSERT (!memcmp (data[j], ptrs[j], MEM_POOL_CHUNK_SIZE));

        mem_pools_free (ptrs[j]);
      }
    }
  }

#ifdef MEM_STATS
  mem_pools_stats_t stats;
  mem_pools_get_stats (&stats);

  printf ("Pools stats:\n");
  printf (" Chunk size: %u\n"
          "  Pools: %zu\n"
          "  Allocated chunks: %zu\n"
          "  Free chunks: %zu\n"
          "  Peak pools: %zu\n"
          "  Peak allocated chunks: %zu\n\n",
          MEM_POOL_CHUNK_SIZE,
          stats.pools_count,
          stats.allocated_chunks,
          stats.free_chunks,
          stats.peak_pools_count,
          stats.peak_allocated_chunks);
#endif /* MEM_STATS */

  mem_finalize (false);

  return 0;
} /* main */
