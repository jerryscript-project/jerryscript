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
main (int argc, char *argv[])
{
  const jerry_char_t script_1[] = "var s = 'Hello, World!';";
  const jerry_char_t script_2[] = "print (s);";

  /* Initialize engine */
  jerry_init (JERRY_INIT_EMPTY);

  /* Register 'print' function from the extensions */
  jerryx_handler_register_global ((const jerry_char_t *) "print",
                                  jerryx_handler_print);

  jerry_value_t eval_ret;

  /* Evaluate script1 */
  eval_ret = jerry_eval (script_1,
                         strlen ((const char *) script_1),
                         false);

  /* Free JavaScript value, returned by eval */
  jerry_release_value (eval_ret);

  /* Evaluate script2 */
  eval_ret = jerry_eval (script_2,
                         strlen ((const char *) script_2),
                         false);

  /* Free JavaScript value, returned by eval */
  jerry_release_value (eval_ret);

  /* Cleanup engine */
  jerry_cleanup ();

  return 0;
}
