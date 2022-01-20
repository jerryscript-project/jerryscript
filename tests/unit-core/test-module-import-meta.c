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

static int counter = 0;
static jerry_value_t global_module_value;

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
module_import_meta_callback (const jerry_value_t module, /**< module */
                             const jerry_value_t meta_object, /**< import.meta object */
                             void *user_p) /**< user pointer */
{
  TEST_ASSERT (user_p == (void *) &counter);
  TEST_ASSERT (module == global_module_value);

  jerry_value_t property_name_value = jerry_string_sz ("prop");
  jerry_value_t result_value = jerry_object_set (meta_object, property_name_value, property_name_value);
  jerry_value_free (result_value);
  jerry_value_free (property_name_value);

  counter++;
} /* module_import_meta_callback */

static void
test_syntax_error (const char *source_p, /**< source code */
                   const jerry_parse_options_t *options_p) /**< parse options */
{
  jerry_value_t result_value = jerry_parse ((const jerry_char_t *) source_p, strlen (source_p), options_p);
  TEST_ASSERT (jerry_value_is_exception (result_value) && jerry_error_type (result_value) == JERRY_ERROR_SYNTAX);
  jerry_value_free (result_value);
} /* test_syntax_error */

static void
run_module (const char *source_p, /* source code */
            jerry_parse_options_t *parse_options_p) /* parse options */
{
  global_module_value = jerry_parse ((const jerry_char_t *) source_p, strlen (source_p), parse_options_p);
  TEST_ASSERT (!jerry_value_is_exception (global_module_value));

  jerry_value_t result_value = jerry_module_link (global_module_value, NULL, NULL);
  TEST_ASSERT (!jerry_value_is_exception (result_value));
  jerry_value_free (result_value);

  result_value = jerry_module_evaluate (global_module_value);

  jerry_value_free (global_module_value);

  TEST_ASSERT (!jerry_value_is_exception (result_value));
  jerry_value_free (result_value);
} /* run_module */

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
  jerry_module_on_import_meta (module_import_meta_callback, (void *) &counter);

  /* Syntax errors. */
  test_syntax_error ("import.meta", NULL);
  test_syntax_error ("var a = import.meta", NULL);

  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_MODULE;

  test_syntax_error ("import.m\\u0065ta", &parse_options);
  test_syntax_error ("import.invalid", &parse_options);

  counter = 0;

  run_module (TEST_STRING_LITERAL ("assert(typeof import.meta === 'object')\n"), &parse_options);

  run_module (TEST_STRING_LITERAL ("assert(Object.getPrototypeOf(import.meta) === null)\n"), &parse_options);

  run_module (TEST_STRING_LITERAL ("var meta = import.meta\n"
                                   "assert(import.meta === meta)\n"
                                   "assert(import.meta === meta)\n"
                                   "function f() {\n"
                                   "  assert(import.meta === meta)\n"
                                   "}\n"
                                   "f()\n"),
              &parse_options);

  run_module (TEST_STRING_LITERAL ("import.meta.x = 5.5\n"
                                   "assert(import.meta.x === 5.5)\n"),
              &parse_options);

  run_module (TEST_STRING_LITERAL ("assert(import.meta.prop === 'prop')\n"
                                   "function f() {\n"
                                   "  import.meta.prop = 6.25\n"
                                   "  import.meta.prop2 = 's'\n"
                                   "\n"
                                   "  return function() {\n"
                                   "    assert(import.meta.prop === 6.25)\n"
                                   "    assert(import.meta.prop2 === 's')\n"
                                   "  }\n"
                                   "}\n"
                                   "f()()\n"),
              &parse_options);

  TEST_ASSERT (counter == 5);

  jerry_cleanup ();
  return 0;
} /* main */
