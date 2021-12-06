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

#define T(lhs, rhs, res) \
  {                      \
    lhs, rhs, res        \
  }

typedef struct
{
  jerry_value_t lhs;
  jerry_value_t rhs;
  bool expected;
} test_entry_t;

static jerry_value_t
my_constructor (const jerry_call_info_t *call_info_p, /**< call information */
                const jerry_value_t argv[], /**< arguments */
                const jerry_length_t argc) /**< number of arguments */
{
  (void) call_info_p;
  (void) argv;
  (void) argc;
  return jerry_undefined ();
} /* my_constructor */

int
main (void)
{
  TEST_INIT ();

  jerry_init (JERRY_INIT_EMPTY);

  jerry_value_t base_obj = jerry_object ();
  jerry_value_t constructor = jerry_function_external (my_constructor);

  jerry_value_t no_proto_instance_val = jerry_construct (constructor, NULL, 0);

  jerry_value_t prototype_str = jerry_string_sz ("prototype");
  jerry_value_t res = jerry_object_set (constructor, prototype_str, base_obj);
  jerry_value_free (prototype_str);
  TEST_ASSERT (!jerry_value_is_exception (res));
  jerry_value_free (res);

  jerry_value_t instance_val = jerry_construct (constructor, NULL, 0);

  jerry_value_t error = jerry_throw_value (base_obj, false);

  test_entry_t bool_tests[] = { T (jerry_value_copy (instance_val), jerry_value_copy (constructor), true),
                                T (jerry_value_copy (no_proto_instance_val), jerry_value_copy (constructor), false),
                                T (jerry_value_copy (base_obj), jerry_value_copy (constructor), false) };

  for (uint32_t idx = 0; idx < sizeof (bool_tests) / sizeof (test_entry_t); idx++)
  {
    jerry_value_t result = jerry_binary_op (JERRY_BIN_OP_INSTANCEOF, bool_tests[idx].lhs, bool_tests[idx].rhs);
    TEST_ASSERT (!jerry_value_is_exception (result));
    TEST_ASSERT (jerry_value_is_true (result) == bool_tests[idx].expected);
    jerry_value_free (bool_tests[idx].lhs);
    jerry_value_free (bool_tests[idx].rhs);
    jerry_value_free (result);
  }

  test_entry_t error_tests[] = { T (jerry_value_copy (constructor), jerry_value_copy (instance_val), true),
                                 T (jerry_undefined (), jerry_value_copy (constructor), true),
                                 T (jerry_value_copy (instance_val), jerry_undefined (), true),
                                 T (jerry_value_copy (instance_val), jerry_value_copy (base_obj), true),
                                 T (jerry_value_copy (error), jerry_value_copy (constructor), true),
                                 T (jerry_value_copy (instance_val), jerry_value_copy (error), true),
                                 T (jerry_string_sz (""), jerry_string_sz (""), true),
                                 T (jerry_string_sz (""), jerry_number (5.0), true),
                                 T (jerry_number (5.0), jerry_string_sz (""), true),
                                 T (jerry_array (1), jerry_array (1), true),
                                 T (jerry_array (1), jerry_object (), true),
                                 T (jerry_object (), jerry_array (1), true),
                                 T (jerry_null (), jerry_object (), true),
                                 T (jerry_object (), jerry_string_sz (""), true) };

  for (uint32_t idx = 0; idx < sizeof (error_tests) / sizeof (test_entry_t); idx++)
  {
    jerry_value_t result = jerry_binary_op (JERRY_BIN_OP_INSTANCEOF, error_tests[idx].lhs, error_tests[idx].rhs);
    TEST_ASSERT (jerry_value_is_exception (result) == error_tests[idx].expected);
    jerry_value_free (error_tests[idx].lhs);
    jerry_value_free (error_tests[idx].rhs);
    jerry_value_free (result);
  }

  jerry_value_free (base_obj);
  jerry_value_free (constructor);
  jerry_value_free (error);
  jerry_value_free (instance_val);
  jerry_value_free (no_proto_instance_val);

  jerry_cleanup ();

  return 0;
} /* main */
