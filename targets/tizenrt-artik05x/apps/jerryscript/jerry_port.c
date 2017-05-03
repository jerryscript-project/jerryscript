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
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdarg.h>

#include "jerryscript-port.h"

/**
 * Aborts the program.
 */
void jerry_port_fatal (jerry_fatal_code_t code)
{
  exit (code);
} /* jerry_port_fatal */

/**
 * Provide log message implementation for the engine.
 */
void
jerry_port_log (jerry_log_level_t level, /**< log level */
                const char *format, /**< format string */
                ...)  /**< parameters */
{
  if (level <= JERRY_LOG_LEVEL_ERROR)
  {
    va_list args;
    va_start (args, format);
    vprintf (format, args);
    va_end (args);
  }
} /* jerry_port_log */

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

/**
 * Dummy function to get the current time.
 *
 * @return 0
 */
double
jerry_port_get_current_time (void)
{
  return 0;
} /* jerry_port_get_current_time */

/**
 * Compiler built-in setjmp function.
 *
 * @return 0 when called the first time
 *         1 when returns from a longjmp call
 */
int
setjmp (jmp_buf buf)
{
  return __builtin_setjmp (buf);
} /* setjmp */

/**
 * Compiler built-in longjmp function.
 *
 * Note:
 *   ignores value argument
 */
void
longjmp (jmp_buf buf, int value)
{
  /* Must be called with 1. */
  (void)(value); // suppress unused param warning
  __builtin_longjmp (buf, 1);
} /* longjmp */
