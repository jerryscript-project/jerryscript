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

#include "jerry.h"
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
    jerry_port_errormsg ("Error: failed to open file: %s\n", file_name);
    return NULL;
  }

  size_t bytes_read = fread (buffer, 1u, sizeof (buffer), file);
  if (!bytes_read)
  {
    jerry_port_errormsg ("Error: failed to read file: %s\n", file_name);
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
static bool
assert_handler (const jerry_object_t *function_obj_p __attribute__((unused)), /**< function object */
                const jerry_value_t *this_p __attribute__((unused)), /**< this arg */
                jerry_value_t *ret_val_p __attribute__((unused)), /**< return argument */
                const jerry_value_t args_p[], /**< function arguments */
                const jerry_length_t args_cnt) /**< number of function arguments */
{
  if (args_cnt == 1
      && args_p[0].type == JERRY_DATA_TYPE_BOOLEAN
      && args_p[0].u.v_bool == true)
  {
    return true;
  }
  else
  {
    jerry_port_errormsg ("Script error: assertion failed\n");
    exit (JERRY_STANDALONE_EXIT_CODE_FAIL);
  }
} /* assert_handler */

static void
print_usage (char *name)
{
  printf ("Usage: %s [OPTION]... [FILE]...\n"
          "Try '%s --help' for more information.\n",
          name, name);
} /* print_usage */

static void
print_help (char *name)
{
  printf ("Usage: %s [OPTION]... [FILE]...\n"
          "\n"
          "Options:\n"
          "  -h, --help\n"
          "  -v, --version\n"
          "  --mem-stats\n"
          "  --mem-stats-separate\n"
          "  --parse-only\n"
          "  --show-opcodes\n"
          "  --save-snapshot-for-global FILE\n"
          "  --save-snapshot-for-eval FILE\n"
          "  --exec-snapshot FILE\n"
          "  --log-level [0-3]\n"
          "  --log-file FILE\n"
          "  --abort-on-fail\n"
          "\n",
          name);
} /* print_help */

int
main (int argc,
      char **argv)
{
  if (argc > JERRY_MAX_COMMAND_LINE_ARGS)
  {
    jerry_port_errormsg ("Error: too many command line arguments: %d (JERRY_MAX_COMMAND_LINE_ARGS=%d)\n",
                         argc, JERRY_MAX_COMMAND_LINE_ARGS);

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

  const char *exec_snapshot_file_names[JERRY_MAX_COMMAND_LINE_ARGS];
  int exec_snapshots_count = 0;

  bool is_save_snapshot_mode = false;
  bool is_save_snapshot_mode_for_global_or_eval = false;
  const char *save_snapshot_file_name_p = NULL;

  bool is_repl_mode = false;

#ifdef JERRY_ENABLE_LOG
  const char *log_file_name = NULL;
#endif /* JERRY_ENABLE_LOG */
  for (i = 1; i < argc; i++)
  {
    if (!strcmp ("-h", argv[i]) || !strcmp ("--help", argv[i]))
    {
      print_help (argv[0]);
      return JERRY_STANDALONE_EXIT_CODE_OK;
    }
    else if (!strcmp ("-v", argv[i]) || !strcmp ("--version", argv[i]))
    {
      printf ("Build date: \t%s\n", jerry_build_date);
      printf ("Commit hash:\t%s\n", jerry_commit_hash);
      printf ("Branch name:\t%s\n", jerry_branch_name);
      printf ("\n");
      return JERRY_STANDALONE_EXIT_CODE_OK;
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
    else if (!strcmp ("--save-snapshot-for-global", argv[i])
             || !strcmp ("--save-snapshot-for-eval", argv[i]))
    {
      is_save_snapshot_mode = true;
      is_save_snapshot_mode_for_global_or_eval = !strcmp ("--save-snapshot-for-global", argv[i]);

      if (save_snapshot_file_name_p != NULL)
      {
        jerry_port_errormsg ("Error: snapshot file name already specified\n");
        print_usage (argv[0]);
        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }

      if (++i >= argc)
      {
        jerry_port_errormsg ("Error: no file specified for %s\n", argv[i - 1]);
        print_usage (argv[0]);
        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }

      flags |= JERRY_FLAG_PARSE_ONLY;
      save_snapshot_file_name_p = argv[i];
    }
    else if (!strcmp ("--exec-snapshot", argv[i]))
    {
      if (++i >= argc)
      {
        jerry_port_errormsg ("Error: no file specified for %s\n", argv[i - 1]);
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
        jerry_port_errormsg ("Error: no level specified for %s\n", argv[i - 1]);
        print_usage (argv[0]);
        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }

      if (strlen (argv[i]) != 1 || argv[i][0] < '0' || argv[i][0] > '3')
      {
        jerry_port_errormsg ("Error: wrong format for %s\n", argv[i - 1]);
        print_usage (argv[0]);
        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }

#ifdef JERRY_ENABLE_LOG
      flags |= JERRY_FLAG_ENABLE_LOG;
      jerry_debug_level = argv[i][0] - '0';
#endif /* JERRY_ENABLE_LOG */
    }
    else if (!strcmp ("--log-file", argv[i]))
    {
      if (++i >= argc)
      {
        jerry_port_errormsg ("Error: no file specified for %s\n", argv[i - 1]);
        print_usage (argv[0]);
        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }

#ifdef JERRY_ENABLE_LOG
      flags |= JERRY_FLAG_ENABLE_LOG;
      log_file_name = argv[i];
#endif /* JERRY_ENABLE_LOG */
    }
    else if (!strcmp ("--abort-on-fail", argv[i]))
    {
      jerry_port_default_set_abort_on_fail (true);
    }
    else if (!strncmp ("-", argv[i], 1))
    {
      jerry_port_errormsg ("Error: unrecognized option: %s\n", argv[i]);
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
      jerry_port_errormsg ("Error: --save-snapshot argument works with exactly one script\n");
      return JERRY_STANDALONE_EXIT_CODE_FAIL;
    }

    if (exec_snapshots_count != 0)
    {
      jerry_port_errormsg ("Error: --save-snapshot and --exec-snapshot options can't be passed simultaneously\n");
      return JERRY_STANDALONE_EXIT_CODE_FAIL;
    }
  }

  if (files_counter == 0
      && exec_snapshots_count == 0)
  {
    is_repl_mode = true;
  }

#ifdef JERRY_ENABLE_LOG
  if (log_file_name)
  {
    jerry_log_file = fopen (log_file_name, "w");
    if (jerry_log_file == NULL)
    {
      jerry_port_errormsg ("Error: failed to open log file: %s\n", log_file_name);
      return JERRY_STANDALONE_EXIT_CODE_FAIL;
    }
  }
  else
  {
    jerry_log_file = stdout;
  }
#endif /* JERRY_ENABLE_LOG */

  jerry_init (flags);

  jerry_object_t *global_obj_p = jerry_get_global ();
  jerry_object_t *assert_func_p = jerry_create_external_function (assert_handler);
  jerry_value_t assert_value;
  assert_value.type = JERRY_DATA_TYPE_OBJECT;
  assert_value.u.v_object = assert_func_p;

  bool is_assert_added = jerry_set_object_field_value (global_obj_p,
                                                       (jerry_char_t *) "assert",
                                                       &assert_value);

  jerry_release_value (&assert_value);
  jerry_release_object (global_obj_p);

  if (!is_assert_added)
  {
    jerry_port_errormsg ("Warning: failed to register 'assert' method.");
  }

  jerry_completion_code_t ret_code = JERRY_COMPLETION_CODE_OK;

  for (int i = 0; i < exec_snapshots_count; i++)
  {
    size_t snapshot_size;
    const uint8_t *snapshot_p = read_file (exec_snapshot_file_names[i], &snapshot_size);

    if (snapshot_p == NULL)
    {
      ret_code = JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION;
    }
    else
    {
      jerry_value_t ret_value;
      ret_code = jerry_exec_snapshot ((void *) snapshot_p,
                                      snapshot_size,
                                      true,
                                      &ret_value);
      assert (ret_value.type == JERRY_DATA_TYPE_UNDEFINED);
    }

    if (ret_code != JERRY_COMPLETION_CODE_OK)
    {
      break;
    }
  }

  jerry_object_t *err_obj_p = NULL;
  jerry_value_t err_value = jerry_create_void_value ();

  if (ret_code == JERRY_COMPLETION_CODE_OK)
  {
    for (int i = 0; i < files_counter; i++)
    {
      size_t source_size;
      const jerry_char_t *source_p = read_file (file_names[i], &source_size);

      if (source_p == NULL)
      {
        ret_code = JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION;
      }

      if (is_save_snapshot_mode)
      {
        static uint8_t snapshot_save_buffer[ JERRY_BUFFER_SIZE ];

        size_t snapshot_size = jerry_parse_and_save_snapshot ((jerry_char_t *) source_p,
                                                              source_size,
                                                              is_save_snapshot_mode_for_global_or_eval,
                                                              snapshot_save_buffer,
                                                              JERRY_BUFFER_SIZE);
        if (snapshot_size == 0)
        {
          ret_code = JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION;
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
        if (!jerry_parse (source_p, source_size, &err_obj_p))
        {
          /* unhandled SyntaxError */
          ret_code = JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION;
        }
        else if ((flags & JERRY_FLAG_PARSE_ONLY) == 0)
        {
          ret_code = jerry_run (&err_value);
        }
      }

      if (ret_code != JERRY_COMPLETION_CODE_OK)
      {
        break;
      }
    }
  }

  if (is_repl_mode)
  {
    const char *prompt = "jerry> ";
    bool is_done = false;

    jerry_object_t *global_obj_p = jerry_get_global ();
    jerry_value_t print_function;

    if (!jerry_get_object_field_value (global_obj_p, (jerry_char_t *) "print", &print_function))
    {
      return JERRY_STANDALONE_EXIT_CODE_FAIL;
    }

    if (!jerry_is_function (print_function.u.v_object))
    {
      return JERRY_STANDALONE_EXIT_CODE_FAIL;
    }

    while (!is_done)
    {
      uint8_t *source_buffer_tail = buffer;
      size_t len = 0;

      printf ("%s", prompt);

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
        jerry_value_t ret_val;
        ret_code = jerry_eval (buffer, len, false, false, &ret_val);

        /* Print return value */
        const jerry_value_t args[] = { ret_val };
        jerry_value_t ret_val_print;
        if (jerry_call_function (print_function.u.v_object, NULL, &ret_val_print, args, 1))
        {
          jerry_release_value (&ret_val_print);
        }

        jerry_release_value (&ret_val);
      }
    }

    jerry_release_object (global_obj_p);
    jerry_release_value (&print_function);
  }

#ifdef JERRY_ENABLE_LOG
  if (jerry_log_file && jerry_log_file != stdout)
  {
    fclose (jerry_log_file);
    jerry_log_file = NULL;
  }
#endif /* JERRY_ENABLE_LOG */

  if (ret_code == JERRY_COMPLETION_CODE_OK)
  {
    jerry_cleanup ();
    return JERRY_STANDALONE_EXIT_CODE_OK;
  }
  else if (ret_code == JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION)
  {
    jerry_string_t *err_str_p = NULL;

    if (err_obj_p != NULL)
    {
      jerry_value_t err_value = jerry_create_object_value (err_obj_p);
      err_str_p = jerry_value_to_string (&err_value);
      jerry_release_object (err_obj_p);
    }
    else if (!jerry_value_is_void (&err_value))
    {
      err_str_p = jerry_value_to_string (&err_value);
      jerry_release_value (&err_value);
    }

    if (__builtin_expect (!!(err_str_p != NULL), 1))
    {
      jerry_char_t err_str_buf[256];

      jerry_size_t err_str_size = jerry_get_string_size (err_str_p);
      assert (err_str_size < 256);
      jerry_size_t sz = jerry_string_to_char_buffer (err_str_p, err_str_buf, err_str_size);
      assert (sz == err_str_size);
      err_str_buf[err_str_size] = 0;

      jerry_port_errormsg ("Script Error: unhandled exception: %s\n", err_str_buf);

      jerry_release_string (err_str_p);
    }

    jerry_cleanup ();
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  assert (ret_code == JERRY_COMPLETION_CODE_INVALID_SNAPSHOT_FORMAT
          || ret_code == JERRY_COMPLETION_CODE_INVALID_SNAPSHOT_VERSION);
  jerry_cleanup ();
  return JERRY_STANDALONE_EXIT_CODE_FAIL;
} /* main */
