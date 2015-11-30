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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jerry.h"
#include "jrt/jrt.h"
#include "main.h"

/**
 * Maximum command line arguments number
 */
#define JERRY_MAX_COMMAND_LINE_ARGS (64)

/**
 * Maximum size of source code buffer
 */
#define JERRY_SOURCE_BUFFER_SIZE (1048576)

/**
 * Standalone Jerry exit codes
 */
#define JERRY_STANDALONE_EXIT_CODE_OK   (0)
#define JERRY_STANDALONE_EXIT_CODE_FAIL (1)

static uint8_t source_buffer[ JERRY_SOURCE_BUFFER_SIZE ];

static const jerry_api_char_t *
read_sources (const char *script_file_names[],
              int files_count,
              size_t *out_source_size_p)
{
  int i;
  uint8_t *source_buffer_tail = source_buffer;

  for (i = 0; i < files_count; i++)
  {
    const char *script_file_name = script_file_names[i];

    FILE *file = fopen (script_file_name, "r");

    if (file == NULL)
    {
      break;
    }

    int fseek_status = fseek (file, 0, SEEK_END);

    if (fseek_status != 0)
    {
      break;
    }

    long script_len = ftell (file);

    if (script_len < 0)
    {
      fclose (file);
      break;
    }

    rewind (file);

    const size_t current_source_size = (size_t)script_len;

    if (source_buffer_tail + current_source_size >= source_buffer + sizeof (source_buffer))
    {
      fclose (file);
      break;
    }

    size_t bytes_read = fread (source_buffer_tail, 1, current_source_size, file);
    if (bytes_read < current_source_size)
    {
      fclose (file);
      break;
    }

    fclose (file);

    source_buffer_tail += current_source_size;
  }

  if (i < files_count)
  {
    JERRY_ERROR_MSG ("Failed to read script N%d\n", i + 1);

    return NULL;
  }
  else
  {
    const size_t source_size = (size_t) (source_buffer_tail - source_buffer);

    *out_source_size_p = source_size;

    return (const jerry_api_char_t *) source_buffer;
  }
}

/**
 * Provide the 'assert' implementation for the engine.
 *
 * @return true - if only one argument was passed and the argument is a boolean true.
 */
static bool
assert_handler (const jerry_api_object_t *function_obj_p __attr_unused___, /** < function object */
                const jerry_api_value_t *this_p __attr_unused___, /** < this arg */
                jerry_api_value_t *ret_val_p __attr_unused___, /** < return argument */
                const jerry_api_value_t args_p[], /** < function arguments */
                const jerry_api_length_t args_cnt) /** < number of function arguments */
{
  if (args_cnt == 1
      && args_p[0].type == JERRY_API_DATA_TYPE_BOOLEAN
      && args_p[0].v_bool == true)
  {
    return true;
  }
  else
  {
    JERRY_ERROR_MSG ("Script assertion failed\n");
    exit (JERRY_STANDALONE_EXIT_CODE_FAIL);
  }
} /* assert_handler */

