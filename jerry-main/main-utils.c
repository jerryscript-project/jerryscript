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
  jerry_value_t result_val = jerryx_handler_register_global ((const jerry_char_t *) name_p, handler_p);
  assert (!jerry_value_is_error (result_val));
  jerry_release_value (result_val);
} /* main_register_global_function */

static jerry_value_t
main_create_realm (const jerry_call_info_t *call_info_p, /**< call information */
                   const jerry_value_t args_p[], /**< function arguments */
                   const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */
  (void) args_p; /* unused */
  (void) args_cnt; /* unused */
  return jerry_create_realm ();
} /* main_create_realm */

/**
 * Register a method for the $262 object.
 */
static void
test262_register_function (jerry_value_t test262_obj, /** $262 object */
                           const char *name_p, /**< name of the function */
                           jerry_external_handler_t handler_p) /**< function callback */
{
  jerry_value_t function_name_val = jerry_create_string ((const jerry_char_t *) name_p);
  jerry_value_t function_val = jerry_create_external_function (handler_p);

  jerry_value_t result_val = jerry_set_property (test262_obj, function_name_val, function_val);

  jerry_release_value (function_val);
  jerry_release_value (function_name_val);

  assert (!jerry_value_is_error (result_val));
  jerry_release_value (result_val);
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
    return jerry_create_error (JERRY_ERROR_TYPE, (jerry_char_t *) "Expected an ArrayBuffer object");
  }

  /* TODO: support the optional 'key' argument */

  return jerry_detach_arraybuffer (args_p[0]);
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
    return jerry_create_error (JERRY_ERROR_TYPE, (jerry_char_t *) "Expected a string");
  }

  jerry_size_t str_size = jerry_get_utf8_string_size (args_p[0]);
  jerry_char_t *str_buf_p = malloc (str_size * sizeof (jerry_char_t));

  if (str_buf_p == NULL || jerry_string_to_utf8_char_buffer (args_p[0], str_buf_p, str_size) != str_size)
  {
    free (str_buf_p);
    return jerry_create_error (JERRY_ERROR_RANGE, (jerry_char_t *) "Internal error");
  }

  jerry_value_t ret_value = jerry_parse (str_buf_p, str_size, NULL);

  if (!jerry_value_is_error (ret_value))
  {
    jerry_value_t func_val = ret_value;
    ret_value = jerry_run (func_val);
    jerry_release_value (func_val);
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

  jerry_value_t realm_object = jerry_create_realm ();
  jerry_value_t previous_realm = jerry_set_realm (realm_object);
  assert (!jerry_value_is_error (previous_realm));
  jerry_value_t test262_object = create_test262 (realm_object);
  jerry_set_realm (previous_realm);
  jerry_release_value (realm_object);

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
  jerry_value_t test262_object = jerry_create_object ();

  test262_register_function (test262_object, "detachArrayBuffer", test262_detach_array_buffer);
  test262_register_function (test262_object, "evalScript", test262_eval_script);
  test262_register_function (test262_object, "createRealm", test262_create_realm);
  test262_register_function (test262_object, "gc", jerryx_handler_gc);

  jerry_value_t prop_name = jerry_create_string ((const jerry_char_t *) "global");
  jerry_value_t result = jerry_set_property (test262_object, prop_name, global_obj);
  assert (!jerry_value_is_error (result));
  jerry_release_value (prop_name);
  jerry_release_value (result);
  prop_name = jerry_create_string ((const jerry_char_t *) "$262");
  result = jerry_set_property (global_obj, prop_name, test262_object);

  jerry_release_value (prop_name);
  assert (!jerry_value_is_error (result));
  jerry_release_value (result);

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

  jerry_value_t reason = jerry_get_promise_result (object);
  jerry_value_t reason_to_string = jerry_value_to_string (reason);

  if (!jerry_value_is_error (reason_to_string))
  {
    jerry_size_t buffer_size = jerry_get_utf8_string_size (reason_to_string);

    if (buffer_size > max_allowed_size)
    {
      buffer_size = max_allowed_size;
    }

    JERRY_VLA (jerry_char_t, str_buf_p, buffer_size + 1);
    jerry_string_to_utf8_char_buffer (reason_to_string, str_buf_p, buffer_size);
    str_buf_p[buffer_size] = '\0';

    jerry_port_log (JERRY_LOG_LEVEL_WARNING, "Uncaught Promise rejection: %s\n", str_buf_p);
  }
  else
  {
    jerry_port_log (JERRY_LOG_LEVEL_WARNING, "Uncaught Promise rejection (reason cannot be converted to string)\n");
  }

  jerry_release_value (reason_to_string);
  jerry_release_value (reason);
} /* promise_callback */

