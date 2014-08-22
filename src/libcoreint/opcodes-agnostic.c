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

#include "opcodes.h"
#include "opcodes-ecma-support.h"

/**
 * 'Jump if true' opcode handler.
 *
 * Note:
 *      the opcode changes current opcode position to specified opcode index
 *      if argument evaluates to true.
 */
ecma_completion_value_t
opfunc_is_true_jmp (opcode_t opdata, /**< operation data */
                    int_data_t *int_data) /**< interpreter context */
{
  const idx_t cond_var_idx = opdata.data.is_true_jmp.value;
  const idx_t dst_opcode_idx = opdata.data.is_true_jmp.opcode;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (cond_value, get_variable_value (int_data, cond_var_idx, false), ret_value);

  ecma_completion_value_t to_bool_completion = ecma_op_to_boolean (cond_value.value);
  JERRY_ASSERT (ecma_is_completion_value_normal (to_bool_completion));

  if (ecma_is_value_true (to_bool_completion.value))
  {
    int_data->pos = dst_opcode_idx;
  }
  else
  {
    int_data->pos++;
  }

  ret_value = ecma_make_empty_completion_value ();

  ECMA_FINALIZE (cond_value);

  return ret_value;
} /* opfunc_is_true_jmp */

/**
 * 'Jump if false' opcode handler.
 *
 * Note:
 *      the opcode changes current opcode position to specified opcode index
 *      if argument evaluates to false.
 */
ecma_completion_value_t
opfunc_is_false_jmp (opcode_t opdata, /**< operation data */
                     int_data_t *int_data) /**< interpreter context */
{
  const idx_t cond_var_idx = opdata.data.is_false_jmp.value;
  const idx_t dst_opcode_idx = opdata.data.is_false_jmp.opcode;

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (cond_value, get_variable_value (int_data, cond_var_idx, false), ret_value);

  ecma_completion_value_t to_bool_completion = ecma_op_to_boolean (cond_value.value);
  JERRY_ASSERT (ecma_is_completion_value_normal (to_bool_completion));

  if (!ecma_is_value_true (to_bool_completion.value))
  {
    int_data->pos = dst_opcode_idx;
  }
  else
  {
    int_data->pos++;
  }

  ret_value = ecma_make_empty_completion_value ();

  ECMA_FINALIZE (cond_value);

  return ret_value;
} /* opfunc_is_false_jmp */

/**
 * 'Jump' opcode handler.
 *
 * Note:
 *      the opcode changes current opcode position to specified opcode index
 */
ecma_completion_value_t
opfunc_jmp (opcode_t opdata, /**< operation data */
            int_data_t *int_data) /**< interpreter context */
{
  int_data->pos = opdata.data.jmp.opcode_idx;

  return ecma_make_empty_completion_value ();
} /* opfunc_jmp */

/**
 * 'Jump down' opcode handler.
 *
 * Note:
 *      the opcode changes adds specified value to current opcode position
 */
ecma_completion_value_t
opfunc_jmp_down (opcode_t opdata, /**< operation data */
                 int_data_t *int_data) /**< interpreter context */
{
  JERRY_ASSERT (int_data->pos <= int_data->pos + opdata.data.jmp_up.opcode_count);

  int_data->pos = (opcode_counter_t) (int_data->pos + opdata.data.jmp_down.opcode_count);

  return ecma_make_empty_completion_value ();
} /* opfunc_jmp_down */

/**
 * 'Jump up' opcode handler.
 *
 * Note:
 *      the opcode changes substracts specified value from current opcode position
 */
ecma_completion_value_t
opfunc_jmp_up (opcode_t opdata, /**< operation data */
               int_data_t *int_data) /**< interpreter context */
{
  JERRY_ASSERT (int_data->pos >= opdata.data.jmp_up.opcode_count);

  int_data->pos = (opcode_counter_t) (int_data->pos - opdata.data.jmp_down.opcode_count);

  return ecma_make_empty_completion_value ();
} /* opfunc_jmp_up */
