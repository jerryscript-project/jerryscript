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

#include "config.h"
#include "test-common.h"

static jerry_value_t
backtrace_handler (const jerry_call_info_t *call_info_p, /**< call information */
                   const jerry_value_t args_p[], /**< argument list */
                   const jerry_length_t args_count) /**< argument count */
{
  JERRY_UNUSED (call_info_p);

  uint32_t max_depth = 0;

  if (args_count >= 1 && jerry_value_is_number (args_p[0]))
  {
    max_depth = (uint32_t) jerry_value_as_number (args_p[0]);
  }

  return jerry_backtrace (max_depth);
} /* backtrace_handler */

static void
compare_string (jerry_value_t left_value, /* string value */
                const char *right_p) /* string to compare */
{
  jerry_char_t buffer[64];
  size_t length = strlen (right_p);

  TEST_ASSERT (length <= sizeof (buffer));
  TEST_ASSERT (jerry_value_is_string (left_value));
  TEST_ASSERT (jerry_string_size (left_value, JERRY_ENCODING_CESU8) == length);

  TEST_ASSERT (jerry_string_to_buffer (left_value, JERRY_ENCODING_CESU8, buffer, sizeof (buffer)) == length);
  TEST_ASSERT (memcmp (buffer, right_p, length) == 0);
} /* compare_string */

static const jerry_value_t *handler_args_p;
static int frame_index;

static bool
backtrace_callback (jerry_frame_t *frame_p, /* frame information */
                    void *user_p) /* user data */
{
  TEST_ASSERT ((void *) handler_args_p == user_p);
  TEST_ASSERT (jerry_frame_type (frame_p) == JERRY_BACKTRACE_FRAME_JS);

  const jerry_frame_location_t *location_p = jerry_frame_location (frame_p);
  const jerry_value_t *function_p = jerry_frame_callee (frame_p);
  const jerry_value_t *this_p = jerry_frame_this (frame_p);

  TEST_ASSERT (location_p != NULL);
  TEST_ASSERT (function_p != NULL);
  TEST_ASSERT (this_p != NULL);

  compare_string (location_p->source_name, "capture_test.js");

  ++frame_index;

  if (frame_index == 1)
  {
    TEST_ASSERT (!jerry_frame_is_strict (frame_p));
    TEST_ASSERT (location_p->line == 2);
    TEST_ASSERT (location_p->column == 3);
    TEST_ASSERT (handler_args_p[0] == *function_p);
    TEST_ASSERT (handler_args_p[1] == *this_p);
    return true;
  }

  if (frame_index == 2)
  {
    TEST_ASSERT (jerry_frame_is_strict (frame_p));
    TEST_ASSERT (location_p->line == 7);
    TEST_ASSERT (location_p->column == 6);
    TEST_ASSERT (handler_args_p[2] == *function_p);
    TEST_ASSERT (jerry_value_is_undefined (*this_p));
    return true;
  }

  jerry_value_t global = jerry_current_realm ();

  TEST_ASSERT (frame_index == 3);
  TEST_ASSERT (!jerry_frame_is_strict (frame_p));
  TEST_ASSERT (location_p->line == 11);
  TEST_ASSERT (location_p->column == 3);
  TEST_ASSERT (handler_args_p[3] == *function_p);
  TEST_ASSERT (global == *this_p);

  jerry_value_free (global);
  return false;
} /* backtrace_callback */

static bool
async_backtrace_callback (jerry_frame_t *frame_p, /* frame information */
                          void *user_p) /* user data */
{
  TEST_ASSERT ((void *) handler_args_p == user_p);
  TEST_ASSERT (jerry_frame_type (frame_p) == JERRY_BACKTRACE_FRAME_JS);

  const jerry_frame_location_t *location_p = jerry_frame_location (frame_p);
  const jerry_value_t *function_p = jerry_frame_callee (frame_p);

  TEST_ASSERT (location_p != NULL);
  TEST_ASSERT (function_p != NULL);

  compare_string (location_p->source_name, "async_capture_test.js");

  ++frame_index;

  if (frame_index == 1)
  {
    TEST_ASSERT (jerry_frame_is_strict (frame_p));
    TEST_ASSERT (location_p->line == 3);
    TEST_ASSERT (location_p->column == 3);
    TEST_ASSERT (handler_args_p[0] == *function_p);
    return true;
  }

  TEST_ASSERT (frame_index == 2);
  TEST_ASSERT (!jerry_frame_is_strict (frame_p));
  TEST_ASSERT (location_p->line == 8);
  TEST_ASSERT (location_p->column == 3);
  TEST_ASSERT (handler_args_p[1] == *function_p);
  return true;
} /* async_backtrace_callback */

