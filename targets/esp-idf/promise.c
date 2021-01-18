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
 * Track unhandled promise rejections.
 *
 * Note:
 *      This port function is called by jerry-core when JERRY_BUILTIN_PROMISE
 *      is enabled.
 *
 * @param promise rejected promise
 * @param operation HostPromiseRejectionTracker operation
 */
void
jerry_port_track_promise_rejection (const jerry_value_t promise,
                                    const jerry_promise_rejection_operation_t operation)
{
  (void) operation; /* unused */

  jerry_value_t reason = jerry_get_promise_result (promise);
  jerry_value_t reason_to_string = jerry_value_to_string (reason);
  jerry_size_t req_sz = jerry_get_utf8_string_size (reason_to_string);
  jerry_char_t str_buf_p[req_sz + 1];
  jerry_string_to_utf8_char_buffer (reason_to_string, str_buf_p, req_sz);
  str_buf_p[req_sz] = '\0';

  jerry_release_value (reason_to_string);
  jerry_release_value (reason);

  jerry_port_log (JERRY_LOG_LEVEL_WARNING, "Uncaught (in promise) %s\n", str_buf_p);
} /* jerry_port_track_promise_rejection */
