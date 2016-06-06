/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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
#include "js-parser.h"
#include "vm.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup eval eval
 * @{
 */

/**
 * Perform 'eval' with code stored in ecma-string
 *
 * See also:
 *          ecma_op_eval_chars_buffer
 *          ECMA-262 v5, 15.1.2.1 (steps 2 to 8)
 *
 * @return ecma value
 */
ecma_value_t
ecma_op_eval (ecma_string_t *code_p, /**< code string */
              bool is_direct, /**< is eval called directly (ECMA-262 v5, 15.1.2.1.1) */
              bool is_called_from_strict_mode_code) /**< is eval is called from strict mode code */
{
  ecma_value_t ret_value;

  lit_utf8_size_t chars_num = ecma_string_get_size (code_p);
  if (chars_num == 0)
  {
    ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  }
  else
  {
    ECMA_STRING_TO_UTF8_STRING (code_p, code_utf8_buffer_p, code_utf8_buffer_size);

    ret_value = ecma_op_eval_chars_buffer ((jerry_char_t *) code_utf8_buffer_p,
                                           chars_num,
                                           is_direct,
                                           is_called_from_strict_mode_code);

    ECMA_FINALIZE_UTF8_STRING (code_utf8_buffer_p, code_utf8_buffer_size);
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
 * @return ecma value
 */
ecma_value_t
ecma_op_eval_chars_buffer (const jerry_char_t *code_p, /**< code characters buffer */
                           size_t code_buffer_size, /**< size of the buffer */
                           bool is_direct, /**< is eval called directly (ECMA-262 v5, 15.1.2.1.1) */
                           bool is_called_from_strict_mode_code) /**< is eval is called from strict mode code */
{
  JERRY_ASSERT (code_p != NULL);

  ecma_value_t ret_value;

  ecma_compiled_code_t *bytecode_data_p;
  jsp_status_t parse_status;

  bool is_strict_call = (is_direct && is_called_from_strict_mode_code);
  jerry_object_t *error_obj_p = NULL;

  parse_status = parser_parse_eval (code_p,
                                    code_buffer_size,
                                    is_strict_call,
                                    &bytecode_data_p,
                                    &error_obj_p);

  if (parse_status == JSP_STATUS_OK)
  {
    ret_value = vm_run_eval (bytecode_data_p, is_direct);
  }
  else
  {
    JERRY_ASSERT (parse_status == JSP_STATUS_SYNTAX_ERROR);

    ret_value = ecma_make_error_obj_value (error_obj_p);
  }

  return ret_value;
} /* ecma_op_eval_chars_buffer */

/**
 * @}
 * @}
 */
