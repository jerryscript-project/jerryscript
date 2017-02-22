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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jerryscript.h"
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

/**
 * Context size of the SYNTAX_ERROR
 */
#define SYNTAX_ERROR_CONTEXT_SIZE 2

static uint8_t buffer[ JERRY_BUFFER_SIZE ];

static const uint8_t *
read_file (const char *file_name,
           size_t *out_size_p)
{
  FILE *file;
  if (!strcmp ("-", file_name))
  {
    file = stdin;
  }
  else
  {
    file = fopen (file_name, "r");
    if (file == NULL)
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: failed to open file: %s\n", file_name);
      return NULL;
    }
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
                      "  --start-debug-server\n"
                      "  --save-snapshot-for-global FILE\n"
                      "  --save-snapshot-for-eval FILE\n"
                      "  --save-literals-list-format FILE\n"
                      "  --save-literals-c-format FILE\n"
                      "  --exec-snapshot FILE\n"
                      "  --log-level [0-3]\n"
                      "  --abort-on-fail\n"
                      "  --no-prompt\n"
                      "\n",
                      name);
} /* print_help */

/**
 * Check whether an error is a SyntaxError or not
 *
 * @return true  - if param is SyntaxError
 *         false - otherwise
 */
static bool
jerry_value_is_syntax_error (jerry_value_t error_value) /**< error value */
{
  assert (jerry_is_feature_enabled (JERRY_FEATURE_ERROR_MESSAGES));

  if (!jerry_value_is_object (error_value))
  {
    return false;
  }

  jerry_value_t prop_name = jerry_create_string ((const jerry_char_t *)"name");
  jerry_value_t error_name = jerry_get_property (error_value, prop_name);
  jerry_release_value (prop_name);

  if (jerry_value_has_error_flag (error_name)
      || !jerry_value_is_string (error_name))
  {
    return false;
  }

  jerry_size_t err_str_size = jerry_get_string_size (error_name);
  jerry_char_t err_str_buf[err_str_size];

  jerry_size_t sz = jerry_string_to_char_buffer (error_name, err_str_buf, err_str_size);
  jerry_release_value (error_name);

  if (sz == 0)
  {
    return false;
  }

  if (!strcmp ((char *) err_str_buf, "SyntaxError"))
  {
    return true;
  }

  return false;
} /* jerry_value_is_syntax_error */

/**
 * Convert string into unsigned integer
 *
 * @return converted number
 */
static uint32_t
str_to_uint (const char *num_str_p) /**< string to convert */
{
  assert (jerry_is_feature_enabled (JERRY_FEATURE_ERROR_MESSAGES));

  uint32_t result = 0;

  while (*num_str_p != '\0')
  {
    assert (*num_str_p >= '0' && *num_str_p <= '9');

    result *= 10;
    result += (uint32_t) (*num_str_p - '0');
    num_str_p++;
  }

  return result;
} /* str_to_uint */

/**
 * Print error value
 */
