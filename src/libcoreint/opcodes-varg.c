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
  ecma_completion_value_t get_arg_completion = ecma_make_empty_completion_value ();

  ecma_length_t arg_index;
  for (arg_index = 0;
       arg_index < args_number;
       arg_index++)
  {
    opcode_t next_opcode = read_opcode (int_data->pos);
    if (next_opcode.op_idx == __op__idx_varg)
    {
      get_arg_completion = get_variable_value (int_data, next_opcode.data.varg.arg_lit_idx, false);
      if (unlikely (ecma_is_completion_value_throw (get_arg_completion)))
      {
        break;
      }
      else
      {
        JERRY_ASSERT (ecma_is_completion_value_normal (get_arg_completion));
        arg_values[arg_index] = get_arg_completion.value;
      }
    }
    else
    {
      get_arg_completion = run_int_loop (int_data);

      if (get_arg_completion.type == ECMA_COMPLETION_TYPE_VARG)
      {
        arg_values[arg_index] = get_arg_completion.value;
      }
      else
      {
        break;
      }
    }

    int_data->pos++;
  }

  *out_arg_number_p = arg_index;

  if (get_arg_completion.type == ECMA_COMPLETION_TYPE_NORMAL
      || get_arg_completion.type == ECMA_COMPLETION_TYPE_VARG)
  {
    /* values are copied to arg_values */
    return ecma_make_empty_completion_value ();
  }
  else
  {
    return get_arg_completion;
  }
} /* fill_varg_list */

/**
 * 'varg' opcode handler
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_varg (opcode_t opdata, /**< operation data */
             int_data_t *int_data) /**< interpreter context */
{
  const idx_t arg_lit_idx = opdata.data.varg.arg_lit_idx;

  ecma_completion_value_t completion = get_variable_value (int_data,
                                                           arg_lit_idx,
                                                           false);

  if (ecma_is_completion_value_normal (completion))
  {
    completion.type = ECMA_COMPLETION_TYPE_VARG;
  }

  return completion;
} /* opfunc_varg */
