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
#include "jerryscript-ext/handler.h"
#include "jerryscript-port.h"
#include "jerryscript-port-default.h"

#include "cli.h"

/**
 * Maximum size of source code
 */
#define JERRY_BUFFER_SIZE (1048576)

/**
 * Maximum size of snapshots buffer
 */
#define JERRY_SNAPSHOT_BUFFER_SIZE (JERRY_BUFFER_SIZE / sizeof (uint32_t))

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

static const uint32_t *
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
  return (const uint32_t *) buffer;
} /* read_file */

/**
 * Check whether an error is a SyntaxError or not
 *
 * @return true - if param is SyntaxError
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
    jerry_release_value (error_name);
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
 * Print error value
 */
static void
print_unhandled_exception (jerry_value_t error_value) /**< error value */
{
  assert (!jerry_value_has_error_flag (error_value));

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
      unsigned int err_line = 0;
      unsigned int err_col = 0;

      /* 1. parse column and line information */
      for (jerry_size_t i = 0; i < sz; i++)
      {
        if (!strncmp ((char *) (err_str_buf + i), "[line: ", 7))
        {
          i += 7;

          char num_str[8];
          unsigned int j = 0;

          while (i < sz && err_str_buf[i] != ',')
          {
            num_str[j] = (char) err_str_buf[i];
            j++;
            i++;
          }
          num_str[j] = '\0';

          err_line = (unsigned int) strtol (num_str, NULL, 10);

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

          err_col = (unsigned int) strtol (num_str, NULL, 10);
          break;
        }
      } /* for */

      if (err_line != 0 && err_col != 0)
      {
        unsigned int curr_line = 1;

        bool is_printing_context = false;
        unsigned int pos = 0;

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

/**
 * Register a JavaScript function in the global object.
 */
static void
register_js_function (const char *name_p, /**< name of the function */
                      jerry_external_handler_t handler_p) /**< function callback */
{
  jerry_value_t result_val = jerryx_handler_register_global ((const jerry_char_t *) name_p, handler_p);

  if (jerry_value_has_error_flag (result_val))
  {
    jerry_port_log (JERRY_LOG_LEVEL_WARNING, "Warning: failed to register '%s' method.", name_p);
    jerry_value_clear_error_flag (&result_val);
    print_unhandled_exception (result_val);
  }

  jerry_release_value (result_val);
} /* register_js_function */

#ifdef JERRY_DEBUGGER

/**
 * Runs the source code received by jerry_debugger_wait_for_client_source.
 *
 * @return result fo the source code execution
 */
static jerry_value_t
wait_for_source_callback (const jerry_char_t *resource_name_p, /**< resource name */
                          size_t resource_name_size, /**< size of resource name */
                          const jerry_char_t *source_p, /**< source code */
                          size_t source_size, /**< source code size */
                          void *user_p __attribute__((unused))) /**< user pointer */
{
  jerry_value_t ret_val = jerry_parse_named_resource (resource_name_p,
                                                      resource_name_size,
                                                      source_p,
                                                      source_size,
                                                      false);

  if (!jerry_value_has_error_flag (ret_val))
  {
    jerry_value_t func_val = ret_val;
    ret_val = jerry_run (func_val);
    jerry_release_value (func_val);
  }

  return ret_val;
} /* wait_for_source_callback */

#endif /* JERRY_DEBUGGER */

/**
 * Command line option IDs
 */
typedef enum
{
  OPT_HELP,
  OPT_VERSION,
  OPT_MEM_STATS,
  OPT_PARSE_ONLY,
  OPT_SHOW_OP,
  OPT_SHOW_RE_OP,
  OPT_DEBUG_SERVER,
  OPT_DEBUG_PORT,
  OPT_DEBUGGER_WAIT_SOURCE,
  OPT_SAVE_SNAP_GLOBAL,
  OPT_SAVE_SNAP_EVAL,
  OPT_SAVE_LIT_LIST,
  OPT_SAVE_LIT_C,
  OPT_EXEC_SNAP,
  OPT_EXEC_SNAP_FUNC,
  OPT_LOG_LEVEL,
  OPT_ABORT_ON_FAIL,
  OPT_NO_PROMPT
} main_opt_id_t;

/**
 * Command line options
 */
static const cli_opt_t main_opts[] =
{
  CLI_OPT_DEF (.id = OPT_HELP, .opt = "h", .longopt = "help",
               .help = "print this help and exit"),
  CLI_OPT_DEF (.id = OPT_VERSION, .opt = "v", .longopt = "version",
               .help = "print tool and library version and exit"),
  CLI_OPT_DEF (.id = OPT_MEM_STATS, .longopt = "mem-stats",
               .help = "dump memory statistics"),
  CLI_OPT_DEF (.id = OPT_PARSE_ONLY, .longopt = "parse-only",
               .help = "don't execute JS input"),
  CLI_OPT_DEF (.id = OPT_SHOW_OP, .longopt = "show-opcodes",
               .help = "dump parser byte-code"),
  CLI_OPT_DEF (.id = OPT_SHOW_RE_OP, .longopt = "show-regexp-opcodes",
               .help = "dump regexp byte-code"),
  CLI_OPT_DEF (.id = OPT_DEBUG_SERVER, .longopt = "start-debug-server",
               .help = "start debug server and wait for a connecting client"),
  CLI_OPT_DEF (.id = OPT_DEBUG_PORT, .longopt = "debug-port", .meta = "NUM",
               .help = "debug server port (default: 5001)"),
  CLI_OPT_DEF (.id = OPT_DEBUGGER_WAIT_SOURCE, .longopt = "debugger-wait-source",
               .help = "wait for an executable source from the client"),
  CLI_OPT_DEF (.id = OPT_SAVE_SNAP_GLOBAL, .longopt = "save-snapshot-for-global", .meta = "FILE",
               .help = "save binary snapshot of parsed JS input (for execution in global context)"),
  CLI_OPT_DEF (.id = OPT_SAVE_SNAP_EVAL, .longopt = "save-snapshot-for-eval", .meta = "FILE",
               .help = "save binary snapshot of parsed JS input (for execution in local context by eval)"),
  CLI_OPT_DEF (.id = OPT_SAVE_LIT_LIST, .longopt = "save-literals-list-format", .meta = "FILE",
               .help = "export literals found in parsed JS input (in list format)"),
  CLI_OPT_DEF (.id = OPT_SAVE_LIT_C, .longopt = "save-literals-c-format", .meta = "FILE",
               .help = "export literals found in parsed JS input (in C source format)"),
  CLI_OPT_DEF (.id = OPT_EXEC_SNAP, .longopt = "exec-snapshot", .meta = "FILE",
               .help = "execute input snapshot file(s)"),
  CLI_OPT_DEF (.id = OPT_EXEC_SNAP_FUNC, .longopt = "exec-snapshot-func", .meta = "FILE NUM",
               .help = "execute specific function from input snapshot file(s)"),
  CLI_OPT_DEF (.id = OPT_LOG_LEVEL, .longopt = "log-level", .meta = "NUM",
               .help = "set log level (0-3)"),
  CLI_OPT_DEF (.id = OPT_ABORT_ON_FAIL, .longopt = "abort-on-fail",
               .help = "segfault on internal failure (instead of non-zero exit code)"),
  CLI_OPT_DEF (.id = OPT_NO_PROMPT, .longopt = "no-prompt",
               .help = "don't print prompt in REPL mode"),
  CLI_OPT_DEF (.id = CLI_OPT_DEFAULT, .meta = "FILE",
               .help = "input JS file(s)")
};

/**
 * Check whether JerryScript has a requested feature enabled or not. If not,
 * print a warning message.
 *
 * @return the status of the feature.
 */
static bool
check_feature (jerry_feature_t feature, /**< feature to check */
               const char *option) /**< command line option that triggered this check */
{
  if (!jerry_is_feature_enabled (feature))
  {
    jerry_port_default_set_log_level (JERRY_LOG_LEVEL_WARNING);
    jerry_port_log (JERRY_LOG_LEVEL_WARNING, "Ignoring '%s' option because this feature is disabled!\n", option);
    return false;
  }
  return true;
} /* check_feature */

/**
 * Check whether a usage-related condition holds. If not, print an error
 * message, print the usage, and terminate the application.
 */
static void
check_usage (bool condition, /**< the condition that must hold */
             const char *name, /**< name of the application (argv[0]) */
             const char *msg, /**< error message to print if condition does not hold */
             const char *opt) /**< optional part of the error message */
{
  if (!condition)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "%s: %s%s\n", name, msg, opt != NULL ? opt : "");
    exit (JERRY_STANDALONE_EXIT_CODE_FAIL);
  }
} /* check_usage */

