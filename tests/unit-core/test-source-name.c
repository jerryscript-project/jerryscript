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
source_name_handler (const jerry_call_info_t *call_info_p, /**< call information */
                     const jerry_value_t args_p[], /**< argument list */
                     const jerry_length_t args_count) /**< argument count */
{
  (void) call_info_p;

  jerry_value_t undefined_value = jerry_undefined ();
  jerry_value_t source_name = jerry_source_name (args_count > 0 ? args_p[0] : undefined_value);
  jerry_value_free (undefined_value);

  return source_name;
} /* source_name_handler */

int
main (void)
{
  TEST_INIT ();

  if (!jerry_feature_enabled (JERRY_FEATURE_LINE_INFO))
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Line info support is disabled!\n");
    return 0;
  }

  jerry_init (JERRY_INIT_EMPTY);

  jerry_value_t global = jerry_current_realm ();

  /* Register the "sourceName" method. */
  {
    jerry_value_t func = jerry_function_external (source_name_handler);
    jerry_value_t name = jerry_string_sz ("sourceName");
    jerry_value_t result = jerry_object_set (global, name, func);
    jerry_value_free (result);
    jerry_value_free (name);
    jerry_value_free (func);
  }

  jerry_value_free (global);

  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_HAS_SOURCE_NAME;

  const char *source_1 = ("function f1 () {\n"
                          "  if (sourceName() !== 'demo1.js') return false; \n"
                          "  if (sourceName(f1) !== 'demo1.js') return false; \n"
                          "  if (sourceName(5) !== '<anonymous>') return false; \n"
                          "  return f1; \n"
                          "} \n"
                          "f1();");

  parse_options.source_name = jerry_string_sz ("demo1.js");

  jerry_value_t program = jerry_parse ((const jerry_char_t *) source_1, strlen (source_1), &parse_options);
  TEST_ASSERT (!jerry_value_is_exception (program));

  jerry_value_t run_result = jerry_run (program);
  TEST_ASSERT (!jerry_value_is_exception (run_result));
  TEST_ASSERT (jerry_value_is_object (run_result));

  jerry_value_t source_name_value = jerry_source_name (run_result);
  jerry_value_t compare_result =
    jerry_binary_op (JERRY_BIN_OP_STRICT_EQUAL, source_name_value, parse_options.source_name);
  TEST_ASSERT (jerry_value_is_true (compare_result));

  jerry_value_free (compare_result);
  jerry_value_free (source_name_value);
  jerry_value_free (parse_options.source_name);

  jerry_value_free (run_result);
  jerry_value_free (program);

  const char *source_2 = ("function f2 () { \n"
                          "  if (sourceName() !== 'demo2.js') return false; \n"
                          "  if (sourceName(f2) !== 'demo2.js') return false; \n"
                          "  if (sourceName(f1) !== 'demo1.js') return false; \n"
                          "  if (sourceName(Object.prototype) !== '<anonymous>') return false; \n"
                          "  if (sourceName(Function) !== '<anonymous>') return false; \n"
                          "  return f2; \n"
                          "} \n"
                          "f2(); \n");

  parse_options.source_name = jerry_string_sz ("demo2.js");

  program = jerry_parse ((const jerry_char_t *) source_2, strlen (source_2), &parse_options);
  TEST_ASSERT (!jerry_value_is_exception (program));

  run_result = jerry_run (program);
  TEST_ASSERT (!jerry_value_is_exception (run_result));
  TEST_ASSERT (jerry_value_is_object (run_result));

  source_name_value = jerry_source_name (run_result);
  compare_result = jerry_binary_op (JERRY_BIN_OP_STRICT_EQUAL, source_name_value, parse_options.source_name);
  TEST_ASSERT (jerry_value_is_true (compare_result));

  jerry_value_free (compare_result);
  jerry_value_free (source_name_value);
  jerry_value_free (parse_options.source_name);

  jerry_value_free (run_result);
  jerry_value_free (program);
  if (jerry_feature_enabled (JERRY_FEATURE_MODULE))
  {
    jerry_value_t anon = jerry_string_sz ("<anonymous>");
    const char *source_3 = "";

    parse_options.options = JERRY_PARSE_MODULE | JERRY_PARSE_HAS_SOURCE_NAME;
    parse_options.source_name = jerry_string_sz ("demo3.js");

    program = jerry_parse ((const jerry_char_t *) source_3, strlen (source_3), &parse_options);
    TEST_ASSERT (!jerry_value_is_exception (program));

    source_name_value = jerry_source_name (program);
    compare_result = jerry_binary_op (JERRY_BIN_OP_STRICT_EQUAL, source_name_value, parse_options.source_name);
    TEST_ASSERT (jerry_value_is_true (compare_result));

    jerry_value_free (compare_result);
    jerry_value_free (source_name_value);

    run_result = jerry_module_link (program, NULL, NULL);
    TEST_ASSERT (!jerry_value_is_exception (run_result));

    source_name_value = jerry_source_name (run_result);
    compare_result = jerry_binary_op (JERRY_BIN_OP_STRICT_EQUAL, source_name_value, anon);
    TEST_ASSERT (jerry_value_is_true (compare_result));

    jerry_value_free (compare_result);
    jerry_value_free (source_name_value);
    jerry_value_free (run_result);

    run_result = jerry_module_evaluate (program);
    TEST_ASSERT (!jerry_value_is_exception (run_result));

    source_name_value = jerry_source_name (run_result);
    compare_result = jerry_binary_op (JERRY_BIN_OP_STRICT_EQUAL, source_name_value, anon);
    TEST_ASSERT (jerry_value_is_true (compare_result));

    jerry_value_free (compare_result);
    jerry_value_free (source_name_value);
    jerry_value_free (run_result);
    jerry_value_free (program);
    jerry_value_free (parse_options.source_name);
  }
  const char *source_4 = ("function f(){} \n"
                          "f.bind().bind();");

  parse_options.options = JERRY_PARSE_HAS_SOURCE_NAME;
  parse_options.source_name = jerry_string_sz ("demo4.js");

  program = jerry_parse ((const jerry_char_t *) source_4, strlen (source_4), &parse_options);
  TEST_ASSERT (!jerry_value_is_exception (program));

  run_result = jerry_run (program);
  TEST_ASSERT (!jerry_value_is_exception (run_result));
  TEST_ASSERT (jerry_value_is_object (run_result));

  source_name_value = jerry_source_name (run_result);
  compare_result = jerry_binary_op (JERRY_BIN_OP_STRICT_EQUAL, source_name_value, parse_options.source_name);
  TEST_ASSERT (jerry_value_is_true (compare_result));
  jerry_value_free (compare_result);

  jerry_value_free (source_name_value);
  jerry_value_free (parse_options.source_name);
  jerry_value_free (run_result);
  jerry_value_free (program);

  const char *source_5 = "";

  parse_options.options = JERRY_PARSE_HAS_USER_VALUE | JERRY_PARSE_HAS_SOURCE_NAME;
  parse_options.user_value = jerry_object ();
  parse_options.source_name = jerry_string_sz ("demo5.js");

  program = jerry_parse ((const jerry_char_t *) source_5, strlen (source_5), &parse_options);
  TEST_ASSERT (!jerry_value_is_exception (program));

  source_name_value = jerry_source_name (program);
  compare_result = jerry_binary_op (JERRY_BIN_OP_STRICT_EQUAL, source_name_value, parse_options.source_name);
  TEST_ASSERT (jerry_value_is_true (compare_result));

  jerry_value_free (source_name_value);
  jerry_value_free (compare_result);
  jerry_value_free (parse_options.user_value);
  jerry_value_free (parse_options.source_name);
  jerry_value_free (program);

  const char *source_6 = "(class {})";

  parse_options.options = JERRY_PARSE_HAS_SOURCE_NAME;
  parse_options.source_name = jerry_string_sz ("demo6.js");

  program = jerry_parse ((const jerry_char_t *) source_6, strlen (source_6), &parse_options);
  if (!jerry_value_is_exception (program))
  {
    source_name_value = jerry_source_name (program);
    compare_result = jerry_binary_op (JERRY_BIN_OP_STRICT_EQUAL, source_name_value, parse_options.source_name);
    TEST_ASSERT (jerry_value_is_true (compare_result));

    jerry_value_free (source_name_value);
    jerry_value_free (compare_result);
  }

  jerry_value_free (parse_options.source_name);
  jerry_value_free (program);

  jerry_cleanup ();

  return 0;
} /* main */
