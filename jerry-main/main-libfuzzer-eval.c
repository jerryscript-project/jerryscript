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

#include "jerryscript.h"

int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  srand (0);
  jerry_init (JERRY_INIT_EMPTY);

  if (jerry_validate_string ((jerry_char_t *) data, (jerry_size_t) size, JERRY_ENCODING_UTF8))
  {
    jerry_value_t parse_value = jerry_eval ((jerry_char_t *) data, size, JERRY_PARSE_NO_OPTS);

    jerry_value_free (parse_value);
  }

  jerry_cleanup ();

  return 0;
} /* LLVMFuzzerTestOneInput */
