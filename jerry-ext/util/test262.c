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

#include "jerryscript-ext/test262.h"

#include <assert.h>
#include <string.h>

#include "jerryscript.h"

#include "jerryscript-ext/handlers.h"

/**
 * Register a method for the $262 object.
 */
static void
jerryx_test262_register_function (jerry_value_t test262_obj, /** $262 object */
                                  const char *name_p, /**< name of the function */
                                  jerry_external_handler_t handler_p) /**< function callback */
{
  jerry_value_t function_val = jerry_function_external (handler_p);
  jerry_value_t result_val = jerry_object_set_sz (test262_obj, name_p, function_val);
  jerry_value_free (function_val);

  assert (!jerry_value_is_exception (result_val));
  jerry_value_free (result_val);
} /* jerryx_test262_register_function */

/**
 * $262.detachArrayBuffer
 *
 * A function which implements the DetachArrayBuffer abstract operation
 *
 * @return null value - if success
 *         value marked with error flag - otherwise
 */
static jerry_value_t
jerryx_test262_detach_array_buffer (const jerry_call_info_t *call_info_p, /**< call information */
                                    const jerry_value_t args_p[], /**< function arguments */
                                    const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */

  if (args_cnt < 1 || !jerry_value_is_arraybuffer (args_p[0]))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, "Expected an ArrayBuffer object");
  }

  /* TODO: support the optional 'key' argument */

  return jerry_arraybuffer_detach (args_p[0]);
} /* jerryx_test262_detach_array_buffer */

/**
 * $262.evalScript
 *
 * A function which accepts a string value as its first argument and executes it
 *
 * @return completion of the script parsing and execution.
 */
static jerry_value_t
jerryx_test262_eval_script (const jerry_call_info_t *call_info_p, /**< call information */
                            const jerry_value_t args_p[], /**< function arguments */
                            const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */

  if (args_cnt < 1 || !jerry_value_is_string (args_p[0]))
  {
    return jerry_throw_sz (JERRY_ERROR_TYPE, "Expected a string");
  }

  jerry_value_t ret_value = jerry_parse_value (args_p[0], NULL);

  if (!jerry_value_is_exception (ret_value))
  {
    jerry_value_t func_val = ret_value;
    ret_value = jerry_run (func_val);
    jerry_value_free (func_val);
  }

  return ret_value;
} /* jerryx_test262_eval_script */

static jerry_value_t jerryx_test262_create (jerry_value_t global_obj);

/**
 * $262.createRealm
 *
 * A function which creates a new realm object, and returns a newly created $262 object
 *
 * @return a new $262 object
 */
static jerry_value_t
jerryx_test262_create_realm (const jerry_call_info_t *call_info_p, /**< call information */
                             const jerry_value_t args_p[], /**< function arguments */
                             const jerry_length_t args_cnt) /**< number of function arguments */
{
  (void) call_info_p; /* unused */
  (void) args_p; /* unused */
  (void) args_cnt; /* unused */

  jerry_value_t realm_object = jerry_realm ();
  jerry_value_t previous_realm = jerry_set_realm (realm_object);
  assert (!jerry_value_is_exception (previous_realm));

  jerry_value_t test262_object = jerryx_test262_create (realm_object);
  jerry_set_realm (previous_realm);
  jerry_value_free (realm_object);

  return test262_object;
} /* jerryx_test262_create_realm */

/**
 * Create a new $262 object
 *
 * @return a new $262 object
 */
static jerry_value_t
jerryx_test262_create (jerry_value_t global_obj) /**< global object */
{
  jerry_value_t test262_object = jerry_object ();

  jerryx_test262_register_function (test262_object, "detachArrayBuffer", jerryx_test262_detach_array_buffer);
  jerryx_test262_register_function (test262_object, "evalScript", jerryx_test262_eval_script);
  jerryx_test262_register_function (test262_object, "createRealm", jerryx_test262_create_realm);
  jerryx_test262_register_function (test262_object, "gc", jerryx_handler_gc);

  jerry_value_t result = jerry_object_set_sz (test262_object, "global", global_obj);
  assert (!jerry_value_is_exception (result));
  jerry_value_free (result);

  return test262_object;
} /* create_test262 */

/**
 * Add a new test262 object to the current global object.
 */
void
jerryx_test262_register (void)
{
  jerry_value_t global_obj = jerry_current_realm ();
  jerry_value_t test262_obj = jerryx_test262_create (global_obj);

  jerry_value_t result = jerry_object_set_sz (global_obj, "$262", test262_obj);
  assert (!jerry_value_is_exception (result));

  jerry_value_free (result);
  jerry_value_free (test262_obj);
  jerry_value_free (global_obj);
} /* jerryx_test262_register */
