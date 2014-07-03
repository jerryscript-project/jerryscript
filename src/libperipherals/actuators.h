/* Copyright 2014 Samsung Electronics Co., Ltd.
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
#define	ACTUATORS_H

// STM32 F4
#define LED_GREEN      12
#define LED_ORANGE     13
#define LED_RED        14
#define LED_BLUE       15

void led_toggle(int);
void led_on(int);
void led_off(int);

#endif	/* ACTUATORS_H */

