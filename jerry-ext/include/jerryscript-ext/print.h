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

#ifndef JERRYX_PRINT_H
#define JERRYX_PRINT_H

#include "jerryscript-port.h"
#include "jerryscript-types.h"

JERRY_C_API_BEGIN

jerry_value_t jerryx_print_value (const jerry_value_t value);
void jerryx_print_buffer (const jerry_char_t *buffer_p, jerry_size_t buffer_size);
void jerryx_print_backtrace (unsigned depth);
void jerryx_print_unhandled_exception (jerry_value_t exception);
void jerryx_print_unhandled_rejection (jerry_value_t exception);

JERRY_C_API_END

#endif /* !JERRYX_PRINT_H */
