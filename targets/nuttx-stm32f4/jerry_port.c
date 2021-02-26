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
#include <unistd.h>

#include "jerryscript.h"
#include "jerryscript-port.h"

/**
 * JerryScript log level
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
  if (level <= jerry_log_level)
  {
    va_list args;
    va_start (args, format);
    vfprintf (stderr, format, args);
    va_end (args);
  }
} /* jerry_port_log */

/**
 * Determines the size of the given file.
 * @return size of the file
 */
static size_t
jerry_port_get_file_size (FILE *file_p) /**< opened file */
{
  fseek (file_p, 0, SEEK_END);
  long size = ftell (file_p);
  fseek (file_p, 0, SEEK_SET);

  return (size_t) size;
} /* jerry_port_get_file_size */

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
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: failed to open file: %s\n", file_name_p);
    return NULL;
  }

  size_t file_size = jerry_port_get_file_size (file_p);
  uint8_t *buffer_p = (uint8_t *) malloc (file_size);

  if (buffer_p == NULL)
  {
    fclose (file_p);

    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: failed to allocate memory for module");
    return NULL;
  }

  size_t bytes_read = fread (buffer_p, 1u, file_size, file_p);

  if (!bytes_read)
  {
    fclose (file_p);
    free (buffer_p);

    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: failed to read file: %s\n", file_name_p);
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
 * Normalize a file path
 *
 * @return length of the path written to the output buffer
 */
size_t
jerry_port_normalize_path (const char *in_path_p,   /**< input file path */
                           char *out_buf_p,         /**< output buffer */
                           size_t out_buf_size,     /**< size of output buffer */
                           char *base_file_p) /**< base file path */
{
  (void) base_file_p;

  size_t len = strlen (in_path_p);
  if (len + 1 > out_buf_size)
  {
    return 0;
  }

  /* Return the original string. */
  strcpy (out_buf_p, in_path_p);
  return len;
} /* jerry_port_normalize_path */

/**
 * Get the module object of a native module.
 *
 * @return undefined
 */
jerry_value_t
jerry_port_get_native_module (jerry_value_t name) /**< module specifier */
{
  (void) name;
  return jerry_create_undefined ();
} /* jerry_port_get_native_module */

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
 * Provide the implementation of jerry_port_print_char.
 * Uses 'printf' to print a single character to standard output.
 */
void
jerry_port_print_char (char c) /**< the character to print */
{
  printf ("%c", c);
} /* jerry_port_print_char */

/**
 * Provide implementation of jerry_port_sleep.
 */
void jerry_port_sleep (uint32_t sleep_time) /**< milliseconds to sleep */
{
  usleep ((useconds_t) sleep_time * 1000);
} /* jerry_port_sleep */

/**
 * Pointer to the current context.
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
