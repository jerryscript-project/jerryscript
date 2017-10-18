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
#include "jerryscript-mbed-event-loop/EventLoop.h"
#include "jerryscript-mbed-drivers/Serial-js.h"
#include "jerryscript-mbed-library-registry/wrap_tools.h"

#include "mbed.h"

/**
 * Serial#destructor
 *
 * Called if/when the Serial object is GC'ed.
 */
void NAME_FOR_CLASS_NATIVE_DESTRUCTOR(Serial) (void *void_ptr) {
    delete static_cast<Serial*>(void_ptr);
}

/**
 * Type infomation of the native Serial pointer
 *
 * Set Serial#destructor as the free callback.
 */
static const jerry_object_native_info_t native_obj_type_info = {
    .free_cb = NAME_FOR_CLASS_NATIVE_DESTRUCTOR(Serial)
};

/**
 * Serial#baud (native JavaScript method)
 * 
 * Sets the baud rate
 *
 * @param integer byte value
 * @returns undefined
 */

DECLARE_CLASS_FUNCTION(Serial, baud) {
    CHECK_ARGUMENT_COUNT(Serial, baud, (args_count == 1));
    CHECK_ARGUMENT_TYPE_ALWAYS(Serial, baud, 0, number);

    // Extract native Serial pointer
    void* void_ptr;
    const jerry_object_native_info_t* type_ptr;
    bool has_ptr = jerry_get_object_native_pointer(this_obj, &void_ptr, &type_ptr);

    if (!has_ptr || type_ptr != &native_obj_type_info) {
        return jerry_create_error(JERRY_ERROR_TYPE,
                                  (const jerry_char_t *) "Failed to get native Serial pointer");
    }

    

    Serial* native_ptr = static_cast<Serial*>(void_ptr);

    int arg0 = jerry_get_number_value(args[0]);
    native_ptr->baud(arg0);

    return jerry_create_undefined();
}

 /**
 * Serial#putc (native JavaScript method)
 * 
 * Writes an integer value to a Serial connection.
 *
 * @param integer byte value or char
 * @returns 1 on success, 0 on error
 */

DECLARE_CLASS_FUNCTION(Serial, putc) {
    CHECK_ARGUMENT_COUNT(Serial, putc, (args_count == 1));
    CHECK_ARGUMENT_TYPE_ALWAYS(Serial, putc, 0, number);

    // Extract native Serial pointer
    void* void_ptr;
    const jerry_object_native_info_t* type_ptr;
    bool has_ptr = jerry_get_object_native_pointer(this_obj, &void_ptr, &type_ptr);

    if (!has_ptr || type_ptr != &native_obj_type_info) {
        return jerry_create_error(JERRY_ERROR_TYPE,
                                  (const jerry_char_t *) "Failed to get native Serial pointer");
    }

    Serial* native_ptr = static_cast<Serial*>(void_ptr);

    int arg0 = jerry_get_number_value(args[0]);
    native_ptr->putc(arg0);

    return jerry_create_undefined();
}



/**
 * Serial#getc (native JavaScript method)
 * 
 * Readed an integer value from a Serial connection.
 *
 * @returns byte read 
 */

DECLARE_CLASS_FUNCTION(Serial, getc) {
    CHECK_ARGUMENT_COUNT(Serial, getc, (args_count == 0));
    
    // Extract native Serial pointer
    void* void_ptr;
    const jerry_object_native_info_t* type_ptr;
    bool has_ptr = jerry_get_object_native_pointer(this_obj, &void_ptr, &type_ptr);

    if (!has_ptr || type_ptr != &native_obj_type_info) {
        return jerry_create_error(JERRY_ERROR_TYPE,
                                    (const jerry_char_t *) "Failed to get native Serial pointer");
    }

    Serial* native_ptr = static_cast<Serial*>(void_ptr);

    int result = native_ptr->getc();
    return jerry_create_number(result);
}



/**
 * Serial#writable (native JavaScript method)
 * 
 * Checks if a serialport is writable
 *
 * @returns true if writable, false if not writable
 */

DECLARE_CLASS_FUNCTION(Serial, writable) {
    CHECK_ARGUMENT_COUNT(Serial, writable, (args_count == 0));
    
    // Extract native Serial pointer
    void* void_ptr;
    const jerry_object_native_info_t* type_ptr;
    bool has_ptr = jerry_get_object_native_pointer(this_obj, &void_ptr, &type_ptr);

    if (!has_ptr || type_ptr != &native_obj_type_info) {
        return jerry_create_error(JERRY_ERROR_TYPE,
                                    (const jerry_char_t *) "Failed to get native Serial pointer");
    }

    Serial* native_ptr = static_cast<Serial*>(void_ptr);

    bool result = native_ptr->writable();
    return jerry_create_number(result);
}



/**
 * Serial#readable (native JavaScript method)
 * 
 * Checks if a serialport is readable
 *
 * @returns true if readable, false if not readable
 */

DECLARE_CLASS_FUNCTION(Serial, readable) {
    CHECK_ARGUMENT_COUNT(Serial, readable, (args_count == 0));
    
    // Extract native Serial pointer
    void* void_ptr;
    const jerry_object_native_info_t* type_ptr;
    bool has_ptr = jerry_get_object_native_pointer(this_obj, &void_ptr, &type_ptr);

    if (!has_ptr || type_ptr != &native_obj_type_info) {
        return jerry_create_error(JERRY_ERROR_TYPE,
                                    (const jerry_char_t *) "Failed to get native Serial pointer");
    }

    Serial* native_ptr = static_cast<Serial*>(void_ptr);

    bool result = native_ptr->readable();
    return jerry_create_number(result);
}



/**
 * Serial (native JavaScript constructor)
 * @param tx mbed pin for Serial tx
 * @param rx mbed pin for Serial rx
 * @returns a JavaScript object representing the Serial bus.
 */

DECLARE_CLASS_CONSTRUCTOR(Serial) {
    CHECK_ARGUMENT_COUNT(Serial, __constructor, (args_count == 2 || args_count == 3));
    CHECK_ARGUMENT_TYPE_ALWAYS(Serial, __constructor, 0, number);
    CHECK_ARGUMENT_TYPE_ALWAYS(Serial, __constructor, 1, number);
    CHECK_ARGUMENT_TYPE_ON_CONDITION(Serial, __constructor, 2, number, (args_count == 3));

    Serial* native_ptr;

    int tx = jerry_get_number_value(args[0]);
    int rx = jerry_get_number_value(args[1]);

    switch (args_count) {
        case 2:
            native_ptr = new Serial((PinName)tx, (PinName)rx);
            break;
        case 3:
            int baud = static_cast<int>(jerry_get_number_value(args[2]));
            native_ptr = new Serial((PinName)tx, (PinName)rx, baud);
            break;
    }


    jerry_value_t js_object = jerry_create_object();
    jerry_set_object_native_pointer(js_object, native_ptr, &native_obj_type_info);

    //ATTACH_CLASS_FUNCTION(js_object, Serial, attach); //DISABLED, NON FUNCTIONAL
    ATTACH_CLASS_FUNCTION(js_object, Serial, baud);
    ATTACH_CLASS_FUNCTION(js_object, Serial, putc);
    ATTACH_CLASS_FUNCTION(js_object, Serial, getc);
    ATTACH_CLASS_FUNCTION(js_object, Serial, writable);
    ATTACH_CLASS_FUNCTION(js_object, Serial, readable);
    //ATTACH_CLASS_FUNCTION(js_object, Serial, write);

    return js_object;
}
