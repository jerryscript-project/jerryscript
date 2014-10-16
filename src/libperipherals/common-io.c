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

#include "common-io.h"
#include "jerry-libc.h"

#ifdef __TARGET_MCU
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "stm32f4xx_conf.h"
#include "stm32f4xx.h"
#pragma GCC diagnostic pop

// STM32 F4
#define LED_GREEN      12
#define LED_ORANGE     13
#define LED_RED        14
#define LED_BLUE       15
#endif

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
#ifdef __TARGET_HOST
  // 1 millisecond = 1,000,000 Nanoseconds
#define NANO_SECOND_MULTIPLIER  1000000
  __printf ("wait_ms: %d\n", time_ms);
#endif

#ifdef __TARGET_MCU
  while (time_ms --)
  {
    wait_1ms ();
  }
#endif
}

#ifdef __TARGET_MCU

void
initialize_timer ()
{
  RCC_APB1PeriphClockCmd (RCC_APB1Periph_TIM2, ENABLE);

  TIM_TimeBaseInitTypeDef timerInitStructure;
  timerInitStructure.TIM_Prescaler = 40000;
  timerInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
  timerInitStructure.TIM_Period = 500;
  timerInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  timerInitStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit (TIM2, &timerInitStructure);
  TIM_Cmd (TIM2, ENABLE);
}

void
fake_exit (void)
{
  uint32_t pin = LED_ORANGE;
  volatile GPIO_TypeDef* gpio = GPIOD;
  volatile int index;

  int dot = 600000;
  int dash = dot * 3;

  while (1)
  {
    gpio->BSRRL = (uint16_t) (1 << pin);
    for ( index = 0; index < dot; index ++)
    {
    };
    gpio->BSRRH = (uint16_t) (1 << pin);
    for (index = 0; index < dash; index ++)
    {
    };
  }
}

static __IO uint32_t sys_tick_counter;

void
initialize_sys_tick (void)
{
  /****************************************
   *SystemFrequency/1000      1ms         *
   *SystemFrequency/100000    10us        *
   *SystemFrequency/1000000   1us         *
   *****************************************/
  while (SysTick_Config (SystemCoreClock / 1000000) != 0)
  {
  } // One SysTick interrupt now equals 1us

}

void
set_sys_tick_counter (uint32_t set_value)
{
  sys_tick_counter = set_value;
}

uint32_t
get_sys_tick_counter (void)
{
  return sys_tick_counter;
}

void
SysTick_Handler (void)
{
  time_tick_decrement ();
}

void
time_tick_decrement (void)
{
  if (sys_tick_counter != 0x00)
  {
    sys_tick_counter --;
  }
}

void
wait_1ms (void)
{
  sys_tick_counter = 1000;
  while (sys_tick_counter != 0)
  {
  }
}
#endif
