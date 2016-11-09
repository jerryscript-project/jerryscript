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

#include "js_runtime.h"
#include "gpio.h"

#include <jerryscript.h>
#include <jerryscript-ext/arg.h>
#include <jerryscript-ext/autorelease.h>
#include <stdio.h>

#define JERRY_UNUSED(x) ((void) (x))

#define API_NAME ("api")
#define GPIO_NAME ("gpio")
#define ONUART_NAME ("onuart")

// call api.onuart(msg)
void
js_runtime_call_on_uart(char *c_str_msg)
{
  JERRYX_AR_VALUE_T global = jerry_get_global_object ();
  JERRYX_AR_VALUE_T api_name = jerry_create_string ((const jerry_char_t *)API_NAME);
  JERRYX_AR_VALUE_T api = jerry_get_property (global, api_name);
  JERRYX_AR_VALUE_T onuart_name = jerry_create_string ((const jerry_char_t *)ONUART_NAME);
  JERRYX_AR_VALUE_T onuart = jerry_get_property (api, onuart_name);

  if (!jerry_value_is_function (onuart)) {
    printf("Got message but no api.onuart function!");
    return;
  }

  JERRYX_AR_VALUE_T msg = jerry_create_string ((const jerry_char_t *)c_str_msg);
  JERRYX_AR_VALUE_T result = jerry_call_function (onuart, api, &msg, 1);
}

// api.gpio(pin, on) handler
static
jerry_value_t
_gpio_handler(const jerry_value_t function_obj,
              const jerry_value_t this_val,
              const jerry_value_t args_p[],
              const jerry_length_t args_cnt)
{
  uint8_t pin = 0;
  bool on = false;

  jerryx_arg_t mapping[] = {
      jerryx_arg_uint8 (&pin, JERRYX_ARG_ROUND, JERRYX_ARG_CLAMP, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED),
      jerryx_arg_boolean (&on, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED),
  };

  JERRYX_AR_VALUE_T result = jerryx_arg_transform_args (args_p, args_cnt, mapping,
                                                       sizeof(mapping) / sizeof(jerryx_arg_t));
  if (jerry_value_has_error_flag (result)) {
    return result;
  }

  gpio_set (pin, on);

  return jerry_create_undefined ();
}

// Create api object
static jerry_value_t
_create_api (void)
{
  const jerry_value_t api = jerry_create_object ();
  JERRYX_AR_VALUE_T gpio = jerry_create_external_function (_gpio_handler);
  JERRYX_AR_VALUE_T gpio_name = jerry_create_string ((const jerry_char_t *)GPIO_NAME);
  JERRYX_AR_VALUE_T rv = jerry_set_property (api, gpio_name, gpio);
  return api;
}

// Init JerryScript and add global.api
void
js_runtime_init (void)
{
  jerry_init (JERRY_INIT_EMPTY);

  JERRYX_AR_VALUE_T api = _create_api();
  JERRYX_AR_VALUE_T global = jerry_get_global_object ();
  JERRYX_AR_VALUE_T api_name = jerry_create_string ((const jerry_char_t *)API_NAME);
  JERRYX_AR_VALUE_T rv = jerry_set_property (global, api_name, api);
}