/**
 * Inits the engine and the debugger
 */
void
main_init_engine (main_args_t *arguments_p) /**< main arguments */
{
  jerry_init (arguments_p->init_flags);

  jerry_promise_set_callback (JERRY_PROMISE_EVENT_FILTER_ERROR, promise_callback, NULL);

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
    jerry_value_t global_obj = jerry_get_global_object ();
    jerry_value_t test262_object = create_test262 (global_obj);
    jerry_release_value (test262_object);
    jerry_release_value (global_obj);
  }
  main_register_global_function ("assert", jerryx_handler_assert);
  main_register_global_function ("gc", jerryx_handler_gc);
  main_register_global_function ("print", jerryx_handler_print);
  main_register_global_function ("resourceName", jerryx_handler_resource_name);
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
  assert (jerry_value_is_error (error_value));
  error_value = jerry_get_value_from_error (error_value, true);

  jerry_char_t err_str_buf[256];

  jerry_syntax_error_location_t location = { 0 };
  jerry_value_t resource_value = jerry_get_syntax_error_location (error_value, &location);

  /* Certain unicode newlines are not supported, and tabs does not update
   * column position, so the error printing is not always precise. */

  if (jerry_value_is_string (resource_value))
  {
    jerry_size_t resource_str_size = jerry_get_utf8_string_size (resource_value);

    if (resource_str_size < sizeof (err_str_buf))
    {
      jerry_string_to_utf8_char_buffer (resource_value, err_str_buf, resource_str_size);
      err_str_buf[resource_str_size] = 0;

      uint32_t err_line = location.line;
      uint32_t err_col = location.column_start;

      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error at %s:%d:%d\n", err_str_buf, (int) err_line, (int) err_col);

      if (err_col < SYNTAX_ERROR_MAX_LINE_LENGTH)
      {
        size_t source_size;
        uint8_t *source_p = jerry_port_read_source ((char *) err_str_buf, &source_size);

        if (source_p != NULL)
        {
          uint32_t pos = 0;

          /* 2. seek and print */
          while (pos < source_size && err_line > 1)
          {
            switch (source_p[pos])
            {
              case '\r':
              {
                if (pos + 1 < source_size && source_p[pos + 1] == '\n')
                {
                  pos++;
                }
                /* FALLTHRU */
              }
              case '\n':
              {
                err_line--;
                break;
              }
            }
            pos++;
          }

          /* Print character if:
           * - The max line length is not reached.
           * - The current position is valid (it is not the end of the source).
           * - The current character is not a newline.
           **/
          uint32_t char_count = 0;

          while (char_count < SYNTAX_ERROR_MAX_LINE_LENGTH && pos < source_size && source_p[pos] != '\r'
                 && source_p[pos] != '\n')
          {
            jerry_port_log (JERRY_LOG_LEVEL_ERROR, "%c", source_p[pos++]);
            char_count++;
          }
          jerry_port_log (JERRY_LOG_LEVEL_ERROR, "\n");

          jerry_port_release_source (source_p);

          while (--err_col)
          {
            jerry_port_log (JERRY_LOG_LEVEL_ERROR, " ");
          }

          err_col = location.column_end;

          if (err_col > char_count)
          {
            err_col = char_count + 1;
          }

          if (err_col <= location.column_start)
          {
            jerry_port_log (JERRY_LOG_LEVEL_ERROR, "^\n\n");
          }
          else
          {
            err_col -= location.column_start;

            do
            {
              jerry_port_log (JERRY_LOG_LEVEL_ERROR, "^");
            } while (--err_col);

            jerry_port_log (JERRY_LOG_LEVEL_ERROR, "\n\n");
          }
        }
      }
    }
  }

  jerry_release_value (resource_value);

  jerry_value_t err_str_val = jerry_value_to_string (error_value);
  jerry_size_t err_str_size = jerry_get_utf8_string_size (err_str_val);

  if (err_str_size >= sizeof (err_str_buf))
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "[Error message too long]\n");
  }
  else
  {
    jerry_string_to_utf8_char_buffer (err_str_val, err_str_buf, err_str_size);
    err_str_buf[err_str_size] = 0;
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "%s\n", err_str_buf);
  }

  jerry_release_value (err_str_val);

  if (jerry_value_is_object (error_value))
  {
    jerry_value_t stack_str = jerry_create_string ((const jerry_char_t *) "stack");
    jerry_value_t backtrace_val = jerry_get_property (error_value, stack_str);
    jerry_release_value (stack_str);

    if (jerry_value_is_array (backtrace_val))
    {
      uint32_t length = jerry_get_array_length (backtrace_val);

      /* This length should be enough. */
      if (length > 32)
      {
        length = 32;
      }

      for (uint32_t i = 0; i < length; i++)
      {
        jerry_value_t item_val = jerry_get_property_by_index (backtrace_val, i);

        if (jerry_value_is_string (item_val))
        {
          jerry_size_t str_size = jerry_get_utf8_string_size (item_val);

          if (str_size >= 256)
          {
            printf ("%6u: [Backtrace string too long]\n", i);
          }
          else
          {
            jerry_size_t string_end = jerry_string_to_utf8_char_buffer (item_val, err_str_buf, str_size);
            assert (string_end == str_size);
            err_str_buf[string_end] = 0;

            printf ("%6u: %s\n", i, err_str_buf);
          }
        }

        jerry_release_value (item_val);
      }
    }

    jerry_release_value (backtrace_val);
  }

  jerry_release_value (error_value);
} /* main_print_unhandled_exception */

