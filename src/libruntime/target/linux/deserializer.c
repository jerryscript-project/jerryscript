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

int *num_data = NULL;
uint8_t num_size = 0;

const ecma_char_t *
deserialize_string_by_id (uint8_t id)
{
  uint8_t size, *data;
  uint16_t offset;

  if (bytecode_data == NULL)
    return NULL;
  
  size = *bytecode_data;

  if (id >= size)
    return NULL;

  data = bytecode_data;

  data += id * 2 + 1;

  offset = *((uint16_t *) data);

  return ((const ecma_char_t *) bytecode_data + offset);
}

int 
deserialize_num_by_id (uint8_t id)
{
  uint16_t str_size, str_offset;
  uint8_t *data;

  str_size = *bytecode_data;
  if (id < str_size)
    {
      return 0;
    }
  id = (uint8_t) (id - str_size);
  
  if (num_data == NULL)
    {
      // Go to last string's offset
      data = (uint8_t *) (bytecode_data + str_size * 2 - 1);
      str_offset = *((uint16_t *) data);
      data = bytecode_data + str_offset;

      while (*data)
        data++;
  
      num_size = *(++data);
      num_data = (int *) ++data;
    }

  if (id >= num_size)
    return 0;

  return num_data[id];
}

const void *
deserialize_bytecode (void)
{
  return bytecode_opcodes;
}

uint8_t 
deserialize_min_temp (void)
{
  uint8_t str_size = *bytecode_data;
  
  if (num_size == 0)
    {
      deserialize_num_by_id (str_size); // Init num_data and num_size
    }

  return (uint8_t) (str_size + num_size);
}
