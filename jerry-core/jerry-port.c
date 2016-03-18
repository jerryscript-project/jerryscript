/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
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

#include "jerry-port.h"
#include <stdarg.h>
#ifdef JERRY_ENABLE_DATE_SYS_CALLS
#include <sys/time.h>
#endif /* JERRY_ENABLE_DATE_SYS_CALLS */

/**
 * Provide log message to filestream implementation for the engine.
 */
int jerry_port_logmsg (FILE *stream, /**< stream pointer */
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
int jerry_port_errormsg (const char *format, /**< format string */
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
 * Provide output character to console implementation for the engine.
 */
int jerry_port_putchar (int c) /**< character to put */
{
  return putchar ((unsigned char) c);
} /* jerry_port_putchar */

/**
 * Provide datetime implementation for the engine
 */
int jerry_port_get_time (double *out_time)
{
  (void) out_time;

#ifdef JERRY_ENABLE_DATE_SYS_CALLS
  struct timeval tv;

  if (gettimeofday (&tv, NULL) != 0)
  {
    return -1;
  }
  else
  {
    *out_time = ((double) tv.tv_sec) * 1000.0 + ((double) (tv.tv_usec / 1000.0));
    return 0;
  }
#endif /* JERRY_ENABLE_DATE_SYS_CALLS */

  return 0;
} /* jerry_port_get_time */
