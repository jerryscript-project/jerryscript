/* Copyright JS Foundation and other contributors, http://js.foundation
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

#include "ecma-alloc.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-eval.h"
#include "ecma-gc.h"
#include "ecma-function-object.h"
#include "ecma-lex-env.h"
#include "ecma-try-catch-macro.h"
#include "js-parser.h"
#include "lit-magic-strings.h"

#if ENABLED (JERRY_LINE_INFO) || ENABLED (JERRY_ERROR_MESSAGES) || ENABLED (JERRY_ES2015_MODULE_SYSTEM)
#include "jcontext.h"
#endif /* ENABLED (JERRY_LINE_INFO) || ENABLED (JERRY_ERROR_MESSAGES) || ENABLED (JERRY_ES2015_MODULE_SYSTEM) */

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-function.inc.h"
#define BUILTIN_UNDERSCORED_ID function
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup function ECMA Function object built-in
 * @{
 */

/**
 * Handle calling [[Call]] of built-in Function object
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_function_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                     ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_builtin_function_dispatch_construct (arguments_list_p, arguments_list_len);
} /* ecma_builtin_function_dispatch_call */

/**
 * Helper method to count and convert the arguments for the Function constructor call.
 *
 * Performs the operation described in ECMA 262 v5.1 15.3.2.1 steps 5.a-d
 *
 *
 * @return ecma value - concatenated arguments as a string.
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_function_helper_get_function_arguments (const ecma_value_t *arguments_list_p, /**< arguments list */
                                                     ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  if (arguments_list_len <= 1)
  {
    return ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
  }

  ecma_string_t *final_str_p = ecma_op_to_string (arguments_list_p[0]);

  if (JERRY_UNLIKELY (final_str_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  if (arguments_list_len == 2)
  {
    return ecma_make_string_value (final_str_p);
  }

  for (ecma_length_t idx = 1; idx < arguments_list_len - 1; idx++)
  {
    ecma_string_t *new_str_p = ecma_op_to_string (arguments_list_p[idx]);

    if (JERRY_UNLIKELY (new_str_p == NULL))
    {
      ecma_deref_ecma_string (final_str_p);
      return ECMA_VALUE_ERROR;
    }

    final_str_p = ecma_append_magic_string_to_string (final_str_p,
                                                      LIT_MAGIC_STRING_COMMA_CHAR);

    final_str_p = ecma_concat_ecma_strings (final_str_p, new_str_p);
    ecma_deref_ecma_string (new_str_p);
  }

  return ecma_make_string_value (final_str_p);
} /* ecma_builtin_function_helper_get_function_arguments */

/**
 * Handle calling [[Construct]] of built-in Function object
 *
 * See also:
 *          ECMA-262 v5, 15.3.
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_function_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                          ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_value_t arguments_value = ecma_builtin_function_helper_get_function_arguments (arguments_list_p,
                                                                                      arguments_list_len);

  if (ECMA_IS_VALUE_ERROR (arguments_value))
  {
    return arguments_value;
  }

  ecma_string_t *function_body_str_p;

  if (arguments_list_len > 0)
  {
    function_body_str_p = ecma_op_to_string (arguments_list_p[arguments_list_len - 1]);

    if (JERRY_UNLIKELY (function_body_str_p == NULL))
    {
      ecma_free_value (arguments_value);
      return ECMA_VALUE_ERROR;
    }
  }
  else
  {
    /* Very unlikely code path, not optimized. */
    function_body_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
  }

  ecma_string_t *arguments_str_p = ecma_get_string_from_value (arguments_value);

  ECMA_STRING_TO_UTF8_STRING (arguments_str_p, arguments_buffer_p, arguments_buffer_size);
  ECMA_STRING_TO_UTF8_STRING (function_body_str_p, function_body_buffer_p, function_body_buffer_size);

#if ENABLED (JERRY_LINE_INFO) || ENABLED (JERRY_ERROR_MESSAGES) || ENABLED (JERRY_ES2015_MODULE_SYSTEM)
  JERRY_CONTEXT (resource_name) = ecma_make_magic_string_value (LIT_MAGIC_STRING_RESOURCE_ANON);
#endif /* ENABLED (JERRY_LINE_INFO) || ENABLED (JERRY_ERROR_MESSAGES) || ENABLED (JERRY_ES2015_MODULE_SYSTEM) */

  ecma_compiled_code_t *bytecode_data_p = NULL;

  ecma_value_t ret_value = parser_parse_script (arguments_buffer_p,
                                                arguments_buffer_size,
                                                function_body_buffer_p,
                                                function_body_buffer_size,
                                                ECMA_PARSE_NO_OPTS,
                                                &bytecode_data_p);

  if (!ECMA_IS_VALUE_ERROR (ret_value))
  {
    JERRY_ASSERT (ecma_is_value_true (ret_value));

    ecma_object_t *func_obj_p = ecma_op_create_function_object (ecma_get_global_environment (),
                                                                bytecode_data_p);

    ecma_bytecode_deref (bytecode_data_p);
    ret_value = ecma_make_object_value (func_obj_p);
  }

  ECMA_FINALIZE_UTF8_STRING (function_body_buffer_p, function_body_buffer_size);
  ECMA_FINALIZE_UTF8_STRING (arguments_buffer_p, arguments_buffer_size);

  ecma_deref_ecma_string (arguments_str_p);
  ecma_deref_ecma_string (function_body_str_p);

  return ret_value;
} /* ecma_builtin_function_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */
