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

#include "jerryscript-port.h"
#include "jerryscript-port-default.h"
#include "debugger.h"

#ifndef DISABLE_EXTRA_API

/**
 * Actual log level
 */
static jerry_log_level_t jerry_port_default_log_level = JERRY_LOG_LEVEL_ERROR;

#define JERRY_PORT_DEFAULT_LOG_LEVEL jerry_port_default_log_level

/**
 * Get the log level
 *
 * @return current log level
 *
 * Note:
 *      This function is only available if the port implementation library is
 *      compiled without the DISABLE_EXTRA_API macro.
 */
jerry_log_level_t
jerry_port_default_get_log_level (void)
{
  return jerry_port_default_log_level;
} /* jerry_port_default_get_log_level */

/**
 * Set the log level
 *
 * Note:
 *      This function is only available if the port implementation library is
 *      compiled without the DISABLE_EXTRA_API macro.
 */
void
jerry_port_default_set_log_level (jerry_log_level_t level) /**< log level */
{
  jerry_port_default_log_level = level;
} /* jerry_port_default_set_log_level */

#else /* DISABLE_EXTRA_API */
#define JERRY_PORT_DEFAULT_LOG_LEVEL JERRY_LOG_LEVEL_ERROR
#endif /* !DISABLE_EXTRA_API */

/**
 * Default implementation of jerry_port_log. Prints log message to a buffer
 * then prints the buffer to the standard error with 'fprintf' if message
 * level is less than or equal to the set log level.
 * Additionally, sends the message to the debugger client.
 *
 * Note:
 *      Changing the log level from JERRY_LOG_LEVEL_ERROR is only possible if
 *      the port implementation library is compiled without the
 *      DISABLE_EXTRA_API macro.
 */
void
jerry_port_log (jerry_log_level_t level, /**< log level */
                const char *format, /**< format string */
                ...)  /**< parameters */
{
  if (level <= JERRY_PORT_DEFAULT_LOG_LEVEL)
  {
    va_list args;
    va_start (args, format);
#ifdef JERRY_DEBUGGER
    char buffer[256];
    int length = 0;
    length = vsnprintf (buffer, 255, format, args);
    buffer[length] = '\0';
    fprintf (stderr, "%s", buffer);
    jerry_char_t *jbuffer = (jerry_char_t *) buffer;
    jerry_debugger_send_output (jbuffer, (jerry_size_t) length, (uint8_t) (level + 2));
#else /* If jerry-debugger isn't defined, libc is turned on */
    vfprintf (stderr, format, args);
#endif /* JERRY_DEBUGGER */
    va_end (args);
  }
} /* jerry_port_log */
