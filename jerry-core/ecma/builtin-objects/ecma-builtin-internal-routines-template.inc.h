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

#ifndef BUILTIN_UNDERSCORED_ID
#error "Please, define BUILTIN_UNDERSCORED_ID"
#endif /* !BUILTIN_UNDERSCORED_ID */

#ifndef BUILTIN_INC_HEADER_NAME
#error "Please, define BUILTIN_INC_HEADER_NAME"
#endif /* !BUILTIN_INC_HEADER_NAME */

#include "ecma-objects.h"

#define PASTE__(x, y) x##y
#define PASTE_(x, y)  PASTE__ (x, y)
#define PASTE(x, y)   PASTE_ (x, y)

#define PROPERTY_DESCRIPTOR_LIST_NAME PASTE (PASTE (ecma_builtin_, BUILTIN_UNDERSCORED_ID), _property_descriptor_list)
#define DISPATCH_ROUTINE_ROUTINE_NAME PASTE (PASTE (ecma_builtin_, BUILTIN_UNDERSCORED_ID), _dispatch_routine)

/**
 * Built-in property list of the built-in object.
 */
const ecma_builtin_property_descriptor_t PROPERTY_DESCRIPTOR_LIST_NAME[] = {
#define ROUTINE(name, c_function_name, args_number, length_prop_value) \
  { name,                                                              \
    ECMA_BUILTIN_PROPERTY_ROUTINE,                                     \
    ECMA_PROPERTY_BUILT_IN_CONFIGURABLE_WRITABLE,                      \
    ECMA_ROUTINE_VALUE (c_function_name, length_prop_value) },
#define ROUTINE_CONFIGURABLE_ONLY(name, c_function_name, args_number, length_prop_value) \
  { name,                                                                                \
    ECMA_BUILTIN_PROPERTY_ROUTINE,                                                       \
    ECMA_PROPERTY_BUILT_IN_CONFIGURABLE,                                                 \
    ECMA_ROUTINE_VALUE (c_function_name, length_prop_value) },
#define ROUTINE_WITH_FLAGS(name, c_function_name, args_number, length_prop_value, prop_attributes) \
  { name,                                                                                          \
    ECMA_BUILTIN_PROPERTY_ROUTINE,                                                                 \
    (prop_attributes) | ECMA_PROPERTY_FLAG_BUILT_IN,                                               \
    ECMA_ROUTINE_VALUE (c_function_name, length_prop_value) },
#define ACCESSOR_READ_ONLY(name, c_getter_func_name, prop_attributes) \
  { name,                                                             \
    ECMA_BUILTIN_PROPERTY_ACCESSOR_READ_ONLY,                         \
    (prop_attributes) | ECMA_PROPERTY_FLAG_BUILT_IN,                  \
    c_getter_func_name },
#define ACCESSOR_READ_WRITE(name, c_getter_func_name, c_setter_func_name, prop_attributes) \
  { name,                                                                                  \
    ECMA_BUILTIN_PROPERTY_ACCESSOR_READ_WRITE,                                             \
    (prop_attributes) | ECMA_PROPERTY_FLAG_BUILT_IN,                                       \
    ECMA_ACCESSOR_READ_WRITE (c_getter_func_name, c_setter_func_name) },
#define OBJECT_VALUE(name, obj_builtin_id, prop_attributes) \
  { name, ECMA_BUILTIN_PROPERTY_OBJECT, (prop_attributes) | ECMA_PROPERTY_FLAG_BUILT_IN, obj_builtin_id },
#define SIMPLE_VALUE(name, simple_value, prop_attributes) \
  { name, ECMA_BUILTIN_PROPERTY_SIMPLE, (prop_attributes) | ECMA_PROPERTY_FLAG_BUILT_IN, simple_value },
#define NUMBER_VALUE(name, number_value, prop_attributes) \
  { name, ECMA_BUILTIN_PROPERTY_NUMBER, (prop_attributes) | ECMA_PROPERTY_FLAG_BUILT_IN, number_value },
#define STRING_VALUE(name, magic_string_id, prop_attributes) \
  { name, ECMA_BUILTIN_PROPERTY_STRING, (prop_attributes) | ECMA_PROPERTY_FLAG_BUILT_IN, magic_string_id },
#if JERRY_ESNEXT
#define SYMBOL_VALUE(name, symbol) { name, ECMA_BUILTIN_PROPERTY_SYMBOL, ECMA_PROPERTY_BUILT_IN_FIXED, symbol },
#define INTRINSIC_PROPERTY(name, magic_string_id, prop_attributes) \
  { name, ECMA_BUILTIN_PROPERTY_INTRINSIC_PROPERTY, (prop_attributes) | ECMA_PROPERTY_FLAG_BUILT_IN, magic_string_id },
#define ACCESSOR_BUILTIN_FUNCTION(name, getter_builtin_id, setter_builtin_id, prop_attributes) \
  { name,                                                                                      \
    ECMA_BUILTIN_PROPERTY_ACCESSOR_BUILTIN_FUNCTION,                                           \
    (prop_attributes) | ECMA_PROPERTY_FLAG_BUILT_IN,                                           \
    ECMA_ACCESSOR_READ_WRITE (getter_builtin_id, setter_builtin_id) },
#endif /* JERRY_ESNEXT */
#include BUILTIN_INC_HEADER_NAME
  { LIT_MAGIC_STRING__COUNT, ECMA_BUILTIN_PROPERTY_END, 0, 0 }
};

#undef BUILTIN_INC_HEADER_NAME
#undef BUILTIN_UNDERSCORED_ID
#undef DISPATCH_ROUTINE_ROUTINE_NAME
#undef ECMA_BUILTIN_PROPERTY_NAME_INDEX
#undef PASTE__
#undef PASTE_
#undef PASTE
#undef PROPERTY_DESCRIPTOR_LIST_NAME
