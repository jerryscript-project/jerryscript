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

#include "main-utils.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jerryscript-port-default.h"
#include "jerryscript-port.h"
#include "jerryscript.h"

#include "jerryscript-ext/debugger.h"
#include "jerryscript-ext/handler.h"
#include "main-options.h"

/**
 * Max line size that will be printed on a Syntax Error
 */
#define SYNTAX_ERROR_MAX_LINE_LENGTH 256

/**
 * Register a JavaScript function in the global object.
 */
static void
main_register_global_function (const char *name_p, /**< name of the function */
                               jerry_external_handler_t handler_p) /**< function callback */
{
  jerry_value_t result_val = jerryx_handler_register_global (name_p, handler_p);
  assert (!jerry_value_is_exception (result_val));
  jerry_value_free (result_val);
} /* main_register_global_function */

static jerry_value_t
main_create_realm (const jerry_call_info_t *call_info_p, /**< call information */
                   const jerry_value_t args_p[], /**< function arguments */
                   const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */
  (void) args_p; /* unused */
  (void) args_cnt; /* unused */
  return jerry_realm ();
} /* main_create_realm */

/**
 * Register a method for the $262 object.
 */
static void
test262_register_function (jerry_value_t test262_obj, /** $262 object */
                           const char *name_p, /**< name of the function */
                           jerry_external_handler_t handler_p) /**< function callback */
{
  jerry_value_t function_name_val = jerry_string_sz (name_p);
  jerry_value_t function_val = jerry_function_external (handler_p);

  jerry_value_t result_val = jerry_object_set (test262_obj, function_name_val, function_val);

  jerry_value_free (function_val);
  jerry_value_free (function_name_val);

  assert (!jerry_value_is_exception (result_val));
  jerry_value_free (result_val);
} /* test262_register_function */

/**
 * $262.detachArrayBuffer
 *
 * A function which implements the DetachArrayBuffer abstract operation
 *
 * @return null value - if success
 *         value marked with error flag - otherwise
 */
static jerry_value_t
test262_detach_array_buffer (const jerry_call_info_t *call_info_p, /**< call information */
                             const jerry_value_t args_p[], /**< function arguments */
                             const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */

  if (args_cnt < 1 || !jerry_value_is_arraybuffer (args_p[0]))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, "Expected an ArrayBuffer object");
  }

  /* TODO: support the optional 'key' argument */

  return jerry_arraybuffer_detach (args_p[0]);
} /* test262_detach_array_buffer */

/**
 * $262.evalScript
 *
 * A function which accepts a string value as its first argument and executes it
 *
 * @return completion of the script parsing and execution.
 */
static jerry_value_t
test262_eval_script (const jerry_call_info_t *call_info_p, /**< call information */
                     const jerry_value_t args_p[], /**< function arguments */
                     const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */

  if (args_cnt < 1 || !jerry_value_is_string (args_p[0]))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, "Expected a string");
  }

  jerry_size_t str_size = jerry_string_size (args_p[0], JERRY_ENCODING_UTF8);
  jerry_char_t *str_buf_p = malloc (str_size * sizeof (jerry_char_t));

  if (str_buf_p == NULL)
  {
    return jerry_throw_sz (JERRY_ERROR_RANGE, "Internal allocation error");
  }

  jerry_string_to_buffer (args_p[0], JERRY_ENCODING_UTF8, str_buf_p, str_size);

  jerry_value_t ret_value = jerry_parse (str_buf_p, str_size, NULL);

  if (!jerry_value_is_exception (ret_value))
  {
    jerry_value_t func_val = ret_value;
    ret_value = jerry_run (func_val);
    jerry_value_free (func_val);
  }

  free (str_buf_p);

  return ret_value;
} /* test262_eval_script */

static jerry_value_t create_test262 (jerry_value_t global_obj);

