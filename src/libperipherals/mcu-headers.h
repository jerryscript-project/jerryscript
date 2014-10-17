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

#ifndef MCU_HEADERS_H
#define MCU_HEADERS_H

#ifdef __TARGET_MCU
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wsign-conversion"
# ifdef __TARGET_MCU_STM32F4
#  include "stm32f4xx_conf.h"
#  include "stm32f4xx.h"

#  define LED_GREEN      0
#  define LED_ORANGE     1
#  define LED_RED        2
#  define LED_BLUE       3
# elif defined (__TARGET_MCU_STM32F3)
#  include "stm32f30x_conf.h"
#  include "stm32f30x.h"

#  define LED_GREEN      0
#  define LED_ORANGE     1
#  define LED_RED        2
#  define LED_BLUE       3
# else /* !__TARGET_MCU_STM32F4 && !__TARGET_MCU_STM32F3 */
#  error "!__TARGET_MCU_STM32F4 && !__TARGET_MCU_STM32F3"
# endif /* !__TARGET_MCU_STM32F4 && !__TARGET_MCU_STM32F3 */
#pragma GCC diagnostic pop
#endif /* __TARGET_MCU */

#endif /* !MCU_HEADERS_H */