#ifdef JERRY_ENABLE_EXTERNAL_CONTEXT

/**
 * The alloc function passed to jerry_create_instance
 */
static void *
instance_alloc (size_t size,
                void *cb_data_p __attribute__((unused)))
{
  return malloc (size);
} /* instance_alloc */

#endif /* JERRY_ENABLE_EXTERNAL_CONTEXT */

int
main (int argc,
      char **argv)
{
  const char *file_names[argc];
  int files_counter = 0;

  jerry_init_flag_t flags = JERRY_INIT_EMPTY;

  const char *exec_snapshot_file_names[argc];
  uint32_t exec_snapshot_file_indices[argc];
  int exec_snapshots_count = 0;

  bool is_parse_only = false;
  bool is_save_snapshot_mode = false;
  bool is_save_snapshot_mode_for_global_or_eval = false;
  const char *save_snapshot_file_name_p = NULL;

  bool is_save_literals_mode = false;
  bool is_save_literals_mode_in_c_format_or_list = false;
  const char *save_literals_file_name_p = NULL;

  bool start_debug_server = false;
  uint16_t debug_port = 5001;

  bool is_repl_mode = false;
  bool is_wait_mode = false;
  bool no_prompt = false;

  cli_state_t cli_state = cli_init (main_opts, argc - 1, argv + 1);
  for (int id = cli_consume_option (&cli_state); id != CLI_OPT_END; id = cli_consume_option (&cli_state))
  {
    switch (id)
    {
      case OPT_HELP:
      {
        cli_help (argv[0], NULL, main_opts);
        return JERRY_STANDALONE_EXIT_CODE_OK;
      }
      case OPT_VERSION:
      {
        printf ("Version: %d.%d%s\n", JERRY_API_MAJOR_VERSION, JERRY_API_MINOR_VERSION, JERRY_COMMIT_HASH);
        return JERRY_STANDALONE_EXIT_CODE_OK;
      }
      case OPT_MEM_STATS:
      {
        if (check_feature (JERRY_FEATURE_MEM_STATS, cli_state.arg))
        {
          jerry_port_default_set_log_level (JERRY_LOG_LEVEL_DEBUG);
          flags |= JERRY_INIT_MEM_STATS;
        }
        break;
      }
      case OPT_PARSE_ONLY:
      {
        is_parse_only = true;
        break;
      }
      case OPT_SHOW_OP:
      {
        if (check_feature (JERRY_FEATURE_PARSER_DUMP, cli_state.arg))
        {
          jerry_port_default_set_log_level (JERRY_LOG_LEVEL_DEBUG);
          flags |= JERRY_INIT_SHOW_OPCODES;
        }
        break;
      }
      case OPT_SHOW_RE_OP:
      {
        if (check_feature (JERRY_FEATURE_REGEXP_DUMP, cli_state.arg))
        {
          jerry_port_default_set_log_level (JERRY_LOG_LEVEL_DEBUG);
          flags |= JERRY_INIT_SHOW_REGEXP_OPCODES;
        }
        break;
      }
      case OPT_DEBUG_SERVER:
      {
        if (check_feature (JERRY_FEATURE_DEBUGGER, cli_state.arg))
        {
          start_debug_server = true;
        }
        break;
      }
      case OPT_DEBUG_PORT:
      {
        if (check_feature (JERRY_FEATURE_DEBUGGER, cli_state.arg))
        {
          debug_port = (uint16_t) cli_consume_int (&cli_state);
        }
        break;
      }
      case OPT_DEBUGGER_WAIT_SOURCE:
      {
        if (check_feature (JERRY_FEATURE_DEBUGGER, cli_state.arg))
        {
          is_wait_mode = true;
        }
        break;
      }
      case OPT_SAVE_SNAP_GLOBAL:
      case OPT_SAVE_SNAP_EVAL:
      {
        check_usage (save_snapshot_file_name_p == NULL, argv[0], "Error: snapshot file name already specified", NULL);
        if (check_feature (JERRY_FEATURE_SNAPSHOT_SAVE, cli_state.arg))
        {
          is_save_snapshot_mode = true;
          is_save_snapshot_mode_for_global_or_eval = (id == OPT_SAVE_SNAP_GLOBAL);
        }
        save_snapshot_file_name_p = cli_consume_string (&cli_state);
        break;
      }
      case OPT_SAVE_LIT_LIST:
      case OPT_SAVE_LIT_C:
      {
        check_usage (save_literals_file_name_p == NULL, argv[0], "Error: literal file name already specified", NULL);
        if (check_feature (JERRY_FEATURE_SNAPSHOT_SAVE, cli_state.arg))
        {
          is_save_literals_mode = true;
          is_save_literals_mode_in_c_format_or_list = (id == OPT_SAVE_LIT_C);
        }
        save_literals_file_name_p = cli_consume_string (&cli_state);
        break;
      }
      case OPT_EXEC_SNAP:
      {
        if (check_feature (JERRY_FEATURE_SNAPSHOT_EXEC, cli_state.arg))
        {
          exec_snapshot_file_names[exec_snapshots_count] = cli_consume_string (&cli_state);
          exec_snapshot_file_indices[exec_snapshots_count++] = 0;
        }
        else
        {
          cli_consume_string (&cli_state);
        }
        break;
      }
      case OPT_EXEC_SNAP_FUNC:
      {
        if (check_feature (JERRY_FEATURE_SNAPSHOT_EXEC, cli_state.arg))
        {
          exec_snapshot_file_names[exec_snapshots_count] = cli_consume_string (&cli_state);
          exec_snapshot_file_indices[exec_snapshots_count++] = (uint32_t) cli_consume_int (&cli_state);
        }
        else
        {
          cli_consume_string (&cli_state);
        }
        break;
      }
      case OPT_LOG_LEVEL:
      {
        long int log_level = cli_consume_int (&cli_state);
        check_usage (log_level >= 0 && log_level <= 3,
                     argv[0], "Error: invalid value for --log-level: ", cli_state.arg);

        jerry_port_default_set_log_level ((jerry_log_level_t) log_level);
        break;
      }
      case OPT_ABORT_ON_FAIL:
      {
        jerry_port_default_set_abort_on_fail (true);
        break;
      }
      case OPT_NO_PROMPT:
      {
        no_prompt = true;
        break;
      }
      case CLI_OPT_DEFAULT:
      {
        file_names[files_counter++] = cli_consume_string (&cli_state);
        break;
      }
      default:
      {
        cli_state.error = "Internal error";
        break;
      }
    }
  }

  if (cli_state.error != NULL)
  {
    if (cli_state.arg != NULL)
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: %s %s\n", cli_state.error, cli_state.arg);
    }
    else
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: %s\n", cli_state.error);
    }

    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  if (is_save_snapshot_mode)
  {
    check_usage (files_counter == 1,
                 argv[0], "Error: --save-snapshot-* options work with exactly one script", NULL);
    check_usage (exec_snapshots_count == 0,
                 argv[0], "Error: --save-snapshot-* and --exec-snapshot options can't be passed simultaneously", NULL);
  }

  if (is_save_literals_mode)
  {
    check_usage (files_counter == 1,
                argv[0], "Error: --save-literals-* options work with exactly one script", NULL);
  }

  if (files_counter == 0
      && exec_snapshots_count == 0)
  {
    is_repl_mode = true;
  }

