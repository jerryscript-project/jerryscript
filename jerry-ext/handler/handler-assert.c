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
#include "jerryscript-port.h"

/**
 * Hard assert for scripts. The routine calls jerry_port_fatal on assertion failure.
 *
 * @return true - if only one argument was passed and that argument was a boolean true.
 *         Note that the function does not return otherwise.
 */
jerry_value_t
jerryx_handler_assert_fatal (const jerry_value_t func_obj_val, /**< function object */
                             const jerry_value_t this_p, /**< this arg */
                             const jerry_value_t args_p[], /**< function arguments */
                             const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) func_obj_val; /* unused */
  (void) this_p; /* unused */

  if (args_cnt == 1
      && jerry_value_is_boolean (args_p[0])
      && jerry_get_boolean_value (args_p[0]))
  {
    return jerry_create_boolean (true);
  }

  jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Script Error: assertion failed\n");
  jerry_port_fatal (ERR_FAILED_INTERNAL_ASSERTION);
} /* jerryx_handler_assert_fatal */

/**
 * Soft assert for scripts. The routine throws an error on assertion failure.
 *
 * @return true - if only one argument was passed and that argument was a boolean true.
 *         error - otherwise.
 */
jerry_value_t
jerryx_handler_assert_throw (const jerry_value_t func_obj_val, /**< function object */
                             const jerry_value_t this_p, /**< this arg */
                             const jerry_value_t args_p[], /**< function arguments */
                             const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) func_obj_val; /* unused */
  (void) this_p; /* unused */

  if (args_cnt == 1
      && jerry_value_is_boolean (args_p[0])
      && jerry_get_boolean_value (args_p[0]))
  {
    return jerry_create_boolean (true);
  }

  return jerry_create_error (JERRY_ERROR_COMMON, (jerry_char_t *) "assertion failed");
} /* jerryx_handler_assert_throw */

/**
 * An alias to `jerryx_handler_assert_fatal`.
 *
 * @return true - if only one argument was passed and that argument was a boolean true.
 *         Note that the function does not return otherwise.
 */
jerry_value_t
jerryx_handler_assert (const jerry_value_t func_obj_val, /**< function object */
                       const jerry_value_t this_p, /**< this arg */
                       const jerry_value_t args_p[], /**< function arguments */
                       const jerry_length_t args_cnt) /**< number of function arguments */
{
  return jerryx_handler_assert_fatal (func_obj_val, this_p, args_p, args_cnt);
} /* jerryx_handler_assert */
