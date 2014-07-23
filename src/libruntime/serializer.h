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

#ifndef SERIALIZER_H
#define SERIALIZER_H 

#include "globals.h"

void serializer_init (void);

uint8_t serializer_dump_strings (const char **, uint8_t);

void serializer_dump_nums (const int *, uint8_t, uint8_t, uint8_t);

void serializer_dump_opcode (const void *);

void serializer_rewrite_opcode (const int8_t, const void *);

#endif // SERIALIZER_H