/**
 * $262.createRealm
 *
 * A function which creates a new realm object, and returns a newly created $262 object
 *
 * @return a new $262 object
 */
static jerry_value_t
test262_create_realm (const jerry_call_info_t *call_info_p, /**< call information */
                      const jerry_value_t args_p[], /**< function arguments */
                      const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */
  (void) args_p; /* unused */
  (void) args_cnt; /* unused */

  jerry_value_t realm_object = jerry_realm ();
  jerry_value_t previous_realm = jerry_set_realm (realm_object);
  assert (!jerry_value_is_exception (previous_realm));
  jerry_value_t test262_object = create_test262 (realm_object);
  jerry_set_realm (previous_realm);
  jerry_value_free (realm_object);

  return test262_object;
} /* test262_create_realm */

/**
 * Create a new $262 object
 *
 * @return a new $262 object
 */
static jerry_value_t
create_test262 (jerry_value_t global_obj) /**< global object */
{
  jerry_value_t test262_object = jerry_object ();

  test262_register_function (test262_object, "detachArrayBuffer", test262_detach_array_buffer);
  test262_register_function (test262_object, "evalScript", test262_eval_script);
  test262_register_function (test262_object, "createRealm", test262_create_realm);
  test262_register_function (test262_object, "gc", jerryx_handler_gc);

  jerry_value_t prop_name = jerry_string_sz ("global");
  jerry_value_t result = jerry_object_set (test262_object, prop_name, global_obj);
  assert (!jerry_value_is_exception (result));

  jerry_value_free (prop_name);
  jerry_value_free (result);

  prop_name = jerry_string_sz ("$262");
  result = jerry_object_set (global_obj, prop_name, test262_object);
  assert (!jerry_value_is_exception (result));

  jerry_value_free (prop_name);
  jerry_value_free (result);

  return test262_object;
} /* create_test262 */

static void
promise_callback (jerry_promise_event_type_t event_type, /**< event type */
                  const jerry_value_t object, /**< target object */
                  const jerry_value_t value, /**< optional argument */
                  void *user_p) /**< user pointer passed to the callback */
{
  (void) value; /* unused */
  (void) user_p; /* unused */
  const jerry_size_t max_allowed_size = 5 * 1024 - 1;

  if (event_type != JERRY_PROMISE_EVENT_REJECT_WITHOUT_HANDLER)
  {
    return;
  }

  jerry_value_t reason = jerry_promise_result (object);
  jerry_value_t reason_to_string = jerry_value_to_string (reason);

  if (!jerry_value_is_exception (reason_to_string))
  {
    jerry_size_t buffer_size = jerry_string_size (reason_to_string, JERRY_ENCODING_UTF8);

    if (buffer_size > max_allowed_size)
    {
      buffer_size = max_allowed_size;
    }

    JERRY_VLA (jerry_char_t, str_buf_p, buffer_size + 1);
    jerry_string_to_buffer (reason_to_string, JERRY_ENCODING_UTF8, str_buf_p, buffer_size);
    str_buf_p[buffer_size] = '\0';

    jerry_port_log (JERRY_LOG_LEVEL_WARNING, "Uncaught Promise rejection: %s\n", str_buf_p);
  }
  else
  {
    jerry_port_log (JERRY_LOG_LEVEL_WARNING, "Uncaught Promise rejection (reason cannot be converted to string)\n");
  }

  jerry_value_free (reason_to_string);
  jerry_value_free (reason);
} /* promise_callback */

/**
 * Inits the engine and the debugger
 */
