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
 * PwmOut#destructor
 *
 * Called if/when the PwmOut is GC'ed.
 */
void NAME_FOR_CLASS_NATIVE_DESTRUCTOR(PwmOut)(void* void_ptr) {
    delete static_cast<PwmOut*>(void_ptr);
}

/**
 * Type infomation of the native PwmOut pointer
 *
 * Set PwmOut#destructor as the free callback.
 */
static const jerry_object_native_info_t native_obj_type_info = {
    .free_cb = NAME_FOR_CLASS_NATIVE_DESTRUCTOR(PwmOut)
};

/**
 * PwmOut#write (native JavaScript method)
 *
 * Set the ouput duty-cycle, specified as a percentage (float)
 *
 * @param value A floating-point value representing the output duty-cycle,
 *    specified as a percentage. The value should lie between
 *    0.0 (representing on 0%) and 1.0 (representing on 100%).
 *    Values outside this range will be saturated to 0.0f or 1.0f
 * @returns undefined
 */
DECLARE_CLASS_FUNCTION(PwmOut, write) {
    CHECK_ARGUMENT_COUNT(PwmOut, write, (args_count == 1));
    CHECK_ARGUMENT_TYPE_ALWAYS(PwmOut, write, 0, number);

    // Extract native PwmOut pointer
    void* void_ptr;
    bool has_ptr = jerry_get_object_native_pointer(this_obj, &void_ptr, &native_obj_type_info);

    if (!has_ptr) {
        return jerry_create_error(JERRY_ERROR_TYPE,
                                  (const jerry_char_t *) "Failed to get native PwmOut pointer");
    }

    PwmOut* native_ptr = static_cast<PwmOut*>(void_ptr);

    double arg0 = jerry_get_number_value(args[0]);
    native_ptr->write(static_cast<float>(arg0));

    return jerry_create_undefined();
}

/**
 * PwmOut#read (native JavaScript method)
 *
 * Return the current output duty-cycle setting, measured as a percentage (float)
 *
 * @returns
 *    A floating-point value representing the current duty-cycle being output on the pin,
 *    measured as a percentage. The returned value will lie between
 *    0.0 (representing on 0%) and 1.0 (representing on 100%).
 *
 * @note
 * This value may not match exactly the value set by a previous <write>.
 */
DECLARE_CLASS_FUNCTION(PwmOut, read) {
    CHECK_ARGUMENT_COUNT(PwmOut, read, (args_count == 0));

    // Extract native PwmOut pointer
    void* void_ptr;
    bool has_ptr = jerry_get_object_native_pointer(this_obj, &void_ptr, &native_obj_type_info);

    if (!has_ptr) {
        return jerry_create_error(JERRY_ERROR_TYPE,
                                  (const jerry_char_t *) "Failed to get native PwmOut pointer");
    }

    PwmOut* native_ptr = static_cast<PwmOut*>(void_ptr);

    float result = native_ptr->read();
    return jerry_create_number(result);
}

/**
 * PwmOut#period (native JavaScript method)
 *
 * Set the PWM period, specified in seconds (float), keeping the duty cycle the same.
 *
 * @note
 *   The resolution is currently in microseconds; periods smaller than this
 *   will be set to zero.
 */
DECLARE_CLASS_FUNCTION(PwmOut, period) {
    CHECK_ARGUMENT_COUNT(PwmOut, period, (args_count == 1));
    CHECK_ARGUMENT_TYPE_ALWAYS(PwmOut, period, 0, number);

    // Extract native PwmOut pointer
    void* void_ptr;
    bool has_ptr = jerry_get_object_native_pointer(this_obj, &void_ptr, &native_obj_type_info);

    if (!has_ptr) {
        return jerry_create_error(JERRY_ERROR_TYPE,
                                  (const jerry_char_t *) "Failed to get native PwmOut pointer");
    }

    PwmOut* native_ptr = static_cast<PwmOut*>(void_ptr);

    double arg0 = jerry_get_number_value(args[0]);
    native_ptr->period(static_cast<float>(arg0));

    return jerry_create_undefined();
}

/**
 * PwmOut#period_ms (native JavaScript method)
 *
 * Set the PWM period, specified in milli-seconds (int), keeping the duty cycle the same.
 */
DECLARE_CLASS_FUNCTION(PwmOut, period_ms) {
    CHECK_ARGUMENT_COUNT(PwmOut, period_ms, (args_count == 1));
    CHECK_ARGUMENT_TYPE_ALWAYS(PwmOut, period_ms, 0, number);

    // Extract native PwmOut pointer
    void* void_ptr;
    bool has_ptr = jerry_get_object_native_pointer(this_obj, &void_ptr, &native_obj_type_info);

    if (!has_ptr) {
        return jerry_create_error(JERRY_ERROR_TYPE,
                                  (const jerry_char_t *) "Failed to get native PwmOut pointer");
    }

    PwmOut* native_ptr = static_cast<PwmOut*>(void_ptr);

    double arg0 = jerry_get_number_value(args[0]);
    native_ptr->period_ms(static_cast<int>(arg0));

    return jerry_create_undefined();
}

/**
 * PwmOut#period_us (native JavaScript method)
 *
 * Set the PWM period, specified in micro-seconds (int), keeping the duty cycle the same.
 */
