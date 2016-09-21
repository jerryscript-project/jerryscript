/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
 * Copyright (c) 2016 ARM Limited.
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
#include <stdio.h>

#include "jerryscript-mbed-library-registry/wrap_tools.h"

bool jsmbed_wrap_register_global_function(const char* name, jerry_external_handler_t handler) {
    jerry_value_t global_object_val = jerry_get_global_object();
    jerry_value_t reg_function = jerry_create_external_function(handler);

    bool is_ok = true;

    if (!(jerry_value_is_function(reg_function)
        && jerry_value_is_constructor(reg_function))) {
        is_ok = false;
        LOG_PRINT_ALWAYS("Error: jerry_create_external_function failed!\r\n");
        jerry_release_value(global_object_val);
        jerry_release_value(reg_function);
        return is_ok;
    }

    if (jerry_value_has_error_flag(reg_function)) {
        is_ok = false;
        LOG_PRINT_ALWAYS("Error: jerry_create_external_function has error flag! \r\n");
        jerry_release_value(global_object_val);
        jerry_release_value(reg_function);
        return is_ok;
    }

    jerry_value_t jerry_name = jerry_create_string((jerry_char_t *) name);

    jerry_value_t set_result = jerry_set_property(global_object_val, jerry_name, reg_function);


    if (jerry_value_has_error_flag(set_result)) {
        is_ok = false;
        LOG_PRINT_ALWAYS("Error: jerry_create_external_function failed: [%s]\r\n", name);
    }

    jerry_release_value(jerry_name);
    jerry_release_value(global_object_val);
    jerry_release_value(reg_function);
    jerry_release_value(set_result);

    return is_ok;
}

bool jsmbed_wrap_register_class_constructor(const char* name, jerry_external_handler_t handler) {
    // Register class constructor as a global function
    return jsmbed_wrap_register_global_function(name, handler);
}

bool jsmbed_wrap_register_class_function(jerry_value_t this_obj, const char* name, jerry_external_handler_t handler) {
    jerry_value_t property_name = jerry_create_string(reinterpret_cast<const jerry_char_t *>(name));
    jerry_value_t handler_obj = jerry_create_external_function(handler);

    jerry_set_property(this_obj, property_name, handler_obj);

    jerry_release_value(handler_obj);
    jerry_release_value(property_name);

    // TODO: check for errors, and return false in the case of errors
    return true;
}
