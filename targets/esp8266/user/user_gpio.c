/* Copyright 2015 Samsung Electronics Co., Ltd.
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

/*
 *  Copyright (C) 2014 -2016  Espressif System
 *
 */

#include "esp_common.h"

#include "esp8266_gpio.h"


//-----------------------------------------------------------------------------

void gpio_output_conf(uint32 set_mask, uint32 clear_mask, uint32 enable_mask,
                      uint32 disable_mask) {
  GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, set_mask);
  GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, clear_mask);
  GPIO_REG_WRITE(GPIO_ENABLE_W1TS_ADDRESS, enable_mask);
  GPIO_REG_WRITE(GPIO_ENABLE_W1TC_ADDRESS, disable_mask);
}


uint32 gpio_input_get(void) {
  return GPIO_REG_READ(GPIO_IN_ADDRESS);
}
