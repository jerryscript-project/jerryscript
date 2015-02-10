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

#pragma GCC optimize "O0"

#include "actuators.h"
#include "common-io.h"
#include "jerry-libc.h"

#ifdef __TARGET_HOST
/**
 * Host stub for LEDToggle operation
 */
void
led_toggle (uint32_t led_id) /**< index of LED */
{
  __printf ("led_toggle: %d\n", led_id);
}

/**
 * Host stub for LEDOn operation
 */
void
led_on (uint32_t led_id) /**< index of LED */
{
  __printf ("led_on: %d\n", led_id);
}

/**
 * Host stub for LEDOff operation
 */
void
led_off (uint32_t led_id) /**< index of LED */
{
  __printf ("led_off: %d\n", led_id);
}

/**
 * Host stub for LEDOnce operation
 */
void
led_blink_once (uint32_t led_id) /**< index of LED */
{
  __printf ("led_blink_once: %d\n", led_id);
}
#else /* !__TARGET_HOST */
#ifndef __TARGET_MCU
# error "!__TARGET_HOST && !__TARGET_MCU"
#endif /* !__TARGET_MCU */

#include "mcu-headers.h"

#ifdef __TARGET_MCU_STM32F4
/**
 * Number of LEDs on the board
 */
#define LED_NUMBER 4

/**
 * LEDs' GPIO pins
 */
static const uint16_t led_pins [LED_NUMBER] =
{
  GPIO_Pin_12, /* LD4: Green */
  GPIO_Pin_13, /* LD3: Orange */
  GPIO_Pin_14, /* LD5: Red */
  GPIO_Pin_15  /* LD6: Blue */
};

/**
 * LEDs' GPIO ports
 */
static GPIO_TypeDef* leds_port = GPIOD;

/**
 * Initialize LEDs on the board
 */
void
initialize_leds (void)
{
  GPIO_InitTypeDef gpio_init_structure;

  RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOD, ENABLE);

  uint16_t led_pins_mask = 0;
  for (uint32_t led_index = 0;
       led_index < LED_NUMBER;
       led_index++)
  {
    led_pins_mask |= led_pins[led_index];
  }

  gpio_init_structure.GPIO_Pin = led_pins_mask;
  gpio_init_structure.GPIO_Mode = GPIO_Mode_OUT;
  gpio_init_structure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_Init (leds_port, &gpio_init_structure);

  for (uint32_t led_index = 0;
       led_index < LED_NUMBER;
       led_index++)
  {
    led_off (led_index);
  }
} /* initialize_leds */

#else /* !__TARGET_MCU_STM32F4 */

# ifndef __TARGET_MCU_STM32F3
#  error "!__TARGET_MCU_STM32F4 && !__TARGET_MCU_STM32F3"
# endif /* !__TARGET_MCU_STM32F3 */

/**
 * Number of LEDs on the board
 */
#define LED_NUMBER 8

/**
 * LEDs' GPIO pins
 */
static const uint16_t led_pins [LED_NUMBER] =
{
  GPIO_Pin_15, /* LD6  - Green */
  GPIO_Pin_14, /* LD8  - Orange */
  GPIO_Pin_13, /* LD10 - Red */
  GPIO_Pin_12, /* LD9  - Blue */
  GPIO_Pin_11, /* LD7  - Green */
  GPIO_Pin_10, /* LD5  - Orange */
  GPIO_Pin_9,  /* LD3  - Red */
  GPIO_Pin_8   /* LD4  - Blue */
};

/**
 * LEDs' GPIO ports
 */
static GPIO_TypeDef* leds_port = GPIOE;

/**
 * Initialize LEDs on the board
 */
void
initialize_leds (void)
{
  GPIO_InitTypeDef gpio_init_structure;

  RCC_AHBPeriphClockCmd (RCC_AHBPeriph_GPIOE, ENABLE);

  uint16_t led_pins_mask = 0;
  for (uint32_t led_index = 0;
       led_index < LED_NUMBER;
       led_index++)
  {
    led_pins_mask |= led_pins[led_index];
  }

  gpio_init_structure.GPIO_Pin = led_pins_mask;
  gpio_init_structure.GPIO_Mode = GPIO_Mode_OUT;
  gpio_init_structure.GPIO_OType = GPIO_OType_PP;
  gpio_init_structure.GPIO_PuPd = GPIO_PuPd_UP;
  gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init (leds_port, &gpio_init_structure);

  for (uint32_t led_index = 0;
       led_index < LED_NUMBER;
       led_index++)
  {
    led_off (led_index);
  }
} /* initialize_leds */
#endif /* !__TARGET_MCU_STM32F4 && __TARGET_MCU_STM32F3 */

/**
 * Toggle specified LED
 */
void
led_toggle (uint32_t led_id) /**< index of LED */
{
  if (led_id < LED_NUMBER)
  {
    leds_port->ODR ^= led_pins [led_id];
  }
} /* led_toggle */

/**
 * Turn specified LED on
 */
void
led_on (uint32_t led_id) /**< index of LED */
{
  if (led_id < LED_NUMBER)
  {
    GPIO_WriteBit (leds_port, led_pins[led_id], Bit_SET);
  }
} /* led_on */

/**
 * Turn specified LED off
 */
void
led_off (uint32_t led_id) /**< index of LED */
{
  if (led_id < LED_NUMBER)
  {
    GPIO_WriteBit (leds_port, led_pins[led_id], Bit_RESET);
  }
} /* led_off */

/**
 * Blink once with specified LED
 */
void
led_blink_once (uint32_t led_id) /**< index of LED */
{
  uint32_t dot = 300;

  led_on (led_id);
  wait_ms (dot);
  led_off (led_id);
} /* led_blink_once */

#endif /* !__TARGET_HOST && __TARGET_MCU */