#ifdef JERRY_ENABLE_EXTERNAL_CONTEXT

  jerry_instance_t *instance_p = jerry_create_instance (512*1024, instance_alloc, NULL);
  jerry_port_default_set_instance (instance_p);

#endif /* JERRY_ENABLE_EXTERNAL_CONTEXT */

  jerry_init (flags);
  if (start_debug_server)
  {
    jerry_debugger_init (debug_port);
  }

  register_js_function ("assert", jerryx_handler_assert);
  register_js_function ("gc", jerryx_handler_gc);
  register_js_function ("print", jerryx_handler_print);

  jerry_value_t ret_value = jerry_create_undefined ();

  if (jerry_is_feature_enabled (JERRY_FEATURE_SNAPSHOT_EXEC))
  {
    for (int i = 0; i < exec_snapshots_count; i++)
    {
      size_t snapshot_size;
      const uint32_t *snapshot_p = read_file (exec_snapshot_file_names[i], &snapshot_size);

      if (snapshot_p == NULL)
      {
        ret_value = jerry_create_error (JERRY_ERROR_COMMON, (jerry_char_t *) "Snapshot file load error");
      }
      else
      {
        ret_value = jerry_exec_snapshot_at (snapshot_p,
                                            snapshot_size,
                                            exec_snapshot_file_indices[i],
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
      const jerry_char_t *source_p = (jerry_char_t *) read_file (file_names[i], &source_size);

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

      if (is_save_snapshot_mode || is_save_literals_mode)
      {
        static uint32_t snapshot_save_buffer[ JERRY_SNAPSHOT_BUFFER_SIZE ];

        if (is_save_snapshot_mode)
        {
          size_t snapshot_size = jerry_parse_and_save_snapshot ((jerry_char_t *) source_p,
                                                                source_size,
                                                                is_save_snapshot_mode_for_global_or_eval,
                                                                false,
                                                                snapshot_save_buffer,
                                                                JERRY_SNAPSHOT_BUFFER_SIZE);
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
                                                                            JERRY_SNAPSHOT_BUFFER_SIZE,
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

  if (is_wait_mode)
  {
    is_repl_mode = false;
#ifdef JERRY_DEBUGGER

    while (true)
    {
      jerry_debugger_wait_for_source_status_t receive_status;

      do
      {
        jerry_value_t run_result;

        receive_status = jerry_debugger_wait_for_client_source (wait_for_source_callback,
                                                                NULL,
                                                                &run_result);

        if (receive_status == JERRY_DEBUGGER_SOURCE_RECEIVE_FAILED)
        {
          ret_value = jerry_create_error (JERRY_ERROR_COMMON,
                                          (jerry_char_t *) "Connection aborted before source arrived.");
        }

        if (receive_status == JERRY_DEBUGGER_SOURCE_END)
        {
          jerry_port_log (JERRY_LOG_LEVEL_DEBUG, "No more client source.\n");
        }

        jerry_release_value (run_result);
      }
      while (receive_status == JERRY_DEBUGGER_SOURCE_RECEIVED);

      if (receive_status != JERRY_DEBUGGER_CONTEXT_RESET_RECEIVED)
      {
        break;
      }

      jerry_cleanup ();

      jerry_init (flags);
      jerry_debugger_init (debug_port);

      register_js_function ("assert", jerryx_handler_assert);
      register_js_function ("gc", jerryx_handler_gc);
      register_js_function ("print", jerryx_handler_print);

      ret_value = jerry_create_undefined ();
    }

#endif /* JERRY_DEBUGGER */
  }

  if (is_repl_mode)
  {
    const char *prompt = !no_prompt ? "jerry> " : "";
    bool is_done = false;

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
        jerry_value_t ret_val_eval = jerry_eval (buffer, len, false);

        if (!jerry_value_has_error_flag (ret_val_eval))
        {
          /* Print return value */
          const jerry_value_t args[] = { ret_val_eval };
          jerry_value_t ret_val_print = jerryx_handler_print (jerry_create_undefined (),
                                                              jerry_create_undefined (),
                                                              args,
                                                              1);
          jerry_release_value (ret_val_print);
          jerry_release_value (ret_val_eval);
          ret_val_eval = jerry_run_all_enqueued_jobs ();

          if (jerry_value_has_error_flag (ret_val_eval))
          {
            jerry_value_clear_error_flag (&ret_val_eval);
            print_unhandled_exception (ret_val_eval);
          }
        }
        else
        {
          jerry_value_clear_error_flag (&ret_val_eval);
          print_unhandled_exception (ret_val_eval);
        }

        jerry_release_value (ret_val_eval);
      }
    }
  }

  int ret_code = JERRY_STANDALONE_EXIT_CODE_OK;

  if (jerry_value_has_error_flag (ret_value))
  {
    jerry_value_clear_error_flag (&ret_value);
    print_unhandled_exception (ret_value);

    ret_code = JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  jerry_release_value (ret_value);

  ret_value = jerry_run_all_enqueued_jobs ();

  if (jerry_value_has_error_flag (ret_value))
  {
    jerry_value_clear_error_flag (&ret_value);
    print_unhandled_exception (ret_value);
    ret_code = JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  jerry_release_value (ret_value);

  jerry_cleanup ();
#ifdef JERRY_ENABLE_EXTERNAL_CONTEXT
  free (instance_p);
#endif /* JERRY_ENABLE_EXTERNAL_CONTEXT */
  return ret_code;
} /* main */
