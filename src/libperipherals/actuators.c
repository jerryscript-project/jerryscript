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
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#pragma GCC diagnostic pop
#endif

#include "actuators.h"
#include "common-io.h"
#include "jerry-libc.h"

void
led_toggle (uint32_t led_id)
{
#ifdef __HOST
  __printf ("led_toggle: %d\n", led_id);
#endif


#ifdef __TARGET_MCU
  init_led (led_id);

  GPIOD->ODR ^= (uint16_t) (1 << led_id);
#endif
}

void
led_on (uint32_t led_id)
{
#ifdef __HOST
  __printf ("led_on: %d\n", led_id);
#endif


#ifdef __TARGET_MCU
  init_led (led_id);

  GPIOD->BSRRH = (uint16_t) (1 << led_id);
#endif
}

void
led_off (uint32_t led_id)
{
#ifdef __HOST
  __printf ("led_off: %d\n", led_id);
#endif

#ifdef __TARGET_MCU
  init_led (led_id);

  GPIOD->BSRRH = (uint16_t) (1 << led_id);
#endif
}

void
led_blink_once (uint32_t led_id)
{
#ifdef __HOST
  __printf ("led_blink_once: %d\n", led_id);
#endif

#ifdef __TARGET_MCU
  init_led (led_id);

  uint32_t dot = 600000 * 3;

  GPIOD->BSRRL = (uint16_t) (1 << led_id);
  wait_ms (dot);

  GPIOD->BSRRH = (uint16_t) (1 << led_id);
  wait_ms (dot);
#endif
}

#ifdef __TARGET_MCU
void
init_led (uint32_t led_id)
{
  uint32_t pin = led_id;
  uint32_t mode = (uint32_t) GPIO_Mode_OUT << (pin * 2);
  uint32_t speed = (uint32_t) GPIO_Speed_100MHz << (pin * 2);
  uint32_t type = (uint32_t) GPIO_OType_PP << pin;
  uint32_t pullup = (uint32_t) GPIO_PuPd_NOPULL << (pin * 2);

  TODO (INITIALIZE ONCE);

  //  Initialise the peripheral clock.
  RCC->AHB1ENR |= RCC_AHB1Periph_GPIOD;

  //  Initilaise the GPIO port.
  volatile GPIO_TypeDef* gpio = GPIOD;

  gpio->MODER |= mode;
  gpio->OSPEEDR |= speed;
  gpio->OTYPER |= type;
  gpio->PUPDR |= pullup;
}
#endif