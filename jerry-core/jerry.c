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

#include <stdio.h>

#include "ecma-alloc.h"
#include "ecma-array-object.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-eval.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-init-finalize.h"
#include "ecma-lex-env.h"
#include "ecma-literal-storage.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"
#include "jcontext.h"
#include "jerry-api.h"
#include "js-parser.h"
#include "re-compiler.h"

#define JERRY_INTERNAL
#include "jerry-internal.h"

JERRY_STATIC_ASSERT (sizeof (jerry_value_t) == sizeof (ecma_value_t),
                     size_of_jerry_value_t_must_be_equal_to_size_of_ecma_value_t);

JERRY_STATIC_ASSERT ((int) ECMA_ERROR_COMMON == (int) JERRY_ERROR_COMMON
                     && (int) ECMA_ERROR_EVAL == (int) JERRY_ERROR_EVAL
                     && (int) ECMA_ERROR_RANGE == (int) JERRY_ERROR_RANGE
                     && (int) ECMA_ERROR_REFERENCE == (int) JERRY_ERROR_REFERENCE
                     && (int) ECMA_ERROR_SYNTAX == (int) JERRY_ERROR_SYNTAX
                     && (int) ECMA_ERROR_TYPE == (int) JERRY_ERROR_TYPE
                     && (int) ECMA_ERROR_URI == (int) JERRY_ERROR_URI,
                     ecma_standard_error_t_must_be_equal_to_jerry_error_t);

#ifdef JERRY_ENABLE_ERROR_MESSAGES

/**
 * Error message, if an argument is has an error flag
 */
static const char * const error_value_msg_p = "argument cannot have an error flag";

/**
 * Error message, if types of arguments are incorrect
 */
static const char * const wrong_args_msg_p = "wrong type of argument";

#endif /* JERRY_ENABLE_ERROR_MESSAGES */

/** \addtogroup jerry Jerry engine interface
 * @{
 */

/**
 * Assert that it is correct to call API in current state.
 *
 * Note:
 *         By convention, there can be some states when API could not be invoked.
 *
 *         While, API can be invoked jerry_api_available flag is set,
 *         and while it is incorrect to invoke API - it is not set.
 *
 *         This procedure checks whether the API is available, and terminates
 *         the engine if it is unavailable. Otherwise it is a no-op.
 *
 * Note:
 *         The API could not be invoked in the following cases:
 *           - before jerry_init and after jerry_cleanup
 *           - between enter to and return from a native free callback
 */
static inline void __attr_always_inline___
jerry_assert_api_available (void)
{
  if (unlikely (!JERRY_CONTEXT (jerry_api_available)))
  {
    /* Terminates the execution. */
    JERRY_UNREACHABLE ();
  }
} /* jerry_assert_api_available */

/**
 * Turn on API availability
 */
static inline void __attr_always_inline___
jerry_make_api_available (void)
{
  JERRY_CONTEXT (jerry_api_available) = true;
} /* jerry_make_api_available */

/**
 * Turn off API availability
 */
static inline void __attr_always_inline___
jerry_make_api_unavailable (void)
{
  JERRY_CONTEXT (jerry_api_available) = false;
} /* jerry_make_api_unavailable */

/**
 * Construct new TypeError object
 *
 * @return TypeError object value
 */
static jerry_value_t
jerry_create_type_error (void)
{
  ecma_object_t *type_error_obj_p = ecma_new_standard_error (ECMA_ERROR_TYPE);
  return ecma_make_error_obj_value (type_error_obj_p);
} /* jerry_create_type_error */

/**
 * Jerry engine initialization
 */
void
jerry_init (jerry_init_flag_t flags) /**< combination of Jerry flags */
{
  if (unlikely (JERRY_CONTEXT (jerry_api_available)))
  {
    /* This function cannot be called twice unless jerry_cleanup is called. */
    JERRY_UNREACHABLE ();
  }

  /* Zero out all members. */
  memset (&JERRY_CONTEXT (JERRY_CONTEXT_FIRST_MEMBER), 0, sizeof (jerry_context_t));

  if (flags & JERRY_INIT_MEM_STATS_SEPARATE)
  {
    flags |= JERRY_INIT_MEM_STATS;
  }

  JERRY_CONTEXT (jerry_init_flags) = flags;

  jerry_make_api_available ();

  jmem_init ();
  ecma_init ();
} /* jerry_init */

/**
 * Terminate Jerry engine
 */
void
jerry_cleanup (void)
{
  jerry_assert_api_available ();

  ecma_finalize ();
  jmem_finalize ();
  jerry_make_api_unavailable ();
} /* jerry_cleanup */

/**
 * Register external magic string array
 */
void
jerry_register_magic_strings (const jerry_char_ptr_t *ex_str_items_p, /**< character arrays, representing
                                                                       *   external magic strings' contents */
                              uint32_t count, /**< number of the strings */
                              const jerry_length_t *str_lengths_p) /**< lengths of all strings */
{
  jerry_assert_api_available ();

  lit_magic_strings_ex_set ((const lit_utf8_byte_t **) ex_str_items_p, count, (const lit_utf8_size_t *) str_lengths_p);
} /* jerry_register_magic_strings */

/**
 * Get Jerry configured memory limits
 */
void
jerry_get_memory_limits (size_t *out_data_bss_brk_limit_p, /**< [out] Jerry's maximum usage of
                                                            *         data + bss + brk sections */
                         size_t *out_stack_limit_p) /**< [out] Jerry's maximum usage of stack */
{
  *out_data_bss_brk_limit_p = CONFIG_MEM_HEAP_AREA_SIZE + CONFIG_MEM_DATA_LIMIT_MINUS_HEAP_SIZE;
  *out_stack_limit_p = CONFIG_MEM_STACK_LIMIT;
} /* jerry_get_memory_limits */

/**
 * Run garbage collection
 */
void
jerry_gc (void)
{
  jerry_assert_api_available ();

  ecma_gc_run (JMEM_FREE_UNUSED_MEMORY_SEVERITY_LOW);
} /* jerry_gc */

/**
 * Simple Jerry runner
 *
 * @return true  - if run was successful
 *         false - otherwise
 */
