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
}

uint8_t
serializer_dump_strings (const char *strings[] __unused, uint8_t size __unused)
{
}

void 
serializer_dump_nums (const int nums[] __unused, uint8_t size __unused, uint8_t offset __unused, uint8_t strings_num __unused)
{
}

void
serializer_dump_opcode (const void *opcode __unused)
{
}

void
serializer_rewrite_opcode (const int8_t offset __unused, const void *opcode __unused)
{
}


TODO (Dump memory)