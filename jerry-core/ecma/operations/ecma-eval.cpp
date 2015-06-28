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

#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-eval.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "parser.h"
#include "serializer.h"
#include "vm.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup eval eval
 */

/**
 * Perform 'eval' with code stored in ecma-string
 *
 * See also:
 *          ecma_op_eval_chars_buffer
 *          ECMA-262 v5, 15.1.2.1 (steps 2 to 8)
 *
 * @return completion value
 */
ecma_completion_value_t
ecma_op_eval (ecma_string_t *code_p, /**< code string */
              bool is_direct, /**< is eval called directly (ECMA-262 v5, 15.1.2.1.1) */
              bool is_called_from_strict_mode_code) /**< is eval is called from strict mode code */
{
  ecma_completion_value_t ret_value;

  int32_t chars_num = ecma_string_get_length (code_p);
  MEM_DEFINE_LOCAL_ARRAY (code_zt_buffer_p,
                          chars_num + 1,
                          ecma_char_t);

  const ssize_t buf_size = (ssize_t) sizeof (ecma_char_t) * (chars_num + 1);
  ssize_t buffer_size_req = ecma_string_to_zt_string (code_p,
                                                      code_zt_buffer_p,
                                                      buf_size);
  JERRY_ASSERT (buffer_size_req == buf_size);

  ret_value = ecma_op_eval_chars_buffer (code_zt_buffer_p,
                                         (size_t) buf_size,
                                         is_direct,
                                         is_called_from_strict_mode_code);

  MEM_FINALIZE_LOCAL_ARRAY (code_zt_buffer_p);

  return ret_value;
} /* ecma_op_eval */

/**
 * Perform 'eval' with code stored in continuous character buffer
 *
 * See also:
 *          ecma_op_eval
 *          ECMA-262 v5, 15.1.2.1 (steps 2 to 8)
 *
 * @return completion value
 */
ecma_completion_value_t
ecma_op_eval_chars_buffer (const ecma_char_t *code_p, /**< code characters buffer */
                           size_t code_buffer_size, /**< size of the buffer */
                           bool is_direct, /**< is eval called directly (ECMA-262 v5, 15.1.2.1.1) */
                           bool is_called_from_strict_mode_code) /**< is eval is called from strict mode code */
{
  JERRY_ASSERT (code_p != NULL);

  ecma_completion_value_t completion;

  const opcode_t *opcodes_p;
  bool is_syntax_correct;

  is_syntax_correct = parser_parse_eval ((const char *) code_p,
                                         code_buffer_size,
                                         &opcodes_p);

  if (!is_syntax_correct)
  {
    completion = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_SYNTAX));
  }
  else
  {
    opcode_counter_t first_opcode_index = 0u;
    bool is_strict_prologue = false;
    opcode_scope_code_flags_t scope_flags = vm_get_scope_flags (opcodes_p,
                                                                first_opcode_index++);
    if (scope_flags & OPCODE_SCOPE_CODE_FLAGS_STRICT)
    {
      is_strict_prologue = true;
    }

    bool is_strict = (is_strict_prologue || (is_direct && is_called_from_strict_mode_code));

    ecma_value_t this_binding;
    ecma_object_t *lex_env_p;

    /* ECMA-262 v5, 10.4.2 */
    if (is_direct)
    {
      this_binding = vm_get_this_binding ();
      lex_env_p = vm_get_lex_env ();
    }
    else
    {
      this_binding = ecma_make_object_value (ecma_builtin_get (ECMA_BUILTIN_ID_GLOBAL));
      lex_env_p = ecma_get_global_environment ();
    }

    if (is_strict)
    {
      ecma_object_t *strict_lex_env_p = ecma_create_decl_lex_env (lex_env_p);
      ecma_deref_object (lex_env_p);

      lex_env_p = strict_lex_env_p;
    }

    completion = vm_run_from_pos (opcodes_p,
                                  first_opcode_index,
                                  this_binding,
                                  lex_env_p,
                                  is_strict,
                                  true);

    if (ecma_is_completion_value_return (completion))
    {
      completion = ecma_make_normal_completion_value (ecma_get_completion_value_value (completion));
    }
    else
    {
      JERRY_ASSERT (ecma_is_completion_value_throw (completion));
    }

    ecma_deref_object (lex_env_p);
    ecma_free_value (this_binding, true);
  }

  return completion;
} /* ecma_op_eval_chars_buffer */

/**
 * @}
 * @}
 */