bool
jerry_run_simple (const jerry_char_t *script_source_p, /**< script source */
                  size_t script_source_size, /**< script source size */
                  jerry_init_flag_t flags) /**< combination of Jerry flags */
{
  bool result = false;

  jerry_init (flags);

  jerry_value_t parse_ret_val = jerry_parse (script_source_p, script_source_size, false);

  if (!ECMA_IS_VALUE_ERROR (parse_ret_val))
  {
    jerry_value_t run_ret_val = jerry_run (parse_ret_val);

    if (!ECMA_IS_VALUE_ERROR (run_ret_val))
    {
      result = true;
    }

    jerry_release_value (run_ret_val);
  }

  jerry_release_value (parse_ret_val);
  jerry_cleanup ();

  return result;
} /* jerry_run_simple */

/**
 * Parse script and construct an EcmaScript function. The lexical
 * environment is set to the global lexical environment.
 *
 * @return function object value - if script was parsed successfully,
 *         thrown error - otherwise
 */
jerry_value_t
jerry_parse (const jerry_char_t *source_p, /**< script source */
             size_t source_size, /**< script source size */
             bool is_strict) /**< strict mode */
{
  jerry_assert_api_available ();

  ecma_compiled_code_t *bytecode_data_p;
  ecma_value_t parse_status;

  parse_status = parser_parse_script (source_p,
                                      source_size,
                                      is_strict,
                                      &bytecode_data_p);

  if (ECMA_IS_VALUE_ERROR (parse_status))
  {
    return parse_status;
  }

  ecma_free_value (parse_status);

#ifdef JMEM_STATS
  if (JERRY_CONTEXT (jerry_init_flags) & JERRY_INIT_MEM_STATS_SEPARATE)
  {
    jmem_stats_print ();
    jmem_stats_reset_peak ();
  }
#endif /* JMEM_STATS */

  is_strict = ((bytecode_data_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE) != 0);
  ecma_object_t *lex_env_p = ecma_get_global_environment ();
  ecma_object_t *func_obj_p = ecma_op_create_function_object (lex_env_p,
                                                              is_strict,
                                                              bytecode_data_p);
  ecma_bytecode_deref (bytecode_data_p);

  return ecma_make_object_value (func_obj_p);
} /* jerry_parse */

/**
 * Run an EcmaScript function created by jerry_parse.
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return result of bytecode - if run was successful
 *         thrown error - otherwise
 */
jerry_value_t
jerry_run (const jerry_value_t func_val) /**< function to run */
{
  jerry_assert_api_available ();

  if (!ecma_is_value_object (func_val))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p));
  }

  ecma_object_t *func_obj_p = ecma_get_object_from_value (func_val);

  if (ecma_get_object_type (func_obj_p) != ECMA_OBJECT_TYPE_FUNCTION
      || ecma_get_object_is_builtin (func_obj_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p));
  }

  ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) func_obj_p;

  ecma_object_t *scope_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                            ext_func_p->u.function.scope_cp);

  if (scope_p != ecma_get_global_environment ())
  {
    return ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p));
  }

  const ecma_compiled_code_t *bytecode_data_p;
  bytecode_data_p = ECMA_GET_INTERNAL_VALUE_POINTER (const ecma_compiled_code_t,
                                                     ext_func_p->u.function.bytecode_cp);

  return vm_run_global (bytecode_data_p);
} /* jerry_run */

/**
 * Perform eval
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return result of eval, may be error value.
 */
jerry_value_t
jerry_eval (const jerry_char_t *source_p, /**< source code */
            size_t source_size, /**< length of source code */
            bool is_strict) /**< source must conform with strict mode */
{
  jerry_assert_api_available ();

  return ecma_op_eval_chars_buffer ((const lit_utf8_byte_t *) source_p,
                                    source_size,
                                    false,
                                    is_strict);
} /* jerry_eval */

/**
 * Get global object
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return api value of global object
 */
jerry_value_t
jerry_get_global_object (void)
{
  jerry_assert_api_available ();

  return ecma_make_object_value (ecma_builtin_get (ECMA_BUILTIN_ID_GLOBAL));
} /* jerry_get_global_object */

/**
 * Check if the specified value is an array object value.
 *
 * @return true  - if the specified value is an array object,
 *         false - otherwise.
 */
bool
jerry_value_is_array (const jerry_value_t value) /**< jerry api value */
{
  jerry_assert_api_available ();

  return (ecma_is_value_object (value)
          && ecma_get_object_type (ecma_get_object_from_value (value)) == ECMA_OBJECT_TYPE_ARRAY);
} /* jerry_value_is_array */

/**
 * Check if the specified value is boolean.
 *
 * @return true  - if the specified value is boolean,
 *         false - otherwise.
 */
bool
jerry_value_is_boolean (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();

  return ecma_is_value_boolean (value);
} /* jerry_value_is_boolean */

/**
 * Check if the specified value is a constructor function object value.
 *
 * @return true - if the specified value is a function value that implements [[Construct]],
 *         false - otherwise.
 */
bool
jerry_value_is_constructor (const jerry_value_t value) /**< jerry api value */
{
  jerry_assert_api_available ();

  return ecma_is_constructor (value);
} /* jerry_value_is_constructor */

/**
 * Check if the specified value is a function object value.
 *
 * @return true - if the specified value is callable,
 *         false - otherwise.
 */
bool
jerry_value_is_function (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();

  return ecma_op_is_callable (value);
} /* jerry_value_is_function */

/**
 * Check if the specified value is number.
 *
 * @return true  - if the specified value is number,
 *         false - otherwise.
 */
bool
jerry_value_is_number (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();

  return ecma_is_value_number (value);
} /* jerry_value_is_number */

/**
 * Check if the specified value is null.
 *
 * @return true  - if the specified value is null,
 *         false - otherwise.
 */
bool
jerry_value_is_null (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();

  return ecma_is_value_null (value);
} /* jerry_value_is_null */

/**
 * Check if the specified value is object.
 *
 * @return true  - if the specified value is object,
 *         false - otherwise.
 */
bool
jerry_value_is_object (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();

  return ecma_is_value_object (value);
} /* jerry_value_is_object */

/**
 * Check if the specified value is string.
 *
 * @return true  - if the specified value is string,
 *         false - otherwise.
 */
bool
jerry_value_is_string (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();

  return ecma_is_value_string (value);
} /* jerry_value_is_string */

/**
 * Check if the specified value is undefined.
 *
 * @return true  - if the specified value is undefined,
 *         false - otherwise.
 */
