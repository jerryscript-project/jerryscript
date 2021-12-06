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

/**
 * Get the source name (usually a file name) of the currently executed script or the given function object
 *
 * Note: returned value must be freed with jerry_value_free, when it is no longer needed
 *
 * @return JS string constructed from
 *         - the currently executed function object's source name, if the given value is undefined
 *         - source name of the function object, if the given value is a function object
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
