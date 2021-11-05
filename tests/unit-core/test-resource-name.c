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

#include "jerryscript.h"

#include "config.h"
#include "test-common.h"

static jerry_value_t
resource_name_handler (const jerry_call_info_t *call_info_p, /**< call information */
                       const jerry_value_t args_p[], /**< argument list */
                       const jerry_length_t args_count) /**< argument count */
{
  (void) call_info_p;

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

  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_HAS_RESOURCE;

  const char *source_1 = ("function f1 () {\n"
                          "  if (resourceName() !== 'demo1.js') return false; \n"
                          "  if (resourceName(f1) !== 'demo1.js') return false; \n"
                          "  if (resourceName(5) !== '<anonymous>') return false; \n"
                          "  return f1; \n"
                          "} \n"
                          "f1();");

  parse_options.resource_name = jerry_create_string ((jerry_char_t *) "demo1.js");

  jerry_value_t program = jerry_parse ((const jerry_char_t *) source_1, strlen (source_1), &parse_options);
  TEST_ASSERT (!jerry_value_is_error (program));

  jerry_value_t run_result = jerry_run (program);
  TEST_ASSERT (!jerry_value_is_error (run_result));
  TEST_ASSERT (jerry_value_is_object (run_result));

  jerry_value_t resource_value = jerry_get_resource_name (run_result);
  jerry_value_t compare_result =
    jerry_binary_operation (JERRY_BIN_OP_STRICT_EQUAL, resource_value, parse_options.resource_name);
  TEST_ASSERT (jerry_value_is_true (compare_result));

  jerry_release_value (compare_result);
  jerry_release_value (resource_value);
  jerry_release_value (parse_options.resource_name);

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

  parse_options.resource_name = jerry_create_string ((const jerry_char_t *) "demo2.js");

  program = jerry_parse ((const jerry_char_t *) source_2, strlen (source_2), &parse_options);
  TEST_ASSERT (!jerry_value_is_error (program));

  run_result = jerry_run (program);
  TEST_ASSERT (!jerry_value_is_error (run_result));
  TEST_ASSERT (jerry_value_is_object (run_result));

  resource_value = jerry_get_resource_name (run_result);
  compare_result = jerry_binary_operation (JERRY_BIN_OP_STRICT_EQUAL, resource_value, parse_options.resource_name);
  TEST_ASSERT (jerry_value_is_true (compare_result));

  jerry_release_value (compare_result);
  jerry_release_value (resource_value);
  jerry_release_value (parse_options.resource_name);

  jerry_release_value (run_result);
  jerry_release_value (program);
  if (jerry_is_feature_enabled (JERRY_FEATURE_MODULE))
  {
    jerry_value_t anon = jerry_create_string ((const jerry_char_t *) "<anonymous>");
    const char *source_3 = "";

    parse_options.options = JERRY_PARSE_MODULE | JERRY_PARSE_HAS_RESOURCE;
    parse_options.resource_name = jerry_create_string ((const jerry_char_t *) "demo3.js");

    program = jerry_parse ((const jerry_char_t *) source_3, strlen (source_3), &parse_options);
    TEST_ASSERT (!jerry_value_is_error (program));

    resource_value = jerry_get_resource_name (program);
    compare_result = jerry_binary_operation (JERRY_BIN_OP_STRICT_EQUAL, resource_value, parse_options.resource_name);
    TEST_ASSERT (jerry_value_is_true (compare_result));

    jerry_release_value (compare_result);
    jerry_release_value (resource_value);

    run_result = jerry_module_link (program, NULL, NULL);
    TEST_ASSERT (!jerry_value_is_error (run_result));

    resource_value = jerry_get_resource_name (run_result);
    compare_result = jerry_binary_operation (JERRY_BIN_OP_STRICT_EQUAL, resource_value, anon);
    TEST_ASSERT (jerry_value_is_true (compare_result));

    jerry_release_value (compare_result);
    jerry_release_value (resource_value);
    jerry_release_value (run_result);

    run_result = jerry_module_evaluate (program);
    TEST_ASSERT (!jerry_value_is_error (run_result));

    resource_value = jerry_get_resource_name (run_result);
    compare_result = jerry_binary_operation (JERRY_BIN_OP_STRICT_EQUAL, resource_value, anon);
    TEST_ASSERT (jerry_value_is_true (compare_result));

    jerry_release_value (compare_result);
    jerry_release_value (resource_value);
    jerry_release_value (run_result);
    jerry_release_value (program);
    jerry_release_value (parse_options.resource_name);
  }
  const char *source_4 = ("function f(){} \n"
                          "f.bind().bind();");

  parse_options.options = JERRY_PARSE_HAS_RESOURCE;
  parse_options.resource_name = jerry_create_string ((jerry_char_t *) "demo4.js");

  program = jerry_parse ((const jerry_char_t *) source_4, strlen (source_4), &parse_options);
  TEST_ASSERT (!jerry_value_is_error (program));

  run_result = jerry_run (program);
  TEST_ASSERT (!jerry_value_is_error (run_result));
  TEST_ASSERT (jerry_value_is_object (run_result));

  resource_value = jerry_get_resource_name (run_result);
  compare_result = jerry_binary_operation (JERRY_BIN_OP_STRICT_EQUAL, resource_value, parse_options.resource_name);
  TEST_ASSERT (jerry_value_is_true (compare_result));
  jerry_release_value (compare_result);

  jerry_release_value (resource_value);
  jerry_release_value (parse_options.resource_name);
  jerry_release_value (run_result);
  jerry_release_value (program);

  const char *source_5 = "";

  parse_options.options = JERRY_PARSE_HAS_USER_VALUE | JERRY_PARSE_HAS_RESOURCE;
  parse_options.user_value = jerry_create_object ();
  parse_options.resource_name = jerry_create_string ((jerry_char_t *) "demo5.js");

  program = jerry_parse ((const jerry_char_t *) source_5, strlen (source_5), &parse_options);
  TEST_ASSERT (!jerry_value_is_error (program));

  resource_value = jerry_get_resource_name (program);
  compare_result = jerry_binary_operation (JERRY_BIN_OP_STRICT_EQUAL, resource_value, parse_options.resource_name);
  TEST_ASSERT (jerry_value_is_true (compare_result));

  jerry_release_value (resource_value);
  jerry_release_value (compare_result);
  jerry_release_value (parse_options.user_value);
  jerry_release_value (parse_options.resource_name);
  jerry_release_value (program);

  const char *source_6 = "(class {})";

  parse_options.options = JERRY_PARSE_HAS_RESOURCE;
  parse_options.resource_name = jerry_create_string ((jerry_char_t *) "demo6.js");

  program = jerry_parse ((const jerry_char_t *) source_6, strlen (source_6), &parse_options);
  if (!jerry_value_is_error (program))
  {
    resource_value = jerry_get_resource_name (program);
    compare_result = jerry_binary_operation (JERRY_BIN_OP_STRICT_EQUAL, resource_value, parse_options.resource_name);
    TEST_ASSERT (jerry_value_is_true (compare_result));

    jerry_release_value (resource_value);
    jerry_release_value (compare_result);
  }

  jerry_release_value (parse_options.resource_name);
  jerry_release_value (program);

  jerry_cleanup ();

  return 0;
} /* main */
