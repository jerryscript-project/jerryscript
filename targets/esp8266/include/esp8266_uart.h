/* Copyright 2015 Samsung Electronics Co., Ltd.
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

/*
 *  Copyright (C) 2014 -2016  Espressif System
 *
 */

#ifndef __ESP8266_UART_H__
#define __ESP8266_UART_H__

typedef enum {
  UART0 = 0x0,
  UART1 = 0x1,
} UART_Port;

typedef enum {
  BIT_RATE_300     = 300,
  BIT_RATE_600     = 600,
  BIT_RATE_1200    = 1200,
  BIT_RATE_2400    = 2400,
  BIT_RATE_4800    = 4800,
  BIT_RATE_9600    = 9600,
  BIT_RATE_19200   = 19200,
  BIT_RATE_38400   = 38400,
  BIT_RATE_57600   = 57600,
  BIT_RATE_74880   = 74880,
  BIT_RATE_115200  = 115200,
  BIT_RATE_230400  = 230400,
  BIT_RATE_460800  = 460800,
  BIT_RATE_921600  = 921600,
  BIT_RATE_1843200 = 1843200,
  BIT_RATE_3686400 = 3686400,
} UART_BautRate;

#endif
