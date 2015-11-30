/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#include "mbed-drivers/mbed.h"

#include "jerry-core/jerry.h"


/**
 * Provide log message to filestream implementation for the engine.
 */
int jerry_port_logmsg (FILE* stream, const char* format, ...)
{
  va_list args;
  int count;
  va_start (args, format);
  count = vfprintf (stream, format, args);
  va_end (args);
  return count;
}

/**
 * Provide error message to console implementation for the engine.
 */
int jerry_port_errormsg (const char* format, ...)
{
  va_list args;
  int count;
  va_start (args, format);
  count = vfprintf (stderr, format, args);
  va_end (args);
  return count;
}

/**
 * Provide output character to console implementation for the engine.
 */
int jerry_port_putchar (int c)
{
  return putchar(c);
}
