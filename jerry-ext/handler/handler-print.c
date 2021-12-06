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

#include "jerryscript-debugger.h"
#include "jerryscript-port.h"

#include "jerryscript-ext/handler.h"

/**
 * Provide a 'print' implementation for scripts.
 *
 * The routine converts all of its arguments to strings and outputs them
 * char-by-char using jerry_port_print_char.
 *
 * The NUL character is output as "\u0000", other characters are output
 * bytewise.
 *
 * Note:
 *      This implementation does not use standard C `printf` to print its
 *      output. This allows more flexibility but also extends the core
 *      JerryScript engine port API. Applications that want to use
 *      `jerryx_handler_print` must ensure that their port implementation also
 *      provides `jerry_port_print_char`.
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

  jerry_value_t ret_val = jerry_undefined ();

  for (jerry_length_t arg_index = 0; arg_index < args_cnt; arg_index++)
  {
    jerry_value_t str_val;

    if (jerry_value_is_symbol (args_p[arg_index]))
    {
      str_val = jerry_symbol_descriptive_string (args_p[arg_index]);
    }
    else
    {
      str_val = jerry_value_to_string (args_p[arg_index]);
    }

    if (jerry_value_is_exception (str_val))
    {
      /* There is no need to free the undefined value. */
      ret_val = str_val;
      break;
    }

    if (arg_index > 0)
    {
      jerry_port_print_char (' ');
    }

    jerry_string_print (str_val);
    jerry_value_free (str_val);
  }

  jerry_port_print_char ('\n');
  return ret_val;
} /* jerryx_handler_print */
