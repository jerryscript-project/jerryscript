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
 *      current instruction's position changes by adding specified offset
 *      if argument evaluates to true.
 */
ecma_completion_value_t
opfunc_is_true_jmp_down (vm_instr_t instr, /**< instruction */
                         vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t cond_var_idx = instr.data.is_true_jmp_down.value;
  const vm_instr_counter_t offset = vm_calc_instr_counter_from_idx_idx (instr.data.is_true_jmp_down.oc_idx_1,
                                                                        instr.data.is_true_jmp_down.oc_idx_2);

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (cond_value, get_variable_value (frame_ctx_p, cond_var_idx, false), ret_value);

  ecma_completion_value_t to_bool_completion = ecma_op_to_boolean (cond_value);
  JERRY_ASSERT (ecma_is_completion_value_normal (to_bool_completion));

  if (ecma_is_completion_value_normal_true (to_bool_completion))
  {
    JERRY_ASSERT ((uint32_t) frame_ctx_p->pos + offset < MAX_OPCODES);
    frame_ctx_p->pos = (vm_instr_counter_t) (frame_ctx_p->pos + offset);
  }
  else
  {
    frame_ctx_p->pos++;
  }

  ret_value = ecma_make_empty_completion_value ();

  ECMA_FINALIZE (cond_value);

  return ret_value;
}

/* Likewise to opfunc_is_true_jmp_down, but jumps up.  */
ecma_completion_value_t
opfunc_is_true_jmp_up (vm_instr_t instr, /**< instruction */
                       vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t cond_var_idx = instr.data.is_true_jmp_up.value;
  const vm_instr_counter_t offset = vm_calc_instr_counter_from_idx_idx (instr.data.is_true_jmp_up.oc_idx_1,
                                                                        instr.data.is_true_jmp_up.oc_idx_2);

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (cond_value, get_variable_value (frame_ctx_p, cond_var_idx, false), ret_value);

  ecma_completion_value_t to_bool_completion = ecma_op_to_boolean (cond_value);
  JERRY_ASSERT (ecma_is_completion_value_normal (to_bool_completion));

  if (ecma_is_completion_value_normal_true (to_bool_completion))
  {
    JERRY_ASSERT ((uint32_t) frame_ctx_p->pos >= offset);
    frame_ctx_p->pos = (vm_instr_counter_t) (frame_ctx_p->pos - offset);
  }
  else
  {
    frame_ctx_p->pos++;
  }

  ret_value = ecma_make_empty_completion_value ();

  ECMA_FINALIZE (cond_value);

  return ret_value;
}

/**
 * 'Jump down if false' opcode handler.
 *
 * Note:
 *      current instruction's position changes by adding specified offset
 *      if argument evaluates to false.
 */
ecma_completion_value_t
opfunc_is_false_jmp_down (vm_instr_t instr, /**< instruction */
                          vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t cond_var_idx = instr.data.is_false_jmp_down.value;
  const vm_instr_counter_t offset = vm_calc_instr_counter_from_idx_idx (instr.data.is_false_jmp_down.oc_idx_1,
                                                                        instr.data.is_false_jmp_down.oc_idx_2);

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (cond_value, get_variable_value (frame_ctx_p, cond_var_idx, false), ret_value);

  ecma_completion_value_t to_bool_completion = ecma_op_to_boolean (cond_value);
  JERRY_ASSERT (ecma_is_completion_value_normal (to_bool_completion));

  if (!ecma_is_completion_value_normal_true (to_bool_completion))
  {
    JERRY_ASSERT ((uint32_t) frame_ctx_p->pos + offset < MAX_OPCODES);
    frame_ctx_p->pos = (vm_instr_counter_t) (frame_ctx_p->pos + offset);
  }
  else
  {
    frame_ctx_p->pos++;
  }

  ret_value = ecma_make_empty_completion_value ();

  ECMA_FINALIZE (cond_value);

  return ret_value;
}

/* Likewise to opfunc_is_false_jmp_down, but jumps up.  */
ecma_completion_value_t
opfunc_is_false_jmp_up (vm_instr_t instr, /**< instruction */
                        vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t cond_var_idx = instr.data.is_false_jmp_up.value;
  const vm_instr_counter_t offset = vm_calc_instr_counter_from_idx_idx (instr.data.is_false_jmp_up.oc_idx_1,
                                                                        instr.data.is_false_jmp_up.oc_idx_2);

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (cond_value, get_variable_value (frame_ctx_p, cond_var_idx, false), ret_value);

  ecma_completion_value_t to_bool_completion = ecma_op_to_boolean (cond_value);
  JERRY_ASSERT (ecma_is_completion_value_normal (to_bool_completion));

  if (!ecma_is_completion_value_normal_true (to_bool_completion))
  {
    JERRY_ASSERT ((uint32_t) frame_ctx_p->pos >= offset);
    frame_ctx_p->pos = (vm_instr_counter_t) (frame_ctx_p->pos - offset);
  }
  else
  {
    frame_ctx_p->pos++;
  }

  ret_value = ecma_make_empty_completion_value ();

  ECMA_FINALIZE (cond_value);

  return ret_value;
}

/**
 * 'Jump down' opcode handler.
 *
 * Note:
 *      the opcode changes adds specified value to current instruction position
 */
ecma_completion_value_t
opfunc_jmp_down (vm_instr_t instr, /**< instruction */
                 vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_instr_counter_t offset = vm_calc_instr_counter_from_idx_idx (instr.data.jmp_down.oc_idx_1,
                                                                        instr.data.jmp_down.oc_idx_2);

  JERRY_ASSERT (((uint32_t) frame_ctx_p->pos + offset < MAX_OPCODES));

  frame_ctx_p->pos = (vm_instr_counter_t) (frame_ctx_p->pos + offset);

  return ecma_make_empty_completion_value ();
}

/**
 * 'Jump up' opcode handler.
 *
 * Note:
 *      the opcode changes substracts specified value from current instruction position
 */
ecma_completion_value_t
opfunc_jmp_up (vm_instr_t instr, /**< instruction */
               vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_instr_counter_t offset = vm_calc_instr_counter_from_idx_idx (instr.data.jmp_up.oc_idx_1,
                                                                        instr.data.jmp_up.oc_idx_2);
  JERRY_ASSERT ((uint32_t) frame_ctx_p->pos >= offset);

  frame_ctx_p->pos = (vm_instr_counter_t) (frame_ctx_p->pos - offset);

  return ecma_make_empty_completion_value ();
}

/**
 * 'Break or continue jump' opcode handler.
 *
 * Note:
 *      the opcode returns break-continue completion value with jump target
 */
ecma_completion_value_t
opfunc_jmp_break_continue (vm_instr_t instr, /**< instruction */
                           vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  vm_instr_counter_t target = frame_ctx_p->pos;
  target = (vm_instr_counter_t) (target + vm_calc_instr_counter_from_idx_idx (instr.data.jmp_down.oc_idx_1,
                                                                              instr.data.jmp_down.oc_idx_2));

  return ecma_make_jump_completion_value (target);
} /* opfunc_jmp_break_continue */
