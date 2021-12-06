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
#include "jerryscript-mbed-launcher/setup.h"

#include <stdio.h>
#include <stdlib.h>

#include "jerryscript-mbed-util/logging.h"

extern uint32_t jsmbed_js_magic_string_count;
extern uint32_t jsmbed_js_magic_string_values[];

extern const char *jsmbed_js_magic_strings[];
extern const jerry_length_t jsmbed_js_magic_string_lengths[];

void
jsmbed_js_load_magic_strings ()
{
  if (jsmbed_js_magic_string_count == 0)
  {
    return;
  }

  jerry_register_magic_strings ((const jerry_char_t **) jsmbed_js_magic_strings,
                                jsmbed_js_magic_string_count,
                                jsmbed_js_magic_string_lengths);

  jerry_value_t global = jerry_current_realm ();

  for (unsigned int idx = 0; idx < jsmbed_js_magic_string_count; idx++)
  {
    jerry_value_t constant_value = jerry_number (jsmbed_js_magic_string_values[idx]);
    jerry_value_t magic_string = jerry_string_sz (jsmbed_js_magic_strings[idx]);

    jerry_value_free (jerry_object_set (global, magic_string, constant_value));

    jerry_value_free (constant_value);
    jerry_value_free (magic_string);
  }

  jerry_value_free (global);
}
