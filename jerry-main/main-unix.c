/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jerry-api.h"
#include "jerry-port.h"
#include "jerry-port-default.h"

/**
 * Maximum command line arguments number
 */
#define JERRY_MAX_COMMAND_LINE_ARGS (64)

/**
 * Maximum size of source code / snapshots buffer
 */
#define JERRY_BUFFER_SIZE (1048576)

/**
 * Standalone Jerry exit codes
 */
#define JERRY_STANDALONE_EXIT_CODE_OK   (0)
#define JERRY_STANDALONE_EXIT_CODE_FAIL (1)

static uint8_t buffer[ JERRY_BUFFER_SIZE ];

static const uint8_t *
read_file (const char *file_name,
           size_t *out_size_p)
{
  FILE *file = fopen (file_name, "r");
  if (file == NULL)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: failed to open file: %s\n", file_name);
    return NULL;
  }

  size_t bytes_read = fread (buffer, 1u, sizeof (buffer), file);
  if (!bytes_read)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: failed to read file: %s\n", file_name);
    fclose (file);
    return NULL;
  }

  fclose (file);

  *out_size_p = bytes_read;
  return (const uint8_t *) buffer;
} /* read_file */

/**
 * Provide the 'assert' implementation for the engine.
 *
 * @return true - if only one argument was passed and the argument is a boolean true.
 */
static jerry_value_t
assert_handler (const jerry_value_t func_obj_val __attribute__((unused)), /**< function object */
                const jerry_value_t this_p __attribute__((unused)), /**< this arg */
                const jerry_value_t args_p[], /**< function arguments */
                const jerry_length_t args_cnt) /**< number of function arguments */
{
  if (args_cnt == 1
      && jerry_value_is_boolean (args_p[0])
      && jerry_get_boolean_value (args_p[0]))
  {
    return jerry_create_boolean (true);
  }
  else
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Script Error: assertion failed\n");
    exit (JERRY_STANDALONE_EXIT_CODE_FAIL);
  }
} /* assert_handler */

static void
print_usage (char *name)
{
  jerry_port_console ("Usage: %s [OPTION]... [FILE]...\n"
                      "Try '%s --help' for more information.\n",
                      name,
                      name);
} /* print_usage */

static void
print_help (char *name)
{
  jerry_port_console ("Usage: %s [OPTION]... [FILE]...\n"
                      "\n"
                      "Options:\n"
                      "  -h, --help\n"
                      "  -v, --version\n"
                      "  --mem-stats\n"
                      "  --mem-stats-separate\n"
                      "  --parse-only\n"
                      "  --show-opcodes\n"
                      "  --show-regexp-opcodes\n"
                      "  --save-snapshot-for-global FILE\n"
                      "  --save-snapshot-for-eval FILE\n"
                      "  --exec-snapshot FILE\n"
                      "  --log-level [0-3]\n"
                      "  --abort-on-fail\n"
                      "\n",
                      name);
} /* print_help */

static void
print_unhandled_exception (jerry_value_t error_value)
{
  assert (jerry_value_has_error_flag (error_value));

  jerry_value_clear_error_flag (&error_value);
  jerry_value_t err_str_val = jerry_value_to_string (error_value);
  jerry_size_t err_str_size = jerry_get_string_size (err_str_val);
  jerry_char_t err_str_buf[256];

  if (err_str_size >= 256)
  {
    const char msg[] = "[Error message too long]";
    err_str_size = sizeof (msg) / sizeof (char) - 1;
    memcpy (err_str_buf, msg, err_str_size);
  }
  else
  {
    jerry_size_t sz = jerry_string_to_char_buffer (err_str_val, err_str_buf, err_str_size);
    assert (sz == err_str_size);
  }
  err_str_buf[err_str_size] = 0;

  jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Script Error: %s\n", err_str_buf);
  jerry_release_value (err_str_val);
} /* print_unhandled_exception */