static bool
class_backtrace_callback (jerry_frame_t *frame_p, /* frame information */
                          void *user_p) /* user data */
{
  TEST_ASSERT ((void *) handler_args_p == user_p);
  TEST_ASSERT (jerry_frame_type (frame_p) == JERRY_BACKTRACE_FRAME_JS);

  const jerry_frame_location_t *location_p = jerry_frame_location (frame_p);
  const jerry_value_t *function_p = jerry_frame_callee (frame_p);

  TEST_ASSERT (location_p != NULL);
  TEST_ASSERT (function_p != NULL);

  compare_string (location_p->source_name, "class_capture_test.js");

  ++frame_index;

  if (frame_index == 1)
  {
    TEST_ASSERT (jerry_frame_is_strict (frame_p));
    TEST_ASSERT (location_p->line == 3);
    TEST_ASSERT (location_p->column == 14);
    return false;
  }

  TEST_ASSERT (frame_index == 2);
  TEST_ASSERT (jerry_frame_is_strict (frame_p));
  TEST_ASSERT (location_p->line == 2);
  TEST_ASSERT (location_p->column == 7);
  return false;
} /* class_backtrace_callback */

static jerry_value_t
capture_handler (const jerry_call_info_t *call_info_p, /**< call information */
                 const jerry_value_t args_p[], /**< argument list */
                 const jerry_length_t args_count) /**< argument count */
{
  JERRY_UNUSED (call_info_p);

  TEST_ASSERT (args_count == 0 || args_count == 2 || args_count == 4);
  TEST_ASSERT (args_count == 0 || frame_index == 0);

  jerry_backtrace_cb_t callback = backtrace_callback;

  if (args_count == 0)
  {
    callback = class_backtrace_callback;
  }
  else if (args_count == 2)
  {
    callback = async_backtrace_callback;
  }

  handler_args_p = args_p;
  jerry_backtrace_capture (callback, (void *) args_p);

  TEST_ASSERT (args_count == 0 || frame_index == (args_count == 4 ? 3 : 2));

  return jerry_undefined ();
} /* capture_handler */

static bool
global_backtrace_callback (jerry_frame_t *frame_p, /* frame information */
                           void *user_p) /* user data */
{
  TEST_ASSERT (user_p != NULL && frame_index == 0);
  frame_index++;

  const jerry_value_t *function_p = jerry_frame_callee (frame_p);
  jerry_value_t *result_p = ((jerry_value_t *) user_p);

  TEST_ASSERT (function_p != NULL);
  jerry_value_free (*result_p);
  *result_p = jerry_value_copy (*function_p);
  return true;
} /* global_backtrace_callback */

static jerry_value_t
global_capture_handler (const jerry_call_info_t *call_info_p, /**< call information */
                        const jerry_value_t args_p[], /**< argument list */
                        const jerry_length_t args_count) /**< argument count */
{
  JERRY_UNUSED (call_info_p);
  JERRY_UNUSED (args_p);
  JERRY_UNUSED (args_count);

  jerry_value_t result = jerry_undefined ();
  jerry_backtrace_capture (global_backtrace_callback, &result);

  TEST_ASSERT (jerry_value_is_object (result));
  return result;
} /* global_capture_handler */

static void
register_callback (jerry_external_handler_t handler_p, /**< callback function */
                   char *name_p) /**< name of the function */
{
  jerry_value_t global = jerry_current_realm ();

  jerry_value_t func = jerry_function_external (handler_p);
  jerry_value_t name = jerry_string_sz (name_p);
  jerry_value_t result = jerry_object_set (global, name, func);
  TEST_ASSERT (!jerry_value_is_exception (result));

  jerry_value_free (result);
  jerry_value_free (name);
  jerry_value_free (func);

  jerry_value_free (global);
} /* register_callback */

