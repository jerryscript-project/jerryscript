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

#include "jerryscript.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>

#include "jerryscript-debugger-transport.h"

#include "ecma-alloc.h"
#include "ecma-array-object.h"
#include "ecma-arraybuffer-object.h"
#include "ecma-bigint.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-comparison.h"
#include "ecma-container-object.h"
#include "ecma-dataview-object.h"
#include "ecma-errors.h"
#include "ecma-eval.h"
#include "ecma-exceptions.h"
#include "ecma-extended-info.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-init-finalize.h"
#include "ecma-iterator-object.h"
#include "ecma-lex-env.h"
#include "ecma-line-info.h"
#include "ecma-literal-storage.h"
#include "ecma-objects-general.h"
#include "ecma-objects.h"
#include "ecma-promise-object.h"
#include "ecma-proxy-object.h"
#include "ecma-regexp-object.h"
#include "ecma-shared-arraybuffer-object.h"
#include "ecma-symbol-object.h"
#include "ecma-typedarray-object.h"

#include "debugger.h"
#include "jcontext.h"
#include "jmem.h"
#include "jrt.h"
#include "js-parser.h"
#include "lit-char-helpers.h"
#include "opcodes.h"
#include "re-compiler.h"

JERRY_STATIC_ASSERT (sizeof (jerry_value_t) == sizeof (ecma_value_t),
                     size_of_jerry_value_t_must_be_equal_to_size_of_ecma_value_t);

#if JERRY_BUILTIN_REGEXP
JERRY_STATIC_ASSERT ((int) RE_FLAG_GLOBAL == (int) JERRY_REGEXP_FLAG_GLOBAL
                       && (int) RE_FLAG_MULTILINE == (int) JERRY_REGEXP_FLAG_MULTILINE
                       && (int) RE_FLAG_IGNORE_CASE == (int) JERRY_REGEXP_FLAG_IGNORE_CASE
                       && (int) RE_FLAG_STICKY == (int) JERRY_REGEXP_FLAG_STICKY
                       && (int) RE_FLAG_UNICODE == (int) JERRY_REGEXP_FLAG_UNICODE
                       && (int) RE_FLAG_DOTALL == (int) JERRY_REGEXP_FLAG_DOTALL,
                     re_flags_t_must_be_equal_to_jerry_regexp_flags_t);
#endif /* JERRY_BUILTIN_REGEXP */

/* The internal ECMA_PROMISE_STATE_* values are "one byte away" from the API values */
JERRY_STATIC_ASSERT ((int) ECMA_PROMISE_IS_PENDING == (int) JERRY_PROMISE_STATE_PENDING
                       && (int) ECMA_PROMISE_IS_FULFILLED == (int) JERRY_PROMISE_STATE_FULFILLED,
                     promise_internal_state_matches_external);

/**
 * Offset between internal and external arithmetic operator types
 */
#define ECMA_NUMBER_ARITHMETIC_OP_API_OFFSET (JERRY_BIN_OP_SUB - NUMBER_ARITHMETIC_SUBTRACTION)

JERRY_STATIC_ASSERT (((NUMBER_ARITHMETIC_SUBTRACTION + ECMA_NUMBER_ARITHMETIC_OP_API_OFFSET) == JERRY_BIN_OP_SUB)
                       && ((NUMBER_ARITHMETIC_MULTIPLICATION + ECMA_NUMBER_ARITHMETIC_OP_API_OFFSET)
                           == JERRY_BIN_OP_MUL)
                       && ((NUMBER_ARITHMETIC_DIVISION + ECMA_NUMBER_ARITHMETIC_OP_API_OFFSET) == JERRY_BIN_OP_DIV)
                       && ((NUMBER_ARITHMETIC_REMAINDER + ECMA_NUMBER_ARITHMETIC_OP_API_OFFSET) == JERRY_BIN_OP_REM),
                     number_arithmetics_operation_type_matches_external);

#if !JERRY_PARSER && !JERRY_SNAPSHOT_EXEC
#error "JERRY_SNAPSHOT_EXEC must be enabled if JERRY_PARSER is disabled!"
#endif /* !JERRY_PARSER && !JERRY_SNAPSHOT_EXEC */

/** \addtogroup jerry Jerry engine interface
 * @{
 */

/**
 * Assert that it is correct to call API in current state.
 *
 * Note:
 *         By convention, there are some states when API could not be invoked.
 *
 *         The API can be and only be invoked when the ECMA_STATUS_API_ENABLED
 *         flag is set.
 *
 *         This procedure checks whether the API is available, and terminates
 *         the engine if it is unavailable. Otherwise it is a no-op.
 *
 * Note:
 *         The API could not be invoked in the following cases:
 *           - before jerry_init and after jerry_cleanup
 *           - between enter to and return from a native free callback
 */
static inline void JERRY_ATTR_ALWAYS_INLINE
jerry_assert_api_enabled (void)
{
  JERRY_ASSERT (JERRY_CONTEXT (status_flags) & ECMA_STATUS_API_ENABLED);
} /* jerry_assert_api_enabled */

/**
 * Turn on API availability
 */
static inline void JERRY_ATTR_ALWAYS_INLINE
jerry_api_enable (void)
{
#ifndef JERRY_NDEBUG
  JERRY_CONTEXT (status_flags) |= ECMA_STATUS_API_ENABLED;
#endif /* JERRY_NDEBUG */
} /* jerry_make_api_available */

/**
 * Turn off API availability
 */
static inline void JERRY_ATTR_ALWAYS_INLINE
jerry_api_disable (void)
{
#ifndef JERRY_NDEBUG
  JERRY_CONTEXT (status_flags) &= (uint32_t) ~ECMA_STATUS_API_ENABLED;
#endif /* JERRY_NDEBUG */
} /* jerry_make_api_unavailable */

/**
 * Create an API compatible return value.
 *
 * @return return value for Jerry API functions
 */
static jerry_value_t
jerry_return (const jerry_value_t value) /**< return value */
{
  if (ECMA_IS_VALUE_ERROR (value))
  {
    return ecma_create_exception_from_context ();
  }

  return value;
} /* jerry_return */

/**
 * Jerry engine initialization
 */
void
jerry_init (jerry_init_flag_t flags) /**< combination of Jerry flags */
{
  jerry_port_init ();
#if JERRY_EXTERNAL_CONTEXT
  size_t total_size = jerry_port_context_alloc (sizeof (jerry_context_t));
  JERRY_UNUSED (total_size);
#endif /* JERRY_EXTERNAL_CONTEXT */

  jerry_context_t *context_p = &JERRY_CONTEXT_STRUCT;
  memset (context_p, 0, sizeof (jerry_context_t));

#if JERRY_EXTERNAL_CONTEXT && !JERRY_SYSTEM_ALLOCATOR
  uint32_t heap_start_offset = JERRY_ALIGNUP (sizeof (jerry_context_t), JMEM_ALIGNMENT);
  uint8_t *heap_p = ((uint8_t *) context_p) + heap_start_offset;
  uint32_t heap_size = JERRY_ALIGNDOWN (total_size - heap_start_offset, JMEM_ALIGNMENT);

  JERRY_ASSERT (heap_p + heap_size <= ((uint8_t *) context_p) + total_size);

  context_p->heap_p = (jmem_heap_t *) heap_p;
  context_p->heap_size = heap_size;
#endif /* JERRY_EXTERNAL_CONTEXT && !JERRY_SYSTEM_ALLOCATOR */

  JERRY_CONTEXT (jerry_init_flags) = flags;

  jerry_api_enable ();

  jmem_init ();
  ecma_init ();
} /* jerry_init */

/**
 * Terminate Jerry engine
 */
void
jerry_cleanup (void)
{
  jerry_assert_api_enabled ();

#if JERRY_DEBUGGER
  if (JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED)
  {
    jerry_debugger_send_type (JERRY_DEBUGGER_CLOSE_CONNECTION);

    jerry_debugger_transport_close ();
  }
#endif /* JERRY_DEBUGGER */

  for (jerry_context_data_header_t *this_p = JERRY_CONTEXT (context_data_p); this_p != NULL; this_p = this_p->next_p)
  {
    if (this_p->manager_p->deinit_cb)
    {
      void *data = (this_p->manager_p->bytes_needed > 0) ? JERRY_CONTEXT_DATA_HEADER_USER_DATA (this_p) : NULL;
      this_p->manager_p->deinit_cb (data);
    }
  }

  ecma_free_all_enqueued_jobs ();
  ecma_finalize ();
  jerry_api_disable ();

  for (jerry_context_data_header_t *this_p = JERRY_CONTEXT (context_data_p), *next_p = NULL; this_p != NULL;
       this_p = next_p)
  {
    next_p = this_p->next_p;

    if (this_p->manager_p->finalize_cb)
    {
      void *data = (this_p->manager_p->bytes_needed > 0) ? JERRY_CONTEXT_DATA_HEADER_USER_DATA (this_p) : NULL;
      this_p->manager_p->finalize_cb (data);
    }

    jmem_heap_free_block (this_p, sizeof (jerry_context_data_header_t) + this_p->manager_p->bytes_needed);
  }

  jmem_finalize ();
#if JERRY_EXTERNAL_CONTEXT
  jerry_port_context_free ();
#endif /* JERRY_EXTERNAL_CONTEXT */
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
jerry_context_data (const jerry_context_data_manager_t *manager_p)
{
  void *ret = NULL;
  jerry_context_data_header_t *item_p;

  for (item_p = JERRY_CONTEXT (context_data_p); item_p != NULL; item_p = item_p->next_p)
  {
    if (item_p->manager_p == manager_p)
    {
      return (manager_p->bytes_needed > 0) ? JERRY_CONTEXT_DATA_HEADER_USER_DATA (item_p) : NULL;
    }
  }

  item_p = jmem_heap_alloc_block (sizeof (jerry_context_data_header_t) + manager_p->bytes_needed);
  item_p->manager_p = manager_p;
  item_p->next_p = JERRY_CONTEXT (context_data_p);
  JERRY_CONTEXT (context_data_p) = item_p;

  if (manager_p->bytes_needed > 0)
  {
    ret = JERRY_CONTEXT_DATA_HEADER_USER_DATA (item_p);
    memset (ret, 0, manager_p->bytes_needed);
  }

  if (manager_p->init_cb)
  {
    manager_p->init_cb (ret);
  }

  return ret;
} /* jerry_context_data */

/**
 * Register external magic string array
 */
void
jerry_register_magic_strings (const jerry_char_t *const *ext_strings_p, /**< character arrays, representing
                                                                         *   external magic strings' contents */
                              uint32_t count, /**< number of the strings */
                              const jerry_length_t *str_lengths_p) /**< lengths of all strings */
{
  jerry_assert_api_enabled ();

  lit_magic_strings_ex_set ((const lit_utf8_byte_t *const *) ext_strings_p,
                            count,
                            (const lit_utf8_size_t *) str_lengths_p);
} /* jerry_register_magic_strings */

/**
 * Run garbage collection
 */
void
jerry_heap_gc (jerry_gc_mode_t mode) /**< operational mode */
{
  jerry_assert_api_enabled ();

  if (mode == JERRY_GC_PRESSURE_LOW)
  {
    /* Call GC directly, because 'ecma_free_unused_memory' might decide it's not yet worth it. */
    ecma_gc_run ();
    return;
  }

  ecma_free_unused_memory (JMEM_PRESSURE_HIGH);
} /* jerry_heap_gc */

/**
 * Get heap memory stats.
 *
 * @return true - get the heap stats successful
 *         false - otherwise. Usually it is because the MEM_STATS feature is not enabled.
 */
bool
jerry_heap_stats (jerry_heap_stats_t *out_stats_p) /**< [out] heap memory stats */
{
#if JERRY_MEM_STATS
  if (out_stats_p == NULL)
  {
    return false;
  }

  jmem_heap_stats_t jmem_heap_stats;
  memset (&jmem_heap_stats, 0, sizeof (jmem_heap_stats));
  jmem_heap_get_stats (&jmem_heap_stats);

  *out_stats_p = (jerry_heap_stats_t){ .version = 1,
                                       .size = jmem_heap_stats.size,
                                       .allocated_bytes = jmem_heap_stats.allocated_bytes,
                                       .peak_allocated_bytes = jmem_heap_stats.peak_allocated_bytes };

  return true;
#else /* !JERRY_MEM_STATS */
  JERRY_UNUSED (out_stats_p);
  return false;
#endif /* JERRY_MEM_STATS */
} /* jerry_heap_stats */

#if JERRY_PARSER
/**
 * Common code for parsing a script, module, or function.
 *
 * @return function object value - if script was parsed successfully,
 *         thrown error - otherwise
 */
static jerry_value_t
jerry_parse_common (void *source_p, /**< script source */
                    const jerry_parse_options_t *options_p, /**< parsing options, can be NULL if not used */
                    uint32_t parse_opts) /**< internal parsing options */
{
  jerry_assert_api_enabled ();

  if (options_p != NULL)
  {
    const uint32_t allowed_options =
      (JERRY_PARSE_STRICT_MODE | JERRY_PARSE_MODULE | JERRY_PARSE_HAS_ARGUMENT_LIST | JERRY_PARSE_HAS_SOURCE_NAME
       | JERRY_PARSE_HAS_START | JERRY_PARSE_HAS_USER_VALUE);
    uint32_t options = options_p->options;

    if ((options & ~allowed_options) != 0
        || ((options_p->options & JERRY_PARSE_HAS_ARGUMENT_LIST)
            && ((options_p->options & JERRY_PARSE_MODULE) || !ecma_is_value_string (options_p->argument_list)))
        || ((options_p->options & JERRY_PARSE_HAS_SOURCE_NAME) && !ecma_is_value_string (options_p->source_name)))
    {
      return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
    }
  }

#if JERRY_DEBUGGER
  if ((JERRY_CONTEXT (debugger_flags) & JERRY_DEBUGGER_CONNECTED) && options_p != NULL
      && (options_p->options & JERRY_PARSE_HAS_SOURCE_NAME) && ecma_is_value_string (options_p->source_name))
  {
    ECMA_STRING_TO_UTF8_STRING (ecma_get_string_from_value (options_p->source_name),
                                source_name_start_p,
                                source_name_size);
    jerry_debugger_send_string (JERRY_DEBUGGER_SOURCE_CODE_NAME,
                                JERRY_DEBUGGER_NO_SUBTYPE,
                                source_name_start_p,
                                source_name_size);
    ECMA_FINALIZE_UTF8_STRING (source_name_start_p, source_name_size);
  }
#endif /* JERRY_DEBUGGER */

  if (options_p != NULL)
  {
    parse_opts |= options_p->options & (JERRY_PARSE_STRICT_MODE | JERRY_PARSE_MODULE);
  }

  if ((parse_opts & JERRY_PARSE_MODULE) != 0)
  {
#if JERRY_MODULE_SYSTEM
    JERRY_CONTEXT (module_current_p) = ecma_module_create ();
#else /* !JERRY_MODULE_SYSTEM */
    return jerry_throw_sz (JERRY_ERROR_SYNTAX, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JERRY_MODULE_SYSTEM */
  }

  ecma_compiled_code_t *bytecode_data_p;
  bytecode_data_p = parser_parse_script (source_p, parse_opts, options_p);

  if (JERRY_UNLIKELY (bytecode_data_p == NULL))
  {
#if JERRY_MODULE_SYSTEM
    if ((parse_opts & JERRY_PARSE_MODULE) != 0)
    {
      ecma_module_cleanup_context ();
    }
#endif /* JERRY_MODULE_SYSTEM */

    return ecma_create_exception_from_context ();
  }

#if JERRY_MODULE_SYSTEM
  if (JERRY_UNLIKELY (parse_opts & JERRY_PARSE_MODULE))
  {
    ecma_module_t *module_p = JERRY_CONTEXT (module_current_p);
    module_p->u.compiled_code_p = bytecode_data_p;

    JERRY_CONTEXT (module_current_p) = NULL;

    return ecma_make_object_value ((ecma_object_t *) module_p);
  }
#endif /* JERRY_MODULE_SYSTEM */

  if (JERRY_UNLIKELY (options_p != NULL && (options_p->options & JERRY_PARSE_HAS_ARGUMENT_LIST)))
  {
    ecma_object_t *global_object_p = ecma_builtin_get_global ();

#if JERRY_BUILTIN_REALMS
    JERRY_ASSERT (global_object_p == (ecma_object_t *) ecma_op_function_get_realm (bytecode_data_p));
#endif /* JERRY_BUILTIN_REALMS */

    ecma_object_t *lex_env_p = ecma_get_global_environment (global_object_p);
    ecma_object_t *func_obj_p = ecma_op_create_simple_function_object (lex_env_p, bytecode_data_p);
    ecma_bytecode_deref (bytecode_data_p);

    return ecma_make_object_value (func_obj_p);
  }

  ecma_object_t *object_p = ecma_create_object (NULL, sizeof (ecma_extended_object_t), ECMA_OBJECT_TYPE_CLASS);

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  ext_object_p->u.cls.type = ECMA_OBJECT_CLASS_SCRIPT;
  ECMA_SET_INTERNAL_VALUE_POINTER (ext_object_p->u.cls.u3.value, bytecode_data_p);

  return ecma_make_object_value (object_p);
} /* jerry_parse_common */

#endif /* JERRY_PARSER */

/**
 * Parse a script, module, or function and create a compiled code using a character string
 *
 * @return function object value - if script was parsed successfully,
 *         thrown error - otherwise
 */
jerry_value_t
jerry_parse (const jerry_char_t *source_p, /**< script source */
             size_t source_size, /**< script source size */
             const jerry_parse_options_t *options_p) /**< parsing options, can be NULL if not used */
{
#if JERRY_PARSER
  parser_source_char_t source_char;
  source_char.source_p = source_p;
  source_char.source_size = source_size;

  return jerry_parse_common ((void *) &source_char, options_p, JERRY_PARSE_NO_OPTS);
#else /* !JERRY_PARSER */
  JERRY_UNUSED (source_p);
  JERRY_UNUSED (source_size);
  JERRY_UNUSED (options_p);

  return jerry_throw_sz (JERRY_ERROR_SYNTAX, ecma_get_error_msg (ECMA_ERR_PARSER_NOT_SUPPORTED));
#endif /* JERRY_PARSER */
} /* jerry_parse */

/**
 * Parse a script, module, or function and create a compiled code using a string value
 *
 * @return function object value - if script was parsed successfully,
 *         thrown error - otherwise
 */
jerry_value_t
jerry_parse_value (const jerry_value_t source, /**< script source */
                   const jerry_parse_options_t *options_p) /**< parsing options, can be NULL if not used */
{
#if JERRY_PARSER
  if (!ecma_is_value_string (source))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  return jerry_parse_common ((void *) &source, options_p, ECMA_PARSE_HAS_SOURCE_VALUE);
#else /* !JERRY_PARSER */
  JERRY_UNUSED (source);
  JERRY_UNUSED (options_p);

  return jerry_throw_sz (JERRY_ERROR_SYNTAX, ecma_get_error_msg (ECMA_ERR_PARSER_NOT_SUPPORTED));
#endif /* JERRY_PARSER */
} /* jerry_parse_value */

/**
 * Run a Script or Module created by jerry_parse.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return result of bytecode - if run was successful
 *         thrown error - otherwise
 */
jerry_value_t
jerry_run (const jerry_value_t script) /**< script or module to run */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (script))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_object_t *object_p = ecma_get_object_from_value (script);

  if (!ecma_object_class_is (object_p, ECMA_OBJECT_CLASS_SCRIPT))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

  const ecma_compiled_code_t *bytecode_data_p;
  bytecode_data_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_compiled_code_t, ext_object_p->u.cls.u3.value);

  JERRY_ASSERT (CBC_FUNCTION_GET_TYPE (bytecode_data_p->status_flags) == CBC_FUNCTION_SCRIPT);

  return jerry_return (vm_run_global (bytecode_data_p, object_p));
} /* jerry_run */

/**
 * Perform eval
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return result of eval, may be error value.
 */
jerry_value_t
jerry_eval (const jerry_char_t *source_p, /**< source code */
            size_t source_size, /**< length of source code */
            uint32_t flags) /**< jerry_parse_opts_t flags */
{
  jerry_assert_api_enabled ();

  uint32_t allowed_parse_options = JERRY_PARSE_STRICT_MODE;

  if ((flags & ~allowed_parse_options) != 0)
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  parser_source_char_t source_char;
  source_char.source_p = source_p;
  source_char.source_size = source_size;

  return jerry_return (ecma_op_eval_chars_buffer ((void *) &source_char, flags));
} /* jerry_eval */

/**
 * Link modules to their dependencies. The dependencies are resolved by a user callback.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return true - if linking is successful, error - otherwise
 */
jerry_value_t
jerry_module_link (const jerry_value_t module, /**< root module */
                   jerry_module_resolve_cb_t callback, /**< resolve module callback, uses
                                                        *   jerry_module_resolve when NULL is passed */
                   void *user_p) /**< pointer passed to the resolve callback */
{
  jerry_assert_api_enabled ();

#if JERRY_MODULE_SYSTEM
  if (callback == NULL)
  {
    callback = jerry_module_resolve;
  }

  ecma_module_t *module_p = ecma_module_get_resolved_module (module);

  if (module_p == NULL)
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_NOT_MODULE));
  }

  return jerry_return (ecma_module_link (module_p, callback, user_p));
#else /* !JERRY_MODULE_SYSTEM */
  JERRY_UNUSED (module);
  JERRY_UNUSED (callback);
  JERRY_UNUSED (user_p);

  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JERRY_MODULE_SYSTEM */
} /* jerry_module_link */

/**
 * Evaluate a module and its dependencies. The module must be in linked state.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return result of module bytecode execution - if evaluation was successful
 *         error - otherwise
 */
jerry_value_t
jerry_module_evaluate (const jerry_value_t module) /**< root module */
{
  jerry_assert_api_enabled ();

#if JERRY_MODULE_SYSTEM
  ecma_module_t *module_p = ecma_module_get_resolved_module (module);

  if (module_p == NULL)
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_NOT_MODULE));
  }

  if (module_p->header.u.cls.u1.module_state != JERRY_MODULE_STATE_LINKED)
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_MUST_BE_IN_LINKED_STATE));
  }

  return jerry_return (ecma_module_evaluate (module_p));
#else /* !JERRY_MODULE_SYSTEM */
  JERRY_UNUSED (module);

  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JERRY_MODULE_SYSTEM */
} /* jerry_module_evaluate */

/**
 * Returns the current status of a module
 *
 * @return current status - if module is a module,
 *         JERRY_MODULE_STATE_INVALID - otherwise
 */
jerry_module_state_t
jerry_module_state (const jerry_value_t module) /**< module object */
{
  jerry_assert_api_enabled ();

#if JERRY_MODULE_SYSTEM
  ecma_module_t *module_p = ecma_module_get_resolved_module (module);

  if (module_p == NULL)
  {
    return JERRY_MODULE_STATE_INVALID;
  }

  return (jerry_module_state_t) module_p->header.u.cls.u1.module_state;
#else /* !JERRY_MODULE_SYSTEM */
  JERRY_UNUSED (module);

  return JERRY_MODULE_STATE_INVALID;
#endif /* JERRY_MODULE_SYSTEM */
} /* jerry_module_state */

/**
 * Sets a callback which is called after a module state is changed to linked, evaluated, or error.
 */
void
jerry_module_on_state_changed (jerry_module_state_changed_cb_t callback, /**< callback */
                               void *user_p) /**< pointer passed to the callback */
{
  jerry_assert_api_enabled ();

#if JERRY_MODULE_SYSTEM
  JERRY_CONTEXT (module_state_changed_callback_p) = callback;
  JERRY_CONTEXT (module_state_changed_callback_user_p) = user_p;
#else /* !JERRY_MODULE_SYSTEM */
  JERRY_UNUSED (callback);
  JERRY_UNUSED (user_p);
#endif /* JERRY_MODULE_SYSTEM */
} /* jerry_module_on_state_changed */

/**
 * Sets a callback which is called when an import.meta expression of a module is evaluated the first time.
 */
void
jerry_module_on_import_meta (jerry_module_import_meta_cb_t callback, /**< callback */
                             void *user_p) /**< pointer passed to the callback */
{
  jerry_assert_api_enabled ();

#if JERRY_MODULE_SYSTEM
  JERRY_CONTEXT (module_import_meta_callback_p) = callback;
  JERRY_CONTEXT (module_import_meta_callback_user_p) = user_p;
#else /* !JERRY_MODULE_SYSTEM */
  JERRY_UNUSED (callback);
  JERRY_UNUSED (user_p);
#endif /* JERRY_MODULE_SYSTEM */
} /* jerry_module_on_import_meta */

/**
 * Returns the number of import/export requests of a module
 *
 * @return number of import/export requests of a module
 */
size_t
jerry_module_request_count (const jerry_value_t module) /**< module */
{
  jerry_assert_api_enabled ();

#if JERRY_MODULE_SYSTEM
  ecma_module_t *module_p = ecma_module_get_resolved_module (module);

  if (module_p == NULL)
  {
    return 0;
  }

  size_t number_of_requests = 0;

  ecma_module_node_t *node_p = module_p->imports_p;

  while (node_p != NULL)
  {
    number_of_requests++;
    node_p = node_p->next_p;
  }

  return number_of_requests;
#else /* !JERRY_MODULE_SYSTEM */
  JERRY_UNUSED (module);

  return 0;
#endif /* JERRY_MODULE_SYSTEM */
} /* jerry_module_request_count */

/**
 * Returns the module request specified by the request_index argument
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return string - if the request has not been resolved yet,
 *         module object - if the request has been resolved successfully,
 *         error - otherwise
 */
jerry_value_t
jerry_module_request (const jerry_value_t module, /**< module */
                      size_t request_index) /**< request index */
{
  jerry_assert_api_enabled ();

#if JERRY_MODULE_SYSTEM
  ecma_module_t *module_p = ecma_module_get_resolved_module (module);

  if (module_p == NULL)
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_NOT_MODULE));
  }

  ecma_module_node_t *node_p = module_p->imports_p;

  while (node_p != NULL)
  {
    if (request_index == 0)
    {
      return ecma_copy_value (node_p->u.path_or_module);
    }

    --request_index;
    node_p = node_p->next_p;
  }

  return jerry_throw_sz (JERRY_ERROR_RANGE, ecma_get_error_msg (ECMA_ERR_REQUEST_IS_NOT_AVAILABLE));
#else /* !JERRY_MODULE_SYSTEM */
  JERRY_UNUSED (module);
  JERRY_UNUSED (request_index);

  return jerry_throw_sz (JERRY_ERROR_RANGE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JERRY_MODULE_SYSTEM */
} /* jerry_module_request */

/**
 * Returns the namespace object of a module
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return object - if namespace object is available,
 *         error - otherwise
 */
jerry_value_t
jerry_module_namespace (const jerry_value_t module) /**< module */
{
  jerry_assert_api_enabled ();

#if JERRY_MODULE_SYSTEM
  ecma_module_t *module_p = ecma_module_get_resolved_module (module);

  if (module_p == NULL)
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_NOT_MODULE));
  }

  if (module_p->header.u.cls.u1.module_state < JERRY_MODULE_STATE_LINKED
      || module_p->header.u.cls.u1.module_state > JERRY_MODULE_STATE_EVALUATED)
  {
    return jerry_throw_sz (JERRY_ERROR_RANGE, ecma_get_error_msg (ECMA_ERR_NAMESPACE_OBJECT_IS_NOT_AVAILABLE));
  }

  JERRY_ASSERT (module_p->namespace_object_p != NULL);

  ecma_ref_object (module_p->namespace_object_p);
  return ecma_make_object_value (module_p->namespace_object_p);
