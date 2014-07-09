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

#ifdef JERRY_NDEBUG
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#ifdef __TARGET_MCU
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"

#define LED_GREEN      12
#define LED_ORANGE     13
#define LED_RED        14
#define LED_BLUE       15
#endif

#include "error.h"

#include "ctx-manager.h"
#include "mem-allocator.h"

#include "interpreter.h"

#include "generated.h"

void fake_exit ();

void
fake_exit (void)
{
#ifdef __TARGET_MCU
  int pin = LED_RED;
  uint32_t mode = GPIO_Mode_OUT << (pin * 2);
  uint32_t speed = GPIO_Speed_100MHz << (pin * 2);
  uint32_t type = GPIO_OType_PP << pin;
  uint32_t pullup = GPIO_PuPd_NOPULL << (pin * 2);
  //
  //  Initialise the peripheral clock.
  //
  RCC->AHB1ENR |= RCC_AHB1Periph_GPIOD;
  //
  //  Initilaise the GPIO port.
  //
  GPIOD->MODER |= mode;
  GPIOD->OSPEEDR |= speed;
  GPIOD->OTYPER |= type;
  GPIOD->PUPDR |= pullup;
  //
  //  Toggle the selected LED indefinitely.
  //
  int index;
  
  // SOS
  
  int dot = 600000;
  int dash = dot * 3;
  
  while (1)
  {
    GPIOD->BSRRL = (1 << pin); for (index = 0; index < dot; index++); GPIOD->BSRRH = (1 << pin); for (index = 0; index < dash; index++);
    GPIOD->BSRRL = (1 << pin); for (index = 0; index < dot; index++); GPIOD->BSRRH = (1 << pin); for (index = 0; index < dash; index++);
    GPIOD->BSRRL = (1 << pin); for (index = 0; index < dot; index++); GPIOD->BSRRH = (1 << pin); for (index = 0; index < dash; index++);
    
    GPIOD->BSRRL = (1 << pin); for (index = 0; index < dash; index++); GPIOD->BSRRH = (1 << pin); for (index = 0; index < dash; index++);
    GPIOD->BSRRL = (1 << pin); for (index = 0; index < dash; index++); GPIOD->BSRRH = (1 << pin); for (index = 0; index < dash; index++);
    GPIOD->BSRRL = (1 << pin); for (index = 0; index < dash; index++); GPIOD->BSRRH = (1 << pin); for (index = 0; index < dash; index++);
    
    GPIOD->BSRRL = (1 << pin); for (index = 0; index < dot; index++); GPIOD->BSRRH = (1 << pin); for (index = 0; index < dash; index++);
    GPIOD->BSRRL = (1 << pin); for (index = 0; index < dot; index++); GPIOD->BSRRH = (1 << pin); for (index = 0; index < dash; index++);
    GPIOD->BSRRL = (1 << pin); for (index = 0; index < dot; index++); GPIOD->BSRRH = (1 << pin);
    
    for (index = 0; index < dash * 7; index++);
  }
#else
  for (;;);
#endif
}

int
main (int argc, char **argv)
{
  bool dump_tokens = false;
  bool dump_ast = true;
  const char *file_name = NULL;
#ifdef JERRY_NDEBUG
  FILE *file = NULL;
#endif

  mem_Init ();
  ctx_Init ();

  if (argc > 0)
    for (int i = 1; i < argc; i++)
    {
      if (!strcmp ("-t", argv[i]))
        dump_tokens = true;
      else if (!strcmp ("-a", argv[i]))
        dump_ast = true;
      else if (file_name == NULL)
        file_name = argv[i];
      else
        fatal (ERR_SEVERAL_FILES);
    }

  if (file_name == NULL)
    fatal (ERR_NO_FILES);

  if (dump_tokens && dump_ast)
    fatal (ERR_SEVERAL_FILES);

#ifdef JERRY_NDEBUG
  file = fopen (file_name, "r");

  if (file == NULL)
  {
    fatal (ERR_IO);
  }
#endif

  TODO(Call parser);

  //gen_bytecode (generated_source);
  //gen_bytecode ();
  //run_int ();

#ifdef __TARGET_MCU
  fake_exit ();
#endif 

  return 0;
}
