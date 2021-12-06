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

#include <infra/time.h>
#include <misc/printk.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "jerryscript-port.h"

/**
 * Provide log message implementation for the engine.
 * Curie BSP implementation
 */
void
jerry_port_log (jerry_log_level_t level, /**< log level */
                const char *format, /**< format string */
                ...)  /**< parameters */
{
  if (level <= JERRY_LOG_LEVEL_ERROR)
  {
    char buf[256];
    int length = 0;
    va_list args;
    va_start (args, format);
    length = vsnprintf (buf, 256, format, args);
    buf[length] = '\0';
    printk ("%s", buf);
    va_end (args);
  }
} /* jerry_port_log */

/**
 * Curie BSP implementation of jerry_port_fatal.
 */
void jerry_port_fatal (jerry_fatal_code_t code)
{
  jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Jerry Fatal Error!\n");
  while (true);
} /* jerry_port_fatal */

/**
 * Curie BSP implementation of jerry_port_get_local_time_zone_adjustment.
 */
double jerry_port_get_local_time_zone_adjustment (double unix_ms, bool is_utc)
{
  //EMPTY implementation
  return 0;
} /* jerry_port_get_local_time_zone_adjustment */

/**
 * Curie BSP implementation of jerry_port_get_current_time.
 */
double jerry_port_get_current_time (void)
{
  uint32_t uptime_ms = get_uptime_ms ();
  uint32_t epoch_time = uptime_to_epoch (uptime_ms);

  return ((double) epoch_time) * 1000.0;
} /* jerry_port_get_current_time */