bool
jerry_value_is_undefined (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();

  return ecma_is_value_undefined (value);
} /* jerry_value_is_undefined */

/**
 * Check if the specified feature is enabled.
 *
 * @return true  - if the specified feature is enabled,
 *         false - otherwise.
 */
bool jerry_is_feature_enabled (const jerry_feature_t feature)
{
  JERRY_ASSERT (feature < JERRY_FEATURE__COUNT);

  return (false
#ifdef JERRY_ENABLE_ERROR_MESSAGES
          || feature == JERRY_FEATURE_ERROR_MESSAGES
#endif /* JERRY_ENABLE_ERROR_MESSAGES */
#ifdef JERRY_CPOINTER_32_BIT
          || feature == JERRY_FEATURE_CPOINTER_32_BIT
#endif /* JERRY_CPOINTER_32_BIT */
#ifdef JMEM_STATS
          || feature == JERRY_FEATURE_MEM_STATS
#endif /* JMEM_STATS */
#ifdef PARSER_DUMP_BYTE_CODE
          || feature == JERRY_FEATURE_PARSER_DUMP
#endif /* PARSER_DUMP_BYTE_CODE */
#ifdef REGEXP_DUMP_BYTE_CODE
          || feature == JERRY_FEATURE_REGEXP_DUMP
#endif /* REGEXP_DUMP_BYTE_CODE */
#ifdef JERRY_ENABLE_SNAPSHOT_SAVE
          || feature == JERRY_FEATURE_SNAPSHOT_SAVE
#endif /* JERRY_ENABLE_SNAPSHOT_SAVE */
#ifdef JERRY_ENABLE_SNAPSHOT_EXEC
          || feature == JERRY_FEATURE_SNAPSHOT_EXEC
#endif /* JERRY_ENABLE_SNAPSHOT_EXEC */
          );
} /* jerry_is_feature_enabled */

/**
 * Check if the specified value is an error value.
 *
 * @return true  - if the error flag of the specified value is true,
 *         false - otherwise.
 */
bool
jerry_value_has_error_flag (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();

  return ECMA_IS_VALUE_ERROR (value);
} /* jerry_value_has_error_flag */

/**
 * Clear the error flag
 */
void
jerry_value_clear_error_flag (jerry_value_t *value_p)
{
  jerry_assert_api_available ();

  *value_p = (*value_p) & ~ECMA_VALUE_ERROR_FLAG;
} /* jerry_value_clear_error_flag */

/**
 * Set the error flag.
 */
void
jerry_value_set_error_flag (jerry_value_t *value_p)
{
  jerry_assert_api_available ();

  *value_p = (*value_p) | ECMA_VALUE_ERROR_FLAG;
} /* jerry_value_set_error_flag */

/**
 * Get boolean from the specified value.
 *
 * @return true or false.
 */
bool
jerry_get_boolean_value (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();

  if (!jerry_value_is_boolean (value))
  {
    return false;
  }

  return ecma_is_value_true (value);
} /* jerry_get_boolean_value */

/**
 * Get number from the specified value as a double.
 *
 * @return stored number as double
 */
double
jerry_get_number_value (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();

  if (!jerry_value_is_number (value))
  {
    return 0;
  }

  return (double) ecma_get_number_from_value (value);
} /* jerry_get_number_value */

/**
 * Call ToBoolean operation on the api value.
 *
 * @return true  - if the logical value is true
 *         false - otherwise
 */
bool
jerry_value_to_boolean (const jerry_value_t value) /**< input value */
{
  jerry_assert_api_available ();

  if (ECMA_IS_VALUE_ERROR (value))
  {
    return false;
  }

  return ecma_op_to_boolean (value);
} /* jerry_value_to_boolean */

/**
 * Call ToNumber operation on the api value.
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return converted number value - if success
 *         thrown error - otherwise
 */
jerry_value_t
jerry_value_to_number (const jerry_value_t value) /**< input value */
{
  jerry_assert_api_available ();

  if (ECMA_IS_VALUE_ERROR (value))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG (error_value_msg_p));
  }

  return ecma_op_to_number (value);
} /* jerry_value_to_number */

/**
 * Call ToObject operation on the api value.
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return converted object value - if success
 *         thrown error - otherwise
 */
jerry_value_t
jerry_value_to_object (const jerry_value_t value) /**< input value */
{
  jerry_assert_api_available ();

  if (ECMA_IS_VALUE_ERROR (value))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG (error_value_msg_p));
  }

  return ecma_op_to_object (value);
} /* jerry_value_to_object */

/**
 * Call ToPrimitive operation on the api value.
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return converted primitive value - if success
 *         thrown error - otherwise
 */
jerry_value_t
jerry_value_to_primitive (const jerry_value_t value) /**< input value */
{
  jerry_assert_api_available ();

  if (ECMA_IS_VALUE_ERROR (value))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG (error_value_msg_p));
  }

  return ecma_op_to_primitive (value, ECMA_PREFERRED_TYPE_NO);
} /* jerry_value_to_primitive */

/**
 * Call the ToString ecma builtin operation on the api value.
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return converted string value - if success
 *         thrown error - otherwise
 */
jerry_value_t
jerry_value_to_string (const jerry_value_t value) /**< input value */
{

  jerry_assert_api_available ();

  if (ECMA_IS_VALUE_ERROR (value))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG (error_value_msg_p));
  }

  return ecma_op_to_string (value);
} /* jerry_value_to_string */

/**
 * Acquire specified Jerry API value.
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return acquired api value
 */
jerry_value_t
jerry_acquire_value (jerry_value_t value) /**< API value */
{
  jerry_assert_api_available ();

  return ecma_copy_value (value);
} /* jerry_acquire_value */

/**
 * Release specified Jerry API value
 */
void
jerry_release_value (jerry_value_t value) /**< API value */
{
  jerry_assert_api_available ();

  ecma_free_value (value);
} /* jerry_release_value */

/**
 * Create an array object value
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return value of the constructed array object
 */
jerry_value_t
jerry_create_array (uint32_t size) /**< size of array */
{
  jerry_assert_api_available ();

  ecma_value_t array_length = ecma_make_uint32_value (size);

  jerry_length_t argument_size = 1;
  ecma_value_t array_value = ecma_op_create_array_object (&array_length, argument_size, true);
  ecma_free_value (array_length);

  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (array_value));

  return array_value;
} /* jerry_create_array */