static void
print_unhandled_exception (jerry_value_t error_value) /**< error value */
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
    err_str_buf[err_str_size] = 0;

    if (jerry_is_feature_enabled (JERRY_FEATURE_ERROR_MESSAGES) && jerry_value_is_syntax_error (error_value))
    {
      uint32_t err_line = 0;
      uint32_t err_col = 0;

      /* 1. parse column and line information */
      for (uint32_t i = 0; i < sz; i++)
      {
        if (!strncmp ((char *) (err_str_buf + i), "[line: ", 7))
        {
          i += 7;

          char num_str[8];
          uint32_t j = 0;

          while (i < sz && err_str_buf[i] != ',')
          {
            num_str[j] = (char) err_str_buf[i];
            j++;
            i++;
          }
          num_str[j] = '\0';

          err_line = str_to_uint (num_str);

          if (strncmp ((char *) (err_str_buf + i), ", column: ", 10))
          {
            break; /* wrong position info format */
          }

          i += 10;
          j = 0;

          while (i < sz && err_str_buf[i] != ']')
          {
            num_str[j] = (char) err_str_buf[i];
            j++;
            i++;
          }
          num_str[j] = '\0';

          err_col = str_to_uint (num_str);
          break;
        }
      } /* for */

      if (err_line != 0 && err_col != 0)
      {
        uint32_t curr_line = 1;

        bool is_printing_context = false;
        uint32_t pos = 0;

        /* 2. seek and print */
        while (buffer[pos] != '\0')
        {
          if (buffer[pos] == '\n')
          {
            curr_line++;
          }

          if (err_line < SYNTAX_ERROR_CONTEXT_SIZE
              || (err_line >= curr_line
                  && (err_line - curr_line) <= SYNTAX_ERROR_CONTEXT_SIZE))
          {
            /* context must be printed */
            is_printing_context = true;
          }

          if (curr_line > err_line)
          {
            break;
          }

          if (is_printing_context)
          {
            jerry_port_log (JERRY_LOG_LEVEL_ERROR, "%c", buffer[pos]);
          }

          pos++;
        }

        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "\n");

        while (--err_col)
        {
          jerry_port_log (JERRY_LOG_LEVEL_ERROR, "~");
        }

        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "^\n");
      }
    }
  }

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
  int files_counter = 0;

  jerry_init_flag_t flags = JERRY_INIT_EMPTY;

  const char *exec_snapshot_file_names[JERRY_MAX_COMMAND_LINE_ARGS];
  int exec_snapshots_count = 0;

  bool is_parse_only = false;
  bool is_save_snapshot_mode = false;
  bool is_save_snapshot_mode_for_global_or_eval = false;
  const char *save_snapshot_file_name_p = NULL;

  bool is_save_literals_mode = false;
  bool is_save_literals_mode_in_c_format_or_list = false;
  const char *save_literals_file_name_p = NULL;

  bool is_repl_mode = false;
  bool no_prompt = false;

  for (int i = 1; i < argc; i++)
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
      if (jerry_is_feature_enabled (JERRY_FEATURE_MEM_STATS))
      {
        jerry_port_default_set_log_level (JERRY_LOG_LEVEL_DEBUG);
        flags |= JERRY_INIT_MEM_STATS;
      }
      else
      {
        jerry_port_default_set_log_level (JERRY_LOG_LEVEL_WARNING);
        jerry_port_log (JERRY_LOG_LEVEL_WARNING,
                        "Ignoring 'mem-stats' option because this feature is disabled!\n");
      }
    }
    else if (!strcmp ("--mem-stats-separate", argv[i]))
    {
      if (jerry_is_feature_enabled (JERRY_FEATURE_MEM_STATS))
      {
        jerry_port_default_set_log_level (JERRY_LOG_LEVEL_DEBUG);
        flags |= JERRY_INIT_MEM_STATS_SEPARATE;
      }
      else
      {
        jerry_port_default_set_log_level (JERRY_LOG_LEVEL_WARNING);
        jerry_port_log (JERRY_LOG_LEVEL_WARNING,
                        "Ignoring 'mem-stats-separate' option because this feature is disabled!\n");
      }
    }
    else if (!strcmp ("--parse-only", argv[i]))
    {
      is_parse_only = true;
    }
    else if (!strcmp ("--show-opcodes", argv[i]))
    {
      if (jerry_is_feature_enabled (JERRY_FEATURE_PARSER_DUMP))
      {
        jerry_port_default_set_log_level (JERRY_LOG_LEVEL_DEBUG);
        flags |= JERRY_INIT_SHOW_OPCODES;
      }
      else
      {
        jerry_port_default_set_log_level (JERRY_LOG_LEVEL_WARNING);
        jerry_port_log (JERRY_LOG_LEVEL_WARNING,
                        "Ignoring 'show-opcodes' option because this feature is disabled!\n");
      }
    }
    else if (!strcmp ("--show-regexp-opcodes", argv[i]))
    {
      if (jerry_is_feature_enabled (JERRY_FEATURE_PARSER_DUMP))
      {
        jerry_port_default_set_log_level (JERRY_LOG_LEVEL_DEBUG);
        flags |= JERRY_INIT_SHOW_REGEXP_OPCODES;
      }
      else
      {
        jerry_port_default_set_log_level (JERRY_LOG_LEVEL_WARNING);
        jerry_port_log (JERRY_LOG_LEVEL_WARNING,
                        "Ignoring 'show-regexp-opcodes' option because this feature is disabled!\n");
      }
    }
    else if (!strcmp ("--start-debug-server", argv[i]))
    {
      flags |= JERRY_INIT_DEBUGGER;
    }
    else if (!strcmp ("--save-snapshot-for-global", argv[i])
             || !strcmp ("--save-snapshot-for-eval", argv[i]))
    {
      if (++i >= argc)
      {
        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: no file specified for %s\n", argv[i - 1]);
        print_usage (argv[0]);
        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }

      if (jerry_is_feature_enabled (JERRY_FEATURE_SNAPSHOT_SAVE))
      {
        is_save_snapshot_mode = true;
        is_save_snapshot_mode_for_global_or_eval = !strcmp ("--save-snapshot-for-global", argv[i - 1]);

        if (save_snapshot_file_name_p != NULL)
        {
          jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: snapshot file name already specified\n");
          print_usage (argv[0]);
          return JERRY_STANDALONE_EXIT_CODE_FAIL;
        }

        save_snapshot_file_name_p = argv[i];
      }
      else
      {
        jerry_port_default_set_log_level (JERRY_LOG_LEVEL_WARNING);

        jerry_port_log (JERRY_LOG_LEVEL_WARNING,
                        "Ignoring 'save-snapshot' option because this feature is disabled!\n");
      }
    }
    else if (!strcmp ("--exec-snapshot", argv[i]))
    {
      if (++i >= argc)
      {
        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: no file specified for %s\n", argv[i - 1]);
        print_usage (argv[0]);
        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }

      if (jerry_is_feature_enabled (JERRY_FEATURE_SNAPSHOT_EXEC))
      {
        jerry_port_default_set_log_level (JERRY_LOG_LEVEL_DEBUG);

        assert (exec_snapshots_count < JERRY_MAX_COMMAND_LINE_ARGS);
        exec_snapshot_file_names[exec_snapshots_count++] = argv[i];
      }
      else
      {
        jerry_port_default_set_log_level (JERRY_LOG_LEVEL_WARNING);

        jerry_port_log (JERRY_LOG_LEVEL_WARNING,
                        "Ignoring 'exec-snapshot' option because this feature isn't enabled!\n");
      }
    }
    else if (!strcmp ("--save-literals-list-format", argv[i])
             || !strcmp ("--save-literals-c-format", argv[i]))
    {
      if (++i >= argc)
      {
        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: no file specified for %s\n", argv[i - 1]);
        print_usage (argv[0]);
        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }

      if (jerry_is_feature_enabled (JERRY_FEATURE_SNAPSHOT_SAVE))
      {
        is_save_literals_mode = true;
        is_save_literals_mode_in_c_format_or_list = !strcmp ("--save-literals-c-format", argv[i - 1]);

        if (save_literals_file_name_p != NULL)
        {
          jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: literal file name already specified\n");
          print_usage (argv[0]);
          return JERRY_STANDALONE_EXIT_CODE_FAIL;
        }

        save_literals_file_name_p = argv[i];
      }
      else
      {
        jerry_port_default_set_log_level (JERRY_LOG_LEVEL_WARNING);

        jerry_port_log (JERRY_LOG_LEVEL_WARNING,
                        "Ignoring 'save-literals' option because this feature is disabled!\n");
      }
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
    else if (!strcmp ("--no-prompt", argv[i]))
    {
      no_prompt = true;
    }
    else if (!strcmp ("-", argv[i]))
    {
      file_names[files_counter++] = argv[i];
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

  if (jerry_is_feature_enabled (JERRY_FEATURE_SNAPSHOT_SAVE) && is_save_snapshot_mode)
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

  if (jerry_is_feature_enabled (JERRY_FEATURE_SNAPSHOT_SAVE) && is_save_literals_mode)
  {
    if (files_counter != 1)
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR,
                      "Error: --save-literals-* options work with exactly one script\n");
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
  jerry_value_t ret_value = jerry_set_property (global_obj_val, assert_func_name_val, assert_value);

  jerry_release_value (assert_func_name_val);
  jerry_release_value (assert_value);
  jerry_release_value (global_obj_val);

  if (jerry_value_has_error_flag (ret_value))
  {
    jerry_port_log (JERRY_LOG_LEVEL_WARNING, "Warning: failed to register 'assert' method.");
    print_unhandled_exception (ret_value);
  }

  jerry_release_value (ret_value);

  ret_value = jerry_create_undefined ();

  if (jerry_is_feature_enabled (JERRY_FEATURE_SNAPSHOT_EXEC))
  {
    for (int i = 0; i < exec_snapshots_count; i++)
    {
      size_t snapshot_size;
      const uint8_t *snapshot_p = read_file (exec_snapshot_file_names[i], &snapshot_size);

      if (snapshot_p == NULL)
      {
        ret_value = jerry_create_error (JERRY_ERROR_COMMON, (jerry_char_t *) "Snapshot file load error");
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
  }

  if (!jerry_value_has_error_flag (ret_value))
  {
    for (int i = 0; i < files_counter; i++)
    {
      size_t source_size;
      const jerry_char_t *source_p = read_file (file_names[i], &source_size);

      if (source_p == NULL)
      {
        ret_value = jerry_create_error (JERRY_ERROR_COMMON, (jerry_char_t *) "Source file load error");
        break;
      }

      if (!jerry_is_valid_utf8_string (source_p, (jerry_size_t) source_size))
      {
        ret_value = jerry_create_error (JERRY_ERROR_COMMON, (jerry_char_t *) ("Input must be a valid UTF-8 string."));
        break;
      }

      if (jerry_is_feature_enabled (JERRY_FEATURE_SNAPSHOT_SAVE) && (is_save_snapshot_mode || is_save_literals_mode))
      {
        static uint8_t snapshot_save_buffer[ JERRY_BUFFER_SIZE ];

        if (is_save_snapshot_mode)
        {
          size_t snapshot_size = jerry_parse_and_save_snapshot ((jerry_char_t *) source_p,
                                                                source_size,
                                                                is_save_snapshot_mode_for_global_or_eval,
                                                                false,
                                                                snapshot_save_buffer,
                                                                JERRY_BUFFER_SIZE);
          if (snapshot_size == 0)
          {
            ret_value = jerry_create_error (JERRY_ERROR_COMMON, (jerry_char_t *) "Snapshot saving failed!");
          }
          else
          {
            FILE *snapshot_file_p = fopen (save_snapshot_file_name_p, "w");
            fwrite (snapshot_save_buffer, sizeof (uint8_t), snapshot_size, snapshot_file_p);
            fclose (snapshot_file_p);
          }
        }

        if (!jerry_value_has_error_flag (ret_value) && is_save_literals_mode)
        {
          const size_t literal_buffer_size = jerry_parse_and_save_literals ((jerry_char_t *) source_p,
                                                                            source_size,
                                                                            false,
                                                                            snapshot_save_buffer,
                                                                            JERRY_BUFFER_SIZE,
                                                                            is_save_literals_mode_in_c_format_or_list);
          if (literal_buffer_size == 0)
          {
            ret_value = jerry_create_error (JERRY_ERROR_COMMON, (jerry_char_t *) "Literal saving failed!");
          }
          else
          {
            FILE *literal_file_p = fopen (save_literals_file_name_p, "w");
            fwrite (snapshot_save_buffer, sizeof (uint8_t), literal_buffer_size, literal_file_p);
            fclose (literal_file_p);
          }
        }
      }
      else
      {
        ret_value = jerry_parse_named_resource ((jerry_char_t *) file_names[i],
                                                strlen (file_names[i]),
                                                source_p,
                                                source_size,
                                                false);

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

      jerry_release_value (ret_value);
      ret_value = jerry_create_undefined ();
    }
  }

  if (is_repl_mode)
  {
    const char *prompt = !no_prompt ? "jerry> " : "";
    bool is_done = false;

    jerry_value_t global_object_val = jerry_get_global_object ();
    jerry_value_t print_func_name_val = jerry_create_string ((jerry_char_t *) "print");
    jerry_value_t print_function = jerry_get_property (global_object_val, print_func_name_val);

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

    jerry_release_value (global_object_val);
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
