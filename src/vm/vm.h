/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#ifndef VM_H
#define VM_H

#include "ecma-globals.h"
#include "jrt.h"
#include "opcodes.h"

void init_int (const opcode_t* program_p, bool dump_mem_stats);
jerry_completion_code_t run_int (void);
ecma_completion_value_t run_int_loop (int_data_t *int_data);
ecma_completion_value_t run_int_from_pos (opcode_counter_t start_pos,
                                          const ecma_value_t& this_binding_value,
                                          ecma_object_t *lex_env_p,
                                          bool is_strict,
                                          bool is_eval_code);

opcode_t read_opcode (opcode_counter_t counter);

#endif /* VM_H */

