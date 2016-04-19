/* Copyright 2016 University of Szeged.
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

#define _BSD_SOURCE
#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>

#include "jerry-core/jerry-port.h"

#include "mbed-hal/us_ticker_api.h"

/**
 * Provide log message to filestream implementation for the engine.
 */
int
jerry_port_logmsg (FILE *stream, /**< stream pointer */
                   const char *format, /**< format string */
                   ...) /**< parameters */
{
  va_list args;
  int count;
  va_start (args, format);
  count = vfprintf (stream, format, args);
  va_end (args);
  return count;
} /* jerry_port_logmsg */

/**
 * Provide error message to console implementation for the engine.
 */
int
jerry_port_errormsg (const char *format, /**< format string */
                     ...) /**< parameters */
{
  va_list args;
  int count;
  va_start (args, format);
  count = vfprintf (stderr, format, args);
  va_end (args);
  return count;
} /* jerry_port_errormsg */

/**
 * Implementation of jerry_port_fatal.
 */
void
jerry_port_fatal (jerry_fatal_code_t code) /**< fatal code enum item */
{
  exit (code);
} /* jerry_port_fatal */

/**
 * Implementation of jerry_port_get_time_zone.
 * 
 * @return true - if success
 */
bool
jerry_port_get_time_zone (jerry_time_zone_t *tz_p) /**< timezone pointer */
{
  tz_p->offset = 0;
  tz_p->daylight_saving_time = 0;
  return true;
} /* jerry_port_get_time_zone */

/**
 * Implementation of jerry_port_get_current_time.
 *
 * @return current timer's counter value in microseconds 
 */
double
jerry_port_get_current_time ()
{
  return (double) us_ticker_read ();
} /* jerry_port_get_current_time */
