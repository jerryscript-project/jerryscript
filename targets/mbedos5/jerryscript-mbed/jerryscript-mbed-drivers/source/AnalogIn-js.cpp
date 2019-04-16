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
#include "jerryscript-mbed-util/logging.h"
#include "jerryscript-mbed-library-registry/wrap_tools.h"

#include "mbed.h"

/**
 * AnalogIn#destructor
 *
 * Called if/when the AnalogIn is GC'ed.
 */
void NAME_FOR_CLASS_NATIVE_DESTRUCTOR(AnalogIn)(void* void_ptr) {
    delete static_cast<AnalogIn*>(void_ptr);
}

/**
 * Type infomation of the native AnalogIn pointer
 *
 * Set AnalogIn#destructor as the free callback.
 */
static const jerry_object_native_info_t native_obj_type_info = {
    .free_cb = NAME_FOR_CLASS_NATIVE_DESTRUCTOR(AnalogIn)
};

/**
 * AnalogIn#read (native JavaScript method)
 *
 * Read the input voltage, represented as a float in the range [0.0, 1.0]
 *
 * @returns A floating-point value representing the current input voltage, measured as a percentage
 */
DECLARE_CLASS_FUNCTION(AnalogIn, read) {
    CHECK_ARGUMENT_COUNT(AnalogIn, read, (args_count == 0));

    // Extract native AnalogIn pointer
    void* void_ptr;
    bool has_ptr = jerry_get_object_native_pointer(this_obj, &void_ptr, &native_obj_type_info);

    if (!has_ptr) {
        return jerry_create_error(JERRY_ERROR_TYPE,
                                  (const jerry_char_t *) "Failed to get native AnalogIn pointer");
    }

    AnalogIn* native_ptr = static_cast<AnalogIn*>(void_ptr);

    float result = native_ptr->read();
    return jerry_create_number(result);
}

/**
 * AnalogIn#read_u16 (native JavaScript method)
 *
 * Read the input voltage, represented as an unsigned short in the range [0x0, 0xFFFF]
 *
 * @returns 16-bit unsigned short representing the current input voltage, normalised to a 16-bit value
 */
DECLARE_CLASS_FUNCTION(AnalogIn, read_u16) {
    CHECK_ARGUMENT_COUNT(AnalogIn, read_u16, (args_count == 0));

    // Extract native AnalogIn pointer
    void* void_ptr;
    bool has_ptr = jerry_get_object_native_pointer(this_obj, &void_ptr, &native_obj_type_info);

    if (!has_ptr) {
        return jerry_create_error(JERRY_ERROR_TYPE,
                                  (const jerry_char_t *) "Failed to get native AnalogIn pointer");
    }

    AnalogIn* native_ptr = static_cast<AnalogIn*>(void_ptr);

    uint16_t result = native_ptr->read_u16();
    return jerry_create_number(result);
}

/**
 * AnalogIn (native JavaScript constructor)
 *
 * @param pin_name mbed pin to connect the AnalogIn to.
 * @returns a JavaScript object representing a AnalogIn.
 */
DECLARE_CLASS_CONSTRUCTOR(AnalogIn) {
    CHECK_ARGUMENT_COUNT(AnalogIn, __constructor, args_count == 1);
    CHECK_ARGUMENT_TYPE_ALWAYS(AnalogIn, __constructor, 0, number);

    PinName pin_name = PinName(jerry_get_number_value(args[0]));

    // create native object
    AnalogIn* native_ptr = new AnalogIn(pin_name);

    // create the jerryscript object
    jerry_value_t js_object = jerry_create_object();
    jerry_set_object_native_pointer(js_object, native_ptr, &native_obj_type_info);

    // attach methods
    ATTACH_CLASS_FUNCTION(js_object, AnalogIn, read);
    ATTACH_CLASS_FUNCTION(js_object, AnalogIn, read_u16);

    return js_object;
}
