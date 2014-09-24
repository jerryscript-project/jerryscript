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
#include "opcodes.h"
#include "interpreter.h"
#include "lp-string.h"

void serializer_init (bool show_opcodes);
void serializer_dump_strings_and_nums (const lp_string *, uint8_t,
                                       const ecma_number_t *, uint8_t);
void serializer_dump_opcode (opcode_t);
void serializer_set_writing_position (opcode_counter_t);
void serializer_rewrite_opcode (const opcode_counter_t, opcode_t);
void serializer_print_opcodes (void);
void serializer_adjust_strings (void);
void serializer_free (void);

#endif // SERIALIZER_H
