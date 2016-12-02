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
#include "jerryscript-mbed-drivers/assert-js.h"

/**
 * assert (native JavaScript function)
 * 
 * @param condition The condition to assert to be true
 * @returns undefined if the assertion passes, returns an error if the assertion
 * 	fails.
 */
DECLARE_GLOBAL_FUNCTION(assert) {
    CHECK_ARGUMENT_COUNT(global, assert, (args_count == 1));
    CHECK_ARGUMENT_TYPE_ALWAYS(global, assert, 0, boolean);

    if (!jerry_get_boolean_value(args[0])) {
        const char* error_msg = "Assertion failed";
        return jerry_create_error(JERRY_ERROR_TYPE, reinterpret_cast<const jerry_char_t*>(error_msg));
    }

    return jerry_create_undefined();
}
