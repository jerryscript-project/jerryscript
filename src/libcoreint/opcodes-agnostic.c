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

#include "opcodes.h"
#include "opcodes-ecma-support.h"

/**
 * 'Jump down if true' opcode handler.
 *
 * Note:
 *      current opcode's position changes by adding specified offset
 *      if argument evaluates to true.
 */
void
opfunc_is_true_jmp_down (ecma_completion_value_t &ret_value, /**< out: completion value */
                         opcode_t opdata, /**< operation data */
                         int_data_t *int_data) /**< interpreter context */
{
  const idx_t cond_var_idx = opdata.data.is_true_jmp_down.value;
  const opcode_counter_t offset = calc_opcode_counter_from_idx_idx (opdata.data.is_true_jmp_down.opcode_1,
                                                                    opdata.data.is_true_jmp_down.opcode_2);

  ECMA_TRY_CATCH (ret_value, get_variable_value, cond_value, int_data, cond_var_idx, false);

  ecma_completion_value_t to_bool_completion;
  ecma_op_to_boolean (to_bool_completion, cond_value);
  JERRY_ASSERT (ecma_is_completion_value_normal (to_bool_completion));

  if (ecma_is_completion_value_normal_true (to_bool_completion))
  {
    JERRY_ASSERT (offset != 0 && ((uint32_t) int_data->pos + offset < MAX_OPCODES));
    int_data->pos = (opcode_counter_t) (int_data->pos + offset);
  }
  else
  {
    int_data->pos++;
  }

  ecma_make_empty_completion_value (ret_value);

  ECMA_FINALIZE (cond_value);
}

/* Likewise to opfunc_is_true_jmp_down, but jumps up.  */
void
opfunc_is_true_jmp_up (ecma_completion_value_t &ret_value, /**< out: completion value */
                       opcode_t opdata, /**< operation data */
                       int_data_t *int_data) /**< interpreter context */
{
  const idx_t cond_var_idx = opdata.data.is_true_jmp_up.value;
  const opcode_counter_t offset = calc_opcode_counter_from_idx_idx (opdata.data.is_true_jmp_up.opcode_1,
                                                                    opdata.data.is_true_jmp_up.opcode_2);

  ECMA_TRY_CATCH (ret_value, get_variable_value, cond_value, int_data, cond_var_idx, false);

  ecma_completion_value_t to_bool_completion;
  ecma_op_to_boolean (to_bool_completion, cond_value);
  JERRY_ASSERT (ecma_is_completion_value_normal (to_bool_completion));

  if (ecma_is_completion_value_normal_true (to_bool_completion))
  {
    JERRY_ASSERT (offset != 0 && (uint32_t) int_data->pos >= offset);
    int_data->pos = (opcode_counter_t) (int_data->pos - offset);
  }
  else
  {
    int_data->pos++;
  }

  ecma_make_empty_completion_value (ret_value);

  ECMA_FINALIZE (cond_value);
}

/**
 * 'Jump down if false' opcode handler.
 *
 * Note:
 *      current opcode's position changes by adding specified offset
 *      if argument evaluates to false.
 */
void
opfunc_is_false_jmp_down (ecma_completion_value_t &ret_value, /**< out: completion value */
                          opcode_t opdata, /**< operation data */
                          int_data_t *int_data) /**< interpreter context */
{
  const idx_t cond_var_idx = opdata.data.is_false_jmp_down.value;
  const opcode_counter_t offset = calc_opcode_counter_from_idx_idx (opdata.data.is_false_jmp_down.opcode_1,
                                                                    opdata.data.is_false_jmp_down.opcode_2);

  ECMA_TRY_CATCH (ret_value, get_variable_value, cond_value, int_data, cond_var_idx, false);

  ecma_completion_value_t to_bool_completion;
  ecma_op_to_boolean (to_bool_completion, cond_value);
  JERRY_ASSERT (ecma_is_completion_value_normal (to_bool_completion));

  if (!ecma_is_completion_value_normal_true (to_bool_completion))
  {
    JERRY_ASSERT (offset != 0 && ((uint32_t) int_data->pos + offset < MAX_OPCODES));
    int_data->pos = (opcode_counter_t) (int_data->pos + offset);
  }
  else
  {
    int_data->pos++;
  }

  ecma_make_empty_completion_value (ret_value);

  ECMA_FINALIZE (cond_value);
}

/* Likewise to opfunc_is_false_jmp_down, but jumps up.  */
void
opfunc_is_false_jmp_up (ecma_completion_value_t &ret_value, /**< out: completion value */
                        opcode_t opdata, /**< operation data */
                        int_data_t *int_data) /**< interpreter context */
{
  const idx_t cond_var_idx = opdata.data.is_false_jmp_up.value;
  const opcode_counter_t offset = calc_opcode_counter_from_idx_idx (opdata.data.is_false_jmp_up.opcode_1,
                                                                    opdata.data.is_false_jmp_up.opcode_2);

  ECMA_TRY_CATCH (ret_value, get_variable_value, cond_value, int_data, cond_var_idx, false);

  ecma_completion_value_t to_bool_completion;
  ecma_op_to_boolean (to_bool_completion, cond_value);
  JERRY_ASSERT (ecma_is_completion_value_normal (to_bool_completion));

  if (!ecma_is_completion_value_normal_true (to_bool_completion))
  {
    JERRY_ASSERT (offset != 0 && (uint32_t) int_data->pos >= offset);
    int_data->pos = (opcode_counter_t) (int_data->pos - offset);
  }
  else
  {
    int_data->pos++;
  }

  ecma_make_empty_completion_value (ret_value);

  ECMA_FINALIZE (cond_value);
}

/**
 * 'Jump down' opcode handler.
 *
 * Note:
 *      the opcode changes adds specified value to current opcode position
 */
void
opfunc_jmp_down (ecma_completion_value_t &ret_value, /**< out: completion value */
                 opcode_t opdata, /**< operation data */
                 int_data_t *int_data) /**< interpreter context */
{
  const opcode_counter_t offset = calc_opcode_counter_from_idx_idx (opdata.data.jmp_down.opcode_1,
                                                                    opdata.data.jmp_down.opcode_2);

  JERRY_ASSERT (offset != 0 && ((uint32_t) int_data->pos + offset < MAX_OPCODES));

  int_data->pos = (opcode_counter_t) (int_data->pos + offset);

  ecma_make_empty_completion_value (ret_value);
}

/**
 * 'Jump up' opcode handler.
 *
 * Note:
 *      the opcode changes substracts specified value from current opcode position
 */
void
opfunc_jmp_up (ecma_completion_value_t &ret_value, /**< out: completion value */
               opcode_t opdata, /**< operation data */
               int_data_t *int_data) /**< interpreter context */
{
  const opcode_counter_t offset = calc_opcode_counter_from_idx_idx (opdata.data.jmp_up.opcode_1,
                                                                    opdata.data.jmp_up.opcode_2);
  JERRY_ASSERT (offset != 0 && (uint32_t) int_data->pos >= offset);

  int_data->pos = (opcode_counter_t) (int_data->pos - offset);

  ecma_make_empty_completion_value (ret_value);
}
