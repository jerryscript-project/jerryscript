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

#include "jerryscript.h"
#include "jerry-port.h"
#include "jmem.h"

/**
 * Maximum command line arguments number.
 */
#define JERRY_MAX_COMMAND_LINE_ARGS (16)

/**
 * Standalone Jerry exit codes.
 */
#define JERRY_STANDALONE_EXIT_CODE_OK   (0)
#define JERRY_STANDALONE_EXIT_CODE_FAIL (1)

/**
 * Print usage and available options
 */
static void
print_help (char *name)
{
  jerry_port_console ("Usage: %s [OPTION]... [FILE]...\n"
                      "\n"
                      "Options:\n"
                      "  --log-level [0-3]\n"
                      "  --mem-stats\n"
                      "  --mem-stats-separate\n"
                      "  --show-opcodes\n"
                      "  --start-debug-server\n"
                      "\n",
                      name);
} /* print_help */

/**
 * Read source code into buffer.
 *
 * Returned value must be freed with jmem_heap_free_block if it's not NULL.
 * @return NULL, if read or allocation has failed
 *         pointer to the allocated memory block, otherwise
 */
static const uint8_t *
read_file (const char *file_name, /**< source code */
           size_t *out_size_p) /**< [out] number of bytes successfully read from source */
{
  FILE *file = fopen (file_name, "r");
  if (file == NULL)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: cannot open file: %s\n", file_name);
    return NULL;
  }

  int fseek_status = fseek (file, 0, SEEK_END);
  if (fseek_status != 0)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Failed to seek (error: %d)\n", fseek_status);
    fclose (file);
    return NULL;
  }

  long script_len = ftell (file);
  if (script_len < 0)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Failed to get the file size(error %ld)\n", script_len);
    fclose (file);
    return NULL;
  }

  rewind (file);

  uint8_t *buffer = jmem_heap_alloc_block_null_on_error (script_len);

  if (buffer == NULL)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Out of memory error\n");
    fclose (file);
    return NULL;
  }

  size_t bytes_read = fread (buffer, 1u, script_len, file);

  if (!bytes_read || bytes_read != script_len)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: failed to read file: %s\n", file_name);
    jmem_heap_free_block ((void*) buffer, script_len);

    fclose (file);
    return NULL;
  }

  fclose (file);

  *out_size_p = bytes_read;
  return (const uint8_t *) buffer;
} /* read_file */

/**
 * JerryScript log level
 */
static jerry_log_level_t jerry_log_level = JERRY_LOG_LEVEL_ERROR;

/**
 * Main program.
 *
 * @return 0 if success, error code otherwise
 */
#ifdef CONFIG_BUILD_KERNEL
int main (int argc, FAR char *argv[])
#else
int jerry_main (int argc, char *argv[])
#endif
{
  if (argc > JERRY_MAX_COMMAND_LINE_ARGS)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR,
                    "Too many command line arguments. Current maximum is %d\n",
                    JERRY_MAX_COMMAND_LINE_ARGS);

    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  const char *file_names[JERRY_MAX_COMMAND_LINE_ARGS];
  int i;
  int files_counter = 0;

  jerry_init_flag_t flags = JERRY_INIT_EMPTY;

  for (i = 1; i < argc; i++)
  {
    if (!strcmp ("-h", argv[i]) || !strcmp ("--help", argv[i]))
    {
      print_help (argv[0]);
      return JERRY_STANDALONE_EXIT_CODE_OK;
    }
    else if (!strcmp ("--mem-stats", argv[i]))
    {
      flags |= JERRY_INIT_MEM_STATS;
    }
    else if (!strcmp ("--mem-stats-separate", argv[i]))
    {
      flags |= JERRY_INIT_MEM_STATS_SEPARATE;
    }
    else if (!strcmp ("--show-opcodes", argv[i]))
    {
      flags |= JERRY_INIT_SHOW_OPCODES | JERRY_INIT_SHOW_REGEXP_OPCODES;
    }
    else if (!strcmp ("--log-level", argv[i]))
    {
      if (++i < argc && strlen (argv[i]) == 1 && argv[i][0] >='0' && argv[i][0] <= '3')
      {
        jerry_log_level = argv[i][0] - '0';
      }
      else
      {
        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: wrong format or invalid argument\n");
        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }
    }
    else if (!strcmp ("--start-debug-server", argv[i]))
    {
      flags |= JERRY_INIT_DEBUGGER;
    }
    else
    {
      file_names[files_counter++] = argv[i];
    }
  }

  if (files_counter == 0)
  {
    jerry_port_console ("No input files, running a hello world demo:\n");
    char *source_p = "var a = 3.5; print('Hello world ' + (a + 1.5) + ' times from JerryScript')";

    jerry_run_simple ((jerry_char_t *) source_p, strlen (source_p), flags);
    return JERRY_STANDALONE_EXIT_CODE_OK;
  }

  jerry_init (flags);
  jerry_value_t ret_value = jerry_create_undefined ();

  for (i = 0; i < files_counter; i++)
  {
    size_t source_size;
    const jerry_char_t *source_p = read_file (file_names[i], &source_size);

    if (source_p == NULL)
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Source file load error\n");
      return JERRY_STANDALONE_EXIT_CODE_FAIL;
    }

    ret_value = jerry_parse_named_resource ((jerry_char_t *) file_names[i],
                                            strlen (file_names[i]),
                                            source_p,
                                            source_size,
                                            false);

    jmem_heap_free_block ((void*) source_p, source_size);

    if (!jerry_value_has_error_flag (ret_value))
    {
      jerry_value_t func_val = ret_value;
      ret_value = jerry_run (func_val);
      jerry_release_value (func_val);
    }

    if (jerry_value_has_error_flag (ret_value))
    {
      break;
    }

    jerry_release_value (ret_value);
    ret_value = jerry_create_undefined ();
  }

  int ret_code = JERRY_STANDALONE_EXIT_CODE_OK;

  if (jerry_value_has_error_flag (ret_value))
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Unhandled exception: Script Error!\n");
    ret_code = JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  jerry_release_value (ret_value);
  jerry_cleanup ();

  return ret_code;
} /* main */

/**
 * Aborts the program.
 */
void jerry_port_fatal (jerry_fatal_code_t code)
{
  exit (1);
} /* jerry_port_fatal */

/**
 * Provide console message implementation for the engine.
 */
void
jerry_port_console (const char *format, /**< format string */
                    ...) /**< parameters */
{
  va_list args;
  va_start (args, format);
  vfprintf (stdout, format, args);
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
  if (level <= jerry_log_level)
  {
    va_list args;
    va_start (args, format);
    vfprintf (stderr, format, args);
    va_end (args);
  }
} /* jerry_port_log */

/**
 * Dummy function to get the time zone.
 *
 * @return false
 */
bool
jerry_port_get_time_zone (jerry_time_zone_t *tz_p)
{
  tz_p->offset = 0;
  tz_p->daylight_saving_time = 0;

  return false;
} /* jerry_port_get_time_zone */

/**
 * Dummy function to get the current time.
 *
 * @return 0
 */
double
jerry_port_get_current_time ()
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
  __builtin_longjmp (buf, 1);
} /* longjmp */