/**
 * Runs the source code received by jerry_debugger_wait_for_client_source.
 *
 * @return result fo the source code execution
 */
jerry_value_t
main_wait_for_source_callback (const jerry_char_t *resource_name_p, /**< resource name */
                               size_t resource_name_size, /**< size of resource name */
                               const jerry_char_t *source_p, /**< source code */
                               size_t source_size, /**< source code size */
                               void *user_p) /**< user pointer */
{
  (void) user_p; /* unused */

  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_HAS_RESOURCE;
  parse_options.resource_name = jerry_create_string_sz (resource_name_p, (jerry_size_t) resource_name_size);

  jerry_value_t ret_val = jerry_parse (source_p, source_size, &parse_options);

  jerry_release_value (parse_options.resource_name);

  if (!jerry_value_is_error (ret_val))
  {
    jerry_value_t func_val = ret_val;
    ret_val = jerry_run (func_val);
    jerry_release_value (func_val);
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

  jerry_value_t abort_value = jerry_get_value_from_error (value, false);

  if (!jerry_value_is_string (abort_value))
  {
    jerry_release_value (abort_value);
    return false;
  }

  static const char restart_str[] = "r353t";

  jerry_size_t str_size = jerry_get_string_size (abort_value);
  bool is_reset = false;

  if (str_size == sizeof (restart_str) - 1)
  {
    JERRY_VLA (jerry_char_t, str_buf, str_size);
    jerry_string_to_char_buffer (abort_value, str_buf, str_size);

    is_reset = memcmp (restart_str, (char *) (str_buf), str_size) == 0;

    if (is_reset)
    {
      jerry_release_value (value);
    }
  }

  jerry_release_value (abort_value);
  return is_reset;
} /* main_is_value_reset */
