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

#include "esp_common.h"
#include <stdio.h>
#include <stdarg.h>


/**
 * Provide log message to filestream implementation for the engine.
 */
int jerry_port_logmsg (FILE* stream, const char* format, ...)
{
  va_list args;
  int count = 0;
  va_start (args, format);
  // TODO, uncomment when vfprint link is ok
  //count = vfprintf (stream, format, args);
  va_end (args);
  return count;
}

/**
 * Provide error message to console implementation for the engine.
 */
int jerry_port_errormsg (const char* format, ...)
{
  va_list args;
  int count = 0;
  va_start (args, format);
  // TODO, uncomment when vprint link is ok
  //count = vprintf (format, args);
  va_end (args);
  return count;
}


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
