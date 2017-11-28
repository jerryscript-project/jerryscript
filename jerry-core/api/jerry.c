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

#include "debugger.h"
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
#include "ecma-promise-object.h"
#include "jcontext.h"
#include "jerryscript.h"
#include "jmem.h"
#include "js-parser.h"
#include "re-compiler.h"

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

JERRY_STATIC_ASSERT ((int) ECMA_INIT_EMPTY == (int) JERRY_INIT_EMPTY
                     && (int) ECMA_INIT_SHOW_OPCODES == (int) JERRY_INIT_SHOW_OPCODES
                     && (int) ECMA_INIT_SHOW_REGEXP_OPCODES == (int) JERRY_INIT_SHOW_REGEXP_OPCODES
                     && (int) ECMA_INIT_MEM_STATS == (int) JERRY_INIT_MEM_STATS,
                     ecma_init_flag_t_must_be_equal_to_jerry_init_flag_t);

#if defined JERRY_DISABLE_JS_PARSER && !defined JERRY_ENABLE_SNAPSHOT_EXEC
#error JERRY_ENABLE_SNAPSHOT_EXEC must be defined if JERRY_DISABLE_JS_PARSER is defined!
#endif /* JERRY_DISABLE_JS_PARSER && !JERRY_ENABLE_SNAPSHOT_EXEC */

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
 * Remove the error flag from the argument value.
 *
 * Note:
 *   Compatiblity function, should go away with JerryScript 2.0
 *
 * @return return value for Jerry API functions
 */
static inline jerry_value_t __attr_always_inline___
jerry_get_arg_value (jerry_value_t value) /**< return value */
{
  if (unlikely (ecma_is_value_error_reference (value)))
  {
    value = ecma_get_error_reference_from_value (value)->value;
  }

  return value;
} /* jerry_get_arg_value */

/**
 * Create an API compatible return value.
 *
 * @return return value for Jerry API functions
 */
static jerry_value_t
jerry_return (jerry_value_t value) /**< return value */
{
  if (ECMA_IS_VALUE_ERROR (value))
  {
    value = ecma_create_error_reference (JERRY_CONTEXT (error_value));
  }

  return value;
} /* jerry_return */

/**
 * Throw an API compatible return value.
 *
 * @return return value for Jerry API functions
 */
static inline jerry_value_t __attr_always_inline___
jerry_throw (jerry_value_t value) /**< return value */
{
  JERRY_ASSERT (ECMA_IS_VALUE_ERROR (value));
  return ecma_create_error_reference (JERRY_CONTEXT (error_value));
} /* jerry_throw */

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

#ifdef JERRY_DEBUGGER
  if (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
  {
    jerry_debugger_close_connection ();
  }
#endif /* JERRY_DEBUGGER */

  for (jerry_context_data_header_t *this_p = JERRY_CONTEXT (context_data_p), *next_p = NULL;
       this_p != NULL;
       this_p = next_p)
  {
    next_p = this_p->next_p;
    this_p->manager_p->deinit_cb (JERRY_CONTEXT_DATA_HEADER_USER_DATA (this_p));
    jmem_heap_free_block (this_p, sizeof (jerry_context_data_header_t) + this_p->manager_p->bytes_needed);
  }

  ecma_finalize ();
  jmem_finalize ();
  jerry_make_api_unavailable ();
} /* jerry_cleanup */

/**
 * Retrieve a context data item, or create a new one.
 *
 * @param manager_p pointer to the manager whose context data item should be returned.
 *
 * @return a pointer to the user-provided context-specific data item for the given manager, creating such a pointer if
 * none was found.
 */
void *
jerry_get_context_data (const jerry_context_data_manager_t *manager_p)
{
  void *ret = NULL;
  jerry_context_data_header_t *item_p;

  for (item_p = JERRY_CONTEXT (context_data_p); item_p != NULL; item_p = item_p->next_p)
  {
    if (item_p->manager_p == manager_p)
    {
      return JERRY_CONTEXT_DATA_HEADER_USER_DATA (item_p);
    }
  }

  item_p = jmem_heap_alloc_block (sizeof (jerry_context_data_header_t) + manager_p->bytes_needed);
  item_p->manager_p = manager_p;
  item_p->next_p = JERRY_CONTEXT (context_data_p);
  JERRY_CONTEXT (context_data_p) = item_p;
  ret = JERRY_CONTEXT_DATA_HEADER_USER_DATA (item_p);

  memset (ret, 0, manager_p->bytes_needed);
  if (manager_p->init_cb)
  {
    manager_p->init_cb (ret);
  }

  return ret;
} /* jerry_get_context_data */

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
 * Get heap memory stats.
 *
 * @return true - get the heap stats successful
 *         false - otherwise. Usually it is because the MEM_STATS feature is not enabled.
 */
bool
jerry_get_memory_stats (jerry_heap_stats_t *out_stats_p) /**< [out] heap memory stats */
{
#ifdef JMEM_STATS
  if (out_stats_p == NULL)
  {
    return false;
  }

  jmem_heap_stats_t jmem_heap_stats = {0};
  jmem_heap_get_stats (&jmem_heap_stats);

  *out_stats_p = (jerry_heap_stats_t)
  {
    .version = 1,
    .size = jmem_heap_stats.size,
    .allocated_bytes = jmem_heap_stats.allocated_bytes,
    .peak_allocated_bytes = jmem_heap_stats.peak_allocated_bytes
  };

  return true;
#else
  JERRY_UNUSED (out_stats_p);
  return false;
#endif
} /* jerry_get_memory_stats */

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

  if (!ecma_is_value_error_reference (parse_ret_val))
  {
    jerry_value_t run_ret_val = jerry_run (parse_ret_val);

    if (!ecma_is_value_error_reference (run_ret_val))
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
#ifndef JERRY_DISABLE_JS_PARSER
  jerry_assert_api_available ();

  ecma_compiled_code_t *bytecode_data_p;
  ecma_value_t parse_status;

  parse_status = parser_parse_script (NULL,
                                      0,
                                      source_p,
                                      source_size,
                                      is_strict,
                                      &bytecode_data_p);

  if (ECMA_IS_VALUE_ERROR (parse_status))
  {
    return ecma_create_error_reference (JERRY_CONTEXT (error_value));
  }

  ecma_free_value (parse_status);

  ecma_object_t *lex_env_p = ecma_get_global_environment ();
  ecma_object_t *func_obj_p = ecma_op_create_function_object (lex_env_p,
                                                              bytecode_data_p);
  ecma_bytecode_deref (bytecode_data_p);

  return ecma_make_object_value (func_obj_p);
#else /* JERRY_DISABLE_JS_PARSER */
  JERRY_UNUSED (source_p);
  JERRY_UNUSED (source_size);
  JERRY_UNUSED (is_strict);

  return jerry_throw (ecma_raise_syntax_error (ECMA_ERR_MSG ("The parser has been disabled.")));
#endif /* !JERRY_DISABLE_JS_PARSER */
} /* jerry_parse */