void
main_init_engine (main_args_t *arguments_p) /**< main arguments */
{
  jerry_init (arguments_p->init_flags);

  jerry_promise_on_event (JERRY_PROMISE_EVENT_FILTER_ERROR, promise_callback, NULL);

  if (arguments_p->option_flags & OPT_FLAG_DEBUG_SERVER)
  {
    bool protocol = false;

    if (!strcmp (arguments_p->debug_protocol, "tcp"))
    {
      protocol = jerryx_debugger_tcp_create (arguments_p->debug_port);
    }
    else
    {
      assert (!strcmp (arguments_p->debug_protocol, "serial"));
      protocol = jerryx_debugger_serial_create (arguments_p->debug_serial_config);
    }

    if (!strcmp (arguments_p->debug_channel, "rawpacket"))
    {
      jerryx_debugger_after_connect (protocol && jerryx_debugger_rp_create ());
    }
    else
    {
      assert (!strcmp (arguments_p->debug_channel, "websocket"));
      jerryx_debugger_after_connect (protocol && jerryx_debugger_ws_create ());
    }
  }
  if (arguments_p->option_flags & OPT_FLAG_TEST262_OBJECT)
  {
    jerry_value_t global_obj = jerry_current_realm ();
    jerry_value_t test262_object = create_test262 (global_obj);
    jerry_value_free (test262_object);
    jerry_value_free (global_obj);
  }
  main_register_global_function ("assert", jerryx_handler_assert);
  main_register_global_function ("gc", jerryx_handler_gc);
  main_register_global_function ("print", jerryx_handler_print);
  main_register_global_function ("sourceName", jerryx_handler_source_name);
  main_register_global_function ("createRealm", main_create_realm);
} /* main_init_engine */

/**
 * Print an error value.
 *
 * Note: the error value will be released.
 */
void
main_print_unhandled_exception (jerry_value_t error_value) /**< error value */
{
  assert (jerry_value_is_exception (error_value));
  error_value = jerry_exception_value (error_value, true);

  jerry_char_t err_str_buf[256];

  jerry_value_t err_str_val = jerry_value_to_string (error_value);

  jerry_size_t string_end =
    jerry_string_to_buffer (err_str_val, JERRY_ENCODING_UTF8, err_str_buf, sizeof (err_str_buf) - 1);
  err_str_buf[string_end] = '\0';

  if (jerry_feature_enabled (JERRY_FEATURE_ERROR_MESSAGES) && jerry_error_type (error_value) == JERRY_ERROR_SYNTAX)
  {
    jerry_char_t *string_end_p = err_str_buf + string_end;
    unsigned int err_line = 0;
    unsigned int err_col = 0;
    char *path_str_p = NULL;
    char *path_str_end_p = NULL;

    /* 1. parse column and line information */
    for (jerry_char_t *current_p = err_str_buf; current_p < string_end_p; current_p++)
    {
      if (*current_p == '[')
      {
        current_p++;

        if (*current_p == '<')
        {
          break;
        }

        path_str_p = (char *) current_p;
        while (current_p < string_end_p && *current_p != ':')
        {
          current_p++;
        }

        path_str_end_p = (char *) current_p++;

        err_line = (unsigned int) strtol ((char *) current_p, (char **) &current_p, 10);

        current_p++;

        err_col = (unsigned int) strtol ((char *) current_p, NULL, 10);
        break;
      }
    } /* for */

    if (err_line != 0 && err_col > 0 && err_col < SYNTAX_ERROR_MAX_LINE_LENGTH)
    {
      /* Temporarily modify the error message, so we can use the path. */
      *path_str_end_p = '\0';

      size_t source_size;
      uint8_t *source_p = jerry_port_read_source (path_str_p, &source_size);

      /* Revert the error message. */
      *path_str_end_p = ':';

      if (source_p != NULL)
      {
        uint32_t curr_line = 1;
        uint32_t pos = 0;

        /* 2. seek and print */
        while (pos < source_size && curr_line < err_line)
        {
          if (source_p[pos] == '\n')
          {
            curr_line++;
          }

          pos++;
        }

        /* Print character if:
         * - The max line length is not reached.
         * - The current position is valid (it is not the end of the source).
         * - The current character is not a newline.
         **/
        for (uint32_t char_count = 0;
             (char_count < SYNTAX_ERROR_MAX_LINE_LENGTH) && (pos < source_size) && (source_p[pos] != '\n');
             char_count++, pos++)
        {
          jerry_port_log (JERRY_LOG_LEVEL_ERROR, "%c", source_p[pos]);
        }
        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "\n");

        jerry_port_release_source (source_p);

        while (--err_col)
        {
          jerry_port_log (JERRY_LOG_LEVEL_ERROR, "~");
        }

        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "^\n\n");
      }
    }
  }

  jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Unhandled exception: %s\n", err_str_buf);
  jerry_value_free (err_str_val);

  if (jerry_value_is_object (error_value))
  {
    jerry_value_t stack_str = jerry_string_sz ("stack");
    jerry_value_t backtrace_val = jerry_object_get (error_value, stack_str);
    jerry_value_free (stack_str);

    if (jerry_value_is_array (backtrace_val))
    {
      uint32_t length = jerry_array_length (backtrace_val);

      /* This length should be enough. */
      if (length > 32)
      {
        length = 32;
      }

      for (uint32_t i = 0; i < length; i++)
      {
        jerry_value_t item_val = jerry_object_get_index (backtrace_val, i);

        if (jerry_value_is_string (item_val))
        {
          string_end = jerry_string_to_buffer (item_val, JERRY_ENCODING_UTF8, err_str_buf, sizeof (err_str_buf) - 1);
          err_str_buf[string_end] = '\0';

          printf ("%6u: %s\n", i, err_str_buf);
        }

        jerry_value_free (item_val);
      }
    }

    jerry_value_free (backtrace_val);
  }

  jerry_value_free (error_value);
} /* main_print_unhandled_exception */

