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
#include "jerry_extapi.h"
#include "jerryscript-port.h"
#include "jerryscript.h"
#include "user_external.h"

#define GLOBAL_PRINT "print"
#define GLOBAL_ASSERT "assert"

/**
 * Register a JavaScript value in the given object.
 */
void
register_js_value_to_object (char *name_p, /**< name of the function */
                             jerry_value_t value, /**< function callback */
                             jerry_value_t object) /**< destination object */
{
  jerry_value_t name_val = jerry_create_string ((const jerry_char_t *) name_p);
  jerry_value_t result_val = jerry_set_property (object, name_val, value);

  jerry_release_value (name_val);
  jerry_release_value (result_val);
} /* register_js_value_to_object */

/**
 * Register a number property to the given object.
 */
void
register_number_to_object (char *name, /**< name of the function */
                           double number, /**< double value */
                           jerry_value_t object) /**< destination object */
{
  jerry_value_t value = jerry_create_number (number);
  register_js_value_to_object (name, value, object);
  jerry_release_value (value);
} /* register_number_to_object */


jerry_value_t
raise_argument_count_error (char *object, /**< name of the object */
                            char *property, /**< name of the object's property */
                            int expected_argument_count) /**< expected number of arguments */
{
  char buff[256];
  snprintf (buff, sizeof (buff), "%s.%s function requires %d parameter(s)!", object, property, expected_argument_count);
  return jerry_create_error (JERRY_ERROR_COMMON, (const jerry_char_t *) buff);
} /* raise_argument_count_error */

jerry_value_t
raise_argument_type_error (int arg_count, /**< index of argument */
                           char *type) /**< expected argument type */
{
  char buff[256];
  snprintf (buff, sizeof (buff), "Argument number (%d)  must be a(n) %s!", arg_count, type);
  return jerry_create_error (JERRY_ERROR_TYPE, (const jerry_char_t *) buff);
} /* raise_argument_type_error */

bool
register_native_function (char *name, /**< name of the native function */
                          jerry_external_handler_t handler, /**< native handler */
                          jerry_value_t object) /**< destination object */
{
  jerry_value_t reg_func_val = jerry_create_external_function (handler);

  if (!(jerry_value_is_function (reg_func_val)
        && jerry_value_is_constructor (reg_func_val)))
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "create_external_function failed!");
    jerry_release_value (reg_func_val);
    return false;
  }

  jerry_value_t prop_name_val = jerry_create_string ((const jerry_char_t *) name);
  jerry_value_t res = jerry_set_property (object, prop_name_val, reg_func_val);

  jerry_release_value (reg_func_val);
  jerry_release_value (prop_name_val);

  if (jerry_value_is_error (res))
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "register_native_function failed: [%s]", name);
    jerry_release_value (res);
    return false;
  }

  jerry_release_value (res);

  return true;
} /* register_native_function */

DELCARE_HANDLER (assert)
{
  if (args_cnt == 1
      && jerry_value_is_boolean (args_p[0])
      && jerry_get_boolean_value (args_p[0]))
  {
    return jerry_create_boolean (true);
  }

  jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Assertion failed");
  exit (JERRY_STANDALONE_EXIT_CODE_FAIL);
  return jerry_create_boolean (false);
} /* assert */


DELCARE_HANDLER (print)
{
  jerry_value_t ret_val = jerry_create_undefined ();

  for (jerry_length_t arg_index = 0;
       jerry_value_is_undefined (ret_val) && arg_index < args_cnt;
       arg_index++)
  {
    jerry_value_t str_val = jerry_value_to_string (args_p[arg_index]);

    if (!jerry_value_is_error (str_val))
    {
      if (arg_index != 0)
      {
        printf (" ");
      }

      jerry_size_t substr_size;
      jerry_length_t substr_pos = 0;
      jerry_char_t substr_buf[256];

      while ((substr_size = jerry_substring_to_char_buffer (str_val,
                                                            substr_pos,
                                                            substr_pos + 256,
                                                            substr_buf,
                                                            256)) != 0)
      {
#ifdef JERRY_DEBUGGER
        jerry_debugger_send_output (substr_buf, substr_size, 1);
#endif /* JERRY_DEBUGGER */
        printf ("%.*s", substr_size, substr_buf);

        substr_pos += substr_size;
      }

      jerry_release_value (str_val);
    }
    else
    {
      ret_val = str_val;
    }
  }

  printf ("\n");

  return jerry_create_boolean (true);
} /* print */


void
register_js_entries (void)
{
  jerry_value_t global_object = jerry_get_global_object ();
  register_native_function (GLOBAL_ASSERT, assert_handler, global_object);
  register_native_function (GLOBAL_PRINT, print_handler, global_object);

  register_modules (global_object);
  register_user_external ();

  jerry_release_value (global_object);
} /* register_js_entries */
