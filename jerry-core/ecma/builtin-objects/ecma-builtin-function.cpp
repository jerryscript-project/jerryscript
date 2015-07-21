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
#include "ecma-gc.h"
#include "ecma-function-object.h"
#include "ecma-lex-env.h"
#include "ecma-try-catch-macro.h"
#include "serializer.h"
#include "lit-magic-strings.h"
#include "parser.h"

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
 * @return completion-value
 */
ecma_completion_value_t
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
 * @return completion value - concatenated arguments as a string.
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_function_helper_get_arguments (const ecma_value_t *arguments_list_p, /** < arguments list */
                                            ecma_length_t arguments_list_len, /** < number of arguments */
                                            ecma_length_t *out_total_number_of_args_p) /** < out: number of
                                                                                        * arguments found */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);
  JERRY_ASSERT (out_total_number_of_args_p != NULL);

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* We are only processing the function arguments skipping the function body */
  ecma_length_t number_of_function_args = (arguments_list_len == 0 ? 0 : arguments_list_len - 1);
  ecma_string_t *arguments_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);

  ecma_string_t *separator_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_COMMA_CHAR);

  for (ecma_length_t idx = 0;
       idx < number_of_function_args && ecma_is_completion_value_empty (ret_value);
       idx++)
  {
    ECMA_TRY_CATCH (str_arg_value,
                    ecma_op_to_string (arguments_list_p[idx]),
                    ret_value);

    ecma_string_t *str_p = ecma_get_string_from_value (str_arg_value);

    lit_utf8_size_t str_size = ecma_string_get_size (str_p);
    MEM_DEFINE_LOCAL_ARRAY (start_p, str_size, lit_utf8_byte_t);

    ecma_string_to_utf8_string (str_p, start_p, (ssize_t) str_size);
    lit_utf8_iterator_t iter = lit_utf8_iterator_create (start_p, str_size);

    while (!lit_utf8_iterator_is_eos (&iter))
    {
      ecma_char_t current_char = lit_utf8_iterator_read_next (&iter);

      if (current_char == ',')
      {
        (*out_total_number_of_args_p)++;
      }
    }

    MEM_FINALIZE_LOCAL_ARRAY (start_p);

    ecma_string_t *concated_str_p = ecma_concat_ecma_strings (arguments_str_p, str_p);
    ecma_deref_ecma_string (arguments_str_p);
    arguments_str_p = concated_str_p;

    if (idx < number_of_function_args - 1)
    {
      ecma_string_t *concated_str_p = ecma_concat_ecma_strings (arguments_str_p, separator_string_p);
      ecma_deref_ecma_string (arguments_str_p);
      arguments_str_p = concated_str_p;
    }

    (*out_total_number_of_args_p)++;

    ECMA_FINALIZE (str_arg_value);
  }

  ecma_deref_ecma_string (separator_string_p);

  if (ecma_is_completion_value_empty (ret_value))
  {
    ret_value = ecma_make_normal_completion_value (ecma_make_string_value (arguments_str_p));
  }

  return ret_value;
} /* ecma_builtin_function_helper_get_arguments */

