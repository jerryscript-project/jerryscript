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
#include "globals.h"

void
serializer_init (void)
{
  JERRY_UNIMPLEMENTED();
}

uint8_t
serializer_dump_strings (const char *strings[], uint8_t size)
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS( strings, size);
}

void 
serializer_dump_nums (const int nums[], uint8_t size, uint8_t offset, uint8_t strings_num)
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS( nums, size, offset, strings_num);
}

void
serializer_dump_opcode (const void *opcode)
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS( opcode);
}

void
serializer_rewrite_opcode (const uint8_t offset, const void *opcode)
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS( offset, opcode);
}


TODO (Dump memory)
