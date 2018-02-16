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

#ifndef JERRY_EXTAPI_H
#define JERRY_EXTAPI_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <espressif/esp_common.h>
#include "FreeRTOS.h"
#include "task.h"

#include "jerryscript-core.h"
#ifdef JERRY_DEBUGGER
#include "jerryscript-debugger.h"
#endif /* JERRY_DEBUGGER */

#include "esp_modules.h"

#define DELCARE_HANDLER(NAME) \
static jerry_value_t \
NAME ## _handler (const jerry_value_t function_obj_val __attribute__((unused)), \
                  const jerry_value_t this_val __attribute__((unused)), \
                  const jerry_value_t args_p[], \
                  const jerry_length_t args_cnt)

#define TYPE_OBJECT "object"
#define TYPE_NUMBER "number"
#define TYPE_STRING "string"
#define TYPE_TYPEDARRAY "typedArray"
#define TYPE_BOOLEAN "boolean"
#define TYPE_ARRAY "array"
#define JERRY_STANDALONE_EXIT_CODE_OK 0
#define JERRY_STANDALONE_EXIT_CODE_FAIL 1

void register_js_value_to_object (char *name_p, jerry_value_t value, jerry_value_t object);
void register_number_to_object (char *name_p, double number, jerry_value_t object);
void register_string_to_object (char *name_p, char *string, jerry_value_t object);
void register_boolean_to_object (char *name_p, bool boolean, jerry_value_t object);
bool register_native_function (char *name, jerry_external_handler_t handler, jerry_value_t object);
jerry_value_t raise_argument_count_error (char *object, char *property, int expected_argument_count);
jerry_value_t raise_argument_type_error (int arg_count, char *type);
void register_js_entries (void);

#endif /* JERRY_EXTAPI_H */
