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

#include <zephyr.h>

#include "jerryscript-port.h"


/**
 * Provide log message implementation for the engine.
 */
void
jerry_port_log (jerry_log_level_t level, /**< log level */
                const char *format, /**< format string */
                ...)  /**< parameters */
{
  (void) level; /* ignore log level */

  va_list args;
  va_start (args, format);
  vfprintf (stderr, format, args);
  va_end (args);
} /* jerry_port_log */


/**
 * Provide fatal message implementation for the engine.
 */
void jerry_port_fatal (jerry_fatal_code_t code)
{
  jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Jerry Fatal Error!\n");
  while (true);
} /* jerry_port_fatal */

/**
 * Implementation of jerry_port_get_current_time.
 *
 * @return current timer's counter value in milliseconds
 */
double
jerry_port_get_current_time (void)
{
  int64_t ms = k_uptime_get();
  return (double) ms;
} /* jerry_port_get_current_time */

/**
 * Dummy function to get the time zone adjustment.
 *
 * @return 0
 */
double
jerry_port_get_local_time_zone_adjustment (double unix_ms, bool is_utc)
{
  /* We live in UTC. */
  return 0;
} /* jerry_port_get_local_time_zone_adjustment */

/**
 * Provide the implementation of jerry_port_print_char.
 * Uses 'printf' to print a single character to standard output.
 */
void
jerry_port_print_char (char c) /**< the character to print */
{
  printf ("%c", c);
} /* jerry_port_print_char */
