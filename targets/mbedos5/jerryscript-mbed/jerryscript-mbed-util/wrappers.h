/* Copyright (c) 2016 ARM Limited
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
#ifndef _JERRYSCRIPT_MBED_UTIL_WRAPPERS_H
#define _JERRYSCRIPT_MBED_UTIL_WRAPPERS_H

/*
 * Used in header/source files for wrappers, to declare the signature of the
 * registration function.
 */
#define DECLARE_JS_WRAPPER_REGISTRATION(NAME) \
  void jsmbed_wrap_registry_entry__ ## NAME (void)

//
// 2. Wrapper function declaration/use macros
//

// Global functions
#define DECLARE_GLOBAL_FUNCTION(NAME) \
jerry_value_t \
NAME_FOR_GLOBAL_FUNCTION(NAME) (const jerry_value_t function_obj_p, \
                  const jerry_value_t  this_obj, \
                  const jerry_value_t    args[], \
                  const jerry_length_t   args_count)

#define REGISTER_GLOBAL_FUNCTION(NAME) \
  jsmbed_wrap_register_global_function ( # NAME, NAME_FOR_GLOBAL_FUNCTION(NAME) )

// Class constructors
#define DECLARE_CLASS_CONSTRUCTOR(CLASS) \
jerry_value_t \
NAME_FOR_CLASS_CONSTRUCTOR(CLASS) (const jerry_value_t function_obj, \
                  const jerry_value_t    this_obj, \
                  const jerry_value_t    args[], \
                  const jerry_length_t   args_count)

#define REGISTER_CLASS_CONSTRUCTOR(CLASS) \
  jsmbed_wrap_register_class_constructor ( # CLASS, NAME_FOR_CLASS_CONSTRUCTOR(CLASS) )

// Class functions
#define DECLARE_CLASS_FUNCTION(CLASS, NAME) \
jerry_value_t \
NAME_FOR_CLASS_FUNCTION(CLASS, NAME) (const jerry_value_t function_obj, \
                  const jerry_value_t  this_obj, \
                  const jerry_value_t    args[], \
                  const jerry_length_t   args_count)

#define ATTACH_CLASS_FUNCTION(OBJECT, CLASS, NAME) \
  jsmbed_wrap_register_class_function (OBJECT, # NAME, NAME_FOR_CLASS_FUNCTION(CLASS, NAME) )

//
// 3. Argument checking macros
//
#define CHECK_ARGUMENT_COUNT(CLASS, NAME, EXPR) \
    if (!(EXPR)) { \
        const char* error_msg = "ERROR: wrong argument count for " # CLASS "." # NAME ", expected " # EXPR "."; \
        return jerry_create_error(JERRY_ERROR_TYPE, reinterpret_cast<const jerry_char_t*>(error_msg)); \
    }

#define CHECK_ARGUMENT_TYPE_ALWAYS(CLASS, NAME, INDEX, TYPE) \
    if (!jerry_value_is_ ## TYPE (args[INDEX])) { \
        const char* error_msg = "ERROR: wrong argument type for " # CLASS "." # NAME ", expected argument " # INDEX " to be a " # TYPE ".\n"; \
        return jerry_create_error(JERRY_ERROR_TYPE, reinterpret_cast<const jerry_char_t*>(error_msg)); \
    }

#define CHECK_ARGUMENT_TYPE_ON_CONDITION(CLASS, NAME, INDEX, TYPE, EXPR) \
    if ((EXPR)) { \
        if (!jerry_value_is_ ## TYPE (args[INDEX])) { \
            const char* error_msg = "ERROR: wrong argument type for " # CLASS "." # NAME ", expected argument " # INDEX " to be a " # TYPE ".\n"; \
            return jerry_create_error(JERRY_ERROR_TYPE, reinterpret_cast<const jerry_char_t*>(error_msg)); \
        } \
    }

#define NAME_FOR_GLOBAL_FUNCTION(NAME) __gen_jsmbed_global_func_ ## NAME
#define NAME_FOR_CLASS_CONSTRUCTOR(CLASS) __gen_jsmbed_class_constructor_ ## CLASS
#define NAME_FOR_CLASS_FUNCTION(CLASS, NAME) __gen_jsmbed_func_c_ ## CLASS ## _f_ ## NAME

#define NAME_FOR_CLASS_NATIVE_CONSTRUCTOR(CLASS, TYPELIST) __gen_native_jsmbed_ ## CLASS ## __Special_create_ ## TYPELIST
#define NAME_FOR_CLASS_NATIVE_DESTRUCTOR(CLASS) __gen_native_jsmbed_ ## CLASS ## __Special_destroy
#define NAME_FOR_CLASS_NATIVE_FUNCTION(CLASS, NAME) __gen_native_jsmbed_ ## CLASS ## _ ## NAME

#endif  // _JERRYSCRIPT_MBED_UTIL_WRAPPERS_H