/**
 * Create a jerry_value_t representing a boolean value from the given boolean parameter.
 *
 * @return value of the created boolean
 */
jerry_value_t
jerry_create_boolean (bool value) /**< bool value from which a jerry_value_t will be created */
{
  jerry_assert_api_available ();

  return ecma_make_boolean_value (value);
} /* jerry_create_boolean */

/**
 * Create an error object
 *
 * Note:
 *      - returned value must be freed with jerry_release_value, when it is no longer needed
 *      - the error flag is set for the returned value
 *
 * @return value of the constructed error object
 */
jerry_value_t
jerry_create_error (jerry_error_t error_type, /**< type of error */
                    const jerry_char_t *message_p) /**< value of 'message' property
                                                    *   of constructed error object */
{
  return jerry_create_error_sz (error_type,
                                (lit_utf8_byte_t *) message_p,
                                lit_zt_utf8_string_size (message_p));
} /* jerry_create_error */

/**
 * Create an error object
 *
 * Note:
 *      - returned value must be freed with jerry_release_value, when it is no longer needed
 *      - the error flag is set for the returned value
 *
 * @return value of the constructed error object
 */
jerry_value_t
jerry_create_error_sz (jerry_error_t error_type, /**< type of error */
                       const jerry_char_t *message_p, /**< value of 'message' property
                                                       *   of constructed error object */
                       jerry_size_t message_size) /**< size of the message in bytes */
{
  jerry_assert_api_available ();

  if (message_p == NULL || message_size == 0)
  {
    return ecma_make_error_obj_value (ecma_new_standard_error ((ecma_standard_error_t) error_type));
  }
  else
  {
    ecma_string_t *message_string_p = ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) message_p,
                                                                      (lit_utf8_size_t) message_size);

    ecma_object_t *error_object_p = ecma_new_standard_error_with_message ((ecma_standard_error_t) error_type,
                                                                          message_string_p);

    ecma_deref_ecma_string (message_string_p);

    return ecma_make_error_obj_value (error_object_p);
  }
} /* jerry_create_error_sz */

/**
 * Create an external function object
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return value of the constructed function object
 */
jerry_value_t
jerry_create_external_function (jerry_external_handler_t handler_p) /**< pointer to native handler
                                                                     *   for the function */
{
  jerry_assert_api_available ();

  ecma_object_t *func_obj_p = ecma_op_create_external_function_object ((ecma_external_pointer_t) handler_p);
  return ecma_make_object_value (func_obj_p);
} /* jerry_create_external_function */

/**
 * Creates a jerry_value_t representing a number value.
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return jerry_value_t created from the given double argument.
 */
jerry_value_t
jerry_create_number (double value) /**< double value from which a jerry_value_t will be created */
{
  jerry_assert_api_available ();

  return ecma_make_number_value ((ecma_number_t) value);
} /* jerry_create_number */

/**
 * Creates a jerry_value_t representing a positive or negative infinity value.
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return jerry_value_t representing an infinity value.
 */
jerry_value_t
jerry_create_number_infinity (bool sign) /**< true for negative Infinity
                                          *   false for positive Infinity */
{
  jerry_assert_api_available ();

  return ecma_make_number_value (ecma_number_make_infinity (sign));
} /* jerry_create_number_infinity */

/**
 * Creates a jerry_value_t representing a not-a-number value.
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return jerry_value_t representing a not-a-number value.
 */
jerry_value_t
jerry_create_number_nan (void)
{
  jerry_assert_api_available ();

  return ecma_make_nan_value ();
} /* jerry_create_number_nan */

/**
 * Creates and returns a jerry_value_t with type null object.
 *
 * @return jerry_value_t representing null
 */
jerry_value_t
jerry_create_null (void)
{
  jerry_assert_api_available ();

  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_NULL);
} /* jerry_create_null */

/**
 * Create new JavaScript object, like with new Object().
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return value of the created object
 */
jerry_value_t
jerry_create_object (void)
{
  jerry_assert_api_available ();

  return ecma_make_object_value (ecma_op_create_object_object_noarg ());
} /* jerry_create_object */

/**
 * Create string from a valid UTF8 string
 *
 * Note:
 *      returned value must be freed with jerry_release_value when it is no longer needed.
 *
 * @return value of the created string
 */
jerry_value_t
jerry_create_string_from_utf8 (const jerry_char_t *str_p) /**< pointer to string */
{
  return jerry_create_string_sz_from_utf8 (str_p, lit_zt_utf8_string_size ((lit_utf8_byte_t *) str_p));
} /* jerry_create_string_from_utf8 */

/**
 * Create string from a valid UTF8 string
 *
 * Note:
 *      returned value must be freed with jerry_release_value when it is no longer needed.
 *
 * @return value of the created string
 */
jerry_value_t
jerry_create_string_sz_from_utf8 (const jerry_char_t *str_p, /**< pointer to string */
                                  jerry_size_t str_size) /**< string size */
{
  jerry_assert_api_available ();

  ecma_string_t *ecma_str_p = ecma_new_ecma_string_from_utf8_converted_to_cesu8 ((lit_utf8_byte_t *) str_p,
                                                                                 (lit_utf8_size_t) str_size);

  return ecma_make_string_value (ecma_str_p);
} /* jerry_create_string_sz_from_utf8 */

/**
 * Create string from a valid CESU8 string
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return value of the created string
 */
jerry_value_t
jerry_create_string (const jerry_char_t *str_p) /**< pointer to string */
{
  return jerry_create_string_sz (str_p, lit_zt_utf8_string_size ((lit_utf8_byte_t *) str_p));
} /* jerry_create_string */

/**
 * Create string from a valid CESU8 string
 *
 * Note:
 *      returned value must be freed with jerry_release_value when it is no longer needed.
 *
 * @return value of the created string
 */
jerry_value_t
jerry_create_string_sz (const jerry_char_t *str_p, /**< pointer to string */
                        jerry_size_t str_size) /**< string size */
{
  jerry_assert_api_available ();

  ecma_string_t *ecma_str_p = ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) str_p,
                                                              (lit_utf8_size_t) str_size);
  return ecma_make_string_value (ecma_str_p);
} /* jerry_create_string_sz */

/**
 * Creates a jerry_value_t representing an undefined value.
 *
 * @return value of undefined
 */