static jerry_value_t
run (const char *source_name_p, /**< source name */
     const char *source_p) /**< source code */
{
  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_HAS_SOURCE_NAME;
  parse_options.source_name = jerry_string_sz (source_name_p);

  jerry_value_t code = jerry_parse ((const jerry_char_t *) source_p, strlen (source_p), &parse_options);
  jerry_value_free (parse_options.source_name);
  TEST_ASSERT (!jerry_value_is_exception (code));

  jerry_value_t result = jerry_run (code);
  jerry_value_free (code);

  return result;
} /* run */

static void
compare (jerry_value_t array, /**< array */
         uint32_t index, /**< item index */
         const char *str) /**< string to compare */
{
  jerry_char_t buf[64];

  size_t len = strlen (str);

  TEST_ASSERT (len < sizeof (buf));

  jerry_value_t value = jerry_object_get_index (array, index);

  TEST_ASSERT (!jerry_value_is_exception (value) && jerry_value_is_string (value));

  TEST_ASSERT (jerry_string_size (value, JERRY_ENCODING_CESU8) == len);

  jerry_size_t str_len = jerry_string_to_buffer (value, JERRY_ENCODING_CESU8, buf, (jerry_size_t) len);
  TEST_ASSERT (str_len == len);

  jerry_value_free (value);

  TEST_ASSERT (memcmp (buf, str, len) == 0);
} /* compare */

static void
test_get_backtrace_api_call (void)
{
  jerry_init (JERRY_INIT_EMPTY);

  register_callback (backtrace_handler, "backtrace");
  register_callback (capture_handler, "capture");

  const char *source_p = ("function f() {\n"
                          "  return backtrace(0);\n"
                          "}\n"
                          "\n"
                          "function g() {\n"
                          "  return f();\n"
                          "}\n"
                          "\n"
                          "function h() {\n"
                          "  return g();\n"
                          "}\n"
                          "\n"
                          "h();\n");

  jerry_value_t backtrace = run ("something.js", source_p);

  TEST_ASSERT (!jerry_value_is_exception (backtrace) && jerry_value_is_array (backtrace));

  TEST_ASSERT (jerry_array_length (backtrace) == 4);

  compare (backtrace, 0, "something.js:2:3");
  compare (backtrace, 1, "something.js:6:3");
  compare (backtrace, 2, "something.js:10:3");
  compare (backtrace, 3, "something.js:13:1");

  jerry_value_free (backtrace);

  /* Depth set to 2 this time. */

  source_p = ("function f() {\n"
              "  1; return backtrace(2);\n"
              "}\n"
              "\n"
              "function g() {\n"
              "  return f();\n"
              "}\n"
              "\n"
              "function h() {\n"
              "  return g();\n"
              "}\n"
              "\n"
              "h();\n");

  backtrace = run ("something_else.js", source_p);

  TEST_ASSERT (!jerry_value_is_exception (backtrace) && jerry_value_is_array (backtrace));

  TEST_ASSERT (jerry_array_length (backtrace) == 2);

  compare (backtrace, 0, "something_else.js:2:6");
  compare (backtrace, 1, "something_else.js:6:3");

  jerry_value_free (backtrace);

  /* Test frame capturing. */

  frame_index = 0;
  source_p = ("var o = { f:function() {\n"
              "  return capture(o.f, o, g, h);\n"
              "} }\n"
              "\n"
              "function g() {\n"
              "  'use strict';\n"
              "  1; return o.f();\n"
              "}\n"
              "\n"
              "function h() {\n"
              "  return g();\n"
              "}\n"
              "\n"
              "h();\n");

  jerry_value_t result = run ("capture_test.js", source_p);

  TEST_ASSERT (jerry_value_is_undefined (result));
  jerry_value_free (result);

  TEST_ASSERT (frame_index == 3);

  /* Test async frame capturing. */
  source_p = "async function f() {}";
  result = jerry_eval ((const jerry_char_t *) source_p, strlen (source_p), JERRY_PARSE_NO_OPTS);

  if (!jerry_value_is_exception (result))
  {
    jerry_value_free (result);

    frame_index = 0;
    source_p = ("function f() {\n"
                "  'use strict';\n"
                "  return capture(f, g);\n"
                "}\n"
                "\n"
                "async function g() {\n"
                "  await 0;\n"
                "  return f();\n"
                "}\n"
                "\n"
                "g();\n");

    result = run ("async_capture_test.js", source_p);

    TEST_ASSERT (jerry_value_is_promise (result));
    jerry_value_free (result);

    TEST_ASSERT (frame_index == 0);

    result = jerry_run_jobs ();
    TEST_ASSERT (!jerry_value_is_exception (result));

    TEST_ASSERT (frame_index == 2);
  }
  else
  {
    TEST_ASSERT (jerry_error_type (result) == JERRY_ERROR_SYNTAX);
  }

  jerry_value_free (result);

  /* Test class initializer frame capturing. */
  source_p = "class C {}";
  result = jerry_eval ((const jerry_char_t *) source_p, strlen (source_p), JERRY_PARSE_NO_OPTS);

  if (!jerry_value_is_exception (result))
  {
    jerry_value_free (result);

    frame_index = 0;
    source_p = ("class C {\n"
                "  a = capture();\n"
                "  static b = capture();\n"
                "}\n"
                "new C;\n");

    result = run ("class_capture_test.js", source_p);

    TEST_ASSERT (!jerry_value_is_exception (result));
    TEST_ASSERT (frame_index == 2);
  }
  else
  {
    TEST_ASSERT (jerry_error_type (result) == JERRY_ERROR_SYNTAX);
  }

  jerry_value_free (result);

  register_callback (global_capture_handler, "global_capture");

  frame_index = 0;

  source_p = "global_capture()";

  jerry_value_t code = jerry_parse ((const jerry_char_t *) source_p, strlen (source_p), NULL);
  TEST_ASSERT (!jerry_value_is_exception (code));

  result = jerry_run (code);

  jerry_value_t compare_value = jerry_binary_op (JERRY_BIN_OP_STRICT_EQUAL, result, code);
  TEST_ASSERT (jerry_value_is_true (compare_value));

  jerry_value_free (compare_value);
  jerry_value_free (result);
  jerry_value_free (code);

  jerry_cleanup ();
} /* test_get_backtrace_api_call */

