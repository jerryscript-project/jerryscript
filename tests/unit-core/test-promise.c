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

static const jerry_char_t test_source[] = TEST_STRING_LITERAL ("var p1 = create_promise1();"
                                                               "var p2 = create_promise2();"
                                                               "p1.then(function(x) { "
                                                               "  assert(x==='resolved'); "
                                                               "}); "
                                                               "p2.catch(function(x) { "
                                                               "  assert(x==='rejected'); "
                                                               "}); ");

static int count_in_assert = 0;
static jerry_value_t my_promise1;
static jerry_value_t my_promise2;
static const char s1[] = "resolved";
static const char s2[] = "rejected";

static jerry_value_t
create_promise1_handler (const jerry_call_info_t *call_info_p, /**< call information */
                         const jerry_value_t args_p[], /**< arguments list */
                         const jerry_length_t args_cnt) /**< arguments length */
{
  JERRY_UNUSED (call_info_p);
  JERRY_UNUSED (args_p);
  JERRY_UNUSED (args_cnt);

  jerry_value_t ret = jerry_promise ();
  my_promise1 = jerry_value_copy (ret);

  return ret;
} /* create_promise1_handler */

static jerry_value_t
create_promise2_handler (const jerry_call_info_t *call_info_p, /**< call information */
                         const jerry_value_t args_p[], /**< arguments list */
                         const jerry_length_t args_cnt) /**< arguments length */
{
  JERRY_UNUSED (call_info_p);
  JERRY_UNUSED (args_p);
  JERRY_UNUSED (args_cnt);

  jerry_value_t ret = jerry_promise ();
  my_promise2 = jerry_value_copy (ret);

  return ret;
} /* create_promise2_handler */

static jerry_value_t
assert_handler (const jerry_call_info_t *call_info_p, /**< call information */
                const jerry_value_t args_p[], /**< function arguments */
                const jerry_length_t args_cnt) /**< number of function arguments */
{
  JERRY_UNUSED (call_info_p);

  count_in_assert++;

  if (args_cnt == 1 && jerry_value_is_true (args_p[0]))
  {
    return jerry_boolean (true);
  }
  else
  {
    TEST_ASSERT (false);
  }
} /* assert_handler */

/**
 * Register a JavaScript function in the global object.
 */
static void
register_js_function (const char *name_p, /**< name of the function */
                      jerry_external_handler_t handler_p) /**< function callback */
{
  jerry_value_t global_obj_val = jerry_current_realm ();

  jerry_value_t function_val = jerry_function_external (handler_p);
  jerry_value_t function_name_val = jerry_string_sz (name_p);
  jerry_value_t result_val = jerry_object_set (global_obj_val, function_name_val, function_val);

  jerry_value_free (function_name_val);
  jerry_value_free (function_val);
  jerry_value_free (global_obj_val);

  jerry_value_free (result_val);
} /* register_js_function */

int
main (void)
{
  jerry_init (JERRY_INIT_EMPTY);

  register_js_function ("create_promise1", create_promise1_handler);
  register_js_function ("create_promise2", create_promise2_handler);
  register_js_function ("assert", assert_handler);

  jerry_value_t parsed_code_val = jerry_parse (test_source, sizeof (test_source) - 1, NULL);
  TEST_ASSERT (!jerry_value_is_exception (parsed_code_val));

  jerry_value_t res = jerry_run (parsed_code_val);
  TEST_ASSERT (!jerry_value_is_exception (res));

  jerry_value_free (res);
  jerry_value_free (parsed_code_val);

  /* Test jerry_promise and jerry_value_is_promise. */
  TEST_ASSERT (jerry_value_is_promise (my_promise1));
  TEST_ASSERT (jerry_value_is_promise (my_promise2));

  TEST_ASSERT (count_in_assert == 0);

  /* Test jerry_resolve_or_reject_promise. */
  jerry_value_t str_resolve = jerry_string_sz (s1);
  jerry_value_t str_reject = jerry_string_sz (s2);

  jerry_promise_resolve (my_promise1, str_resolve);
  jerry_promise_reject (my_promise2, str_reject);

  /* The resolve/reject function should be invalid after the promise has the result. */
  jerry_promise_resolve (my_promise2, str_resolve);
  jerry_promise_reject (my_promise1, str_reject);

  /* Run the jobqueue. */
  res = jerry_run_jobs ();

  TEST_ASSERT (!jerry_value_is_exception (res));
  TEST_ASSERT (count_in_assert == 2);

  jerry_value_free (my_promise1);
  jerry_value_free (my_promise2);
  jerry_value_free (str_resolve);
  jerry_value_free (str_reject);

  jerry_cleanup ();

  return 0;
} /* main */
