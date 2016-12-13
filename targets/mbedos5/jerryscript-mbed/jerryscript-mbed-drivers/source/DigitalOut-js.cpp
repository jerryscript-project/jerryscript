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
 * DigitalOut#write (native JavaScript method)
 *
 * Writes a binary value to a DigitalOut.
 *
 * @param value 1 or 0, specifying whether the output pin is high or low,
 *      respectively
 * @returns undefined, or an error if invalid arguments are provided.
 */
DECLARE_CLASS_FUNCTION(DigitalOut, write) {
    CHECK_ARGUMENT_COUNT(DigitalOut, write, (args_count == 1));
    CHECK_ARGUMENT_TYPE_ALWAYS(DigitalOut, write, 0, number);

    // Extract native DigitalOut pointer
    uintptr_t ptr_val;
    jerry_get_object_native_handle(this_obj, &ptr_val);

    DigitalOut* native_ptr = reinterpret_cast<DigitalOut*>(ptr_val);
    
    int arg0 = jerry_get_number_value(args[0]);
    native_ptr->write(arg0);

    return jerry_create_undefined();
}

/**
 * DigitalOut#read (native JavaScript method)
 *
 * Reads the current status of a DigitalOut
 *
 * @returns 1 if the pin is currently high, or 0 if the pin is currently low.
 */
DECLARE_CLASS_FUNCTION(DigitalOut, read) {
    CHECK_ARGUMENT_COUNT(DigitalOut, read, (args_count == 0));

    // Extract native DigitalOut pointer
    uintptr_t ptr_val;
    jerry_get_object_native_handle(this_obj, &ptr_val);

    DigitalOut* native_ptr = reinterpret_cast<DigitalOut*>(ptr_val);

    int result = native_ptr->read();
    return jerry_create_number(result);
}

/**
 * DigitalOut#is_connected (native JavaScript method)
 *
 * @returns 0 if the DigitalOut is set to NC, or 1 if it is connected to an
 *  actual pin
 */
DECLARE_CLASS_FUNCTION(DigitalOut, is_connected) {
    CHECK_ARGUMENT_COUNT(DigitalOut, is_connected, (args_count == 0));

    // Extract native DigitalOut pointer
    uintptr_t ptr_val;
    jerry_get_object_native_handle(this_obj, &ptr_val);

    DigitalOut* native_ptr = reinterpret_cast<DigitalOut*>(ptr_val);

    int result = native_ptr->is_connected();
    return jerry_create_number(result);
}

/**
 * DigitalOut#destructor
 * 
 * Called if/when the DigitalOut is GC'ed.
 */
void NAME_FOR_CLASS_NATIVE_DESTRUCTOR(DigitalOut)(const uintptr_t native_handle) {
    delete reinterpret_cast<DigitalOut*>(native_handle);
}

/**
 * DigitalOut (native JavaScript constructor)
 *
 * @param pin_name mbed pin to connect the DigitalOut to.
 * @param value (optional) Initial value of the DigitalOut.
 * @returns a JavaScript object representing a DigitalOut.
 */
DECLARE_CLASS_CONSTRUCTOR(DigitalOut) {
    CHECK_ARGUMENT_COUNT(DigitalOut, __constructor, (args_count == 1 || args_count == 2));
    CHECK_ARGUMENT_TYPE_ALWAYS(DigitalOut, __constructor, 0, number);
    CHECK_ARGUMENT_TYPE_ON_CONDITION(DigitalOut, __constructor, 1, number, (args_count == 2));

    uintptr_t native_ptr;

    // Call correct overload of DigitalOut::DigitalOut depending on the
    // arguments passed.
    PinName pin_name = PinName(jerry_get_number_value(args[0]));

    switch (args_count) {
        case 1:
            native_ptr = (uintptr_t) new DigitalOut(pin_name);
            break;
        case 2:
            int value = static_cast<int>(jerry_get_number_value(args[1]));
            native_ptr = (uintptr_t) new DigitalOut(pin_name, value);
            break;
    }

    // create the jerryscript object
    jerry_value_t js_object = jerry_create_object();
    jerry_set_object_native_handle(js_object, native_ptr, NAME_FOR_CLASS_NATIVE_DESTRUCTOR(DigitalOut));

    // attach methods
    ATTACH_CLASS_FUNCTION(js_object, DigitalOut, write);
    ATTACH_CLASS_FUNCTION(js_object, DigitalOut, read);
    ATTACH_CLASS_FUNCTION(js_object, DigitalOut, is_connected);

    return js_object;
}
