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
#ifndef _JERRYSCRIPT_MBED_DRIVERS_LIB_DRIVERS_H
#define _JERRYSCRIPT_MBED_DRIVERS_LIB_DRIVERS_H

#include "jerryscript-ext/handler.h"
#include "jerryscript-mbed-drivers/InterruptIn-js.h"
#include "jerryscript-mbed-drivers/DigitalOut-js.h"
#include "jerryscript-mbed-drivers/setInterval-js.h"
#include "jerryscript-mbed-drivers/setTimeout-js.h"
#include "jerryscript-mbed-drivers/I2C-js.h"
#include "jerryscript-mbed-drivers/AnalogIn-js.h"
#include "jerryscript-mbed-drivers/PwmOut-js.h"

DECLARE_JS_WRAPPER_REGISTRATION (base) {
    REGISTER_GLOBAL_FUNCTION_WITH_HANDLER(assert, jerryx_handler_assert);
    REGISTER_GLOBAL_FUNCTION_WITH_HANDLER(gc, jerryx_handler_gc);
    REGISTER_GLOBAL_FUNCTION_WITH_HANDLER(print, jerryx_handler_print);
    REGISTER_GLOBAL_FUNCTION(setInterval);
    REGISTER_GLOBAL_FUNCTION(setTimeout);
    REGISTER_GLOBAL_FUNCTION(clearInterval);
    REGISTER_GLOBAL_FUNCTION(clearTimeout);
    REGISTER_CLASS_CONSTRUCTOR(DigitalOut);
    REGISTER_CLASS_CONSTRUCTOR(I2C);
    REGISTER_CLASS_CONSTRUCTOR(InterruptIn);
    REGISTER_CLASS_CONSTRUCTOR(AnalogIn);
    REGISTER_CLASS_CONSTRUCTOR(PwmOut);
}

#endif  // _JERRYSCRIPT_MBED_DRIVERS_LIB_DRIVERS_H
