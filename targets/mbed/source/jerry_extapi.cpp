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

#include <stdlib.h>
#include <stdio.h>

#include "jerry-core/jerryscript.h"
#include "jerry_extapi.h"

#include "native_mbed.h"

#ifndef MIN
#define MIN(A,B) ((A)<(B)?(A):(B))
#endif

//-----------------------------------------------------------------------------

#define __UNSED__ __attribute__((unused))

#define DECLARE_HANDLER(NAME) \
static jerry_value_t \
NAME ## _handler (const jerry_value_t func_value __UNSED__, \
                  const jerry_value_t this_value __UNSED__, \
                  const jerry_value_t args[], \
                  const jerry_length_t args_cnt )

#define REGISTER_HANDLER(NAME) \
  register_native_function ( # NAME, NAME ## _handler)

//-----------------------------------------------------------------------------

DECLARE_HANDLER(assert)
{
  if (args_cnt == 1
      && jerry_value_is_boolean (args[0])
      && jerry_get_boolean_value (args[0]))
  {
    printf (">> Jerry assert true\r\n");
    return jerry_create_boolean (true);
  }
  printf ("ERROR: Script assertion failed\n");
  exit (JERRY_STANDALONE_EXIT_CODE_FAIL);
  return jerry_create_boolean (false);
}

DECLARE_HANDLER(led)
{
  jerry_value_t ret_val;

  if (args_cnt < 2)
  {
    ret_val = jerry_create_boolean (false);
    printf ("Error: invalid arguments number!\r\n");
    return ret_val;
  }

  if (!(jerry_value_is_number (args[0])
        && jerry_value_is_number (args[1])))
  {
    ret_val = jerry_create_boolean (false);
    printf ("Error: arguments must be numbers!\r\n");
    return ret_val;
  }

  int port, value;
  port = (int) jerry_get_number_value (args[0]);
  value = (int) jerry_get_number_value (args[1]);

  if (port >= 0 && port <= 3)
  {
    native_led (port, value);
    ret_val = jerry_create_boolean (true);
  }
  else
  {
    ret_val = jerry_create_boolean (false);
  }
  return ret_val;
}

//-----------------------------------------------------------------------------

static bool
register_native_function (const char* name,
                          jerry_external_handler_t handler)
{
  jerry_value_t global_object_val = jerry_get_global_object ();
  jerry_value_t reg_function = jerry_create_external_function (handler);

  bool is_ok = true; 

  if (!(jerry_value_is_function (reg_function)
        && jerry_value_is_constructor (reg_function)))
  {
    is_ok = false;
    printf ("Error: create_external_function failed !!!\r\n");
    jerry_release_value (global_object_val);
    jerry_release_value (reg_function);
    return is_ok;
  }

  if (jerry_value_has_error_flag (reg_function))
  {
    is_ok = false;
    printf ("Error: create_external_function has error flag! \n\r");
    jerry_release_value (global_object_val);
    jerry_release_value (reg_function);
    return is_ok;
  }

  jerry_value_t jerry_name = jerry_create_string ((jerry_char_t *) name);

  jerry_value_t set_result = jerry_set_property (global_object_val,
                                                 jerry_name,
                                                 reg_function);


  if (jerry_value_has_error_flag (set_result))
  {
    is_ok = false;
    printf ("Error: register_native_function failed: [%s]\r\n", name);
  }

  jerry_release_value (jerry_name);
  jerry_release_value (global_object_val);
  jerry_release_value (reg_function);
  jerry_release_value (set_result);

  return is_ok;
}

void js_register_functions (void)
{
  REGISTER_HANDLER (assert);
  REGISTER_HANDLER (led);
}
