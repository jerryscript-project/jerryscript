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

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"

#include "jerryscript.h"
#include "jerryscript-port.h"

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

/**
 * Default implementation of jerry_port_fatal. Calls 'abort' if exit code is
 * non-zero, 'exit' otherwise.
 */
void jerry_port_fatal (jerry_fatal_code_t code) /**< cause of error */
{
  ESP_LOGE(TAG, "Fatal error %d", code);
  vTaskSuspend(NULL);
  abort();
} /* jerry_port_fatal */

/**
 * Pointer to the current context.
 * Note that it is a global variable, and is not a thread safe implementation.
 * But I don't see how jerryscript can make that thread-safe, only the appication can
 */
static jerry_context_t *current_context_p = NULL;

/**
 * Set the current_context_p as the passed pointer.
 */
void
jerry_port_default_set_current_context (jerry_context_t *context_p) /**< points to the created context */
{
  current_context_p = context_p;
} /* jerry_port_default_set_current_context */

/**
 * Get the current context.
 *
 * @return the pointer to the current context
 */
jerry_context_t *
jerry_port_get_current_context (void)
{
  return current_context_p;
} /* jerry_port_get_current_context */

/**
 * Default implementation of jerry_port_sleep. Uses 'nanosleep' or 'usleep' if
 * available on the system, does nothing otherwise.
 */
void jerry_port_sleep (uint32_t sleep_time) /**< milliseconds to sleep */
{
  vTaskDelay( sleep_time / portTICK_PERIOD_MS);
} /* jerry_port_sleep */

/**
 * Opens file with the given path and reads its source.
 * @return the source of the file
 */
uint8_t *
jerry_port_read_source (const char *file_name_p, /**< file name */
                        size_t *out_size_p) /**< [out] read bytes */
{
  FILE *file_p = fopen (file_name_p, "rb");

  if (file_p == NULL)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: Failed to open file: %s\n", file_name_p);
    return NULL;
  }

  struct stat info = { };
  fstat(fileno(file_p), &info);	
  uint8_t *buffer_p = (uint8_t *) malloc (info.st_size);

  if (buffer_p == NULL)
  {
    fclose (file_p);

    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: Failed to allocate memory for file: %s\n", file_name_p);
    return NULL;
  }

  size_t bytes_read = fread (buffer_p, 1u, info.st_size, file_p);
  if (bytes_read != info.st_size)
  {
    fclose (file_p);
    free (buffer_p);

    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: Failed to read file: %s\n", file_name_p);
    return NULL;
  }

  fclose (file_p);
  *out_size_p = bytes_read;

  return buffer_p;
} /* jerry_port_read_source */

/**
 * Release the previously opened file's content.
 */
void
jerry_port_release_source (uint8_t *buffer_p) /**< buffer to free */
{
  free (buffer_p);
} /* jerry_port_release_source */

/**
 * Default implementation of jerry_port_get_local_time_zone_adjustment. Uses the 'tm_gmtoff' field
 * of 'struct tm' (a GNU extension) filled by 'localtime_r' if available on the
 * system, does nothing otherwise.
 *
 * @return offset between UTC and local time at the given unix timestamp, if
 *         available. Otherwise, returns 0, assuming UTC time.
 */
double jerry_port_get_local_time_zone_adjustment (double unix_ms,  /**< ms since unix epoch */
                                                  bool is_utc)  /**< is the time above in UTC? */
{
  struct tm tm;
  char buf[8];
  time_t now = (time_t) (unix_ms / 1000);

  localtime_r (&now, &tm);

  if (!is_utc)
  {
    strftime(buf, 8, "%z", &tm);
    now -= -atof(buf) * 3600 * 1000 / 100;
    localtime_r (&now, &tm);
  }

  strftime(buf, 8, "%z", &tm);

  return -atof(buf) * 3600 * 1000 / 100;
} /* jerry_port_get_local_time_zone_adjustment */

/**
 * Default implementation of jerry_port_get_current_time. Uses 'gettimeofday' if
 * available on the system, does nothing otherwise.
 *
 * @return milliseconds since Unix epoch - if 'gettimeofday' is available and
 *                                         executed successfully,
 *         0 - otherwise.
 */
double jerry_port_get_current_time (void)
{
  struct timeval tv;

  if (gettimeofday (&tv, NULL) == 0)
  {
    return ((double) tv.tv_sec) * 1000.0 + ((double) tv.tv_usec) / 1000.0;
  }
  return 0.0;
} /* jerry_port_get_current_time */

