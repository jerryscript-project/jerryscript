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
#define	INTERPRETER_H

#include "ecma-globals.h"
#include "globals.h"
#include "opcodes.h"

typedef uint16_t opcode_counter_t;

struct __int_data
{
  opcode_counter_t pos; /**< current opcode to execute */
  ecma_value_t this_binding; /**< this binding for current context */
  ecma_object_t *lex_env_p; /**< current lexical environment */
  bool is_strict; /**< is current code execution mode strict? */
  T_IDX min_reg_num; /**< minimum idx used for register identification */
  T_IDX max_reg_num; /**< maximum idx used for register identification */
  ecma_value_t *regs_p; /**< register variables */
};

void init_int (const OPCODE* program_p);
bool run_int (void);
ecma_completion_value_t run_int_from_pos (opcode_counter_t start_pos,
                                          ecma_value_t this_binding_value,
                                          ecma_object_t *lex_env_p,
                                          bool is_strict);

ssize_t try_get_string_by_idx( T_IDX idx, ecma_char_t *buffer_p, ssize_t buffer_size);
ecma_number_t get_number_by_idx(T_IDX idx);

#endif	/* INTERPRETER_H */