int
main (int argc,
      char **argv)
{
  if (argc >= JERRY_MAX_COMMAND_LINE_ARGS)
  {
    JERRY_ERROR_MSG ("Too many command line arguments. Current maximum is %d (JERRY_MAX_COMMAND_LINE_ARGS)\n", argc);

    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  const char *file_names[JERRY_MAX_COMMAND_LINE_ARGS];
  int i;
  int files_counter = 0;

  size_t max_data_bss_size, max_stack_size;
  jerry_get_memory_limits (&max_data_bss_size, &max_stack_size);

  // FIXME:
  //  jrt_set_mem_limits (max_data_bss_size, max_stack_size);

  jerry_flag_t flags = JERRY_FLAG_EMPTY;

#ifdef JERRY_ENABLE_LOG
  const char *log_file_name = NULL;
#endif /* JERRY_ENABLE_LOG */
  for (i = 1; i < argc; i++)
  {
    if (!strcmp ("-v", argv[i]))
    {
      printf ("Build date: \t%s\n", jerry_build_date);
      printf ("Commit hash:\t%s\n", jerry_commit_hash);
      printf ("Branch name:\t%s\n", jerry_branch_name);
      printf ("\n");
    }
    else if (!strcmp ("--mem-stats", argv[i]))
    {
      flags |= JERRY_FLAG_MEM_STATS;
    }
    else if (!strcmp ("--mem-stats-per-opcode", argv[i]))
    {
      flags |= JERRY_FLAG_MEM_STATS_PER_OPCODE;
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
    else if (!strcmp ("--log-level", argv[i]))
    {
      flags |= JERRY_FLAG_ENABLE_LOG;
      if (++i < argc && strlen (argv[i]) == 1 && argv[i][0] >='0' && argv[i][0] <= '3')
      {
#ifdef JERRY_ENABLE_LOG
        jerry_debug_level = argv[i][0] - '0';
#endif /* JERRY_ENABLE_LOG */
      }
      else
      {
        JERRY_ERROR_MSG ("Error: wrong format or invalid argument\n");
        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }
    }
    else if (!strcmp ("--log-file", argv[i]))
    {
      flags |= JERRY_FLAG_ENABLE_LOG;
      if (++i < argc)
      {
#ifdef JERRY_ENABLE_LOG
        log_file_name = argv[i];
#endif /* JERRY_ENABLE_LOG */
      }
      else
      {
        JERRY_ERROR_MSG ("Error: wrong format of the arguments\n");
        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }
    }
    else if (!strcmp ("--abort-on-fail", argv[i]))
    {
      flags |= JERRY_FLAG_ABORT_ON_FAIL;
    }
    else
    {
      file_names[files_counter++] = argv[i];
    }
  }

  if (files_counter == 0)
  {
    return JERRY_STANDALONE_EXIT_CODE_OK;
  }
  else
  {
    size_t source_size;
    const jerry_api_char_t *source_p = read_sources (file_names, files_counter, &source_size);

    if (source_p == NULL)
    {
      return JERRY_STANDALONE_EXIT_CODE_FAIL;
    }
    else
    {
#ifdef JERRY_ENABLE_LOG
      if (log_file_name)
      {
        jerry_log_file = fopen (log_file_name, "w");
        if (jerry_log_file == NULL)
        {
          JERRY_ERROR_MSG ("Failed to open log file: %s\n", log_file_name);
          return JERRY_STANDALONE_EXIT_CODE_FAIL;
        }
      }
      else
      {
        jerry_log_file = stdout;
      }
#endif /* JERRY_ENABLE_LOG */

      jerry_init (flags);

      jerry_api_object_t *global_obj_p = jerry_api_get_global ();
      jerry_api_object_t *assert_func_p = jerry_api_create_external_function (assert_handler);
      jerry_api_value_t assert_value;
      assert_value.type = JERRY_API_DATA_TYPE_OBJECT;
      assert_value.v_object = assert_func_p;

      bool is_assert_added = jerry_api_set_object_field_value (global_obj_p,
                                                               (jerry_api_char_t *) "assert",
                                                               &assert_value);

      jerry_api_release_value (&assert_value);
      jerry_api_release_object (global_obj_p);

      if (!is_assert_added)
      {
        JERRY_ERROR_MSG ("Failed to register 'assert' method.");
      }

      jerry_completion_code_t ret_code = JERRY_COMPLETION_CODE_OK;

      if (!jerry_parse (source_p, source_size))
      {
        /* unhandled SyntaxError */
        ret_code = JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION;
      }
      else
      {
        if ((flags & JERRY_FLAG_PARSE_ONLY) == 0)
        {
          ret_code = jerry_run ();
        }
      }

      jerry_cleanup ();

#ifdef JERRY_ENABLE_LOG
      if (jerry_log_file && jerry_log_file != stdout)
      {
        fclose (jerry_log_file);
        jerry_log_file = NULL;
      }
#endif /* JERRY_ENABLE_LOG */

      if (ret_code == JERRY_COMPLETION_CODE_OK)
      {
        return JERRY_STANDALONE_EXIT_CODE_OK;
      }
      else
      {
        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }
    }
  }
}
