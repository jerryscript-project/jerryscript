/* Copyright JS Foundation and other contributors, http://js.foundation
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

#ifndef JERRY_RUN_H
#define JERRY_RUN_H

#include <stdint.h>
#include <stdbool.h>

#define STDOUT_REDIRECT_OFF 0
#define STDOUT_REDIRECT_ON 1
#define STDOUT_REDIRECT_DISCARD 2

#if REDIRECT_STDOUT >= STDOUT_REDIRECT_ON
#include <sys/reent.h>
#include <semphr.h>
#include <stdout_redirect.h>
#if REDIRECT_STDOUT == STDOUT_REDIRECT_ON
#include <espressif/esp8266/gpio_register.h>
#include <espressif/esp8266/pin_mux_register.h>
#endif /* REDIRECT_STDOUT == STDOUT_REDIRECT_ON */
#endif /* REDIRECT_STDOUT >= STDOUT_REDIRECT_ON */

#if REDIRECT_STDOUT >= STDOUT_REDIRECT_ON
#define STDOUT_UART_NUM 1
#else
#define STDOUT_UART_NUM 0
#endif /* REDIRECT_STDOUT >= STDOUT_REDIRECT_ON */

bool jerry_task_init (void);
bool js_loop (void);
void jerry_task_exit (void);

#endif /* JERRY_RUN_H */
