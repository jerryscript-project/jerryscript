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
#include "jrt.h"
#include "vm.h"
#include "opcodes.h"
#include "opcodes-ecma-support.h"

/**
 * 'Try' opcode handler.
 *
 * See also: ECMA-262 v5, 12.14
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_try_block (vm_instr_t instr, /**< instruction */
                  vm_frame_ctx_t *frame_ctx_p) /**< interpreter context */
{
  const vm_idx_t block_end_oc_idx_1 = instr.data.try_block.oc_idx_1;
  const vm_idx_t block_end_oc_idx_2 = instr.data.try_block.oc_idx_2;
  const vm_instr_counter_t try_end_oc = (vm_instr_counter_t) (
    vm_calc_instr_counter_from_idx_idx (block_end_oc_idx_1, block_end_oc_idx_2) + frame_ctx_p->pos);

  frame_ctx_p->pos++;

  vm_run_scope_t run_scope_try = { frame_ctx_p->pos, try_end_oc };
  ecma_completion_value_t try_completion = vm_loop (frame_ctx_p, &run_scope_try);
  JERRY_ASSERT ((!ecma_is_completion_value_empty (try_completion) && frame_ctx_p->pos <= try_end_oc)
                || (ecma_is_completion_value_empty (try_completion) && frame_ctx_p->pos == try_end_oc));
  frame_ctx_p->pos = try_end_oc;

  vm_instr_t next_instr = vm_get_instr (frame_ctx_p->bytecode_header_p->instrs_p, frame_ctx_p->pos);
  JERRY_ASSERT (next_instr.op_idx == VM_OP_META);

  if (next_instr.data.meta.type == OPCODE_META_TYPE_CATCH)
  {
    const vm_instr_counter_t catch_end_oc = (vm_instr_counter_t) (
      vm_read_instr_counter_from_meta (OPCODE_META_TYPE_CATCH,
                                       frame_ctx_p->bytecode_header_p,
                                       frame_ctx_p->pos) + frame_ctx_p->pos);
    frame_ctx_p->pos++;

    if (ecma_is_completion_value_throw (try_completion))
    {
      next_instr = vm_get_instr (frame_ctx_p->bytecode_header_p->instrs_p, frame_ctx_p->pos);
      JERRY_ASSERT (next_instr.op_idx == VM_OP_META);
      JERRY_ASSERT (next_instr.data.meta.type == OPCODE_META_TYPE_CATCH_EXCEPTION_IDENTIFIER);

      lit_cpointer_t catch_exc_val_var_name_lit_cp = bc_get_literal_cp_by_uid (next_instr.data.meta.data_1,
                                                                               frame_ctx_p->bytecode_header_p,
                                                                               frame_ctx_p->pos);
      frame_ctx_p->pos++;

      ecma_string_t *catch_exc_var_name_str_p = ecma_new_ecma_string_from_lit_cp (catch_exc_val_var_name_lit_cp);

      ecma_object_t *old_env_p = frame_ctx_p->lex_env_p;
      ecma_object_t *catch_env_p = ecma_create_decl_lex_env (old_env_p);
      ecma_completion_value_t completion = ecma_op_create_mutable_binding (catch_env_p,
                                                                           catch_exc_var_name_str_p,
                                                                           false);
      JERRY_ASSERT (ecma_is_completion_value_empty (completion));

      completion = ecma_op_set_mutable_binding (catch_env_p,
                                                catch_exc_var_name_str_p,
                                                ecma_get_completion_value_value (try_completion),
                                                false);
      JERRY_ASSERT (ecma_is_completion_value_empty (completion));

      ecma_deref_ecma_string (catch_exc_var_name_str_p);

      frame_ctx_p->lex_env_p = catch_env_p;

      ecma_free_completion_value (try_completion);

      vm_run_scope_t run_scope_catch = { frame_ctx_p->pos, catch_end_oc };
      try_completion = vm_loop (frame_ctx_p, &run_scope_catch);

      frame_ctx_p->lex_env_p = old_env_p;

      ecma_deref_object (catch_env_p);

      JERRY_ASSERT ((!ecma_is_completion_value_empty (try_completion) && frame_ctx_p->pos <= catch_end_oc)
                    || (ecma_is_completion_value_empty (try_completion) && frame_ctx_p->pos == catch_end_oc));
    }

    frame_ctx_p->pos = catch_end_oc;
  }

  next_instr = vm_get_instr (frame_ctx_p->bytecode_header_p->instrs_p, frame_ctx_p->pos);
  JERRY_ASSERT (next_instr.op_idx == VM_OP_META);

  if (next_instr.data.meta.type == OPCODE_META_TYPE_FINALLY)
  {
    const vm_instr_counter_t finally_end_oc = (vm_instr_counter_t) (
      vm_read_instr_counter_from_meta (OPCODE_META_TYPE_FINALLY,
                                       frame_ctx_p->bytecode_header_p,
                                       frame_ctx_p->pos) + frame_ctx_p->pos);
    frame_ctx_p->pos++;

    vm_run_scope_t run_scope_finally = { frame_ctx_p->pos, finally_end_oc };
    ecma_completion_value_t finally_completion = vm_loop (frame_ctx_p, &run_scope_finally);

    JERRY_ASSERT ((!ecma_is_completion_value_empty (finally_completion) && frame_ctx_p->pos <= finally_end_oc)
                  || (ecma_is_completion_value_empty (finally_completion) && frame_ctx_p->pos == finally_end_oc));
    frame_ctx_p->pos = finally_end_oc;

    if (!ecma_is_completion_value_empty (finally_completion))
    {
      ecma_free_completion_value (try_completion);
      try_completion = finally_completion;
    }
  }

  next_instr = vm_get_instr (frame_ctx_p->bytecode_header_p->instrs_p, frame_ctx_p->pos++);
  JERRY_ASSERT (next_instr.op_idx == VM_OP_META);
  JERRY_ASSERT (next_instr.data.meta.type == OPCODE_META_TYPE_END_TRY_CATCH_FINALLY);

  return try_completion;
} /* opfunc_try_block */
