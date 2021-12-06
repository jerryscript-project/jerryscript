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
#include "jerryscript-mbed-library-registry/wrap_tools.h"

#include <stdio.h>
#include <stdlib.h>

bool
jsmbed_wrap_register_global_function (const char* name, jerry_external_handler_t handler)
{
  jerry_value_t global_object_val = jerry_current_realm ();
  jerry_value_t reg_function = jerry_function_external (handler);

  bool is_ok = true;

  if (!(jerry_value_is_function (reg_function) && jerry_value_is_constructor (reg_function)))
  {
    is_ok = false;
    LOG_PRINT_ALWAYS ("Error: jerry_function_external failed!\r\n");
    jerry_value_free (global_object_val);
    jerry_value_free (reg_function);
    return is_ok;
  }

  if (jerry_value_is_exception (reg_function))
  {
    is_ok = false;
    LOG_PRINT_ALWAYS ("Error: jerry_function_external has error flag! \r\n");
    jerry_value_free (global_object_val);
    jerry_value_free (reg_function);
    return is_ok;
  }

  jerry_value_t jerry_name = jerry_string_sz (name);

  jerry_value_t set_result = jerry_object_set (global_object_val, jerry_name, reg_function);

  if (jerry_value_is_exception (set_result))
  {
    is_ok = false;
    LOG_PRINT_ALWAYS ("Error: jerry_function_external failed: [%s]\r\n", name);
  }

  jerry_value_free (jerry_name);
  jerry_value_free (global_object_val);
  jerry_value_free (reg_function);
  jerry_value_free (set_result);

  return is_ok;
}

bool
jsmbed_wrap_register_class_constructor (const char* name, jerry_external_handler_t handler)
{
  // Register class constructor as a global function
  return jsmbed_wrap_register_global_function (name, handler);
}

bool
jsmbed_wrap_register_class_function (jerry_value_t this_obj, const char* name, jerry_external_handler_t handler)
{
  jerry_value_t property_name = jerry_string_sz (name);
  jerry_value_t handler_obj = jerry_function_external (handler);

  jerry_value_free (jerry_object_set (this_obj, property_name, handler_obj));

  jerry_value_free (handler_obj);
  jerry_value_free (property_name);

  // TODO: check for errors, and return false in the case of errors
  return true;
}