static void
test_exception_backtrace (void)
{
  jerry_init (JERRY_INIT_EMPTY);

  const char *source = ("function f() {\n"
                        "  undef_reference;\n"
                        "}\n"
                        "\n"
                        "function g() {\n"
                        "  return f();\n"
                        "}\n"
                        "\n"
                        "g();\n");

  jerry_value_t error = run ("bad.js", source);

  TEST_ASSERT (jerry_value_is_exception (error));

  error = jerry_exception_value (error, true);

  TEST_ASSERT (jerry_value_is_object (error));

  jerry_value_t name = jerry_string_sz ("stack");
  jerry_value_t backtrace = jerry_object_get (error, name);

  jerry_value_free (name);
  jerry_value_free (error);

  TEST_ASSERT (!jerry_value_is_exception (backtrace) && jerry_value_is_array (backtrace));

  TEST_ASSERT (jerry_array_length (backtrace) == 3);

  compare (backtrace, 0, "bad.js:2:3");
  compare (backtrace, 1, "bad.js:6:3");
  compare (backtrace, 2, "bad.js:9:1");

  jerry_value_free (backtrace);

  jerry_cleanup ();
} /* test_exception_backtrace */

static void
test_large_line_count (void)
{
  jerry_init (JERRY_INIT_EMPTY);

  const char *source = ("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
                        "g();\n");

  jerry_value_t error = run ("bad.js", source);

  TEST_ASSERT (jerry_value_is_exception (error));

  error = jerry_exception_value (error, true);

  TEST_ASSERT (jerry_value_is_object (error));

  jerry_value_t name = jerry_string_sz ("stack");
  jerry_value_t backtrace = jerry_object_get (error, name);

  jerry_value_free (name);
  jerry_value_free (error);

  TEST_ASSERT (!jerry_value_is_exception (backtrace) && jerry_value_is_array (backtrace));

  TEST_ASSERT (jerry_array_length (backtrace) == 1);

  compare (backtrace, 0, "bad.js:385:1");

  jerry_value_free (backtrace);

  jerry_cleanup ();
} /* test_large_line_count */

int
main (void)
{
  TEST_INIT ();

  TEST_ASSERT (jerry_feature_enabled (JERRY_FEATURE_LINE_INFO));

  test_get_backtrace_api_call ();
  test_exception_backtrace ();
  test_large_line_count ();

  return 0;
} /* main */
