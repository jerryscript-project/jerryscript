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

#include "jerryscript-port.h"

/**
 * Default implementation of jerry_port_track_promise_rejection.
 * Prints the reason of the unhandled rejections.
 */
void
jerry_port_track_promise_rejection (const jerry_value_t promise, /**< rejected promise */
                                    const jerry_promise_rejection_operation_t operation) /**< operation */
{
  (void) operation; /* unused */
  const jerry_size_t max_allowed_size = 5 * 1024 - 1;

  jerry_value_t reason = jerry_get_promise_result (promise);
  jerry_value_t reason_to_string = jerry_value_to_string (reason);
  jerry_release_value (reason);

  jerry_length_t end_pos = jerry_get_utf8_string_length (reason_to_string);
  jerry_size_t string_size = jerry_get_utf8_string_size (reason_to_string);
  jerry_size_t buffer_size = (string_size < max_allowed_size) ? string_size : max_allowed_size;

  JERRY_VLA (jerry_char_t, str_buf_p, buffer_size + 1);
  jerry_size_t copied = jerry_substring_to_utf8_char_buffer (reason_to_string,
                                                             0,
                                                             end_pos,
                                                             str_buf_p,
                                                             buffer_size);
  str_buf_p[copied] = '\0';

  jerry_release_value (reason_to_string);

  jerry_port_log (JERRY_LOG_LEVEL_WARNING, "Uncaught (in promise) %s\n", str_buf_p);
} /* jerry_port_track_promise_rejection */