jerry_value_t
jerry_create_undefined (void)
{
  jerry_assert_api_available ();

  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
} /* jerry_create_undefined */

/**
 * Get length of an array object
 *
 * Note:
 *      Returns 0, if the value parameter is not an array object.
 *
 * @return length of the given array
 */
uint32_t
jerry_get_array_length (const jerry_value_t value)
{
  jerry_assert_api_available ();

  if (!jerry_value_is_array (value))
  {
    return 0;
  }

  jerry_length_t length = 0;
  ecma_string_t magic_string_length;
  ecma_init_ecma_length_string (&magic_string_length);

  ecma_value_t len_value = ecma_op_object_get (ecma_get_object_from_value (value),
                                               &magic_string_length);

  length = ecma_number_to_uint32 (ecma_get_number_from_value (len_value));
  ecma_free_value (len_value);

  return length;
} /* jerry_get_array_length */

/**
 * Get size of Jerry string
 *
 * Note:
 *      Returns 0, if the value parameter is not a string.
 *
 * @return number of bytes in the buffer needed to represent the string
 */
jerry_size_t
jerry_get_string_size (const jerry_value_t value) /**< input string */
{
  jerry_assert_api_available ();

  if (!ecma_is_value_string (value))
  {
    return 0;
  }

  return ecma_string_get_size (ecma_get_string_from_value (value));
} /* jerry_get_string_size */

/**
 * Get UTF-8 encoded string size from Jerry string
 *
 * Note:
 *      Returns 0, if the value parameter is not a string.
 *
 * @return number of bytes in the buffer needed to represent the UTF-8 encoded string
 */
jerry_size_t
jerry_get_utf8_string_size (const jerry_value_t value)
{
  jerry_assert_api_available ();

  if (!ecma_is_value_string (value))
  {
    return 0;
  }

  return ecma_string_get_utf8_size (ecma_get_string_from_value (value));
} /* jerry_get_utf8_string_size */

/**
 * Get length of Jerry string
 *
 * Note:
 *      Returns 0, if the value parameter is not a string.
 *
 * @return number of characters in the string
 */
jerry_length_t
jerry_get_string_length (const jerry_value_t value) /**< input string */
{
  jerry_assert_api_available ();

  if (!ecma_is_value_string (value))
  {
    return 0;
  }

  return ecma_string_get_length (ecma_get_string_from_value (value));
} /* jerry_get_string_length */

/**
 * Get UTF-8 string length from Jerry string
 *
 * Note:
 *      Returns 0, if the value parameter is not a string.
 *
 * @return number of characters in the string
 */
jerry_length_t
jerry_get_utf8_string_length (const jerry_value_t value) /**< input string */
{
  jerry_assert_api_available ();

  if (!ecma_is_value_string (value))
  {
    return 0;
  }

  return ecma_string_get_utf8_length (ecma_get_string_from_value (value));
} /* jerry_get_utf8_string_length */

/**
 * Copy the characters of a string into a specified buffer.
 *
 * Note:
 *      The '\0' character could occur in character buffer.
 *      Returns 0, if the value parameter is not a string or
 *      the buffer is not large enough for the whole string.
 *
 * @return number of bytes, actually copied to the buffer.
 */
jerry_size_t
jerry_string_to_char_buffer (const jerry_value_t value, /**< input string value */
                             jerry_char_t *buffer_p, /**< [out] output characters buffer */
                             jerry_size_t buffer_size) /**< size of output buffer */
{
  jerry_assert_api_available ();

  if (!ecma_is_value_string (value) || buffer_p == NULL)
  {
    return 0;
  }

  ecma_string_t *str_p = ecma_get_string_from_value (value);

  if (ecma_string_get_size (str_p) > buffer_size)
  {
    return 0;
  }

  return ecma_string_copy_to_utf8_buffer (str_p,
                                          (lit_utf8_byte_t *) buffer_p,
                                          buffer_size);
} /* jerry_string_to_char_buffer */

/**
 * Checks whether the object or it's prototype objects have the given property.
 *
 * @return true  - if the property exists
 *         false - otherwise
 */
bool
jerry_has_property (const jerry_value_t obj_val, /**< object value */
                    const jerry_value_t prop_name_val) /**< property name (string value) */
{
  jerry_assert_api_available ();

  if (!ecma_is_value_object (obj_val)
      || !ecma_is_value_string (prop_name_val))
  {
    return false;
  }

  bool has_property = ecma_op_object_has_property (ecma_get_object_from_value (obj_val),
                                                   ecma_get_string_from_value (prop_name_val));

  return ecma_make_boolean_value (has_property);
} /* jerry_has_property */

/**
 * Checks whether the object has the given property.
 *
 * @return true  - if the property exists
 *         false - otherwise
 */
bool
jerry_has_own_property (const jerry_value_t obj_val, /**< object value */
                        const jerry_value_t prop_name_val) /**< property name (string value) */
{
  jerry_assert_api_available ();

  if (!ecma_is_value_object (obj_val)
      || !ecma_is_value_string (prop_name_val))
  {
    return false;
  }

  bool has_property = ecma_op_object_has_own_property (ecma_get_object_from_value (obj_val),
                                                       ecma_get_string_from_value (prop_name_val));

  return ecma_make_boolean_value (has_property);
} /* jerry_has_own_property */

/**
 * Delete a property from an object.
 *
 * @return true  - if property was deleted successfully
 *         false - otherwise
 */
bool
jerry_delete_property (const jerry_value_t obj_val, /**< object value */
                       const jerry_value_t prop_name_val) /**< property name (string value) */
{
  jerry_assert_api_available ();

  if (!ecma_is_value_object (obj_val)
      || !ecma_is_value_string (prop_name_val))
  {
    return false;
  }

  ecma_value_t ret_value = ecma_op_object_delete (ecma_get_object_from_value (obj_val),
                                                  ecma_get_string_from_value (prop_name_val),
                                                  false);
  return ecma_is_value_true (ret_value);
} /* jerry_delete_property */

/**
 * Get value of a property to the specified object with the given name.
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return value of the property - if success
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_get_property (const jerry_value_t obj_val, /**< object value */
                    const jerry_value_t prop_name_val) /**< property name (string value) */
{
  jerry_assert_api_available ();

  if (!ecma_is_value_object (obj_val)
      || !ecma_is_value_string (prop_name_val))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p));
  }

  return ecma_op_object_get (ecma_get_object_from_value (obj_val),
                             ecma_get_string_from_value (prop_name_val));
} /* jerry_get_property */

