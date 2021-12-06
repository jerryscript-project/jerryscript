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

#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>

#include "jerryscript-port.h"

#include "esp_common.h"

/**
 * Provide log message implementation for the engine.
 */
void
jerry_port_log (const char *message_p) /**< message */
{
  fputs (message_p, stderr);
} /* jerry_port_log */

/**
 * Provide fatal message implementation for the engine.
 */
void
jerry_port_fatal (jerry_fatal_code_t code)
{
  (void) code;

  jerry_port_log ("Jerry Fatal Error!\n");

  while (true)
  {
  }
} /* jerry_port_fatal */

/**
 * Implementation of jerry_port_current_time.
 *
 * @return current timer's counter value in milliseconds
 */
double
jerry_port_current_time (void)
{
  uint32_t rtc_time = system_rtc_clock_cali_proc ();
  return (double) rtc_time;
} /* jerry_port_current_time */

/**
 * Dummy function to get the time zone adjustment.
 *
 * @return 0
 */
int32_t
jerry_port_local_tza (double unix_ms)
{
  (void) unix_ms;

  /* We live in UTC. */
  return 0;
} /* jerry_port_local_tza */
