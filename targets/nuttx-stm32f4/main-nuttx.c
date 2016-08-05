/* Copyright 2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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

#include "jerry.h"
#include "jerry-port.h"
#include "jerry-port-default.h"

/**
 * The module interface routine
 */
extern "C" int jerryscript_entry (int argc, char *argv[]);

/**
 * Maximum command line arguments number
 */
#define JERRY_MAX_COMMAND_LINE_ARGS (16)

/**
 * Standalone Jerry exit codes
 */
#define JERRY_STANDALONE_EXIT_CODE_OK   (0)
#define JERRY_STANDALONE_EXIT_CODE_FAIL (1)

static char* read_sources (const char *script_file_names[],
                           int files_count,
                           size_t *out_source_size_p)
{
  int i;
  char* source_buffer = NULL;
  char *source_buffer_tail = NULL;
  size_t total_length = 0;
  FILE *file = NULL;

  for (i = 0; i < files_count; i++)
  {
    const char *script_file_name = script_file_names[i];

    file = fopen (script_file_name, "r");
    if (file == NULL)
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Failed to fopen [%s]\n", script_file_name);
      return NULL;
    }

    int fseek_status = fseek (file, 0, SEEK_END);
    if (fseek_status != 0)
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Failed to fseek fseek_status(%d)\n", fseek_status);
      fclose (file);
      return NULL;
    }

    long script_len = ftell (file);
    if (script_len < 0)
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Failed to ftell script_len(%ld)\n", script_len);
      fclose (file);
      break;
    }

    total_length += (size_t)script_len;

    fclose (file);
    file = NULL;
  }

  if (total_length <= 0)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Theres noting to read\n");
    return NULL;
  }

  source_buffer = (char*)malloc(total_length);
  if (source_buffer == NULL)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Out of memory error\n");
    return NULL;
  }
  memset(source_buffer, 0, sizeof(char)*total_length);
  source_buffer_tail = source_buffer;

  for (i = 0; i < files_count; i++)
  {
    const char *script_file_name = script_file_names[i];
    file = fopen (script_file_name, "r");

    if (file == NULL)
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Failed to fopen [%s]\n", script_file_name);
      break;
    }

    int fseek_status = fseek (file, 0, SEEK_END);
    if (fseek_status != 0)
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Failed to fseek fseek_status(%d)\n", fseek_status);
      break;
    }

    long script_len = ftell (file);
    if (script_len < 0)
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Failed to ftell script_len(%ld)\n", script_len);
      break;
    }

    rewind (file);

    const size_t current_source_size = (size_t)script_len;
    size_t bytes_read = fread (source_buffer_tail, 1, current_source_size, file);
    if (bytes_read < current_source_size)
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Failed to fread bytes_read(%d)\n", bytes_read);
      break;
    }

    fclose (file);
    file = NULL;

    source_buffer_tail += current_source_size;
  }

  if (file != NULL)
  {
    fclose (file);
  }

  if (i < files_count)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Failed to read script N%d\n", i + 1);
    free(source_buffer);
    return NULL;
  }

  *out_source_size_p = (size_t)total_length;

  return source_buffer;
}

int jerryscript_entry (int argc, char *argv[])
{
  if (argc >= JERRY_MAX_COMMAND_LINE_ARGS)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR,
                    "Too many command line arguments. Current maximum is %d\n",
                    JERRY_MAX_COMMAND_LINE_ARGS);

    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  const char *file_names[JERRY_MAX_COMMAND_LINE_ARGS];
  int i;
  int files_counter = 0;

  for (i = 1; i < argc; i++)
  {
    printf ("PARAM %d : [%s]\n", i, argv[i]);
  }

  jerry_flag_t flags = JERRY_FLAG_EMPTY;

  for (i = 1; i < argc; i++)
  {
    if (!strcmp ("-v", argv[i]))
    {
      printf ("Build date: \t%s\n", jerry_build_date);
      printf ("Commit hash:\t%s\n", jerry_commit_hash);
      printf ("Branch name:\t%s\n", jerry_branch_name);
    }
    else if (!strcmp ("--mem-stats", argv[i]))
    {
      flags |= JERRY_FLAG_MEM_STATS;
    }
    else if (!strcmp ("--mem-stats-separate", argv[i]))
    {
      flags |= JERRY_FLAG_MEM_STATS_SEPARATE;
    }
    else if (!strcmp ("--parse-only", argv[i]))
    {
      flags |= JERRY_FLAG_PARSE_ONLY;
    }
    else if (!strcmp ("--show-opcodes", argv[i]))
    {
      flags |= JERRY_FLAG_SHOW_OPCODES;
    }
    else if (!strcmp ("--abort-on-fail", argv[i]))
    {
      jerry_port_default_set_abort_on_fail (true);
    }
    else if (!strcmp ("--log-level", argv[i]))
    {
      flags |= JERRY_FLAG_ENABLE_LOG;
      if (++i < argc && strlen (argv[i]) == 1 && argv[i][0] >='0' && argv[i][0] <= '3')
      {
        jerry_debug_level = argv[i][0] - '0';
      }
      else
      {
        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: wrong format or invalid argument\n");
        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }
    }
    else
    {
      file_names[files_counter++] = argv[i];
    }
  }

  if (files_counter == 0)
  {
    printf ("Jerry: file count 0\n");
    return JERRY_STANDALONE_EXIT_CODE_OK;
  }

  size_t source_size;
  char *source_p = read_sources (file_names, files_counter, &source_size);

  if (source_p == NULL)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "JERRY_STANDALONE_EXIT_CODE_FAIL\n");
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  jerry_completion_code_t ret_code;

  ret_code = jerry_run_simple ((jerry_api_char_t *) source_p, source_size, flags);

  free(source_p);

  if (ret_code != JERRY_COMPLETION_CODE_OK)
  {
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }
  return JERRY_STANDALONE_EXIT_CODE_OK;
}

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
  (void) level; /* ignore the log level */

  va_list args;
  va_start (args, format);
  vfprintf (stderr, format, args);
  va_end (args);
} /* jerry_port_log */
