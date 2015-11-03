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

extern void vm_init (const bytecode_data_header_t *, bool);
extern void vm_finalize (void);
extern jerry_completion_code_t vm_run_global (void);
extern ecma_completion_value_t vm_run_eval (const bytecode_data_header_t *, bool);

extern ecma_completion_value_t vm_loop (vm_frame_ctx_t *, vm_run_scope_t *);
extern ecma_completion_value_t vm_run_from_pos (const bytecode_data_header_t *, vm_instr_counter_t,
                                                ecma_value_t, ecma_object_t *, bool, bool, ecma_collection_header_t *);

extern vm_instr_t vm_get_instr (const vm_instr_t *, vm_instr_counter_t);
extern uint8_t vm_get_scope_args_num (const bytecode_data_header_t *, vm_instr_counter_t);

extern bool vm_is_strict_mode (void);
extern bool vm_is_direct_eval_form_call (void);

extern ecma_value_t vm_get_this_binding (void);
extern ecma_object_t *vm_get_lex_env (void);

#endif /* VM_H */

