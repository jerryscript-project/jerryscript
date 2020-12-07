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

#include "ecma-globals.h"
#include "ecma-helpers.h"

#include "test-common.h"

static void
create_number_property (jerry_value_t object_value,
                        char *name_p,
                        double number)
{
  jerry_value_t name_value = jerry_create_string ((const jerry_char_t *) name_p);
  jerry_value_t number_value = jerry_create_number (number);
  jerry_value_t result_value = jerry_set_property (object_value, name_value, number_value);
  TEST_ASSERT (!jerry_value_is_error (result_value));

  jerry_release_value (result_value);
  jerry_release_value (number_value);
  jerry_release_value (name_value);
} /* create_number_property */

static double
eval_and_get_number (char *script_p)
{
  jerry_value_t result_value;
  result_value = jerry_eval ((const jerry_char_t *) script_p, strlen (script_p), JERRY_PARSE_NO_OPTS);

  TEST_ASSERT (jerry_value_is_number (result_value));
  double result = jerry_get_number_value (result_value);
  jerry_release_value (result_value);
  return result;
} /* eval_and_get_number */

/**
 * Unit test's main function.
 */
int
main (void)
{
  TEST_INIT ();

  jerry_init (JERRY_INIT_EMPTY);

  if (!jerry_is_feature_enabled (JERRY_FEATURE_REALM))
  {
    printf ("Skipping test, Realms not enabled\n");
    return 0;
  }

  jerry_value_t global_value = jerry_get_global_object ();
  jerry_value_t realm_value = jerry_create_realm ();

  create_number_property (global_value, "a", 3.5);
  create_number_property (global_value, "b", 7.25);
  create_number_property (realm_value, "a", -1.25);
  create_number_property (realm_value, "b", -6.75);

  TEST_ASSERT (eval_and_get_number ("a") == 3.5);

  jerry_value_t result_value = jerry_set_realm (realm_value);
  TEST_ASSERT (result_value == global_value);
  TEST_ASSERT (eval_and_get_number ("a") == -1.25);

  result_value = jerry_set_realm (global_value);
  TEST_ASSERT (result_value == realm_value);
  TEST_ASSERT (eval_and_get_number ("b") == 7.25);

  result_value = jerry_set_realm (realm_value);
  TEST_ASSERT (result_value == global_value);
  TEST_ASSERT (eval_and_get_number ("b") == -6.75);

  result_value = jerry_set_realm (global_value);
  TEST_ASSERT (result_value == realm_value);

  jerry_value_t object_value = jerry_create_object ();
  result_value = jerry_set_realm (object_value);
  TEST_ASSERT (jerry_value_is_error (result_value));
  jerry_release_value (result_value);
  jerry_release_value (object_value);

  jerry_release_value (global_value);
  jerry_release_value (realm_value);

  jerry_cleanup ();
  return 0;
} /* main */
