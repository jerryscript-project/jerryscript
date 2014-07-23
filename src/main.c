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

#define LED_GREEN      12
#define LED_ORANGE     13
#define LED_RED        14
#define LED_BLUE       15
#endif

#include "generated.h"
#include "globals.h"
#include "interpreter.h"
#include "jerry-libc.h"
#include "lexer.h"
#include "parser.h"

#define DUMP_TOKENS   (1u << 0)
#define DUMP_AST      (1u << 1)
#define DUMP_BYTECODE (1u << 2)

#define MAX_STRINGS 100

void fake_exit (void);

void
fake_exit (void)
{
#ifdef __TARGET_MCU
  uint32_t pin = LED_RED;
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
  int index;
  
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
#else
  for (;;);
#endif
}

int
main (int argc __unused,
      char **argv __unused)
{
#ifdef __HOST
  const char *file_name = NULL;
  FILE *file = NULL;
#endif

  mem_init ();

#ifdef __HOST
  if (argc > 0)
    {
      if (file_name == NULL)
        file_name = argv[1];
      else
        jerry_exit (ERR_SEVERAL_FILES);
    }
#endif

#ifdef __HOST
  if (file_name == NULL)
    jerry_exit (ERR_NO_FILES);

  file = __fopen (file_name, "r");

  if (file == NULL)
  {
    jerry_exit (ERR_IO);
  }

  lexer_set_file (file);
#else
  lexer_set_source (generated_source);
#endif

  // const char *strings[MAX_STRINGS];
  // uint8_t strings_num;
  // First run parser to fill list of strings
  token tok = lexer_next_token ();
  while (tok.type != TOK_EOF)
    tok = lexer_next_token ();

  // strings_num = lexer_get_strings (strings);

  // Reset lexer
#ifdef __HOST
  __rewind (file);
  lexer_set_file (file);
#else
  lexer_set_source (generated_source);
#endif

  parser_init ();
  TODO (serializer_dump_data (strings strings_num));
  parser_parse_program ();

  //gen_bytecode (generated_source);
  //gen_bytecode ();
  //run_int ();

#ifdef __TARGET_MCU
  fake_exit ();
#endif 

  return 0;
}
