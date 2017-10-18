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
#include "jerryscript-mbed-drivers/DigitalIn-js.h"
#include "jerryscript-mbed-drivers/DigitalOut-js.h"
#include "jerryscript-mbed-drivers/setInterval-js.h"
#include "jerryscript-mbed-drivers/setTimeout-js.h"
#include "jerryscript-mbed-drivers/I2C-js.h"
#include "jerryscript-mbed-drivers/AnalogIn-js.h"
#include "jerryscript-mbed-drivers/PwmOut-js.h"
#include "jerryscript-mbed-drivers/Serial-js.h"

DECLARE_JS_WRAPPER_REGISTRATION (base) {
    REGISTER_GLOBAL_FUNCTION_WITH_HANDLER(assert, jerryx_handler_assert);
    REGISTER_GLOBAL_FUNCTION_WITH_HANDLER(gc, jerryx_handler_gc);
    REGISTER_GLOBAL_FUNCTION_WITH_HANDLER(print, jerryx_handler_print);
    REGISTER_GLOBAL_FUNCTION(setInterval);
    REGISTER_GLOBAL_FUNCTION(setTimeout);
    REGISTER_GLOBAL_FUNCTION(clearInterval);
    REGISTER_GLOBAL_FUNCTION(clearTimeout);
    //REGISTER_CLASS_CONSTRUCTOR(AnalogOut);
    //REGISTER_CLASS_CONSTRUCTOR(BusIn);
    //REGISTER_CLASS_CONSTRUCTOR(BusInOut);
    //REGISTER_CLASS_CONSTRUCTOR(BusOut);
    //REGISTER_CLASS_CONSTRUCTOR(CAN);
    REGISTER_CLASS_CONSTRUCTOR(DigitalIn);          //test     
    //REGISTER_CLASS_CONSTRUCTOR(DigitalInOut);
    REGISTER_CLASS_CONSTRUCTOR(DigitalOut);
    //REGISTER_CLASS_CONSTRUCTOR(Ethernet);
    //REGISTER_CLASS_CONSTRUCTOR(FlashIAP);
    REGISTER_CLASS_CONSTRUCTOR(I2C);
    //REGISTER_CLASS_CONSTRUCTOR(I2CSlave);
    REGISTER_CLASS_CONSTRUCTOR(InterruptIn);
    REGISTER_CLASS_CONSTRUCTOR(AnalogIn);
    //REGISTER_CLASS_CONSTRUCTOR(InterruptManager);
    //REGISTER_CLASS_CONSTRUCTOR(LowPowerTicker);
    //REGISTER_CLASS_CONSTRUCTOR(LowPowerTimeout);
    //REGISTER_CLASS_CONSTRUCTOR(LowPowerTimer);
    //REGISTER_CLASS_CONSTRUCTOR(PortIn);
    //REGISTER_CLASS_CONSTRUCTOR(PortInOut);
    //REGISTER_CLASS_CONSTRUCTOR(PortOut);
    REGISTER_CLASS_CONSTRUCTOR(PwmOut);
    //REGISTER_CLASS_CONSTRUCTOR(RawSerial);
    //REGISTER_CLASS_CONSTRUCTOR(SerialBase);
    REGISTER_CLASS_CONSTRUCTOR(Serial);             //test
    //REGISTER_CLASS_CONSTRUCTOR(SPI);
    //REGISTER_CLASS_CONSTRUCTOR(SPISlave);
    //REGISTER_CLASS_CONSTRUCTOR(Ticker);
    //REGISTER_CLASS_CONSTRUCTOR(Timeout);
    //REGISTER_CLASS_CONSTRUCTOR(Timer);
    //REGISTER_CLASS_CONSTRUCTOR(TimerEvent);
    //REGISTER_CLASS_CONSTRUCTOR(Timer);
    //REGISTER_CLASS_CONSTRUCTOR(UARTSerial);
}

#endif  // _JERRYSCRIPT_MBED_DRIVERS_LIB_DRIVERS_H
