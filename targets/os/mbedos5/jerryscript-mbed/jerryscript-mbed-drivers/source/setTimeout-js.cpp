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
#include "jerryscript-mbed-drivers/setTimeout-js.h"

#include "jerryscript-mbed-event-loop/EventLoop.h"

/**
 * setTimeout (native JavaScript function)
 *
 * Call a JavaScript function once, after a fixed time period.
 *
 * @param function Function to call
 * @param wait_time Time before function is called, in ms.
 */
DECLARE_GLOBAL_FUNCTION (setTimeout)
{
  CHECK_ARGUMENT_COUNT (global, setTimeout, (args_count == 2));
  CHECK_ARGUMENT_TYPE_ALWAYS (global, setTimeout, 0, function);
  CHECK_ARGUMENT_TYPE_ALWAYS (global, setTimeout, 1, number);

  int interval = int (jerry_value_as_number (args[1]));

  int id = mbed::js::EventLoop::getInstance ().getQueue ().call_in (interval,
                                                                    jerry_call,
                                                                    args[0],
                                                                    jerry_null (),
                                                                    (jerry_value_t*) NULL,
                                                                    0);

  jerry_value_t result = jerry_object_set_index (call_info_p->function, id, args[0]);

  if (jerry_value_is_exception (result))
  {
    jerry_value_free (result);
    mbed::js::EventLoop::getInstance ().getQueue ().cancel (id);

    return jerry_throw_sz (JERRY_ERROR_TYPE, "Failed to run setTimeout");
  }

  jerry_value_free (result);
  return jerry_number (id);
}

/**
 * clearTimeout (native JavaScript function)
 *
 * Cancel an event that was previously scheduled via setTimeout.
 *
 * @param id ID of the timeout event, returned by setTimeout.
 */
DECLARE_GLOBAL_FUNCTION (clearTimeout)
{
  CHECK_ARGUMENT_COUNT (global, clearTimeout, (args_count == 1));
  CHECK_ARGUMENT_TYPE_ALWAYS (global, clearTimeout, 0, number);

  int id = int (jerry_value_as_number (args[0]));

  mbed::js::EventLoop::getInstance ().getQueue ().cancel (id);

  jerry_value_t global_obj = jerry_current_realm ();
  jerry_value_t prop_name = jerry_string_sz ("setTimeout");
  jerry_value_t func_obj = jerry_object_get (global_obj, prop_name);
  jerry_value_free (prop_name);

  jerry_object_delete_index (func_obj, id);
  jerry_value_free (func_obj);
  jerry_value_free (global_obj);

  return jerry_undefined ();
}
