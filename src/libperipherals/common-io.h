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

#ifndef COMMON_IO_H
#define	COMMON_IO_H

#include "globals.h"

int digital_read(uint32_t, uint32_t);
void digital_write(uint32_t, uint32_t);

int analog_read(uint32_t, uint32_t);
void analog_write(uint32_t, uint32_t);
    
void wait_ms(uint32_t);


#ifdef __TARGET_MCU
void fake_exit(void);

void initialize_timer(void);
void initialize_sys_tick(void);
void SysTick_Handler(void);

void time_tick_decrement(void);
void wait_1ms(void);
#endif

#endif	/* COMMON_IO_H */

