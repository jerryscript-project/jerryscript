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

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ecma-globals.h"
#include "globals.h"
#include "opcodes.h"

void init_int (const opcode_t* program_p);
bool run_int (void);
ecma_completion_value_t run_int_from_pos (opcode_counter_t start_pos,
                                          ecma_value_t this_binding_value,
                                          ecma_object_t *lex_env_p,
                                          bool is_strict,
                                          bool is_eval_code);

ssize_t try_get_string_by_idx (idx_t idx, ecma_char_t *buffer_p, ssize_t buffer_size);
ecma_number_t get_number_by_idx (idx_t idx);

opcode_t read_opcode (opcode_counter_t counter);

#endif /* INTERPRETER_H */