/**
 * Handle calling [[Construct]] of built-in Function object
 *
 * See also:
 *          ECMA-262 v5, 15.3.
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_function_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                          ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ecma_length_t total_number_of_function_args = 0;

  ECMA_TRY_CATCH (arguments_value,
                  ecma_builtin_function_helper_get_arguments (arguments_list_p,
                                                              arguments_list_len,
                                                              &total_number_of_function_args),
                  ret_value);

  ecma_string_t *arguments_str_p = ecma_get_string_from_value (arguments_value);

  /* Last string, if any, is the function's body, and the rest, if any - are the function's parameter names */
  MEM_DEFINE_LOCAL_ARRAY (string_params_p,
                          arguments_list_len == 0 ? 1 : total_number_of_function_args + 1,
                          ecma_string_t *);
  uint32_t params_count;

  size_t strings_buffer_size;

  if (arguments_list_len == 0)
  {
    /* 3. */
    string_params_p[0] = ecma_new_ecma_string_from_magic_string_id (LIT_MAGIC_STRING__EMPTY);
    strings_buffer_size = lit_get_magic_string_size (LIT_MAGIC_STRING__EMPTY);
    params_count = 1;
  }
  else
  {
    /* 4., 5., 6. */
    params_count = 0;

    lit_utf8_size_t str_size = ecma_string_get_size (arguments_str_p);
    strings_buffer_size = str_size;

    MEM_DEFINE_LOCAL_ARRAY (start_p, str_size, lit_utf8_byte_t);

    ecma_string_to_utf8_string (arguments_str_p, start_p, (ssize_t) str_size);
    lit_utf8_iterator_t iter = lit_utf8_iterator_create (start_p, str_size);
    ecma_length_t last_separator = lit_utf8_iterator_get_index (&iter);
    ecma_length_t end_position;

    while (!lit_utf8_iterator_is_eos (&iter))
    {
      ecma_char_t current_char = lit_utf8_iterator_read_next (&iter);

      if (current_char == ',')
      {
        lit_utf8_iterator_decr (&iter);
        end_position = lit_utf8_iterator_get_index (&iter);

        string_params_p[params_count] = ecma_string_substr (arguments_str_p, last_separator,  end_position);

        lit_utf8_iterator_incr (&iter);
        last_separator = lit_utf8_iterator_get_index (&iter);

        params_count++;
      }
    }

    end_position = lit_utf8_string_length (start_p, str_size);
    string_params_p[params_count] = ecma_string_substr (arguments_str_p, last_separator, end_position);
    params_count++;

    MEM_FINALIZE_LOCAL_ARRAY (start_p);

    ECMA_TRY_CATCH (str_arg_value,
                    ecma_op_to_string (arguments_list_p[arguments_list_len - 1]),
                    ret_value);

    ecma_string_t *str_p = ecma_get_string_from_value (str_arg_value);
    string_params_p[params_count] = ecma_copy_or_ref_ecma_string (str_p);
    strings_buffer_size += ecma_string_get_size (str_p);
    params_count++;

    ECMA_FINALIZE (str_arg_value);
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    JERRY_ASSERT (params_count >= 1);

    MEM_DEFINE_LOCAL_ARRAY (utf8_string_params_p,
                            params_count,
                            lit_utf8_byte_t *);
    MEM_DEFINE_LOCAL_ARRAY (utf8_string_params_size,
                            params_count,
                            size_t);
    MEM_DEFINE_LOCAL_ARRAY (utf8_string_buffer_p,
                            strings_buffer_size,
                            lit_utf8_byte_t);

    ssize_t utf8_string_buffer_pos = 0;
    for (uint32_t i = 0; i < params_count; i++)
    {
      ssize_t sz = ecma_string_to_utf8_string (string_params_p[i],
                                               &utf8_string_buffer_p[utf8_string_buffer_pos],
                                               (ssize_t) strings_buffer_size - utf8_string_buffer_pos);
      JERRY_ASSERT (sz >= 0);

      utf8_string_params_p[i] = utf8_string_buffer_p + utf8_string_buffer_pos;
      utf8_string_params_size[i] = (size_t) sz;

      utf8_string_buffer_pos += sz;
    }

    const vm_instr_t* instrs_p;
    jsp_status_t parse_status;

    parse_status = parser_parse_new_function ((const jerry_api_char_t **) utf8_string_params_p,
                                              utf8_string_params_size,
                                              params_count,
                                              &instrs_p);

    if (parse_status == JSP_STATUS_SYNTAX_ERROR)
    {
      ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_SYNTAX));
    }
    else if (parse_status == JSP_STATUS_REFERENCE_ERROR)
    {
      ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_REFERENCE));
    }
    else
    {
      JERRY_ASSERT (parse_status == JSP_STATUS_OK);
      bool is_strict = false;
      bool do_instantiate_arguments_object = true;

      opcode_scope_code_flags_t scope_flags = vm_get_scope_flags (instrs_p,
                                                                  0);

      if (scope_flags & OPCODE_SCOPE_CODE_FLAGS_STRICT)
      {
        is_strict = true;
      }

      if ((scope_flags & OPCODE_SCOPE_CODE_FLAGS_NOT_REF_ARGUMENTS_IDENTIFIER)
          && (scope_flags & OPCODE_SCOPE_CODE_FLAGS_NOT_REF_EVAL_IDENTIFIER))
      {
        /* the code doesn't use 'arguments' identifier
         * and doesn't perform direct call to eval,
         * so Arguments object can't be referenced */
        do_instantiate_arguments_object = false;
      }

      /* 11. */
      ecma_object_t *glob_lex_env_p = ecma_get_global_environment ();

      ecma_object_t *func_obj_p = ecma_op_create_function_object (params_count > 1u ? string_params_p : NULL,
                                                                  (ecma_length_t) (params_count - 1u),
                                                                  glob_lex_env_p,
                                                                  is_strict,
                                                                  do_instantiate_arguments_object,
                                                                  instrs_p,
                                                                  1);

      ecma_deref_object (glob_lex_env_p);

      ret_value = ecma_make_normal_completion_value (ecma_make_object_value (func_obj_p));
    }

    MEM_FINALIZE_LOCAL_ARRAY (utf8_string_buffer_p);
    MEM_FINALIZE_LOCAL_ARRAY (utf8_string_params_size);
    MEM_FINALIZE_LOCAL_ARRAY (utf8_string_params_p);
  }

  for (uint32_t i = 0; i < params_count; i++)
  {
    ecma_deref_ecma_string (string_params_p[i]);
  }

  MEM_FINALIZE_LOCAL_ARRAY (string_params_p);

  ECMA_FINALIZE (arguments_value);

  return ret_value;
} /* ecma_builtin_function_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */
