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

#include "jerryscript-debugger.h"
#include "jerryscript-port-default.h"
#include "jerryscript-port.h"

#ifdef _WIN32
#include <windows.h>
#endif /* _WIN32 */

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
                ...) /**< parameters */
{
  if (level <= jerry_port_default_log_level)
  {
    va_list args;
    va_start (args, format);
#if defined(JERRY_DEBUGGER) && (JERRY_DEBUGGER == 1)
    int length = vsnprintf (NULL, 0, format, args);
    va_end (args);
    va_start (args, format);

    JERRY_VLA (char, buffer, length + 1);
    vsnprintf (buffer, (size_t) length + 1, format, args);

    fprintf (stderr, "%s", buffer);
    jerry_debugger_send_log (level, (jerry_char_t *) buffer, (jerry_size_t) length);
#else /* If jerry-debugger isn't defined, libc is turned on */
    vfprintf (stderr, format, args);
#endif /* defined (JERRY_DEBUGGER) && (JERRY_DEBUGGER == 1) */
    va_end (args);
  }
} /* jerry_port_log */

/**
 * Default implementation of jerry_port_string_print.
 * On win32 use WriteConsoleW and WriteFile to print UTF-8 string to standard output.
 * On non-win32 os use fwrite to print UTF-8 string to standard output.
 *
 * @param s The zero-terminated UTF-8 string to print
 * @param len The length of the UTF-8 string.
 */
void
jerry_port_string_print (const char *s, size_t len)
{
#ifdef _WIN32
  HANDLE hOut = GetStdHandle (STD_OUTPUT_HANDLE);
  if (hOut != INVALID_HANDLE_VALUE)
  {
    int hType = GetFileType (hOut);
    if (FILE_TYPE_CHAR == hType)
    {
      DWORD charsWritten = -1;
      JERRY_VLA (wchar_t, ws, (len + 1));
      int utf16_count = MultiByteToWideChar (CP_UTF8, 0, s, (int) len, ws, (int) (len + 1));
      ws[utf16_count] = '\0';
      WriteConsoleW (hOut, ws, utf16_count, &charsWritten, 0);
    }
    else
    {
      WriteFile (hOut, s, len, NULL, NULL);
    }
  }
  else
  {
    fwrite (s, 1, len, stdout);
  }
#else /* !_WIN32 */
  fwrite (s, 1, len, stdout);
#endif /* _WIN32 */

#if defined(JERRY_DEBUGGER) && (JERRY_DEBUGGER == 1)
  jerry_debugger_send_output ((const jerry_char_t *) s, (jerry_size_t) len);
#endif /* defined (JERRY_DEBUGGER) && (JERRY_DEBUGGER == 1) */
} /* jerry_port_string_print */
