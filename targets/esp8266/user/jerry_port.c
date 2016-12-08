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

#include "esp_common.h"
#include <stdio.h>
#include <stdarg.h>

#include "jerry-core/jerry-port.h"
extern int ets_putc (int);

/**
 * Provide console message implementation for the engine.
 */
void
jerry_port_console (const char *format, /**< format string */
                    ...) /**< parameters */
{
  va_list args;
  va_start (args, format);
  ets_vprintf (ets_putc, format, args);
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
  /* TODO, uncomment when vprint link is ok */
  /* vprintf (stderr, format, args); */
  va_end (args);
} /* jerry_port_log */


/** exit - cause normal process termination  */
void exit (int status)
{
  while (true)
  {
  }
} /* exit */

/** abort - cause abnormal process termination  */
void abort (void)
{
  while (true)
  {
  }
} /* abort */

/**
 * fwrite
 *
 * @return number of bytes written
 */
size_t
fwrite (const void *ptr, /**< data to write */
        size_t size, /**< size of elements to write */
        size_t nmemb, /**< number of elements */
        FILE *stream) /**< stream pointer */
{
  return size * nmemb;
} /* fwrite */

/**
 * This function can get the time as well as a timezone.
 *
 * @return 0 if success, -1 otherwise
 */
int
gettimeofday (void *tp,  /**< struct timeval */
              void *tzp) /**< struct timezone */
{
  return -1;
} /* gettimeofday */
