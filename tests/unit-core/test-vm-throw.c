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

static int mode = 0;
static int counter = 0;

static void
vm_throw_callback (const jerry_value_t error_value, /**< captured error */
                   void *user_p) /**< user pointer */
{
  TEST_ASSERT (user_p == (void *) &mode);
  counter++;

  switch (mode)
  {
    case 0:
    {
      TEST_ASSERT (counter == 1);
      TEST_ASSERT (jerry_value_is_number (error_value) && jerry_value_as_number (error_value) == -5.6);
      break;
    }
    case 1:
    {
      TEST_ASSERT (counter == 1);
      TEST_ASSERT (jerry_value_is_null (error_value));
      break;
    }
    case 2:
    {
      jerry_char_t string_buf[2];
      jerry_size_t size = sizeof (string_buf);

      string_buf[0] = '\0';
      string_buf[1] = '\0';

      TEST_ASSERT (counter >= 1 && counter <= 3);
      TEST_ASSERT (jerry_value_is_string (error_value));
      TEST_ASSERT (jerry_string_size (error_value, JERRY_ENCODING_CESU8) == size);
      TEST_ASSERT (jerry_string_to_buffer (error_value, JERRY_ENCODING_CESU8, string_buf, size) == size);
      TEST_ASSERT (string_buf[0] == 'e' && string_buf[1] == (char) ('0' + counter));
      break;
    }
    case 3:
    {
      TEST_ASSERT (counter == 1);
      TEST_ASSERT (jerry_error_type (error_value) == JERRY_ERROR_RANGE);
      break;
    }
    case 4:
    {
      TEST_ASSERT (mode == 4);
      TEST_ASSERT (counter >= 1 && counter <= 2);

      jerry_error_t error = (counter == 1) ? JERRY_ERROR_REFERENCE : JERRY_ERROR_TYPE;
      TEST_ASSERT (jerry_error_type (error_value) == error);
      break;
    }
    case 5:
    case 6:
    {
      TEST_ASSERT (counter >= 1 && counter <= 2);
      TEST_ASSERT (jerry_value_is_false (error_value));
      break;
    }
    default:
    {
      TEST_ASSERT (mode == 8 || mode == 9);
      TEST_ASSERT (counter == 1);
      TEST_ASSERT (jerry_value_is_true (error_value));
      break;
    }
  }
} /* vm_throw_callback */

static jerry_value_t
native_handler (const jerry_call_info_t *call_info_p, /**< call info */
                const jerry_value_t args_p[], /**< arguments */
                const jerry_length_t args_count) /**< arguments length */
{
  (void) call_info_p;
  (void) args_p;
  TEST_ASSERT (args_count == 0);

  if (mode == 7)
  {
    jerry_value_t result = jerry_throw_sz (JERRY_ERROR_COMMON, "Error!");

    TEST_ASSERT (!jerry_exception_is_captured (result));
    jerry_exception_allow_capture (result, false);
    TEST_ASSERT (jerry_exception_is_captured (result));
    return result;
  }

  jerry_char_t source[] = TEST_STRING_LITERAL ("throw false");
  jerry_value_t result = jerry_eval (source, sizeof (source) - 1, JERRY_PARSE_NO_OPTS);

  TEST_ASSERT (jerry_exception_is_captured (result));

  if (mode == 6)
  {
    jerry_exception_allow_capture (result, true);
    TEST_ASSERT (!jerry_exception_is_captured (result));
  }
  return result;
} /* native_handler */

static void
do_eval (const char *script_p, /**< script to evaluate */
         bool should_throw) /**< script throws an error */
{
  jerry_value_t result = jerry_eval ((const jerry_char_t *) script_p, strlen (script_p), JERRY_PARSE_NO_OPTS);
  TEST_ASSERT (jerry_value_is_exception (result) == should_throw);
  jerry_value_free (result);
} /* do_eval */

int
main (void)
{
  TEST_INIT ();

  /* Test stopping an infinite loop. */
  if (!jerry_feature_enabled (JERRY_FEATURE_VM_THROW))
  {
    return 0;
  }

  jerry_init (JERRY_INIT_EMPTY);

  jerry_on_throw (vm_throw_callback, (void *) &mode);

  mode = 0;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("throw -5.6"), true);
  TEST_ASSERT (counter == 1);

  mode = 1;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("function f() { throw null }\n"
                                "function g() { f() }\n"
                                "g()\n"),
           true);
  TEST_ASSERT (counter == 1);

  mode = 2;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("function f() { throw 'e1' }\n"
                                "function g() { try { f() } catch (e) { throw 'e2' } }\n"
                                "try { g() } catch (e) { throw 'e3' }\n"),
           true);
  TEST_ASSERT (counter == 3);

  mode = 3;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("function f() { throw new RangeError() }\n"
                                "function g() { try { f() } finally { } }\n"
                                "try { g() } finally { }\n"),
           true);
  TEST_ASSERT (counter == 1);

  mode = 4;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("function f() { unresolved }\n"
                                "function g() { try { f() } finally { null.member } }\n"
                                "try { g() } finally { }\n"),
           true);
  TEST_ASSERT (counter == 2);

  /* Native functions may trigger the call twice: */
  jerry_value_t global_object_value = jerry_current_realm ();
  jerry_value_t function_value = jerry_function_external (native_handler);
  jerry_value_t function_name_value = jerry_string_sz ("native");

  jerry_value_free (jerry_object_set (global_object_value, function_name_value, function_value));
  jerry_value_free (function_name_value);
  jerry_value_free (function_value);
  jerry_value_free (global_object_value);

  mode = 5;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("native()\n"), true);
  TEST_ASSERT (counter == 1);

  mode = 6;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("native()\n"), true);
  TEST_ASSERT (counter == 2);

  mode = 7;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("native()\n"), true);
  TEST_ASSERT (counter == 0);

  /* Built-in functions should not trigger the call twice: */
  mode = 8;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("function f() { eval('eval(\\'throw true\\')') }\n"
                                "f()\n"),
           true);
  TEST_ASSERT (counter == 1);

  mode = 9;
  counter = 0;
  do_eval (TEST_STRING_LITERAL ("function f() { [1].map(function() { throw true }) }\n"
                                "f()\n"),
           true);
  TEST_ASSERT (counter == 1);

  jerry_value_t value = jerry_object ();
  TEST_ASSERT (!jerry_exception_is_captured (value));
  jerry_exception_allow_capture (value, false);
  TEST_ASSERT (!jerry_exception_is_captured (value));
  jerry_value_free (value);

  jerry_cleanup ();
  return 0;
} /* main */
