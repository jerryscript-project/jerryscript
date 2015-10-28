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

#include "mem-allocator.h"

#include "test-common.h"

// Heap size is 32K
#define test_heap_size (32 * 1024)

// Iterations count
#define test_iters (4 * 1024)

// Subiterations count
#define test_sub_iters 32

// Threshold size of block to allocate
#define test_threshold_block_size 8192

uint8_t *ptrs[test_sub_iters];
size_t sizes[test_sub_iters];
bool is_one_chunked[test_sub_iters];

static void
test_heap_give_some_memory_back (mem_try_give_memory_back_severity_t severity)
{
  int p;

  if (severity == MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_LOW)
  {
    p = 8;
  }
  else
  {
    JERRY_ASSERT (severity == MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_HIGH);

    p = 1;
  }

  for (int i = 0; i < test_sub_iters; i++)
  {
    if (rand () % p == 0)
    {
      if (ptrs[i] != NULL)
      {
        for (size_t k = 0; k < sizes[i]; k++)
        {
          JERRY_ASSERT (ptrs[i][k] == 0);
        }

        mem_heap_free_block (ptrs[i]);
        ptrs[i] = NULL;
      }
    }
  }
} /* test_heap_give_some_memory_back */

int
main (int __attr_unused___ argc,
      char __attr_unused___ **argv)
{
  TEST_INIT ();

  mem_heap_init ();

  mem_register_a_try_give_memory_back_callback (test_heap_give_some_memory_back);

  mem_heap_print (true, false, true);

  for (uint32_t i = 0; i < test_iters; i++)
  {
    for (uint32_t j = 0; j < test_sub_iters; j++)
    {
      if (rand () % 2)
      {
        size_t size = (size_t) rand () % test_threshold_block_size;
        ptrs[j] = (uint8_t*) mem_heap_alloc_block (size,
                                                   (rand () % 2) ?
                                                   MEM_HEAP_ALLOC_LONG_TERM : MEM_HEAP_ALLOC_SHORT_TERM);
        sizes[j] = size;
        is_one_chunked[j] = false;
      }
      else
      {
        ptrs[j] = (uint8_t*) mem_heap_alloc_chunked_block ((rand () % 2) ?
                                                           MEM_HEAP_ALLOC_LONG_TERM : MEM_HEAP_ALLOC_SHORT_TERM);
        sizes[j] = mem_heap_get_chunked_block_data_size ();
        is_one_chunked[j] = true;
      }

      JERRY_ASSERT (sizes[j] == 0 || ptrs[j] != NULL);
      memset (ptrs[j], 0, sizes[j]);

      if (is_one_chunked[j])
      {
        JERRY_ASSERT (ptrs[j] != NULL
                      && mem_heap_get_chunked_block_start (ptrs[j] + (size_t) rand () % sizes[j]) == ptrs[j]);
      }
    }

    // mem_heap_print (true);

    for (uint32_t j = 0; j < test_sub_iters; j++)
    {
      if (ptrs[j] != NULL)
      {
        for (size_t k = 0; k < sizes[j]; k++)
        {
          JERRY_ASSERT (ptrs[j][k] == 0);
        }

        if (is_one_chunked[j])
        {
          JERRY_ASSERT (sizes[j] == 0
                        || mem_heap_get_chunked_block_start (ptrs[j] + (size_t) rand () % sizes[j]) == ptrs[j]);
        }

        mem_heap_free_block (ptrs[j]);

        ptrs[j] = NULL;
      }
    }
  }

  mem_heap_print (true, false, true);

  return 0;
} /* main */
