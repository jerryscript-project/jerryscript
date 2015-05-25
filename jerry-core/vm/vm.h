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

extern void vm_init (const opcode_t* program_p, bool dump_mem_stats);
extern jerry_completion_code_t vm_run_global (void);
extern ecma_completion_value_t vm_loop (int_data_t *int_data);
extern ecma_completion_value_t vm_run_from_pos (opcode_counter_t start_pos,
                                                ecma_value_t this_binding_value,
                                                ecma_object_t *lex_env_p,
                                                bool is_strict,
                                                bool is_eval_code);

extern opcode_t vm_get_opcode (opcode_counter_t counter);

extern ecma_value_t vm_get_this_binding (void);
extern ecma_object_t* vm_get_lex_env (void);

#endif /* VM_H */