/**
 * Parse script and construct an ECMAScript function. The lexical
 * environment is set to the global lexical environment. The name
 * (usually a file name) is also passed to this function which is
 * used by the debugger to find the source code.
 *
 * @return function object value - if script was parsed successfully,
 *         thrown error - otherwise
 */
jerry_value_t
jerry_parse_named_resource (const jerry_char_t *resource_name_p, /**< resource name (usually a file name) */
                            size_t resource_name_length, /**< length of resource name */
                            const jerry_char_t *source_p, /**< script source */
                            size_t source_size, /**< script source size */
                            bool is_strict) /**< strict mode */
{
#if defined JERRY_DEBUGGER && !defined JERRY_DISABLE_JS_PARSER
  if (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
  {
    jerry_debugger_send_string (JERRY_DEBUGGER_SOURCE_CODE_NAME,
                                JERRY_DEBUGGER_NO_SUBTYPE,
                                resource_name_p,
                                resource_name_length);
  }
#else /* !(JERRY_DEBUGGER && !JERRY_DISABLE_JS_PARSER) */
  JERRY_UNUSED (resource_name_p);
  JERRY_UNUSED (resource_name_length);
#endif /* JERRY_DEBUGGER && !JERRY_DISABLE_JS_PARSER */

  return jerry_parse (source_p, source_size, is_strict);
} /* jerry_parse_named_resource */

/**
 * Parse function and construct an EcmaScript function. The lexical
 * environment is set to the global lexical environment.
 *
 * @return function object value - if script was parsed successfully,
 *         thrown error - otherwise
 */
jerry_value_t
jerry_parse_function (const jerry_char_t *resource_name_p, /**< resource name (usually a file name) */
                      size_t resource_name_length, /**< length of resource name */
                      const jerry_char_t *arg_list_p, /**< script source */
                      size_t arg_list_size, /**< script source size */
                      const jerry_char_t *source_p, /**< script source */
                      size_t source_size, /**< script source size */
                      bool is_strict) /**< strict mode */
{
#if defined JERRY_DEBUGGER && !defined JERRY_DISABLE_JS_PARSER
  if (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
  {
    jerry_debugger_send_string (JERRY_DEBUGGER_SOURCE_CODE_NAME,
                                JERRY_DEBUGGER_NO_SUBTYPE,
                                resource_name_p,
                                resource_name_length);
  }
#else /* !(JERRY_DEBUGGER && !JERRY_DISABLE_JS_PARSER) */
  JERRY_UNUSED (resource_name_p);
  JERRY_UNUSED (resource_name_length);
#endif /* JERRY_DEBUGGER && !JERRY_DISABLE_JS_PARSER */

#ifndef JERRY_DISABLE_JS_PARSER
  jerry_assert_api_available ();

  ecma_compiled_code_t *bytecode_data_p;
  ecma_value_t parse_status;

  if (arg_list_p == NULL)
  {
    /* Must not be a NULL value. */
    arg_list_p = (const jerry_char_t *) "";
  }

  parse_status = parser_parse_script (arg_list_p,
                                      arg_list_size,
                                      source_p,
                                      source_size,
                                      is_strict,
                                      &bytecode_data_p);

  if (ECMA_IS_VALUE_ERROR (parse_status))
  {
    return ecma_create_error_reference (JERRY_CONTEXT (error_value));
  }

  ecma_free_value (parse_status);

  ecma_object_t *lex_env_p = ecma_get_global_environment ();
  ecma_object_t *func_obj_p = ecma_op_create_function_object (lex_env_p,
                                                              bytecode_data_p);
  ecma_bytecode_deref (bytecode_data_p);

  return ecma_make_object_value (func_obj_p);
#else /* JERRY_DISABLE_JS_PARSER */
  JERRY_UNUSED (arg_list_p);
  JERRY_UNUSED (arg_list_size);
  JERRY_UNUSED (source_p);
  JERRY_UNUSED (source_size);
  JERRY_UNUSED (is_strict);

  return jerry_throw (ecma_raise_syntax_error (ECMA_ERR_MSG ("The parser has been disabled.")));
#endif /* !JERRY_DISABLE_JS_PARSER */
} /* jerry_parse_function */

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
    return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  ecma_object_t *func_obj_p = ecma_get_object_from_value (func_val);

  if (ecma_get_object_type (func_obj_p) != ECMA_OBJECT_TYPE_FUNCTION
      || ecma_get_object_is_builtin (func_obj_p))
  {
    return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) func_obj_p;

  ecma_object_t *scope_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                            ext_func_p->u.function.scope_cp);

  if (scope_p != ecma_get_global_environment ())
  {
    return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  const ecma_compiled_code_t *bytecode_data_p;
  bytecode_data_p = ECMA_GET_INTERNAL_VALUE_POINTER (const ecma_compiled_code_t,
                                                     ext_func_p->u.function.bytecode_cp);

  return jerry_return (vm_run_global (bytecode_data_p));
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

  return jerry_return (ecma_op_eval_chars_buffer ((const lit_utf8_byte_t *) source_p,
                                                  source_size,
                                                  false,
                                                  is_strict));
} /* jerry_eval */

/**
 * Run enqueued Promise jobs until the first thrown error or until all get executed.
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return result of last executed job, may be error value.
 */
jerry_value_t
jerry_run_all_enqueued_jobs (void)
{
  jerry_assert_api_available ();

#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN
  return ecma_process_all_enqueued_jobs ();
#else /* CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
  return ECMA_VALUE_UNDEFINED;
#endif /* CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
} /* jerry_run_all_enqueued_jobs */

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
 *         false - otherwise
 */
bool
jerry_value_is_array (const jerry_value_t value) /**< jerry api value */
{
  jerry_assert_api_available ();

  jerry_value_t array = jerry_get_arg_value (value);
  return (ecma_is_value_object (array)
          && ecma_get_object_type (ecma_get_object_from_value (array)) == ECMA_OBJECT_TYPE_ARRAY);
} /* jerry_value_is_array */

/**
 * Check if the specified value is boolean.
 *
 * @return true  - if the specified value is boolean,
 *         false - otherwise
 */
bool
jerry_value_is_boolean (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();

  return ecma_is_value_boolean (jerry_get_arg_value (value));
} /* jerry_value_is_boolean */

/**
 * Check if the specified value is a constructor function object value.
 *
 * @return true - if the specified value is a function value that implements [[Construct]],
 *         false - otherwise
 */
bool
jerry_value_is_constructor (const jerry_value_t value) /**< jerry api value */
{
  jerry_assert_api_available ();

  return ecma_is_constructor (jerry_get_arg_value (value));
} /* jerry_value_is_constructor */

/**
 * Check if the specified value is a function object value.
 *
 * @return true - if the specified value is callable,
 *         false - otherwise
 */
bool
jerry_value_is_function (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();

  return ecma_op_is_callable (jerry_get_arg_value (value));
} /* jerry_value_is_function */

/**
 * Check if the specified value is number.
 *
 * @return true  - if the specified value is number,
 *         false - otherwise
 */
bool
jerry_value_is_number (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();

  return ecma_is_value_number (jerry_get_arg_value (value));
} /* jerry_value_is_number */

/**
 * Check if the specified value is null.
 *
 * @return true  - if the specified value is null,
 *         false - otherwise
 */
bool
jerry_value_is_null (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();

  return ecma_is_value_null (jerry_get_arg_value (value));
} /* jerry_value_is_null */

/**
 * Check if the specified value is object.
 *
 * @return true  - if the specified value is object,
 *         false - otherwise
 */
bool
jerry_value_is_object (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();

  return ecma_is_value_object (jerry_get_arg_value (value));
} /* jerry_value_is_object */

/**
 * Check if the specified value is promise.
 *
 * @return true  - if the specified value is promise,
 *         false - otherwise
 */
bool
jerry_value_is_promise (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();
#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN
  jerry_value_t promise = jerry_get_arg_value (value);
  return (ecma_is_value_object (promise)
          && ecma_is_promise (ecma_get_object_from_value (promise)));
#else /* CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
  JERRY_UNUSED (value);
  return false;
#endif /* !CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
} /* jerry_value_is_promise */

/**
 * Check if the specified value is string.
 *
 * @return true  - if the specified value is string,
 *         false - otherwise
 */
bool
jerry_value_is_string (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();

  return ecma_is_value_string (jerry_get_arg_value (value));
} /* jerry_value_is_string */

/**
 * Check if the specified value is undefined.
 *
 * @return true  - if the specified value is undefined,
 *         false - otherwise
 */
bool
jerry_value_is_undefined (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();

  return ecma_is_value_undefined (jerry_get_arg_value (value));
} /* jerry_value_is_undefined */

/**
 * Check if the specified feature is enabled.
 *
 * @return true  - if the specified feature is enabled,
 *         false - otherwise
 */
bool jerry_is_feature_enabled (const jerry_feature_t feature)
{
  JERRY_ASSERT (feature < JERRY_FEATURE__COUNT);

  return (false
#ifdef JERRY_CPOINTER_32_BIT
          || feature == JERRY_FEATURE_CPOINTER_32_BIT
#endif /* JERRY_CPOINTER_32_BIT */
#ifdef JERRY_ENABLE_ERROR_MESSAGES
          || feature == JERRY_FEATURE_ERROR_MESSAGES
#endif /* JERRY_ENABLE_ERROR_MESSAGES */
#ifndef JERRY_DISABLE_JS_PARSER
          || feature == JERRY_FEATURE_JS_PARSER
#endif /* !JERRY_DISABLE_JS_PARSER */
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
#ifdef JERRY_DEBUGGER
          || feature == JERRY_FEATURE_DEBUGGER
#endif /* JERRY_DEBUGGER */
#ifdef JERRY_VM_EXEC_STOP
          || feature == JERRY_FEATURE_VM_EXEC_STOP
#endif /* JERRY_VM_EXEC_STOP */
          );
} /* jerry_is_feature_enabled */

/**
 * Check if the specified value is an error value.
 *
 * @return true  - if the error flag of the specified value is true,
 *         false - otherwise
 */
bool
jerry_value_has_error_flag (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();

  return ecma_is_value_error_reference (value);
} /* jerry_value_has_error_flag */

/**
 * Clear the error flag
 */
void
jerry_value_clear_error_flag (jerry_value_t *value_p)
{
  jerry_assert_api_available ();

  if (ecma_is_value_error_reference (*value_p))
  {
    *value_p = ecma_clear_error_reference (*value_p);
  }
} /* jerry_value_clear_error_flag */

/**
 * Set the error flag.
 */
void
jerry_value_set_error_flag (jerry_value_t *value_p)
{
  jerry_assert_api_available ();

  if (!ecma_is_value_error_reference (*value_p))
  {
    *value_p = ecma_create_error_reference (*value_p);
  }
} /* jerry_value_set_error_flag */

/**
 * If the input value is an error value, then return a new reference to its referenced value.
 * Otherwise, return a new reference to the value itself.
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return the real value of the jerry_value
 */
jerry_value_t jerry_get_value_without_error_flag (jerry_value_t value) /**< api value */
{
  return jerry_acquire_value (jerry_get_arg_value (value));
} /* jerry_get_value_without_error_flag */

/**
 * Get boolean from the specified value.
 *
 * @return true or false.
 */
bool
jerry_get_boolean_value (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_available ();

  jerry_value_t boolean = jerry_get_arg_value (value);

  if (!ecma_is_value_boolean (boolean))
  {
    return false;
  }

  return ecma_is_value_true (boolean);
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

  jerry_value_t number = jerry_get_arg_value (value);

  if (!ecma_is_value_number (number))
  {
    return 0;
  }

  return (double) ecma_get_number_from_value (number);
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

  if (ecma_is_value_error_reference (value))
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

  if (ecma_is_value_error_reference (value))
  {
    return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (error_value_msg_p)));
  }

  return jerry_return (ecma_op_to_number (value));
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

  if (ecma_is_value_error_reference (value))
  {
    return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (error_value_msg_p)));
  }

  return jerry_return (ecma_op_to_object (value));
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

  if (ecma_is_value_error_reference (value))
  {
    return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (error_value_msg_p)));
  }

  return jerry_return (ecma_op_to_primitive (value, ECMA_PREFERRED_TYPE_NO));
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

  if (ecma_is_value_error_reference (value))
  {
    return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (error_value_msg_p)));
  }

  return jerry_return (ecma_op_to_string (value));
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

  if (unlikely (ecma_is_value_error_reference (value)))
  {
    ecma_ref_error_reference (ecma_get_error_reference_from_value (value));
    return value;
  }

  return ecma_copy_value (value);
} /* jerry_acquire_value */

