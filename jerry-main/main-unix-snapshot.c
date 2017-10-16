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

#include <string.h>

#include "jerryscript.h"
#include "jerryscript-port.h"
#include "jerryscript-port-default.h"

#include "cli.h"

/**
 * Maximum size for loaded snapshots
 */
#define JERRY_BUFFER_SIZE (1048576)

/**
 * Standalone Jerry exit codes
 */
#define JERRY_STANDALONE_EXIT_CODE_OK   (0)
#define JERRY_STANDALONE_EXIT_CODE_FAIL (1)

static uint8_t input_buffer[ JERRY_BUFFER_SIZE ];
static uint32_t output_buffer[ JERRY_BUFFER_SIZE / 4 ];
static const char *output_file_name_p = "js.snapshot";

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
 * Utility method to check and print error in the given cli state.
 *
 * @return true - if any error is detected
 *         false - if there is no error in the cli state
 */
static bool
check_cli_error (const cli_state_t *const cli_state_p)
{
  if (cli_state_p->error != NULL)
  {
    if (cli_state_p->arg != NULL)
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: %s %s\n", cli_state_p->error, cli_state_p->arg);
    }
    else
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: %s\n", cli_state_p->error);
    }

    return true;
  }

  return false;
} /* check_cli_error */

/**
 * Loading a single file into the memory.
 *
 * @return size of file - if loading is successful
 *         0 - otherwise
 */
static size_t
read_file (uint8_t *input_pos_p, /**< next position in the input buffer */
           const char *file_name) /**< file name */
{
  FILE *file = fopen (file_name, "r");

  if (file == NULL)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: failed to open file: %s\n", file_name);
    return 0;
  }

  size_t max_size = (size_t) (input_buffer + JERRY_BUFFER_SIZE - input_pos_p);

  size_t bytes_read = fread (input_pos_p, 1u, max_size, file);
  fclose (file);

  if (bytes_read == 0)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: failed to read file: %s\n", file_name);
    return 0;
  }

  if (bytes_read == max_size)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: file too large: %s\n", file_name);
    return 0;
  }

  printf ("Input file '%s' (%d bytes) loaded.\n", file_name, (int) bytes_read);
  return bytes_read;
} /* read_file */

/**
 * Generate command line option IDs
 */
typedef enum
{
  OPT_GENERATE_HELP,
  OPT_GENERATE_CONTEXT,
  OPT_GENERATE_SHOW_OP,
  OPT_GENERATE_LITERAL_LIST,
  OPT_GENERATE_LITERAL_C,
  OPT_GENERATE_OUT,
} generate_opt_id_t;

/**
 * Generate command line options
 */
static const cli_opt_t generate_opts[] =
{
  CLI_OPT_DEF (.id = OPT_GENERATE_HELP, .opt = "h", .longopt = "help",
               .help = "print this help and exit"),
  CLI_OPT_DEF (.id = OPT_GENERATE_SHOW_OP, .longopt = "show-opcodes",
               .help = "print generated opcodes"),
  CLI_OPT_DEF (.id = OPT_GENERATE_CONTEXT, .opt = "c", .longopt = "context",
               .meta = "MODE",
               .help = "specify the execution context of the snapshot: "
                       "global or eval (default: global)."),
  CLI_OPT_DEF (.id = OPT_GENERATE_LITERAL_LIST, .longopt = "save-literals-list-format",
               .meta = "FILE",
               .help = "export literals found in parsed JS input (in list format)"),
  CLI_OPT_DEF (.id = OPT_GENERATE_LITERAL_C, .longopt = "save-literals-c-format",
               .meta = "FILE",
               .help = "export literals found in parsed JS input (in C source format)"),
  CLI_OPT_DEF (.id = OPT_GENERATE_OUT, .opt = "o",  .meta="FILE",
               .help = "specify output file name (default: js.snapshot)"),
  CLI_OPT_DEF (.id = CLI_OPT_DEFAULT, .meta = "FILE",
               .help = "input snapshot file")
};

/**
 * Process 'generate' command.
 *
 * @return error code (0 - no error)
 */