#else /* !JERRY_MODULE_SYSTEM */
  JERRY_UNUSED (module);

  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JERRY_MODULE_SYSTEM */
} /* jerry_module_namespace */

/**
 * Sets the callback which is called when dynamic imports are resolved
 */
void
jerry_module_on_import (jerry_module_import_cb_t callback_p, /**< callback which handles
                                                              *   dynamic import calls */
                        void *user_p) /**< user pointer passed to the callback */
{
  jerry_assert_api_enabled ();

#if JERRY_MODULE_SYSTEM
  JERRY_CONTEXT (module_import_callback_p) = callback_p;
  JERRY_CONTEXT (module_import_callback_user_p) = user_p;
#else /* !JERRY_MODULE_SYSTEM */
  JERRY_UNUSED (callback_p);
  JERRY_UNUSED (user_p);
#endif /* JERRY_MODULE_SYSTEM */
} /* jerry_module_on_import */

/**
 * Creates a native module with a list of exports. The initial state of the module is linked.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return native module - if the module is successfully created,
 *         error - otherwise
 */
jerry_value_t
jerry_native_module (jerry_native_module_evaluate_cb_t callback, /**< evaluation callback for
                                                                  *   native modules */
                     const jerry_value_t *const exports_p, /**< list of the exported bindings of the module,
                                                            *   must be valid string identifiers */
                     size_t export_count) /**< number of exports in the exports_p list */
{
  jerry_assert_api_enabled ();

#if JERRY_MODULE_SYSTEM
  ecma_object_t *global_object_p = ecma_builtin_get_global ();
  ecma_object_t *scope_p = ecma_create_decl_lex_env (ecma_get_global_environment (global_object_p));
  ecma_module_names_t *local_exports_p = NULL;

  for (size_t i = 0; i < export_count; i++)
  {
    if (!ecma_is_value_string (exports_p[i]))
    {
      ecma_deref_object (scope_p);
      ecma_module_release_module_names (local_exports_p);
      return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_EXPORTS_MUST_BE_STRING_VALUES));
    }

    ecma_string_t *name_str_p = ecma_get_string_from_value (exports_p[i]);

    bool valid_identifier = false;

    ECMA_STRING_TO_UTF8_STRING (name_str_p, name_start_p, name_size);

    if (name_size > 0)
    {
      const lit_utf8_byte_t *name_p = name_start_p;
      const lit_utf8_byte_t *name_end_p = name_start_p + name_size;
      lit_code_point_t code_point;

      lit_utf8_size_t size = lit_read_code_point_from_cesu8 (name_p, name_end_p, &code_point);

      if (lit_code_point_is_identifier_start (code_point))
      {
        name_p += size;

        valid_identifier = true;

        while (name_p < name_end_p)
        {
          size = lit_read_code_point_from_cesu8 (name_p, name_end_p, &code_point);

          if (!lit_code_point_is_identifier_part (code_point))
          {
            valid_identifier = false;
            break;
          }

          name_p += size;
        }
      }
    }

    ECMA_FINALIZE_UTF8_STRING (name_start_p, name_size);

    if (!valid_identifier)
    {
      ecma_deref_object (scope_p);
      ecma_module_release_module_names (local_exports_p);
      return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_EXPORTS_MUST_BE_VALID_IDENTIFIERS));
    }

    if (ecma_find_named_property (scope_p, name_str_p) != NULL)
    {
      continue;
    }

    ecma_create_named_data_property (scope_p, name_str_p, ECMA_PROPERTY_FLAG_WRITABLE, NULL);

    ecma_module_names_t *new_export_p;
    new_export_p = (ecma_module_names_t *) jmem_heap_alloc_block (sizeof (ecma_module_names_t));

    new_export_p->next_p = local_exports_p;
    local_exports_p = new_export_p;

    ecma_ref_ecma_string (name_str_p);
    new_export_p->imex_name_p = name_str_p;

    ecma_ref_ecma_string (name_str_p);
    new_export_p->local_name_p = name_str_p;
  }

  ecma_module_t *module_p = ecma_module_create ();

  module_p->header.u.cls.u2.module_flags |= ECMA_MODULE_IS_NATIVE;
  module_p->scope_p = scope_p;
  module_p->local_exports_p = local_exports_p;
  module_p->u.callback = callback;

  ecma_deref_object (scope_p);

  return ecma_make_object_value (&module_p->header.object);

#else /* !JERRY_MODULE_SYSTEM */
  JERRY_UNUSED (callback);
  JERRY_UNUSED (exports_p);
  JERRY_UNUSED (export_count);

  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JERRY_MODULE_SYSTEM */
} /* jerry_native_module */

/**
 * Gets the value of an export which belongs to a native module.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return value of the export - if success
 *         error - otherwise
 */
jerry_value_t
jerry_native_module_get (const jerry_value_t native_module, /**< a native module object */
                         const jerry_value_t export_name) /**< string identifier of the export */
{
  jerry_assert_api_enabled ();

#if JERRY_MODULE_SYSTEM
  ecma_module_t *module_p = ecma_module_get_resolved_module (native_module);

  if (module_p == NULL)
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_NOT_MODULE));
  }

  if (!(module_p->header.u.cls.u2.module_flags & ECMA_MODULE_IS_NATIVE) || !ecma_is_value_string (export_name))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_property_t *property_p = ecma_find_named_property (module_p->scope_p, ecma_get_string_from_value (export_name));

  if (property_p == NULL)
  {
    return jerry_throw_sz (JERRY_ERROR_REFERENCE, ecma_get_error_msg (ECMA_ERR_UNKNOWN_EXPORT));
  }

  return ecma_copy_value (ECMA_PROPERTY_VALUE_PTR (property_p)->value);
#else /* !JERRY_MODULE_SYSTEM */
  JERRY_UNUSED (native_module);
  JERRY_UNUSED (export_name);

  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JERRY_MODULE_SYSTEM */
} /* jerry_native_module_get */

/**
 * Sets the value of an export which belongs to a native module.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return true value - if the operation was successful
 *         error - otherwise
 */
jerry_value_t
jerry_native_module_set (jerry_value_t native_module, /**< a native module object */
                         const jerry_value_t export_name, /**< string identifier of the export */
                         const jerry_value_t value) /**< new value of the export */
{
  jerry_assert_api_enabled ();

#if JERRY_MODULE_SYSTEM
  ecma_module_t *module_p = ecma_module_get_resolved_module (native_module);

  if (module_p == NULL)
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_NOT_MODULE));
  }

  if (!(module_p->header.u.cls.u2.module_flags & ECMA_MODULE_IS_NATIVE) || !ecma_is_value_string (export_name)
      || ecma_is_value_exception (value))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_property_t *property_p = ecma_find_named_property (module_p->scope_p, ecma_get_string_from_value (export_name));

  if (property_p == NULL)
  {
    return jerry_throw_sz (JERRY_ERROR_REFERENCE, ecma_get_error_msg (ECMA_ERR_UNKNOWN_EXPORT));
  }

  ecma_named_data_property_assign_value (module_p->scope_p, ECMA_PROPERTY_VALUE_PTR (property_p), value);
  return ECMA_VALUE_TRUE;
#else /* !JERRY_MODULE_SYSTEM */
  JERRY_UNUSED (native_module);
  JERRY_UNUSED (export_name);
  JERRY_UNUSED (value);

  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_MODULE_NOT_SUPPORTED));
#endif /* JERRY_MODULE_SYSTEM */
} /* jerry_native_module_set */

/**
 * Run enqueued microtasks created by Promise or AsyncFunction objects.
 * Tasks are executed until an exception is thrown or all tasks are executed.
 *
 * Note: returned value must be freed with jerry_value_free
 *
 * @return result of last executed job, possibly an exception.
 */
jerry_value_t
jerry_run_jobs (void)
{
  jerry_assert_api_enabled ();

  return jerry_return (ecma_process_all_enqueued_jobs ());
} /* jerry_run_jobs */

/**
 * Get global object
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return api value of global object
 */
jerry_value_t
jerry_current_realm (void)
{
  jerry_assert_api_enabled ();
  ecma_object_t *global_obj_p = ecma_builtin_get_global ();
  ecma_ref_object (global_obj_p);
  return ecma_make_object_value (global_obj_p);
} /* jerry_current_realm */

/**
 * Check if the specified value is an abort value.
 *
 * @return true  - if both the error and abort values are set,
 *         false - otherwise
 */
bool
jerry_value_is_abort (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_exception (value))
  {
    return false;
  }

  ecma_extended_primitive_t *error_ref_p = ecma_get_extended_primitive_from_value (value);

  return (error_ref_p->refs_and_type & ECMA_ERROR_API_FLAG_ABORT) != 0;
} /* jerry_value_is_abort */

/**
 * Check if the specified value is an array object value.
 *
 * @return true  - if the specified value is an array object,
 *         false - otherwise
 */
bool
jerry_value_is_array (const jerry_value_t value) /**< jerry api value */
{
  jerry_assert_api_enabled ();

  return (ecma_is_value_object (value)
          && ecma_get_object_base_type (ecma_get_object_from_value (value)) == ECMA_OBJECT_BASE_TYPE_ARRAY);
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
  jerry_assert_api_enabled ();

  return ecma_is_value_boolean (value);
} /* jerry_value_is_boolean */

/**
 * Check if the specified value is true.
 *
 * @return true  - if the specified value is true
 *         false - otherwise
 */
bool
jerry_value_is_true (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_enabled ();

  return ecma_is_value_true (value);
} /* jerry_value_is_true */

/**
 * Check if the specified value is false.
 *
 * @return true  - if the specified value is false
 *         false - otherwise
 */
bool
jerry_value_is_false (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_enabled ();

  return ecma_is_value_false (value);
} /* jerry_value_is_false */

/**
 * Check if the specified value is a constructor function object value.
 *
 * @return true - if the specified value is a function value that implements [[Construct]],
 *         false - otherwise
 */
bool
jerry_value_is_constructor (const jerry_value_t value) /**< jerry api value */
{
  jerry_assert_api_enabled ();

  return ecma_is_constructor (value);
} /* jerry_value_is_constructor */

/**
 * Check if the specified value is an error or abort value.
 *
 * @return true  - if the specified value is an error value,
 *         false - otherwise
 */
bool
jerry_value_is_exception (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_enabled ();

  return ecma_is_value_exception (value);
} /* jerry_value_is_exception */

/**
 * Check if the specified value is a function object value.
 *
 * @return true - if the specified value is callable,
 *         false - otherwise
 */
bool
jerry_value_is_function (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_enabled ();

  return ecma_op_is_callable (value);
} /* jerry_value_is_function */

/**
 * Check if the specified value is an async function object value.
 *
 * @return true - if the specified value is an async function,
 *         false - otherwise
 */
bool
jerry_value_is_async_function (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_enabled ();

  if (ecma_is_value_object (value))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (value);

    if (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_FUNCTION)
    {
      const ecma_compiled_code_t *bytecode_data_p;
      bytecode_data_p = ecma_op_function_get_compiled_code ((ecma_extended_object_t *) obj_p);
      uint16_t type = CBC_FUNCTION_GET_TYPE (bytecode_data_p->status_flags);

      return (type == CBC_FUNCTION_ASYNC || type == CBC_FUNCTION_ASYNC_ARROW || type == CBC_FUNCTION_ASYNC_GENERATOR);
    }
  }

  return false;
} /* jerry_value_is_async_function */

/**
 * Check if the specified value is number.
 *
 * @return true  - if the specified value is number,
 *         false - otherwise
 */
bool
jerry_value_is_number (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_enabled ();

  return ecma_is_value_number (value);
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
  jerry_assert_api_enabled ();

  return ecma_is_value_null (value);
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
  jerry_assert_api_enabled ();

  return ecma_is_value_object (value);
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
  jerry_assert_api_enabled ();

  return (ecma_is_value_object (value) && ecma_is_promise (ecma_get_object_from_value (value)));
} /* jerry_value_is_promise */

/**
 * Check if the specified value is a proxy object.
 *
 * @return true  - if the specified value is a proxy object,
 *         false - otherwise
 */
bool
jerry_value_is_proxy (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_enabled ();
#if JERRY_BUILTIN_PROXY
  return (ecma_is_value_object (value) && ECMA_OBJECT_IS_PROXY (ecma_get_object_from_value (value)));
#else /* !JERRY_BUILTIN_PROXY */
  JERRY_UNUSED (value);
  return false;
#endif /* JERRY_BUILTIN_PROXY */
} /* jerry_value_is_proxy */

/**
 * Check if the specified value is string.
 *
 * @return true  - if the specified value is string,
 *         false - otherwise
 */
bool
jerry_value_is_string (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_enabled ();

  return ecma_is_value_string (value);
} /* jerry_value_is_string */

/**
 * Check if the specified value is symbol.
 *
 * @return true  - if the specified value is symbol,
 *         false - otherwise
 */
bool
jerry_value_is_symbol (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_enabled ();

  return ecma_is_value_symbol (value);
} /* jerry_value_is_symbol */

/**
 * Check if the specified value is BigInt.
 *
 * @return true  - if the specified value is BigInt,
 *         false - otherwise
 */
bool
jerry_value_is_bigint (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_BIGINT
  return ecma_is_value_bigint (value);
#else /* !JERRY_BUILTIN_BIGINT */
  JERRY_UNUSED (value);
  return false;
#endif /* JERRY_BUILTIN_BIGINT */
} /* jerry_value_is_bigint */

/**
 * Check if the specified value is undefined.
 *
 * @return true  - if the specified value is undefined,
 *         false - otherwise
 */
bool
jerry_value_is_undefined (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_enabled ();

  return ecma_is_value_undefined (value);
} /* jerry_value_is_undefined */

/**
 * Perform the base type of the JavaScript value.
 *
 * @return jerry_type_t value
 */
jerry_type_t
jerry_value_type (const jerry_value_t value) /**< input value to check */
{
  jerry_assert_api_enabled ();

  if (ecma_is_value_exception (value))
  {
    return JERRY_TYPE_EXCEPTION;
  }

  lit_magic_string_id_t lit_id = ecma_get_typeof_lit_id (value);

  JERRY_ASSERT (lit_id != LIT_MAGIC_STRING__EMPTY);

  switch (lit_id)
  {
    case LIT_MAGIC_STRING_UNDEFINED:
    {
      return JERRY_TYPE_UNDEFINED;
    }
    case LIT_MAGIC_STRING_BOOLEAN:
    {
      return JERRY_TYPE_BOOLEAN;
    }
    case LIT_MAGIC_STRING_NUMBER:
    {
      return JERRY_TYPE_NUMBER;
    }
    case LIT_MAGIC_STRING_STRING:
    {
      return JERRY_TYPE_STRING;
    }
    case LIT_MAGIC_STRING_SYMBOL:
    {
      return JERRY_TYPE_SYMBOL;
    }
    case LIT_MAGIC_STRING_FUNCTION:
    {
      return JERRY_TYPE_FUNCTION;
    }
#if JERRY_BUILTIN_BIGINT
    case LIT_MAGIC_STRING_BIGINT:
    {
      return JERRY_TYPE_BIGINT;
    }
#endif /* JERRY_BUILTIN_BIGINT */
    default:
    {
      JERRY_ASSERT (lit_id == LIT_MAGIC_STRING_OBJECT);

      /* Based on the ECMA 262 5.1 standard the 'null' value is an object.
       * Thus we'll do an extra check for 'null' here.
       */
      return ecma_is_value_null (value) ? JERRY_TYPE_NULL : JERRY_TYPE_OBJECT;
    }
  }
} /* jerry_value_type */

/**
 * Used by jerry_object_type to get the type of class objects
 */
static const uint8_t jerry_class_object_type[] = {
  /* These objects require custom property resolving. */
  JERRY_OBJECT_TYPE_STRING, /**< type of ECMA_OBJECT_CLASS_STRING */
  JERRY_OBJECT_TYPE_ARGUMENTS, /**< type of ECMA_OBJECT_CLASS_ARGUMENTS */
#if JERRY_BUILTIN_TYPEDARRAY
  JERRY_OBJECT_TYPE_TYPEDARRAY, /**< type of ECMA_OBJECT_CLASS_TYPEDARRAY */
#endif /* JERRY_BUILTIN_TYPEDARRAY */
#if JERRY_MODULE_SYSTEM
  JERRY_OBJECT_TYPE_MODULE_NAMESPACE, /**< type of ECMA_OBJECT_CLASS_MODULE_NAMESPACE */
#endif /* JERRY_MODULE_SYSTEM */

  /* These objects are marked by Garbage Collector. */
  JERRY_OBJECT_TYPE_GENERATOR, /**< type of ECMA_OBJECT_CLASS_GENERATOR */
  JERRY_OBJECT_TYPE_GENERATOR, /**< type of ECMA_OBJECT_CLASS_ASYNC_GENERATOR */
  JERRY_OBJECT_TYPE_ITERATOR, /**< type of ECMA_OBJECT_CLASS_ARRAY_ITERATOR */
  JERRY_OBJECT_TYPE_ITERATOR, /**< type of ECMA_OBJECT_CLASS_SET_ITERATOR */
  JERRY_OBJECT_TYPE_ITERATOR, /**< type of ECMA_OBJECT_CLASS_MAP_ITERATOR */
#if JERRY_BUILTIN_REGEXP
  JERRY_OBJECT_TYPE_ITERATOR, /**< type of ECMA_OBJECT_CLASS_REGEXP_STRING_ITERATOR */
#endif /* JERRY_BUILTIN_REGEXP */
#if JERRY_MODULE_SYSTEM
  JERRY_OBJECT_TYPE_MODULE, /**< type of ECMA_OBJECT_CLASS_MODULE */
#endif /* JERRY_MODULE_SYSTEM */
  JERRY_OBJECT_TYPE_PROMISE, /**< type of ECMA_OBJECT_CLASS_PROMISE */
  JERRY_OBJECT_TYPE_GENERIC, /**< type of ECMA_OBJECT_CLASS_PROMISE_CAPABILITY */
  JERRY_OBJECT_TYPE_GENERIC, /**< type of ECMA_OBJECT_CLASS_ASYNC_FROM_SYNC_ITERATOR */
#if JERRY_BUILTIN_DATAVIEW
  JERRY_OBJECT_TYPE_DATAVIEW, /**< type of ECMA_OBJECT_CLASS_DATAVIEW */
#endif /* JERRY_BUILTIN_DATAVIEW */
#if JERRY_BUILTIN_CONTAINER
  JERRY_OBJECT_TYPE_CONTAINER, /**< type of ECMA_OBJECT_CLASS_CONTAINER */
#endif /* JERRY_BUILTIN_CONTAINER */

  /* Normal objects. */
  JERRY_OBJECT_TYPE_BOOLEAN, /**< type of ECMA_OBJECT_CLASS_BOOLEAN */
  JERRY_OBJECT_TYPE_NUMBER, /**< type of ECMA_OBJECT_CLASS_NUMBER */
  JERRY_OBJECT_TYPE_ERROR, /**< type of ECMA_OBJECT_CLASS_ERROR */
  JERRY_OBJECT_TYPE_GENERIC, /**< type of ECMA_OBJECT_CLASS_INTERNAL_OBJECT */
#if JERRY_PARSER
  JERRY_OBJECT_TYPE_SCRIPT, /**< type of ECMA_OBJECT_CLASS_SCRIPT */
#endif /* JERRY_PARSER */
#if JERRY_BUILTIN_DATE
  JERRY_OBJECT_TYPE_DATE, /**< type of ECMA_OBJECT_CLASS_DATE */
#endif /* JERRY_BUILTIN_DATE */
#if JERRY_BUILTIN_REGEXP
  JERRY_OBJECT_TYPE_REGEXP, /**< type of ECMA_OBJECT_CLASS_REGEXP */
#endif /* JERRY_BUILTIN_REGEXP */
  JERRY_OBJECT_TYPE_SYMBOL, /**< type of ECMA_OBJECT_CLASS_SYMBOL */
  JERRY_OBJECT_TYPE_ITERATOR, /**< type of ECMA_OBJECT_CLASS_STRING_ITERATOR */
#if JERRY_BUILTIN_TYPEDARRAY
  JERRY_OBJECT_TYPE_ARRAYBUFFER, /**< type of ECMA_OBJECT_CLASS_ARRAY_BUFFER */
#if JERRY_BUILTIN_SHAREDARRAYBUFFER
  JERRY_OBJECT_TYPE_SHARED_ARRAY_BUFFER, /**< type of ECMA_OBJECT_CLASS_SHARED_ARRAY_BUFFER */
#endif /* JERRY_BUILTIN_SHAREDARRAYBUFFER */
#endif /* JERRY_BUILTIN_TYPEDARRAY */
#if JERRY_BUILTIN_BIGINT
  JERRY_OBJECT_TYPE_BIGINT, /**< type of ECMA_OBJECT_CLASS_BIGINT */
#endif /* JERRY_BUILTIN_BIGINT */
#if JERRY_BUILTIN_WEAKREF
  JERRY_OBJECT_TYPE_WEAKREF, /**< type of ECMA_OBJECT_CLASS_WEAKREF */
#endif /* JERRY_BUILTIN_WEAKREF */
};

JERRY_STATIC_ASSERT (sizeof (jerry_class_object_type) == ECMA_OBJECT_CLASS__MAX,
                     jerry_class_object_type_must_have_object_class_max_elements);

/**
 * Get the object type of the given value
 *
 * @return JERRY_OBJECT_TYPE_NONE - if the given value is not an object
 *         jerry_object_type_t value - otherwise
 */
jerry_object_type_t
jerry_object_type (const jerry_value_t value) /**< input value to check */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (value))
  {
    return JERRY_OBJECT_TYPE_NONE;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (value);
  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

  switch (ecma_get_object_type (obj_p))
  {
    case ECMA_OBJECT_TYPE_CLASS:
    case ECMA_OBJECT_TYPE_BUILT_IN_CLASS:
    {
      JERRY_ASSERT (ext_obj_p->u.cls.type < ECMA_OBJECT_CLASS__MAX);
      return jerry_class_object_type[ext_obj_p->u.cls.type];
    }
    case ECMA_OBJECT_TYPE_ARRAY:
    case ECMA_OBJECT_TYPE_BUILT_IN_ARRAY:
    {
      return JERRY_OBJECT_TYPE_ARRAY;
    }
    case ECMA_OBJECT_TYPE_PROXY:
    {
      return JERRY_OBJECT_TYPE_PROXY;
    }
    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    case ECMA_OBJECT_TYPE_NATIVE_FUNCTION:
    case ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION:
    {
      return JERRY_OBJECT_TYPE_FUNCTION;
    }
    default:
    {
      break;
    }
  }

  return JERRY_OBJECT_TYPE_GENERIC;
} /* jerry_object_type */

/**
 * Get the function type of the given value
 *
 * @return JERRY_FUNCTION_TYPE_NONE - if the given value is not a function object
 *         jerry_function_type_t value - otherwise
 */
jerry_function_type_t
jerry_function_type (const jerry_value_t value) /**< input value to check */
{
  jerry_assert_api_enabled ();

  if (ecma_is_value_object (value))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (value);
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

    switch (ecma_get_object_type (obj_p))
    {
      case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
      {
        return JERRY_FUNCTION_TYPE_BOUND;
      }
      case ECMA_OBJECT_TYPE_NATIVE_FUNCTION:
      case ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION:
      {
        return JERRY_FUNCTION_TYPE_GENERIC;
      }
      case ECMA_OBJECT_TYPE_FUNCTION:
      {
        const ecma_compiled_code_t *bytecode_data_p = ecma_op_function_get_compiled_code (ext_obj_p);

        switch (CBC_FUNCTION_GET_TYPE (bytecode_data_p->status_flags))
        {
          case CBC_FUNCTION_ARROW:
          case CBC_FUNCTION_ASYNC_ARROW:
          {
            return JERRY_FUNCTION_TYPE_ARROW;
          }
          case CBC_FUNCTION_GENERATOR:
          case CBC_FUNCTION_ASYNC_GENERATOR:
          {
            return JERRY_FUNCTION_TYPE_GENERATOR;
          }
          case CBC_FUNCTION_ACCESSOR:
          {
            return JERRY_FUNCTION_TYPE_ACCESSOR;
          }
          default:
          {
            break;
          }
        }
        return JERRY_FUNCTION_TYPE_GENERIC;
      }
      default:
      {
        break;
      }
    }
  }

  return JERRY_FUNCTION_TYPE_NONE;
} /* jerry_function_type */

/**
 * Get the itearator type of the given value
 *
 * @return JERRY_ITERATOR_TYPE_NONE - if the given value is not an iterator object
 *         jerry_iterator_type_t value - otherwise
 */
jerry_iterator_type_t
jerry_iterator_type (const jerry_value_t value) /**< input value to check */
{
  jerry_assert_api_enabled ();

  if (ecma_is_value_object (value))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (value);
    ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

    if (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_CLASS)
    {
      switch (ext_obj_p->u.cls.type)
      {
        case ECMA_OBJECT_CLASS_ARRAY_ITERATOR:
        {
          return JERRY_ITERATOR_TYPE_ARRAY;
        }
#if JERRY_BUILTIN_CONTAINER
        case ECMA_OBJECT_CLASS_SET_ITERATOR:
        {
          return JERRY_ITERATOR_TYPE_SET;
        }
        case ECMA_OBJECT_CLASS_MAP_ITERATOR:
        {
          return JERRY_ITERATOR_TYPE_MAP;
        }
#endif /* JERRY_BUILTIN_CONTAINER */
        case ECMA_OBJECT_CLASS_STRING_ITERATOR:
        {
          return JERRY_ITERATOR_TYPE_STRING;
        }
        default:
        {
          break;
        }
      }
    }
  }

  return JERRY_ITERATOR_TYPE_NONE;
} /* jerry_iterator_type */

/**
 * Check if the specified feature is enabled.
 *
 * @return true  - if the specified feature is enabled,
 *         false - otherwise
 */