/**
 * Release specified Jerry API value
 */
void
jerry_release_value (jerry_value_t value) /**< API value */
{
  jerry_assert_api_available ();

  if (unlikely (ecma_is_value_error_reference (value)))
  {
    ecma_deref_error_reference (ecma_get_error_reference_from_value (value));
    return;
  }

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

  const jerry_length_t argument_size = 1;
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

  return jerry_return (ecma_make_boolean_value (value));
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
    return ecma_create_error_object_reference (ecma_new_standard_error ((ecma_standard_error_t) error_type));
  }
  else
  {
    ecma_string_t *message_string_p = ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) message_p,
                                                                      (lit_utf8_size_t) message_size);

    ecma_object_t *error_object_p = ecma_new_standard_error_with_message ((ecma_standard_error_t) error_type,
                                                                          message_string_p);

    ecma_deref_ecma_string (message_string_p);

    return ecma_create_error_object_reference (error_object_p);
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

  ecma_object_t *func_obj_p = ecma_op_create_external_function_object (handler_p);
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
 * Creates a jerry_value_t representing an undefined value.
 *
 * @return value of undefined
 */
jerry_value_t
jerry_create_undefined (void)
{
  jerry_assert_api_available ();

  return ECMA_VALUE_UNDEFINED;
} /* jerry_create_undefined */

