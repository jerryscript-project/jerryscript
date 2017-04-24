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
#include "test-common.h"
#ifdef JERRYX_MODULE_HAVE_CONTEXT
#include "jerry-context.h"
#endif /* JERRYX_MODULE_HAVE_CONTEXT */
#include "jerry-module.h"

const char eval_string1[] = "require ('my_custom_module');";
const char eval_string2[] = "require ('differently-handled-module');";

JERRYX_MODULE_RESOLVER (resolve_differently_handled_module)
{
  if (!strcmp ((char *) name, "differently-handled-module"))
  {
    return jerry_create_number (29);
  }
  return 0;
} /* JERRYX_MODULE_RESOLVER */

static jerry_value_t
handle_require (const jerry_value_t js_function,
                const jerry_value_t this_val,
                const jerry_value_t args_p[],
                const jerry_length_t args_count)
{
  (void) js_function;
  (void) this_val;
  (void) args_count;

  jerry_value_t return_value = 0;
  jerry_char_t module_name[256] = "";
  jerry_size_t bytes_copied = 0;

  TEST_ASSERT (args_count == 1);
  bytes_copied = jerry_string_to_char_buffer (args_p[0], module_name, 256);
  if (bytes_copied < 256)
  {
    module_name[bytes_copied] = 0;
    return_value = jerryx_module_resolve (module_name);
  }

  return return_value;
} /* handle_require */

static void
eval_one (const char *the_string, double expected_result)
{
  jerry_value_t js_eval_result = jerry_eval ((const jerry_char_t *) the_string, strlen (the_string), true);
  TEST_ASSERT (!jerry_value_has_error_flag (js_eval_result));
  TEST_ASSERT (jerry_get_number_value (js_eval_result) == expected_result);
  jerry_release_value (js_eval_result);
} /* eval_one */

int
main (int argc, char **argv)
{
  (void) argc;
  (void) argv;
  jerry_value_t js_global = 0, js_function = 0, js_property_name = 0;

  TEST_INIT ();

#ifdef JERRYX_MODULE_HAVE_CONTEXT
  jerry_init_with_user_context (JERRY_INIT_EMPTY, jerryx_context_init, jerryx_context_deinit);
#else /* !JERRYX_MODULE_HAVE_CONTEXT */
  jerry_init_with_user_context (JERRY_INIT_EMPTY, jerryx_module_manager_init, jerryx_module_manager_deinit);
#endif /* JERRYX_MODULE_HAVE_CONTEXT */

  js_global = jerry_get_global_object ();
  js_function = jerry_create_external_function (handle_require);
  js_property_name = jerry_create_string ((const jerry_char_t *) "require");
  jerry_set_property (js_global, js_property_name, js_function);

  eval_one (eval_string1, 42);
  eval_one (eval_string2, 29);

  jerry_release_value (js_property_name);
  jerry_release_value (js_function);
  jerry_release_value (js_global);

  jerry_cleanup ();
} /* main */
