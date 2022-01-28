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
 * Register a JavaScript function in the global object.
 */
static jerry_value_t
register_js_function (const char *name_p, /**< name of the function */
                      jerry_external_handler_t handler_p) /**< function callback */
{
  jerry_value_t global_obj_val = jerry_current_realm ();

  jerry_value_t function_val = jerry_function_external (handler_p);
  jerry_value_t function_name_val = jerry_string_sz (name_p);
  jerry_value_t result_val = jerry_object_set (global_obj_val, function_name_val, function_val);

  jerry_value_free (function_name_val);
  jerry_value_free (global_obj_val);

  jerry_value_free (result_val);

  return function_val;
} /* register_js_function */

enum
{
  TEST_ID_SIMPLE_CONSTRUCT = 1,
  TEST_ID_SIMPLE_CALL = 2,
  TEST_ID_CONSTRUCT_AND_CALL_SUB = 3,
};

static jerry_value_t
construct_handler (const jerry_call_info_t *call_info_p, /**< call information */
                   const jerry_value_t args_p[], /**< function arguments */
                   const jerry_length_t args_cnt) /**< number of function arguments */
{
  JERRY_UNUSED (args_p);

  if (args_cnt != 1 || !jerry_value_is_number (args_p[0]))
  {
    TEST_ASSERT (0 && "Invalid arguments for demo method");
  }

  int test_id = (int) jerry_value_as_number (args_p[0]);

  switch (test_id)
  {
    case TEST_ID_SIMPLE_CONSTRUCT:
    {
      /* Method was called with "new": new.target should be equal to the function object. */
      jerry_value_t target = call_info_p->new_target;
      TEST_ASSERT (!jerry_value_is_undefined (target));
      TEST_ASSERT (target == call_info_p->function);
      break;
    }
    case TEST_ID_SIMPLE_CALL:
    {
      /* Method was called directly without "new": new.target should be equal undefined. */
      jerry_value_t target = call_info_p->new_target;
      TEST_ASSERT (jerry_value_is_undefined (target));
      TEST_ASSERT (target != call_info_p->function);
      break;
    }
    case TEST_ID_CONSTRUCT_AND_CALL_SUB:
    {
      /* Method was called with "new": new.target should be equal to the function object. */
      jerry_value_t target = call_info_p->new_target;
      TEST_ASSERT (!jerry_value_is_undefined (target));
      TEST_ASSERT (target == call_info_p->function);

      /* Calling a function should hide the old "new.target". */
      jerry_value_t sub_arg = jerry_number (TEST_ID_SIMPLE_CALL);
      jerry_value_t func_call_result;

      func_call_result = jerry_call (call_info_p->function, call_info_p->this_value, &sub_arg, 1);
      TEST_ASSERT (!jerry_value_is_exception (func_call_result));
      TEST_ASSERT (jerry_value_is_undefined (func_call_result));
      break;
    }

    default:
    {
      TEST_ASSERT (0 && "Incorrect test ID");
      break;
    }
  }

  return jerry_undefined ();
} /* construct_handler */

int
main (void)
{
  jerry_init (JERRY_INIT_EMPTY);

  jerry_value_t demo_func = register_js_function ("Demo", construct_handler);

  {
    jerry_value_t test_arg = jerry_number (TEST_ID_SIMPLE_CONSTRUCT);
    jerry_value_t constructed = jerry_construct (demo_func, &test_arg, 1);
    TEST_ASSERT (!jerry_value_is_exception (constructed));
    TEST_ASSERT (jerry_value_is_object (constructed));
    jerry_value_free (test_arg);
    jerry_value_free (constructed);
  }

  {
    jerry_value_t test_arg = jerry_number (TEST_ID_SIMPLE_CALL);
    jerry_value_t this_arg = jerry_undefined ();
    jerry_value_t constructed = jerry_call (demo_func, this_arg, &test_arg, 1);
    TEST_ASSERT (jerry_value_is_undefined (constructed));
    jerry_value_free (constructed);
    jerry_value_free (constructed);
    jerry_value_free (test_arg);
  }

  {
    jerry_value_t test_arg = jerry_number (TEST_ID_CONSTRUCT_AND_CALL_SUB);
    jerry_value_t constructed = jerry_construct (demo_func, &test_arg, 1);
    TEST_ASSERT (!jerry_value_is_exception (constructed));
    TEST_ASSERT (jerry_value_is_object (constructed));
    jerry_value_free (test_arg);
    jerry_value_free (constructed);
  }

  {
    static const jerry_char_t test_source[] = TEST_STRING_LITERAL ("new Demo (1)");

    jerry_value_t parsed_code_val = jerry_parse (test_source, sizeof (test_source) - 1, NULL);
    TEST_ASSERT (!jerry_value_is_exception (parsed_code_val));

    jerry_value_t res = jerry_run (parsed_code_val);
    TEST_ASSERT (!jerry_value_is_exception (res));

    jerry_value_free (res);
    jerry_value_free (parsed_code_val);
  }

  {
    static const jerry_char_t test_source[] = TEST_STRING_LITERAL ("Demo (2)");

    jerry_value_t parsed_code_val = jerry_parse (test_source, sizeof (test_source) - 1, NULL);
    TEST_ASSERT (!jerry_value_is_exception (parsed_code_val));

    jerry_value_t res = jerry_run (parsed_code_val);
    TEST_ASSERT (!jerry_value_is_exception (res));

    jerry_value_free (res);
    jerry_value_free (parsed_code_val);
  }

  {
    static const jerry_char_t test_source[] = TEST_STRING_LITERAL ("function base(arg) { new Demo (arg); };"
                                                                   "base (1);"
                                                                   "new base(1);"
                                                                   "new base(3);");

    jerry_value_t parsed_code_val = jerry_parse (test_source, sizeof (test_source) - 1, NULL);
    TEST_ASSERT (!jerry_value_is_exception (parsed_code_val));

    jerry_value_t res = jerry_run (parsed_code_val);
    TEST_ASSERT (!jerry_value_is_exception (res));

    jerry_value_free (res);
    jerry_value_free (parsed_code_val);
  }

  jerry_value_free (demo_func);
  jerry_cleanup ();
  return 0;
} /* main */
