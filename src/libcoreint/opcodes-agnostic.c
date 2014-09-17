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
 * 'Jump down if true' opcode handler.
 *
 * Note:
 *      current opcode's position changes by adding specified offset
 *      if argument evaluates to true.
 */
ecma_completion_value_t
opfunc_is_true_jmp_down (opcode_t opdata, /**< operation data */
                         int_data_t *int_data) /**< interpreter context */
{
  const idx_t cond_var_idx = opdata.data.is_true_jmp_down.value;
  const opcode_counter_t offset = calc_opcode_counter_from_idx_idx (opdata.data.is_true_jmp_down.opcode_1,
                                                                    opdata.data.is_true_jmp_down.opcode_2);

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (cond_value, get_variable_value (int_data, cond_var_idx, false), ret_value);

  ecma_completion_value_t to_bool_completion = ecma_op_to_boolean (cond_value.u.value);
  JERRY_ASSERT (ecma_is_completion_value_normal (to_bool_completion));

  if (ecma_is_value_true (to_bool_completion.u.value))
  {
    JERRY_ASSERT (offset != 0 && ((uint32_t) int_data->pos + offset < MAX_OPCODES));
    int_data->pos = (opcode_counter_t) (int_data->pos + offset);
  }
  else
  {
    int_data->pos++;
  }

  ret_value = ecma_make_empty_completion_value ();

  ECMA_FINALIZE (cond_value);

  return ret_value;
}

/* Likewise to opfunc_is_true_jmp_down, but jumps up.  */
ecma_completion_value_t
opfunc_is_true_jmp_up (opcode_t opdata, /**< operation data */
                       int_data_t *int_data) /**< interpreter context */
{
  const idx_t cond_var_idx = opdata.data.is_true_jmp_up.value;
  const opcode_counter_t offset = calc_opcode_counter_from_idx_idx (opdata.data.is_true_jmp_up.opcode_1,
                                                                    opdata.data.is_true_jmp_up.opcode_2);

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (cond_value, get_variable_value (int_data, cond_var_idx, false), ret_value);

  ecma_completion_value_t to_bool_completion = ecma_op_to_boolean (cond_value.u.value);
  JERRY_ASSERT (ecma_is_completion_value_normal (to_bool_completion));

  if (ecma_is_value_true (to_bool_completion.u.value))
  {
    JERRY_ASSERT (offset != 0 && (uint32_t) int_data->pos >= offset);
    int_data->pos = (opcode_counter_t) (int_data->pos - offset);
  }
  else
  {
    int_data->pos++;
  }

  ret_value = ecma_make_empty_completion_value ();

  ECMA_FINALIZE (cond_value);

  return ret_value;
}

/**
 * 'Jump down if false' opcode handler.
 *
 * Note:
 *      current opcode's position changes by adding specified offset
 *      if argument evaluates to false.
 */
ecma_completion_value_t
opfunc_is_false_jmp_down (opcode_t opdata, /**< operation data */
                          int_data_t *int_data) /**< interpreter context */
{
  const idx_t cond_var_idx = opdata.data.is_false_jmp_down.value;
  const opcode_counter_t offset = calc_opcode_counter_from_idx_idx (opdata.data.is_false_jmp_down.opcode_1,
                                                                    opdata.data.is_false_jmp_down.opcode_2);

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (cond_value, get_variable_value (int_data, cond_var_idx, false), ret_value);

  ecma_completion_value_t to_bool_completion = ecma_op_to_boolean (cond_value.u.value);
  JERRY_ASSERT (ecma_is_completion_value_normal (to_bool_completion));

  if (!ecma_is_value_true (to_bool_completion.u.value))
  {
    JERRY_ASSERT (offset != 0 && ((uint32_t) int_data->pos + offset < MAX_OPCODES));
    int_data->pos = (opcode_counter_t) (int_data->pos + offset);
  }
  else
  {
    int_data->pos++;
  }

  ret_value = ecma_make_empty_completion_value ();

  ECMA_FINALIZE (cond_value);

  return ret_value;
}

/* Likewise to opfunc_is_false_jmp_down, but jumps up.  */
ecma_completion_value_t
opfunc_is_false_jmp_up (opcode_t opdata, /**< operation data */
                        int_data_t *int_data) /**< interpreter context */
{
  const idx_t cond_var_idx = opdata.data.is_false_jmp_up.value;
  const opcode_counter_t offset = calc_opcode_counter_from_idx_idx (opdata.data.is_false_jmp_up.opcode_1,
                                                                    opdata.data.is_false_jmp_up.opcode_2);

  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (cond_value, get_variable_value (int_data, cond_var_idx, false), ret_value);

  ecma_completion_value_t to_bool_completion = ecma_op_to_boolean (cond_value.u.value);
  JERRY_ASSERT (ecma_is_completion_value_normal (to_bool_completion));

  if (!ecma_is_value_true (to_bool_completion.u.value))
  {
    JERRY_ASSERT (offset != 0 && (uint32_t) int_data->pos >= offset);
    int_data->pos = (opcode_counter_t) (int_data->pos - offset);
  }
  else
  {
    int_data->pos++;
  }

  ret_value = ecma_make_empty_completion_value ();

  ECMA_FINALIZE (cond_value);

  return ret_value;
}

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
  const opcode_counter_t offset = calc_opcode_counter_from_idx_idx (opdata.data.jmp_down.opcode_1,
                                                                    opdata.data.jmp_down.opcode_2);

  JERRY_ASSERT (offset != 0 && ((uint32_t) int_data->pos + offset < MAX_OPCODES));

  int_data->pos = (opcode_counter_t) (int_data->pos + offset);

  return ecma_make_empty_completion_value ();
}

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
  const opcode_counter_t offset = calc_opcode_counter_from_idx_idx (opdata.data.jmp_up.opcode_1,
                                                                    opdata.data.jmp_up.opcode_2);
  JERRY_ASSERT (offset != 0 && (uint32_t) int_data->pos >= offset);

  int_data->pos = (opcode_counter_t) (int_data->pos - offset);

  return ecma_make_empty_completion_value ();
}