/**
 * Creates and returns a jerry_value_t with type null object.
 *
 * @return jerry_value_t representing null
 */
jerry_value_t
jerry_create_null (void)
{
  jerry_assert_api_available ();

  return ECMA_VALUE_NULL;
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
 * Create an empty Promise object which can be resolve/reject later
 * by calling jerry_resolve_or_reject_promise.
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return value of the created object
 */
jerry_value_t
jerry_create_promise (void)
{
  jerry_assert_api_available ();

#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN
  return ecma_op_create_promise_object (ECMA_VALUE_EMPTY, ECMA_PROMISE_EXECUTOR_EMPTY);
#else /* !CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
  return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG ("Promise not supported.")));
#endif /* CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
} /* jerry_create_promise */

/**
 * Create string from a valid UTF-8 string
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
 * Create string from a valid UTF-8 string
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
 * Create string from a valid CESU-8 string
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
 * Create string from a valid CESU-8 string
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

  jerry_value_t array = jerry_get_arg_value (value);

  if (!jerry_value_is_array (array))
  {
    return 0;
  }

  ecma_value_t len_value = ecma_op_object_get_by_magic_id (ecma_get_object_from_value (array),
                                                           LIT_MAGIC_STRING_LENGTH);

  jerry_length_t length = ecma_number_to_uint32 (ecma_get_number_from_value (len_value));
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

  jerry_value_t string = jerry_get_arg_value (value);

  if (!ecma_is_value_string (string))
  {
    return 0;
  }

  return ecma_string_get_size (ecma_get_string_from_value (string));
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

  jerry_value_t string = jerry_get_arg_value (value);

  if (!ecma_is_value_string (string))
  {
    return 0;
  }

  return ecma_string_get_utf8_size (ecma_get_string_from_value (string));
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

  jerry_value_t string = jerry_get_arg_value (value);

  if (!ecma_is_value_string (string))
  {
    return 0;
  }

  return ecma_string_get_length (ecma_get_string_from_value (string));
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

  jerry_value_t string = jerry_get_arg_value (value);

  if (!ecma_is_value_string (string))
  {
    return 0;
  }

  return ecma_string_get_utf8_length (ecma_get_string_from_value (string));
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

  jerry_value_t string = jerry_get_arg_value (value);

  if (!ecma_is_value_string (string) || buffer_p == NULL)
  {
    return 0;
  }

  ecma_string_t *str_p = ecma_get_string_from_value (string);

  if (ecma_string_get_size (str_p) > buffer_size)
  {
    return 0;
  }

  return ecma_string_copy_to_cesu8_buffer (str_p,
                                           (lit_utf8_byte_t *) buffer_p,
                                           buffer_size);
} /* jerry_string_to_char_buffer */

/**
 * Copy the characters of an utf-8 encoded string into a specified buffer.
 *
 * Note:
 *      The '\0' character could occur anywhere in the returned string
 *      Returns 0, if the value parameter is not a string or the buffer
 *      is not large enough for the whole string.
 *
 * @return number of bytes copied to the buffer.
 */
jerry_size_t
jerry_string_to_utf8_char_buffer (const jerry_value_t value, /**< input string value */
                                  jerry_char_t *buffer_p, /**< [out] output characters buffer */
                                  jerry_size_t buffer_size) /**< size of output buffer */
{
  jerry_assert_api_available ();

  jerry_value_t string = jerry_get_arg_value (value);

  if (!ecma_is_value_string (string) || buffer_p == NULL)
  {
    return 0;
  }

  ecma_string_t *str_p = ecma_get_string_from_value (string);

  if (ecma_string_get_utf8_size (str_p) > buffer_size)
  {
    return 0;
  }

  return ecma_string_copy_to_utf8_buffer (str_p,
                                          (lit_utf8_byte_t *) buffer_p,
                                          buffer_size);
} /* jerry_string_to_utf8_char_buffer */

/**
 * Copy the characters of an cesu-8 encoded substring into a specified buffer.
 *
 * Note:
 *      The '\0' character could occur anywhere in the returned string
 *      Returns 0, if the value parameter is not a string.
 *      It will extract the substring beetween the specified start position
 *      and the end position (or the end of the string, whichever comes first).
 *
 * @return number of bytes copied to the buffer.
 */