DECLARE_CLASS_FUNCTION(PwmOut, period_us) {
    CHECK_ARGUMENT_COUNT(PwmOut, period_us, (args_count == 1));
    CHECK_ARGUMENT_TYPE_ALWAYS(PwmOut, period_us, 0, number);

    // Extract native PwmOut pointer
    void* void_ptr;
    bool has_ptr = jerry_get_object_native_pointer(this_obj, &void_ptr, &native_obj_type_info);

    if (!has_ptr) {
        return jerry_create_error(JERRY_ERROR_TYPE,
                                  (const jerry_char_t *) "Failed to get native PwmOut pointer");
    }

    PwmOut* native_ptr = static_cast<PwmOut*>(void_ptr);

    double arg0 = jerry_get_number_value(args[0]);
    native_ptr->period_us(static_cast<int>(arg0));

    return jerry_create_undefined();
}

/**
 * PwmOut#pulsewidth (native JavaScript method)
 *
 * Set the PWM pulsewidth, specified in seconds (float), keeping the period the same.
 */
DECLARE_CLASS_FUNCTION(PwmOut, pulsewidth) {
    CHECK_ARGUMENT_COUNT(PwmOut, pulsewidth, (args_count == 1));
    CHECK_ARGUMENT_TYPE_ALWAYS(PwmOut, pulsewidth, 0, number);

    // Extract native PwmOut pointer
    void* void_ptr;
    bool has_ptr = jerry_get_object_native_pointer(this_obj, &void_ptr, &native_obj_type_info);

    if (!has_ptr) {
        return jerry_create_error(JERRY_ERROR_TYPE,
                                  (const jerry_char_t *) "Failed to get native PwmOut pointer");
    }

    PwmOut* native_ptr = static_cast<PwmOut*>(void_ptr);

    double arg0 = jerry_get_number_value(args[0]);
    native_ptr->pulsewidth(static_cast<float>(arg0));

    return jerry_create_undefined();
}

/**
 * PwmOut#pulsewidth_ms (native JavaScript method)
 *
 * Set the PWM pulsewidth, specified in milli-seconds (int), keeping the period the same.
 */
DECLARE_CLASS_FUNCTION(PwmOut, pulsewidth_ms) {
    CHECK_ARGUMENT_COUNT(PwmOut, pulsewidth_ms, (args_count == 1));
    CHECK_ARGUMENT_TYPE_ALWAYS(PwmOut, pulsewidth_ms, 0, number);

    // Extract native PwmOut pointer
    void* void_ptr;
    bool has_ptr = jerry_get_object_native_pointer(this_obj, &void_ptr, &native_obj_type_info);

    if (!has_ptr) {
        return jerry_create_error(JERRY_ERROR_TYPE,
                                  (const jerry_char_t *) "Failed to get native PwmOut pointer");
    }

    PwmOut* native_ptr = static_cast<PwmOut*>(void_ptr);

    double arg0 = jerry_get_number_value(args[0]);
    native_ptr->pulsewidth_ms(static_cast<int>(arg0));

    return jerry_create_undefined();
}

/**
 * PwmOut#pulsewidth_us (native JavaScript method)
 *
 * Set the PWM pulsewidth, specified in micro-seconds (int), keeping the period the same.
 */
DECLARE_CLASS_FUNCTION(PwmOut, pulsewidth_us) {
    CHECK_ARGUMENT_COUNT(PwmOut, pulsewidth_us, (args_count == 1));
    CHECK_ARGUMENT_TYPE_ALWAYS(PwmOut, pulsewidth_us, 0, number);

    // Extract native PwmOut pointer
    void* void_ptr;
    bool has_ptr = jerry_get_object_native_pointer(this_obj, &void_ptr, &native_obj_type_info);

    if (!has_ptr) {
        return jerry_create_error(JERRY_ERROR_TYPE,
                                  (const jerry_char_t *) "Failed to get native PwmOut pointer");
    }

    PwmOut* native_ptr = static_cast<PwmOut*>(void_ptr);

    double arg0 = jerry_get_number_value(args[0]);
    native_ptr->pulsewidth_us(static_cast<int>(arg0));

    return jerry_create_undefined();
}

/**
 * PwmOut (native JavaScript constructor)
 *
 * @param pin_name mbed pin to connect the PwmOut to.
 * @returns a JavaScript object representing a PwmOut.
 */
DECLARE_CLASS_CONSTRUCTOR(PwmOut) {
    CHECK_ARGUMENT_COUNT(PwmOut, __constructor, args_count == 1);
    CHECK_ARGUMENT_TYPE_ALWAYS(PwmOut, __constructor, 0, number);

    PinName pin_name = PinName(jerry_get_number_value(args[0]));

    // Create the native object
    PwmOut* native_ptr = new PwmOut(pin_name);

    // create the jerryscript object
    jerry_value_t js_object = jerry_create_object();
    jerry_set_object_native_pointer(js_object, native_ptr, &native_obj_type_info);

    // attach methods
    ATTACH_CLASS_FUNCTION(js_object, PwmOut, write);
    ATTACH_CLASS_FUNCTION(js_object, PwmOut, read);
    ATTACH_CLASS_FUNCTION(js_object, PwmOut, period);
    ATTACH_CLASS_FUNCTION(js_object, PwmOut, period_ms);
    ATTACH_CLASS_FUNCTION(js_object, PwmOut, period_us);
    ATTACH_CLASS_FUNCTION(js_object, PwmOut, pulsewidth);
    ATTACH_CLASS_FUNCTION(js_object, PwmOut, pulsewidth_ms);
    ATTACH_CLASS_FUNCTION(js_object, PwmOut, pulsewidth_us);

    return js_object;
}
