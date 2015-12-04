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
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-eval.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "parser.h"
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

  lit_utf8_size_t chars_num = ecma_string_get_size (code_p);
  if (chars_num == 0)
  {
    ret_value = ecma_make_normal_completion_value (ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED));
  }
  else
  {
    MEM_DEFINE_LOCAL_ARRAY (code_utf8_buffer_p,
                            chars_num,
                            lit_utf8_byte_t);

    const ssize_t buf_size = (ssize_t) chars_num;
    ssize_t buffer_size_req = ecma_string_to_utf8_string (code_p,
                                                          code_utf8_buffer_p,
                                                          buf_size);
    JERRY_ASSERT (buffer_size_req == buf_size);

    ret_value = ecma_op_eval_chars_buffer ((jerry_api_char_t *) code_utf8_buffer_p,
                                           (size_t) buf_size,
                                           is_direct,
                                           is_called_from_strict_mode_code);

    MEM_FINALIZE_LOCAL_ARRAY (code_utf8_buffer_p);
  }

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
ecma_op_eval_chars_buffer (const jerry_api_char_t *code_p, /**< code characters buffer */
                           size_t code_buffer_size, /**< size of the buffer */
                           bool is_direct, /**< is eval called directly (ECMA-262 v5, 15.1.2.1.1) */
                           bool is_called_from_strict_mode_code) /**< is eval is called from strict mode code */
{
  JERRY_ASSERT (code_p != NULL);

  ecma_completion_value_t completion;

  const bytecode_data_header_t *bytecode_data_p;
  jsp_status_t parse_status;

  bool is_strict_call = (is_direct && is_called_from_strict_mode_code);

  bool code_contains_functions;
  parse_status = parser_parse_eval (code_p,
                                    code_buffer_size,
                                    is_strict_call,
                                    &bytecode_data_p,
                                    &code_contains_functions);

  if (parse_status == JSP_STATUS_SYNTAX_ERROR)
  {
    completion = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_SYNTAX));
  }
  else if (parse_status == JSP_STATUS_REFERENCE_ERROR)
  {
    completion = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_REFERENCE));
  }
  else
  {
    JERRY_ASSERT (parse_status == JSP_STATUS_OK);

    completion = vm_run_eval (bytecode_data_p, is_direct);

    if (!code_contains_functions)
    {
      bc_remove_bytecode_data (bytecode_data_p);
    }
  }

  return completion;
} /* ecma_op_eval_chars_buffer */

/**
 * @}
 * @}
 */
