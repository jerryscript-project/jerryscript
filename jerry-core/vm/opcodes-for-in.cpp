/* Copyright 2015 Samsung Electronics Co., Ltd.
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
#include "opcodes.h"
#include "opcodes-ecma-support.h"

/**
 * 'for-in' opcode handler
 *
 * See also:
 *          ECMA-262 v5, 12.6.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_for_in (vm_instr_t instr, /**< instruction */
               vm_frame_ctx_t *int_data_p) /**< interpreter context */
{
  const vm_idx_t expr_idx = instr.data.for_in.expr;
  const vm_idx_t block_end_oc_idx_1 = instr.data.for_in.oc_idx_1;
  const vm_idx_t block_end_oc_idx_2 = instr.data.for_in.oc_idx_2;
  const vm_instr_counter_t for_in_end_oc = (vm_instr_counter_t) (
    vm_calc_instr_counter_from_idx_idx (block_end_oc_idx_1,
                                        block_end_oc_idx_2) + int_data_p->pos);

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1., 2. */
  ECMA_TRY_CATCH (expr_value,
                  get_variable_value (int_data_p,
                                      expr_idx,
                                      false),
                  ret_value);

  int_data_p->pos++;

  vm_instr_t meta_instr = vm_get_instr (int_data_p->bytecode_header_p->instrs_p, for_in_end_oc);
  JERRY_ASSERT (meta_instr.op_idx == VM_OP_META);
  JERRY_ASSERT (meta_instr.data.meta.type == OPCODE_META_TYPE_END_FOR_IN);

  /* 3. */
  if (!ecma_is_value_undefined (expr_value)
      && !ecma_is_value_null (expr_value))
  {
    /* 4. */
    ECMA_TRY_CATCH (obj_expr_value,
                    ecma_op_to_object (expr_value),
                    ret_value);

    ecma_object_t *obj_p = ecma_get_object_from_value (obj_expr_value);

    ecma_collection_iterator_t names_iterator;
    ecma_collection_header_t *names_p = ecma_op_object_get_property_names (obj_p, false, true, true);

    if (names_p != NULL)
    {
      ecma_collection_iterator_init (&names_iterator, names_p);

      const vm_instr_counter_t for_in_body_begin_oc = int_data_p->pos;
      const vm_instr_counter_t for_in_body_end_oc = for_in_end_oc;

      while (ecma_collection_iterator_next (&names_iterator))
      {
        ecma_value_t name_value = *names_iterator.current_value_p;

        ecma_string_t *name_p = ecma_get_string_from_value (name_value);

        if (ecma_op_object_get_property (obj_p, name_p) != NULL)
        {
          ecma_completion_value_t completion = set_variable_value (int_data_p,
                                                                   int_data_p->pos,
                                                                   VM_REG_SPECIAL_FOR_IN_PROPERTY_NAME,
                                                                   name_value);
          JERRY_ASSERT (ecma_is_completion_value_empty (completion));

          vm_run_scope_t run_scope_for_in = { for_in_body_begin_oc, for_in_body_end_oc };

          ecma_completion_value_t for_in_body_completion = vm_loop (int_data_p, &run_scope_for_in);
          if (ecma_is_completion_value_empty (for_in_body_completion))
          {
            JERRY_ASSERT (int_data_p->pos == for_in_body_end_oc);

            int_data_p->pos = for_in_body_begin_oc;
          }
          else
          {
            JERRY_ASSERT (ecma_is_completion_value_throw (for_in_body_completion)
                          || ecma_is_completion_value_return (for_in_body_completion)
                          || ecma_is_completion_value_jump (for_in_body_completion));
            JERRY_ASSERT (int_data_p->pos <= for_in_body_end_oc);

            ret_value = for_in_body_completion;
            break;
          }
        }
      }

      ecma_free_values_collection (names_p, true);
    }

    ECMA_FINALIZE (obj_expr_value);
  }

  int_data_p->pos = (vm_instr_counter_t) (for_in_end_oc + 1u);

  ECMA_FINALIZE (expr_value);

  return ret_value;
} /* opfunc_for_in */