int
main (int argc,
      char **argv)
{
  if (argc > JERRY_MAX_COMMAND_LINE_ARGS)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR,
                    "Error: too many command line arguments: %d (JERRY_MAX_COMMAND_LINE_ARGS=%d)\n",
                    argc,
                    JERRY_MAX_COMMAND_LINE_ARGS);

    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  const char *file_names[JERRY_MAX_COMMAND_LINE_ARGS];
  int i;
  int files_counter = 0;

  size_t max_data_bss_size, max_stack_size;
  jerry_get_memory_limits (&max_data_bss_size, &max_stack_size);

  // FIXME:
  //  jrt_set_mem_limits (max_data_bss_size, max_stack_size);

  jerry_init_flag_t flags = JERRY_INIT_EMPTY;

  const char *exec_snapshot_file_names[JERRY_MAX_COMMAND_LINE_ARGS];
  int exec_snapshots_count = 0;

  bool is_parse_only = false;
  bool is_save_snapshot_mode = false;
  bool is_save_snapshot_mode_for_global_or_eval = false;
  const char *save_snapshot_file_name_p = NULL;

  bool is_repl_mode = false;

  for (i = 1; i < argc; i++)
  {
    if (!strcmp ("-h", argv[i]) || !strcmp ("--help", argv[i]))
    {
      print_help (argv[0]);
      return JERRY_STANDALONE_EXIT_CODE_OK;
    }
    else if (!strcmp ("-v", argv[i]) || !strcmp ("--version", argv[i]))
    {
      jerry_port_console ("Version: %d.%d%s\n", JERRY_API_MAJOR_VERSION, JERRY_API_MINOR_VERSION, JERRY_COMMIT_HASH);
      return JERRY_STANDALONE_EXIT_CODE_OK;
    }
    else if (!strcmp ("--mem-stats", argv[i]))
    {
      flags |= JERRY_INIT_MEM_STATS;
      jerry_port_default_set_log_level (JERRY_LOG_LEVEL_DEBUG);
    }
    else if (!strcmp ("--mem-stats-separate", argv[i]))
    {
      flags |= JERRY_INIT_MEM_STATS_SEPARATE;
      jerry_port_default_set_log_level (JERRY_LOG_LEVEL_DEBUG);
    }
    else if (!strcmp ("--parse-only", argv[i]))
    {
      is_parse_only = true;
    }
    else if (!strcmp ("--show-opcodes", argv[i]))
    {
      flags |= JERRY_INIT_SHOW_OPCODES;
      jerry_port_default_set_log_level (JERRY_LOG_LEVEL_DEBUG);
    }
    else if (!strcmp ("--show-regexp-opcodes", argv[i]))
    {
      flags |= JERRY_INIT_SHOW_REGEXP_OPCODES;
      jerry_port_default_set_log_level (JERRY_LOG_LEVEL_DEBUG);
    }
    else if (!strcmp ("--save-snapshot-for-global", argv[i])
             || !strcmp ("--save-snapshot-for-eval", argv[i]))
    {
      is_save_snapshot_mode = true;
      is_save_snapshot_mode_for_global_or_eval = !strcmp ("--save-snapshot-for-global", argv[i]);

      if (save_snapshot_file_name_p != NULL)
      {
        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: snapshot file name already specified\n");
        print_usage (argv[0]);
        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }

      if (++i >= argc)
      {
        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: no file specified for %s\n", argv[i - 1]);
        print_usage (argv[0]);
        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }

      save_snapshot_file_name_p = argv[i];
    }
    else if (!strcmp ("--exec-snapshot", argv[i]))
    {
      if (++i >= argc)
      {
        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: no file specified for %s\n", argv[i - 1]);
        print_usage (argv[0]);
        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }

      assert (exec_snapshots_count < JERRY_MAX_COMMAND_LINE_ARGS);
      exec_snapshot_file_names[exec_snapshots_count++] = argv[i];
    }
    else if (!strcmp ("--log-level", argv[i]))
    {
      if (++i >= argc)
      {
        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: no level specified for %s\n", argv[i - 1]);
        print_usage (argv[0]);
        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }

      if (strlen (argv[i]) != 1 || argv[i][0] < '0' || argv[i][0] > '3')
      {
        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: wrong format for %s\n", argv[i - 1]);
        print_usage (argv[0]);
        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }

      jerry_port_default_set_log_level ((jerry_log_level_t) (argv[i][0] - '0'));
    }
    else if (!strcmp ("--abort-on-fail", argv[i]))
    {
      jerry_port_default_set_abort_on_fail (true);
    }
    else if (!strncmp ("-", argv[i], 1))
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: unrecognized option: %s\n", argv[i]);
      print_usage (argv[0]);
      return JERRY_STANDALONE_EXIT_CODE_FAIL;
    }
    else
    {
      file_names[files_counter++] = argv[i];
    }
  }

  if (is_save_snapshot_mode)
  {
    if (files_counter != 1)
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR,
                      "Error: --save-snapshot argument works with exactly one script\n");
      return JERRY_STANDALONE_EXIT_CODE_FAIL;
    }

    if (exec_snapshots_count != 0)
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR,
                      "Error: --save-snapshot and --exec-snapshot options can't be passed simultaneously\n");
      return JERRY_STANDALONE_EXIT_CODE_FAIL;
    }
  }

  if (files_counter == 0
      && exec_snapshots_count == 0)
  {
    is_repl_mode = true;
  }

  jerry_init (flags);

  jerry_value_t global_obj_val = jerry_get_global_object ();
  jerry_value_t assert_value = jerry_create_external_function (assert_handler);

  jerry_value_t assert_func_name_val = jerry_create_string ((jerry_char_t *) "assert");
  bool is_assert_added = jerry_set_property (global_obj_val, assert_func_name_val, assert_value);

  jerry_release_value (assert_func_name_val);
  jerry_release_value (assert_value);
  jerry_release_value (global_obj_val);

  if (!is_assert_added)
  {
    jerry_port_log (JERRY_LOG_LEVEL_WARNING, "Warning: failed to register 'assert' method.");
  }

  jerry_value_t ret_value = jerry_create_undefined ();

  for (int i = 0; i < exec_snapshots_count; i++)
  {
    size_t snapshot_size;
    const uint8_t *snapshot_p = read_file (exec_snapshot_file_names[i], &snapshot_size);

    if (snapshot_p == NULL)
    {
      ret_value = jerry_create_error (JERRY_ERROR_COMMON, (jerry_char_t *) "");
    }
    else
    {
      ret_value = jerry_exec_snapshot ((void *) snapshot_p,
                                       snapshot_size,
                                       true);
    }

    if (jerry_value_has_error_flag (ret_value))
    {
      break;
    }
  }

  if (!jerry_value_has_error_flag (ret_value))
  {
    for (int i = 0; i < files_counter; i++)
    {
      size_t source_size;
      const jerry_char_t *source_p = read_file (file_names[i], &source_size);

      if (source_p == NULL)
      {
        ret_value = jerry_create_error (JERRY_ERROR_COMMON, (jerry_char_t *) "");
        break;
      }

      if (is_save_snapshot_mode)
      {
        static uint8_t snapshot_save_buffer[ JERRY_BUFFER_SIZE ];

        size_t snapshot_size = jerry_parse_and_save_snapshot ((jerry_char_t *) source_p,
                                                              source_size,
                                                              is_save_snapshot_mode_for_global_or_eval,
                                                              false,
                                                              snapshot_save_buffer,
                                                              JERRY_BUFFER_SIZE);
        if (snapshot_size == 0)
        {
          ret_value = jerry_create_error (JERRY_ERROR_COMMON, (jerry_char_t *) "");
        }
        else
        {
          FILE *snapshot_file_p = fopen (save_snapshot_file_name_p, "w");
          fwrite (snapshot_save_buffer, sizeof (uint8_t), snapshot_size, snapshot_file_p);
          fclose (snapshot_file_p);
        }
      }
      else
      {
        ret_value = jerry_parse (source_p, source_size, false);

        if (!jerry_value_has_error_flag (ret_value) && !is_parse_only)
        {
          jerry_value_t func_val = ret_value;
          ret_value = jerry_run (func_val);
          jerry_release_value (func_val);
        }
      }

      if (jerry_value_has_error_flag (ret_value))
      {
        break;
      }
    }
  }

  if (is_repl_mode)
  {
    const char *prompt = "jerry> ";
    bool is_done = false;

    jerry_value_t global_obj_val = jerry_get_global_object ();
    jerry_value_t print_func_name_val = jerry_create_string ((jerry_char_t *) "print");
    jerry_value_t print_function = jerry_get_property (global_obj_val, print_func_name_val);

    jerry_release_value (print_func_name_val);

    if (jerry_value_has_error_flag (print_function))
    {
      return JERRY_STANDALONE_EXIT_CODE_FAIL;
    }

    if (!jerry_value_is_function (print_function))
    {
      return JERRY_STANDALONE_EXIT_CODE_FAIL;
    }

    while (!is_done)
    {
      uint8_t *source_buffer_tail = buffer;
      size_t len = 0;

      jerry_port_console ("%s", prompt);

      /* Read a line */
      while (true)
      {
        if (fread (source_buffer_tail, 1, 1, stdin) != 1 && len == 0)
        {
          is_done = true;
          break;
        }
        if (*source_buffer_tail == '\n')
        {
          break;
        }
        source_buffer_tail ++;
        len ++;
      }
      *source_buffer_tail = 0;

      if (len > 0)
      {
        /* Evaluate the line */
        jerry_value_t ret_val_eval = jerry_eval (buffer, len, false);

        if (!jerry_value_has_error_flag (ret_val_eval))
        {
          /* Print return value */
          const jerry_value_t args[] = { ret_val_eval };
          jerry_value_t ret_val_print = jerry_call_function (print_function,
                                                             jerry_create_undefined (),
                                                             args,
                                                             1);
          jerry_release_value (ret_val_print);
        }
        else
        {
          print_unhandled_exception (ret_val_eval);
        }

        jerry_release_value (ret_val_eval);
      }
    }

    jerry_release_value (global_obj_val);
    jerry_release_value (print_function);
  }

  int ret_code = JERRY_STANDALONE_EXIT_CODE_OK;

  if (jerry_value_has_error_flag (ret_value))
  {
    print_unhandled_exception (ret_value);

    ret_code = JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  jerry_release_value (ret_value);
  jerry_cleanup ();

  return ret_code;

} /* main */