/**
 * Get value by an index from the specified object.
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return value of the property specified by the index - if success
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_get_property_by_index (const jerry_value_t obj_val, /**< object value */
                             uint32_t index) /**< index to be written */
{
  jerry_assert_api_available ();

  if (!ecma_is_value_object (obj_val))
  {
    return jerry_create_type_error ();
  }

  ecma_string_t str_idx;
  ecma_init_ecma_string_from_uint32 (&str_idx, index);
  ecma_value_t ret_value = ecma_op_object_get (ecma_get_object_from_value (obj_val), &str_idx);

  return ret_value;
} /* jerry_get_property_by_index */

/**
 * Set a property to the specified object with the given name.
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return true value - if the operation was successful
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_set_property (const jerry_value_t obj_val, /**< object value */
                    const jerry_value_t prop_name_val, /**< property name (string value) */
                    const jerry_value_t value_to_set) /**< value to set */
{
  jerry_assert_api_available ();

  if (ECMA_IS_VALUE_ERROR (value_to_set)
      || !ecma_is_value_object (obj_val)
      || !ecma_is_value_string (prop_name_val))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p));
  }

  return ecma_op_object_put (ecma_get_object_from_value (obj_val),
                             ecma_get_string_from_value (prop_name_val),
                             value_to_set,
                             true);
} /* jerry_set_property */

/**
 * Set indexed value in the specified object
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return true value - if the operation was successful
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_set_property_by_index (const jerry_value_t obj_val, /**< object value */
                             uint32_t index, /**< index to be written */
                             const jerry_value_t value_to_set) /**< value to set */
{
  jerry_assert_api_available ();

  if (ECMA_IS_VALUE_ERROR (value_to_set)
      || !ecma_is_value_object (obj_val))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p));
  }

  ecma_string_t *str_idx_p = ecma_new_ecma_string_from_uint32 ((uint32_t) index);
  ecma_value_t ret_value = ecma_op_object_put (ecma_get_object_from_value (obj_val),
                                               str_idx_p,
                                               value_to_set,
                                               true);
  ecma_deref_ecma_string (str_idx_p);

  return ret_value;
} /* jerry_set_property_by_index */

/**
 * Initialize property descriptor.
 */
void
jerry_init_property_descriptor_fields (jerry_property_descriptor_t *prop_desc_p) /**< [out] property descriptor */
{
  prop_desc_p->is_value_defined = false;
  prop_desc_p->value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  prop_desc_p->is_writable_defined = false;
  prop_desc_p->is_writable = false;
  prop_desc_p->is_enumerable_defined = false;
  prop_desc_p->is_enumerable = false;
  prop_desc_p->is_configurable_defined = false;
  prop_desc_p->is_configurable = false;
  prop_desc_p->is_get_defined = false;
  prop_desc_p->getter = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  prop_desc_p->is_set_defined = false;
  prop_desc_p->setter = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
} /* jerry_init_property_descriptor_fields */

/**
 * Define a property to the specified object with the given name.
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return true value - if the operation was successful
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_define_own_property (const jerry_value_t obj_val, /**< object value */
                           const jerry_value_t prop_name_val, /**< property name (string value) */
                           const jerry_property_descriptor_t *prop_desc_p) /**< property descriptor */
{
  jerry_assert_api_available ();

  if (!ecma_is_value_object (obj_val)
      || !ecma_is_value_string (prop_name_val))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p));
  }

  if ((prop_desc_p->is_writable_defined || prop_desc_p->is_value_defined)
      && (prop_desc_p->is_get_defined || prop_desc_p->is_set_defined))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p));
  }

  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();

  prop_desc.is_enumerable_defined = prop_desc_p->is_enumerable_defined;
  prop_desc.is_enumerable = prop_desc_p->is_enumerable_defined ? prop_desc_p->is_enumerable : false;

  prop_desc.is_configurable_defined = prop_desc_p->is_configurable_defined;
  prop_desc.is_configurable = prop_desc_p->is_configurable_defined ? prop_desc_p->is_configurable : false;

  /* Copy data property info. */
  prop_desc.is_value_defined = prop_desc_p->is_value_defined;

  if (prop_desc_p->is_value_defined)
  {
    if (ECMA_IS_VALUE_ERROR (prop_desc_p->value))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p));
    }

    prop_desc.value = prop_desc_p->value;
  }

  prop_desc.is_writable_defined = prop_desc_p->is_writable_defined;
  prop_desc.is_writable = prop_desc_p->is_writable_defined ? prop_desc_p->is_writable : false;

  /* Copy accessor property info. */
  if (prop_desc_p->is_get_defined)
  {
    ecma_value_t getter = prop_desc_p->getter;
    prop_desc.is_get_defined = true;

    if (ECMA_IS_VALUE_ERROR (getter))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p));
    }

    if (ecma_op_is_callable (getter))
    {
      prop_desc.get_p = ecma_get_object_from_value (getter);
    }
    else if (!ecma_is_value_null (getter))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p));
    }
  }

  if (prop_desc_p->is_set_defined)
  {
    ecma_value_t setter = prop_desc_p->setter;
    prop_desc.is_set_defined = true;

    if (ECMA_IS_VALUE_ERROR (setter))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p));
    }

    if (ecma_op_is_callable (setter))
    {
      prop_desc.set_p = ecma_get_object_from_value (setter);
    }
    else if (!ecma_is_value_null (setter))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p));
    }
  }

  return ecma_op_object_define_own_property (ecma_get_object_from_value (obj_val),
                                             ecma_get_string_from_value (prop_name_val),
                                             &prop_desc,
                                             true);
} /* jerry_define_own_property */

/**
 * Construct property descriptor from specified property.
 *
 * @return true - if success, the prop_desc_p fields contains the property info
 *         false - otherwise, the prop_desc_p is unchanged
 */