jerry_size_t
jerry_substring_to_char_buffer (const jerry_value_t value, /**< input string value */
                                jerry_length_t start_pos, /**< position of the first character */
                                jerry_length_t end_pos, /**< position of the last character */
                                jerry_char_t *buffer_p, /**< [out] output characters buffer */
                                jerry_size_t buffer_size) /**< size of output buffer */
{
  jerry_assert_api_available ();

  jerry_value_t string = jerry_get_arg_value (value);

  if (!ecma_is_value_string (string) || buffer_p == NULL)
  {
    return 0;
  }

  ecma_string_t *str_p = ecma_get_string_from_value (string);

  return ecma_substring_copy_to_cesu8_buffer (str_p,
                                              start_pos,
                                              end_pos,
                                              (lit_utf8_byte_t *) buffer_p,
                                              buffer_size);
} /* jerry_substring_to_char_buffer */

/**
 * Copy the characters of an utf-8 encoded substring into a specified buffer.
 *
 * Note:
 *      The '\0' character could occur anywhere in the returned string
 *      Returns 0, if the value parameter is not a string.
 *      It will extract the substring beetween the specified start position
 *      and the end position (or the end of the string, whichever comes first).
 *
 * @return number of bytes copied to the buffer.
 */
jerry_size_t
jerry_substring_to_utf8_char_buffer (const jerry_value_t value, /**< input string value */
                                     jerry_length_t start_pos, /**< position of the first character */
                                     jerry_length_t end_pos, /**< position of the last character */
                                     jerry_char_t *buffer_p, /**< [out] output characters buffer */
                                     jerry_size_t buffer_size) /**< size of output buffer */
{
  jerry_assert_api_available ();

  jerry_value_t string = jerry_get_arg_value (value);

  if (!ecma_is_value_string (string) || buffer_p == NULL)
  {
    return 0;
  }

  ecma_string_t *str_p = ecma_get_string_from_value (string);

  return ecma_substring_copy_to_utf8_buffer (str_p,
                                             start_pos,
                                             end_pos,
                                             (lit_utf8_byte_t *) buffer_p,
                                             buffer_size);
} /* jerry_substring_to_utf8_char_buffer */

/**
 * Checks whether the object or it's prototype objects have the given property.
 *
 * @return true  - if the property exists
 *         false - otherwise
 */
jerry_value_t
jerry_has_property (const jerry_value_t obj_val, /**< object value */
                    const jerry_value_t prop_name_val) /**< property name (string value) */
{
  jerry_assert_api_available ();

  jerry_value_t obj_value = jerry_get_arg_value (obj_val);
  jerry_value_t prop_name_value = jerry_get_arg_value (prop_name_val);

  if (!ecma_is_value_object (obj_value)
      || !ecma_is_value_string (prop_name_value))
  {
    return ecma_make_boolean_value (false);
  }

  bool has_property = ecma_op_object_has_property (ecma_get_object_from_value (obj_value),
                                                   ecma_get_string_from_value (prop_name_value));

  return ecma_make_boolean_value (has_property);
} /* jerry_has_property */

/**
 * Checks whether the object has the given property.
 *
 * @return true  - if the property exists
 *         false - otherwise
 */
jerry_value_t
jerry_has_own_property (const jerry_value_t obj_val, /**< object value */
                        const jerry_value_t prop_name_val) /**< property name (string value) */
{
  jerry_assert_api_available ();

  jerry_value_t obj_value = jerry_get_arg_value (obj_val);
  jerry_value_t prop_name_value = jerry_get_arg_value (prop_name_val);

  if (!ecma_is_value_object (obj_value)
      || !ecma_is_value_string (prop_name_value))
  {
    return ecma_make_boolean_value (false);
  }

  bool has_property = ecma_op_object_has_own_property (ecma_get_object_from_value (obj_value),
                                                       ecma_get_string_from_value (prop_name_value));

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

  jerry_value_t obj_value = jerry_get_arg_value (obj_val);
  jerry_value_t prop_name_value = jerry_get_arg_value (prop_name_val);

  if (!ecma_is_value_object (obj_value)
      || !ecma_is_value_string (prop_name_value))
  {
    return false;
  }

  ecma_value_t ret_value = ecma_op_object_delete (ecma_get_object_from_value (obj_value),
                                                  ecma_get_string_from_value (prop_name_value),
                                                  false);
  return ecma_is_value_true (ret_value);
} /* jerry_delete_property */

/**
 * Delete indexed property from the specified object.
 *
 * @return true  - if property was deleted successfully
 *         false - otherwise
 */
