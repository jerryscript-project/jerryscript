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

static jerry_value_t user_values[4];

#define USER_VALUES_SIZE (sizeof (user_values) / sizeof (jerry_value_t))

static void
test_parse (const char *source_p, /**< source code */
            jerry_parse_options_t *options_p, /**< options passed to jerry_parse */
            bool run_code) /**< run the code after parsing */
{
  for (size_t i = 0; i < USER_VALUES_SIZE; i++)
  {
    options_p->user_value = user_values[i];

    jerry_value_t result = jerry_parse ((const jerry_char_t *) source_p, strlen (source_p), options_p);
    TEST_ASSERT (!jerry_value_is_error (result));

    if (run_code)
    {
      jerry_value_t parse_result = result;
      result = jerry_run (result);
      jerry_release_value (parse_result);
      TEST_ASSERT (!jerry_value_is_error (result));
    }

    jerry_value_t user_value = jerry_get_user_value (result);
    jerry_value_t compare_value = jerry_binary_operation (JERRY_BIN_OP_STRICT_EQUAL, user_value, user_values[i]);

    TEST_ASSERT (jerry_value_is_true (compare_value));

    jerry_release_value (compare_value);
    jerry_release_value (user_value);
    jerry_release_value (result);
  }
} /* test_parse */

static void
test_parse_function (const char *source_p, /**< source code */
                     jerry_parse_options_t *options_p, /**< options passed to jerry_parse */
                     bool run_code) /**< run the code after parsing */
{
  options_p->options |= JERRY_PARSE_HAS_ARGUMENT_LIST;
  options_p->argument_list = jerry_create_string ((const jerry_char_t *) "");

  for (size_t i = 0; i < USER_VALUES_SIZE; i++)
  {
    options_p->user_value = user_values[i];

    jerry_value_t result = jerry_parse ((const jerry_char_t *) source_p, strlen (source_p), options_p);
    TEST_ASSERT (!jerry_value_is_error (result));

    if (run_code)
    {
      jerry_value_t parse_result = result;
      jerry_value_t this_value = jerry_create_undefined ();
      result = jerry_call_function (result, this_value, NULL, 0);
      jerry_release_value (parse_result);
      jerry_release_value (this_value);
      TEST_ASSERT (!jerry_value_is_error (result));
    }

    jerry_value_t user_value = jerry_get_user_value (result);
    jerry_value_t compare_value = jerry_binary_operation (JERRY_BIN_OP_STRICT_EQUAL, user_value, user_values[i]);

    TEST_ASSERT (jerry_value_is_true (compare_value));

    jerry_release_value (compare_value);
    jerry_release_value (user_value);
    jerry_release_value (result);
  }

  jerry_release_value (options_p->argument_list);
} /* test_parse_function */

int
main (void)
{
  TEST_INIT ();

  jerry_init (JERRY_INIT_EMPTY);

  user_values[0] = jerry_create_object ();
  user_values[1] = jerry_create_null ();
  user_values[2] = jerry_create_number (5.5);
  user_values[3] = jerry_create_string ((const jerry_char_t *) "AnyString...");

  jerry_parse_options_t parse_options;
  const char *source_p = TEST_STRING_LITERAL ("");

  parse_options.options = JERRY_PARSE_HAS_USER_VALUE;
  test_parse (source_p, &parse_options, false);
  test_parse_function (source_p, &parse_options, false);

  if (jerry_is_feature_enabled (JERRY_FEATURE_MODULE))
  {
    parse_options.options = JERRY_PARSE_MODULE | JERRY_PARSE_HAS_USER_VALUE;
    test_parse (source_p, &parse_options, false);
  }

  source_p = TEST_STRING_LITERAL ("function f() { }\n"
                                  "f");
  parse_options.options = JERRY_PARSE_HAS_USER_VALUE;
  test_parse (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("function f() { return function() {} }\n"
                                  "f()");
  parse_options.options = JERRY_PARSE_HAS_USER_VALUE;
  test_parse (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("return function() {}");
  parse_options.options = JERRY_PARSE_HAS_USER_VALUE;
  test_parse_function (source_p, &parse_options, true);

  /* There is no test for ESNEXT, using SYMBOL instead. */
  if (jerry_is_feature_enabled (JERRY_FEATURE_SYMBOL))
  {
    source_p = TEST_STRING_LITERAL ("(class {})");
    parse_options.options = JERRY_PARSE_HAS_USER_VALUE;
    test_parse (source_p, &parse_options, true);
  }

  source_p = TEST_STRING_LITERAL ("eval('function f() {}')\n"
                                  "f");
  parse_options.options = JERRY_PARSE_HAS_USER_VALUE;
  test_parse (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("eval('function f() { return eval(\\'(function () {})\\') }')\n"
                                  "f()");
  parse_options.options = JERRY_PARSE_HAS_USER_VALUE;
  test_parse (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("eval('function f() {}')\n"
                                  "return f");
  parse_options.options = JERRY_PARSE_HAS_USER_VALUE;
  test_parse_function (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("eval('function f() { return eval(\\'(function () {})\\') }')\n"
                                  "return f()");
  parse_options.options = JERRY_PARSE_HAS_USER_VALUE;
  test_parse_function (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("function f() {}\n"
                                  "f.bind(1)");
  parse_options.options = JERRY_PARSE_HAS_USER_VALUE;
  test_parse (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("function f() {}\n"
                                  "f.bind(1).bind(2, 3)");
  parse_options.options = JERRY_PARSE_HAS_USER_VALUE;
  test_parse (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("function f() {}\n"
                                  "return f.bind(1)");
  parse_options.options = JERRY_PARSE_HAS_USER_VALUE;
  test_parse_function (source_p, &parse_options, true);

  source_p = TEST_STRING_LITERAL ("function f() {}\n"
                                  "return f.bind(1).bind(2, 3)");
  parse_options.options = JERRY_PARSE_HAS_USER_VALUE;
  test_parse_function (source_p, &parse_options, true);

  for (size_t i = 0; i < USER_VALUES_SIZE; i++)
  {
    jerry_value_t result = jerry_get_user_value (user_values[i]);
    TEST_ASSERT (jerry_value_is_undefined (result));
    jerry_release_value (result);
  }

  for (size_t i = 0; i < USER_VALUES_SIZE; i++)
  {
    jerry_release_value (user_values[i]);
  }

  jerry_cleanup ();
  return 0;
} /* main */
