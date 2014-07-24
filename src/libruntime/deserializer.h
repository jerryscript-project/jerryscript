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

#ifndef DESERIALIZER_H
#define DESERIALIZER_H

#include "globals.h"
#include "ecma-globals.h"

const ecma_char_t *deserialize_string_by_id (uint8_t);
int deserialize_num_by_id (uint8_t);
const void *deserialize_bytecode (void);
uint8_t deserialize_min_temp (void);

#endif //DESERIALIZER_H