static int
process_generate (cli_state_t *cli_state_p, /**< cli state */
                  int argc, /**< number of arguments */
                  char *prog_name_p) /**< program name */
{
  (void) argc;

  bool is_save_literals_mode_in_c_format = false;
  bool is_snapshot_mode_for_global = true;
  jerry_init_flag_t flags = JERRY_INIT_EMPTY;

  uint32_t number_of_files = 0;
  uint8_t *source_p = input_buffer;
  size_t source_length = 0;
  const char *save_literals_file_name_p = NULL;

  cli_change_opts (cli_state_p, generate_opts);

  for (int id = cli_consume_option (cli_state_p); id != CLI_OPT_END; id = cli_consume_option (cli_state_p))
  {
    switch (id)
    {
      case OPT_GENERATE_HELP:
      {
        cli_help (prog_name_p, "generate", generate_opts);
        return JERRY_STANDALONE_EXIT_CODE_OK;
      }
      case OPT_GENERATE_OUT:
      {
        output_file_name_p = cli_consume_string (cli_state_p);
        break;
      }
      case OPT_GENERATE_LITERAL_LIST:
      case OPT_GENERATE_LITERAL_C:
      {
        if (save_literals_file_name_p != NULL)
        {
          jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: literal file name already specified");
          return JERRY_STANDALONE_EXIT_CODE_FAIL;
        }

        is_save_literals_mode_in_c_format = (id == OPT_GENERATE_LITERAL_C);
        save_literals_file_name_p = cli_consume_string (cli_state_p);
        break;
      }
      case OPT_GENERATE_SHOW_OP:
      {
        if (check_feature (JERRY_FEATURE_PARSER_DUMP, cli_state_p->arg))
        {
          jerry_port_default_set_log_level (JERRY_LOG_LEVEL_DEBUG);
          flags |= JERRY_INIT_SHOW_OPCODES;
        }
        break;
      }
      case OPT_GENERATE_CONTEXT:
      {
        const char *mode_str_p = cli_consume_string (cli_state_p);

        if (cli_state_p->error != NULL)
        {
          break;
        }

        if (!strcmp ("global", mode_str_p))
        {
          is_snapshot_mode_for_global = true;
        }
        else if (!strcmp ("eval", mode_str_p))
        {
          is_snapshot_mode_for_global = false;
        }
        else
        {
          jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Incorrect argument for context mode: %s\n", mode_str_p);
          return JERRY_STANDALONE_EXIT_CODE_FAIL;
        }
        break;
      }
      case CLI_OPT_DEFAULT:
      {
        const char *file_name_p = cli_consume_string (cli_state_p);

        if (cli_state_p->error == NULL)
        {
          source_length = read_file (source_p, file_name_p);

          if (source_length == 0)
          {
            jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Input file is empty\n");
            return JERRY_STANDALONE_EXIT_CODE_FAIL;
          }

          number_of_files++;
        }
        break;
      }
      default:
      {
        cli_state_p->error = "Internal error";
        break;
      }
    }
  }

  if (check_cli_error (cli_state_p))
  {
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  if (number_of_files != 1)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: No input file specified!\n");
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  jerry_init (flags);

  if (!jerry_is_valid_utf8_string (source_p, (jerry_size_t) source_length))
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: Input must be a valid UTF-8 string.\n");
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  size_t snapshot_size = jerry_parse_and_save_snapshot ((jerry_char_t *) source_p,
                                                        source_length,
                                                        is_snapshot_mode_for_global,
                                                        false,
                                                        output_buffer,
                                                        sizeof (output_buffer) / sizeof (uint32_t));
  if (snapshot_size == 0)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: Generating snapshot failed!\n");
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  FILE *snapshot_file_p = fopen (output_file_name_p, "w");
  if (snapshot_file_p == NULL)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: Unable to write snapshot file: '%s'\n", output_file_name_p);
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  fwrite (output_buffer, sizeof (uint8_t), snapshot_size, snapshot_file_p);
  fclose (snapshot_file_p);

  printf ("Created snapshot file: '%s' (%lu bytes)\n", output_file_name_p, (unsigned long) snapshot_size);

  if (save_literals_file_name_p != NULL)
  {
    const size_t literal_buffer_size = jerry_parse_and_save_literals ((jerry_char_t *) source_p,
                                                                      source_length,
                                                                      false,
                                                                      output_buffer,
                                                                      sizeof (output_buffer) / sizeof (uint32_t),
                                                                      is_save_literals_mode_in_c_format);
    if (literal_buffer_size == 0)
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: Literal saving failed!\n");
      return JERRY_STANDALONE_EXIT_CODE_FAIL;
    }

    FILE *literal_file_p = fopen (save_literals_file_name_p, "w");

    if (literal_file_p == NULL)
    {
      jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: Unable to write literal file: '%s'\n", save_literals_file_name_p);
      return JERRY_STANDALONE_EXIT_CODE_FAIL;
    }

    fwrite (output_buffer, sizeof (uint8_t), literal_buffer_size, literal_file_p);
    fclose (literal_file_p);

    printf ("Created literal file: '%s' (%lu bytes)\n", save_literals_file_name_p, (unsigned long) literal_buffer_size);
  }

  return 0;
} /* process_generate */

/**
 * Merge command line option IDs
 */
typedef enum
{
  OPT_MERGE_HELP,
  OPT_MERGE_OUT,
} merge_opt_id_t;

/**
 * Merge command line options
 */
static const cli_opt_t merge_opts[] =
{
  CLI_OPT_DEF (.id = OPT_MERGE_HELP, .opt = "h", .longopt = "help",
               .help = "print this help and exit"),
  CLI_OPT_DEF (.id = OPT_MERGE_OUT, .opt = "o",
               .help = "specify output file name (default: js.snapshot)"),
  CLI_OPT_DEF (.id = CLI_OPT_DEFAULT, .meta = "FILE",
               .help = "input snapshot files, minimum two")
};

