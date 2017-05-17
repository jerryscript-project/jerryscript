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

/**
 * Unit test for jerry-ext/args.
 */

#include "jerryscript.h"
#include "jerryscript-ext/arg.h"
#include "test-common.h"

#include <string.h>

const char *test_source = (
                           "var arg1 = true;"
                           "var arg2 = 10.5;"
                           "var arg3 = 'abc';"
                           "var arg4 = function foo() {};"
                           "test_validator1(arg1, arg2, arg3, arg4);"
                           "arg1 = new Boolean(true);"
                           "arg3 = new String('abc');"
                           "test_validator1(arg1, arg2, arg3);"
                           "arg2 = new Number(10.5);"
                           "test_validator1(arg1, arg2, arg3);"
                           "test_validator1(arg1, 10.5, 'abcdef');"
                           "var obj_a = new MyObjectA();"
                           "var obj_b = new MyObjectB();"
                           "test_validator2.call(obj_a, 5);"
                           "test_validator2.call(obj_b, 5);"
                           "test_validator2.call(obj_a, 1);"
                           );

static const jerry_object_native_info_t thing_a_info =
{
  .free_cb = NULL
};

static const jerry_object_native_info_t thing_b_info =
{
  .free_cb = NULL
};

typedef struct
{
  int x;
} my_type_a_t;

typedef struct
{
  bool x;
} my_type_b_t;

static my_type_a_t my_thing_a;
static my_type_b_t my_thing_b;

static int validator1_count = 0;
static int validator2_count = 0;

/**
 * The handler should have following arguments:
 *   this: Ignore.
 *   arg1: Bool.
 *   arg2: Number. It must be strict primitive number.
 *   arg3: String.
 *   arg4: function. It is an optional argument.
 *
 */
static jerry_value_t
test_validator1_handler (const jerry_value_t func_obj_val __attribute__((unused)), /**< function object */
                         const jerry_value_t this_val, /**< this value */
                         const jerry_value_t args_p[], /**< arguments list */
                         const jerry_length_t args_cnt) /**< arguments length */
{

  bool arg1;
  double arg2 = 0.0;
  char arg3[5] = "1234";
  jerry_value_t arg4 = jerry_create_undefined ();

  jerryx_arg_t mapping[] =
  {
    /* ignore this */
    jerryx_arg_ignore (),
    /* 1st argument should be boolean */
    jerryx_arg_boolean (&arg1, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED),
    /* 2nd argument should be strict number */
    jerryx_arg_number (&arg2, JERRYX_ARG_NO_COERCE, JERRYX_ARG_REQUIRED),
    /* 3th argument should be string */
    jerryx_arg_string (arg3, 5, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED),
    /* 4th argument should be function, and it is optional */
    jerryx_arg_function (&arg4, JERRYX_ARG_OPTIONAL)
  };

  jerry_value_t is_ok = jerryx_arg_transform_this_and_args (this_val,
                                                            args_p,
                                                            args_cnt,
                                                            mapping,
                                                            5);

  if (validator1_count == 0)
  {
    TEST_ASSERT (!jerry_value_has_error_flag (is_ok));
    TEST_ASSERT (arg1);
    TEST_ASSERT (arg2 == 10.5);
    TEST_ASSERT (strcmp (arg3, "abc") == 0);
    TEST_ASSERT (jerry_value_is_function (arg4));
  }
  else if (validator1_count == 1)
  {
    TEST_ASSERT (!jerry_value_has_error_flag (is_ok));
    TEST_ASSERT (arg1);
    TEST_ASSERT (arg2 == 10.5);
    TEST_ASSERT (strcmp (arg3, "abc") == 0);
    TEST_ASSERT (jerry_value_is_undefined (arg4));
  }
  else
  {
    TEST_ASSERT (jerry_value_has_error_flag (is_ok));
  }

  jerry_release_value (is_ok);
  jerry_release_value (arg4);
  validator1_count ++;

  return jerry_create_undefined ();
} /* test_validator1_handler */

/**
 * The JS argument should be number, whose value is equal with the extra_info .
 */
static jerry_value_t
my_custom_transform (jerryx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                     const jerryx_arg_t *c_arg_p) /**< the native arg */
{
  jerry_value_t js_arg = jerryx_arg_js_iterator_pop (js_arg_iter_p);
  jerry_value_t to_number = jerry_value_to_number (js_arg);

  if (jerry_value_has_error_flag (to_number))
  {
    jerry_release_value (to_number);

    return jerry_create_error (JERRY_ERROR_TYPE,
                               (jerry_char_t *) "It can not be converted to a number.");
  }

  int expected_num = (int) c_arg_p->extra_info;
  int get_num = (int) jerry_get_number_value (to_number);

  if (get_num != expected_num)
  {
    return jerry_create_error (JERRY_ERROR_TYPE,
                               (jerry_char_t *) "Number value is not expected.");
  }

  return jerry_create_undefined ();
} /* my_custom_transform */

