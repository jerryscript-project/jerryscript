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
 * eval
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.1
 *
 * @return completion value
 */
ecma_completion_value_t
ecma_op_eval (ecma_string_t *code_p, /**< code string */
              bool is_direct, /**< is eval called directly (ECMA-262 v5, 15.1.2.1.1) */
              bool is_called_from_strict_mode_code) /**< is eval is called from strict mode code */
{
  ecma_completion_value_t completion;

  int32_t chars_num = ecma_string_get_length (code_p);
  MEM_DEFINE_LOCAL_ARRAY (code_zt_buffer_p,
                          chars_num + 1,
                          ecma_char_t);

  const ssize_t buf_size = (ssize_t) sizeof (ecma_char_t) * (chars_num + 1);
  ssize_t buffer_size_req = ecma_string_to_zt_string (code_p,
                                                      code_zt_buffer_p,
                                                      buf_size);
  JERRY_ASSERT (buffer_size_req == buf_size);

  parser_init ();
  bool is_syntax_correct = parser_parse_eval ((const char *) code_p, (size_t) buf_size);
  const opcode_t* opcodes_p = (const opcode_t*) serializer_get_bytecode ();
  serializer_print_opcodes ();
  parser_free ();

  // FIXME:
  bool is_strict_prologue = false;
  (void) is_strict_prologue;

  if (!is_syntax_correct)
  {
    completion = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_SYNTAX));
  }
  else
  {
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

    if (is_strict_prologue
        || (is_direct && is_called_from_strict_mode_code))
    {
      ecma_object_t *strict_lex_env_p = ecma_create_decl_lex_env (lex_env_p);
      ecma_deref_object (lex_env_p);

      lex_env_p = strict_lex_env_p;
    }

    // FIXME: Call interpreter
    (void) opcodes_p;
    completion = ecma_make_return_completion_value (ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED));
    JERRY_UNIMPLEMENTED ("eval operation is not implemented");

    if (ecma_is_completion_value_return (completion))
    {
      completion = ecma_make_normal_completion_value (ecma_get_completion_value_value (completion));
    }
    else
    {
      JERRY_ASSERT (ecma_is_completion_value_throw (completion)
                    || ecma_is_completion_value_exit (completion));
    }

    ecma_deref_object (lex_env_p);
    ecma_free_value (this_binding, true);
  }


  MEM_FINALIZE_LOCAL_ARRAY (code_zt_buffer_p);

  return completion;
} /* ecma_op_eval */

/**
 * @}
 * @}
 */