bool
jerry_feature_enabled (const jerry_feature_t feature) /**< feature to check */
{
  JERRY_ASSERT (feature < JERRY_FEATURE__COUNT);

  return (false
#if JERRY_CPOINTER_32_BIT
          || feature == JERRY_FEATURE_CPOINTER_32_BIT
#endif /* JERRY_CPOINTER_32_BIT */
#if JERRY_ERROR_MESSAGES
          || feature == JERRY_FEATURE_ERROR_MESSAGES
#endif /* JERRY_ERROR_MESSAGES */
#if JERRY_PARSER
          || feature == JERRY_FEATURE_JS_PARSER
#endif /* JERRY_PARSER */
#if JERRY_MEM_STATS
          || feature == JERRY_FEATURE_HEAP_STATS
#endif /* JERRY_MEM_STATS */
#if JERRY_PARSER_DUMP_BYTE_CODE
          || feature == JERRY_FEATURE_PARSER_DUMP
#endif /* JERRY_PARSER_DUMP_BYTE_CODE */
#if JERRY_REGEXP_DUMP_BYTE_CODE
          || feature == JERRY_FEATURE_REGEXP_DUMP
#endif /* JERRY_REGEXP_DUMP_BYTE_CODE */
#if JERRY_SNAPSHOT_SAVE
          || feature == JERRY_FEATURE_SNAPSHOT_SAVE
#endif /* JERRY_SNAPSHOT_SAVE */
#if JERRY_SNAPSHOT_EXEC
          || feature == JERRY_FEATURE_SNAPSHOT_EXEC
#endif /* JERRY_SNAPSHOT_EXEC */
#if JERRY_DEBUGGER
          || feature == JERRY_FEATURE_DEBUGGER
#endif /* JERRY_DEBUGGER */
#if JERRY_VM_HALT
          || feature == JERRY_FEATURE_VM_EXEC_STOP
#endif /* JERRY_VM_HALT */
#if JERRY_VM_THROW
          || feature == JERRY_FEATURE_VM_THROW
#endif /* JERRY_VM_THROW */
#if JERRY_BUILTIN_JSON
          || feature == JERRY_FEATURE_JSON
#endif /* JERRY_BUILTIN_JSON */
#if JERRY_BUILTIN_TYPEDARRAY
          || feature == JERRY_FEATURE_TYPEDARRAY
#endif /* JERRY_BUILTIN_TYPEDARRAY */
#if JERRY_BUILTIN_DATAVIEW
          || feature == JERRY_FEATURE_DATAVIEW
#endif /* JERRY_BUILTIN_DATAVIEW */
#if JERRY_BUILTIN_PROXY
          || feature == JERRY_FEATURE_PROXY
#endif /* JERRY_BUILTIN_PROXY */
#if JERRY_BUILTIN_DATE
          || feature == JERRY_FEATURE_DATE
#endif /* JERRY_BUILTIN_DATE */
#if JERRY_BUILTIN_REGEXP
          || feature == JERRY_FEATURE_REGEXP
#endif /* JERRY_BUILTIN_REGEXP */
#if JERRY_LINE_INFO
          || feature == JERRY_FEATURE_LINE_INFO
#endif /* JERRY_LINE_INFO */
#if JERRY_LOGGING
          || feature == JERRY_FEATURE_LOGGING
#endif /* JERRY_LOGGING */
#if JERRY_BUILTIN_GLOBAL_THIS
          || feature == JERRY_FEATURE_GLOBAL_THIS
#endif /* JERRY_BUILTIN_GLOBAL_THIS */
#if JERRY_BUILTIN_CONTAINER
          || feature == JERRY_FEATURE_MAP || feature == JERRY_FEATURE_SET || feature == JERRY_FEATURE_WEAKMAP
          || feature == JERRY_FEATURE_WEAKSET
#endif /* JERRY_BUILTIN_CONTAINER */
#if JERRY_BUILTIN_WEAKREF
          || feature == JERRY_FEATURE_WEAKREF
#endif /* JERRY_BUILTIN_WEAKREF */
#if JERRY_BUILTIN_BIGINT
          || feature == JERRY_FEATURE_BIGINT
#endif /* JERRY_BUILTIN_BIGINT */
#if JERRY_BUILTIN_REALMS
          || feature == JERRY_FEATURE_REALM
#endif /* JERRY_BUILTIN_REALMS */
#if JERRY_PROMISE_CALLBACK
          || feature == JERRY_FEATURE_PROMISE_CALLBACK
#endif /* JERRY_PROMISE_CALLBACK */
#if JERRY_MODULE_SYSTEM
          || feature == JERRY_FEATURE_MODULE
#endif /* JERRY_MODULE_SYSTEM */
#if JERRY_FUNCTION_TO_STRING
          || feature == JERRY_FEATURE_FUNCTION_TO_STRING
#endif /* JERRY_FUNCTION_TO_STRING */
  );
} /* jerry_feature_enabled */

/**
 * Perform binary operation on the given operands (==, ===, <, >, etc.).
 *
 * @return error - if argument has an error flag or operation is unsuccessful or unsupported
 *         true/false - the result of the binary operation on the given operands otherwise
 */
jerry_value_t
jerry_binary_op (jerry_binary_op_t operation, /**< operation */
                 const jerry_value_t lhs, /**< first operand */
                 const jerry_value_t rhs) /**< second operand */
{
  jerry_assert_api_enabled ();

  if (ecma_is_value_exception (lhs) || ecma_is_value_exception (rhs))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  switch (operation)
  {
    case JERRY_BIN_OP_EQUAL:
    {
      return jerry_return (ecma_op_abstract_equality_compare (lhs, rhs));
    }
    case JERRY_BIN_OP_STRICT_EQUAL:
    {
      return ecma_make_boolean_value (ecma_op_strict_equality_compare (lhs, rhs));
    }
    case JERRY_BIN_OP_LESS:
    {
      return jerry_return (opfunc_relation (lhs, rhs, true, false));
    }
    case JERRY_BIN_OP_LESS_EQUAL:
    {
      return jerry_return (opfunc_relation (lhs, rhs, false, true));
    }
    case JERRY_BIN_OP_GREATER:
    {
      return jerry_return (opfunc_relation (lhs, rhs, false, false));
    }
    case JERRY_BIN_OP_GREATER_EQUAL:
    {
      return jerry_return (opfunc_relation (lhs, rhs, true, true));
    }
    case JERRY_BIN_OP_INSTANCEOF:
    {
      if (!ecma_is_value_object (lhs) || !ecma_op_is_callable (rhs))
      {
        return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
      }

      ecma_object_t *proto_obj_p = ecma_get_object_from_value (rhs);
      return jerry_return (ecma_op_object_has_instance (proto_obj_p, lhs));
    }
    case JERRY_BIN_OP_ADD:
    {
      return jerry_return (opfunc_addition (lhs, rhs));
    }
    case JERRY_BIN_OP_SUB:
    case JERRY_BIN_OP_MUL:
    case JERRY_BIN_OP_DIV:
    case JERRY_BIN_OP_REM:
    {
      return jerry_return (do_number_arithmetic (operation - ECMA_NUMBER_ARITHMETIC_OP_API_OFFSET, lhs, rhs));
    }
    default:
    {
      return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_UNSUPPORTED_BINARY_OPERATION));
    }
  }
} /* jerry_binary_op */

/**
 * Create an abort value containing the argument value. If the second argument is true
 * the function will take ownership ofthe input value, otherwise the value will be copied.
 *
 * @return api abort value
 */
jerry_value_t
jerry_throw_abort (jerry_value_t value, /**< api value */
                   bool take_ownership) /**< release api value */
{
  jerry_assert_api_enabled ();

  if (JERRY_UNLIKELY (ecma_is_value_exception (value)))
  {
    /* This is a rare case so it is optimized for
     * binary size rather than performance. */
    if (jerry_value_is_abort (value))
    {
      return take_ownership ? value : jerry_value_copy (value);
    }

    value = jerry_exception_value (value, take_ownership);
    take_ownership = true;
  }

  if (!take_ownership)
  {
    value = ecma_copy_value (value);
  }

  return ecma_create_exception (value, ECMA_ERROR_API_FLAG_ABORT);
} /* jerry_throw_abort */

/**
 * Create an exception value containing the argument value. If the second argument is true
 * the function will take ownership ofthe input value, otherwise the value will be copied.
 *
 * @return exception value
 */
jerry_value_t
jerry_throw_value (jerry_value_t value, /**< value */
                   bool take_ownership) /**< take ownership of the value */
{
  jerry_assert_api_enabled ();

  if (JERRY_UNLIKELY (ecma_is_value_exception (value)))
  {
    /* This is a rare case so it is optimized for
     * binary size rather than performance. */
    if (!jerry_value_is_abort (value))
    {
      return take_ownership ? value : jerry_value_copy (value);
    }

    value = jerry_exception_value (value, take_ownership);
    take_ownership = true;
  }

  if (!take_ownership)
  {
    value = ecma_copy_value (value);
  }

  return ecma_create_exception (value, ECMA_ERROR_API_FLAG_NONE);
} /* jerry_throw_value */

/**
 * Get the value contained in an exception. If the second argument is true
 * it will release the argument exception value in the process.
 *
 * @return value in exception
 */
jerry_value_t
jerry_exception_value (jerry_value_t value, /**< api value */
                       bool free_exception) /**< release api value */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_exception (value))
  {
    return free_exception ? value : ecma_copy_value (value);
  }

  jerry_value_t ret_val = jerry_value_copy (ecma_get_extended_primitive_from_value (value)->u.value);

  if (free_exception)
  {
    jerry_value_free (value);
  }
  return ret_val;
} /* jerry_exception_value */

/**
 * Set new decorator callback for Error objects. The decorator can
 * create or update any properties of the newly created Error object.
 */
void
jerry_error_on_created (jerry_error_object_created_cb_t callback, /**< new callback */
                        void *user_p) /**< user pointer passed to the callback */
{
  jerry_assert_api_enabled ();

  JERRY_CONTEXT (error_object_created_callback_p) = callback;
  JERRY_CONTEXT (error_object_created_callback_user_p) = user_p;
} /* jerry_error_on_created */

/**
 * When JERRY_VM_THROW is enabled, the callback passed to this
 * function is called when an error is thrown in ECMAScript code.
 */
void
jerry_on_throw (jerry_throw_cb_t callback, /**< callback which is called on throws */
                void *user_p) /**< pointer passed to the function */
{
#if JERRY_VM_THROW
  JERRY_CONTEXT (vm_throw_callback_p) = callback;
  JERRY_CONTEXT (vm_throw_callback_user_p) = user_p;
#else /* !JERRY_VM_THROW */
  JERRY_UNUSED (callback);
  JERRY_UNUSED (user_p);
#endif /* JERRY_VM_THROW */
} /* jerry_on_throw */

/**
 * Checks whether the callback set by jerry_on_throw captured the error
 *
 * @return true, if the vm throw callback captured the error
 *         false, otherwise
 */
bool
jerry_exception_is_captured (const jerry_value_t value) /**< exception value */
{
  jerry_assert_api_enabled ();

#if JERRY_VM_THROW
  if (!ecma_is_value_exception (value))
  {
    return false;
  }

  ecma_extended_primitive_t *error_ref_p = ecma_get_extended_primitive_from_value (value);

  return (error_ref_p->refs_and_type & ECMA_ERROR_API_FLAG_THROW_CAPTURED) != 0;
#else /* !JERRY_VM_THROW */
  JERRY_UNUSED (value);
  return false;
#endif /* JERRY_VM_THROW */
} /* jerry_exception_is_captured */

/**
 * Sets whether the callback set by jerry_on_throw should capture the exception or not
 */
void
jerry_exception_allow_capture (jerry_value_t value, /**< exception value */
                               bool should_capture) /**< callback should capture this error */
{
  jerry_assert_api_enabled ();

#if JERRY_VM_THROW
  if (!ecma_is_value_exception (value))
  {
    return;
  }

  ecma_extended_primitive_t *error_ref_p = ecma_get_extended_primitive_from_value (value);

  if (should_capture)
  {
    error_ref_p->refs_and_type &= ~(uint32_t) ECMA_ERROR_API_FLAG_THROW_CAPTURED;
    return;
  }

  error_ref_p->refs_and_type |= ECMA_ERROR_API_FLAG_THROW_CAPTURED;
#else /* !JERRY_VM_THROW */
  JERRY_UNUSED (value);
  JERRY_UNUSED (should_capture);
#endif /* JERRY_VM_THROW */
} /* jerry_exception_allow_capture */

/**
 * Check if the given value is an Error object.
 *
 * @return true - if it is an Error object
 *         false - otherwise
 */
bool
jerry_value_is_error (const jerry_value_t value) /**< api value */
{
  return ecma_is_value_object (value)
         && ecma_object_class_is (ecma_get_object_from_value (value), ECMA_OBJECT_CLASS_ERROR);
} /* jerry_value_is_error */

/**
 * Return the type of the Error object if possible.
 *
 * @return one of the jerry_error_t value as the type of the Error object
 *         JERRY_ERROR_NONE - if the input value is not an Error object
 */
jerry_error_t
jerry_error_type (jerry_value_t value) /**< api value */
{
  if (JERRY_UNLIKELY (ecma_is_value_exception (value)))
  {
    value = ecma_get_extended_primitive_from_value (value)->u.value;
  }

  if (!ecma_is_value_object (value))
  {
    return JERRY_ERROR_NONE;
  }

  ecma_object_t *object_p = ecma_get_object_from_value (value);
  /* TODO(check if error object) */
  jerry_error_t error_type = ecma_get_error_type (object_p);

  return (jerry_error_t) error_type;
} /* jerry_error_type */

/**
 * Get number from the specified value as a double.
 *
 * @return stored number as double
 */
double
jerry_value_as_number (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_number (value))
  {
    return 0;
  }

  return (double) ecma_get_number_from_value (value);
} /* jerry_value_as_number */

/**
 * Call ToBoolean operation on the api value.
 *
 * @return true  - if the logical value is true
 *         false - otherwise
 */
bool
jerry_value_to_boolean (const jerry_value_t value) /**< input value */
{
  jerry_assert_api_enabled ();

  if (ecma_is_value_exception (value))
  {
    return false;
  }

  return ecma_op_to_boolean (value);
} /* jerry_value_to_boolean */

/**
 * Call ToNumber operation on the api value.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return converted number value - if success
 *         thrown error - otherwise
 */
jerry_value_t
jerry_value_to_number (const jerry_value_t value) /**< input value */
{
  jerry_assert_api_enabled ();

  if (ecma_is_value_exception (value))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  ecma_number_t num;
  ecma_value_t ret_value = ecma_op_to_number (value, &num);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    return ecma_create_exception_from_context ();
  }

  return ecma_make_number_value (num);
} /* jerry_value_to_number */

/**
 * Call ToObject operation on the api value.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return converted object value - if success
 *         thrown error - otherwise
 */
jerry_value_t
jerry_value_to_object (const jerry_value_t value) /**< input value */
{
  jerry_assert_api_enabled ();

  if (ecma_is_value_exception (value))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  return jerry_return (ecma_op_to_object (value));
} /* jerry_value_to_object */

/**
 * Call ToPrimitive operation on the api value.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return converted primitive value - if success
 *         thrown error - otherwise
 */
jerry_value_t
jerry_value_to_primitive (const jerry_value_t value) /**< input value */
{
  jerry_assert_api_enabled ();

  if (ecma_is_value_exception (value))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  return jerry_return (ecma_op_to_primitive (value, ECMA_PREFERRED_TYPE_NO));
} /* jerry_value_to_primitive */

/**
 * Call the ToString ecma builtin operation on the api value.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return converted string value - if success
 *         thrown error - otherwise
 */
jerry_value_t
jerry_value_to_string (const jerry_value_t value) /**< input value */
{
  jerry_assert_api_enabled ();

  if (ecma_is_value_exception (value))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  ecma_string_t *str_p = ecma_op_to_string (value);
  if (JERRY_UNLIKELY (str_p == NULL))
  {
    return ecma_create_exception_from_context ();
  }

  return ecma_make_string_value (str_p);
} /* jerry_value_to_string */

/**
 * Call the BigInt constructor ecma builtin operation on the api value.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return BigInt value - if success
 *         thrown error - otherwise
 */
jerry_value_t
jerry_value_to_bigint (const jerry_value_t value) /**< input value */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_BIGINT
  if (ecma_is_value_exception (value))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  return jerry_return (ecma_bigint_to_bigint (value, true));
#else /* !JERRY_BUILTIN_BIGINT */
  JERRY_UNUSED (value);
  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_BIGINT_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_BIGINT */
} /* jerry_value_to_bigint */

/**
 * Convert any number to integer number.
 *
 * Note:
 *      For non-number values 0 is returned.
 *
 * @return integer representation of the number.
 */
double
jerry_value_as_integer (const jerry_value_t value) /**< input value */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_number (value))
  {
    return 0;
  }

  double number = ecma_get_number_from_value (value);

  if (ecma_number_is_nan (number))
  {
    return ECMA_NUMBER_ZERO;
  }

  if (ecma_number_is_zero (number) || ecma_number_is_infinity (number))
  {
    return number;
  }

  ecma_number_t floor_fabs = (ecma_number_t) floor (fabs (number));

  return ecma_number_is_negative (number) ? -floor_fabs : floor_fabs;
} /* jerry_value_as_integer */

/**
 * Convert any number to int32 number.
 *
 * Note:
 *      For non-number values 0 is returned.
 *
 * @return int32 representation of the number.
 */
int32_t
jerry_value_as_int32 (const jerry_value_t value) /**< input value */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_number (value))
  {
    return 0;
  }

  return ecma_number_to_int32 (ecma_get_number_from_value (value));
} /* jerry_value_as_int32 */

/**
 * Convert any number to uint32 number.
 *
 * Note:
 *      For non-number values 0 is returned.
 *
 * @return uint32 representation of the number.
 */
uint32_t
jerry_value_as_uint32 (const jerry_value_t value) /**< input value */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_number (value))
  {
    return 0;
  }

  return ecma_number_to_uint32 (ecma_get_number_from_value (value));
} /* jerry_value_as_uint32 */

/**
 * Take additional ownership over the argument value.
 * The value will be copied by reference when possible, changes made to the new value will be reflected
 * in the original.
 *
 * @return copied value
 */
jerry_value_t
jerry_value_copy (const jerry_value_t value) /**< value */
{
  jerry_assert_api_enabled ();

  if (JERRY_UNLIKELY (ecma_is_value_exception (value)))
  {
    ecma_ref_extended_primitive (ecma_get_extended_primitive_from_value (value));
    return value;
  }

  return ecma_copy_value (value);
} /* jerry_value_copy */

/**
 * Release ownership of the argument value
 */
void
jerry_value_free (jerry_value_t value) /**< value */
{
  jerry_assert_api_enabled ();

  if (JERRY_UNLIKELY (ecma_is_value_exception (value)))
  {
    ecma_deref_exception (ecma_get_extended_primitive_from_value (value));
    return;
  }

  ecma_free_value (value);
} /* jerry_value_free */

/**
 * Create an array object value
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return value of the constructed array object
 */
jerry_value_t
jerry_array (jerry_length_t length) /**< length of array */
{
  jerry_assert_api_enabled ();

  ecma_object_t *array_p = ecma_op_new_array_object (length);
  return ecma_make_object_value (array_p);
} /* jerry_array */

/**
 * Create a jerry_value_t representing a boolean value from the given boolean parameter.
 *
 * @return value of the created boolean
 */
jerry_value_t
jerry_boolean (bool value) /**< bool value from which a jerry_value_t will be created */
{
  jerry_assert_api_enabled ();

  return ecma_make_boolean_value (value);
} /* jerry_boolean */

/**
 * Create an Error object with the provided string value as the error message.
 * If the message value is not a string, the created error will not have a message property.
 *
 * @return Error object
 */
jerry_value_t
jerry_error (jerry_error_t error_type, /**< type of error */
             const jerry_value_t message) /**< message of the error */
{
  jerry_assert_api_enabled ();

  ecma_string_t *message_p = NULL;
  if (ecma_is_value_string (message))
  {
    message_p = ecma_get_string_from_value (message);
  }

  ecma_object_t *error_object_p = ecma_new_standard_error ((jerry_error_t) error_type, message_p);

  return ecma_make_object_value (error_object_p);
} /* jerry_error */

/**
 * Create an Error object with a zero-terminated string as a message. If the message string is NULL, the created error
 * will not have a message property.
 *
 * @return Error object
 */
jerry_value_t
jerry_error_sz (jerry_error_t error_type, /**< type of error */
                const char *message_p) /**< value of 'message' property
                                        *   of constructed error object */
{
  jerry_value_t message = ECMA_VALUE_UNDEFINED;

  if (message_p != NULL)
  {
    message = jerry_string_sz (message_p);
  }

  ecma_value_t error = jerry_error (error_type, message);
  ecma_free_value (message);

  return error;
} /* jerry_error_sz */

/**
 * Create an exception by constructing an Error object with the specified type and the provided string value as the
 * error message.  If the message value is not a string, the created error will not have a message property.
 *
 * @return exception value
 */
jerry_value_t
jerry_throw (jerry_error_t error_type, /**< type of error */
             const jerry_value_t message) /**< message value */
{
  return jerry_throw_value (jerry_error (error_type, message), true);
} /* jerry_throw */

/**
 * Create an exception by constructing an Error object with the specified type and the provided zero-terminated ASCII
 * string as the error message.  If the message string is NULL, the created error will not have a message property.
 *
 * @return exception value
 */
jerry_value_t
jerry_throw_sz (jerry_error_t error_type, /**< type of error */
                const char *message_p) /**< value of 'message' property
                                        *   of constructed error object */
{
  return jerry_throw_value (jerry_error_sz (error_type, message_p), true);
} /* jerry_throw_sz */

/**
 * Create an external function object
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return value of the constructed function object
 */
jerry_value_t
jerry_function_external (jerry_external_handler_t handler) /**< native handler
                                                            *   for the function */
{
  jerry_assert_api_enabled ();

  ecma_object_t *func_obj_p = ecma_op_create_external_function_object (handler);
  return ecma_make_object_value (func_obj_p);
} /* jerry_function_external */

/**
 * Creates a jerry_value_t representing a number value.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return jerry_value_t created from the given double argument.
 */
jerry_value_t
jerry_number (double value) /**< double value from which a jerry_value_t will be created */
{
  jerry_assert_api_enabled ();

  return ecma_make_number_value ((ecma_number_t) value);
} /* jerry_number */

/**
 * Creates a jerry_value_t representing a positive or negative infinity value.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return jerry_value_t representing an infinity value.
 */
jerry_value_t
jerry_infinity (bool sign) /**< true for negative Infinity
                            *   false for positive Infinity */
{
  jerry_assert_api_enabled ();

  return ecma_make_number_value (ecma_number_make_infinity (sign));
} /* jerry_infinity */

/**
 * Creates a jerry_value_t representing a not-a-number value.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return jerry_value_t representing a not-a-number value.
 */
jerry_value_t
jerry_nan (void)
{
  jerry_assert_api_enabled ();

  return ecma_make_nan_value ();
} /* jerry_nan */

/**
 * Creates a jerry_value_t representing an undefined value.
 *
 * @return value of undefined
 */
jerry_value_t
jerry_undefined (void)
{
  jerry_assert_api_enabled ();

  return ECMA_VALUE_UNDEFINED;
} /* jerry_undefined */

/**
 * Creates and returns a jerry_value_t with type null object.
 *
 * @return jerry_value_t representing null
 */
jerry_value_t
jerry_null (void)
{
  jerry_assert_api_enabled ();

  return ECMA_VALUE_NULL;
} /* jerry_null */

/**
 * Create new JavaScript object, like with new Object().
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return value of the created object
 */
jerry_value_t
jerry_object (void)
{
  jerry_assert_api_enabled ();

  return ecma_make_object_value (ecma_op_create_object_object_noarg ());
} /* jerry_object */

/**
 * Create an empty Promise object which can be resolved/rejected later
 * by calling jerry_promise_resolve or jerry_promise_reject.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return value of the created object
 */
jerry_value_t
jerry_promise (void)
{
  jerry_assert_api_enabled ();

  return jerry_return (ecma_op_create_promise_object (ECMA_VALUE_EMPTY, ECMA_VALUE_UNDEFINED, NULL));
} /* jerry_create_promise */

/**
 * Create a new Proxy object with the given target and handler
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return value of the created Proxy object
 */
jerry_value_t
jerry_proxy (const jerry_value_t target, /**< target argument */
             const jerry_value_t handler) /**< handler argument */
{
  jerry_assert_api_enabled ();

  if (ecma_is_value_exception (target) || ecma_is_value_exception (handler))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

#if JERRY_BUILTIN_PROXY
  ecma_object_t *proxy_p = ecma_proxy_create (target, handler, 0);

  if (proxy_p == NULL)
  {
    return ecma_create_exception_from_context ();
  }

  return ecma_make_object_value (proxy_p);
#else /* !JERRY_BUILTIN_PROXY */
  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_PROXY_IS_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_PROXY */
} /* jerry_proxy */

#if JERRY_BUILTIN_PROXY

JERRY_STATIC_ASSERT ((int) JERRY_PROXY_SKIP_RESULT_VALIDATION == (int) ECMA_PROXY_SKIP_RESULT_VALIDATION,
                     jerry_and_ecma_proxy_skip_result_validation_must_be_equal);

#endif /* JERRY_BUILTIN_PROXY */

/**
 * Create a new Proxy object with the given target, handler, and special options
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return value of the created Proxy object
 */
jerry_value_t
jerry_proxy_custom (const jerry_value_t target, /**< target argument */
                    const jerry_value_t handler, /**< handler argument */
                    uint32_t flags) /**< jerry_proxy_custom_behavior_t option bits */
{
  jerry_assert_api_enabled ();

  if (ecma_is_value_exception (target) || ecma_is_value_exception (handler))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

#if JERRY_BUILTIN_PROXY
  flags &= JERRY_PROXY_SKIP_RESULT_VALIDATION;

  ecma_object_t *proxy_p = ecma_proxy_create (target, handler, flags);

  if (proxy_p == NULL)
  {
    return ecma_create_exception_from_context ();
  }

  return ecma_make_object_value (proxy_p);
#else /* !JERRY_BUILTIN_PROXY */
  JERRY_UNUSED (flags);
  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_PROXY_IS_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_PROXY */
} /* jerry_proxy_custom */

/**
 * Create string value from the input zero-terminated ASCII string.
 *
 * @return created string
 */
jerry_value_t
jerry_string_sz (const char *str_p) /**< pointer to string */
{
  const jerry_char_t *data_p = (const jerry_char_t *) str_p;
  return jerry_string (data_p, lit_zt_utf8_string_size (data_p), JERRY_ENCODING_CESU8);
} /* jerry_string_sz */

/**
 * Create a string value from the input buffer using the specified encoding.
 * The content of the buffer is assumed to be valid in the specified encoding, it's the callers responsibility to
 * validate the input.
 *
 * See also: jerry_validate_string
 *
 * @return created string
 */
jerry_value_t
jerry_string (const jerry_char_t *buffer_p, /**< pointer to buffer */
              jerry_size_t buffer_size, /**< buffer size */
              jerry_encoding_t encoding) /**< buffer encoding */
{
  jerry_assert_api_enabled ();
  ecma_string_t *ecma_str_p = NULL;
  JERRY_ASSERT (jerry_validate_string (buffer_p, buffer_size, encoding));

  switch (encoding)
  {
    case JERRY_ENCODING_CESU8:
    {
      ecma_str_p = ecma_new_ecma_string_from_utf8 (buffer_p, buffer_size);
      break;
    }
    case JERRY_ENCODING_UTF8:
    {
      ecma_str_p = ecma_new_ecma_string_from_utf8_converted_to_cesu8 (buffer_p, buffer_size);
      break;
    }
    default:
    {
      return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_INVALID_ENCODING));
    }
  }

  return ecma_make_string_value (ecma_str_p);
} /* jerry_string */

/**
 * Create external string from input zero-terminated ASCII string.
 *
 * @return created external string
 */
jerry_value_t
jerry_string_external_sz (const char *str_p, /**< pointer to string */
                          void *user_p) /**< user pointer passed to the callback when the string is freed */
{
  const jerry_char_t *data_p = (const jerry_char_t *) str_p;
  return jerry_string_external (data_p, lit_zt_utf8_string_size (data_p), user_p);
} /* jerry_string_external_sz */

/**
 * Create external string from a valid CESU-8 encoded string.
 * The content of the buffer is assumed be encoded correctly, it's the callers responsibility to
 * validate the input.
 *
 * See also: jerry_validate_string
 *
 * @return created external string
 */
jerry_value_t
jerry_string_external (const jerry_char_t *buffer_p, /**< pointer to string */
                       jerry_size_t buffer_size, /**< string size */
                       void *user_p) /**< user pointer passed to the callback when the string is freed */
{
  jerry_assert_api_enabled ();

  JERRY_ASSERT (jerry_validate_string (buffer_p, buffer_size, JERRY_ENCODING_CESU8));
  ecma_string_t *ecma_str_p = ecma_new_ecma_external_string_from_cesu8 (buffer_p, buffer_size, user_p);
  return ecma_make_string_value (ecma_str_p);
} /* jerry_string_external_sz_sz */

/**
 * Create symbol with a description value
 *
 * Note: The given argument is converted to string. This operation can throw an exception.
 *
 * @return created symbol,
 *         or thrown exception
 */
