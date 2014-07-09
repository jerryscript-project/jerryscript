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

/* This allocator only allocates a memmory and doesn't free it.
   Use it only in dedicated parser, otherwise use jerry fixed pool allocator.  */
#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "../globals.h"

#define ALLOCATION_BUFFER_SIZE 4096

char allocation_buffer[ALLOCATION_BUFFER_SIZE];
char *free_memory;

static void *
geppetto_allocate_memory (size_t size)
{
  void *res;
  if (!free_memory)
    free_memory = allocation_buffer;
  
  res = free_memory;
  free_memory += size;
  JERRY_ASSERT (free_memory - allocation_buffer < ALLOCATION_BUFFER_SIZE);
  return res;
}

static inline void *
malloc (size_t size)
{
  return geppetto_allocate_memory (size);
}

static inline void
free (void *mem __unused)
{
  JERRY_UNREACHABLE ();
}

#endif // ALLOCATOR_H
