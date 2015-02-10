/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#ifndef ACTUATORS_H
#define ACTUATORS_H

#include "jrt.h"

void led_toggle (uint32_t);
void led_on (uint32_t);
void led_off (uint32_t);
void led_blink_once (uint32_t);

#ifdef __TARGET_MCU
void initialize_leds (void);
#endif

#endif /* ACTUATORS_H */

