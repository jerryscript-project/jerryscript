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
#include <stdlib.h>
#include <string.h>

#include "jerryscript.h"
#include "jerryscript-port.h"

#include "mbed.h"

/**
 * JerryScript log level.
 */
static jerry_log_level_t jerry_log_level = JERRY_LOG_LEVEL_ERROR;

/**
 * Sets log level.
 */
void set_log_level (jerry_log_level_t level)
{
  jerry_log_level = level;
} /* set_log_level */

/**
 * Aborts the program.
 */
void jerry_port_fatal (jerry_fatal_code_t code)
{
  exit (1);
} /* jerry_port_fatal */

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

  if (strlen (format) == 1 && format[0] == 0x0a) /* line feed (\n) */
  {
    printf ("\r"); /* add CR for proper display in serial monitors */
  }
} /* jerry_port_log */

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
 * Implementation of jerry_port_get_current_time.
 *
 * @return current timer's counter value in milliseconds
 */
double
jerry_port_get_current_time (void)
{
  static uint64_t last_tick = 0;
  static time_t last_time = 0;
  static uint32_t skew = 0;

  uint64_t curr_tick = us_ticker_read (); /* The value is in microseconds. */
  time_t curr_time = time(NULL); /*  The value is in seconds. */
  double result = curr_time * 1000;

  /* The us_ticker_read () has an overflow for each UINT_MAX microseconds
   * (~71 mins). For each overflow event the ticker-based clock is about 33
   * milliseconds fast. Without a timer thread the milliseconds part of the
   * time can be corrected if the difference of two get_current_time calls
   * are within the mentioned 71 mins. Above that interval we can assume
   * that the milliseconds part of the time is negligibe.
   */
  if (curr_time - last_time > (time_t) (((uint32_t) - 1) / 1000000)) {
    skew = 0;
  } else if (last_tick > curr_tick) {
    skew = (skew + 33) % 1000;
  }
  result += (curr_tick / 1000 - skew) % 1000;

  last_tick = curr_tick;
  last_time = curr_time;
  return result;
} /* jerry_port_get_current_time */

/**
 * Provide the implementation of jerry_port_print_char.
 * Uses 'printf' to print a single character to standard output.
 */
void
jerry_port_print_char (char c) /**< the character to print */
{
  printf ("%c", c);
} /* jerry_port_print_char */
