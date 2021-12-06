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

#ifndef JERRYX_PROPERTIES_H
#define JERRYX_PROPERTIES_H

#include "jerryscript-types.h"

JERRY_C_API_BEGIN

/*
 * Handler registration helper
 */

bool jerryx_register_global (const char *name_p, jerry_external_handler_t handler_p);

/**
 * Struct used by the `jerryx_set_functions` method to
 * register multiple methods for a given object.
 */
typedef struct
{
  const char *name; /**< name of the property to add */
  jerry_value_t value; /**< value of the property */
} jerryx_property_entry;

#define JERRYX_PROPERTY_NUMBER(NAME, NUMBER) \
  (jerryx_property_entry)                    \
  {                                          \
    NAME, jerry_number (NUMBER)              \
  }
#define JERRYX_PROPERTY_STRING(NAME, STR, SIZE)                                \
  (jerryx_property_entry)                                                      \
  {                                                                            \
    NAME, jerry_string ((const jerry_char_t *) STR, SIZE, JERRY_ENCODING_UTF8) \
  }
#define JERRYX_PROPERTY_STRING_SZ(NAME, STR) \
  (jerryx_property_entry)                    \
  {                                          \
    NAME, jerry_string_sz (STR)              \
  }
#define JERRYX_PROPERTY_BOOLEAN(NAME, VALUE) \
  (jerryx_property_entry)                    \
  {                                          \
    NAME, jerry_boolean (VALUE)              \
  }
#define JERRYX_PROPERTY_FUNCTION(NAME, FUNC) \
  (jerryx_property_entry)                    \
  {                                          \
    NAME, jerry_function_external (FUNC)     \
  }
#define JERRYX_PROPERTY_UNDEFINED(NAME) \
  (jerryx_property_entry)               \
  {                                     \
    NAME, jerry_undefined ()            \
  }
#define JERRYX_PROPERTY_LIST_END() \
  (jerryx_property_entry)          \
  {                                \
    NULL, 0                        \
  }

/**
 * Stores the result of property register operation.
 */
typedef struct
{
  jerry_value_t result; /**< result of property registration (undefined or error object) */
  uint32_t registered; /**< number of successfully registered methods */
} jerryx_register_result;

jerryx_register_result jerryx_set_properties (const jerry_value_t target_object, const jerryx_property_entry entries[]);

void jerryx_release_property_entry (const jerryx_property_entry entries[],
                                    const jerryx_register_result register_result);

JERRY_C_API_END

#endif /* !JERRYX_PROPERTIES_H */