jerry_value_t
jerry_symbol_with_description (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_enabled ();

  if (ecma_is_value_exception (value))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  return jerry_return (ecma_op_create_symbol (&value, 1));
} /* jerry_create_symbol */

/**
 * Create BigInt from a sequence of uint64 digits.
 *
 * Note: This operation can throw an exception.
 *
 * @return created bigint,
 *         or thrown exception
 */
jerry_value_t
jerry_bigint (const uint64_t *digits_p, /**< BigInt digits (lowest digit first) */
              uint32_t digit_count, /**< number of BigInt digits */
              bool sign) /**< sign bit, true if the result should be negative */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_BIGINT
  return jerry_return (ecma_bigint_create_from_digits (digits_p, digit_count, sign));
#else /* !JERRY_BUILTIN_BIGINT */
  JERRY_UNUSED (digits_p);
  JERRY_UNUSED (digit_count);
  JERRY_UNUSED (sign);
  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_BIGINT_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_BIGINT */
} /* jerry_bigint */

/**
 * Creates a RegExp object with the given ASCII pattern and flags.
 *
 * @return value of the constructed RegExp object.
 */
jerry_value_t
jerry_regexp_sz (const char *pattern_p, /**< RegExp pattern as zero-terminated ASCII string */
                 uint16_t flags) /**< RegExp flags */
{
  jerry_assert_api_enabled ();

  jerry_value_t pattern = jerry_string_sz (pattern_p);
  jerry_value_t result = jerry_regexp (pattern, flags);

  jerry_value_free (pattern);
  return jerry_return (result);
} /* jerry_regexp_sz */

/**
 * Creates a RegExp object with the given pattern and flags.
 *
 * @return value of the constructed RegExp object.
 */
jerry_value_t
jerry_regexp (const jerry_value_t pattern, /**< pattern string */
              uint16_t flags) /**< RegExp flags */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_REGEXP
  if (!ecma_is_value_string (pattern))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_object_t *regexp_obj_p = ecma_op_regexp_alloc (NULL);

  if (JERRY_UNLIKELY (regexp_obj_p == NULL))
  {
    return ecma_create_exception_from_context ();
  }

  jerry_value_t result = ecma_op_create_regexp_with_flags (regexp_obj_p, pattern, flags);

  return jerry_return (result);

#else /* !JERRY_BUILTIN_REGEXP */
  JERRY_UNUSED (pattern);
  JERRY_UNUSED (flags);

  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_REGEXP_IS_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_REGEXP */
} /* jerry_regexp */

/**
 * Creates a new realm (global object).
 *
 * @return new realm object
 */
jerry_value_t
jerry_realm (void)
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_REALMS
  ecma_global_object_t *global_object_p = ecma_builtin_create_global_object ();
  return ecma_make_object_value ((ecma_object_t *) global_object_p);
#else /* !JERRY_BUILTIN_REALMS */
  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_REALMS_ARE_DISABLED));
#endif /* JERRY_BUILTIN_REALMS */
} /* jerry_realm */

/**
 * Get length of an array object
 *
 * Note:
 *      Returns 0, if the value parameter is not an array object.
 *
 * @return length of the given array
 */
jerry_length_t
jerry_array_length (const jerry_value_t value) /**< api value */
{
  jerry_assert_api_enabled ();

  if (!jerry_value_is_object (value))
  {
    return 0;
  }

  ecma_object_t *object_p = ecma_get_object_from_value (value);

  if (JERRY_LIKELY (ecma_get_object_base_type (object_p) == ECMA_OBJECT_BASE_TYPE_ARRAY))
  {
    return ecma_array_get_length (object_p);
  }

  return 0;
} /* jerry_array_length */

/**
 * Get the size of a string value in the specified encoding.
 *
 * @return number of bytes required by the string,
 *         0 - if value is not a string
 */
jerry_size_t
jerry_string_size (const jerry_value_t value, /**< input string */
                   jerry_encoding_t encoding) /**< encoding */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_string (value))
  {
    return 0;
  }

  switch (encoding)
  {
    case JERRY_ENCODING_CESU8:
    {
      return ecma_string_get_size (ecma_get_string_from_value (value));
    }
    case JERRY_ENCODING_UTF8:
    {
      return ecma_string_get_utf8_size (ecma_get_string_from_value (value));
    }
    default:
    {
      return 0;
    }
  }
} /* jerry_string_size */

/**
 * Get length of a string value
 *
 * @return number of characters in the string
 *         0 - if value is not a string
 */
jerry_length_t
jerry_string_length (const jerry_value_t value) /**< input string */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_string (value))
  {
    return 0;
  }

  return ecma_string_get_length (ecma_get_string_from_value (value));
} /* jerry_string_length */

/**
 * Copy the characters of a string into the specified buffer using the specified encoding.  The string is truncated to
 * fit the buffer. If the value is not a string, nothing will be copied to the buffer.
 *
 * @return number of bytes copied to the buffer
 */
jerry_size_t
jerry_string_to_buffer (const jerry_value_t value, /**< input string value */
                        jerry_encoding_t encoding, /**< output encoding */
                        jerry_char_t *buffer_p, /**< [out] output characters buffer */
                        jerry_size_t buffer_size) /**< size of output buffer */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_string (value) || buffer_p == NULL)
  {
    return 0;
  }

  ecma_string_t *str_p = ecma_get_string_from_value (value);

  return ecma_string_copy_to_buffer (str_p, (lit_utf8_byte_t *) buffer_p, buffer_size, encoding);
} /* jerry_string_to_char_buffer */

/**
 * Create a substring of the input string value.
 * Return an empty string if input value is not a string.
 *
 * @param value  the input string value
 * @param start  start position of the substring
 * @param end    end position of the substring
 *
 * @return created string
 */
jerry_value_t
jerry_string_substr (const jerry_value_t value, jerry_length_t start, jerry_length_t end)
{
  if (!ecma_is_value_string (value))
  {
    return ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
  }

  return ecma_make_string_value (ecma_string_substr (ecma_get_string_from_value (value), start, end));
} /* jerry_string_substr */

/**
 * Iterate over the input string value in the specified encoding, visiting each unit of the encoded string once. If
 * the input value is not a string, the function will do nothing.
 *
 * @param value     the input string value
 * @param callback  callback function called for each byte of the encoded string.
 * @param encoding  the requested encoding for the string
 * @param user_p    User pointer passed to the callback function
 */
void
jerry_string_iterate (const jerry_value_t value,
                      jerry_encoding_t encoding,
                      jerry_string_iterate_cb_t callback,
                      void *user_p)
{
  if (!ecma_is_value_string (value))
  {
    return;
  }

  ecma_string_t *str_p = ecma_get_string_from_value (value);
  ECMA_STRING_TO_UTF8_STRING (str_p, buffer_p, buffer_size);

  const lit_utf8_byte_t *current_p = buffer_p;
  const lit_utf8_byte_t *end_p = buffer_p + buffer_size;

  switch (encoding)
  {
    case JERRY_ENCODING_UTF8:
    {
      while (current_p < end_p)
      {
        if (JERRY_UNLIKELY (*current_p >= LIT_UTF8_3_BYTE_MARKER))
        {
          lit_code_point_t cp;
          lit_utf8_size_t read_size = lit_read_code_point_from_cesu8 (current_p, end_p, &cp);

          lit_utf8_byte_t bytes[LIT_UTF8_MAX_BYTES_IN_CODE_POINT];
          lit_utf8_size_t encoded_size = lit_code_point_to_utf8 (cp, bytes);

          for (uint32_t i = 0; i < encoded_size; i++)
          {
            callback (bytes[i], user_p);
          }

          current_p += read_size;
          continue;
        }

        callback (*current_p++, user_p);
      }

      break;
    }
    case JERRY_ENCODING_CESU8:
    {
      while (current_p < end_p)
      {
        callback (*current_p++, user_p);
      }

      break;
    }
    default:
    {
      break;
    }
  }
  ECMA_FINALIZE_UTF8_STRING (buffer_p, buffer_size);
} /* jerry_string_iterate */

/**
 * Sets the global callback which is called when an external string is freed.
 */
void
jerry_string_external_on_free (jerry_external_string_free_cb_t callback) /**< free callback */
{
  JERRY_CONTEXT (external_string_free_callback_p) = callback;
} /* jerry_string_external_on_free */

/**
 * Returns the user pointer assigned to an external string.
 *
 * @return user pointer, if value is an external string
 *         NULL, otherwise
 */
void *
jerry_string_user_ptr (const jerry_value_t value, /**< string value */
                       bool *is_external) /**< [out] true - if value is an external string,
                                           *         false - otherwise */
{
  if (is_external != NULL)
  {
    *is_external = false;
  }

  if (!ecma_is_value_string (value))
  {
    return NULL;
  }

  ecma_string_t *string_p = ecma_get_string_from_value (value);

  if (ECMA_IS_DIRECT_STRING (string_p)
      || ECMA_STRING_GET_CONTAINER (string_p) != ECMA_STRING_CONTAINER_LONG_OR_EXTERNAL_STRING)
  {
    return NULL;
  }

  ecma_long_string_t *long_string_p = (ecma_long_string_t *) string_p;

  if (long_string_p->string_p == ECMA_LONG_STRING_BUFFER_START (long_string_p))
  {
    return NULL;
  }

  if (is_external != NULL)
  {
    *is_external = true;
  }

  return ((ecma_external_string_t *) string_p)->user_p;
} /* jerry_string_user_ptr */

/**
 * Checks whether the object or it's prototype objects have the given property.
 *
 * @return raised error - if the operation fail
 *         true/false API value  - depend on whether the property exists
 */
jerry_value_t
jerry_object_has (const jerry_value_t object, /**< object value */
                  const jerry_value_t key) /**< property name (string value) */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);
  ecma_string_t *prop_name_p = ecma_get_prop_name_from_value (key);

  return jerry_return (ecma_op_object_has_property (obj_p, prop_name_p));
} /* jerry_object_has */

/**
 * Checks whether the object or it's prototype objects have the given property.
 *
 * @return raised error - if the operation fail
 *         true/false API value  - depend on whether the property exists
 */
jerry_value_t
jerry_object_has_sz (const jerry_value_t object, /**< object value */
                     const char *key_p) /**< property key */
{
  jerry_assert_api_enabled ();

  jerry_value_t key_str = jerry_string_sz (key_p);
  jerry_value_t result = jerry_object_has (object, key_str);
  ecma_free_value (key_str);

  return result;
} /* jerry_object_has */

/**
 * Checks whether the object has the given property.
 *
 * @return ECMA_VALUE_ERROR - if the operation raises error
 *         ECMA_VALUE_{TRUE, FALSE} - based on whether the property exists
 */
jerry_value_t
jerry_object_has_own (const jerry_value_t object, /**< object value */
                      const jerry_value_t key) /**< property name (string value) */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);
  ecma_string_t *prop_name_p = ecma_get_prop_name_from_value (key);

  return jerry_return (ecma_op_object_has_own_property (obj_p, prop_name_p));
} /* jerry_has_own_property */

/**
 * Checks whether the object has the given internal property.
 *
 * @return true  - if the internal property exists
 *         false - otherwise
 */
bool
jerry_object_has_internal (const jerry_value_t object, /**< object value */
                           const jerry_value_t key) /**< property name value */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return false;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);

  ecma_string_t *internal_string_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_API_INTERNAL);

  if (ecma_op_object_is_fast_array (obj_p))
  {
    return false;
  }

  ecma_property_t *property_p = ecma_find_named_property (obj_p, internal_string_p);

  if (property_p == NULL)
  {
    return false;
  }

  ecma_object_t *internal_object_p = ecma_get_object_from_value (ECMA_PROPERTY_VALUE_PTR (property_p)->value);
  property_p = ecma_find_named_property (internal_object_p, ecma_get_prop_name_from_value (key));

  return property_p != NULL;
} /* jerry_object_has_internal */

/**
 * Delete a property from an object.
 *
 * @return boolean value - wether the property was deleted successfully
 *         exception - otherwise
 */
jerry_value_t
jerry_object_delete (jerry_value_t object, /**< object value */
                     const jerry_value_t key) /**< property name (string value) */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return false;
  }

  return ecma_op_object_delete (ecma_get_object_from_value (object), ecma_get_prop_name_from_value (key), false);
} /* jerry_object_delete */

/**
 * Delete a property from an object.
 *
 * @return boolean value - wether the property was deleted successfully
 *         exception - otherwise
 */
jerry_value_t
jerry_object_delete_sz (jerry_value_t object, /**< object value */
                        const char *key_p) /**< property key */
{
  jerry_assert_api_enabled ();

  jerry_value_t key_str = jerry_string_sz (key_p);
  jerry_value_t result = jerry_object_delete (object, key_str);
  ecma_free_value (key_str);

  return result;
} /* jerry_object_delete */

/**
 * Delete indexed property from the specified object.
 *
 * @return boolean value - wether the property was deleted successfully
 *         false - otherwise
 */
jerry_value_t
jerry_object_delete_index (jerry_value_t object, /**< object value */
                           uint32_t index) /**< index to be written */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (object))
  {
    return false;
  }

  ecma_string_t *str_idx_p = ecma_new_ecma_string_from_uint32 (index);
  ecma_value_t ret_value = ecma_op_object_delete (ecma_get_object_from_value (object), str_idx_p, false);
  ecma_deref_ecma_string (str_idx_p);

  return ret_value;
} /* jerry_object_delete_index */

/**
 * Delete an internal property from an object.
 *
 * @return true  - if property was deleted successfully
 *         false - otherwise
 */
bool
jerry_object_delete_internal (jerry_value_t object, /**< object value */
                              const jerry_value_t key) /**< property name value */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return false;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);

  ecma_string_t *internal_string_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_API_INTERNAL);

  if (ecma_op_object_is_fast_array (obj_p))
  {
    return true;
  }

  ecma_property_t *property_p = ecma_find_named_property (obj_p, internal_string_p);

  if (property_p == NULL)
  {
    return true;
  }

  ecma_object_t *internal_object_p = ecma_get_object_from_value (ECMA_PROPERTY_VALUE_PTR (property_p)->value);
  property_p = ecma_find_named_property (internal_object_p, ecma_get_prop_name_from_value (key));

  if (property_p == NULL)
  {
    return true;
  }

  ecma_delete_property (internal_object_p, ECMA_PROPERTY_VALUE_PTR (property_p));

  return true;
} /* jerry_object_delete_internal */

/**
 * Get value of a property to the specified object with the given name.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return value of the property - if success
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_object_get (const jerry_value_t object, /**< object value */
                  const jerry_value_t key) /**< property name (string value) */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  jerry_value_t ret_value =
    ecma_op_object_get (ecma_get_object_from_value (object), ecma_get_prop_name_from_value (key));
  return jerry_return (ret_value);
} /* jerry_object_get */

/**
 * Get value of a property to the specified object with the given name.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return value of the property - if success
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_object_get_sz (const jerry_value_t object, /**< object value */
                     const char *key_p) /**< property key */
{
  jerry_assert_api_enabled ();

  jerry_value_t key_str = jerry_string_sz (key_p);
  jerry_value_t result = jerry_object_get (object, key_str);
  ecma_free_value (key_str);

  return result;
} /* jerry_object_get */

/**
 * Get value by an index from the specified object.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return value of the property specified by the index - if success
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_object_get_index (const jerry_value_t object, /**< object value */
                        uint32_t index) /**< index to be written */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (object))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_value_t ret_value = ecma_op_object_get_by_index (ecma_get_object_from_value (object), index);

  return jerry_return (ret_value);
} /* jerry_object_get_index */

/**
 * Get the own property value of an object with the given name.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return value of the property - if success
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_object_find_own (const jerry_value_t object, /**< object value */
                       const jerry_value_t key, /**< property name (string value) */
                       const jerry_value_t receiver, /**< receiver object value */
                       bool *found_p) /**< [out] true, if the property is found
                                       *   or object is a Proxy object, false otherwise */
{
  jerry_assert_api_enabled ();

  if (found_p != NULL)
  {
    *found_p = false;
  }

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key) || !ecma_is_value_object (receiver))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_object_t *object_p = ecma_get_object_from_value (object);
  ecma_string_t *property_name_p = ecma_get_prop_name_from_value (key);

#if JERRY_BUILTIN_PROXY
  if (ECMA_OBJECT_IS_PROXY (object_p))
  {
    if (found_p != NULL)
    {
      *found_p = true;
    }

    return jerry_return (ecma_proxy_object_get (object_p, property_name_p, receiver));
  }
#endif /* JERRY_BUILTIN_PROXY */

  ecma_value_t ret_value = ecma_op_object_find_own (receiver, object_p, property_name_p);

  if (ecma_is_value_found (ret_value))
  {
    if (found_p != NULL)
    {
      *found_p = true;
    }

    return jerry_return (ret_value);
  }

  return ECMA_VALUE_UNDEFINED;
} /* jerry_object_find_own */

/**
 * Get value of an internal property to the specified object with the given name.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return value of the internal property - if the internal property exists
 *         undefined value - if the internal does not property exists
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_object_get_internal (const jerry_value_t object, /**< object value */
                           const jerry_value_t key) /**< property name value */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);

  ecma_string_t *internal_string_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_API_INTERNAL);

  if (ecma_op_object_is_fast_array (obj_p))
  {
    return jerry_return (ECMA_VALUE_UNDEFINED);
  }

  ecma_property_t *property_p = ecma_find_named_property (obj_p, internal_string_p);

  if (property_p == NULL)
  {
    return jerry_return (ECMA_VALUE_UNDEFINED);
  }

  ecma_object_t *internal_object_p = ecma_get_object_from_value (ECMA_PROPERTY_VALUE_PTR (property_p)->value);
  property_p = ecma_find_named_property (internal_object_p, ecma_get_prop_name_from_value (key));

  if (property_p == NULL)
  {
    return jerry_return (ECMA_VALUE_UNDEFINED);
  }

  return jerry_return (ecma_copy_value (ECMA_PROPERTY_VALUE_PTR (property_p)->value));
} /* jerry_object_get_internal */

/**
 * Set a property to the specified object with the given name.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return true value - if the operation was successful
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_object_set (jerry_value_t object, /**< object value */
                  const jerry_value_t key, /**< property name (string value) */
                  const jerry_value_t value) /**< value to set */
{
  jerry_assert_api_enabled ();

  if (ecma_is_value_exception (value) || !ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  return jerry_return (
    ecma_op_object_put (ecma_get_object_from_value (object), ecma_get_prop_name_from_value (key), value, true));
} /* jerry_object_set */

/**
 * Set a property to the specified object with the given name.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return true value - if the operation was successful
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_object_set_sz (jerry_value_t object, /**< object value */
                     const char *key_p, /**< property key */
                     const jerry_value_t value) /**< value to set */
{
  jerry_assert_api_enabled ();

  jerry_value_t key_str = jerry_string_sz (key_p);
  jerry_value_t result = jerry_object_set (object, key_str, value);
  ecma_free_value (key_str);

  return result;
} /* jerry_object_set */

/**
 * Set indexed value in the specified object
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return true value - if the operation was successful
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_object_set_index (jerry_value_t object, /**< object value */
                        uint32_t index, /**< index to be written */
                        const jerry_value_t value) /**< value to set */
{
  jerry_assert_api_enabled ();

  if (ecma_is_value_exception (value) || !ecma_is_value_object (object))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_value_t ret_value = ecma_op_object_put_by_index (ecma_get_object_from_value (object), index, value, true);

  return jerry_return (ret_value);
} /* jerry_object_set_index */

/**
 * Set an internal property to the specified object with the given name.
 *
 * Note:
 *      - the property cannot be accessed from the JavaScript context, only from the public API
 *      - returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return true value - if the operation was successful
 *         value marked with error flag - otherwise
 */
bool
jerry_object_set_internal (jerry_value_t object, /**< object value */
                           const jerry_value_t key, /**< property name value */
                           const jerry_value_t value) /**< value to set */
{
  jerry_assert_api_enabled ();

  if (ecma_is_value_exception (value) || !ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return false;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);

  ecma_string_t *internal_string_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_API_INTERNAL);

  if (ecma_op_object_is_fast_array (obj_p))
  {
    ecma_fast_array_convert_to_normal (obj_p);
  }

  ecma_property_t *property_p = ecma_find_named_property (obj_p, internal_string_p);
  ecma_object_t *internal_object_p;

  if (property_p == NULL)
  {
    ecma_property_value_t *value_p =
      ecma_create_named_data_property (obj_p, internal_string_p, ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE, NULL);

    internal_object_p = ecma_create_object (NULL, sizeof (ecma_extended_object_t), ECMA_OBJECT_TYPE_CLASS);
    {
      ecma_extended_object_t *container_p = (ecma_extended_object_t *) internal_object_p;
      container_p->u.cls.type = ECMA_OBJECT_CLASS_INTERNAL_OBJECT;
    }

    value_p->value = ecma_make_object_value (internal_object_p);
    ecma_deref_object (internal_object_p);
  }
  else
  {
    internal_object_p = ecma_get_object_from_value (ECMA_PROPERTY_VALUE_PTR (property_p)->value);
  }

  ecma_string_t *prop_name_p = ecma_get_prop_name_from_value (key);
  property_p = ecma_find_named_property (internal_object_p, prop_name_p);

  if (property_p == NULL)
  {
    ecma_property_value_t *value_p = ecma_create_named_data_property (internal_object_p,
                                                                      prop_name_p,
                                                                      ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                                      NULL);

    value_p->value = ecma_copy_value_if_not_object (value);
  }
  else
  {
    ecma_named_data_property_assign_value (internal_object_p, ECMA_PROPERTY_VALUE_PTR (property_p), value);
  }

  return true;
} /* jerry_object_set_internal */

/**
 * Construct empty property descriptor, i.e.:
 *  property descriptor with all is_defined flags set to false and the rest - to default value.
 *
 * @return empty property descriptor
 */
jerry_property_descriptor_t
jerry_property_descriptor (void)
{
  jerry_property_descriptor_t prop_desc;

  prop_desc.flags = JERRY_PROP_NO_OPTS;
  prop_desc.value = ECMA_VALUE_UNDEFINED;
  prop_desc.getter = ECMA_VALUE_UNDEFINED;
  prop_desc.setter = ECMA_VALUE_UNDEFINED;

  return prop_desc;
} /* jerry_property_descriptor */

/**
 * Convert a ecma_property_descriptor_t to a jerry_property_descriptor_t
 *
 * if error occurs the property descriptor's value field is filled with ECMA_VALUE_ERROR
 *
 * @return jerry_property_descriptor_t
 */
static jerry_property_descriptor_t
jerry_property_descriptor_from_ecma (const ecma_property_descriptor_t *prop_desc_p) /**<[out] property_descriptor */
{
  jerry_property_descriptor_t prop_desc = jerry_property_descriptor ();

  prop_desc.flags = prop_desc_p->flags;

  if (prop_desc.flags & (JERRY_PROP_IS_VALUE_DEFINED))
  {
    prop_desc.value = prop_desc_p->value;
  }

  if (prop_desc_p->flags & JERRY_PROP_IS_GET_DEFINED)
  {
    prop_desc.getter = ECMA_VALUE_NULL;

    if (prop_desc_p->get_p != NULL)
    {
      prop_desc.getter = ecma_make_object_value (prop_desc_p->get_p);
      JERRY_ASSERT (ecma_op_is_callable (prop_desc.getter));
    }
  }

  if (prop_desc_p->flags & JERRY_PROP_IS_SET_DEFINED)
  {
    prop_desc.setter = ECMA_VALUE_NULL;

    if (prop_desc_p->set_p != NULL)
    {
      prop_desc.setter = ecma_make_object_value (prop_desc_p->set_p);
      JERRY_ASSERT (ecma_op_is_callable (prop_desc.setter));
    }
  }

  return prop_desc;
} /* jerry_property_descriptor_from_ecma */

/**
 * Convert a jerry_property_descriptor_t to a ecma_property_descriptor_t
 *
 * Note:
 *     if error occurs the property descriptor's value field
 *     is set to ECMA_VALUE_ERROR, but no error is thrown
 *
 * @return ecma_property_descriptor_t
 */
static ecma_property_descriptor_t
jerry_property_descriptor_to_ecma (const jerry_property_descriptor_t *prop_desc_p) /**< input property_descriptor */
{
  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();

  prop_desc.flags = prop_desc_p->flags;

  /* Copy data property info. */
  if (prop_desc_p->flags & JERRY_PROP_IS_VALUE_DEFINED)
  {
    if (ecma_is_value_exception (prop_desc_p->value)
        || (prop_desc_p->flags & (JERRY_PROP_IS_GET_DEFINED | JERRY_PROP_IS_SET_DEFINED)))
    {
      prop_desc.value = ECMA_VALUE_ERROR;
      return prop_desc;
    }

    prop_desc.value = prop_desc_p->value;
  }

  /* Copy accessor property info. */
  if (prop_desc_p->flags & JERRY_PROP_IS_GET_DEFINED)
  {
    ecma_value_t getter = prop_desc_p->getter;

    if (ecma_is_value_exception (getter))
    {
      prop_desc.value = ECMA_VALUE_ERROR;
      return prop_desc;
    }

    if (ecma_op_is_callable (getter))
    {
      prop_desc.get_p = ecma_get_object_from_value (getter);
    }
    else if (!ecma_is_value_null (getter))
    {
      prop_desc.value = ECMA_VALUE_ERROR;
      return prop_desc;
    }
  }

  if (prop_desc_p->flags & JERRY_PROP_IS_SET_DEFINED)
  {
    ecma_value_t setter = prop_desc_p->setter;

    if (ecma_is_value_exception (setter))
    {
      prop_desc.value = ECMA_VALUE_ERROR;
      return prop_desc;
    }

    if (ecma_op_is_callable (setter))
    {
      prop_desc.set_p = ecma_get_object_from_value (setter);
    }
    else if (!ecma_is_value_null (setter))
    {
      prop_desc.value = ECMA_VALUE_ERROR;
      return prop_desc;
    }
  }

  const uint16_t configurable_mask = JERRY_PROP_IS_CONFIGURABLE | JERRY_PROP_IS_CONFIGURABLE_DEFINED;
  const uint16_t enumerable_mask = JERRY_PROP_IS_ENUMERABLE | JERRY_PROP_IS_ENUMERABLE_DEFINED;
  const uint16_t writable_mask = JERRY_PROP_IS_WRITABLE | JERRY_PROP_IS_WRITABLE_DEFINED;

  if ((prop_desc_p->flags & configurable_mask) == JERRY_PROP_IS_CONFIGURABLE
      || (prop_desc_p->flags & enumerable_mask) == JERRY_PROP_IS_ENUMERABLE
      || (prop_desc_p->flags & writable_mask) == JERRY_PROP_IS_WRITABLE)
  {
    prop_desc.value = ECMA_VALUE_ERROR;
    return prop_desc;
  }

  prop_desc.flags |= (uint16_t) (prop_desc_p->flags | JERRY_PROP_SHOULD_THROW);

  return prop_desc;
} /* jerry_property_descriptor_to_ecma */

/** Helper function to return false value or error depending on the given flag.
 *
 * @return value marked with error flag - if is_throw is true
 *         false value - otherwise
 */
static jerry_value_t
jerry_type_error_or_false (ecma_error_msg_t msg, /**< message */
                           uint16_t flags) /**< property descriptor flags */
{
  if (!(flags & JERRY_PROP_SHOULD_THROW))
  {
    return ECMA_VALUE_FALSE;
  }

  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (msg));
} /* jerry_type_error_or_false */

