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
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"

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
#include "pretty-printer.h"

/* FIXME: Make general fatal function call it from libjsparser's fatal */
extern void fatal(int);

void fake_exit (void);

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
  statement st;
  bool dump_tokens = false;
  bool dump_ast = false;
  const char *file_name = NULL;
#ifdef __HOST
  FILE *file = NULL;
#endif

  mem_Init ();

  if (argc > 0)
    for (int i = 1; i < argc; i++)
    {
      if (!__strcmp ("-t", argv[i]))
        dump_tokens = true;
      else if (!__strcmp ("-a", argv[i]))
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

  if (!dump_tokens)
    dump_ast = true;

#ifdef __HOST
  file = __fopen (file_name, "r");

  if (file == NULL)
  {
    fatal (ERR_IO);
  }

  lexer_set_file (file);
#else
  lexer_set_source (generated_source);
#endif

  if (dump_ast)
    {
      parser_init ();
      st = parser_parse_statement ();
      JERRY_ASSERT (!is_statement_null (st));
      while (st.type != STMT_EOF)
        {
          pp_statement (st);
          st = parser_parse_statement ();
          JERRY_ASSERT (!is_statement_null (st));
        }
    }

  if (dump_tokens)
    {
      token tok = lexer_next_token ();
      while (tok.type != TOK_EOF)
        {
          pp_token (tok);
          tok = lexer_next_token ();
        }
    }

  //gen_bytecode (generated_source);
  //gen_bytecode ();
  //run_int ();

#ifdef __TARGET_MCU
  fake_exit ();
#endif 

  return 0;
}
