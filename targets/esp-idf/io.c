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
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"

#include "jerryscript-port.h"
#include "jerryscript-debugger.h"

static const char TAG[] = "JS";

static esp_log_level_t crosslog(jerry_log_level_t level)
{
  switch(level)
  {
    case JERRY_LOG_LEVEL_ERROR: return ESP_LOG_ERROR;
    case JERRY_LOG_LEVEL_WARNING: return ESP_LOG_WARN;
    case JERRY_LOG_LEVEL_DEBUG: return ESP_LOG_DEBUG;
    case JERRY_LOG_LEVEL_TRACE: return ESP_LOG_VERBOSE;
  }

 return ESP_LOG_NONE;
}

/**
 * Actual log level
 */
static jerry_log_level_t jerry_port_default_log_level = JERRY_LOG_LEVEL_ERROR;

/**
 * Get the log level
 *
 * @return current log level
 */
jerry_log_level_t
jerry_port_default_get_log_level (void)
{
  return jerry_port_default_log_level;
} /* jerry_port_default_get_log_level */

/**
 * Set the log level
 */
void
jerry_port_default_set_log_level (jerry_log_level_t level) /**< log level */
{
  jerry_port_default_log_level = level;
} /* jerry_port_default_set_log_level */

/**
 * Default implementation of jerry_port_log. Prints log message to the standard
 * error with 'vfprintf' if message log level is less than or equal to the
 * current log level.
 *
 * If debugger support is enabled, printing happens first to an in-memory buffer,
 * which is then sent both to the standard error and to the debugger client.
 */
void
jerry_port_log (jerry_log_level_t level, /**< message log level */
                const char *format, /**< format string */
                ...)  /**< parameters */
{
  if (level <= jerry_port_default_log_level)
  {
    va_list args;
    va_start (args, format);
#if defined (JERRY_DEBUGGER) && (JERRY_DEBUGGER == 1)
    int length = vsnprintf (NULL, 0, format, args);
    va_end (args);
    va_start (args, format);

    JERRY_VLA (char, buffer, length + 1);
    vsnprintf (buffer, (size_t) length + 1, format, args);

    esp_log_write(crosslog(level), TAG, buffer);
    jerry_debugger_send_log (level, (jerry_char_t *) buffer, (jerry_size_t) length);
#else /* If jerry-debugger isn't defined, libc is turned on */
    esp_log_writev(crosslog(level), TAG, format, args);
#endif /* defined (JERRY_DEBUGGER) && (JERRY_DEBUGGER == 1) */
    va_end (args);
  }
} /* jerry_port_log */

#if defined (JERRY_DEBUGGER) && (JERRY_DEBUGGER == 1)

#define DEBUG_BUFFER_SIZE (256)
static char debug_buffer[DEBUG_BUFFER_SIZE];
static int debug_buffer_index = 0;

#endif /* defined (JERRY_DEBUGGER) && (JERRY_DEBUGGER == 1) */

/**
 * Default implementation of jerry_port_print_char. Uses 'putchar' to
 * print a single character to standard output.
 */
void
jerry_port_print_char (char c) /**< the character to print */
{
  putchar(c);

#if defined (JERRY_DEBUGGER) && (JERRY_DEBUGGER == 1)
  debug_buffer[debug_buffer_index++] = c;

  if ((debug_buffer_index == DEBUG_BUFFER_SIZE) || (c == '\n'))
  {
    jerry_debugger_send_output ((jerry_char_t *) debug_buffer, (jerry_size_t) debug_buffer_index);
    debug_buffer_index = 0;
  }
#endif /* defined (JERRY_DEBUGGER) && (JERRY_DEBUGGER == 1) */
} /* jerry_port_print_char */