/**
 * Define a property to the specified object with the given name.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return true value - if the operation was successful
 *         false value - if the property cannot be defined and JERRY_PROP_SHOULD_THROW is not set
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_object_define_own_prop (jerry_value_t object, /**< object value */
                              const jerry_value_t key, /**< property name (string value) */
                              const jerry_property_descriptor_t *prop_desc_p) /**< property descriptor */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return jerry_type_error_or_false (ECMA_ERR_WRONG_ARGS_MSG, prop_desc_p->flags);
  }

  if (prop_desc_p->flags & (JERRY_PROP_IS_WRITABLE_DEFINED | JERRY_PROP_IS_VALUE_DEFINED)
      && prop_desc_p->flags & (JERRY_PROP_IS_GET_DEFINED | JERRY_PROP_IS_SET_DEFINED))
  {
    return jerry_type_error_or_false (ECMA_ERR_WRONG_ARGS_MSG, prop_desc_p->flags);
  }

  ecma_property_descriptor_t prop_desc = jerry_property_descriptor_to_ecma (prop_desc_p);

  if (ECMA_IS_VALUE_ERROR (prop_desc.value))
  {
    return jerry_type_error_or_false (ECMA_ERR_WRONG_ARGS_MSG, prop_desc_p->flags);
  }

  return jerry_return (ecma_op_object_define_own_property (ecma_get_object_from_value (object),
                                                           ecma_get_prop_name_from_value (key),
                                                           &prop_desc));
} /* jerry_object_define_own_prop */

/**
 * Construct property descriptor from specified property.
 *
 * @return true - if success, the prop_desc_p fields contains the property info
 *         false - otherwise, the prop_desc_p is unchanged
 */
jerry_value_t
jerry_object_get_own_prop (const jerry_value_t object, /**< object value */
                           const jerry_value_t key, /**< property name (string value) */
                           jerry_property_descriptor_t *prop_desc_p) /**< property descriptor */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (object) || !ecma_is_value_prop_name (key))
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_property_descriptor_t prop_desc;

  ecma_value_t status = ecma_op_object_get_own_property_descriptor (ecma_get_object_from_value (object),
                                                                    ecma_get_prop_name_from_value (key),
                                                                    &prop_desc);

#if JERRY_BUILTIN_PROXY
  if (ECMA_IS_VALUE_ERROR (status))
  {
    return ecma_create_exception_from_context ();
  }
#endif /* JERRY_BUILTIN_PROXY */

  if (!ecma_is_value_true (status))
  {
    return ECMA_VALUE_FALSE;
  }

  /* The flags are always filled in the returned descriptor. */
  JERRY_ASSERT (
    (prop_desc.flags & JERRY_PROP_IS_CONFIGURABLE_DEFINED) && (prop_desc.flags & JERRY_PROP_IS_ENUMERABLE_DEFINED)
    && ((prop_desc.flags & JERRY_PROP_IS_WRITABLE_DEFINED) || !(prop_desc.flags & JERRY_PROP_IS_VALUE_DEFINED)));

  prop_desc_p->flags = prop_desc.flags;
  prop_desc_p->value = ECMA_VALUE_UNDEFINED;
  prop_desc_p->getter = ECMA_VALUE_UNDEFINED;
  prop_desc_p->setter = ECMA_VALUE_UNDEFINED;

  if (prop_desc_p->flags & JERRY_PROP_IS_VALUE_DEFINED)
  {
    prop_desc_p->value = prop_desc.value;
  }

  if (prop_desc_p->flags & JERRY_PROP_IS_GET_DEFINED)
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

  if (prop_desc_p->flags & JERRY_PROP_IS_SET_DEFINED)
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

  return ECMA_VALUE_TRUE;
} /* jerry_object_get_own_prop */

/**
 * Free fields of property descriptor (setter, getter and value).
 */
void
jerry_property_descriptor_free (jerry_property_descriptor_t *prop_desc_p) /**< property descriptor */
{
  if (prop_desc_p->flags & JERRY_PROP_IS_VALUE_DEFINED)
  {
    jerry_value_free (prop_desc_p->value);
  }

  if (prop_desc_p->flags & JERRY_PROP_IS_GET_DEFINED)
  {
    jerry_value_free (prop_desc_p->getter);
  }

  if (prop_desc_p->flags & JERRY_PROP_IS_SET_DEFINED)
  {
    jerry_value_free (prop_desc_p->setter);
  }
} /* jerry_property_descriptor_free */

/**
 * Call function specified by a function value
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *      error flag must not be set for any arguments of this function.
 *
 * @return returned jerry value of the called function
 */
jerry_value_t
jerry_call (const jerry_value_t func_object, /**< function object to call */
            const jerry_value_t this_value, /**< object for 'this' binding */
            const jerry_value_t *args_p, /**< function's call arguments */
            jerry_size_t args_count) /**< number of the arguments */
{
  jerry_assert_api_enabled ();

  if (ecma_is_value_exception (this_value))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  for (jerry_size_t i = 0; i < args_count; i++)
  {
    if (ecma_is_value_exception (args_p[i]))
    {
      return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
    }
  }

  return jerry_return (ecma_op_function_validated_call (func_object, this_value, args_p, args_count));
} /* jerry_call */

/**
 * Construct object value invoking specified function value as a constructor
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *      error flag must not be set for any arguments of this function.
 *
 * @return returned jerry value of the invoked constructor
 */
jerry_value_t
jerry_construct (const jerry_value_t func_object, /**< function object to call */
                 const jerry_value_t *args_p, /**< function's call arguments
                                               *   (NULL if arguments number is zero) */
                 jerry_size_t args_count) /**< number of the arguments */
{
  jerry_assert_api_enabled ();

  if (!jerry_value_is_constructor (func_object))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  for (jerry_size_t i = 0; i < args_count; i++)
  {
    if (ecma_is_value_exception (args_p[i]))
    {
      return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
    }
  }

  return jerry_return (ecma_op_function_construct (ecma_get_object_from_value (func_object),
                                                   ecma_get_object_from_value (func_object),
                                                   args_p,
                                                   args_count));
} /* jerry_construct */

/**
 * Get keys of the specified object value
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return array object value - if success
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_object_keys (const jerry_value_t object) /**< object value */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (object))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_collection_t *prop_names =
    ecma_op_object_get_enumerable_property_names (ecma_get_object_from_value (object), ECMA_ENUMERABLE_PROPERTY_KEYS);

#if JERRY_BUILTIN_PROXY
  if (JERRY_UNLIKELY (prop_names == NULL))
  {
    return ecma_create_exception_from_context ();
  }
#endif /* JERRY_BUILTIN_PROXY */

  return ecma_op_new_array_object_from_collection (prop_names, false);
} /* jerry_object_keys */

/**
 * Get the prototype of the specified object
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return prototype object or null value - if success
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_object_proto (const jerry_value_t object) /**< object value */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (object))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);

#if JERRY_BUILTIN_PROXY
  if (ECMA_OBJECT_IS_PROXY (obj_p))
  {
    return jerry_return (ecma_proxy_object_get_prototype_of (obj_p));
  }
#endif /* JERRY_BUILTIN_PROXY */

  if (obj_p->u2.prototype_cp == JMEM_CP_NULL)
  {
    return ECMA_VALUE_NULL;
  }

  ecma_object_t *proto_obj_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, obj_p->u2.prototype_cp);
  ecma_ref_object (proto_obj_p);

  return ecma_make_object_value (proto_obj_p);
} /* jerry_object_proto */

/**
 * Set the prototype of the specified object
 *
 * @return true value - if success
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_object_set_proto (jerry_value_t object, /**< object value */
                        const jerry_value_t proto) /**< prototype object value */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (object) || ecma_is_value_exception (proto)
      || (!ecma_is_value_object (proto) && !ecma_is_value_null (proto)))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }
  ecma_object_t *obj_p = ecma_get_object_from_value (object);

#if JERRY_BUILTIN_PROXY
  if (ECMA_OBJECT_IS_PROXY (obj_p))
  {
    return jerry_return (ecma_proxy_object_set_prototype_of (obj_p, proto));
  }
#endif /* JERRY_BUILTIN_PROXY */

  return ecma_op_ordinary_object_set_prototype_of (obj_p, proto);
} /* jerry_object_set_proto */

/**
 * Utility to check if a given object can be used for the foreach api calls.
 *
 * Some objects/classes uses extra internal objects to correctly store data.
 * These extre object should never be exposed externally to the API user.
 *
 * @returns true - if the user can access the object in the callback.
 *          false - if the object is an internal object which should no be accessed by the user.
 */
static bool
jerry_object_is_valid_foreach (ecma_object_t *object_p) /**< object to test */
{
  if (ecma_is_lexical_environment (object_p))
  {
    return false;
  }

  ecma_object_type_t object_type = ecma_get_object_type (object_p);

  if (object_type == ECMA_OBJECT_TYPE_CLASS)
  {
    ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
    switch (ext_object_p->u.cls.type)
    {
      /* An object's internal property object should not be iterable by foreach. */
      case ECMA_OBJECT_CLASS_INTERNAL_OBJECT:
      {
        return false;
      }
    }
  }

  return true;
} /* jerry_object_is_valid_foreach */

/**
 * Traverse objects.
 *
 * @return true - traversal was interrupted by the callback.
 *         false - otherwise - traversal visited all objects.
 */
bool
jerry_foreach_live_object (jerry_foreach_live_object_cb_t callback, /**< function pointer of the iterator function */
                           void *user_data_p) /**< pointer to user data */
{
  jerry_assert_api_enabled ();

  JERRY_ASSERT (callback != NULL);

  jmem_cpointer_t iter_cp = JERRY_CONTEXT (ecma_gc_objects_cp);

  while (iter_cp != JMEM_CP_NULL)
  {
    ecma_object_t *iter_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, iter_cp);

    if (jerry_object_is_valid_foreach (iter_p) && !callback (ecma_make_object_value (iter_p), user_data_p))
    {
      return true;
    }

    iter_cp = iter_p->gc_next_cp;
  }

  return false;
} /* jerry_foreach_live_object */

/**
 * Traverse objects having a given native type info.
 *
 * @return true - traversal was interrupted by the callback.
 *         false - otherwise - traversal visited all objects.
 */
bool
jerry_foreach_live_object_with_info (const jerry_object_native_info_t *native_info_p, /**< the type info
                                                                                       *   of the native pointer */
                                     jerry_foreach_live_object_with_info_cb_t callback, /**< function to apply for
                                                                                         *   each matching object */
                                     void *user_data_p) /**< pointer to user data */
{
  jerry_assert_api_enabled ();

  JERRY_ASSERT (native_info_p != NULL);
  JERRY_ASSERT (callback != NULL);

  ecma_native_pointer_t *native_pointer_p;

  jmem_cpointer_t iter_cp = JERRY_CONTEXT (ecma_gc_objects_cp);

  while (iter_cp != JMEM_CP_NULL)
  {
    ecma_object_t *iter_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, iter_cp);

    if (jerry_object_is_valid_foreach (iter_p))
    {
      native_pointer_p = ecma_get_native_pointer_value (iter_p, (void *) native_info_p);
      if (native_pointer_p && !callback (ecma_make_object_value (iter_p), native_pointer_p->native_p, user_data_p))
      {
        return true;
      }
    }

    iter_cp = iter_p->gc_next_cp;
  }

  return false;
} /* jerry_foreach_live_object_with_info */

/**
 * Get native pointer and its type information, associated with the given native type info.
 *
 * Note:
 *  If native pointer is present, its type information is returned in out_native_pointer_p
 *
 * @return found native pointer,
 *         or NULL
 */
void *
jerry_object_get_native_ptr (const jerry_value_t object, /**< object to get native pointer from */
                             const jerry_object_native_info_t *native_info_p) /**< the type info
                                                                               *   of the native pointer */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (object))
  {
    return NULL;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);
  ecma_native_pointer_t *native_pointer_p = ecma_get_native_pointer_value (obj_p, (void *) native_info_p);

  if (native_pointer_p == NULL)
  {
    return NULL;
  }

  return native_pointer_p->native_p;
} /* jerry_object_get_native_ptr */

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
 *      Referred values by this method must have at least 1 reference. (Correct API usage satisfies this condition)
 *      The type info always overwrites the previous value, so passing
 *      a NULL value deletes the current type info.
 */
void
jerry_object_set_native_ptr (jerry_value_t object, /**< object to set native pointer in */
                             const jerry_object_native_info_t *native_info_p, /**< object's native type info */
                             void *native_pointer_p) /**< native pointer */
{
  jerry_assert_api_enabled ();

  if (ecma_is_value_object (object))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (object);

    ecma_create_native_pointer_property (object_p, native_pointer_p, native_info_p);
  }
} /* jerry_object_set_native_ptr */

/**
 * Checks whether the argument object has a native pointer set for the specified native type info.
 *
 * @return true if the native pointer has been set,
 *         false otherwise
 */
bool
jerry_object_has_native_ptr (const jerry_value_t object, /**< object to set native pointer in */
                             const jerry_object_native_info_t *native_info_p) /**< object's native type info */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (object))
  {
    return false;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);
  ecma_native_pointer_t *native_pointer_p = ecma_get_native_pointer_value (obj_p, (void *) native_info_p);

  return native_pointer_p != NULL;
} /* jerry_object_has_native_ptr */

/**
 * Delete the previously set native pointer by the native type info from the specified object.
 *
 * Note:
 *      If the specified object has no matching native pointer for the given native type info
 *      the function has no effect.
 *
 * Note:
 *      This operation cannot throw an exception.
 *
 * @return true - if the native pointer has been deleted succesfully
 *         false - otherwise
 */
bool
jerry_object_delete_native_ptr (jerry_value_t object, /**< object to delete native pointer from */
                                const jerry_object_native_info_t *native_info_p) /**< object's native type info */
{
  jerry_assert_api_enabled ();

  if (ecma_is_value_object (object))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (object);

    return ecma_delete_native_pointer_property (object_p, (void *) native_info_p);
  }

  return false;
} /* jerry_object_delete_native_ptr */

/**
 * Initialize the references stored in a buffer pointed by a native pointer.
 * The references are initialized to undefined.
 */
void
jerry_native_ptr_init (void *native_pointer_p, /**< a valid non-NULL pointer to a native buffer */
                       const jerry_object_native_info_t *native_info_p) /**< the type info of
                                                                         *   the native pointer */
{
  jerry_assert_api_enabled ();

  if (native_pointer_p == NULL || native_info_p == NULL)
  {
    return;
  }

  ecma_value_t *value_p = (ecma_value_t *) (((uint8_t *) native_pointer_p) + native_info_p->offset_of_references);
  ecma_value_t *end_p = value_p + native_info_p->number_of_references;

  while (value_p < end_p)
  {
    *value_p++ = ECMA_VALUE_UNDEFINED;
  }
} /* jerry_native_ptr_init */

/**
 * Release the value references after a buffer pointed by a native pointer
 * is not attached to an object anymore. All references are set to undefined
 * similar to jerry_native_ptr_init.
 */
void
jerry_native_ptr_free (void *native_pointer_p, /**< a valid non-NULL pointer to a native buffer */
                       const jerry_object_native_info_t *native_info_p) /**< the type info of
                                                                         *   the native pointer */
{
  jerry_assert_api_enabled ();

  if (native_pointer_p == NULL || native_info_p == NULL)
  {
    return;
  }

  ecma_value_t *value_p = (ecma_value_t *) (((uint8_t *) native_pointer_p) + native_info_p->offset_of_references);
  ecma_value_t *end_p = value_p + native_info_p->number_of_references;

  while (value_p < end_p)
  {
    ecma_free_value_if_not_object (*value_p);
    *value_p++ = ECMA_VALUE_UNDEFINED;
  }
} /* jerry_native_ptr_free */

/**
 * Updates a value reference inside the area specified by the number_of_references and
 * offset_of_references fields in its corresponding jerry_object_native_info_t data.
 * The area must be part of a buffer which is currently assigned to an object.
 *
 * Note:
 *      Error references are not supported, they are replaced by undefined values.
 */
void
jerry_native_ptr_set (jerry_value_t *reference_p, /**< a valid non-NULL pointer to
                                                   *   a reference in a native buffer. */
                      const jerry_value_t value) /**< new value of the reference */
{
  jerry_assert_api_enabled ();

  if (reference_p == NULL)
  {
    return;
  }

  ecma_free_value_if_not_object (*reference_p);

  if (ecma_is_value_exception (value))
  {
    *reference_p = ECMA_VALUE_UNDEFINED;
    return;
  }

  *reference_p = ecma_copy_value_if_not_object (value);
} /* jerry_native_ptr_set */

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
jerry_object_foreach (const jerry_value_t object, /**< object value */
                      jerry_object_property_foreach_cb_t foreach_p, /**< foreach function */
                      void *user_data_p) /**< user data for foreach function */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (object))
  {
    return false;
  }

  ecma_object_t *object_p = ecma_get_object_from_value (object);
  ecma_collection_t *names_p = ecma_op_object_enumerate (object_p);

#if JERRY_BUILTIN_PROXY
  if (names_p == NULL)
  {
    // TODO: Due to Proxies the return value must be changed to jerry_value_t on next release
    jcontext_release_exception ();
    return false;
  }
#endif /* JERRY_BUILTIN_PROXY */

  ecma_value_t *buffer_p = names_p->buffer_p;

  ecma_value_t property_value = ECMA_VALUE_EMPTY;

  bool continuous = true;

  for (uint32_t i = 0; continuous && (i < names_p->item_count); i++)
  {
    ecma_string_t *property_name_p = ecma_get_string_from_value (buffer_p[i]);

    property_value = ecma_op_object_get (object_p, property_name_p);

    if (ECMA_IS_VALUE_ERROR (property_value))
    {
      break;
    }

    continuous = foreach_p (buffer_p[i], property_value, user_data_p);
    ecma_free_value (property_value);
  }

  ecma_collection_free (names_p);

  if (!ECMA_IS_VALUE_ERROR (property_value))
  {
    return true;
  }

  jcontext_release_exception ();
  return false;
} /* jerry_object_foreach */

/**
 * Gets the property keys for the given object using the selected filters.
 *
 * @return array containing the filtered property keys in successful operation
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_object_property_names (const jerry_value_t object, /**< object */
                             jerry_property_filter_t filter) /**< property filter options */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_object (object))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (object);
  ecma_object_t *obj_iter_p = obj_p;
  ecma_collection_t *result_p = ecma_new_collection ();

  ecma_ref_object (obj_iter_p);

  while (true)
  {
    /* Step 1. Get Object.[[OwnKeys]] */
    ecma_collection_t *prop_names_p = ecma_op_object_own_property_keys (obj_iter_p, filter);

#if JERRY_BUILTIN_PROXY
    if (prop_names_p == NULL)
    {
      ecma_deref_object (obj_iter_p);
      return ecma_create_exception_from_context ();
    }
#endif /* JERRY_BUILTIN_PROXY */

    for (uint32_t i = 0; i < prop_names_p->item_count; i++)
    {
      ecma_value_t key = prop_names_p->buffer_p[i];
      ecma_string_t *key_p = ecma_get_prop_name_from_value (key);
      uint32_t index = ecma_string_get_array_index (key_p);

      /* Step 2. Filter by key type */
      if (filter
          & (JERRY_PROPERTY_FILTER_EXCLUDE_STRINGS | JERRY_PROPERTY_FILTER_EXCLUDE_SYMBOLS
             | JERRY_PROPERTY_FILTER_EXCLUDE_INTEGER_INDICES))
      {
        if (ecma_is_value_symbol (key))
        {
          if (filter & JERRY_PROPERTY_FILTER_EXCLUDE_SYMBOLS)
          {
            continue;
          }
        }
        else if (index != ECMA_STRING_NOT_ARRAY_INDEX)
        {
          if ((filter & JERRY_PROPERTY_FILTER_EXCLUDE_INTEGER_INDICES)
              || ((filter & JERRY_PROPERTY_FILTER_EXCLUDE_STRINGS)
                  && !(filter & JERRY_PROPERTY_FILTER_INTEGER_INDICES_AS_NUMBER)))
          {
            continue;
          }
        }
        else if (filter & JERRY_PROPERTY_FILTER_EXCLUDE_STRINGS)
        {
          continue;
        }
      }

      /* Step 3. Filter property attributes */
      if (filter
          & (JERRY_PROPERTY_FILTER_EXCLUDE_NON_CONFIGURABLE | JERRY_PROPERTY_FILTER_EXCLUDE_NON_ENUMERABLE
             | JERRY_PROPERTY_FILTER_EXCLUDE_NON_WRITABLE))
      {
        ecma_property_descriptor_t prop_desc;
        ecma_value_t status = ecma_op_object_get_own_property_descriptor (obj_iter_p, key_p, &prop_desc);

#if JERRY_BUILTIN_PROXY
        if (ECMA_IS_VALUE_ERROR (status))
        {
          ecma_collection_free (prop_names_p);
          ecma_collection_free (result_p);
          ecma_deref_object (obj_iter_p);
          return ecma_create_exception_from_context ();
        }
#endif /* JERRY_BUILTIN_PROXY */

        JERRY_ASSERT (ecma_is_value_true (status));
        uint16_t flags = prop_desc.flags;
        ecma_free_property_descriptor (&prop_desc);

        if ((!(flags & JERRY_PROP_IS_CONFIGURABLE) && (filter & JERRY_PROPERTY_FILTER_EXCLUDE_NON_CONFIGURABLE))
            || (!(flags & JERRY_PROP_IS_ENUMERABLE) && (filter & JERRY_PROPERTY_FILTER_EXCLUDE_NON_ENUMERABLE))
            || (!(flags & JERRY_PROP_IS_WRITABLE) && (filter & JERRY_PROPERTY_FILTER_EXCLUDE_NON_WRITABLE)))
        {
          continue;
        }
      }

      if (index != ECMA_STRING_NOT_ARRAY_INDEX && (filter & JERRY_PROPERTY_FILTER_INTEGER_INDICES_AS_NUMBER))
      {
        ecma_deref_ecma_string (key_p);
        key = ecma_make_uint32_value (index);
      }
      else
      {
        ecma_ref_ecma_string (key_p);
      }

      if ((filter & JERRY_PROPERTY_FILTER_TRAVERSE_PROTOTYPE_CHAIN) && obj_iter_p != obj_p)
      {
        uint32_t duplicate_idx = 0;
        while (duplicate_idx < result_p->item_count)
        {
          ecma_value_t value = result_p->buffer_p[duplicate_idx];
          JERRY_ASSERT (ecma_is_value_prop_name (value) || ecma_is_value_number (value));
          if (JERRY_UNLIKELY (ecma_is_value_number (value)))
          {
            if (ecma_get_number_from_value (value) == ecma_get_number_from_value (key))
            {
              break;
            }
          }
          else if (ecma_compare_ecma_strings (ecma_get_prop_name_from_value (value), key_p))
          {
            break;
          }

          duplicate_idx++;
        }

        if (duplicate_idx == result_p->item_count)
        {
          ecma_collection_push_back (result_p, key);
        }
      }
      else
      {
        ecma_collection_push_back (result_p, key);
      }
    }

    ecma_collection_free (prop_names_p);

    /* Step 4: Traverse prototype chain */

    if ((filter & JERRY_PROPERTY_FILTER_TRAVERSE_PROTOTYPE_CHAIN) != JERRY_PROPERTY_FILTER_TRAVERSE_PROTOTYPE_CHAIN)
    {
      break;
    }

    ecma_object_t *proto_p = ecma_op_object_get_prototype_of (obj_iter_p);

    if (proto_p == NULL)
    {
      break;
    }

    ecma_deref_object (obj_iter_p);

    if (JERRY_UNLIKELY (proto_p == ECMA_OBJECT_POINTER_ERROR))
    {
      ecma_collection_free (result_p);
      return ecma_create_exception_from_context ();
    }

    obj_iter_p = proto_p;
  }

  ecma_deref_object (obj_iter_p);

  return ecma_op_new_array_object_from_collection (result_p, false);
} /* jerry_object_property_names */

/**
 * FromPropertyDescriptor abstract operation.
 *
 * @return new jerry_value_t - if success
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_property_descriptor_to_object (const jerry_property_descriptor_t *src_prop_desc_p) /**< property descriptor */
{
  jerry_assert_api_enabled ();

  ecma_property_descriptor_t prop_desc = jerry_property_descriptor_to_ecma (src_prop_desc_p);

  if (ECMA_IS_VALUE_ERROR (prop_desc.value))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_object_t *desc_obj_p = ecma_op_from_property_descriptor (&prop_desc);

  return ecma_make_object_value (desc_obj_p);
} /* jerry_property_descriptor_to_object */

/**
 * ToPropertyDescriptor abstract operation.
 *
 * @return true - if the conversion is successful
 *         thrown error - otherwise
 */
jerry_value_t
jerry_property_descriptor_from_object (const jerry_value_t object, /**< object value */
                                       jerry_property_descriptor_t *out_prop_desc_p) /**< [out] filled property
                                                                                      * descriptor if return value is
                                                                                      * true, unmodified otherwise */
{
  jerry_assert_api_enabled ();

  ecma_property_descriptor_t prop_desc;
  jerry_value_t result = ecma_op_to_property_descriptor (object, &prop_desc);

  if (ECMA_IS_VALUE_ERROR (result))
  {
    return ecma_create_exception_from_context ();
  }

  JERRY_ASSERT (result == ECMA_VALUE_EMPTY);

  *out_prop_desc_p = jerry_property_descriptor_from_ecma (&prop_desc);
  return ECMA_VALUE_TRUE;
} /* jerry_property_descriptor_from_object */

/**
 * Resolve a promise value with an argument.
 *
 * @return undefined - if success,
 *         exception - otherwise
 */
jerry_value_t
jerry_promise_resolve (jerry_value_t promise, /**< the promise value */
                       const jerry_value_t argument) /**< the argument */
{
  jerry_assert_api_enabled ();

  if (!jerry_value_is_promise (promise))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  if (ecma_is_value_exception (argument))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  return ecma_fulfill_promise_with_checks (promise, argument);
} /* jerry_promise_resolve */

/**
 * Reject a promise value with an argument.
 *
 * @return undefined - if success,
 *         exception - otherwise
 */
jerry_value_t
jerry_promise_reject (jerry_value_t promise, /**< the promise value */
                      const jerry_value_t argument) /**< the argument */
{
  jerry_assert_api_enabled ();

  if (!jerry_value_is_promise (promise))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  if (ecma_is_value_exception (argument))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  return ecma_reject_promise_with_checks (promise, argument);
} /* jerry_promise_reject */

/**
 * Get the result of a promise.
 *
 * @return - Promise result
 *         - Type error if the promise support was not enabled or the input was not a promise object
 */
jerry_value_t
jerry_promise_result (const jerry_value_t promise) /**< promise object to get the result from */
{
  jerry_assert_api_enabled ();

  if (!jerry_value_is_promise (promise))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  return ecma_promise_get_result (ecma_get_object_from_value (promise));
} /* jerry_get_promise_result */

/**
 * Get the state of a promise object.
 *
 * @return - the state of the promise (one of the jerry_promise_state_t enum values)
 *         - JERRY_PROMISE_STATE_NONE is only returned if the input is not a promise object
 *           or the promise support was not enabled.
 */
jerry_promise_state_t
jerry_promise_state (const jerry_value_t promise) /**< promise object to get the state from */
{
  jerry_assert_api_enabled ();

  if (!jerry_value_is_promise (promise))
  {
    return JERRY_PROMISE_STATE_NONE;
  }

  uint16_t flags = ecma_promise_get_flags (ecma_get_object_from_value (promise));
  flags &= (ECMA_PROMISE_IS_PENDING | ECMA_PROMISE_IS_FULFILLED);

  return (flags ? flags : JERRY_PROMISE_STATE_REJECTED);
} /* jerry_get_promise_state */

