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

int
main (int argc, char *argv[]) {
  const jerry_char_t str[] = "Hello, World!";
  const jerry_char_t script[] = "print (s);";

  /* Initializing JavaScript environment */
  jerry_init (JERRY_INIT_EMPTY);

  /* Register 'print' function from the extensions */
  jerryx_handler_register_global ((const jerry_char_t *) "print",
                                  jerryx_handler_print);

  /* Getting pointer to the Global object */
  jerry_value_t global_object = jerry_get_global_object ();

  /* Constructing strings */
  jerry_value_t prop_name = jerry_create_string ((const jerry_char_t *) "s");
  jerry_value_t prop_value = jerry_create_string (str);

  /* Setting the string value as a property of the Global object */
  jerry_set_property (global_object, prop_name, prop_value);

  /* Releasing string values, as it is no longer necessary outside of engine */
  jerry_release_value (prop_name);
  jerry_release_value (prop_value);

  /* Releasing the Global object */
  jerry_release_value (global_object);

  /* Now starting script that would output value of just initialized field */
  jerry_value_t eval_ret = jerry_eval (script,
                                       strlen ((const char *) script),
                                       false);

  /* Free JavaScript value, returned by eval */
  jerry_release_value (eval_ret);

  /* Freeing engine */
  jerry_cleanup ();

  return 0;
}