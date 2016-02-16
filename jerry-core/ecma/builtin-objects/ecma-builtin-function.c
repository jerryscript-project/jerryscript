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

#include "ecma-alloc.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-eval.h"
#include "ecma-gc.h"
#include "ecma-function-object.h"
#include "ecma-lex-env.h"
#include "ecma-try-catch-macro.h"
#include "lit-magic-strings.h"
#include "js-parser.h"

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
ecma_builtin_function_helper_get_function_expression (const ecma_value_t *arguments_list_p, /**< arguments list */
                                                      ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ecma_string_t *left_parenthesis_str_p, *right_parenthesis_str_p;
  ecma_string_t *left_brace_str_p, *right_brace_str_p;
  ecma_string_t *comma_str_p;
  ecma_string_t *function_kw_str_p, *empty_str_p;
  ecma_string_t *expr_str_p, *concated_str_p;

  left_parenthesis_str_p = ecma_new_ecma_string_from_magic_string_id (LIT_MAGIC_STRING_LEFT_PARENTHESIS_CHAR);
  right_parenthesis_str_p = ecma_new_ecma_string_from_magic_string_id (LIT_MAGIC_STRING_RIGHT_PARENTHESIS_CHAR);

  left_brace_str_p = ecma_new_ecma_string_from_magic_string_id (LIT_MAGIC_STRING_LEFT_BRACE_CHAR);
  right_brace_str_p = ecma_new_ecma_string_from_magic_string_id (LIT_MAGIC_STRING_RIGHT_BRACE_CHAR);

  comma_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_COMMA_CHAR);

  function_kw_str_p = ecma_new_ecma_string_from_magic_string_id (LIT_MAGIC_STRING_FUNCTION);
  empty_str_p = ecma_new_ecma_string_from_magic_string_id (LIT_MAGIC_STRING__EMPTY);

  /* First, we only process the function arguments skipping the function body */
  ecma_length_t number_of_function_args = (arguments_list_len == 0 ? 0 : arguments_list_len - 1);

  expr_str_p = ecma_concat_ecma_strings (left_parenthesis_str_p, function_kw_str_p);

  concated_str_p = ecma_concat_ecma_strings (expr_str_p, left_parenthesis_str_p);
  ecma_deref_ecma_string (expr_str_p);
  expr_str_p = concated_str_p;

  for (ecma_length_t idx = 0;
       idx < number_of_function_args && ecma_is_value_empty (ret_value);
       idx++)
  {
    ECMA_TRY_CATCH (str_arg_value,
                    ecma_op_to_string (arguments_list_p[idx]),
                    ret_value);

    ecma_string_t *str_p = ecma_get_string_from_value (str_arg_value);

    concated_str_p = ecma_concat_ecma_strings (expr_str_p, str_p);
    ecma_deref_ecma_string (expr_str_p);
    expr_str_p = concated_str_p;

    if (idx < number_of_function_args - 1)
    {
      concated_str_p = ecma_concat_ecma_strings (expr_str_p, comma_str_p);
      ecma_deref_ecma_string (expr_str_p);
      expr_str_p = concated_str_p;
    }

    ECMA_FINALIZE (str_arg_value);
  }

  if (ecma_is_value_empty (ret_value))
  {
    concated_str_p = ecma_concat_ecma_strings (expr_str_p, right_parenthesis_str_p);
    ecma_deref_ecma_string (expr_str_p);
    expr_str_p = concated_str_p;

    concated_str_p = ecma_concat_ecma_strings (expr_str_p, left_brace_str_p);
    ecma_deref_ecma_string (expr_str_p);
    expr_str_p = concated_str_p;

    if (arguments_list_len != 0)
    {
      ECMA_TRY_CATCH (str_arg_value,
                      ecma_op_to_string (arguments_list_p[arguments_list_len - 1]),
                      ret_value);

      ecma_string_t *body_str_p = ecma_get_string_from_value (str_arg_value);

      concated_str_p = ecma_concat_ecma_strings (expr_str_p, body_str_p);
      ecma_deref_ecma_string (expr_str_p);
      expr_str_p = concated_str_p;

      ECMA_FINALIZE (str_arg_value);
    }

    concated_str_p = ecma_concat_ecma_strings (expr_str_p, right_brace_str_p);
    ecma_deref_ecma_string (expr_str_p);
    expr_str_p = concated_str_p;

    concated_str_p = ecma_concat_ecma_strings (expr_str_p, right_parenthesis_str_p);
    ecma_deref_ecma_string (expr_str_p);
    expr_str_p = concated_str_p;
  }

  ecma_deref_ecma_string (left_parenthesis_str_p);
  ecma_deref_ecma_string (right_parenthesis_str_p);
  ecma_deref_ecma_string (left_brace_str_p);
  ecma_deref_ecma_string (right_brace_str_p);
  ecma_deref_ecma_string (comma_str_p);
  ecma_deref_ecma_string (function_kw_str_p);
  ecma_deref_ecma_string (empty_str_p);

  if (ecma_is_value_empty (ret_value))
  {
    ret_value = ecma_make_string_value (expr_str_p);
  }
  else
  {
    ecma_deref_ecma_string (expr_str_p);
  }

  return ret_value;
} /* ecma_builtin_function_helper_get_function_expression */

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

  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_TRY_CATCH (arguments_value,
                  ecma_builtin_function_helper_get_function_expression (arguments_list_p,
                                                                        arguments_list_len),
                  ret_value);

  ecma_string_t *function_expression_p = ecma_get_string_from_value (arguments_value);

  ret_value = ecma_op_eval (function_expression_p, false, false);

  ECMA_FINALIZE (arguments_value);

  return ret_value;
} /* ecma_builtin_function_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */
