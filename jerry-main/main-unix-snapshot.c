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
               .help = "specify output file name (default: merged.snapshot)"),
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

  const char *output_file_name_p = "merged.snapshot";
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

  print_commands (argv[0]);
  return JERRY_STANDALONE_EXIT_CODE_OK;
} /* main */
