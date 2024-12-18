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

#include "jerryscript-ext/module.h"

#define MODULE_NAME my_custom_module

static void
jobject_set_property_jval (jerry_value_t jobj,
                           jerry_value_t prop_name, /**< property name value that will be free/take */
                           jerry_value_t value)
{
  jerry_value_t ret_val = jerry_object_set (jobj, prop_name, value);
  jerry_value_free (prop_name);
  jerry_value_free (ret_val);
} /* jobject_set_property_jval */

static jerry_value_t
call_function_with_callback (const jerry_call_info_t *call_info_p,
                             const jerry_value_t jargv[],
                             const jerry_length_t jargc)
{
  (void) jargc;
  jerry_value_t jval_func = jargv[0];
  return jerry_call (jval_func, call_info_p->this_value, NULL, 0);
} /* call_function_with_callback */

static jerry_value_t
my_custom_module_on_resolve (void)
{
  jerry_value_t mymodule = jerry_object ();
  jerry_value_t val = jerry_number (42);
  jobject_set_property_jval (mymodule, jerry_string_sz ("number_value"), val);
  jerry_value_free (val);

  jerry_value_t jfunc = jerry_function_external (call_function_with_callback);
  jobject_set_property_jval (mymodule, jerry_string_sz ("call_function_with_callback"), jfunc);
  jerry_value_free (jfunc);

  return mymodule;
} /* my_custom_module_on_resolve */

JERRYX_NATIVE_MODULE (MODULE_NAME, my_custom_module_on_resolve)
