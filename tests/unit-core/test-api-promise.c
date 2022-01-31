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

static void
test_promise_resolve_success (void)
{
  jerry_value_t my_promise = jerry_promise ();

  // A created promise has an undefined promise result by default and a pending state
  {
    jerry_value_t promise_result = jerry_promise_result (my_promise);
    TEST_ASSERT (jerry_value_is_undefined (promise_result));

    jerry_promise_state_t promise_state = jerry_promise_state (my_promise);
    TEST_ASSERT (promise_state == JERRY_PROMISE_STATE_PENDING);

    jerry_value_free (promise_result);
  }

  jerry_value_t resolve_value = jerry_object ();
  {
    jerry_value_t obj_key = jerry_string_sz ("key_one");
    jerry_value_t set_result = jerry_object_set (resolve_value, obj_key, jerry_number (3));
    TEST_ASSERT (jerry_value_is_boolean (set_result) && (jerry_value_is_true (set_result)));
    jerry_value_free (set_result);
    jerry_value_free (obj_key);
  }

  // A resolved promise should have the result of from the resolve call and a fulfilled state
  {
    jerry_value_t resolve_result = jerry_promise_resolve (my_promise, resolve_value);

    // Release "old" value of resolve.
    jerry_value_free (resolve_value);

    jerry_value_t promise_result = jerry_promise_result (my_promise);
    {
      TEST_ASSERT (jerry_value_is_object (promise_result));
      jerry_value_t obj_key = jerry_string_sz ("key_one");
      jerry_value_t get_result = jerry_object_get (promise_result, obj_key);
      TEST_ASSERT (jerry_value_is_number (get_result));
      TEST_ASSERT (jerry_value_as_number (get_result) == 3.0);

      jerry_value_free (get_result);
      jerry_value_free (obj_key);
    }

    jerry_promise_state_t promise_state = jerry_promise_state (my_promise);
    TEST_ASSERT (promise_state == JERRY_PROMISE_STATE_FULFILLED);

    jerry_value_free (promise_result);

    jerry_value_free (resolve_result);
  }

  // Resolvind a promise again does not change the result/state
  {
    jerry_value_t resolve_result = jerry_promise_reject (my_promise, jerry_number (50));

    jerry_value_t promise_result = jerry_promise_result (my_promise);
    {
      TEST_ASSERT (jerry_value_is_object (promise_result));
      jerry_value_t obj_key = jerry_string_sz ("key_one");
      jerry_value_t get_result = jerry_object_get (promise_result, obj_key);
      TEST_ASSERT (jerry_value_is_number (get_result));
      TEST_ASSERT (jerry_value_as_number (get_result) == 3.0);

      jerry_value_free (get_result);
      jerry_value_free (obj_key);
    }

    jerry_promise_state_t promise_state = jerry_promise_state (my_promise);
    TEST_ASSERT (promise_state == JERRY_PROMISE_STATE_FULFILLED);

    jerry_value_free (promise_result);

    jerry_value_free (resolve_result);
  }

  jerry_value_free (my_promise);
} /* test_promise_resolve_success */

static void
test_promise_resolve_fail (void)
{
  jerry_value_t my_promise = jerry_promise ();

  // A created promise has an undefined promise result by default and a pending state
  {
    jerry_value_t promise_result = jerry_promise_result (my_promise);
    TEST_ASSERT (jerry_value_is_undefined (promise_result));

    jerry_promise_state_t promise_state = jerry_promise_state (my_promise);
    TEST_ASSERT (promise_state == JERRY_PROMISE_STATE_PENDING);

    jerry_value_free (promise_result);
  }

  // A resolved promise should have the result of from the resolve call and a fulfilled state
  {
    jerry_value_t error_obj = jerry_error_sz (JERRY_ERROR_TYPE, "resolve_fail");
    jerry_value_t resolve_result = jerry_promise_reject (my_promise, error_obj);
    jerry_value_free (error_obj);

    jerry_value_t promise_result = jerry_promise_result (my_promise);
    // The error is not throw that's why it is only an error object.
    TEST_ASSERT (jerry_value_is_object (promise_result));
    TEST_ASSERT (jerry_error_type (promise_result) == JERRY_ERROR_TYPE);

    jerry_promise_state_t promise_state = jerry_promise_state (my_promise);
    TEST_ASSERT (promise_state == JERRY_PROMISE_STATE_REJECTED);

    jerry_value_free (promise_result);

    jerry_value_free (resolve_result);
  }

  // Resolvind a promise again does not change the result/state
  {
    jerry_value_t resolve_result = jerry_promise_resolve (my_promise, jerry_number (50));

    jerry_value_t promise_result = jerry_promise_result (my_promise);
    TEST_ASSERT (jerry_value_is_object (promise_result));
    TEST_ASSERT (jerry_error_type (promise_result) == JERRY_ERROR_TYPE);

    jerry_promise_state_t promise_state = jerry_promise_state (my_promise);
    TEST_ASSERT (promise_state == JERRY_PROMISE_STATE_REJECTED);

    jerry_value_free (promise_result);

    jerry_value_free (resolve_result);
  }

  jerry_value_free (my_promise);
} /* test_promise_resolve_fail */

static void
test_promise_from_js (void)
{
  const jerry_char_t test_source[] = "(new Promise(function(rs, rj) { rs(30); })).then(function(v) { return v + 1; })";

  jerry_value_t parsed_code_val = jerry_parse (test_source, sizeof (test_source) - 1, NULL);
  TEST_ASSERT (!jerry_value_is_exception (parsed_code_val));

  jerry_value_t res = jerry_run (parsed_code_val);
  TEST_ASSERT (jerry_value_is_promise (res));

  TEST_ASSERT (jerry_promise_state (res) == JERRY_PROMISE_STATE_PENDING);

  jerry_value_t run_result = jerry_run_jobs ();
  TEST_ASSERT (jerry_value_is_undefined (run_result));
  jerry_value_free (run_result);

  TEST_ASSERT (jerry_promise_state (res) == JERRY_PROMISE_STATE_FULFILLED);
  jerry_value_t promise_result = jerry_promise_result (res);
  TEST_ASSERT (jerry_value_is_number (promise_result));
  TEST_ASSERT (jerry_value_as_number (promise_result) == 31.0);

  jerry_value_free (promise_result);
  jerry_value_free (res);
  jerry_value_free (parsed_code_val);
} /* test_promise_from_js */

int
main (void)
{
  jerry_init (JERRY_INIT_EMPTY);

  test_promise_resolve_fail ();
  test_promise_resolve_success ();

  test_promise_from_js ();

  jerry_cleanup ();

  return 0;
} /* main */
