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
#include "jerryscript.h"

#include "test-common.h"

static int mode = 0;
static jerry_value_t global_user_value;

static jerry_value_t
global_assert (const jerry_call_info_t *call_info_p, /**< call information */
               const jerry_value_t args_p[], /**< arguments list */
               const jerry_length_t args_cnt) /**< arguments length */
{
  JERRY_UNUSED (call_info_p);

  TEST_ASSERT (args_cnt == 1 && jerry_value_is_true (args_p[0]));
  return jerry_boolean (true);
} /* global_assert */

static void
register_assert (void)
{
  jerry_value_t global_object_value = jerry_current_realm ();

  jerry_value_t function_value = jerry_function_external (global_assert);
  jerry_value_t function_name_value = jerry_string_sz ("assert");
  jerry_value_t result_value = jerry_object_set (global_object_value, function_name_value, function_value);

  jerry_value_free (function_name_value);
  jerry_value_free (function_value);
  jerry_value_free (global_object_value);

  TEST_ASSERT (jerry_value_is_true (result_value));
  jerry_value_free (result_value);
} /* register_assert */

static void
compare_specifier (jerry_value_t specifier, /* string value */
                   int id) /* module id */
{
  jerry_char_t string[] = "XX_module.mjs";

  TEST_ASSERT (id >= 1 && id <= 99 && string[0] == 'X' && string[1] == 'X');

  string[0] = (jerry_char_t) ((id / 10) + '0');
  string[1] = (jerry_char_t) ((id % 10) + '0');

  jerry_size_t length = (jerry_size_t) (sizeof (string) - 1);
  jerry_char_t buffer[sizeof (string) - 1];

  TEST_ASSERT (jerry_value_is_string (specifier));
  TEST_ASSERT (jerry_string_size (specifier, JERRY_ENCODING_CESU8) == length);

  TEST_ASSERT (jerry_string_to_buffer (specifier, JERRY_ENCODING_CESU8, buffer, length) == length);
  TEST_ASSERT (memcmp (buffer, string, length) == 0);
} /* compare_specifier */

static jerry_value_t
module_import_callback (const jerry_value_t specifier, /* string value */
                        const jerry_value_t user_value, /* user value assigned to the script */
                        void *user_p) /* user pointer */
{
  TEST_ASSERT (user_p == (void *) &mode);

  if (mode != 3)
  {
    jerry_value_t compare_value = jerry_binary_op (JERRY_BIN_OP_STRICT_EQUAL, user_value, global_user_value);

    TEST_ASSERT (jerry_value_is_true (compare_value));
    jerry_value_free (compare_value);
  }

  switch (mode)
  {
    case 0:
    {
      compare_specifier (specifier, 1);
      return jerry_throw_sz (JERRY_ERROR_RANGE, "Err01");
    }
    case 1:
    {
      compare_specifier (specifier, 2);
      return jerry_null ();
    }
    case 2:
    {
      compare_specifier (specifier, 3);

      jerry_value_t promise_value = jerry_promise ();
      /* Normally this should be a namespace object. */
      jerry_value_t object_value = jerry_object ();
      jerry_promise_resolve (promise_value, object_value);
      jerry_value_free (object_value);
      return promise_value;
    }
    case 3:
    {
      compare_specifier (specifier, 28);

      TEST_ASSERT (jerry_value_is_object (user_value));
      jerry_value_t property_name = jerry_string_sz ("MyProp1");
      jerry_value_t result = jerry_object_get (user_value, property_name);
      TEST_ASSERT (jerry_value_is_number (result) && jerry_value_as_number (result) == 3.5);
      jerry_value_free (result);
      jerry_value_free (property_name);
      return jerry_undefined ();
    }
  }

  TEST_ASSERT (mode == 4 || mode == 5);

  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_MODULE;

  jerry_value_t parse_result_value = jerry_parse ((const jerry_char_t *) "", 0, &parse_options);
  TEST_ASSERT (!jerry_value_is_exception (parse_result_value));

  jerry_value_t result_value = jerry_module_link (parse_result_value, NULL, NULL);
  TEST_ASSERT (!jerry_value_is_exception (result_value));
  jerry_value_free (result_value);

  if (mode == 4)
  {
    result_value = jerry_module_evaluate (parse_result_value);
    TEST_ASSERT (!jerry_value_is_exception (result_value));
    jerry_value_free (result_value);
  }

  return parse_result_value;
} /* module_import_callback */

static void
run_script (const char *source_p, /* source code */
            jerry_parse_options_t *parse_options_p, /* parse options */
            bool release_user_value) /* release user value */
{
  jerry_value_t parse_result_value;

  parse_result_value = jerry_parse ((const jerry_char_t *) source_p, strlen (source_p), parse_options_p);
  TEST_ASSERT (!jerry_value_is_exception (parse_result_value));

  if (release_user_value)
  {
    jerry_value_free (parse_options_p->user_value);
    jerry_heap_gc (JERRY_GC_PRESSURE_HIGH);
  }

  jerry_value_t result_value;
  if (parse_options_p->options & JERRY_PARSE_MODULE)
  {
    result_value = jerry_module_link (parse_result_value, NULL, NULL);
    TEST_ASSERT (!jerry_value_is_exception (result_value));
    jerry_value_free (result_value);

    result_value = jerry_module_evaluate (parse_result_value);
  }
  else
  {
    result_value = jerry_run (parse_result_value);
  }

  jerry_value_free (parse_result_value);

  TEST_ASSERT (!jerry_value_is_exception (result_value));
  jerry_value_free (result_value);

  result_value = jerry_run_jobs ();
  TEST_ASSERT (!jerry_value_is_exception (result_value));
  jerry_value_free (result_value);
} /* run_script */

