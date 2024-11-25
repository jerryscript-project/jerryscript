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

#include "jerryscript-ext/repl.h"

#include <stdio.h>
#include <string.h>

#include "jerryscript-port.h"
#include "jerryscript.h"

#include "jerryscript-ext/print.h"

void
jerryx_repl (const jerry_char_t *prompt_p, jerry_size_t prompt_size)
{
  jerry_value_t result;

  while (true)
  {
    jerryx_print_buffer (prompt_p, prompt_size);
    fflush (stdout);

    jerry_size_t length;
    jerry_char_t *line_p = jerry_port_line_read (&length);

    if (line_p == NULL)
    {
      jerryx_print_buffer (JERRY_ZSTR_ARG ("\n"));
      return;
    }

    if (length == 0)
    {
      continue;
    }

    if (!jerry_validate_string (line_p, length, JERRY_ENCODING_UTF8))
    {
      jerry_port_line_free (line_p);
      result = jerry_throw_sz (JERRY_ERROR_SYNTAX, "Input is not a valid UTF-8 string");
      goto exception;
    }

    result = jerry_parse (line_p, length, NULL);
    jerry_port_line_free (line_p);

    if (jerry_value_is_exception (result))
    {
      goto exception;
    }

    jerry_value_t script = result;
    result = jerry_run (script);
    jerry_value_free (script);

    if (jerry_value_is_exception (result))
    {
      goto exception;
    }

    jerry_value_t print_result = jerryx_print_value (result);
    jerry_value_free (result);
    result = print_result;

    if (jerry_value_is_exception (result))
    {
      goto exception;
    }

    jerryx_print_buffer (JERRY_ZSTR_ARG ("\n"));

    jerry_value_free (result);
    result = jerry_run_jobs ();

    if (jerry_value_is_exception (result))
    {
      goto exception;
    }

    jerry_value_free (result);
    continue;

exception:
    jerryx_print_unhandled_exception (result);
  }
} /* jerryx_repl */