/**
 * Sets a callback for tracking Promise and async operations.
 *
 * Note:
 *     the previous callback is overwritten
 */
void
jerry_promise_on_event (jerry_promise_event_filter_t filters, /**< combination of event filters */
                        jerry_promise_event_cb_t callback, /**< notification callback */
                        void *user_p) /**< user pointer passed to the callback */
{
  jerry_assert_api_enabled ();

#if JERRY_PROMISE_CALLBACK
  if (filters == JERRY_PROMISE_EVENT_FILTER_DISABLE || callback == NULL)
  {
    JERRY_CONTEXT (promise_callback_filters) = JERRY_PROMISE_EVENT_FILTER_DISABLE;
    return;
  }

  JERRY_CONTEXT (promise_callback_filters) = (uint32_t) filters;
  JERRY_CONTEXT (promise_callback) = callback;
  JERRY_CONTEXT (promise_callback_user_p) = user_p;
#else /* !JERRY_PROMISE_CALLBACK */
  JERRY_UNUSED (filters);
  JERRY_UNUSED (callback);
  JERRY_UNUSED (user_p);
#endif /* JERRY_PROMISE_CALLBACK */
} /* jerry_promise_set_callback */

/**
 * Get the well-knwon symbol represented by the given `symbol` enum value.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return undefined value - if invalid well-known symbol was requested
 *         well-known symbol value - otherwise
 */
jerry_value_t
jerry_symbol (jerry_well_known_symbol_t symbol) /**< jerry_well_known_symbol_t enum value */
{
  jerry_assert_api_enabled ();

  lit_magic_string_id_t id = (lit_magic_string_id_t) (LIT_GLOBAL_SYMBOL__FIRST + symbol);

  if (!LIT_IS_GLOBAL_SYMBOL (id))
  {
    return ECMA_VALUE_UNDEFINED;
  }

  return ecma_make_symbol_value (ecma_op_get_global_symbol (id));
} /* jerry_get_well_known_symbol */

/**
 * Returns the description internal property of a symbol.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return string or undefined value containing the symbol's description - if success
 *         thrown error - otherwise
 */
jerry_value_t
jerry_symbol_description (const jerry_value_t symbol) /**< symbol value */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_symbol (symbol))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  /* Note: This operation cannot throw an error */
  return ecma_copy_value (ecma_get_symbol_description (ecma_get_symbol_from_value (symbol)));
} /* jerry_get_symbol_description */

/**
 * Call the SymbolDescriptiveString ecma builtin operation on the symbol value.
 *
 * Note:
 *      returned value must be freed with jerry_value_free, when it is no longer needed.
 *
 * @return string value containing the symbol's descriptive string - if success
 *         thrown error - otherwise
 */
jerry_value_t
jerry_symbol_descriptive_string (const jerry_value_t symbol) /**< symbol value */
{
  jerry_assert_api_enabled ();

  if (!ecma_is_value_symbol (symbol))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  /* Note: This operation cannot throw an error */
  return ecma_get_symbol_descriptive_string (symbol);
} /* jerry_get_symbol_descriptive_string */

/**
 * Get the number of uint64 digits of a BigInt value
 *
 * @return number of uint64 digits
 */
uint32_t
jerry_bigint_digit_count (const jerry_value_t value) /**< BigInt value */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_BIGINT
  if (!ecma_is_value_bigint (value))
  {
    return 0;
  }

  return ecma_bigint_get_size_in_digits (value);
#else /* !JERRY_BUILTIN_BIGINT */
  JERRY_UNUSED (value);
  return 0;
#endif /* JERRY_BUILTIN_BIGINT */
} /* jerry_bigint_digit_count */

/**
 * Get the uint64 digits of a BigInt value (lowest digit first)
 */
void
jerry_bigint_to_digits (const jerry_value_t value, /**< BigInt value */
                        uint64_t *digits_p, /**< [out] buffer for digits */
                        uint32_t digit_count, /**< buffer size in digits */
                        bool *sign_p) /**< [out] sign of BigInt */
{
#if JERRY_BUILTIN_BIGINT
  if (!ecma_is_value_bigint (value))
  {
    if (sign_p != NULL)
    {
      *sign_p = false;
    }
    memset (digits_p, 0, digit_count * sizeof (uint64_t));
  }

  ecma_bigint_get_digits_and_sign (value, digits_p, digit_count, sign_p);
#else /* !JERRY_BUILTIN_BIGINT */
  JERRY_UNUSED (value);

  if (sign_p != NULL)
  {
    *sign_p = false;
  }
  memset (digits_p, 0, digit_count * sizeof (uint64_t));
#endif /* JERRY_BUILTIN_BIGINT */
} /* jerry_bigint_to_digits */

/**
 * Get the target object of a Proxy object
 *
 * @return type error - if proxy_value is not a Proxy object
 *         target object - otherwise
 */
jerry_value_t
jerry_proxy_target (const jerry_value_t proxy_value) /**< proxy value */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_PROXY
  if (ecma_is_value_object (proxy_value))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (proxy_value);

    if (ECMA_OBJECT_IS_PROXY (object_p))
    {
      ecma_proxy_object_t *proxy_object_p = (ecma_proxy_object_t *) object_p;

      if (!ecma_is_value_null (proxy_object_p->target))
      {
        ecma_ref_object (ecma_get_object_from_value (proxy_object_p->target));
      }
      return proxy_object_p->target;
    }
  }
#else /* !JERRY_BUILTIN_PROXY */
  JERRY_UNUSED (proxy_value);
#endif /* JERRY_BUILTIN_PROXY */

  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ARGUMENT_IS_NOT_A_PROXY));
} /* jerry_proxy_target */

/**
 * Get the handler object of a Proxy object
 *
 * @return type error - if proxy_value is not a Proxy object
 *         handler object - otherwise
 */
jerry_value_t
jerry_proxy_handler (const jerry_value_t proxy_value) /**< proxy value */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_PROXY
  if (ecma_is_value_object (proxy_value))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (proxy_value);

    if (ECMA_OBJECT_IS_PROXY (object_p))
    {
      ecma_proxy_object_t *proxy_object_p = (ecma_proxy_object_t *) object_p;

      if (!ecma_is_value_null (proxy_object_p->handler))
      {
        ecma_ref_object (ecma_get_object_from_value (proxy_object_p->handler));
      }
      return proxy_object_p->handler;
    }
  }
#else /* !JERRY_BUILTIN_PROXY */
  JERRY_UNUSED (proxy_value);
#endif /* JERRY_BUILTIN_PROXY */

  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ARGUMENT_IS_NOT_A_PROXY));
} /* jerry_proxy_handler */

/**
 * Validate string buffer for the specified encoding
 *
 * @return true - if string is well-formed
 *         false - otherwise
 */
bool
jerry_validate_string (const jerry_char_t *buffer_p, /**< string buffer */
                       jerry_size_t buffer_size, /**< buffer size */
                       jerry_encoding_t encoding) /**< buffer encoding */
{
  switch (encoding)
  {
    case JERRY_ENCODING_CESU8:
    {
      return lit_is_valid_cesu8_string (buffer_p, buffer_size);
    }
    case JERRY_ENCODING_UTF8:
    {
      return lit_is_valid_utf8_string (buffer_p, buffer_size, true);
    }
    default:
    {
      return false;
    }
  }
} /* jerry_validate_string */

/**
 * Set the log level of the engine.
 *
 * Log messages with lower significance than the current log level will be ignored by `jerry_log`.
 *
 * @param level: requested log level
 */
void
jerry_log_set_level (jerry_log_level_t level)
{
  jerry_jrt_set_log_level (level);
} /* jerry_log_set_level */

/**
 * Log buffer size
 */
#define JERRY_LOG_BUFFER_SIZE 64

/**
 * Log a zero-terminated string message.
 *
 * @param str_p: message
 */
static void
jerry_log_string (const char *str_p)
{
  jerry_port_log (str_p);

#if JERRY_DEBUGGER
  if (jerry_debugger_is_connected ())
  {
    jerry_debugger_send_string (JERRY_DEBUGGER_OUTPUT_RESULT,
                                JERRY_DEBUGGER_OUTPUT_LOG,
                                (const uint8_t *) str_p,
                                strlen (str_p));
  }
#endif /* JERRY_DEBUGGER */
} /* jerry_log_string */

/**
 * Log a fixed-size string message.
 *
 * @param str_p: message
 * @param size: size
 * @param buffer_p: buffer to use
 */
static void
jerry_log_string_fixed (const char *str_p, size_t size, char *buffer_p)
{
  const size_t batch_size = JERRY_LOG_BUFFER_SIZE - 1;

  while (size > batch_size)
  {
    memcpy (buffer_p, str_p, batch_size);
    buffer_p[batch_size] = '\0';
    jerry_log_string (buffer_p);

    str_p += batch_size;
    size -= batch_size;
  }

  memcpy (buffer_p, str_p, size);
  buffer_p[size] = '\0';
  jerry_log_string (buffer_p);
} /* jerry_log_string_fixed */

/**
 * Format an unsigned number.
 *
 * @param num: number
 * @param width: minimum width of the number
 * @param padding: padding to be used when extending to minimum width
 * @param radix: number radix
 * @param buffer_p: buffer used to construct the number string
 *
 * @return formatted number as string
 */
static char *
jerry_format_unsigned (unsigned int num, uint8_t width, char padding, uint8_t radix, char *buffer_p)
{
  static const char digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
  char *cursor_p = buffer_p + JERRY_LOG_BUFFER_SIZE;
  *(--cursor_p) = '\0';
  uint8_t count = 0;

  do
  {
    *(--cursor_p) = digits[num % radix];
    num /= radix;
    count++;
  } while (num > 0);

  while (count < width)
  {
    *(--cursor_p) = padding;
    count++;
  }

  return cursor_p;
} /* jerry_format_unsigned */

/**
 * Format an integer number.
 *
 * @param num: number
 * @param width: minimum width of the number
 * @param padding: padding to be used when extending to minimum width
 * @param buffer_p: buffer used to construct the number string
 *
 * @return formatted number as string
 */
static char *
jerry_format_int (int num, uint8_t width, char padding, char *buffer_p)
{
  if (num >= 0)
  {
    return jerry_format_unsigned ((unsigned) num, width, padding, 10, buffer_p);
  }

  num = -num;

  if (padding == '0' && width > 0)
  {
    char *cursor_p = jerry_format_unsigned ((unsigned) num, --width, padding, 10, buffer_p);
    *(--cursor_p) = '-';
    return cursor_p;
  }

  char *cursor_p = jerry_format_unsigned ((unsigned) num, 0, ' ', 10, buffer_p);
  *(--cursor_p) = '-';

  char *indent_p = buffer_p + JERRY_LOG_BUFFER_SIZE - width - 1;

  while (cursor_p > indent_p)
  {
    *(--cursor_p) = ' ';
  }

  return cursor_p;
} /* jerry_foramt_int */

/**
 * Log a zero-terminated formatted message with the specified log level.
 *
 * Supported format specifiers:
 *   %s: zero-terminated string
 *   %c: character
 *   %u: unsigned integer
 *   %d: decimal integer
 *   %x: unsigned hexadecimal
 *
 * Width and padding sub-modifiers are also supported.
 *
 * @param format_p: format string
 * @param level: message log level
 */
void
jerry_log (jerry_log_level_t level, const char *format_p, ...)
{
  if (level > jerry_jrt_get_log_level ())
  {
    return;
  }

  va_list vl;
  char buffer_p[JERRY_LOG_BUFFER_SIZE];
  uint32_t buffer_index = 0;
  const char *cursor_p = format_p;
  va_start (vl, format_p);

  while (*cursor_p != '\0')
  {
    if (*cursor_p == '%' || buffer_index > JERRY_LOG_BUFFER_SIZE - 2)
    {
      buffer_p[buffer_index] = '\0';
      jerry_log_string (buffer_p);
      buffer_index = 0;
    }

    if (*cursor_p != '%')
    {
      buffer_p[buffer_index++] = *cursor_p++;
      continue;
    }

    ++cursor_p;
    uint8_t width = 0;
    size_t precision = 0;
    char padding = ' ';

    if (*cursor_p == '0')
    {
      padding = '0';
      cursor_p++;
    }

    if (lit_char_is_decimal_digit ((ecma_char_t) *cursor_p))
    {
      width = (uint8_t) (*cursor_p - '0');
      cursor_p++;
    }
    else if (*cursor_p == '*')
    {
      width = (uint8_t) JERRY_MAX (va_arg (vl, int), 10);
      cursor_p++;
    }
    else if (*cursor_p == '.' && *(cursor_p + 1) == '*')
    {
      precision = (size_t) va_arg (vl, int);
      cursor_p += 2;
    }

    if (*cursor_p == '\0')
    {
      buffer_p[buffer_index++] = '%';
      break;
    }

    /* The buffer is always flushed before a substitution, and can be reused to for formatting. */
    switch (*cursor_p++)
    {
      case 's':
      {
        char *str_p = va_arg (vl, char *);

        if (precision == 0)
        {
          jerry_log_string (str_p);
          break;
        }

        jerry_log_string_fixed (str_p, precision, buffer_p);
        break;
      }
      case 'c':
      {
        /* Arguments of types narrower than int are promoted to int for variadic functions */
        buffer_p[buffer_index++] = (char) va_arg (vl, int);
        break;
      }
      case 'd':
      {
        jerry_log_string (jerry_format_int (va_arg (vl, int), width, padding, buffer_p));
        break;
      }
      case 'u':
      {
        jerry_log_string (jerry_format_unsigned (va_arg (vl, unsigned int), width, padding, 10, buffer_p));
        break;
      }
      case 'x':
      {
        jerry_log_string (jerry_format_unsigned (va_arg (vl, unsigned int), width, padding, 16, buffer_p));
        break;
      }
      default:
      {
        buffer_p[buffer_index++] = '%';
        break;
      }
    }
  }

  if (buffer_index > 0)
  {
    buffer_p[buffer_index] = '\0';
    jerry_log_string (buffer_p);
  }

  va_end (vl);
} /* jerry_log */

/**
 * Allocate memory on the engine's heap.
 *
 * Note:
 *      This function may take away memory from the executed JavaScript code.
 *      If any other dynamic memory allocation API is available (e.g., libc
 *      malloc), it should be used instead.
 *
 * @return allocated memory on success
 *         NULL otherwise
 */
void *
jerry_heap_alloc (jerry_size_t size) /**< size of the memory block */
{
  jerry_assert_api_enabled ();

  return jmem_heap_alloc_block_null_on_error (size);
} /* jerry_heap_alloc */

/**
 * Free memory allocated on the engine's heap.
 */
void
jerry_heap_free (void *mem_p, /**< value returned by jerry_heap_alloc */
                 jerry_size_t size) /**< same size as passed to jerry_heap_alloc */
{
  jerry_assert_api_enabled ();

  jmem_heap_free_block (mem_p, size);
} /* jerry_heap_free */

/**
 * When JERRY_VM_HALT is enabled, the callback passed to this function
 * is periodically called with the user_p argument. If interval is greater
 * than 1, the callback is only called at every interval ticks.
 */
void
jerry_halt_handler (uint32_t interval, /**< interval of the function call */
                    jerry_halt_cb_t callback, /**< periodically called user function */
                    void *user_p) /**< pointer passed to the function */
{
#if JERRY_VM_HALT
  if (interval == 0)
  {
    interval = 1;
  }

  JERRY_CONTEXT (vm_exec_stop_frequency) = interval;
  JERRY_CONTEXT (vm_exec_stop_counter) = interval;
  JERRY_CONTEXT (vm_exec_stop_cb) = callback;
  JERRY_CONTEXT (vm_exec_stop_user_p) = user_p;
#else /* !JERRY_VM_HALT */
  JERRY_UNUSED (interval);
  JERRY_UNUSED (callback);
  JERRY_UNUSED (user_p);
#endif /* JERRY_VM_HALT */
} /* jerry_halt_handler */

/**
 * Get backtrace. The backtrace is an array of strings where
 * each string contains the position of the corresponding frame.
 * The array length is zero if the backtrace is not available.
 *
 * @return array value
 */
jerry_value_t
jerry_backtrace (uint32_t max_depth) /**< depth limit of the backtrace */
{
  return vm_get_backtrace (max_depth);
} /* jerry_backtrace */

/**
 * Low-level function to capture each backtrace frame.
 * The captured frame data is passed to a callback function.
 */
void
jerry_backtrace_capture (jerry_backtrace_cb_t callback, /**< callback function */
                         void *user_p) /**< user pointer passed to the callback function */
{
  jerry_frame_t frame;
  vm_frame_ctx_t *context_p = JERRY_CONTEXT (vm_top_context_p);

  while (context_p != NULL)
  {
    frame.context_p = context_p;
    frame.frame_type = JERRY_BACKTRACE_FRAME_JS;

    if (!callback (&frame, user_p))
    {
      return;
    }

    context_p = context_p->prev_context_p;
  }
} /* jerry_backtrace */

/**
 * Returns with the type of the backtrace frame.
 *
 * @return frame type listed in jerry_frame_type_t
 */
jerry_frame_type_t
jerry_frame_type (const jerry_frame_t *frame_p) /**< frame pointer */
{
  return (jerry_frame_type_t) frame_p->frame_type;
} /* jerry_frame_type */

/**
 * Initialize and return with the location private field of a backtrace frame.
 *
 * @return pointer to the location private field - if the location is available,
 *         NULL - otherwise
 */
const jerry_frame_location_t *
jerry_frame_location (jerry_frame_t *frame_p) /**< frame pointer */
{
  JERRY_UNUSED (frame_p);

#if JERRY_LINE_INFO
  if (frame_p->frame_type == JERRY_BACKTRACE_FRAME_JS)
  {
    vm_frame_ctx_t *context_p = frame_p->context_p;
    const ecma_compiled_code_t *bytecode_header_p = context_p->shared_p->bytecode_header_p;

    if (!(bytecode_header_p->status_flags & CBC_CODE_FLAGS_HAS_LINE_INFO))
    {
      return NULL;
    }

    frame_p->location.source_name = ecma_get_source_name (bytecode_header_p);

    ecma_line_info_get (ecma_compiled_code_get_line_info (bytecode_header_p),
                        (uint32_t) (context_p->byte_code_p - context_p->byte_code_start_p),
                        &frame_p->location);

    return &frame_p->location;
  }
#endif /* JERRY_LINE_INFO */

  return NULL;
} /* jerry_frame_location */

/**
 * Initialize and return with the called function private field of a backtrace frame.
 * The backtrace frame is created for running the code bound to this function.
 *
 * @return pointer to the called function - if the function is available,
 *         NULL - otherwise
 */
const jerry_value_t *
jerry_frame_callee (jerry_frame_t *frame_p) /**< frame pointer */
{
  if (frame_p->frame_type == JERRY_BACKTRACE_FRAME_JS)
  {
    vm_frame_ctx_t *context_p = frame_p->context_p;

    if (context_p->shared_p->function_object_p != NULL)
    {
      frame_p->function = ecma_make_object_value (context_p->shared_p->function_object_p);
      return &frame_p->function;
    }
  }

  return NULL;
} /* jerry_frame_callee */

/**
 * Initialize and return with the 'this' binding private field of a backtrace frame.
 * The 'this' binding is a hidden value passed to the called function. As for arrow
 * functions, the 'this' binding is assigned at function creation.
 *
 * @return pointer to the 'this' binding - if the binding is available,
 *         NULL - otherwise
 */
const jerry_value_t *
jerry_frame_this (jerry_frame_t *frame_p) /**< frame pointer */
{
  if (frame_p->frame_type == JERRY_BACKTRACE_FRAME_JS)
  {
    frame_p->this_binding = frame_p->context_p->this_binding;
    return &frame_p->this_binding;
  }

  return NULL;
} /* jerry_frame_this */

/**
 * Returns true, if the code bound to the backtrace frame is strict mode code.
 *
 * @return true - if strict mode code is bound to the frame,
 *         false - otherwise
 */
bool
jerry_frame_is_strict (jerry_frame_t *frame_p) /**< frame pointer */
{
  return (frame_p->frame_type == JERRY_BACKTRACE_FRAME_JS
          && (frame_p->context_p->status_flags & VM_FRAME_CTX_IS_STRICT) != 0);
} /* jerry_frame_is_strict */

/**
 * Get the source name (usually a file name) of the currently executed script or the given function object
 *
 * Note: returned value must be freed with jerry_value_free, when it is no longer needed
 *
 * @return JS string constructed from
 *         - the currently executed function object's source name, if the given value is undefined
 *         - source name of the function object, if the given value is a function object
 *         - "<anonymous>", otherwise
 */
jerry_value_t
jerry_source_name (const jerry_value_t value) /**< jerry api value */
{
#if JERRY_SOURCE_NAME
  if (ecma_is_value_undefined (value) && JERRY_CONTEXT (vm_top_context_p) != NULL)
  {
    return ecma_copy_value (ecma_get_source_name (JERRY_CONTEXT (vm_top_context_p)->shared_p->bytecode_header_p));
  }

  ecma_value_t script_value = ecma_script_get_from_value (value);

  if (script_value == JMEM_CP_NULL)
  {
    return ecma_make_magic_string_value (LIT_MAGIC_STRING_SOURCE_NAME_ANON);
  }

  const cbc_script_t *script_p = ECMA_GET_INTERNAL_VALUE_POINTER (cbc_script_t, script_value);

  return ecma_copy_value (script_p->source_name);
#else /* !JERRY_SOURCE_NAME */
  JERRY_UNUSED (value);
  return ecma_make_magic_string_value (LIT_MAGIC_STRING_SOURCE_NAME_ANON);
#endif /* JERRY_SOURCE_NAME */
} /* jerry_source_name */

/**
 * Returns the user value assigned to a script / module / function.
 *
 * Note:
 *    This value is usually set by the parser when
 *    the JERRY_PARSE_HAS_USER_VALUE flag is passed.
 *
 * @return user value
 */
jerry_value_t
jerry_source_user_value (const jerry_value_t value) /**< jerry api value */
{
  ecma_value_t script_value = ecma_script_get_from_value (value);

  if (script_value == JMEM_CP_NULL)
  {
    return ECMA_VALUE_UNDEFINED;
  }

  const cbc_script_t *script_p = ECMA_GET_INTERNAL_VALUE_POINTER (cbc_script_t, script_value);

  if (!(script_p->refs_and_type & CBC_SCRIPT_HAS_USER_VALUE))
  {
    return ECMA_VALUE_UNDEFINED;
  }

  return ecma_copy_value (CBC_SCRIPT_GET_USER_VALUE (script_p));
} /* jerry_source_user_value */

/**
 * Checks whether an ECMAScript code is compiled by eval
 * like (eval, new Function, jerry_eval, etc.) command.
 *
 * @return true, if code is compiled by eval like command
 *         false, otherwise
 */
bool
jerry_function_is_dynamic (const jerry_value_t value) /**< jerry api value */
{
  ecma_value_t script_value = ecma_script_get_from_value (value);

  if (script_value == JMEM_CP_NULL)
  {
    return false;
  }

  const cbc_script_t *script_p = ECMA_GET_INTERNAL_VALUE_POINTER (cbc_script_t, script_value);

  return (script_p->refs_and_type & CBC_SCRIPT_IS_EVAL_CODE) != 0;
} /* jerry_function_is_dynamic */

/**
 * Returns a newly created source info structure corresponding to the passed script/module/function.
 *
 * @return a newly created source info, if at least one field is available, NULL otherwise
 */
jerry_source_info_t *
jerry_source_info (const jerry_value_t value) /**< jerry api value */
{
  jerry_assert_api_enabled ();

#if JERRY_FUNCTION_TO_STRING
  if (!ecma_is_value_object (value))
  {
    return NULL;
  }

  jerry_source_info_t source_info;

  source_info.enabled_fields = 0;
  source_info.source_code = ECMA_VALUE_UNDEFINED;
  source_info.function_arguments = ECMA_VALUE_UNDEFINED;
  source_info.source_range_start = 0;
  source_info.source_range_length = 0;

  ecma_object_t *object_p = ecma_get_object_from_value (value);
  cbc_script_t *script_p = NULL;

  while (true)
  {
    switch (ecma_get_object_type (object_p))
    {
      case ECMA_OBJECT_TYPE_CLASS:
      {
        ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
        const ecma_compiled_code_t *bytecode_p = NULL;

        if (ext_object_p->u.cls.type == ECMA_OBJECT_CLASS_SCRIPT)
        {
          bytecode_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_compiled_code_t, ext_object_p->u.cls.u3.value);
        }
#if JERRY_MODULE_SYSTEM
        else if (ext_object_p->u.cls.type == ECMA_OBJECT_CLASS_MODULE)
        {
          ecma_module_t *module_p = (ecma_module_t *) object_p;

          if (!(module_p->header.u.cls.u2.module_flags & ECMA_MODULE_IS_NATIVE))
          {
            bytecode_p = module_p->u.compiled_code_p;
          }
        }
#endif /* JERRY_MODULE_SYSTEM */

        if (bytecode_p == NULL)
        {
          return NULL;
        }

        ecma_value_t script_value = ((cbc_uint8_arguments_t *) bytecode_p)->script_value;
        script_p = ECMA_GET_INTERNAL_VALUE_POINTER (cbc_script_t, script_value);
        break;
      }
      case ECMA_OBJECT_TYPE_FUNCTION:
      {
        const ecma_compiled_code_t *bytecode_p;
        bytecode_p = ecma_op_function_get_compiled_code ((ecma_extended_object_t *) object_p);

        ecma_value_t script_value = ((cbc_uint8_arguments_t *) bytecode_p)->script_value;
        script_p = ECMA_GET_INTERNAL_VALUE_POINTER (cbc_script_t, script_value);

        if (bytecode_p->status_flags & CBC_CODE_FLAGS_HAS_EXTENDED_INFO)
        {
          uint8_t *extended_info_p = ecma_compiled_code_resolve_extended_info (bytecode_p);
          uint8_t extended_info = *extended_info_p;

          if (extended_info & CBC_EXTENDED_CODE_FLAGS_HAS_ARGUMENT_LENGTH)
          {
            ecma_extended_info_decode_vlq (&extended_info_p);
          }

          if (extended_info & CBC_EXTENDED_CODE_FLAGS_SOURCE_CODE_IN_ARGUMENTS)
          {
            ecma_value_t function_arguments = CBC_SCRIPT_GET_FUNCTION_ARGUMENTS (script_p, script_p->refs_and_type);

            ecma_ref_ecma_string (ecma_get_string_from_value (function_arguments));

            source_info.enabled_fields |= JERRY_SOURCE_INFO_HAS_SOURCE_CODE;
            source_info.source_code = function_arguments;
            script_p = NULL;
          }

          source_info.enabled_fields |= JERRY_SOURCE_INFO_HAS_SOURCE_RANGE;
          source_info.source_range_start = ecma_extended_info_decode_vlq (&extended_info_p);
          source_info.source_range_length = ecma_extended_info_decode_vlq (&extended_info_p);
        }

        JERRY_ASSERT (script_p != NULL || (source_info.enabled_fields & JERRY_SOURCE_INFO_HAS_SOURCE_CODE));

        if (source_info.enabled_fields == 0 && (script_p->refs_and_type & CBC_SCRIPT_HAS_FUNCTION_ARGUMENTS))
        {
          ecma_value_t function_arguments = CBC_SCRIPT_GET_FUNCTION_ARGUMENTS (script_p, script_p->refs_and_type);

          ecma_ref_ecma_string (ecma_get_string_from_value (function_arguments));

          source_info.enabled_fields |= JERRY_SOURCE_INFO_HAS_FUNCTION_ARGUMENTS;
          source_info.function_arguments = function_arguments;
        }
        break;
      }
      case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
      {
        ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

        object_p =
          ECMA_GET_NON_NULL_POINTER_FROM_POINTER_TAG (ecma_object_t, ext_object_p->u.bound_function.target_function);
        continue;
      }
      case ECMA_OBJECT_TYPE_CONSTRUCTOR_FUNCTION:
      {
        ecma_value_t script_value = ((ecma_extended_object_t *) object_p)->u.constructor_function.script_value;
        script_p = ECMA_GET_INTERNAL_VALUE_POINTER (cbc_script_t, script_value);
        break;
      }
      default:
      {
        return NULL;
      }
    }

    break;
  }

  jerry_source_info_t *source_info_p = jmem_heap_alloc_block_null_on_error (sizeof (jerry_source_info_t));

  if (source_info_p == NULL)
  {
    return NULL;
  }

  if (script_p != NULL)
  {
    ecma_ref_ecma_string (ecma_get_string_from_value (script_p->source_code));

    source_info.enabled_fields |= JERRY_SOURCE_INFO_HAS_SOURCE_CODE;
    source_info.source_code = script_p->source_code;
  }

  JERRY_ASSERT (source_info.enabled_fields != 0);

  *source_info_p = source_info;
  return source_info_p;