bool
jerry_get_own_property_descriptor (const jerry_value_t  obj_val, /**< object value */
                                   const jerry_value_t prop_name_val, /**< property name (string value) */
                                   jerry_property_descriptor_t *prop_desc_p) /**< property descriptor */
{
  jerry_assert_api_available ();

  if (!ecma_is_value_object (obj_val)
      || !ecma_is_value_string (prop_name_val))
  {
    return false;
  }

  ecma_property_descriptor_t prop_desc;

  if (!ecma_op_object_get_own_property_descriptor (ecma_get_object_from_value (obj_val),
                                                   ecma_get_string_from_value (prop_name_val),
                                                   &prop_desc))
  {
    return false;
  }

  prop_desc_p->is_configurable_defined = true;
  prop_desc_p->is_configurable = prop_desc.is_configurable;
  prop_desc_p->is_enumerable_defined = true;
  prop_desc_p->is_enumerable = prop_desc.is_enumerable;

  prop_desc_p->is_writable_defined = prop_desc.is_writable_defined;
  prop_desc_p->is_writable = prop_desc.is_writable_defined ? prop_desc.is_writable : false;

  prop_desc_p->is_value_defined = prop_desc.is_value_defined;
  prop_desc_p->is_get_defined = prop_desc.is_get_defined;
  prop_desc_p->is_set_defined = prop_desc.is_set_defined;

  prop_desc_p->value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  prop_desc_p->getter = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  prop_desc_p->setter = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

  if (prop_desc.is_value_defined)
  {
    prop_desc_p->value = prop_desc.value;
  }

  if (prop_desc.is_get_defined)
  {
    if (prop_desc.get_p != NULL)
    {
      prop_desc_p->getter = ecma_make_object_value (prop_desc.get_p);
    }
    else
    {
      prop_desc_p->getter = ecma_make_simple_value (ECMA_SIMPLE_VALUE_NULL);
    }
  }

  if (prop_desc.is_set_defined)
  {
    if (prop_desc.set_p != NULL)
    {
      prop_desc_p->setter = ecma_make_object_value (prop_desc.set_p);
    }
    else
    {
      prop_desc_p->setter = ecma_make_simple_value (ECMA_SIMPLE_VALUE_NULL);
    }
  }

  return true;
} /* jerry_get_own_property_descriptor */

/**
 * Free fields of property descriptor (setter, getter and value).
 */
void
jerry_free_property_descriptor_fields (const jerry_property_descriptor_t *prop_desc_p) /**< property descriptor */
{
  if (prop_desc_p->is_value_defined)
  {
    ecma_free_value (prop_desc_p->value);
  }

  if (prop_desc_p->is_get_defined)
  {
    ecma_free_value (prop_desc_p->getter);
  }

  if (prop_desc_p->is_set_defined)
  {
    ecma_free_value (prop_desc_p->setter);
  }
} /* jerry_free_property_descriptor_fields */

/**
 * Invoke function specified by a function value
 *
 * Note:
 *      - returned value must be freed with jerry_release_value, when it is no longer needed.
 *      - If function is invoked as constructor, it should support [[Construct]] method,
 *        otherwise, if function is simply called - it should support [[Call]] method.
 *
 * @return returned jerry value of the invoked function
 */
static jerry_value_t
jerry_invoke_function (bool is_invoke_as_constructor, /**< true - invoke function as constructor
                                                       *          (this_arg_p should be NULL, as it is ignored),
                                                       *   false - perform function call */
                       const jerry_value_t func_obj_val, /**< function object to call */
                       const jerry_value_t this_val, /**< object value of 'this' binding */
                       const jerry_value_t args_p[], /**< function's call arguments */
                       const jerry_size_t args_count) /**< number of the arguments */
{
  JERRY_ASSERT (args_count == 0 || args_p != NULL);

  if (ECMA_IS_VALUE_ERROR (func_obj_val)
      || ECMA_IS_VALUE_ERROR (this_val))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG (error_value_msg_p));
  }

  for (uint32_t i = 0; i < args_count; i++)
  {
    if (ECMA_IS_VALUE_ERROR (args_p[i]))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG (error_value_msg_p));
    }
  }

  if (is_invoke_as_constructor)
  {
    JERRY_ASSERT (jerry_value_is_constructor (func_obj_val));

    return ecma_op_function_construct (ecma_get_object_from_value (func_obj_val),
                                       args_p,
                                       args_count);
  }
  else
  {
    JERRY_ASSERT (jerry_value_is_function (func_obj_val));

    return ecma_op_function_call (ecma_get_object_from_value (func_obj_val),
                                  this_val,
                                  args_p,
                                  args_count);
  }
} /* jerry_invoke_function */

/**
 * Call function specified by a function value
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *      error flag must not be set for any arguments of this function.
 *
 * @return returned jerry value of the called function
 */
jerry_value_t
jerry_call_function (const jerry_value_t func_obj_val, /**< function object to call */
                     const jerry_value_t this_val, /**< object for 'this' binding */
                     const jerry_value_t args_p[], /**< function's call arguments */
                     jerry_size_t args_count) /**< number of the arguments */
{
  jerry_assert_api_available ();

  if (jerry_value_is_function (func_obj_val))
  {
    return jerry_invoke_function (false, func_obj_val, this_val, args_p, args_count);
  }

  return ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p));
} /* jerry_call_function */

/**
 * Construct object value invoking specified function value as a constructor
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *      error flag must not be set for any arguments of this function.
 *
 * @return returned jerry value of the invoked constructor
 */
jerry_value_t
jerry_construct_object (const jerry_value_t func_obj_val, /**< function object to call */
                        const jerry_value_t args_p[], /**< function's call arguments
                                                       *   (NULL if arguments number is zero) */
                        jerry_size_t args_count) /**< number of the arguments */
{
  jerry_assert_api_available ();

  if (jerry_value_is_constructor (func_obj_val))
  {
    ecma_value_t this_val = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    return jerry_invoke_function (true, func_obj_val, this_val, args_p, args_count);
  }

  return ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p));
} /* jerry_construct_object */

/**
 * Get keys of the specified object value
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return array object value - if success
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_get_object_keys (const jerry_value_t obj_val) /**< object value */
{
  jerry_assert_api_available ();

  if (!ecma_is_value_object (obj_val))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p));
  }

  return ecma_builtin_helper_object_get_properties (ecma_get_object_from_value (obj_val), true);
} /* jerry_get_object_keys */

