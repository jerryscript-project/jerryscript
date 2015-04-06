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

#include "jrt.h"
#include "mem-allocator.h"

extern "C"
{
  extern void srand (unsigned int __seed);
  extern int rand (void);
  extern long int time (long int *__timer);
  extern int printf (__const char *__restrict __format, ...);
  extern void *memset (void *__s, int __c, size_t __n);
}

// Heap size is 32K
#define test_heap_size (32 * 1024)

// Iterations count
#define test_iters (64 * 1024)

// Subiterations count
#define test_sub_iters 32

// Threshold size of block to allocate
#define test_threshold_block_size 8192

uint8_t *ptrs[test_sub_iters];
size_t sizes[test_sub_iters];

static void
test_heap_give_some_memory_back (mem_try_give_memory_back_severity_t severity)
{
  int p;

  if (severity == MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_LOW)
  {
    p = 8;
  }
  else if (severity == MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_MEDIUM)
  {
    p = 4;
  }
  else if (severity == MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_HIGH)
  {
    p = 2;
  }
  else
  {
    JERRY_ASSERT (severity == MEM_TRY_GIVE_MEMORY_BACK_SEVERITY_CRITICAL);

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
  uint8_t test_native_heap[test_heap_size];

  mem_heap_init (test_native_heap, sizeof (test_native_heap));

  srand ((unsigned int) time (NULL));
  int k = rand ();
  printf ("seed=%d\n", k);
  srand ((unsigned int) k);

  mem_register_a_try_give_memory_back_callback (test_heap_give_some_memory_back);

  mem_heap_print (true, false, true);

  for (uint32_t i = 0; i < test_iters; i++)
  {
    for (uint32_t j = 0; j < test_sub_iters; j++)
    {
      size_t size = (size_t) rand () % test_threshold_block_size;
      ptrs[j] = (uint8_t*) mem_heap_alloc_block (size,
                                                 (rand () % 2) ?
                                                 MEM_HEAP_ALLOC_SHORT_TERM : MEM_HEAP_ALLOC_SHORT_TERM);
      sizes[j] = size;

      JERRY_ASSERT (size == 0 || ptrs[j] != NULL);
      memset (ptrs[j], 0, sizes[j]);

      JERRY_ASSERT (ptrs[j] == NULL
                    || mem_heap_get_block_start (ptrs[j] + (size_t) rand () % sizes [j]) == ptrs[j]);
    }

    // mem_heap_print (true);

    for (uint32_t j = 0; j < test_sub_iters; j++)
    {
      if (ptrs[j] != NULL && (rand () % 2) == 0)
      {
        for (size_t k = 0; k < sizes[j]; k++)
        {
          JERRY_ASSERT(ptrs[j][k] == 0);
        }

        size_t new_size = (size_t) rand () % (test_threshold_block_size);

        if (mem_heap_try_resize_block (ptrs[j], new_size))
        {
          sizes[j] = new_size;
          memset (ptrs[j], 0, sizes[j]);
        }

        JERRY_ASSERT (sizes [j] == 0
                      || mem_heap_get_block_start (ptrs[j] + (size_t) rand () % sizes [j]) == ptrs[j]);
      }
    }

    for (uint32_t j = 0; j < test_sub_iters; j++)
    {
      if (ptrs[j] != NULL)
      {
        for (size_t k = 0; k < sizes[j]; k++)
        {
          JERRY_ASSERT(ptrs[j][k] == 0);
        }

        JERRY_ASSERT (sizes [j] == 0
                      || mem_heap_get_block_start (ptrs[j] + (size_t) rand () % sizes [j]) == ptrs[j]);

        mem_heap_free_block (ptrs[j]);

        ptrs[j] = NULL;
      }
    }
  }

  mem_heap_print (true, false, true);

  return 0;
} /* main */
