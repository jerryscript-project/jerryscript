/* Copyright 2016 Intel Corporation
 * Copyright 2016 Linaro Limited
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

#include "jerry-port.h"

/**
 * Provide console message implementation for the engine.
 */
void
jerry_port_console (const char *format, /**< format string */
                    ...) /**< parameters */
{
  va_list args;
  va_start (args, format);
  vfprintf (stdout, format, args);
  va_end (args);
} /* jerry_port_console */


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
jerry_port_get_current_time ()
{
  int64_t ms = k_uptime_get();
  return (double) ms;
} /* jerry_port_get_current_time */

/**
 * Dummy function to get the time zone.
 *
 * @return true
 */
bool
jerry_port_get_time_zone (jerry_time_zone_t *tz_p)
{
  /* We live in UTC. */
  tz_p->offset = 0;
  tz_p->daylight_saving_time = 0;

  return true;
} /* jerry_port_get_time_zone */
