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

#ifdef __TARGET_MCU
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "stm32f4xx_conf.h"
#include "stm32f4xx.h"
#pragma GCC diagnostic pop
#endif

#include "actuators.h"
#include "common-io.h"
#include "jerry-libc.h"

void
led_toggle (uint32_t led_id)
{
#ifdef __TARGET_HOST
  __printf ("led_toggle: %d\n", led_id);
#endif


#ifdef __TARGET_MCU
  GPIOD->ODR ^= (uint16_t) (1 << led_id);
#endif
}

void
led_on (uint32_t led_id)
{
#ifdef __TARGET_HOST
  __printf ("led_on: %d\n", led_id);
#endif


#ifdef __TARGET_MCU
  GPIO_WriteBit (GPIOD, (uint16_t) (1 << led_id), Bit_SET);
#endif
}

void
led_off (uint32_t led_id)
{
#ifdef __TARGET_HOST
  __printf ("led_off: %d\n", led_id);
#endif

#ifdef __TARGET_MCU
  GPIO_WriteBit (GPIOD, (uint16_t) (1 << led_id), Bit_RESET);
#endif
}

void
led_blink_once (uint32_t led_id)
{
#ifdef __TARGET_HOST
  __printf ("led_blink_once: %d\n", led_id);
#endif

#ifdef __TARGET_MCU
  uint32_t dot = 300000;

  GPIOD->BSRRL = (uint16_t) (1 << led_id);
  wait_ms (dot);
  GPIOD->BSRRH = (uint16_t) (1 << led_id);
#endif
}

#ifdef __TARGET_MCU

void
initialize_leds ()
{
  RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOD, ENABLE);

  GPIO_InitTypeDef gpioStructure;
  gpioStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
  gpioStructure.GPIO_Mode = GPIO_Mode_OUT;
  gpioStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_Init (GPIOD, &gpioStructure);

  GPIO_WriteBit (GPIOD,
                 GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15,
                 Bit_RESET);
}
#endif
