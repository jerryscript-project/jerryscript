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

#include "jerryscript-ext/handler.h"
#include "debugger.h"

/**
 * Provide a 'print' implementation for scripts.
 *
 * The routine converts all of its arguments to strings and outputs them
 * char-by-char using jerryx_port_handler_print_char.
 *
 * The NUL character is output as "\u0000", other characters are output
 * bytewise.
 *
 * Note:
 *      This implementation does not use standard C `printf` to print its
 *      output. This allows more flexibility but also extends the core
 *      JerryScript engine port API. Applications that want to use
 *      `jerryx_handler_print` must ensure that their port implementation also
 *      provides `jerryx_port_handler_print_char`.
 *
 * @return undefined - if all arguments could be converted to strings,
 *         error - otherwise.
 */
jerry_value_t
jerryx_handler_print (const jerry_value_t func_obj_val, /**< function object */
                      const jerry_value_t this_p, /**< this arg */
                      const jerry_value_t args_p[], /**< function arguments */
                      const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) func_obj_val; /* unused */
  (void) this_p; /* unused */

  static const char *null_str = "\\u0000";

  jerry_value_t ret_val = jerry_create_undefined ();

  for (jerry_length_t arg_index = 0;
       jerry_value_is_undefined (ret_val) && arg_index < args_cnt;
       arg_index++)
  {
    jerry_value_t str_val = jerry_value_to_string (args_p[arg_index]);

    if (!jerry_value_has_error_flag (str_val))
    {
      if (arg_index != 0)
      {
        jerryx_port_handler_print_char (' ');
      }

      jerry_size_t substr_size;
      jerry_length_t substr_pos = 0;
      jerry_char_t substr_buf[256];

      while ((substr_size = jerry_substring_to_char_buffer (str_val,
                                                            substr_pos,
                                                            substr_pos + 256,
                                                            substr_buf,
                                                            256)) != 0)
      {
#ifdef JERRY_DEBUGGER
        jerry_debugger_send_output (substr_buf, substr_size, JERRY_DEBUGGER_OUTPUT_OK);
#endif /* JERRY_DEBUGGER */
        for (jerry_size_t chr_index = 0; chr_index < substr_size; chr_index++)
        {
          char chr = (char) substr_buf[chr_index];
          if (chr == '\0')
          {
            for (jerry_size_t null_index = 0; null_str[null_index] != 0; null_index++)
            {
              jerryx_port_handler_print_char (null_str[null_index]);
            }
          }
          else
          {
            jerryx_port_handler_print_char (chr);
          }
        }

        substr_pos += substr_size;
      }

      jerry_release_value (str_val);
    }
    else
    {
      ret_val = str_val;
    }
  }

  jerryx_port_handler_print_char ('\n');

  return ret_val;
} /* jerryx_handler_print */