#else /* !JERRY_FUNCTION_TO_STRING */
  JERRY_UNUSED (value);
  return NULL;
#endif /* JERRY_FUNCTION_TO_STRING */
} /* jerry_source_info */

/**
 * Frees the the source info structure returned by jerry_source_info.
 */
void
jerry_source_info_free (jerry_source_info_t *source_info_p) /**< source info block */
{
  jerry_assert_api_enabled ();

#if JERRY_FUNCTION_TO_STRING
  if (source_info_p != NULL)
  {
    ecma_free_value (source_info_p->source_code);
    ecma_free_value (source_info_p->function_arguments);
    jmem_heap_free_block (source_info_p, sizeof (jerry_source_info_t));
  }
#else /* !JERRY_FUNCTION_TO_STRING */
  JERRY_UNUSED (source_info_p);
#endif /* JERRY_FUNCTION_TO_STRING */
} /* jerry_source_info_free */

/**
 * Replaces the currently active realm with another realm.
 *
 * The replacement should be temporary, and the original realm must be
 * restored after the tasks are completed. During the replacement, the
 * realm must be referenced by the application (i.e. the gc must not
 * reclaim it). This is also true to the returned previously active
 * realm, so there is no need to free the value after the restoration.
 *
 * @return previous realm value - if the passed value is a realm
 *         exception - otherwise
 */
jerry_value_t
jerry_set_realm (jerry_value_t realm_value) /**< jerry api value */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_REALMS
  if (ecma_is_value_object (realm_value))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (realm_value);

    if (ecma_builtin_is_global (object_p))
    {
      ecma_global_object_t *previous_global_object_p = JERRY_CONTEXT (global_object_p);
      JERRY_CONTEXT (global_object_p) = (ecma_global_object_t *) object_p;
      return ecma_make_object_value ((ecma_object_t *) previous_global_object_p);
    }
  }

  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_PASSED_ARGUMENT_IS_NOT_A_REALM));
#else /* !JERRY_BUILTIN_REALMS */
  JERRY_UNUSED (realm_value);
  return jerry_throw_sz (JERRY_ERROR_REFERENCE, ecma_get_error_msg (ECMA_ERR_REALM_IS_NOT_AVAILABLE));
#endif /* JERRY_BUILTIN_REALMS */
} /* jerry_set_realm */

/**
 * Gets the 'this' binding of a realm
 *
 * @return type error - if realm_value is not a realm
 *         this value - otherwise
 */
jerry_value_t
jerry_realm_this (jerry_value_t realm) /**< realm value */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_REALMS
  if (ecma_is_value_object (realm))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (realm);

    if (ecma_builtin_is_global (object_p))
    {
      ecma_global_object_t *global_object_p = (ecma_global_object_t *) object_p;

      ecma_ref_object (ecma_get_object_from_value (global_object_p->this_binding));
      return global_object_p->this_binding;
    }
  }

#else /* !JERRY_BUILTIN_REALMS */
  ecma_object_t *global_object_p = ecma_builtin_get_global ();

  if (realm == ecma_make_object_value (global_object_p))
  {
    ecma_ref_object (global_object_p);
    return realm;
  }
#endif /* JERRY_BUILTIN_REALMS */

  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_PASSED_ARGUMENT_IS_NOT_A_REALM));
} /* jerry_realm_this */

/**
 * Sets the 'this' binding of a realm
 *
 * This function must be called before executing any script on the realm.
 * Otherwise the operation is undefined.
 *
 * @return type error - if realm_value is not a realm or this_value is not object
 *         true - otherwise
 */
jerry_value_t
jerry_realm_set_this (jerry_value_t realm, /**< realm value */
                      jerry_value_t this_value) /**< this value */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_REALMS
  if (!ecma_is_value_object (this_value))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_SECOND_ARGUMENT_MUST_BE_AN_OBJECT));
  }

  if (ecma_is_value_object (realm))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (realm);

    if (ecma_builtin_is_global (object_p))
    {
      ecma_global_object_t *global_object_p = (ecma_global_object_t *) object_p;
      global_object_p->this_binding = this_value;

      ecma_object_t *global_lex_env_p = ecma_create_object_lex_env (NULL, ecma_get_object_from_value (this_value));

      ECMA_SET_NON_NULL_POINTER (global_object_p->global_env_cp, global_lex_env_p);
      global_object_p->global_scope_cp = global_object_p->global_env_cp;

      ecma_deref_object (global_lex_env_p);
      return ECMA_VALUE_TRUE;
    }
  }

  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_FIRST_ARGUMENT_IS_NOT_A_REALM));
#else /* !JERRY_BUILTIN_REALMS */
  JERRY_UNUSED (realm);
  JERRY_UNUSED (this_value);
  return jerry_throw_sz (JERRY_ERROR_REFERENCE, ecma_get_error_msg (ECMA_ERR_REALM_IS_NOT_AVAILABLE));
#endif /* JERRY_BUILTIN_REALMS */
} /* jerry_realm_set_this */

/**
 * Check if the given value is an ArrayBuffer object.
 *
 * @return true - if it is an ArrayBuffer object
 *         false - otherwise
 */
bool
jerry_value_is_arraybuffer (const jerry_value_t value) /**< value to check if it is an ArrayBuffer */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_TYPEDARRAY
  return ecma_is_arraybuffer (value);
#else /* !JERRY_BUILTIN_TYPEDARRAY */
  JERRY_UNUSED (value);
  return false;
#endif /* JERRY_BUILTIN_TYPEDARRAY */
} /* jerry_value_is_arraybuffer */

/**
 * Creates an ArrayBuffer object with the given length (size).
 *
 * Notes:
 *      * the length is specified in bytes.
 *      * returned value must be freed with jerry_value_free, when it is no longer needed.
 *      * if the typed arrays are disabled this will return a TypeError.
 *
 * @return value of the constructed ArrayBuffer object
 */
jerry_value_t
jerry_arraybuffer (const jerry_length_t size) /**< size of the backing store allocated
                                               *   for the array buffer in bytes */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_TYPEDARRAY
  return jerry_return (ecma_make_object_value (ecma_arraybuffer_new_object (size)));
#else /* !JERRY_BUILTIN_TYPEDARRAY */
  JERRY_UNUSED (size);
  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_TYPED_ARRAY_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_TYPEDARRAY */
} /* jerry_arraybuffer */

/**
 * Creates an ArrayBuffer object with user specified buffer.
 *
 * Notes:
 *     * the size is specified in bytes.
 *     * the buffer passed should be at least the specified bytes big.
 *     * if the typed arrays are disabled this will return a TypeError.
 *     * if the size is zero or buffer_p is a null pointer this will return an empty ArrayBuffer.
 *
 * @return value of the newly constructed array buffer object
 */
jerry_value_t
jerry_arraybuffer_external (uint8_t *buffer_p, /**< the backing store used by the array buffer object */
                            jerry_length_t size, /**< size of the buffer in bytes */
                            void *user_p) /**< user pointer assigned to the array buffer object */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_TYPEDARRAY
  ecma_object_t *arraybuffer_p;

  if (JERRY_UNLIKELY (size == 0))
  {
    arraybuffer_p = ecma_arraybuffer_new_object (0);
  }
  else
  {
    arraybuffer_p = ecma_arraybuffer_create_object_with_buffer (ECMA_OBJECT_CLASS_ARRAY_BUFFER, size);

    ecma_arraybuffer_pointer_t *arraybuffer_pointer_p = (ecma_arraybuffer_pointer_t *) arraybuffer_p;
    arraybuffer_pointer_p->arraybuffer_user_p = user_p;

    if (buffer_p != NULL)
    {
      arraybuffer_pointer_p->extended_object.u.cls.u1.array_buffer_flags |= ECMA_ARRAYBUFFER_ALLOCATED;
      arraybuffer_pointer_p->buffer_p = buffer_p;
    }
  }

  return jerry_return (ecma_make_object_value (arraybuffer_p));
#else /* !JERRY_BUILTIN_TYPEDARRAY */
  JERRY_UNUSED (size);
  JERRY_UNUSED (buffer_p);
  JERRY_UNUSED (user_p);
  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_TYPED_ARRAY_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_TYPEDARRAY */
} /* jerry_arraybuffer_external */

/**
 * Check if the given value is a SharedArrayBuffer object.
 *
 * @return true - if it is a SharedArrayBuffer object
 *         false - otherwise
 */
bool
jerry_value_is_shared_arraybuffer (const jerry_value_t value) /**< value to check if it is a SharedArrayBuffer */
{
  jerry_assert_api_enabled ();

  return ecma_is_shared_arraybuffer (value);
} /* jerry_value_is_shared_arraybuffer */

/**
 * Creates a SharedArrayBuffer object with the given length (size).
 *
 * Notes:
 *      * the length is specified in bytes.
 *      * returned value must be freed with jerry_value_free, when it is no longer needed.
 *      * if the typed arrays are disabled this will return a TypeError.
 *
 * @return value of the constructed SharedArrayBuffer object
 */
jerry_value_t
jerry_shared_arraybuffer (jerry_length_t size) /**< size of the backing store allocated
                                                *   for the shared array buffer in bytes */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_SHAREDARRAYBUFFER
  return jerry_return (ecma_make_object_value (ecma_shared_arraybuffer_new_object (size)));
#else /* !JERRY_BUILTIN_SHAREDARRAYBUFFER */
  JERRY_UNUSED (size);
  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_SHARED_ARRAYBUFFER_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_SHAREDARRAYBUFFER */
} /* jerry_shared_arraybuffer */

/**
 * Creates a SharedArrayBuffer object with user specified buffer.
 *
 * Notes:
 *     * the size is specified in bytes.
 *     * the buffer passed should be at least the specified bytes big.
 *     * if the typed arrays are disabled this will return a TypeError.
 *     * if the size is zero or buffer_p is a null pointer this will return an empty SharedArrayBuffer.
 *
 * @return value of the newly constructed shared array buffer object
 */
jerry_value_t
jerry_shared_arraybuffer_external (uint8_t *buffer_p, /**< the backing store used by the
                                                       *   shared array buffer object */
                                   jerry_length_t size, /**< size of the buffer in bytes */
                                   void *user_p) /**< user pointer assigned to the
                                                  *   shared array buffer object */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_SHAREDARRAYBUFFER
  ecma_object_t *shared_arraybuffer_p;

  if (JERRY_UNLIKELY (size == 0))
  {
    shared_arraybuffer_p = ecma_shared_arraybuffer_new_object (0);
  }
  else
  {
    shared_arraybuffer_p = ecma_arraybuffer_create_object_with_buffer (ECMA_OBJECT_CLASS_SHARED_ARRAY_BUFFER, size);

    ecma_arraybuffer_pointer_t *shared_arraybuffer_pointer_p = (ecma_arraybuffer_pointer_t *) shared_arraybuffer_p;
    shared_arraybuffer_pointer_p->arraybuffer_user_p = user_p;

    if (buffer_p != NULL)
    {
      shared_arraybuffer_pointer_p->extended_object.u.cls.u1.array_buffer_flags |= ECMA_ARRAYBUFFER_ALLOCATED;
      shared_arraybuffer_pointer_p->buffer_p = buffer_p;
    }
  }

  return ecma_make_object_value (shared_arraybuffer_p);
#else /* !JERRY_BUILTIN_SHAREDARRAYBUFFER */
  JERRY_UNUSED (size);
  JERRY_UNUSED (buffer_p);
  JERRY_UNUSED (user_p);
  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_SHARED_ARRAYBUFFER_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_SHAREDARRAYBUFFER */
} /* jerry_shared_arraybuffer_external */

#if JERRY_BUILTIN_TYPEDARRAY

/**
 * Allocate a backing store for an array buffer, ignores allocation fails.
 *
 * @return true on success,
 *         false otherwise
 */
static bool
jerry_arraybuffer_allocate_buffer_no_throw (ecma_object_t *arraybuffer_p) /**< ArrayBuffer object */
{
  JERRY_ASSERT (!(ECMA_ARRAYBUFFER_GET_FLAGS (arraybuffer_p) & ECMA_ARRAYBUFFER_ALLOCATED));

  if (ECMA_ARRAYBUFFER_GET_FLAGS (arraybuffer_p) & ECMA_ARRAYBUFFER_DETACHED)
  {
    return false;
  }

  return ecma_arraybuffer_allocate_buffer (arraybuffer_p) != ECMA_VALUE_ERROR;
} /* jerry_arraybuffer_allocate_buffer_no_throw */

#endif /* JERRY_BUILTIN_TYPEDARRAY */

/**
 * Copy bytes into the ArrayBuffer or SharedArrayBuffer from a buffer.
 *
 * Note:
 *     * returns 0, if the passed object is not an ArrayBuffer or SharedArrayBuffer
 *
 * @return number of bytes copied into the ArrayBuffer or SharedArrayBuffer.
 */
jerry_length_t
jerry_arraybuffer_write (jerry_value_t value, /**< target ArrayBuffer or SharedArrayBuffer */
                         jerry_length_t offset, /**< start offset of the ArrayBuffer */
                         const uint8_t *buf_p, /**< buffer to copy from */
                         jerry_length_t buf_size) /**< number of bytes to copy from the buffer */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_TYPEDARRAY
  if (!(ecma_is_arraybuffer (value) || ecma_is_shared_arraybuffer (value)))
  {
    return 0;
  }

  ecma_object_t *arraybuffer_p = ecma_get_object_from_value (value);

  if (!(ECMA_ARRAYBUFFER_GET_FLAGS (arraybuffer_p) & ECMA_ARRAYBUFFER_ALLOCATED)
      && !jerry_arraybuffer_allocate_buffer_no_throw (arraybuffer_p))
  {
    return 0;
  }

  jerry_length_t length = ecma_arraybuffer_get_length (arraybuffer_p);

  if (offset >= length)
  {
    return 0;
  }

  jerry_length_t copy_count = JERRY_MIN (length - offset, buf_size);

  if (copy_count > 0)
  {
    lit_utf8_byte_t *buffer_p = ecma_arraybuffer_get_buffer (arraybuffer_p);

    memcpy ((void *) (buffer_p + offset), (void *) buf_p, copy_count);
  }

  return copy_count;
#else /* !JERRY_BUILTIN_TYPEDARRAY */
  JERRY_UNUSED (value);
  JERRY_UNUSED (offset);
  JERRY_UNUSED (buf_p);
  JERRY_UNUSED (buf_size);
  return 0;
#endif /* JERRY_BUILTIN_TYPEDARRAY */
} /* jerry_arraybuffer_write */

/**
 * Copy bytes from a buffer into an ArrayBuffer or SharedArrayBuffer.
 *
 * Note:
 *     * if the object passed is not an ArrayBuffer or SharedArrayBuffer will return 0.
 *
 * @return number of bytes read from the ArrayBuffer.
 */
jerry_length_t
jerry_arraybuffer_read (const jerry_value_t value, /**< ArrayBuffer or SharedArrayBuffer to read from */
                        jerry_length_t offset, /**< start offset of the ArrayBuffer or SharedArrayBuffer */
                        uint8_t *buf_p, /**< destination buffer to copy to */
                        jerry_length_t buf_size) /**< number of bytes to copy into the buffer */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_TYPEDARRAY
  if (!(ecma_is_arraybuffer (value) || ecma_is_shared_arraybuffer (value)))
  {
    return 0;
  }

  ecma_object_t *arraybuffer_p = ecma_get_object_from_value (value);

  if (!(ECMA_ARRAYBUFFER_GET_FLAGS (arraybuffer_p) & ECMA_ARRAYBUFFER_ALLOCATED)
      && !jerry_arraybuffer_allocate_buffer_no_throw (arraybuffer_p))
  {
    return 0;
  }

  jerry_length_t length = ecma_arraybuffer_get_length (arraybuffer_p);

  if (offset >= length)
  {
    return 0;
  }

  jerry_length_t copy_count = JERRY_MIN (length - offset, buf_size);

  if (copy_count > 0)
  {
    lit_utf8_byte_t *buffer_p = ecma_arraybuffer_get_buffer (arraybuffer_p);

    memcpy ((void *) buf_p, (void *) (buffer_p + offset), copy_count);
  }

  return copy_count;
#else /* !JERRY_BUILTIN_TYPEDARRAY */
  JERRY_UNUSED (value);
  JERRY_UNUSED (offset);
  JERRY_UNUSED (buf_p);
  JERRY_UNUSED (buf_size);
  return 0;
#endif /* JERRY_BUILTIN_TYPEDARRAY */
} /* jerry_arraybuffer_read */

/**
 * Get the length (size) of the ArrayBuffer or SharedArrayBuffer in bytes.
 *
 * Note:
 *     This is the 'byteLength' property of an ArrayBuffer or SharedArrayBuffer.
 *
 * @return the length of the ArrayBuffer in bytes.
 */
jerry_length_t
jerry_arraybuffer_size (const jerry_value_t value) /**< ArrayBuffer or SharedArrayBuffer */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_TYPEDARRAY
  if (ecma_is_arraybuffer (value) || ecma_is_shared_arraybuffer (value))
  {
    ecma_object_t *arraybuffer_p = ecma_get_object_from_value (value);
    return ecma_arraybuffer_get_length (arraybuffer_p);
  }
#else /* !JERRY_BUILTIN_TYPEDARRAY */
  JERRY_UNUSED (value);
#endif /* JERRY_BUILTIN_TYPEDARRAY */
  return 0;
} /* jerry_arraybuffer_size */

/**
 * Get a pointer for the start of the ArrayBuffer.
 *
 * Note:
 *    * This is a high-risk operation as the bounds are not checked
 *      when accessing the pointer elements.
 *
 * @return pointer to the back-buffer of the ArrayBuffer.
 *         pointer is NULL if:
 *            - the parameter is not an ArrayBuffer
 *            - an external ArrayBuffer has been detached
 */
uint8_t *
jerry_arraybuffer_data (const jerry_value_t array_buffer) /**< Array Buffer to use */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_TYPEDARRAY
  if (!(ecma_is_arraybuffer (array_buffer) || ecma_is_shared_arraybuffer (array_buffer)))
  {
    return NULL;
  }

  ecma_object_t *arraybuffer_p = ecma_get_object_from_value (array_buffer);

  if (!(ECMA_ARRAYBUFFER_GET_FLAGS (arraybuffer_p) & ECMA_ARRAYBUFFER_ALLOCATED)
      && !jerry_arraybuffer_allocate_buffer_no_throw (arraybuffer_p))
  {
    return NULL;
  }

  return (uint8_t *) ecma_arraybuffer_get_buffer (arraybuffer_p);
#else /* !JERRY_BUILTIN_TYPEDARRAY */
  JERRY_UNUSED (array_buffer);
#endif /* JERRY_BUILTIN_TYPEDARRAY */

  return NULL;
} /* jerry_arraybuffer_data */

/**
 * Get if the ArrayBuffer is detachable.
 *
 * @return boolean value - if success
 *         value marked with error flag - otherwise
 */
bool
jerry_arraybuffer_is_detachable (const jerry_value_t value) /**< ArrayBuffer */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_TYPEDARRAY
  if (ecma_is_arraybuffer (value))
  {
    ecma_object_t *buffer_p = ecma_get_object_from_value (value);
    return !ecma_arraybuffer_is_detached (buffer_p);
  }
#else /* !JERRY_BUILTIN_TYPEDARRAY */
  JERRY_UNUSED (value);
#endif /* JERRY_BUILTIN_TYPEDARRAY */
  return false;
} /* jerry_arraybuffer_is_detachable */

/**
 * Detach the underlying data block from ArrayBuffer and set its bytelength to 0.
 *
 * Note: if the ArrayBuffer has a separate data buffer, the free callback set by
 *       jerry_arraybuffer_set_allocation_callbacks is called for this buffer
 *
 * @return null value - if success
 *         value marked with error flag - otherwise
 */
jerry_value_t
jerry_arraybuffer_detach (jerry_value_t value) /**< ArrayBuffer */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_TYPEDARRAY
  if (ecma_is_arraybuffer (value))
  {
    ecma_object_t *buffer_p = ecma_get_object_from_value (value);
    if (ecma_arraybuffer_detach (buffer_p))
    {
      return ECMA_VALUE_NULL;
    }
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ARRAY_BUFFER_DETACHED));
  }
#else /* !JERRY_BUILTIN_TYPEDARRAY */
  JERRY_UNUSED (value);
#endif /* JERRY_BUILTIN_TYPEDARRAY */
  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_EXPECTED_AN_ARRAYBUFFER));
} /* jerry_arraybuffer_detach */

/**
 * Checks whether a buffer is currently allocated for an array buffer or typed array.
 *
 * @return true, if a buffer is allocated for an array buffer or typed array
 *         false, otherwise
 */
bool
jerry_arraybuffer_has_buffer (const jerry_value_t value) /**< array buffer or typed array value */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_TYPEDARRAY
  if (!ecma_is_value_object (value))
  {
    return false;
  }

  ecma_object_t *object_p = ecma_get_object_from_value (value);

  if (ecma_object_is_typedarray (object_p))
  {
    object_p = ecma_typedarray_get_arraybuffer (object_p);
  }
  else if (!(ecma_object_class_is (object_p, ECMA_OBJECT_CLASS_ARRAY_BUFFER)
             || ecma_object_is_shared_arraybuffer (object_p)))
  {
    return false;
  }

  return (ECMA_ARRAYBUFFER_GET_FLAGS (object_p) & ECMA_ARRAYBUFFER_ALLOCATED) != 0;
#else /* !JERRY_BUILTIN_TYPEDARRAY */
  JERRY_UNUSED (value);
  return false;
#endif /* JERRY_BUILTIN_TYPEDARRAY */
} /* jerry_arraybuffer_has_buffer */

/**
 * Array buffers which size is less or equal than the limit passed to this function are allocated in
 * a single memory block. The allocator callbacks set by jerry_arraybuffer_set_allocation_callbacks
 * are not called for these array buffers. The default limit is 256 bytes.
 */
void
jerry_arraybuffer_heap_allocation_limit (jerry_length_t allocation_limit) /**< maximum size of
                                                                           *   compact allocation */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_TYPEDARRAY
  JERRY_CONTEXT (arraybuffer_compact_allocation_limit) = allocation_limit;
#else /* !JERRY_BUILTIN_TYPEDARRAY */
  JERRY_UNUSED (allocation_limit);
#endif /* JERRY_BUILTIN_TYPEDARRAY */
} /* jerry_arraybuffer_heap_allocation_limit */

/**
 * Set callbacks for allocating and freeing backing stores for array buffer objects.
 */
void
jerry_arraybuffer_allocator (jerry_arraybuffer_allocate_cb_t allocate_callback, /**< callback for allocating
                                                                                 *   array buffer memory */
                             jerry_arraybuffer_free_cb_t free_callback, /**< callback for freeing
                                                                         *   array buffer memory */
                             void *user_p) /**< user pointer passed to the callbacks */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_TYPEDARRAY
  JERRY_CONTEXT (arraybuffer_allocate_callback) = allocate_callback;
  JERRY_CONTEXT (arraybuffer_free_callback) = free_callback;
  JERRY_CONTEXT (arraybuffer_allocate_callback_user_p) = user_p;
#else /* !JERRY_BUILTIN_TYPEDARRAY */
  JERRY_UNUSED (allocate_callback);
  JERRY_UNUSED (free_callback);
  JERRY_UNUSED (user_p);
#endif /* JERRY_BUILTIN_TYPEDARRAY */
} /* jerry_arraybuffer_allocator */

/**
 * DataView related functions
 */

/**
 * Creates a DataView object with the given ArrayBuffer, ByteOffset and ByteLength arguments.
 *
 * Notes:
 *      * returned value must be freed with jerry_value_free, when it is no longer needed.
 *      * if the DataView bulitin is disabled this will return a TypeError.
 *
 * @return value of the constructed DataView object - if success
 *         created error - otherwise
 */
jerry_value_t
jerry_dataview (const jerry_value_t array_buffer, /**< arraybuffer to create DataView from */
                jerry_length_t byte_offset, /**< offset in bytes, to the first byte in the buffer */
                jerry_length_t byte_length) /**< number of elements in the byte array */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_DATAVIEW
  if (ecma_is_value_exception (array_buffer))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_value_t arguments_p[3] = { array_buffer,
                                  ecma_make_uint32_value (byte_offset),
                                  ecma_make_uint32_value (byte_length) };
  ecma_object_t *old_new_target_p = JERRY_CONTEXT (current_new_target_p);
  if (old_new_target_p == NULL)
  {
    JERRY_CONTEXT (current_new_target_p) = ecma_builtin_get (ECMA_BUILTIN_ID_DATAVIEW);
  }

  ecma_value_t dataview_value = ecma_op_dataview_create (arguments_p, 3);
  JERRY_CONTEXT (current_new_target_p) = old_new_target_p;
  return jerry_return (dataview_value);
#else /* !JERRY_BUILTIN_DATAVIEW */
  JERRY_UNUSED (array_buffer);
  JERRY_UNUSED (byte_offset);
  JERRY_UNUSED (byte_length);
  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_DATA_VIEW_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_DATAVIEW */
} /* jerry_dataview */

/**
 * Check if the given value is a DataView object.
 *
 * @return true - if it is a DataView object
 *         false - otherwise
 */
bool
jerry_value_is_dataview (const jerry_value_t value) /**< value to check if it is a DataView object */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_DATAVIEW
  return ecma_is_dataview (value);
#else /* !JERRY_BUILTIN_DATAVIEW */
  JERRY_UNUSED (value);
  return false;
#endif /* JERRY_BUILTIN_DATAVIEW */
} /* jerry_value_is_dataview */

/**
 * Get the underlying ArrayBuffer from a DataView.
 *
 * Additionally the byteLength and byteOffset properties are also returned
 * which were specified when the DataView was created.
 *
 * Note:
 *     the returned value must be freed with a jerry_value_free call
 *
 * @return ArrayBuffer of a DataView
 *         TypeError if the object is not a DataView.
 */
jerry_value_t
jerry_dataview_buffer (const jerry_value_t value, /**< DataView to get the arraybuffer from */
                       jerry_length_t *byte_offset, /**< [out] byteOffset property */
                       jerry_length_t *byte_length) /**< [out] byteLength property */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_DATAVIEW
  if (ecma_is_value_exception (value))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
  }

  ecma_dataview_object_t *dataview_p = ecma_op_dataview_get_object (value);

  if (JERRY_UNLIKELY (dataview_p == NULL))
  {
    return ecma_create_exception_from_context ();
  }

  if (byte_offset != NULL)
  {
    *byte_offset = dataview_p->byte_offset;
  }

  if (byte_length != NULL)
  {
    *byte_length = dataview_p->header.u.cls.u3.length;
  }

  ecma_object_t *arraybuffer_p = dataview_p->buffer_p;
  ecma_ref_object (arraybuffer_p);

  return ecma_make_object_value (arraybuffer_p);
