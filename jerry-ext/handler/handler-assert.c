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

#include <inttypes.h>

#include "jerryscript-port.h"

#include "jerryscript-ext/handler.h"

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
jerryx_handler_assert_fatal (const jerry_call_info_t *call_info_p, /**< call information */
                             const jerry_value_t args_p[], /**< function arguments */
                             const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */

  if (args_cnt == 1 && jerry_value_is_true (args_p[0]))
  {
    return jerry_boolean (true);
  }

  /* Assert failed, print a bit of JS backtrace */
  jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Script Error: assertion failed\n");

  if (jerry_feature_enabled (JERRY_FEATURE_LINE_INFO))
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Script backtrace (top 5):\n");

    /* If the line info feature is disabled an empty array will be returned. */
    jerry_value_t backtrace_array = jerry_backtrace (5);
    uint32_t array_length = jerry_array_length (backtrace_array);

    for (uint32_t idx = 0; idx < array_length; idx++)
    {
      jerry_value_t property = jerry_object_get_index (backtrace_array, idx);

      /* On some systems the uint32_t values can't be printed with "%u" and
       * on some systems it can be printed. To avoid differences in the uint32_t typdef
       * The "PRIu32" macro is used to correctly add the formatter.
       */
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, " %" PRIu32 ": ", idx);

      jerry_char_t string_buffer[256];
      const jerry_length_t buffer_size = (jerry_length_t) (sizeof (string_buffer) - 1);

      jerry_size_t copied_bytes = jerry_string_to_buffer (property, JERRY_ENCODING_UTF8, string_buffer, buffer_size);
      string_buffer[copied_bytes] = '\0';

      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "%s\n", string_buffer);
      jerry_value_free (property);
    }

    jerry_value_free (backtrace_array);
  }

  jerry_port_fatal (ERR_FAILED_INTERNAL_ASSERTION);
} /* jerryx_handler_assert_fatal */

/**
 * Soft assert for scripts. The routine throws an error on assertion failure.
 *
 * @return true - if only one argument was passed and that argument was a boolean true.
 *         error - otherwise.
 */
jerry_value_t
jerryx_handler_assert_throw (const jerry_call_info_t *call_info_p, /**< call information */
                             const jerry_value_t args_p[], /**< function arguments */
                             const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */

  if (args_cnt == 1 && jerry_value_is_true (args_p[0]))
  {
    return jerry_boolean (true);
  }

  return jerry_throw_sz (JERRY_ERROR_COMMON, "assertion failed");
} /* jerryx_handler_assert_throw */

/**
 * An alias to `jerryx_handler_assert_fatal`.
 *
 * @return true - if only one argument was passed and that argument was a boolean true.
 *         Note that the function does not return otherwise.
 */
jerry_value_t
jerryx_handler_assert (const jerry_call_info_t *call_info_p, /**< call information */
                       const jerry_value_t args_p[], /**< function arguments */
                       const jerry_length_t args_cnt) /**< number of function arguments */
{
  return jerryx_handler_assert_fatal (call_info_p, args_p, args_cnt);
} /* jerryx_handler_assert */
