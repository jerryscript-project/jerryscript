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
 * AnalogIn#read (native JavaScript method)
 *
 * Read the input voltage, represented as a float in the range [0.0, 1.0]
 *
 * @returns A floating-point value representing the current input voltage, measured as a percentage
 */
DECLARE_CLASS_FUNCTION(AnalogIn, read) {
    CHECK_ARGUMENT_COUNT(AnalogIn, read, (args_count == 0));

    // Extract native AnalogIn pointer
    uintptr_t ptr_val;
    jerry_get_object_native_handle(this_obj, &ptr_val);

    AnalogIn* native_ptr = reinterpret_cast<AnalogIn*>(ptr_val);

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
    uintptr_t ptr_val;
    jerry_get_object_native_handle(this_obj, &ptr_val);

    AnalogIn* native_ptr = reinterpret_cast<AnalogIn*>(ptr_val);

    uint16_t result = native_ptr->read_u16();
    return jerry_create_number(result);
}

/**
 * AnalogIn#destructor
 *
 * Called if/when the AnalogIn is GC'ed.
 */
void NAME_FOR_CLASS_NATIVE_DESTRUCTOR(AnalogIn)(const uintptr_t native_handle) {
    delete reinterpret_cast<AnalogIn*>(native_handle);
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
    uintptr_t native_ptr = (uintptr_t) new AnalogIn(pin_name);

    // create the jerryscript object
    jerry_value_t js_object = jerry_create_object();
    jerry_set_object_native_handle(js_object, native_ptr, NAME_FOR_CLASS_NATIVE_DESTRUCTOR(AnalogIn));

    // attach methods
    ATTACH_CLASS_FUNCTION(js_object, AnalogIn, read);
    ATTACH_CLASS_FUNCTION(js_object, AnalogIn, read_u16);

    return js_object;
}
