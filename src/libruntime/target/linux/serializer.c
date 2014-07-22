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

#include "serializer.h"
#include "jerry-libc.h"
#include "opcodes.h"

FILE *dump;

#define OPCODE_STR(op) \
  #op,

static char* massive[] = {
  OP_LIST (OPCODE_STR)
  ""
};

void
serializer_init (void)
{
}

static int opcode_counter = 0;

void
serializer_dump_data (const void *data, size_t size)
{
  size_t i;

  __printf ("%03d: %20s ", opcode_counter++, massive[(int)((char*)data)[0]]);
  for (i = 1; i < size; i++)
    __printf ("%4d ", ((char*)data)[i]);

  __printf ("\n");
}

void
serializer_rewrite_data (const int8_t offset __unused, const void *data __unused, size_t size __unused)
{
  TODO (implement);
}