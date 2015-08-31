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

#include "opcodes-ecma-support.h"

#include "jrt.h"
#include "vm.h"
#include "opcodes.h"

/**
 * Fill arguments' list
 *
 * @return empty completion value if argument list was filled successfully,
 *         otherwise - not normal completion value indicating completion type
 *         of last expression evaluated
 */
ecma_completion_value_t
vm_fill_varg_list (vm_frame_ctx_t *frame_ctx_p, /**< interpreter context */
                   ecma_length_t args_number, /**< number of arguments */
                   ecma_collection_header_t *arg_collection_p) /** collection to fill with argument values */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ecma_length_t arg_index;
  for (arg_index = 0;
       arg_index < args_number && ecma_is_completion_value_empty (ret_value);
       arg_index++)
  {
    ECMA_TRY_CATCH (evaluate_arg,
                    vm_loop (frame_ctx_p, NULL),
                    ret_value);

    vm_instr_t next_instr = vm_get_instr (frame_ctx_p->instrs_p, frame_ctx_p->pos);
    JERRY_ASSERT (next_instr.op_idx == VM_OP_META);
    JERRY_ASSERT (next_instr.data.meta.type == OPCODE_META_TYPE_VARG);

    const vm_idx_t varg_var_idx = next_instr.data.meta.data_1;

    ECMA_TRY_CATCH (get_arg,
                    get_variable_value (frame_ctx_p, varg_var_idx, false),
                    ret_value);

    ecma_append_to_values_collection (arg_collection_p,
                                      ecma_get_completion_value_value (get_arg_completion),
                                      true);

    ECMA_FINALIZE (get_arg);

    frame_ctx_p->pos++;

    ECMA_FINALIZE (evaluate_arg);
  }

  return ret_value;
} /* vm_fill_varg_list */

/**
 * Fill parameters' list
 */
void
vm_fill_params_list (vm_frame_ctx_t *frame_ctx_p, /**< interpreter context */
                     ecma_length_t params_number, /**< number of parameters */
                     ecma_collection_header_t *formal_params_collection_p) /**< collection to fill with
                                                                            *   parameters' names */
{
  uint32_t param_index;
  for (param_index = 0;
       param_index < params_number;
       param_index++)
  {
    vm_instr_t next_instr = vm_get_instr (frame_ctx_p->instrs_p, frame_ctx_p->pos);
    JERRY_ASSERT (next_instr.op_idx == VM_OP_META);
    JERRY_ASSERT (next_instr.data.meta.type == OPCODE_META_TYPE_VARG);

    const lit_cpointer_t param_name_lit_idx = serializer_get_literal_cp_by_uid (next_instr.data.meta.data_1,
                                                                                frame_ctx_p->instrs_p,
                                                                                frame_ctx_p->pos);


    ecma_string_t *param_name_str_p = ecma_new_ecma_string_from_lit_cp (param_name_lit_idx);
    ecma_value_t param_name_value = ecma_make_string_value (param_name_str_p);

    ecma_append_to_values_collection (formal_params_collection_p, param_name_value, false);

    ecma_deref_ecma_string (param_name_str_p);

    frame_ctx_p->pos++;
  }

  JERRY_ASSERT (param_index == params_number);
} /* vm_fill_params_list */
