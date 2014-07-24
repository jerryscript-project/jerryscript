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

#include "deserializer.h"
#include "bytecode-linux.h"

const char *
deserialize_string_by_id (uint8_t id)
{
  uint8_t size = *bytecode_data, *data = bytecode_data, offset;

  if (id >= size)
    return NULL;

  data += id + 1;

  offset = *data;

  return (char*) (bytecode_data + offset);
}

int 
deserialize_num_by_id (uint8_t id)
{
  int *num_data;
  uint8_t str_size, str_offset, *data, num_size;

  str_size = *bytecode_data;
  data = bytecode_data + str_size;
  if (id < str_size)
    return 0;
  id = (uint8_t) (id - str_size);
  str_offset = *data;
  data = bytecode_data + str_offset;

  while (*data)
    data++;

  num_size = *(++data);
  num_data = (int *) ++data;

  if (id >= num_size)
    return 0;

  return num_data[id];
}

const void *
deserialize_bytecode (void)
{
  return bytecode_opcodes;
}
