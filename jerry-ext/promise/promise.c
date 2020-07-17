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
#include "jerryscript-ext/promise.h"
#include "jext-common.h"
#include "jerryscript-port.h"

/**
 * Provide a 'HostPromiseRejectionTracker' implementation for scripts.
 *
 * See also: ECMAScript 11, 25.6.1.9
 */
void
jerryx_promise_rejection_tracker (jerry_value_t promise, /**< promise object */
                                  jerry_value_t reason) /**< reason of rejection */
{
  (void) promise; /* unused */

  jerry_value_t unchaught_str = jerry_create_string ((const jerry_char_t *) "Uncaught (in promise):");
  jerry_value_t args[] = { unchaught_str, reason };

  jerry_value_t result = jerryx_handler_print_helper (args, sizeof (args) / sizeof (args[0]));

  if (jerry_value_is_error (result))
  {
    result = jerry_get_value_from_error (result, true);
  }

  jerry_release_value (result);
  jerry_release_value (unchaught_str);
} /* jerryx_promise_rejection_tracker */
