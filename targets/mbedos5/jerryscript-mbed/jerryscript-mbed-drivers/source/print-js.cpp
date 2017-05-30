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
#include "mbed.h"
#include "jerryscript-mbed-drivers/print-js.h"

/**
 * print (native JavaScript function)
 *
 * Print a string over serial (baud rate 115,200)
 *
 * @param string String to print
 */
DECLARE_GLOBAL_FUNCTION(print) {
    CHECK_ARGUMENT_COUNT(global, print, (args_count == 1));
    CHECK_ARGUMENT_TYPE_ALWAYS(global, print, 0, string);

    jerry_size_t szArg0 = jerry_get_string_size(args[0]);
    jerry_char_t *sArg0 = (jerry_char_t*) calloc(szArg0 + 1, sizeof(jerry_char_t));
    jerry_string_to_char_buffer(args[0], sArg0, szArg0);

    printf("%s\n", (const char*)sArg0);

    return jerry_create_undefined();
}