/**
 * Process 'merge' command.
 *
 * @return error code (0 - no error)
 */
static int
process_merge (cli_state_t *cli_state_p, /**< cli state */
               int argc, /**< number of arguments */
               char *prog_name_p) /**< program name */
{
  jerry_init (JERRY_INIT_EMPTY);

  uint8_t *input_pos_p = input_buffer;

  cli_change_opts (cli_state_p, merge_opts);

  const uint32_t *merge_buffers[argc];
  size_t merge_buffer_sizes[argc];
  uint32_t number_of_files = 0;

  for (int id = cli_consume_option (cli_state_p); id != CLI_OPT_END; id = cli_consume_option (cli_state_p))
  {
    switch (id)
    {
      case OPT_MERGE_HELP:
      {
        cli_help (prog_name_p, "merge", merge_opts);
        return JERRY_STANDALONE_EXIT_CODE_OK;
      }
      case OPT_MERGE_OUT:
      {
        output_file_name_p = cli_consume_string (cli_state_p);
        break;
      }
      case CLI_OPT_DEFAULT:
      {
        const char *file_name_p = cli_consume_string (cli_state_p);

        if (cli_state_p->error == NULL)
        {
          size_t size = read_file (input_pos_p, file_name_p);

          if (size == 0)
          {
            return JERRY_STANDALONE_EXIT_CODE_FAIL;
          }

          merge_buffers[number_of_files] = (const uint32_t *) input_pos_p;
          merge_buffer_sizes[number_of_files] = size;

          number_of_files++;
          const uintptr_t mask = sizeof (uint32_t) - 1;
          input_pos_p = (uint8_t *) ((((uintptr_t) input_pos_p) + size + mask) & ~mask);
        }
        break;
      }
      default:
      {
        cli_state_p->error = "Internal error";
        break;
      }
    }
  }

  if (check_cli_error (cli_state_p))
  {
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  if (number_of_files < 2)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: at least two input files must be passed.\n");

    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  const char *error_p;
  size_t size = jerry_merge_snapshots (merge_buffers,
                                       merge_buffer_sizes,
                                       number_of_files,
                                       output_buffer,
                                       JERRY_BUFFER_SIZE,
                                       &error_p);

  if (size == 0)
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: %s\n", error_p);
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  FILE *file_p = fopen (output_file_name_p, "w");

  if (file_p != NULL)
  {
    fwrite (output_buffer, 1u, size, file_p);
    fclose (file_p);
  }
  else
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: cannot open file: '%s'\n", output_file_name_p);
  }

  return JERRY_STANDALONE_EXIT_CODE_OK;
} /* process_merge */

/**
 * Command line option IDs
 */
typedef enum
{
  OPT_HELP,
} main_opt_id_t;

/**
 * Command line options
 */
static const cli_opt_t main_opts[] =
{
  CLI_OPT_DEF (.id = OPT_HELP, .opt = "h", .longopt = "help",
               .help = "print this help and exit"),
  CLI_OPT_DEF (.id = CLI_OPT_DEFAULT, .meta = "COMMAND",
               .help = "specify the command")
};

/**
 * Print available commands.
 */
static void
print_commands (char *prog_name_p) /**< program name */
{
  cli_help (prog_name_p, NULL, main_opts);

  printf ("\nAvailable commands:\n"
          "  generate\n"
          "  merge\n"
          "\nPassing -h or --help after a command displays its help.\n");
} /* print_commands */

/**
 * Main function.
 *
 * @return error code (0 - no error)
 */
int
main (int argc, /**< number of arguments */
      char **argv) /**< argument list */
{
  cli_state_t cli_state = cli_init (main_opts, argc - 1, argv + 1);

  for (int id = cli_consume_option (&cli_state); id != CLI_OPT_END; id = cli_consume_option (&cli_state))
  {
    switch (id)
    {
      case OPT_MERGE_HELP:
      {
        /* Help is always printed if no command is provided. */
        break;
      }
      case CLI_OPT_DEFAULT:
      {
        const char *command_p = cli_consume_string (&cli_state);

        if (cli_state.error != NULL)
        {
          break;
        }

        if (!strcmp ("merge", command_p))
        {
          return process_merge (&cli_state, argc, argv[0]);
        }
        else if (!strcmp ("generate", command_p))
        {
          return process_generate (&cli_state, argc, argv[0]);
        }

        jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Error: unknown command: %s\n\n", command_p);
        print_commands (argv[0]);

        return JERRY_STANDALONE_EXIT_CODE_FAIL;
      }
      default:
      {
        cli_state.error = "Internal error";
        break;
      }
    }
  }

  if (check_cli_error (&cli_state))
  {
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  print_commands (argv[0]);
  return JERRY_STANDALONE_EXIT_CODE_OK;
} /* main */
