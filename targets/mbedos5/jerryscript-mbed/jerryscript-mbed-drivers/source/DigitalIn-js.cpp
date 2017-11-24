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
 * 
 * Initial version by Rouan van der Ende @bitlabio / @fluentart
 * usage: 
 *  var a = new DigitalIn(BUTTON1); 
 *  print(a.read());
 * 
 * known issues:
 *  Does not support second argument for PULL setting because I don't
 *  know how to code that yet :)
 * 
 */
#include "jerryscript-mbed-util/logging.h"
#include "jerryscript-mbed-library-registry/wrap_tools.h"

#include "mbed.h"

/**
 * DigitalIn#destructor
 *
 * Called if/when the DigitalIn is GC'ed.
 */
void NAME_FOR_CLASS_NATIVE_DESTRUCTOR(DigitalIn)(void* void_ptr) {
    delete static_cast<DigitalIn*>(void_ptr);
}

/**
 * Type infomation of the native DigitalIn pointer
 *
 * Set DigitalIn#destructor as the free callback.
 */
static const jerry_object_native_info_t native_obj_type_info = {
    .free_cb = NAME_FOR_CLASS_NATIVE_DESTRUCTOR(DigitalIn)
};


/**
 * DigitalIn#read (native JavaScript method)
 *
 * Reads the current status of a DigitalIn
 *
 * @returns 1 if the pin is currently high, or 0 if the pin is currently low.
 */
DECLARE_CLASS_FUNCTION(DigitalIn, read) {
    CHECK_ARGUMENT_COUNT(DigitalIn, read, (args_count == 0));

    // Extract native DigitalIn pointer
    void* void_ptr;
    const jerry_object_native_info_t* type_ptr;
    bool has_ptr = jerry_get_object_native_pointer(this_obj, &void_ptr, &type_ptr);

    if (!has_ptr || type_ptr != &native_obj_type_info) {
        return jerry_create_error(JERRY_ERROR_TYPE,
                                  (const jerry_char_t *) "Failed to get native DigitalIn pointer");
    }

    DigitalIn* native_ptr = static_cast<DigitalIn*>(void_ptr);

    int result = native_ptr->read();
    return jerry_create_number(result);
}

/**
 * DigitalIn#is_connected (native JavaScript method)
 *
 * @returns 0 if the DigitalIn is set to NC, or 1 if it is connected to an
 *  actual pin
 */
DECLARE_CLASS_FUNCTION(DigitalIn, is_connected) {
    CHECK_ARGUMENT_COUNT(DigitalIn, is_connected, (args_count == 0));

    // Extract native DigitalIn pointer
    void* void_ptr;
    const jerry_object_native_info_t* type_ptr;
    bool has_ptr = jerry_get_object_native_pointer(this_obj, &void_ptr, &type_ptr);

    if (!has_ptr || type_ptr != &native_obj_type_info) {
        return jerry_create_error(JERRY_ERROR_TYPE,
                                  (const jerry_char_t *) "Failed to get native DigitalIn pointer");
    }

    DigitalIn* native_ptr = static_cast<DigitalIn*>(void_ptr);

    int result = native_ptr->is_connected();
    return jerry_create_number(result);
}

/**
 * DigitalIn (native JavaScript constructor)
 *
 * @param pin_name mbed pin to connect the DigitalIn to.
 * @returns a JavaScript object representing a DigitalIn.
 */
DECLARE_CLASS_CONSTRUCTOR(DigitalIn) {
    CHECK_ARGUMENT_COUNT(DigitalIn, __constructor, args_count == 1);
    CHECK_ARGUMENT_TYPE_ALWAYS(DigitalIn, __constructor, 0, number);

    DigitalIn* native_ptr;

    // Call correct overload of DigitalIn::DigitalIn depending on the
    // arguments passed.
    PinName pin_name = PinName(jerry_get_number_value(args[0]));
    native_ptr = new DigitalIn(pin_name);

    // create the jerryscript object
    jerry_value_t js_object = jerry_create_object();
    jerry_set_object_native_pointer(js_object, native_ptr, &native_obj_type_info);

    // attach methods
    ATTACH_CLASS_FUNCTION(js_object, DigitalIn, read);
    ATTACH_CLASS_FUNCTION(js_object, DigitalIn, is_connected);

    return js_object;
}