/**
 * Get the prototype of the specified object
 *
 * @return prototype object or null value - if success
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_get_prototype (const jerry_value_t obj_val) /**< object value */
{
  jerry_assert_api_available ();

  if (!ecma_is_value_object (obj_val))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p));
  }

  ecma_object_t *proto_obj_p = ecma_get_object_prototype (ecma_get_object_from_value (obj_val));

  if (proto_obj_p == NULL)
  {
    return ecma_make_simple_value (ECMA_SIMPLE_VALUE_NULL);
  }

  return ecma_make_object_value (proto_obj_p);
} /* jerry_get_prototype */

/**
 * Set the prototype of the specified object
 *
 * @return true value - if success
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_set_prototype (const jerry_value_t obj_val, /**< object value */
                     const jerry_value_t proto_obj_val) /**< prototype object value */
{
  jerry_assert_api_available ();

  if (!ecma_is_value_object (obj_val)
      || ECMA_IS_VALUE_ERROR (proto_obj_val)
      || (!ecma_is_value_object (proto_obj_val) && !ecma_is_value_null (proto_obj_val)))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p));
  }

  if (ecma_is_value_null (proto_obj_val))
  {
    ECMA_SET_POINTER (ecma_get_object_from_value (obj_val)->prototype_or_outer_reference_cp, NULL);
  }
  else
  {
    ECMA_SET_POINTER (ecma_get_object_from_value (obj_val)->prototype_or_outer_reference_cp,
                      ecma_get_object_from_value (proto_obj_val));
  }

  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
} /* jerry_set_prototype */

/**
 * Get native handle, associated with specified object
 *
 * @return true - if there is an associated handle (handle is returned through out_handle_p),
 *         false - otherwise.
 */
bool
jerry_get_object_native_handle (const jerry_value_t obj_val, /**< object to get handle from */
                                uintptr_t *out_handle_p) /**< [out] handle value */
{
  jerry_assert_api_available ();

  uintptr_t handle_value;

  bool does_exist = ecma_get_external_pointer_value (ecma_get_object_from_value (obj_val),
                                                     ECMA_INTERNAL_PROPERTY_NATIVE_HANDLE,
                                                     &handle_value);

  if (does_exist)
  {
    *out_handle_p = handle_value;
  }

  return does_exist;
} /* jerry_get_object_native_handle */

/**
 * Set native handle and an optional free callback for the specified object
 *
 * Note:
 *      If native handle was already set for the object, its value is updated.
 *
 * Note:
 *      If a non-NULL free callback is specified, it will be called
 *      by the garbage collector when the object is freed. The free
 *      callback always overwrites the previous value, so passing
 *      a NULL value deletes the current free callback.
 */
void
jerry_set_object_native_handle (const jerry_value_t obj_val, /**< object to set handle in */
                                uintptr_t handle_p, /**< handle value */
                                jerry_object_free_callback_t freecb_p) /**< object free callback or NULL */
{
  jerry_assert_api_available ();

  ecma_object_t *object_p = ecma_get_object_from_value (obj_val);

  ecma_create_external_pointer_property (object_p,
                                         ECMA_INTERNAL_PROPERTY_NATIVE_HANDLE,
                                         handle_p);

  ecma_create_external_pointer_property (object_p,
                                         ECMA_INTERNAL_PROPERTY_FREE_CALLBACK,
                                         (uintptr_t) freecb_p);
} /* jerry_set_object_native_handle */

/**
 * Applies the given function to the every property in the object.
 *
 * @return true, if object fields traversal was performed successfully, i.e.:
 *                - no unhandled exceptions were thrown in object fields traversal;
 *                - object fields traversal was stopped on callback that returned false;
 *         false - otherwise,
 *                 if getter of field threw a exception or unhandled exceptions were thrown during traversal;
 */
bool
jerry_foreach_object_property (const jerry_value_t obj_val, /**< object value */
                               jerry_object_property_foreach_t foreach_p, /**< foreach function */
                               void *user_data_p) /**< user data for foreach function */
{
  jerry_assert_api_available ();

  ecma_collection_iterator_t names_iter;
  ecma_object_t *object_p = ecma_get_object_from_value (obj_val);
  ecma_collection_header_t *names_p = ecma_op_object_get_property_names (object_p, false, true, true);
  ecma_collection_iterator_init (&names_iter, names_p);

  ecma_value_t property_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  bool continuous = true;

  while (continuous
         && ecma_collection_iterator_next (&names_iter))
  {
    ecma_string_t *property_name_p = ecma_get_string_from_value (*names_iter.current_value_p);
    property_value = ecma_op_object_get (object_p, property_name_p);

    if (ECMA_IS_VALUE_ERROR (property_value))
    {
      break;
    }

    continuous = foreach_p (*names_iter.current_value_p, property_value, user_data_p);
    ecma_free_value (property_value);
  }

  ecma_free_values_collection (names_p, true);

  if (!ECMA_IS_VALUE_ERROR (property_value))
  {
    return true;
  }

  ecma_free_value (property_value);
  return false;
} /* jerry_foreach_object_property */

/**
 * @}
 */

/**
 * ====================== Internal functions ==========================
 */

/**
 * Dispatch call to specified external function using the native handler
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return returned ecma value of the invoked native function - if success
 *         thrown error - otherwise
 */
ecma_value_t
jerry_dispatch_external_function (ecma_object_t *function_object_p, /**< external function object */
                                  ecma_external_pointer_t handler_p, /**< pointer to the function's native handler */
                                  ecma_value_t this_arg_value, /**< 'this' argument */
                                  const ecma_value_t *arguments_list_p, /**< arguments list */
                                  ecma_length_t arguments_list_len) /**< arguments list length */
{
  jerry_assert_api_available ();

  ecma_value_t ret_value = ((jerry_external_handler_t) handler_p) (ecma_make_object_value (function_object_p),
                                                                   this_arg_value,
                                                                   arguments_list_p,
                                                                   arguments_list_len);
  return ret_value;
} /* jerry_dispatch_external_function */

/**
 * Dispatch call to object's native free callback function
 *
 * Note:
 *       the callback is called during critical GC phase,
 *       so should not perform any requests to engine.
 */
void
jerry_dispatch_object_free_callback (ecma_external_pointer_t freecb_p, /**< pointer to free callback handler */
                                     ecma_external_pointer_t native_p) /**< native handle, associated
                                                                        *   with freed object */
{
  jerry_make_api_unavailable ();

  ((jerry_object_free_callback_t) freecb_p) ((uintptr_t) native_p);

  jerry_make_api_available ();
} /* jerry_dispatch_object_free_callback */
