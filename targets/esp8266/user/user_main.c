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
#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <FreeRTOS.h>
#include <task.h>

#include "jerry_run.h"

/**
 * Start JerryScript in a task
 */
static void
jerry_task (void *pvParameters)
{
  if (jerry_task_init ())
  {
    while (js_loop ())
    {
      vTaskDelay (100 / portTICK_PERIOD_MS);
    }
  }

  jerry_task_exit ();
  while (true)
  {
  }
} /* jerry_task */

#if REDIRECT_STDOUT >= STDOUT_REDIRECT_ON
/**
 * Redirect stdout to UART1
 */
ssize_t
_write_stdout_r (struct _reent *r, int fd, const void *ptr, size_t len)
{
#if REDIRECT_STDOUT == STDOUT_REDIRECT_ON
  for (int i = 0; i < len; i++)
  {
    char c = ((char *) ptr)[i];

    if (c == '\r')
    {
      continue;
    }

    if (c == '\n')
    {
      uart_putc (STDOUT_UART_NUM, '\r');
    }

    uart_putc (STDOUT_UART_NUM, c);
  }

  return len;
#else
  return 0;
#endif /* REDIRECT_STDOUT == STDOUT_REDIRECT_ON */
} /* _write_stdout_r */
#endif /* REDIRECT_STDOUT >= STDOUT_REDIRECT_ON */

/**
 * This is entry point for user code
 */
void
user_init (void)
{
#if REDIRECT_STDOUT == STDOUT_REDIRECT_ON
  PIN_FUNC_SELECT (PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);
  uart_set_baud (STDOUT_UART_NUM, 115200);
#endif /* REDIRECT_STDOUT == 1 */

#if REDIRECT_STDOUT >= STDOUT_REDIRECT_ON
  set_write_stdout (_write_stdout_r);
#else
  uart_set_baud (STDOUT_UART_NUM, 115200);
#endif /* REDIRECT_STDOUT >= STDOUT_REDIRECT_ON */

#ifdef JERRY_DEBUGGER
  struct sdk_station_config config =
  {
     .ssid = "SSID",
     .password = "PASSWORD"
  };

  sdk_wifi_set_opmode (STATION_MODE);
  sdk_wifi_station_set_config (&config);
  sdk_wifi_station_connect ();
#endif /* JERRY_DEBUGGER */

  BaseType_t xReturned;
  TaskHandle_t xHandle = NULL;
  xReturned = xTaskCreate (jerry_task, "jerry", 1024, NULL, 10, &xHandle);
  if (xReturned != pdPASS)
  {
    printf ("Cannot allocate memory to task.\n");
    while (true)
    {
    }
  }
} /* user_init */
