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
#include "jerryscript-mbed-drivers/setInterval-js.h"
#include "jerryscript-mbed-event-loop/EventLoop.h"

/**
 * setInterval (native JavaScript function)
 *
 * Call a JavaScript function at fixed intervals.
 *
 * @param function Function to call
 * @param interval Time between function calls, in ms.
 */
DECLARE_GLOBAL_FUNCTION(setInterval) {
    CHECK_ARGUMENT_COUNT(global, setInterval, (args_count == 2));
    CHECK_ARGUMENT_TYPE_ALWAYS(global, setInterval, 0, function);
    CHECK_ARGUMENT_TYPE_ALWAYS(global, setInterval, 1, number);

    jerry_acquire_value(args[0]);
    int interval = int(jerry_get_number_value(args[1]));

    int id = mbed::js::EventLoop::getInstance().getQueue().call_every(interval, jerry_call_function, args[0], jerry_create_null(), (jerry_value_t*)NULL, 0);

    return jerry_create_number(id);
}

/**
 * clearInterval (native JavaScript function)
 *
 * Cancel an event that was previously scheduled via setInterval.
 *
 * @param id ID of the timeout event, returned by setInterval.
 */
DECLARE_GLOBAL_FUNCTION(clearInterval) {
    CHECK_ARGUMENT_COUNT(global, clearInterval, (args_count == 1));
    CHECK_ARGUMENT_TYPE_ALWAYS(global, clearInterval, 0, number);

    int id = int(jerry_get_number_value(args[0]));

    mbed::js::EventLoop::getInstance().getQueue().cancel(id);

    return jerry_create_undefined();
}
