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

int
main (int argc, char *argv[])
{
  const jerry_char_t script[] = "var str = 'Hello, World!';";
  size_t script_size = strlen ((const char *) script);

  bool ret_value = jerry_run_simple (script, script_size, JERRY_INIT_EMPTY);

  return (ret_value ? 0 : 1);
}
