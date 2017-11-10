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
#include "jerryscript-port.h"
#include "jerryscript-port-default.h"
#include "test-common.h"

const char *test_source = "test_object.call_function();";

static jerry_value_t
function_to_call (const jerry_value_t function_obj,
                  const jerry_value_t this,
                  const jerry_value_t argv[],
                  const jerry_length_t argc)
{
  JERRY_UNUSED (function_obj);
  JERRY_UNUSED (argv);
  JERRY_UNUSED (argc);
#define SHOW_BUG
#ifdef SHOW_BUG
  return jerry_acquire_value (this);
#else /* !SHOW_BUG */
  JERRY_UNUSED (this);
  return jerry_create_object ();
#endif /* SHOW_BUG */
} /* function_to_call */

int
main (void)
{
  jerry_init (JERRY_INIT_EMPTY);

  /* Environment setup */
  jerry_value_t parent = jerry_create_object ();
  jerry_value_t call_function = jerry_create_external_function (function_to_call);
  jerry_value_t name = jerry_create_string ((jerry_char_t *) "call_function");
  jerry_release_value (jerry_set_property (parent, name, call_function));
  jerry_release_value (name);
  jerry_value_t global = jerry_get_global_object ();
  name = jerry_create_string ((jerry_char_t *) "test_object");
  jerry_release_value (jerry_set_property (global, name, parent));
  jerry_release_value (name);
  jerry_release_value (call_function);
  jerry_release_value (parent);
  jerry_release_value (global);

  /* Evaluation */
  jerry_release_value (jerry_eval ((jerry_char_t *) test_source, strlen (test_source), true));

  jerry_cleanup ();
} /* main */
