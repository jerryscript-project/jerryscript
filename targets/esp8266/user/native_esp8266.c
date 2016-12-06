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

/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/12/1, v1.0 create this file.
*******************************************************************************/

#include "esp_common.h"

#include "user_config.h"
#include "esp8266_gpio.h"


void native_gpio_dir(int port, int value) {
  if (value) {
    GPIO_AS_OUTPUT(1 << port);
  }
  else {
    GPIO_AS_INPUT(1 << port);
  }
}


void native_gpio_set(int port, int value) {
  GPIO_OUTPUT_SET(port, value);
}


int native_gpio_get(int port) {
  return GPIO_INPUT_GET(port);
}
