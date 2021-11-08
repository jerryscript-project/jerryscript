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

#include "test-common.h"

int
main (void)
{
  TEST_INIT ();

  jerry_init (JERRY_INIT_EMPTY);

  jerry_error_t errors[] = { JERRY_ERROR_COMMON, JERRY_ERROR_EVAL, JERRY_ERROR_RANGE, JERRY_ERROR_REFERENCE,
                             JERRY_ERROR_SYNTAX, JERRY_ERROR_TYPE, JERRY_ERROR_URI };

  for (size_t idx = 0; idx < sizeof (errors) / sizeof (errors[0]); idx++)
  {
    jerry_value_t error_obj = jerry_throw_sz (errors[idx], "test");
    TEST_ASSERT (jerry_value_is_exception (error_obj));
    TEST_ASSERT (jerry_error_type (error_obj) == errors[idx]);

    error_obj = jerry_exception_value (error_obj, true);

    TEST_ASSERT (jerry_error_type (error_obj) == errors[idx]);

    jerry_value_free (error_obj);
  }

  jerry_value_t test_values[] = {
    jerry_number (11),
    jerry_string_sz ("message"),
    jerry_boolean (true),
    jerry_object (),
  };

  for (size_t idx = 0; idx < sizeof (test_values) / sizeof (test_values[0]); idx++)
  {
    jerry_error_t error_type = jerry_error_type (test_values[idx]);
    TEST_ASSERT (error_type == JERRY_ERROR_NONE);
    jerry_value_free (test_values[idx]);
  }

  char test_source[] = "\xF0\x9D\x84\x9E";

  jerry_value_t result = jerry_parse ((const jerry_char_t *) test_source, sizeof (test_source) - 1, NULL);
  TEST_ASSERT (jerry_value_is_exception (result));
  TEST_ASSERT (jerry_error_type (result) == JERRY_ERROR_SYNTAX);

  jerry_value_free (result);

  char test_invalid_error[] = "Object.create(Error.prototype)";
  result = jerry_eval ((const jerry_char_t *) test_invalid_error, sizeof (test_invalid_error) - 1, JERRY_PARSE_NO_OPTS);
  TEST_ASSERT (!jerry_value_is_exception (result) && jerry_value_is_object (result));
  TEST_ASSERT (jerry_error_type (result) == JERRY_ERROR_NONE);

  jerry_value_free (result);

  jerry_cleanup ();
} /* main */