bool
jerry_delete_property_by_index (const jerry_value_t obj_val, /**< object value */
                                uint32_t index) /**< index to be written */
{
  jerry_assert_api_available ();

  jerry_value_t obj_value = jerry_get_arg_value (obj_val);

  if (!ecma_is_value_object (obj_value))
  {
    return false;
  }

  ecma_string_t str_idx;
  ecma_init_ecma_string_from_uint32 (&str_idx, index);
  ecma_value_t ret_value = ecma_op_object_delete (ecma_get_object_from_value (obj_value),
                                                  &str_idx,
                                                  false);
  return ecma_is_value_true (ret_value);
} /* jerry_delete_property_by_index */

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

  jerry_value_t obj_value = jerry_get_arg_value (obj_val);
  jerry_value_t prop_name_value = jerry_get_arg_value (prop_name_val);

  if (!ecma_is_value_object (obj_value)
      || !ecma_is_value_string (prop_name_value))
  {
    return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  jerry_value_t ret_value = ecma_op_object_get (ecma_get_object_from_value (obj_value),
                                                ecma_get_string_from_value (prop_name_value));
  return jerry_return (ret_value);
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

  jerry_value_t obj_value = jerry_get_arg_value (obj_val);

  if (!ecma_is_value_object (obj_value))
  {
    return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  ecma_string_t str_idx;
  ecma_init_ecma_string_from_uint32 (&str_idx, index);
  ecma_value_t ret_value = ecma_op_object_get (ecma_get_object_from_value (obj_value), &str_idx);

  return jerry_return (ret_value);
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

  jerry_value_t obj_value = jerry_get_arg_value (obj_val);
  jerry_value_t prop_name_value = jerry_get_arg_value (prop_name_val);

  if (ecma_is_value_error_reference (value_to_set)
      || !ecma_is_value_object (obj_value)
      || !ecma_is_value_string (prop_name_value))
  {
    return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  return jerry_return (ecma_op_object_put (ecma_get_object_from_value (obj_value),
                                           ecma_get_string_from_value (prop_name_value),
                                           value_to_set,
                                           true));
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

  jerry_value_t obj_value = jerry_get_arg_value (obj_val);

  if (ecma_is_value_error_reference (value_to_set)
      || !ecma_is_value_object (obj_value))
  {
    return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  ecma_string_t *str_idx_p = ecma_new_ecma_string_from_uint32 ((uint32_t) index);
  ecma_value_t ret_value = ecma_op_object_put (ecma_get_object_from_value (obj_value),
                                               str_idx_p,
                                               value_to_set,
                                               true);
  ecma_deref_ecma_string (str_idx_p);

  return jerry_return (ret_value);
} /* jerry_set_property_by_index */

/**
 * Initialize property descriptor.
 */
void
jerry_init_property_descriptor_fields (jerry_property_descriptor_t *prop_desc_p) /**< [out] property descriptor */
{
  prop_desc_p->is_value_defined = false;
  prop_desc_p->value = ECMA_VALUE_UNDEFINED;
  prop_desc_p->is_writable_defined = false;
  prop_desc_p->is_writable = false;
  prop_desc_p->is_enumerable_defined = false;
  prop_desc_p->is_enumerable = false;
  prop_desc_p->is_configurable_defined = false;
  prop_desc_p->is_configurable = false;
  prop_desc_p->is_get_defined = false;
  prop_desc_p->getter = ECMA_VALUE_UNDEFINED;
  prop_desc_p->is_set_defined = false;
  prop_desc_p->setter = ECMA_VALUE_UNDEFINED;
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

  jerry_value_t obj_value = jerry_get_arg_value (obj_val);
  jerry_value_t prop_name_value = jerry_get_arg_value (prop_name_val);

  if (!ecma_is_value_object (obj_value)
      || !ecma_is_value_string (prop_name_value))
  {
    return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  if ((prop_desc_p->is_writable_defined || prop_desc_p->is_value_defined)
      && (prop_desc_p->is_get_defined || prop_desc_p->is_set_defined))
  {
    return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();

  prop_desc.is_enumerable_defined = ECMA_BOOL_TO_BITFIELD (prop_desc_p->is_enumerable_defined);
  prop_desc.is_enumerable = ECMA_BOOL_TO_BITFIELD (prop_desc_p->is_enumerable_defined ? prop_desc_p->is_enumerable
                                                                                      : false);

  prop_desc.is_configurable_defined = ECMA_BOOL_TO_BITFIELD (prop_desc_p->is_configurable_defined);
  prop_desc.is_configurable = ECMA_BOOL_TO_BITFIELD (prop_desc_p->is_configurable_defined ? prop_desc_p->is_configurable
                                                                                          : false);

  /* Copy data property info. */
  prop_desc.is_value_defined = ECMA_BOOL_TO_BITFIELD (prop_desc_p->is_value_defined);

  if (prop_desc_p->is_value_defined)
  {
    if (ecma_is_value_error_reference (prop_desc_p->value))
    {
      return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
    }

    prop_desc.value = prop_desc_p->value;
  }

  prop_desc.is_writable_defined = ECMA_BOOL_TO_BITFIELD (prop_desc_p->is_writable_defined);
  prop_desc.is_writable = ECMA_BOOL_TO_BITFIELD (prop_desc_p->is_writable_defined ? prop_desc_p->is_writable
                                                                                  : false);

  /* Copy accessor property info. */
  if (prop_desc_p->is_get_defined)
  {
    ecma_value_t getter = prop_desc_p->getter;
    prop_desc.is_get_defined = true;

    if (ecma_is_value_error_reference (getter))
    {
      return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
    }

    if (ecma_op_is_callable (getter))
    {
      prop_desc.get_p = ecma_get_object_from_value (getter);
    }
    else if (!ecma_is_value_null (getter))
    {
      return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
    }
  }

  if (prop_desc_p->is_set_defined)
  {
    ecma_value_t setter = prop_desc_p->setter;
    prop_desc.is_set_defined = true;

    if (ecma_is_value_error_reference (setter))
    {
      return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
    }

    if (ecma_op_is_callable (setter))
    {
      prop_desc.set_p = ecma_get_object_from_value (setter);
    }
    else if (!ecma_is_value_null (setter))
    {
      return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
    }
  }

  return ecma_op_object_define_own_property (ecma_get_object_from_value (obj_value),
                                             ecma_get_string_from_value (prop_name_value),
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

  jerry_value_t obj_value = jerry_get_arg_value (obj_val);
  jerry_value_t prop_name_value = jerry_get_arg_value (prop_name_val);

  if (!ecma_is_value_object (obj_value)
      || !ecma_is_value_string (prop_name_value))
  {
    return false;
  }

  ecma_property_descriptor_t prop_desc;

  if (!ecma_op_object_get_own_property_descriptor (ecma_get_object_from_value (obj_value),
                                                   ecma_get_string_from_value (prop_name_value),
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

  prop_desc_p->value = ECMA_VALUE_UNDEFINED;
  prop_desc_p->getter = ECMA_VALUE_UNDEFINED;
  prop_desc_p->setter = ECMA_VALUE_UNDEFINED;

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
      prop_desc_p->getter = ECMA_VALUE_NULL;
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
      prop_desc_p->setter = ECMA_VALUE_NULL;
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
    jerry_release_value (prop_desc_p->value);
  }

  if (prop_desc_p->is_get_defined)
  {
    jerry_release_value (prop_desc_p->getter);
  }

  if (prop_desc_p->is_set_defined)
  {
    jerry_release_value (prop_desc_p->setter);
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

  if (ecma_is_value_error_reference (func_obj_val)
      || ecma_is_value_error_reference (this_val))
  {
    return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (error_value_msg_p)));
  }

  for (uint32_t i = 0; i < args_count; i++)
  {
    if (ecma_is_value_error_reference (args_p[i]))
    {
      return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (error_value_msg_p)));
    }
  }

  if (is_invoke_as_constructor)
  {
    JERRY_ASSERT (jerry_value_is_constructor (func_obj_val));

    return jerry_return (ecma_op_function_construct (ecma_get_object_from_value (func_obj_val),
                                                     args_p,
                                                     args_count));
  }
  else
  {
    JERRY_ASSERT (jerry_value_is_function (func_obj_val));

    return jerry_return (ecma_op_function_call (ecma_get_object_from_value (func_obj_val),
                                                this_val,
                                                args_p,
                                                args_count));
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

  return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
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
    ecma_value_t this_val = ECMA_VALUE_UNDEFINED;
    return jerry_invoke_function (true, func_obj_val, this_val, args_p, args_count);
  }

  return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
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

  jerry_value_t obj_value = jerry_get_arg_value (obj_val);

  if (!ecma_is_value_object (obj_value))
  {
    return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  return ecma_builtin_helper_object_get_properties (ecma_get_object_from_value (obj_value), true);
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

  jerry_value_t obj_value = jerry_get_arg_value (obj_val);

  if (!ecma_is_value_object (obj_value))
  {
    return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  ecma_object_t *proto_obj_p = ecma_get_object_prototype (ecma_get_object_from_value (obj_value));

  if (proto_obj_p == NULL)
  {
    return ECMA_VALUE_NULL;
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

  jerry_value_t obj_value = jerry_get_arg_value (obj_val);

  if (!ecma_is_value_object (obj_value)
      || ecma_is_value_error_reference (proto_obj_val)
      || (!ecma_is_value_object (proto_obj_val) && !ecma_is_value_null (proto_obj_val)))
  {
    return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  if (ecma_is_value_null (proto_obj_val))
  {
    ECMA_SET_POINTER (ecma_get_object_from_value (obj_value)->prototype_or_outer_reference_cp, NULL);
  }
  else
  {
    ECMA_SET_POINTER (ecma_get_object_from_value (obj_value)->prototype_or_outer_reference_cp,
                      ecma_get_object_from_value (proto_obj_val));
  }

  return ECMA_VALUE_TRUE;
} /* jerry_set_prototype */

/**
 * Get native handle, associated with specified object.
 *
 * Note: This API is deprecated, please use jerry_get_object_native_pointer instaed.
 *
 * @return true - if there is an associated handle (handle is returned through out_handle_p),
 *         false - otherwise
 */
bool
jerry_get_object_native_handle (const jerry_value_t obj_val, /**< object to get handle from */
                                uintptr_t *out_handle_p) /**< [out] handle value */
{
  jerry_assert_api_available ();

  jerry_value_t obj_value = jerry_get_arg_value (obj_val);

  if (!ecma_is_value_object (obj_value))
  {
    return false;
  }

  ecma_native_pointer_t *native_pointer_p;
  native_pointer_p = ecma_get_native_pointer_value (ecma_get_object_from_value (obj_value),
                                                    LIT_INTERNAL_MAGIC_STRING_NATIVE_HANDLE);

  if (native_pointer_p == NULL)
  {
    return false;
  }

  *out_handle_p = (uintptr_t) native_pointer_p->data_p;
  return true;
} /* jerry_get_object_native_handle */

/**
 * Set native handle and an optional free callback for the specified object.
 *
 * Note: This API is deprecated, please use jerry_set_object_native_pointer instaed.
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

  jerry_value_t obj_value = jerry_get_arg_value (obj_val);

  if (ecma_is_value_object (obj_value))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (obj_value);

    ecma_create_native_handle_property (object_p,
                                        (void *) handle_p,
                                        (void *) (ecma_external_pointer_t) freecb_p);
  }
} /* jerry_set_object_native_handle */

/**
 * Get native pointer and its type information, associated with specified object.
 *
 * Note:
 *  If native pointer is present, its type information is returned
 *  in out_native_pointer_p and out_native_info_p.
 *
 * @return true - if there is an associated pointer,
 *         false - otherwise
 */
bool
jerry_get_object_native_pointer (const jerry_value_t obj_val, /**< object to get native pointer from */
                                 void **out_native_pointer_p, /**< [out] native pointer */
                                 const jerry_object_native_info_t **out_native_info_p) /**< [out] the type info
                                                                                        *   of the native pointer */
{
  jerry_assert_api_available ();

  jerry_value_t obj_value = jerry_get_arg_value (obj_val);

  if (!ecma_is_value_object (obj_value))
  {
    return false;
  }

  ecma_native_pointer_t *native_pointer_p;
  native_pointer_p = ecma_get_native_pointer_value (ecma_get_object_from_value (obj_value),
                                                    LIT_INTERNAL_MAGIC_STRING_NATIVE_POINTER);

  if (native_pointer_p == NULL)
  {
    return false;
  }

  if (out_native_pointer_p != NULL)
  {
    *out_native_pointer_p = native_pointer_p->data_p;
  }

  if (out_native_info_p != NULL)
  {
    *out_native_info_p = (const jerry_object_native_info_t *) native_pointer_p->u.info_p;
  }

  return true;
} /* jerry_get_object_native_pointer */

/**
 * Set native pointer and an optional type info for the specified object.
 *
 *
 * Note:
 *      If native pointer was already set for the object, its value is updated.
 *
 * Note:
 *      If a non-NULL free callback is specified in the native type info,
 *      it will be called by the garbage collector when the object is freed.
 *      The type info is always overwrites the previous value, so passing
 *      a NULL value deletes the current type info.
 */
void
jerry_set_object_native_pointer (const jerry_value_t obj_val, /**< object to set native pointer in */
                                 void *native_pointer_p, /**< native pointer */
                                 const jerry_object_native_info_t *native_info_p) /**< object's native type info */
{
  jerry_assert_api_available ();

  jerry_value_t obj_value = jerry_get_arg_value (obj_val);

  if (ecma_is_value_object (obj_value))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (obj_value);

    ecma_create_native_pointer_property (object_p, native_pointer_p, (void *) native_info_p);
  }
} /* jerry_set_object_native_pointer */

/**
 * Applies the given function to the every property in the object.
 *
 * @return true - if object fields traversal was performed successfully, i.e.:
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

  jerry_value_t obj_value = jerry_get_arg_value (obj_val);

  if (!ecma_is_value_object (obj_value))
  {
    return false;
  }

  ecma_collection_iterator_t names_iter;
  ecma_object_t *object_p = ecma_get_object_from_value (obj_value);
  ecma_collection_header_t *names_p = ecma_op_object_get_property_names (object_p, false, true, true);
  ecma_collection_iterator_init (&names_iter, names_p);

  ecma_value_t property_value = ECMA_VALUE_EMPTY;

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

  ecma_free_value (JERRY_CONTEXT (error_value));
  return false;
} /* jerry_foreach_object_property */

/**
 * Resolve or reject the promise with an argument.
 *
 * @return undefined value - if success
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_resolve_or_reject_promise (jerry_value_t promise, /**< the promise value */
                                 jerry_value_t argument, /**< the argument */
                                 bool is_resolve) /**< whether the promise should be resolved or rejected */
{
  jerry_assert_api_available ();

#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN
  if (!ecma_is_value_object (promise) || !ecma_is_promise (ecma_get_object_from_value (promise)))
  {
    return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG (wrong_args_msg_p)));
  }

  lit_magic_string_id_t prop_name = (is_resolve ? LIT_INTERNAL_MAGIC_STRING_RESOLVE_FUNCTION
                                                : LIT_INTERNAL_MAGIC_STRING_REJECT_FUNCTION);

  ecma_value_t function = ecma_op_object_get_by_magic_id (ecma_get_object_from_value (promise), prop_name);

  ecma_value_t ret = ecma_op_function_call (ecma_get_object_from_value (function),
                                            ECMA_VALUE_UNDEFINED,
                                            &argument,
                                            1);

  ecma_free_value (function);

  return ret;
#else /* CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
  JERRY_UNUSED (promise);
  JERRY_UNUSED (argument);
  JERRY_UNUSED (is_resolve);

  return jerry_throw (ecma_raise_type_error (ECMA_ERR_MSG ("Promise not supported.")));
#endif /* !CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
} /* jerry_resolve_or_reject_promise */

/**
 * Validate UTF-8 string
 *
 * @return true - if UTF-8 string is well-formed
 *         false - otherwise
 */
bool
jerry_is_valid_utf8_string (const jerry_char_t *utf8_buf_p, /**< UTF-8 string */
                            jerry_size_t buf_size) /**< string size */
{
  return lit_is_valid_utf8_string ((lit_utf8_byte_t *) utf8_buf_p,
                                   (lit_utf8_size_t) buf_size);
} /* jerry_is_valid_utf8_string */

/**
 * Validate CESU-8 string
 *
 * @return true - if CESU-8 string is well-formed
 *         false - otherwise
 */
bool
jerry_is_valid_cesu8_string (const jerry_char_t *cesu8_buf_p, /**< CESU-8 string */
                             jerry_size_t buf_size) /**< string size */
{
  return lit_is_valid_cesu8_string ((lit_utf8_byte_t *) cesu8_buf_p,
                                    (lit_utf8_size_t) buf_size);
} /* jerry_is_valid_cesu8_string */

/*
 * Create a jerry instance for external context.
 *
 * @return the pointer to the instance.
 */
jerry_instance_t *
jerry_create_instance (uint32_t heap_size, /**< the size of heap */
                       jerry_instance_alloc_t alloc, /**< the alloc function */
                       void *cb_data_p) /**< the cb_data for alloc function */
{
  JERRY_UNUSED (heap_size);

#ifdef JERRY_ENABLE_EXTERNAL_CONTEXT

  size_t total_size = sizeof (jerry_instance_t) + JMEM_ALIGNMENT;

#ifndef JERRY_SYSTEM_ALLOCATOR
  heap_size = JERRY_ALIGNUP (heap_size, JMEM_ALIGNMENT);

  /* Minimum heap size is 1Kbyte. */
  if (heap_size < 1024)
  {
    return NULL;
  }

  total_size += heap_size;
#endif /* !JERRY_SYSTEM_ALLOCATOR */

#ifndef CONFIG_ECMA_LCACHE_DISABLE
  total_size += sizeof (jerry_hash_table_t);
#endif /* !CONFIG_ECMA_LCACHE_DISABLE */

  total_size = JERRY_ALIGNUP (total_size, JMEM_ALIGNMENT);

  jerry_instance_t *instance_p = (jerry_instance_t *) alloc (total_size, cb_data_p);

  if (instance_p == NULL)
  {
    return NULL;
  }

  memset (instance_p, 0, total_size);

  uintptr_t instance_ptr = ((uintptr_t) instance_p) + sizeof (jerry_instance_t);
  instance_ptr = JERRY_ALIGNUP (instance_ptr, (uintptr_t) JMEM_ALIGNMENT);

  uint8_t *byte_p = (uint8_t *) instance_ptr;

#ifndef JERRY_SYSTEM_ALLOCATOR
  instance_p->heap_p = (jmem_heap_t *) byte_p;
  instance_p->heap_size = heap_size;
  byte_p += heap_size;
#endif /* !JERRY_SYSTEM_ALLOCATOR */

#ifndef CONFIG_ECMA_LCACHE_DISABLE
  instance_p->lcache_p = byte_p;
  byte_p += sizeof (jerry_hash_table_t);
#endif /* !JERRY_SYSTEM_ALLOCATOR */

  JERRY_ASSERT (byte_p <= ((uint8_t *) instance_p) + total_size);

  JERRY_UNUSED (byte_p);
  return instance_p;

#else /* !JERRY_ENABLE_EXTERNAL_CONTEXT */

  JERRY_UNUSED (alloc);
  JERRY_UNUSED (cb_data_p);

  return NULL;

#endif /* JERRY_ENABLE_EXTERNAL_CONTEXT */
} /* jerry_create_instance */

/**
 * If JERRY_VM_EXEC_STOP is defined the callback passed to this function is
 * periodically called with the user_p argument. If frequency is greater
 * than 1, the callback is only called at every frequency ticks.
 */
void
jerry_set_vm_exec_stop_callback (jerry_vm_exec_stop_callback_t stop_cb, /**< periodically called user function */
                                 void *user_p, /**< pointer passed to the function */
                                 uint32_t frequency) /**< frequency of the function call */
{
#ifdef JERRY_VM_EXEC_STOP
  if (frequency == 0)
  {
    frequency = 1;
  }

  JERRY_CONTEXT (vm_exec_stop_frequency) = frequency;
  JERRY_CONTEXT (vm_exec_stop_counter) = frequency;
  JERRY_CONTEXT (vm_exec_stop_user_p) = user_p;
  JERRY_CONTEXT (vm_exec_stop_cb) = stop_cb;
#else /* !JERRY_VM_EXEC_STOP */
  JERRY_UNUSED (stop_cb);
  JERRY_UNUSED (user_p);
  JERRY_UNUSED (frequency);
#endif /* JERRY_VM_EXEC_STOP */
} /* jerry_set_vm_exec_stop_callback */

/**
 * @}
 */
