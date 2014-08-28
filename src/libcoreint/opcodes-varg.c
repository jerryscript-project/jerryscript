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

#include "opcodes-ecma-support.h"

#include "globals.h"
#include "interpreter.h"
#include "opcodes.h"

/**
 * Fill arguments' list
 *
 * @return empty completion value if argument list was filled successfully,
 *         otherwise - not normal completion value indicating completion type
 *         of last expression evaluated
 */
ecma_completion_value_t
fill_varg_list (int_data_t *int_data, /**< interpreter context */
                ecma_length_t args_number, /**< number of arguments */
                ecma_value_t arg_values[], /**< out: arguments' values */
                ecma_length_t *out_arg_number_p) /**< out: number of arguments
                                                      successfully read */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ecma_length_t arg_index;
  for (arg_index = 0;
       arg_index < args_number;
       arg_index++)
  {
    ecma_completion_value_t evaluate_arg_completion = run_int_loop (int_data);

    if (evaluate_arg_completion.type == ECMA_COMPLETION_TYPE_META)
    {
      opcode_t next_opcode = read_opcode (int_data->pos);
      JERRY_ASSERT (next_opcode.op_idx == __op__idx_meta);
      JERRY_ASSERT (next_opcode.data.meta.type == OPCODE_META_TYPE_VARG);

      const idx_t varg_var_idx = next_opcode.data.meta.data_1;

      ecma_completion_value_t get_arg_completion = get_variable_value (int_data, varg_var_idx, false);

      if (ecma_is_completion_value_normal (get_arg_completion))
      {
        arg_values[arg_index] = get_arg_completion.u.value;
      }
      else
      {
        ret_value = get_arg_completion;
      }
    }
    else
    {
      ret_value = evaluate_arg_completion;
    }

    if (!ecma_is_empty_completion_value (ret_value))
    {
      break;
    }

    int_data->pos++;
  }

  *out_arg_number_p = arg_index;

  return ret_value;
} /* fill_varg_list */

/**
 * Fill parameters' list
 */
void
fill_params_list (int_data_t *int_data, /**< interpreter context */
                  ecma_length_t params_number, /**< number of parameters */
                  ecma_string_t* params_names[]) /**< out: parameters' names */
{
  uint32_t param_index;
  for (param_index = 0;
       param_index < params_number;
       param_index++)
  {
    opcode_t next_opcode = read_opcode (int_data->pos);
    JERRY_ASSERT (next_opcode.op_idx == __op__idx_meta);
    JERRY_ASSERT (next_opcode.data.meta.type == OPCODE_META_TYPE_VARG);

    const idx_t param_name_lit_idx = next_opcode.data.meta.data_1;

    params_names [param_index] = ecma_new_ecma_string_from_lit_index (param_name_lit_idx);

    int_data->pos++;
  }

  JERRY_ASSERT (param_index == params_number);
} /* fill_params_list */
