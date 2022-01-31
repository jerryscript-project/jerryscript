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

/**
 * Empty constructor
 */
static jerry_value_t
construct_handler (const jerry_call_info_t *call_info_p, /**< call information */
                   const jerry_value_t args_p[], /**< function arguments */
                   const jerry_length_t args_cnt) /**< number of function arguments */
{
  JERRY_UNUSED (call_info_p);

  TEST_ASSERT (args_cnt == 1);
  TEST_ASSERT (jerry_value_as_number (args_p[0]) == 1.0);

  return jerry_undefined ();
} /* construct_handler */

int
main (void)
{
  jerry_init (JERRY_INIT_EMPTY);

  {
    jerry_value_t global_obj_val = jerry_current_realm ();

    jerry_value_t function_val = jerry_function_external (construct_handler);
    jerry_value_t function_name_val = jerry_string_sz ("Demo");
    jerry_value_t result_val = jerry_object_set (global_obj_val, function_name_val, function_val);
    TEST_ASSERT (!jerry_value_is_exception (result_val));
    TEST_ASSERT (jerry_value_is_true (result_val));
    jerry_value_free (result_val);
    jerry_value_free (function_name_val);
    jerry_value_free (global_obj_val);
    jerry_value_free (function_val);
  }

  {
    static const jerry_char_t test_source[] =
      TEST_STRING_LITERAL ("class Sub1 extends Demo { constructor () { super (1); } };"
                           "new Sub1 ()");

    jerry_value_t parsed_code_val = jerry_parse (test_source, sizeof (test_source) - 1, NULL);
    TEST_ASSERT (!jerry_value_is_exception (parsed_code_val));

    jerry_value_t result = jerry_run (parsed_code_val);
    TEST_ASSERT (!jerry_value_is_exception (result));

    jerry_value_free (result);
    jerry_value_free (parsed_code_val);
  }

  {
    static const jerry_char_t test_source[] = TEST_STRING_LITERAL ("class Sub2 extends Demo { };"
                                                                   "new Sub2 (1)");

    jerry_value_t parsed_code_val = jerry_parse (test_source, sizeof (test_source) - 1, NULL);
    TEST_ASSERT (!jerry_value_is_exception (parsed_code_val));

    jerry_value_t result = jerry_run (parsed_code_val);
    TEST_ASSERT (!jerry_value_is_exception (result));

    jerry_value_free (result);
    jerry_value_free (parsed_code_val);
  }

  jerry_cleanup ();
  return 0;
} /* main */
