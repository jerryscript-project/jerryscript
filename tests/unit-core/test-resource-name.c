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

#include "config.h"
#include "jerryscript.h"
#include "test-common.h"

static jerry_value_t
resource_name_handler (const jerry_value_t function_obj, /**< function object */
                       const jerry_value_t this_val, /**< this value */
                       const jerry_value_t args_p[], /**< argument list */
                       const jerry_length_t args_count) /**< argument count */
{
  (void) function_obj;
  (void) this_val;

  jerry_value_t undefined_value = jerry_create_undefined ();
  jerry_value_t resource_name = jerry_get_resource_name (args_count > 0 ? args_p[0] : undefined_value);
  jerry_release_value (undefined_value);

  return resource_name;
} /* resource_name_handler */

int
main (void)
{
  TEST_INIT ();

  if (!jerry_is_feature_enabled (JERRY_FEATURE_LINE_INFO))
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Line info support is disabled!\n");
    return 0;
  }

  jerry_init (JERRY_INIT_EMPTY);

  jerry_value_t global = jerry_get_global_object ();

  /* Register the "resourceName" method. */
  {
    jerry_value_t func = jerry_create_external_function (resource_name_handler);
    jerry_value_t name = jerry_create_string ((const jerry_char_t *) "resourceName");
    jerry_value_t result = jerry_set_property (global, name, func);
    jerry_release_value (result);
    jerry_release_value (name);
    jerry_release_value (func);
  }

  jerry_release_value (global);

  const char *source_1 = ("function f1 () {\n"
                          "  if (resourceName() !== 'demo1.js') return false; \n"
                          "  if (resourceName(f1) !== 'demo1.js') return false; \n"
                          "  if (resourceName(5) !== '<anonymous>') return false; \n"
                          "  return f1; \n"
                          "} \n"
                          "f1();");
  const char *resource_1 = "demo1.js";

  jerry_value_t program = jerry_parse ((const jerry_char_t *) resource_1,
                                       strlen (resource_1),
                                       (const jerry_char_t *) source_1,
                                       strlen (source_1),
                                       JERRY_PARSE_NO_OPTS);
  TEST_ASSERT (!jerry_value_is_error (program));

  jerry_value_t run_result = jerry_run (program);
  TEST_ASSERT (!jerry_value_is_error (run_result));
  TEST_ASSERT (jerry_value_is_object (run_result));

  jerry_value_t resource_value = jerry_get_resource_name (run_result);
  jerry_value_t resource1_name_value = jerry_create_string ((const jerry_char_t *) resource_1);
  TEST_ASSERT (jerry_binary_operation (JERRY_BIN_OP_STRICT_EQUAL, resource_value, resource1_name_value));
  jerry_release_value (resource1_name_value);
  jerry_release_value (resource_value);

  jerry_release_value (run_result);
  jerry_release_value (program);

  const char *source_2 = ("function f2 () { \n"
                          "  if (resourceName() !== 'demo2.js') return false; \n"
                          "  if (resourceName(f2) !== 'demo2.js') return false; \n"
                          "  if (resourceName(f1) !== 'demo1.js') return false; \n"
                          "  if (resourceName(Object.prototype) !== '<anonymous>') return false; \n"
                          "  if (resourceName(Function) !== '<anonymous>') return false; \n"
                          "  return f2; \n"
                          "} \n"
                          "f2(); \n");
  const char *resource_2 = "demo2.js";


  program = jerry_parse ((const jerry_char_t *) resource_2,
                         strlen (resource_2),
                         (const jerry_char_t *) source_2,
                         strlen (source_2),
                         JERRY_PARSE_NO_OPTS);
  TEST_ASSERT (!jerry_value_is_error (program));

  run_result = jerry_run (program);
  TEST_ASSERT (!jerry_value_is_error (run_result));
  TEST_ASSERT (jerry_value_is_object (run_result));

  resource_value = jerry_get_resource_name (run_result);
  jerry_value_t resource2_name_value = jerry_create_string ((const jerry_char_t *) resource_2);
  TEST_ASSERT (jerry_binary_operation (JERRY_BIN_OP_STRICT_EQUAL, resource_value, resource2_name_value));
  jerry_release_value (resource2_name_value);
  jerry_release_value (resource_value);

  jerry_release_value (run_result);
  jerry_release_value (program);

  jerry_cleanup ();

  return 0;
} /* main */
