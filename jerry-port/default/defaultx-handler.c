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

#include <stdio.h>

#include "jerryscript-ext/handler.h"

#ifdef JERRY_DEBUGGER

#define DEBUG_BUFFER_SIZE (256)
static char debug_buffer[DEBUG_BUFFER_SIZE];
static int debug_buffer_index = 0;

#endif /* JERRY_DEBUGGER */

/**
 * Default implementation of jerryx_port_handler_print_char. Uses 'printf' to
 * print a single character to standard output.
 */
void
jerryx_port_handler_print_char (char c) /**< the character to print */
{
  printf ("%c", c);

#ifdef JERRY_DEBUGGER
  debug_buffer[debug_buffer_index++] = c;

  if ((debug_buffer_index == DEBUG_BUFFER_SIZE) || (c == '\n'))
  {
    jerry_debugger_send_output ((jerry_char_t *) debug_buffer, (jerry_size_t) debug_buffer_index);
    debug_buffer_index = 0;
  }
#endif /* JERRY_DEBUGGER */
} /* jerryx_port_handler_print_char */
