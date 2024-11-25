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

#include "jerryscript-ext/handlers.h"

#include "jerryscript-core.h"
#include "jerryscript-debugger.h"
#include "jerryscript-port.h"

#include "jerryscript-ext/print.h"

/**
 * Provide a 'print' implementation for scripts.
 *
 * The routine converts all of its arguments to strings and outputs them
 * by using jerry_port_print_buffer.
 *
 * The NULL character is output as "\u0000", other characters are output
 * bytewise.
 *
 * Note:
 *      This implementation does not use standard C `printf` to print its
 *      output. This allows more flexibility but also extends the core
 *      JerryScript engine port API. Applications that want to use
 *      `jerryx_handler_print` must ensure that their port implementation also
 *      provides `jerry_port_print_buffer`.
 *
 * @return undefined - if all arguments could be converted to strings,
 *         error - otherwise.
 */
jerry_value_t
jerryx_handler_print (const jerry_call_info_t *call_info_p, /**< call information */
                      const jerry_value_t args_p[], /**< function arguments */
                      const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */

  for (jerry_length_t index = 0; index < args_cnt; index++)
  {
    if (index > 0)
    {
      jerryx_print_buffer (JERRY_ZSTR_ARG (" "));
    }

    jerry_value_t result = jerryx_print_value (args_p[index]);

    if (jerry_value_is_exception (result))
    {
      return result;
    }
  }

  jerryx_print_buffer (JERRY_ZSTR_ARG ("\n"));
  return jerry_undefined ();
} /* jerryx_handler_print */

/**
 * Hard assert for scripts. The routine calls jerry_port_fatal on assertion failure.
 *
 * Notes:
 *  * If the `JERRY_FEATURE_LINE_INFO` runtime feature is enabled (build option: `JERRY_LINE_INFO`)
 *    a backtrace is also printed out.
 *
 * @return true - if only one argument was passed and that argument was a boolean true.
 *         Note that the function does not return otherwise.
 */
jerry_value_t
jerryx_handler_assert (const jerry_call_info_t *call_info_p, /**< call information */
                       const jerry_value_t args_p[], /**< function arguments */
                       const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */

  if (args_cnt == 1 && jerry_value_is_true (args_p[0]))
  {
    return jerry_boolean (true);
  }

  /* Assert failed, print a bit of JS backtrace */
  jerry_log (JERRY_LOG_LEVEL_ERROR, "Script Error: assertion failed\n");
  jerryx_print_backtrace (5);

  jerry_port_fatal (JERRY_FATAL_FAILED_ASSERTION);
} /* jerryx_handler_assert */

/**
 * Expose garbage collector to scripts.
 *
 * @return undefined.
 */
jerry_value_t
jerryx_handler_gc (const jerry_call_info_t *call_info_p, /**< call information */
                   const jerry_value_t args_p[], /**< function arguments */
                   const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */

  jerry_gc_mode_t mode =
    ((args_cnt > 0 && jerry_value_to_boolean (args_p[0])) ? JERRY_GC_PRESSURE_HIGH : JERRY_GC_PRESSURE_LOW);

  jerry_heap_gc (mode);
  return jerry_undefined ();
} /* jerryx_handler_gc */

/**
 * Get the resource name (usually a file name) of the currently executed script or the given function object
 *
 * Note: returned value must be freed with jerry_value_free, when it is no longer needed
 *
 * @return JS string constructed from
 *         - the currently executed function object's resource name, if the given value is undefined
 *         - resource name of the function object, if the given value is a function object
 *         - "<anonymous>", otherwise
 */
jerry_value_t
jerryx_handler_source_name (const jerry_call_info_t *call_info_p, /**< call information */
                            const jerry_value_t args_p[], /**< function arguments */
                            const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */

  jerry_value_t undefined_value = jerry_undefined ();
  jerry_value_t source_name = jerry_source_name (args_cnt > 0 ? args_p[0] : undefined_value);
  jerry_value_free (undefined_value);

  return source_name;
} /* jerryx_handler_source_name */

/**
 * Create a new realm.
 *
 * @return new Realm object
 */
jerry_value_t
jerryx_handler_create_realm (const jerry_call_info_t *call_info_p, /**< call information */
                             const jerry_value_t args_p[], /**< function arguments */
                             const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */
  (void) args_p; /* unused */
  (void) args_cnt; /* unused */
  return jerry_realm ();
} /* jerryx_handler_create_realm */

/**
 * Handler for unhandled promise rejection events.
 */
void
jerryx_handler_promise_reject (jerry_promise_event_type_t event_type, /**< event type */
                               const jerry_value_t object, /**< target object */
                               const jerry_value_t value, /**< optional argument */
                               void *user_p) /**< user pointer passed to the callback */
{
  (void) value;
  (void) user_p;

  if (event_type != JERRY_PROMISE_EVENT_REJECT_WITHOUT_HANDLER)
  {
    return;
  }

  jerry_value_t result = jerry_promise_result (object);
  jerryx_print_unhandled_rejection (result);

  jerry_value_free (result);
} /* jerryx_handler_promise_reject */

/**
 * Runs the source code received by jerry_debugger_wait_for_client_source.
 *
 * @return result fo the source code execution
 */
jerry_value_t
jerryx_handler_source_received (const jerry_char_t *source_name_p, /**< resource name */
                                size_t source_name_size, /**< size of resource name */
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
} /* jerryx_handler_source_received */
