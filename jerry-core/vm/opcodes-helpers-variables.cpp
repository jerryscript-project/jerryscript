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

#include "bytecode-data.h"
#include "opcodes-ecma-support.h"

#ifndef JERRY_NDEBUG
/**
 * Perform so-called 'strict eval or arguments reference' check
 * that is used in definition of several statement handling algorithms,
 * but has no ECMA-defined name.
 */
static void
do_strict_eval_arguments_check (ecma_object_t *ref_base_lex_env_p, /**< base of ECMA-reference
                                                                        (lexical environment) */
                                ecma_string_t *var_name_string_p, /**< variable name */
                                bool is_strict) /**< flag indicating strict mode */
{
  bool is_check_failed = false;

  if (is_strict)
  {
    if (ref_base_lex_env_p != NULL)
    {
      JERRY_ASSERT (ecma_is_lexical_environment (ref_base_lex_env_p));

      ecma_string_t* magic_string_eval = ecma_get_magic_string (LIT_MAGIC_STRING_EVAL);
      ecma_string_t* magic_string_arguments = ecma_get_magic_string (LIT_MAGIC_STRING_ARGUMENTS);

      is_check_failed = (ecma_compare_ecma_strings (var_name_string_p,
                                                    magic_string_eval)
                         || ecma_compare_ecma_strings (var_name_string_p,
                                                       magic_string_arguments));

      ecma_deref_ecma_string (magic_string_eval);
      ecma_deref_ecma_string (magic_string_arguments);
    }
  }

  JERRY_ASSERT (!is_check_failed);
} /* do_strict_eval_arguments_check */
#endif /* !JERRY_NDEBUG */

/**
 * Check if the variable is register variable.
 *
 * @return true - if var_idx is a register variable,
 *         false - otherwise.
 */
bool
vm_is_reg_variable (vm_idx_t var_idx) /**< variable identifier */
{
  return (var_idx >= VM_REG_FIRST && var_idx <= VM_REG_LAST);
} /* vm_is_reg_variable */

/**
 * Get variable's value.
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
get_variable_value (vm_frame_ctx_t *frame_ctx_p, /**< interpreter context */
                    vm_idx_t var_idx, /**< variable identifier */
                    bool do_eval_or_arguments_check) /** run 'strict eval or arguments reference' check
                                                          See also: do_strict_eval_arguments_check */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (vm_is_reg_variable (var_idx))
  {
    ecma_value_t reg_value = vm_stack_frame_get_reg_value (&frame_ctx_p->stack_frame, var_idx);

    JERRY_ASSERT (!ecma_is_value_empty (reg_value));

    ret_value = ecma_make_normal_completion_value (ecma_copy_value (reg_value, true));
  }
  else
  {
    ecma_string_t var_name_string;
    lit_cpointer_t lit_cp = bc_get_literal_cp_by_uid (var_idx,
                                                      frame_ctx_p->bytecode_header_p,
                                                      frame_ctx_p->pos);
    JERRY_ASSERT (lit_cp.packed_value != MEM_CP_NULL);
    ecma_new_ecma_string_on_stack_from_lit_cp (&var_name_string, lit_cp);

    ecma_object_t *ref_base_lex_env_p = ecma_op_resolve_reference_base (frame_ctx_p->lex_env_p,
                                                                        &var_name_string);

    if (do_eval_or_arguments_check)
    {
#ifndef JERRY_NDEBUG
      do_strict_eval_arguments_check (ref_base_lex_env_p,
                                      &var_name_string,
                                      frame_ctx_p->is_strict);
#endif /* !JERRY_NDEBUG */
    }

    ret_value = ecma_op_get_value_lex_env_base (ref_base_lex_env_p,
                                                &var_name_string,
                                                frame_ctx_p->is_strict);

    ecma_check_that_ecma_string_need_not_be_freed (&var_name_string);
  }

  return ret_value;
} /* get_variable_value */

/**
 * Set variable's value.
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
set_variable_value (vm_frame_ctx_t *frame_ctx_p, /**< interpreter context */
                    vm_instr_counter_t lit_oc, /**< instruction counter for literal */
                    vm_idx_t var_idx, /**< variable identifier */
                    ecma_value_t value) /**< value to set */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (vm_is_reg_variable (var_idx))
  {
    ret_value = ecma_make_empty_completion_value ();

    ecma_value_t reg_value = vm_stack_frame_get_reg_value (&frame_ctx_p->stack_frame, var_idx);

    if (ecma_is_value_number (reg_value)
        && ecma_is_value_number (value))
    {
      *ecma_get_number_from_value (reg_value) = *ecma_get_number_from_value (value);
    }
    else
    {
      if (!ecma_is_value_empty (reg_value))
      {
        ecma_free_value (reg_value, false);
      }

      vm_stack_frame_set_reg_value (&frame_ctx_p->stack_frame, var_idx, ecma_copy_value (value, false));
    }
  }
  else
  {
    ecma_string_t var_name_string;
    lit_cpointer_t lit_cp = bc_get_literal_cp_by_uid (var_idx, frame_ctx_p->bytecode_header_p, lit_oc);
    JERRY_ASSERT (lit_cp.packed_value != MEM_CP_NULL);
    ecma_new_ecma_string_on_stack_from_lit_cp (&var_name_string, lit_cp);

    ecma_object_t *ref_base_lex_env_p = ecma_op_resolve_reference_base (frame_ctx_p->lex_env_p,
                                                                        &var_name_string);

#ifndef JERRY_NDEBUG
    do_strict_eval_arguments_check (ref_base_lex_env_p,
                                    &var_name_string,
                                    frame_ctx_p->is_strict);
#endif /* !JERRY_NDEBUG */

    ret_value = ecma_op_put_value_lex_env_base (ref_base_lex_env_p,
                                                &var_name_string,
                                                frame_ctx_p->is_strict,
                                                value);

    ecma_check_that_ecma_string_need_not_be_freed (&var_name_string);
  }

  return ret_value;
} /* set_variable_value */