/**
 * Runs the source code received by jerry_debugger_wait_for_client_source.
 *
 * @return result fo the source code execution
 */
jerry_value_t
main_wait_for_source_callback (const jerry_char_t *source_name_p, /**< source name */
                               size_t source_name_size, /**< size of source name */
                               const jerry_char_t *source_p, /**< source code */
                               size_t source_size, /**< source code size */
                               void *user_p) /**< user pointer */
{
  (void) user_p; /* unused */

  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_HAS_SOURCE_NAME;
  parse_options.source_name = jerry_string (source_name_p, (jerry_size_t) source_name_size, JERRY_ENCODING_UTF8);

  jerry_value_t ret_val = jerry_parse (source_p, source_size, &parse_options);

  jerry_value_free (parse_options.source_name);

  if (!jerry_value_is_exception (ret_val))
  {
    jerry_value_t func_val = ret_val;
    ret_val = jerry_run (func_val);
    jerry_value_free (func_val);
  }

  return ret_val;
} /* main_wait_for_source_callback */

/**
 * Check that value contains the reset abort value.
 *
 * Note: if the value is the reset abort value, the value is release.
 *
 * return true, if reset abort
 *        false, otherwise
 */
bool
main_is_value_reset (jerry_value_t value) /**< jerry value */
{
  if (!jerry_value_is_abort (value))
  {
    return false;
  }

  jerry_value_t abort_value = jerry_exception_value (value, false);

  if (!jerry_value_is_string (abort_value))
  {
    jerry_value_free (abort_value);
    return false;
  }

  static const char restart_str[] = "r353t";

  jerry_size_t str_size = jerry_string_size (abort_value, JERRY_ENCODING_CESU8);
  bool is_reset = false;

  if (str_size == sizeof (restart_str) - 1)
  {
    JERRY_VLA (jerry_char_t, str_buf, str_size);
    jerry_string_to_buffer (abort_value, JERRY_ENCODING_CESU8, str_buf, str_size);

    is_reset = memcmp (restart_str, (char *) (str_buf), str_size) == 0;

    if (is_reset)
    {
      jerry_value_free (value);
    }
  }

  jerry_value_free (abort_value);
  return is_reset;
} /* main_is_value_reset */
