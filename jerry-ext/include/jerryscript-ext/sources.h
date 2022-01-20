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

#ifndef JERRYX_SOURCES_H
#define JERRYX_SOURCES_H

#include "jerryscript-types.h"

JERRY_C_API_BEGIN

jerry_value_t jerryx_source_parse_script (const char* path);
jerry_value_t jerryx_source_exec_script (const char* path);
jerry_value_t jerryx_source_exec_module (const char* path);
jerry_value_t jerryx_source_exec_snapshot (const char* path, size_t function_index);
jerry_value_t jerryx_source_exec_stdin (void);

JERRY_C_API_END

#endif /* !JERRYX_EXEC_H */
