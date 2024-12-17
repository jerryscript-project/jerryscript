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

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "jerryscript-port.h"
#include "jerryscript.h"

#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char ESP_JS_TAG[] = "JS";

void
jerry_port_log (const char *message_p)
{
  ESP_LOGI (ESP_JS_TAG, "%s", message_p);
} /* jerry_port_log */

void
jerry_port_fatal (jerry_fatal_code_t code)
{
  ESP_LOGE (ESP_JS_TAG, "Fatal error: %d", code);
  vTaskSuspend (NULL);
  abort ();
} /* jerry_port_fatal */

int32_t
jerry_port_local_tza (double unix_ms)
{
  struct tm tm;
  char buf[8];
  time_t now = (time_t) unix_ms / 1000;

  localtime_r (&now, &tm);
  strftime (buf, 8, "%z", &tm);
  return -atoi (buf) * 3600 * 1000 / 100;
} /* jerry_port_local_tza */

double
jerry_port_current_time (void)
{
  struct timeval tv;

  if (gettimeofday (&tv, NULL) == 0)
  {
    return ((double) tv.tv_sec) * 1000.0 + ((double) tv.tv_usec) / 1000.0;
  }
  return 0.0;
} /* jerry_port_current_time */
