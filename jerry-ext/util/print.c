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

#include "jerryscript-ext/print.h"

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "jerryscript-core.h"
#include "jerryscript-debugger.h"
#include "jerryscript-port.h"

/**
 * Print buffer size
 */
#define JERRYX_PRINT_BUFFER_SIZE 64

/**
 * Max line size that will be printed on a Syntax Error
 */
#define JERRYX_SYNTAX_ERROR_MAX_LINE_LENGTH 256

/**
 * Struct for buffering print output
 */
typedef struct
{
  jerry_size_t index; /**< write index */
  jerry_char_t data[JERRYX_PRINT_BUFFER_SIZE]; /**< buffer data */
} jerryx_print_buffer_t;

/**
 * Callback used by jerryx_print_value to batch written characters and print them in bulk.
 * NULL bytes are escaped and written as '\u0000'.
 *
 * @param value:  encoded byte value
 * @param user_p: user pointer
 */
static void
jerryx_buffered_print (uint32_t value, void *user_p)
{
  jerryx_print_buffer_t *buffer_p = (jerryx_print_buffer_t *) user_p;

  if (value == '\0')
  {
    jerryx_print_buffer (buffer_p->data, buffer_p->index);
    buffer_p->index = 0;

    jerryx_print_buffer (JERRY_ZSTR_ARG ("\\u0000"));
    return;
  }

  assert (value <= UINT8_MAX);
  buffer_p->data[buffer_p->index++] = (uint8_t) value;

  if (buffer_p->index >= JERRYX_PRINT_BUFFER_SIZE)
  {
    jerryx_print_buffer (buffer_p->data, buffer_p->index);
    buffer_p->index = 0;
  }
} /* jerryx_buffered_print */

/**
 * Convert a value to string and print it to standard output.
 * NULL characters are escaped to "\u0000", other characters are output as-is.
 *
 * @param value: input value
 */
jerry_value_t
jerryx_print_value (const jerry_value_t value)
{
  jerry_value_t string;

  if (jerry_value_is_symbol (value))
  {
    string = jerry_symbol_descriptive_string (value);
  }
  else
  {
    string = jerry_value_to_string (value);

    if (jerry_value_is_exception (string))
    {
      return string;
    }
  }

  jerryx_print_buffer_t buffer;
  buffer.index = 0;

  jerry_string_iterate (string, JERRY_ENCODING_UTF8, &jerryx_buffered_print, &buffer);
  jerry_value_free (string);

  jerryx_print_buffer (buffer.data, buffer.index);

  return jerry_undefined ();
} /* jerryx_print */

/**
 * Print a buffer to standard output, also sending it to the debugger, if connected.
 *
 * @param buffer_p: inptut string buffer
 * @param buffer_size: size of the string
 */
void
jerryx_print_buffer (const jerry_char_t *buffer_p, jerry_size_t buffer_size)
{
  jerry_port_print_buffer (buffer_p, buffer_size);
#if JERRY_DEBUGGER
  jerry_debugger_send_output (buffer_p, buffer_size);
#endif /* JERRY_DEBUGGER */
} /* jerryx_print_buffer */

/**
 * Print backtrace as log messages up to a specific depth.
 *
 * @param depth: backtrace depth
 */
void
jerryx_print_backtrace (unsigned depth)
{
  if (!jerry_feature_enabled (JERRY_FEATURE_LINE_INFO))
  {
    return;
  }

  jerry_log (JERRY_LOG_LEVEL_ERROR, "Script backtrace (top %u):\n", depth);

  jerry_value_t backtrace_array = jerry_backtrace (depth);
  unsigned array_length = jerry_array_length (backtrace_array);

  for (unsigned idx = 0; idx < array_length; idx++)
  {
    jerry_value_t property = jerry_object_get_index (backtrace_array, idx);

    jerry_char_t buffer[JERRYX_PRINT_BUFFER_SIZE];

    jerry_size_t copied = jerry_string_to_buffer (property, JERRY_ENCODING_UTF8, buffer, JERRYX_PRINT_BUFFER_SIZE - 1);
    buffer[copied] = '\0';

    jerry_log (JERRY_LOG_LEVEL_ERROR, " %u: %s\n", idx, buffer);
    jerry_value_free (property);
  }

  jerry_value_free (backtrace_array);
} /* jerryx_print_backtrace */

/**
 * Print an unhandled exception value
 *
 * The function will take ownership of the value, and free it.
 *
 * @param exception: unhandled exception value
 */
