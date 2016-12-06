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

#include <zephyr.h>
#include <uart.h>
#include <drivers/console/uart_console.h>
#include "getline-zephyr.h"

/* While app processes one input line, Zephyr will have another line
   buffer to accumulate more console input. */
static struct uart_console_input line_bufs[2];

static struct k_fifo free_queue;
static struct k_fifo used_queue;

char *zephyr_getline(void)
{
  static struct uart_console_input *cmd;

  /* Recycle cmd buffer returned previous time */
  if (cmd != NULL)
  {
    k_fifo_put(&free_queue, cmd);
  }

  cmd = k_fifo_get(&used_queue, K_FOREVER);
  return cmd->line;
}

void zephyr_getline_init(void)
{
  int i;

  k_fifo_init(&used_queue);
  k_fifo_init(&free_queue);
  for (i = 0; i < sizeof(line_bufs) / sizeof(*line_bufs); i++)
  {
    k_fifo_put(&free_queue, &line_bufs[i]);
  }

  /* Zephyr UART handler takes an empty buffer from free_queue,
     stores UART input in it until EOL, and then puts it into
     used_queue. */
  uart_register_input(&free_queue, &used_queue, NULL);
}
