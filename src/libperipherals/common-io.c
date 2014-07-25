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

#ifdef __HOST
#include <time.h>
#endif

// STM32 F4
#define LED_GREEN      12
#define LED_ORANGE     13
#define LED_RED        14
#define LED_BLUE       15

#endif

#include "common-io.h"
#include "jerry-libc.h"

int
digital_read (uint32_t arg1 __unused, uint32_t arg2 __unused)
{
  JERRY_UNIMPLEMENTED ();
}

void
digital_write (uint32_t arg1 __unused, uint32_t arg2 __unused)
{
  JERRY_UNIMPLEMENTED ();
}

int
analog_read (uint32_t arg1 __unused, uint32_t arg2 __unused)
{
  JERRY_UNIMPLEMENTED ();
}

void
analog_write (uint32_t arg1 __unused, uint32_t arg2 __unused)
{
  JERRY_UNIMPLEMENTED ();
}

void
wait_ms (uint32_t time_ms)
{
#ifdef __HOST
  // 1 millisecond = 1,000,000 Nanoseconds
#define NANO_SECOND_MULTIPLIER  1000000
  __printf ("wait_ms: %d\n", time_ms);
  //  const long interval_ms = time_ms * NANO_SECOND_MULTIPLIER;
  //
  //  timespec sleep_value = {0};
  //  sleep_value.tv_nsec = interval_ms;
  //  nanosleep (&sleep_value, NULL);
#endif

#ifdef __TARGET_MCU
  volatile uint32_t index;
  for (index = 0; index < time_ms; index++);
#endif
}

#ifdef __TARGET_MCU
void
fake_exit (void)
{
  uint32_t pin = LED_ORANGE;
  uint32_t mode = (uint32_t)GPIO_Mode_OUT << (pin * 2);
  uint32_t speed = (uint32_t)GPIO_Speed_100MHz << (pin * 2);
  uint32_t type = (uint32_t)GPIO_OType_PP << pin;
  uint32_t pullup = (uint32_t)GPIO_PuPd_NOPULL << (pin * 2);
  //
  //  Initialise the peripheral clock.
  //
  RCC->AHB1ENR |= RCC_AHB1Periph_GPIOD;
  //
  //  Initilaise the GPIO port.
  //
  volatile GPIO_TypeDef* gpio = GPIOD;

  gpio->MODER |= mode;
  gpio->OSPEEDR |= speed;
  gpio->OTYPER |= type;
  gpio->PUPDR |= pullup;
  //
  //  Toggle the selected LED indefinitely.
  //
  volatile int index;
  
  // SOS
  
  int dot = 600000;
  int dash = dot * 3;
  
  while (1)
  {
    gpio->BSRRL = (uint16_t) (1 << pin); for (index = 0; index < dot; index++); gpio->BSRRH = (uint16_t) (1 << pin); for (index = 0; index < dash; index++);
    gpio->BSRRL = (uint16_t) (1 << pin); for (index = 0; index < dot; index++); gpio->BSRRH = (uint16_t) (1 << pin); for (index = 0; index < dash; index++);
    gpio->BSRRL = (uint16_t) (1 << pin); for (index = 0; index < dot; index++); gpio->BSRRH = (uint16_t) (1 << pin); for (index = 0; index < dash; index++);
    
    gpio->BSRRL = (uint16_t) (1 << pin); for (index = 0; index < dash; index++); gpio->BSRRH = (uint16_t) (1 << pin); for (index = 0; index < dash; index++);
    gpio->BSRRL = (uint16_t) (1 << pin); for (index = 0; index < dash; index++); gpio->BSRRH = (uint16_t) (1 << pin); for (index = 0; index < dash; index++);
    gpio->BSRRL = (uint16_t) (1 << pin); for (index = 0; index < dash; index++); gpio->BSRRH = (uint16_t) (1 << pin); for (index = 0; index < dash; index++);
    
    gpio->BSRRL = (uint16_t) (1 << pin); for (index = 0; index < dot; index++); gpio->BSRRH = (uint16_t) (1 << pin); for (index = 0; index < dash; index++);
    gpio->BSRRL = (uint16_t) (1 << pin); for (index = 0; index < dot; index++); gpio->BSRRH = (uint16_t) (1 << pin); for (index = 0; index < dash; index++);
    gpio->BSRRL = (uint16_t) (1 << pin); for (index = 0; index < dot; index++); gpio->BSRRH = (uint16_t) (1 << pin);
    
    for (index = 0; index < dash * 7; index++);
  }
}
#endif