/**
 * The handler should have following arguments:
 *   this: with native pointer whose type is bind_a_info.
 *   arg1: should pass the custom tranform function.
 */
static jerry_value_t
test_validator2_handler (const jerry_value_t func_obj_val __attribute__((unused)), /**< function object */
                         const jerry_value_t this_val, /**< this value */
                         const jerry_value_t args_p[], /**< arguments list */
                         const jerry_length_t args_cnt) /**< arguments length */
{
  my_type_a_t *thing_p;

  jerryx_arg_t mapping[] =
  {
    /* this should has native pointer, whose type is thing_a_info */
    jerryx_arg_native_pointer ((void **) &thing_p, &thing_a_info, JERRYX_ARG_REQUIRED),
    /* custom tranform function */
    jerryx_arg_custom (NULL, 5, my_custom_transform)
  };

  jerry_value_t is_ok = jerryx_arg_transform_this_and_args (this_val,
                                                            args_p,
                                                            args_cnt,
                                                            mapping,
                                                            2);

  if (validator2_count == 0)
  {
    TEST_ASSERT (!jerry_value_has_error_flag (is_ok));
    TEST_ASSERT (thing_p == &my_thing_a);
    TEST_ASSERT (thing_p->x == 1);
  }
  else
  {
    TEST_ASSERT (jerry_value_has_error_flag (is_ok));
  }

  jerry_release_value (is_ok);
  validator2_count ++;

  return jerry_create_undefined ();
} /* test_validator2_handler */

static jerry_value_t
create_object_a_handler (const jerry_value_t func_obj_val __attribute__((unused)), /**< function object */
                         const jerry_value_t this_val, /**< this value */
                         const jerry_value_t args_p[] __attribute__((unused)), /**< arguments list */
                         const jerry_length_t args_cnt __attribute__((unused))) /**< arguments length */
{
  TEST_ASSERT (jerry_value_is_object (this_val));

  my_thing_a.x = 1;
  jerry_set_object_native_pointer (this_val,
                                   &my_thing_a,
                                   &thing_a_info);

  return jerry_create_boolean (true);
} /* create_object_a_handler */

static jerry_value_t
create_object_b_handler (const jerry_value_t func_obj_val __attribute__((unused)), /**< function object */
                         const jerry_value_t this_val, /**< this value */
                         const jerry_value_t args_p[] __attribute__((unused)), /**< arguments list */
                         const jerry_length_t args_cnt __attribute__((unused))) /**< arguments length */
{
  TEST_ASSERT (jerry_value_is_object (this_val));

  my_thing_b.x = false;
  jerry_set_object_native_pointer (this_val,
                                   &my_thing_b,
                                   &thing_b_info);

  return jerry_create_boolean (true);
} /* create_object_b_handler */

/**
 * Register a JavaScript function in the global object.
 */
static void
register_js_function (const char *name_p, /**< name of the function */
                      jerry_external_handler_t handler_p) /**< function callback */
{
  jerry_value_t global_obj_val = jerry_get_global_object ();

  jerry_value_t function_val = jerry_create_external_function (handler_p);
  jerry_value_t function_name_val = jerry_create_string ((const jerry_char_t *) name_p);
  jerry_value_t result_val = jerry_set_property (global_obj_val, function_name_val, function_val);

  jerry_release_value (function_name_val);
  jerry_release_value (function_val);
  jerry_release_value (global_obj_val);

  jerry_release_value (result_val);
} /* register_js_function */

int
main (void)
{
  jerry_init (JERRY_INIT_EMPTY);

  register_js_function ("test_validator1", test_validator1_handler);
  register_js_function ("test_validator2", test_validator2_handler);
  register_js_function ("MyObjectA", create_object_a_handler);
  register_js_function ("MyObjectB", create_object_b_handler);

  jerry_value_t parsed_code_val = jerry_parse ((jerry_char_t *) test_source, strlen (test_source), false);
  TEST_ASSERT (!jerry_value_has_error_flag (parsed_code_val));

  jerry_value_t res = jerry_run (parsed_code_val);
  TEST_ASSERT (!jerry_value_has_error_flag (res));
  TEST_ASSERT (validator1_count == 4);
  TEST_ASSERT (validator2_count == 3);

  jerry_release_value (res);
  jerry_release_value (parsed_code_val);

  jerry_cleanup ();
} /* main */
