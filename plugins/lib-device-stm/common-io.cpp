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

#pragma GCC optimize "O0"

#include <stdio.h>

#include "actuators.h"
#include "common-io.h"

#include "mcu-headers.h"

int
digital_read (uint32_t arg1, uint32_t arg2)
{
  (void) arg1;
  (void) arg2;

  return 0;
}

void
digital_write (uint32_t arg1, uint32_t arg2)
{
  (void) arg1;
  (void) arg2;
}

int
analog_read (uint32_t arg1, uint32_t arg2)
{
  (void) arg1;
  (void) arg2;

  return 0;
}

void
analog_write (uint32_t arg1, uint32_t arg2)
{
  (void) arg1;
  (void) arg2;
}

#if defined (__TARGET_HOST) || defined (__TARGET_NUTTX)
void
wait_ms (uint32_t time_ms)
{
  printf ("wait_ms: %lu\n", time_ms);
}
#else /* !__TARGET_HOST */

#ifndef __TARGET_MCU
# error "!__TARGET_HOST && && !__TARGET_NUTTX !__TARGET_MCU"
#endif /* !__TARGET_MCU */

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

void
wait_ms (uint32_t time_ms)
{
  while (time_ms --)
  {
    wait_1ms ();
  }
}

void
fake_exit (void)
{
  uint32_t pin = LED_ORANGE;
  volatile int index;

  int dot = 600000;
  int dash = dot * 3;

  while (1)
  {
    led_on (pin);
    for (index = 0; index < dot; index ++)
    {
    };
    led_off (pin);
    for (index = 0; index < dash; index ++)
    {
    };
  }
}
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
#endif /* !__TARGET_HOST && !__TARGET_NUTTX && __TARGET_MCU */
