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

#ifndef OPCODES_ECMA_SUPPORT_H
#define OPCODES_ECMA_SUPPORT_H

#include "ecma-alloc.h"
#include "ecma-array-object.h"
#include "ecma-builtins.h"
#include "ecma-comparison.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-number-arithmetic.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"
#include "ecma-reference.h"
#include "ecma-regexp-object.h"
#include "ecma-try-catch-macro.h"
#include "serializer.h"

bool is_reg_variable (vm_frame_ctx_t *frame_ctx_p, vm_idx_t var_idx);
ecma_completion_value_t get_variable_value (vm_frame_ctx_t *, vm_idx_t, bool);
ecma_completion_value_t set_variable_value (vm_frame_ctx_t *, vm_instr_counter_t, vm_idx_t, ecma_value_t);
ecma_completion_value_t vm_fill_varg_list (vm_frame_ctx_t *frame_ctx_p,
                                           ecma_length_t args_number,
                                           ecma_collection_header_t *args_values_p);
extern void vm_fill_params_list (vm_frame_ctx_t *frame_ctx_p,
                                 ecma_length_t params_number,
                                 ecma_collection_header_t *formal_params_collection_p);
#endif /* OPCODES_ECMA_SUPPORT_H */