void
jerryx_print_unhandled_exception (jerry_value_t exception) /**< exception value */
{
  assert (jerry_value_is_exception (exception));
  jerry_value_t value = jerry_exception_value (exception, true);

  JERRY_VLA (jerry_char_t, buffer_p, JERRYX_PRINT_BUFFER_SIZE);

  jerry_value_t string = jerry_value_to_string (value);

  jerry_size_t copied = jerry_string_to_buffer (string, JERRY_ENCODING_UTF8, buffer_p, JERRYX_PRINT_BUFFER_SIZE - 1);
  buffer_p[copied] = '\0';

  if (jerry_feature_enabled (JERRY_FEATURE_ERROR_MESSAGES) && jerry_error_type (value) == JERRY_ERROR_SYNTAX)
  {
    jerry_char_t *string_end_p = buffer_p + copied;
    jerry_size_t err_line = 0;
    jerry_size_t err_col = 0;
    char *path_str_p = NULL;
    char *path_str_end_p = NULL;

    /* 1. parse column and line information */
    for (jerry_char_t *current_p = buffer_p; current_p < string_end_p; current_p++)
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

        path_str_end_p = (char *) current_p;

        if (current_p == string_end_p)
        {
          break;
        }

        err_line = (unsigned int) strtol ((char *) current_p + 1, (char **) &current_p, 10);

        if (current_p == string_end_p)
        {
          break;
        }

        err_col = (unsigned int) strtol ((char *) current_p + 1, NULL, 10);
        break;
      }
    } /* for */

    if (err_line > 0 && err_col > 0 && err_col < JERRYX_SYNTAX_ERROR_MAX_LINE_LENGTH)
    {
      /* Temporarily modify the error message, so we can use the path. */
      *path_str_end_p = '\0';

      jerry_size_t source_size;
      jerry_char_t *source_p = jerry_port_source_read (path_str_p, &source_size);

      /* Revert the error message. */
      *path_str_end_p = ':';

      if (source_p != NULL)
      {
        uint32_t curr_line = 1;
        jerry_size_t pos = 0;

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
             (char_count < JERRYX_SYNTAX_ERROR_MAX_LINE_LENGTH) && (pos < source_size) && (source_p[pos] != '\n');
             char_count++, pos++)
        {
          jerry_log (JERRY_LOG_LEVEL_ERROR, "%c", source_p[pos]);
        }

        jerry_log (JERRY_LOG_LEVEL_ERROR, "\n");
        jerry_port_source_free (source_p);

        while (--err_col)
        {
          jerry_log (JERRY_LOG_LEVEL_ERROR, "~");
        }

        jerry_log (JERRY_LOG_LEVEL_ERROR, "^\n\n");
      }
    }
  }

  jerry_log (JERRY_LOG_LEVEL_ERROR, "Unhandled exception: %s\n", buffer_p);
  jerry_value_free (string);

  if (jerry_value_is_object (value))
  {
    jerry_value_t backtrace_val = jerry_object_get_sz (value, "stack");

    if (jerry_value_is_array (backtrace_val))
    {
      uint32_t length = jerry_array_length (backtrace_val);

      /* This length should be enough. */
      if (length > 32)
      {
        length = 32;
      }

      for (unsigned i = 0; i < length; i++)
      {
        jerry_value_t item_val = jerry_object_get_index (backtrace_val, i);

        if (jerry_value_is_string (item_val))
        {
          copied = jerry_string_to_buffer (item_val, JERRY_ENCODING_UTF8, buffer_p, JERRYX_PRINT_BUFFER_SIZE - 1);
          buffer_p[copied] = '\0';

          jerry_log (JERRY_LOG_LEVEL_ERROR, " %u: %s\n", i, buffer_p);
        }

        jerry_value_free (item_val);
      }
    }

    jerry_value_free (backtrace_val);
  }

  jerry_value_free (value);
} /* jerryx_print_unhandled_exception */

/**
 * Print unhandled promise rejection.
 *
 * @param result: promise rejection result
 */
void
jerryx_print_unhandled_rejection (jerry_value_t result) /**< result value */
{
  jerry_value_t reason = jerry_value_to_string (result);

  if (!jerry_value_is_exception (reason))
  {
    JERRY_VLA (jerry_char_t, buffer_p, JERRYX_PRINT_BUFFER_SIZE);
    jerry_size_t copied = jerry_string_to_buffer (reason, JERRY_ENCODING_UTF8, buffer_p, JERRYX_PRINT_BUFFER_SIZE - 1);
    buffer_p[copied] = '\0';

    jerry_log (JERRY_LOG_LEVEL_WARNING, "Uncaught Promise rejection: %s\n", buffer_p);
  }
  else
  {
    jerry_log (JERRY_LOG_LEVEL_WARNING, "Uncaught Promise rejection: (reason cannot be converted to string)\n");
  }

  jerry_value_free (reason);
} /* jerryx_print_unhandled_rejection */
