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

#include <string.h>
#include "jerryscript.h"
#include "jerryscript-ext/handler.h"

/**
 * Add param to 'this.x'
 */
static jerry_value_t
add_handler (const jerry_value_t func_value, /**< function object */
             const jerry_value_t this_val, /**< this arg */
             const jerry_value_t *args_p, /**< function arguments */
             const jerry_length_t args_cnt) /**< number of function arguments */
{
  /* Get 'this.x' */
  jerry_value_t prop_name = jerry_create_string ((const jerry_char_t *) "x");
  jerry_value_t x_val = jerry_get_property (this_val, prop_name);

  if (!jerry_value_has_error_flag (x_val))
  {
    /* Convert Jerry API values to double */
    double x = jerry_get_number_value (x_val);
    double d = jerry_get_number_value (*args_p);

    /* Add the parameter to 'x' */
    jerry_value_t res_val = jerry_create_number (x + d);

    /* Set the new value of 'this.x' */
    jerry_set_property (this_val, prop_name, res_val);
    jerry_release_value (res_val);
  }

  jerry_release_value (x_val);
  jerry_release_value (prop_name);

  return jerry_create_undefined ();
} /* add_handler */

int
main (int argc, char *argv[])
{
  /* Initialize engine */
  jerry_init (JERRY_INIT_EMPTY);

  /* Register 'print' function from the extensions */
  jerryx_handler_register_global ((const jerry_char_t *) "print",
                                  jerryx_handler_print);

  /* Create a JS object */
  const jerry_char_t my_js_object[] = " \
    MyObject = \
    { x : 12, \
      y : 'Value of x is ', \
      foo: function () \
      { \
        return this.y + this.x; \
      } \
    } \
  ";

  jerry_value_t my_js_obj_val;

  /* Evaluate script */
  my_js_obj_val = jerry_eval (my_js_object,
                              strlen ((const char *) my_js_object),
                              false);

  /* Create a JS function object and wrap into a jerry value */
  jerry_value_t add_func_obj = jerry_create_external_function (add_handler);

  /* Set the native function as a property of previously created MyObject */
  jerry_value_t prop_name = jerry_create_string ((const jerry_char_t *) "add2x");
  jerry_set_property (my_js_obj_val, prop_name, add_func_obj);
  jerry_release_value (add_func_obj);
  jerry_release_value (prop_name);

  /* Free JavaScript value, returned by eval (my_js_object) */
  jerry_release_value (my_js_obj_val);

  const jerry_char_t script[] = " \
    var str = MyObject.foo (); \
    print (str); \
    MyObject.add2x (5); \
    print (MyObject.foo ()); \
  ";
  size_t script_size = strlen ((const char *) script);

  /* Evaluate script */
  jerry_value_t eval_ret = jerry_eval (script, script_size, false);

  /* Free JavaScript value, returned by eval */
  jerry_release_value (eval_ret);

  /* Cleanup engine */
  jerry_cleanup ();

  return 0;
}