#else /* !JERRY_BUILTIN_DATAVIEW */
  JERRY_UNUSED (value);
  JERRY_UNUSED (byte_offset);
  JERRY_UNUSED (byte_length);
  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_DATA_VIEW_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_DATAVIEW */
} /* jerry_dataview_buffer */

/**
 * TypedArray related functions
 */

/**
 * Check if the given value is a TypedArray object.
 *
 * @return true - if it is a TypedArray object
 *         false - otherwise
 */
bool
jerry_value_is_typedarray (const jerry_value_t value) /**< value to check if it is a TypedArray */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_TYPEDARRAY
  return ecma_is_typedarray (value);
#else /* !JERRY_BUILTIN_TYPEDARRAY */
  JERRY_UNUSED (value);
  return false;
#endif /* JERRY_BUILTIN_TYPEDARRAY */
} /* jerry_value_is_typedarray */

#if JERRY_BUILTIN_TYPEDARRAY
/**
 * TypedArray mapping type
 */
typedef struct
{
  jerry_typedarray_type_t api_type; /**< api type */
  ecma_builtin_id_t prototype_id; /**< prototype ID */
  ecma_typedarray_type_t id; /**< typedArray ID */
  uint8_t element_size_shift; /**< element size shift */
} jerry_typedarray_mapping_t;

/**
 * List of TypedArray mappings
 */
static jerry_typedarray_mapping_t jerry_typedarray_mappings[] = {
#define TYPEDARRAY_ENTRY(NAME, LIT_NAME, SIZE_SHIFT)                                                      \
  {                                                                                                       \
    JERRY_TYPEDARRAY_##NAME, ECMA_BUILTIN_ID_##NAME##ARRAY_PROTOTYPE, ECMA_##LIT_NAME##_ARRAY, SIZE_SHIFT \
  }

  TYPEDARRAY_ENTRY (UINT8, UINT8, 0),       TYPEDARRAY_ENTRY (UINT8CLAMPED, UINT8_CLAMPED, 0),
  TYPEDARRAY_ENTRY (INT8, INT8, 0),         TYPEDARRAY_ENTRY (UINT16, UINT16, 1),
  TYPEDARRAY_ENTRY (INT16, INT16, 1),       TYPEDARRAY_ENTRY (UINT32, UINT32, 2),
  TYPEDARRAY_ENTRY (INT32, INT32, 2),       TYPEDARRAY_ENTRY (FLOAT32, FLOAT32, 2),
#if JERRY_NUMBER_TYPE_FLOAT64
  TYPEDARRAY_ENTRY (FLOAT64, FLOAT64, 3),
#endif /* JERRY_NUMBER_TYPE_FLOAT64 */
#if JERRY_BUILTIN_BIGINT
  TYPEDARRAY_ENTRY (BIGINT64, BIGINT64, 3), TYPEDARRAY_ENTRY (BIGUINT64, BIGUINT64, 3),
#endif /* JERRY_BUILTIN_BIGINT */
#undef TYPEDARRAY_ENTRY
};

/**
 * Helper function to get the TypedArray prototype, typedArray id, and element size shift
 * information.
 *
 * @return true - if the TypedArray information was found
 *         false - if there is no such TypedArray type
 */
static bool
jerry_typedarray_find_by_type (jerry_typedarray_type_t type_name, /**< type of the TypedArray */
                               ecma_builtin_id_t *prototype_id, /**< [out] found prototype object id */
                               ecma_typedarray_type_t *id, /**< [out] found typedArray id */
                               uint8_t *element_size_shift) /**< [out] found element size shift value */
{
  JERRY_ASSERT (prototype_id != NULL);
  JERRY_ASSERT (id != NULL);
  JERRY_ASSERT (element_size_shift != NULL);

  for (uint32_t i = 0; i < sizeof (jerry_typedarray_mappings) / sizeof (jerry_typedarray_mappings[0]); i++)
  {
    if (type_name == jerry_typedarray_mappings[i].api_type)
    {
      *prototype_id = jerry_typedarray_mappings[i].prototype_id;
      *id = jerry_typedarray_mappings[i].id;
      *element_size_shift = jerry_typedarray_mappings[i].element_size_shift;
      return true;
    }
  }

  return false;
} /* jerry_typedarray_find_by_type */

#endif /* JERRY_BUILTIN_TYPEDARRAY */

/**
 * Create a TypedArray object with a given type and length.
 *
 * Notes:
 *      * returns TypeError if an incorrect type (type_name) is specified.
 *      * byteOffset property will be set to 0.
 *      * byteLength property will be a multiple of the length parameter (based on the type).
 *
 * @return - new TypedArray object
 */
jerry_value_t
jerry_typedarray (jerry_typedarray_type_t type_name, /**< type of TypedArray to create */
                  jerry_length_t length) /**< element count of the new TypedArray */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_TYPEDARRAY
  ecma_builtin_id_t prototype_id = 0;
  ecma_typedarray_type_t id = 0;
  uint8_t element_size_shift = 0;

  if (!jerry_typedarray_find_by_type (type_name, &prototype_id, &id, &element_size_shift))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_INCORRECT_TYPE_FOR_TYPEDARRAY));
  }

  ecma_object_t *prototype_obj_p = ecma_builtin_get (prototype_id);

  ecma_value_t array_value =
    ecma_typedarray_create_object_with_length (length, NULL, prototype_obj_p, element_size_shift, id);

  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (array_value));

  return array_value;
#else /* !JERRY_BUILTIN_TYPEDARRAY */
  JERRY_UNUSED (type_name);
  JERRY_UNUSED (length);
  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_TYPED_ARRAY_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_TYPEDARRAY */
} /* jerry_typedarray */

/**
 * Create a TypedArray object using the given arraybuffer and size information.
 *
 * Notes:
 *      * returns TypeError if an incorrect type (type_name) is specified.
 *      * this is the 'new %TypedArray%(arraybuffer, byteOffset, length)' equivalent call.
 *
 * @return - new TypedArray object
 */
jerry_value_t
jerry_typedarray_with_buffer_span (jerry_typedarray_type_t type, /**< type of TypedArray to create */
                                   const jerry_value_t arraybuffer, /**< ArrayBuffer to use */
                                   jerry_length_t byte_offset, /**< offset for the ArrayBuffer */
                                   jerry_length_t length) /**< number of elements to use from ArrayBuffer */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_TYPEDARRAY
  if (ecma_is_value_exception (arraybuffer))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  ecma_builtin_id_t prototype_id = 0;
  ecma_typedarray_type_t id = 0;
  uint8_t element_size_shift = 0;

  if (!jerry_typedarray_find_by_type (type, &prototype_id, &id, &element_size_shift))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_INCORRECT_TYPE_FOR_TYPEDARRAY));
  }

  if (!ecma_is_arraybuffer (arraybuffer))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_ARGUMENT_NOT_ARRAY_BUFFER));
  }

  ecma_object_t *prototype_obj_p = ecma_builtin_get (prototype_id);
  ecma_value_t arguments_p[3] = { arraybuffer, ecma_make_uint32_value (byte_offset), ecma_make_uint32_value (length) };

  ecma_value_t array_value = ecma_op_create_typedarray (arguments_p, 3, prototype_obj_p, element_size_shift, id);
  ecma_free_value (arguments_p[1]);
  ecma_free_value (arguments_p[2]);

  return jerry_return (array_value);
#else /* !JERRY_BUILTIN_TYPEDARRAY */
  JERRY_UNUSED (type);
  JERRY_UNUSED (arraybuffer);
  JERRY_UNUSED (byte_offset);
  JERRY_UNUSED (length);
  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_TYPED_ARRAY_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_TYPEDARRAY */
} /* jerry_typedarray_with_buffer_span */

/**
 * Create a TypedArray object using the given arraybuffer and size information.
 *
 * Notes:
 *      * returns TypeError if an incorrect type (type_name) is specified.
 *      * this is the 'new %TypedArray%(arraybuffer)' equivalent call.
 *
 * @return - new TypedArray object
 */
jerry_value_t
jerry_typedarray_with_buffer (jerry_typedarray_type_t type, /**< type of TypedArray to create */
                              const jerry_value_t arraybuffer) /**< ArrayBuffer to use */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_TYPEDARRAY
  if (ecma_is_value_exception (arraybuffer))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  jerry_length_t byte_length = jerry_arraybuffer_size (arraybuffer);
  return jerry_typedarray_with_buffer_span (type, arraybuffer, 0, byte_length);
#else /* !JERRY_BUILTIN_TYPEDARRAY */
  JERRY_UNUSED (type);
  JERRY_UNUSED (arraybuffer);
  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_TYPED_ARRAY_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_TYPEDARRAY */
} /* jerry_typedarray_with_buffer */

/**
 * Get the type of the TypedArray.
 *
 * @return - type of the TypedArray
 *         - JERRY_TYPEDARRAY_INVALID if the argument is not a TypedArray
 */
jerry_typedarray_type_t
jerry_typedarray_type (const jerry_value_t value) /**< object to get the TypedArray type */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_TYPEDARRAY
  if (!ecma_is_typedarray (value))
  {
    return JERRY_TYPEDARRAY_INVALID;
  }

  ecma_object_t *array_p = ecma_get_object_from_value (value);
  ecma_typedarray_type_t class_type = ecma_get_typedarray_id (array_p);

  for (uint32_t i = 0; i < sizeof (jerry_typedarray_mappings) / sizeof (jerry_typedarray_mappings[0]); i++)
  {
    if (class_type == jerry_typedarray_mappings[i].id)
    {
      return jerry_typedarray_mappings[i].api_type;
    }
  }
#else /* !JERRY_BUILTIN_TYPEDARRAY */
  JERRY_UNUSED (value);
#endif /* JERRY_BUILTIN_TYPEDARRAY */

  return JERRY_TYPEDARRAY_INVALID;
} /* jerry_typedarray_type */

/**
 * Get the element count of the TypedArray.
 *
 * @return length of the TypedArray.
 */
jerry_length_t
jerry_typedarray_length (const jerry_value_t value) /**< TypedArray to query */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_TYPEDARRAY
  if (ecma_is_typedarray (value))
  {
    ecma_object_t *array_p = ecma_get_object_from_value (value);
    return ecma_typedarray_get_length (array_p);
  }
#else /* !JERRY_BUILTIN_TYPEDARRAY */
  JERRY_UNUSED (value);
#endif /* JERRY_BUILTIN_TYPEDARRAY */

  return 0;
} /* jerry_typedarray_length */

/**
 * Get the underlying ArrayBuffer from a TypedArray.
 *
 * Additionally the byteLength and byteOffset properties are also returned
 * which were specified when the TypedArray was created.
 *
 * Note:
 *     the returned value must be freed with a jerry_value_free call
 *
 * @return ArrayBuffer of a TypedArray
 *         TypeError if the object is not a TypedArray.
 */
jerry_value_t
jerry_typedarray_buffer (const jerry_value_t value, /**< TypedArray to get the arraybuffer from */
                         jerry_length_t *byte_offset, /**< [out] byteOffset property */
                         jerry_length_t *byte_length) /**< [out] byteLength property */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_TYPEDARRAY
  if (!ecma_is_typedarray (value))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_OBJECT_IS_NOT_A_TYPEDARRAY));
  }

  ecma_object_t *array_p = ecma_get_object_from_value (value);
  uint8_t shift = ecma_typedarray_get_element_size_shift (array_p);

  if (byte_length != NULL)
  {
    *byte_length = (jerry_length_t) (ecma_typedarray_get_length (array_p) << shift);
  }

  if (byte_offset != NULL)
  {
    *byte_offset = (jerry_length_t) ecma_typedarray_get_offset (array_p);
  }

  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (array_p);
  ecma_ref_object (arraybuffer_p);
  return jerry_return (ecma_make_object_value (arraybuffer_p));
#else /* !JERRY_BUILTIN_TYPEDARRAY */
  JERRY_UNUSED (value);
  JERRY_UNUSED (byte_length);
  JERRY_UNUSED (byte_offset);
  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_TYPED_ARRAY_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_TYPEDARRAY */
} /* jerry_typedarray_buffer */

/**
 * Parse the given input buffer as a JSON string. The behaviour is equivalent with the "JSON.parse(string)" JS
 * call. The input buffer can be encoded as either cesu-8 or utf-8, but it is the callers responsibility to make sure
 * the encoding is valid.
 *
 *
 * @return object value, or exception
 */
jerry_value_t
jerry_json_parse (const jerry_char_t *string_p, /**< json string */
                  jerry_size_t string_size) /**< json string size */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_JSON
  ecma_value_t ret_value = ecma_builtin_json_parse_buffer (string_p, string_size);

  if (ecma_is_value_undefined (ret_value))
  {
    ret_value = jerry_throw_sz (JERRY_ERROR_SYNTAX, ecma_get_error_msg (ECMA_ERR_JSON_STRING_PARSE_ERROR));
  }

  return jerry_return (ret_value);
#else /* !JERRY_BUILTIN_JSON */
  JERRY_UNUSED (string_p);
  JERRY_UNUSED (string_size);

  return jerry_throw_sz (JERRY_ERROR_SYNTAX, ecma_get_error_msg (ECMA_ERR_JSON_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_JSON */
} /* jerry_json_parse */

/**
 * Create a JSON string from a JavaScript value.
 *
 * The behaviour is equivalent with the "JSON.stringify(input_value)" JS call.
 *
 * Note:
 *      The returned value must be freed with jerry_value_free,
 *
 * @return - jerry_value_t containing a JSON string.
 *         - Error value if there was a problem during the stringification.
 */
jerry_value_t
jerry_json_stringify (const jerry_value_t input_value) /**< a value to stringify */
{
  jerry_assert_api_enabled ();
#if JERRY_BUILTIN_JSON
  if (ecma_is_value_exception (input_value))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
  }

  ecma_value_t ret_value = ecma_builtin_json_stringify_no_opts (input_value);

  if (ecma_is_value_undefined (ret_value))
  {
    ret_value = jerry_throw_sz (JERRY_ERROR_SYNTAX, ecma_get_error_msg (ECMA_ERR_JSON_STRINGIFY_ERROR));
  }

  return jerry_return (ret_value);
#else /* JERRY_BUILTIN_JSON */
  JERRY_UNUSED (input_value);

  return jerry_throw_sz (JERRY_ERROR_SYNTAX, ecma_get_error_msg (ECMA_ERR_JSON_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_JSON */
} /* jerry_json_stringify */

/**
 * Create a container type specified in jerry_container_type_t.
 * The container can be created with a list of arguments, which will be passed to the container constructor to be
 * inserted to the container.
 *
 * Note:
 *      The returned value must be freed with jerry_value_free
 * @return jerry_value_t representing a container with the given type.
 */
jerry_value_t
jerry_container (jerry_container_type_t container_type, /**< Type of the container */
                 const jerry_value_t *arguments_list_p, /**< arguments list */
                 jerry_length_t arguments_list_len) /**< Length of arguments list */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_CONTAINER
  for (jerry_length_t i = 0; i < arguments_list_len; i++)
  {
    if (ecma_is_value_exception (arguments_list_p[i]))
    {
      return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_VALUE_MSG));
    }
  }

  lit_magic_string_id_t lit_id;
  ecma_builtin_id_t proto_id;
  ecma_builtin_id_t ctor_id;

  switch (container_type)
  {
    case JERRY_CONTAINER_TYPE_MAP:
    {
      lit_id = LIT_MAGIC_STRING_MAP_UL;
      proto_id = ECMA_BUILTIN_ID_MAP_PROTOTYPE;
      ctor_id = ECMA_BUILTIN_ID_MAP;
      break;
    }
    case JERRY_CONTAINER_TYPE_SET:
    {
      lit_id = LIT_MAGIC_STRING_SET_UL;
      proto_id = ECMA_BUILTIN_ID_SET_PROTOTYPE;
      ctor_id = ECMA_BUILTIN_ID_SET;
      break;
    }
    case JERRY_CONTAINER_TYPE_WEAKMAP:
    {
      lit_id = LIT_MAGIC_STRING_WEAKMAP_UL;
      proto_id = ECMA_BUILTIN_ID_WEAKMAP_PROTOTYPE;
      ctor_id = ECMA_BUILTIN_ID_WEAKMAP;
      break;
    }
    case JERRY_CONTAINER_TYPE_WEAKSET:
    {
      lit_id = LIT_MAGIC_STRING_WEAKSET_UL;
      proto_id = ECMA_BUILTIN_ID_WEAKSET_PROTOTYPE;
      ctor_id = ECMA_BUILTIN_ID_WEAKSET;
      break;
    }
    default:
    {
      return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_INVALID_CONTAINER_TYPE));
    }
  }
  ecma_object_t *old_new_target_p = JERRY_CONTEXT (current_new_target_p);

  if (old_new_target_p == NULL)
  {
    JERRY_CONTEXT (current_new_target_p) = ecma_builtin_get (ctor_id);
  }

  ecma_value_t container_value = ecma_op_container_create (arguments_list_p, arguments_list_len, lit_id, proto_id);

  JERRY_CONTEXT (current_new_target_p) = old_new_target_p;
  return jerry_return (container_value);
#else /* !JERRY_BUILTIN_CONTAINER */
  JERRY_UNUSED (arguments_list_p);
  JERRY_UNUSED (arguments_list_len);
  JERRY_UNUSED (container_type);
  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_CONTAINER_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_CONTAINER */
} /* jerry_container */

/**
 * Get the type of the given container object.
 *
 * @return Corresponding type to the given container object.
 */
jerry_container_type_t
jerry_container_type (const jerry_value_t value) /**< the container object */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_CONTAINER
  if (ecma_is_value_object (value))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (value);

    if (ecma_object_class_is (obj_p, ECMA_OBJECT_CLASS_CONTAINER))
    {
      switch (((ecma_extended_object_t *) obj_p)->u.cls.u2.container_id)
      {
        case LIT_MAGIC_STRING_MAP_UL:
        {
          return JERRY_CONTAINER_TYPE_MAP;
        }
        case LIT_MAGIC_STRING_SET_UL:
        {
          return JERRY_CONTAINER_TYPE_SET;
        }
        case LIT_MAGIC_STRING_WEAKMAP_UL:
        {
          return JERRY_CONTAINER_TYPE_WEAKMAP;
        }
        case LIT_MAGIC_STRING_WEAKSET_UL:
        {
          return JERRY_CONTAINER_TYPE_WEAKSET;
        }
        default:
        {
          return JERRY_CONTAINER_TYPE_INVALID;
        }
      }
    }
  }

#else /* !JERRY_BUILTIN_CONTAINER */
  JERRY_UNUSED (value);
#endif /* JERRY_BUILTIN_CONTAINER */
  return JERRY_CONTAINER_TYPE_INVALID;
} /* jerry_container_type */

/**
 * Return a new array containing elements from a Container or a Container Iterator.
 * Sets the boolean input value to `true` if the container object has key/value pairs.
 *
 * Note:
 *     the returned value must be freed with a jerry_value_free call
 *
 * @return an array of items for maps/sets or their iterators, error otherwise
 */
jerry_value_t
jerry_container_to_array (const jerry_value_t value, /**< the container or iterator object */
                          bool *is_key_value_p) /**< [out] is key-value structure */
{
  jerry_assert_api_enabled ();

#if JERRY_BUILTIN_CONTAINER
  if (!ecma_is_value_object (value))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_CONTAINER_NEEDED));
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (value);

  if (ecma_get_object_type (obj_p) != ECMA_OBJECT_TYPE_CLASS)
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_CONTAINER_NEEDED));
  }

  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

  uint32_t entry_count;
  uint8_t entry_size;

  uint32_t index = 0;
  uint8_t iterator_kind = ECMA_ITERATOR__COUNT;
  ecma_value_t *start_p;

  *is_key_value_p = false;

  if (ext_obj_p->u.cls.type == ECMA_OBJECT_CLASS_MAP_ITERATOR
      || ext_obj_p->u.cls.type == ECMA_OBJECT_CLASS_SET_ITERATOR)
  {
    ecma_value_t iterated_value = ext_obj_p->u.cls.u3.iterated_value;

    if (ecma_is_value_empty (iterated_value))
    {
      return ecma_op_new_array_object_from_collection (ecma_new_collection (), false);
    }

    ecma_extended_object_t *map_object_p = (ecma_extended_object_t *) (ecma_get_object_from_value (iterated_value));

    ecma_collection_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_collection_t, map_object_p->u.cls.u3.value);
    entry_count = ECMA_CONTAINER_ENTRY_COUNT (container_p);
    index = ext_obj_p->u.cls.u2.iterator_index;

    entry_size = ecma_op_container_entry_size (map_object_p->u.cls.u2.container_id);
    start_p = ECMA_CONTAINER_START (container_p);

    iterator_kind = ext_obj_p->u.cls.u1.iterator_kind;
  }
  else if (jerry_container_type (value) != JERRY_CONTAINER_TYPE_INVALID)
  {
    ecma_collection_t *container_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_collection_t, ext_obj_p->u.cls.u3.value);
    entry_count = ECMA_CONTAINER_ENTRY_COUNT (container_p);
    entry_size = ecma_op_container_entry_size (ext_obj_p->u.cls.u2.container_id);

    index = 0;
    iterator_kind = ECMA_ITERATOR_KEYS;
    start_p = ECMA_CONTAINER_START (container_p);

    if (ext_obj_p->u.cls.u2.container_id == LIT_MAGIC_STRING_MAP_UL
        || ext_obj_p->u.cls.u2.container_id == LIT_MAGIC_STRING_WEAKMAP_UL)
    {
      iterator_kind = ECMA_ITERATOR_ENTRIES;
    }
  }
  else
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_CONTAINER_NEEDED));
  }

  *is_key_value_p = (iterator_kind == ECMA_ITERATOR_ENTRIES);
  ecma_collection_t *collection_buffer = ecma_new_collection ();

  for (uint32_t i = index; i < entry_count; i += entry_size)
  {
    ecma_value_t *entry_p = start_p + i;

    if (ecma_is_value_empty (*entry_p))
    {
      continue;
    }

    if (iterator_kind != ECMA_ITERATOR_VALUES)
    {
      ecma_collection_push_back (collection_buffer, ecma_copy_value_if_not_object (entry_p[0]));
    }

    if (iterator_kind != ECMA_ITERATOR_KEYS)
    {
      ecma_collection_push_back (collection_buffer, ecma_copy_value_if_not_object (entry_p[1]));
    }
  }
  return ecma_op_new_array_object_from_collection (collection_buffer, false);
#else /* !JERRY_BUILTIN_CONTAINER */
  JERRY_UNUSED (value);
  JERRY_UNUSED (is_key_value_p);
  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_CONTAINER_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_CONTAINER */
} /* jerry_container_to_array */

/**
 * Perform container operation on the given operands (add, get, set, has, delete, size, clear).
 *
 * @return error - if argument is invalid or operation is unsuccessful or unsupported
 *                 result of the container operation - otherwise.
 */
jerry_value_t
jerry_container_op (jerry_container_op_t operation, /**< container operation */
                    jerry_value_t container, /**< container */
                    const jerry_value_t *arguments, /**< list of arguments */
                    uint32_t arguments_number) /**< number of arguments */
{
  jerry_assert_api_enabled ();
#if JERRY_BUILTIN_CONTAINER
  if (!ecma_is_value_object (container))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_CONTAINER_IS_NOT_AN_OBJECT));
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (container);

  if (ecma_get_object_type (obj_p) != ECMA_OBJECT_TYPE_CLASS)
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_CONTAINER_IS_NOT_A_CONTAINER_OBJECT));
  }
  uint16_t type = ((ecma_extended_object_t *) obj_p)->u.cls.u2.container_id;
  ecma_extended_object_t *container_object_p = ecma_op_container_get_object (container, type);

  if (container_object_p == NULL)
  {
    return ecma_create_exception_from_context ();
  }

  switch (operation)
  {
    case JERRY_CONTAINER_OP_ADD:
    case JERRY_CONTAINER_OP_DELETE:
    case JERRY_CONTAINER_OP_GET:
    case JERRY_CONTAINER_OP_HAS:
    {
      if (arguments_number != 1 || ecma_is_value_exception (arguments[0]))
      {
        return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
      }
      break;
    }
    case JERRY_CONTAINER_OP_SET:
    {
      if (arguments_number != 2 || ecma_is_value_exception (arguments[0]) || ecma_is_value_exception (arguments[1]))
      {
        return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
      }
      break;
    }
    case JERRY_CONTAINER_OP_CLEAR:
    case JERRY_CONTAINER_OP_SIZE:
    {
      if (arguments_number != 0)
      {
        return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
      }
      break;
    }
    default:
    {
      return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_WRONG_ARGS_MSG));
    }
  }

  jerry_value_t result;

  switch (operation)
  {
    case JERRY_CONTAINER_OP_ADD:
    {
      if (type == LIT_MAGIC_STRING_MAP_UL || type == LIT_MAGIC_STRING_WEAKMAP_UL)
      {
        return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_INCORRECT_TYPE_CALL));
      }
      result = ecma_op_container_set (container_object_p, arguments[0], arguments[0], type);
      break;
    }
    case JERRY_CONTAINER_OP_GET:
    {
      if (type == LIT_MAGIC_STRING_SET_UL || type == LIT_MAGIC_STRING_WEAKSET_UL)
      {
        return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_INCORRECT_TYPE_CALL));
      }
      result = ecma_op_container_get (container_object_p, arguments[0], type);
      break;
    }
    case JERRY_CONTAINER_OP_SET:
    {
      if (type == LIT_MAGIC_STRING_SET_UL || type == LIT_MAGIC_STRING_WEAKSET_UL)
      {
        return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_INCORRECT_TYPE_CALL));
      }
      result = ecma_op_container_set (container_object_p, arguments[0], arguments[1], type);
      break;
    }
    case JERRY_CONTAINER_OP_HAS:
    {
      result = ecma_op_container_has (container_object_p, arguments[0], type);
      break;
    }
    case JERRY_CONTAINER_OP_DELETE:
    {
      if (type == LIT_MAGIC_STRING_WEAKMAP_UL || type == LIT_MAGIC_STRING_WEAKSET_UL)
      {
        result = ecma_op_container_delete_weak (container_object_p, arguments[0], type);
        break;
      }
      result = ecma_op_container_delete (container_object_p, arguments[0], type);
      break;
    }
    case JERRY_CONTAINER_OP_SIZE:
    {
      result = ecma_op_container_size (container_object_p);
      break;
    }
    case JERRY_CONTAINER_OP_CLEAR:
    {
      if (type == LIT_MAGIC_STRING_WEAKSET_UL || type == LIT_MAGIC_STRING_WEAKMAP_UL)
      {
        return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_INCORRECT_TYPE_CALL));
      }
      result = ecma_op_container_clear (container_object_p);
      break;
    }
    default:
    {
      result = jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_UNSUPPORTED_CONTAINER_OPERATION));
      break;
    }
  }
  return jerry_return (result);
#else /* !JERRY_BUILTIN_CONTAINER */
  JERRY_UNUSED (operation);
  JERRY_UNUSED (container);
  JERRY_UNUSED (arguments);
  JERRY_UNUSED (arguments_number);
  return jerry_throw_sz (JERRY_ERROR_TYPE, ecma_get_error_msg (ECMA_ERR_CONTAINER_NOT_SUPPORTED));
#endif /* JERRY_BUILTIN_CONTAINER */
} /* jerry_container_op */

/**
 * @}
 */