int
main (void)
{
  jerry_init (JERRY_INIT_EMPTY);

  if (!jerry_feature_enabled (JERRY_FEATURE_MODULE))
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Module is disabled!\n");
    jerry_cleanup ();
    return 0;
  }

  register_assert ();
  jerry_module_on_import (module_import_callback, (void *) &mode);

  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_NO_OPTS;

  if (jerry_feature_enabled (JERRY_FEATURE_ERROR_MESSAGES))
  {
    run_script ("var expected_message = 'Module cannot be instantiated'", &parse_options, false);
  }
  else
  {
    run_script ("var expected_message = ''", &parse_options, false);
  }

  global_user_value = jerry_object ();
  const char *source_p = TEST_STRING_LITERAL ("import('01_module.mjs').then(\n"
                                              "  function(resolve) { assert(false) },\n"
                                              "  function(reject) {\n"
                                              "    assert(reject instanceof RangeError\n"
                                              "           && reject.message === 'Err01')\n"
                                              "  }\n"
                                              ")");

  mode = 0;
  parse_options.options = JERRY_PARSE_HAS_USER_VALUE;
  parse_options.user_value = global_user_value;
  run_script (source_p, &parse_options, false);
  jerry_value_free (global_user_value);

  global_user_value = jerry_null ();
  source_p = TEST_STRING_LITERAL ("var src = \"import('02_module.mjs').then(\\\n"
                                  "  function(resolve) { assert(false) },\\\n"
                                  "  function(reject) {\\\n"
                                  "    assert(reject instanceof RangeError\\\n"
                                  "           && reject.message === expected_message)\\\n"
                                  "  }\\\n"
                                  ")\"\n"
                                  "eval('eval(src)')");

  mode = 1;
  parse_options.options = JERRY_PARSE_HAS_USER_VALUE;
  parse_options.user_value = global_user_value;
  run_script (source_p, &parse_options, false);
  jerry_value_free (global_user_value);

  global_user_value = jerry_number (5.6);
  source_p = TEST_STRING_LITERAL ("function f() {\n"
                                  "  return function () {\n"
                                  "    return import('03_module.mjs')\n"
                                  "  }\n"
                                  "}\n"
                                  "export var a = f()().then(\n"
                                  "  function(resolve) { assert(typeof resolve == 'object') },\n"
                                  "  function(reject) { assert(false) }\n"
                                  ")");

  mode = 2;
  parse_options.options = JERRY_PARSE_HAS_USER_VALUE | JERRY_PARSE_MODULE;
  parse_options.user_value = global_user_value;
  run_script (source_p, &parse_options, false);
  jerry_value_free (global_user_value);

  global_user_value = jerry_string_sz ("Any string...");
  source_p = TEST_STRING_LITERAL ("var src = \"import('02_module.mjs').then(\\\n"
                                  "  function(resolve) { assert(typeof resolve == 'object') },\\\n"
                                  "  function(reject) { assert(false) }\\\n"
                                  ")\"\n"
                                  "function f() {\n"
                                  "  eval('(function() { return eval(src) })()')\n"
                                  "}\n"
                                  "f()\n");

  for (int i = 0; i < 2; i++)
  {
    mode = 3;
    parse_options.options = JERRY_PARSE_HAS_USER_VALUE | (i == 1 ? JERRY_PARSE_MODULE : 0);
    parse_options.user_value = jerry_object ();
    jerry_value_t property_name = jerry_string_sz ("MyProp1");
    jerry_value_t property_value = jerry_number (3.5);
    jerry_value_t result = jerry_object_set (parse_options.user_value, property_name, property_value);
    TEST_ASSERT (jerry_value_is_true (result));
    jerry_value_free (result);
    jerry_value_free (property_value);
    jerry_value_free (property_name);

    source_p = TEST_STRING_LITERAL ("import('28_module.mjs')");
    run_script (source_p, &parse_options, true);
  }

  mode = 4;
  parse_options.options = JERRY_PARSE_HAS_USER_VALUE;
  parse_options.user_value = global_user_value;
  run_script (source_p, &parse_options, false);
  jerry_value_free (global_user_value);

  global_user_value = jerry_function_external (global_assert);
  source_p = TEST_STRING_LITERAL ("var src = \"import('02_module.mjs').then(\\\n"
                                  "  function(resolve) { assert(false) },\\\n"
                                  "  function(reject) {\\\n"
                                  "    assert(reject instanceof RangeError\\\n"
                                  "           && reject.message === expected_message)\\\n"
                                  "  }\\\n"
                                  ")\"\n"
                                  "export function f() {\n"
                                  "  eval('(function() { return eval(src) })()')\n"
                                  "}\n"
                                  "f()\n");

  mode = 5;
  parse_options.options = JERRY_PARSE_HAS_USER_VALUE | JERRY_PARSE_MODULE;
  parse_options.user_value = global_user_value;
  run_script (source_p, &parse_options, false);
  jerry_value_free (global_user_value);

  jerry_cleanup ();
  return 0;
} /* main */
