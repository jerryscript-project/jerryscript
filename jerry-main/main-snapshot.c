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

#include "jerryscript-port.h"
#include "jerryscript.h"

#include "arguments/cli.h"

/**
 * Maximum size for loaded snapshots
 */
#define JERRY_BUFFER_SIZE (1048576)

/**
 * Maximum number of loaded literals
 */
#define JERRY_LITERAL_LENGTH (4096)

/**
 * Standalone Jerry exit codes
 */
#define JERRY_STANDALONE_EXIT_CODE_OK   (0)
#define JERRY_STANDALONE_EXIT_CODE_FAIL (1)

static uint8_t input_buffer[JERRY_BUFFER_SIZE];
static uint32_t output_buffer[JERRY_BUFFER_SIZE / 4];
static jerry_char_t literal_buffer[JERRY_BUFFER_SIZE];
static const char *output_file_name_p = "js.snapshot";
static jerry_length_t magic_string_lengths[JERRY_LITERAL_LENGTH];
static const jerry_char_t *magic_string_items[JERRY_LITERAL_LENGTH];

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
  if (!jerry_feature_enabled (feature))
  {
    jerry_log_set_level (JERRY_LOG_LEVEL_WARNING);
    jerry_log (JERRY_LOG_LEVEL_WARNING, "Ignoring '%s' option because this feature is disabled!\n", option);
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
      jerry_log (JERRY_LOG_LEVEL_ERROR, "Error: %s %s\n", cli_state_p->error, cli_state_p->arg);
    }
    else
    {
      jerry_log (JERRY_LOG_LEVEL_ERROR, "Error: %s\n", cli_state_p->error);
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
  FILE *file = fopen (file_name, "rb");

  if (file == NULL)
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Error: failed to open file: %s\n", file_name);
    return 0;
  }

  size_t max_size = (size_t) (input_buffer + JERRY_BUFFER_SIZE - input_pos_p);

  size_t bytes_read = fread (input_pos_p, 1u, max_size, file);
  fclose (file);

  if (bytes_read == 0)
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Error: failed to read file: %s\n", file_name);
    return 0;
  }

  if (bytes_read == max_size)
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Error: file too large: %s\n", file_name);
    return 0;
  }

  printf ("Input file '%s' (%zu bytes) loaded.\n", file_name, bytes_read);
  return bytes_read;
} /* read_file */

/**
 * Print error value
 */
static void
print_unhandled_exception (jerry_value_t error_value) /**< error value */
{
  assert (!jerry_value_is_exception (error_value));

  jerry_value_t err_str_val = jerry_value_to_string (error_value);

  if (jerry_value_is_exception (err_str_val))
  {
    /* Avoid recursive error throws. */
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Snapshot error: [value cannot be converted to string]\n");
    jerry_value_free (err_str_val);
    return;
  }

  jerry_char_t err_str_buf[256];
  jerry_size_t bytes = jerry_string_to_buffer (err_str_val, JERRY_ENCODING_UTF8, err_str_buf, sizeof (err_str_buf) - 1);
  err_str_buf[bytes] = '\0';

  jerry_log (JERRY_LOG_LEVEL_ERROR, "Snapshot error: %s\n", (char *) err_str_buf);
  jerry_value_free (err_str_val);
} /* print_unhandled_exception */

/**
 * Generate command line option IDs
 */
typedef enum
{
  OPT_GENERATE_HELP,
  OPT_GENERATE_STATIC,
  OPT_GENERATE_SHOW_OP,
  OPT_GENERATE_FUNCTION,
  OPT_GENERATE_OUT,
  OPT_IMPORT_LITERAL_LIST
} generate_opt_id_t;

/**
 * Generate command line options
 */
static const cli_opt_t generate_opts[] = {
  CLI_OPT_DEF (.id = OPT_GENERATE_HELP, .opt = "h", .longopt = "help", .help = "print this help and exit"),
  CLI_OPT_DEF (.id = OPT_GENERATE_STATIC, .opt = "s", .longopt = "static", .help = "generate static snapshot"),
  CLI_OPT_DEF (.id = OPT_GENERATE_FUNCTION,
               .opt = "f",
               .longopt = "generate-function-snapshot",
               .meta = "ARGUMENTS",
               .help = "generate function snapshot with given arguments"),
  CLI_OPT_DEF (.id = OPT_IMPORT_LITERAL_LIST,
               .longopt = "load-literals-list-format",
               .meta = "FILE",
               .help = "import literals from list format (for static snapshots)"),
  CLI_OPT_DEF (.id = OPT_GENERATE_SHOW_OP, .longopt = "show-opcodes", .help = "print generated opcodes"),
  CLI_OPT_DEF (.id = OPT_GENERATE_OUT,
               .opt = "o",
               .meta = "FILE",
               .help = "specify output file name (default: js.snapshot)"),
  CLI_OPT_DEF (.id = CLI_OPT_DEFAULT, .meta = "FILE", .help = "input source file")
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

  uint32_t snapshot_flags = 0;
  jerry_init_flag_t flags = JERRY_INIT_EMPTY;

  const char *file_name_p = NULL;
  uint8_t *source_p = input_buffer;
  size_t source_length = 0;
  const char *literals_file_name_p = NULL;
  const char *function_args_p = NULL;

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
      case OPT_GENERATE_STATIC:
      {
        snapshot_flags |= JERRY_SNAPSHOT_SAVE_STATIC;
        break;
      }
      case OPT_GENERATE_FUNCTION:
      {
        function_args_p = cli_consume_string (cli_state_p);
        break;
      }
      case OPT_IMPORT_LITERAL_LIST:
      {
        literals_file_name_p = cli_consume_string (cli_state_p);
        break;
      }
      case OPT_GENERATE_SHOW_OP:
      {
        if (check_feature (JERRY_FEATURE_PARSER_DUMP, cli_state_p->arg))
        {
          jerry_log_set_level (JERRY_LOG_LEVEL_DEBUG);
          flags |= JERRY_INIT_SHOW_OPCODES;
        }
        break;
      }
      case OPT_GENERATE_OUT:
      {
        output_file_name_p = cli_consume_string (cli_state_p);
        break;
      }
      case CLI_OPT_DEFAULT:
      {
        if (file_name_p != NULL)
        {
          jerry_log (JERRY_LOG_LEVEL_ERROR, "Error: Exactly one input file must be specified\n");
          return JERRY_STANDALONE_EXIT_CODE_FAIL;
        }

        file_name_p = cli_consume_string (cli_state_p);

        if (cli_state_p->error == NULL)
        {
          source_length = read_file (source_p, file_name_p);

          if (source_length == 0)
          {
            jerry_log (JERRY_LOG_LEVEL_ERROR, "Input file is empty\n");
            return JERRY_STANDALONE_EXIT_CODE_FAIL;
          }
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

  if (file_name_p == NULL)
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Error: Exactly one input file must be specified\n");
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  jerry_init (flags);

  if (!jerry_validate_string (source_p, (jerry_size_t) source_length, JERRY_ENCODING_UTF8))
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Error: Input must be a valid UTF-8 string.\n");
    jerry_cleanup ();
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  if (literals_file_name_p != NULL)
  {
    /* Import literal list */
    uint8_t *sp_buffer_start_p = source_p + source_length + 1;
    size_t sp_buffer_size = read_file (sp_buffer_start_p, literals_file_name_p);

    if (sp_buffer_size > 0)
    {
      const char *sp_buffer_p = (const char *) sp_buffer_start_p;
      uint32_t num_of_lit = 0;

      do
      {
        char *sp_buffer_end_p = NULL;
        jerry_length_t mstr_size = (jerry_length_t) strtol (sp_buffer_p, &sp_buffer_end_p, 10);
        if (mstr_size > 0)
        {
          magic_string_items[num_of_lit] = (jerry_char_t *) (sp_buffer_end_p + 1);
          magic_string_lengths[num_of_lit] = mstr_size;
          num_of_lit++;
        }
        sp_buffer_p = sp_buffer_end_p + mstr_size + 1;
      } while ((size_t) (sp_buffer_p - (char *) sp_buffer_start_p) < sp_buffer_size);

      if (num_of_lit > 0)
      {
        jerry_register_magic_strings (magic_string_items, num_of_lit, magic_string_lengths);
      }
    }
  }

  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_HAS_SOURCE_NAME;
  /* To avoid cppcheck warning. */
  parse_options.argument_list = 0;
  parse_options.source_name =
    jerry_string ((const jerry_char_t *) file_name_p, (jerry_size_t) strlen (file_name_p), JERRY_ENCODING_UTF8);

  if (function_args_p != NULL)
  {
    parse_options.options |= JERRY_PARSE_HAS_ARGUMENT_LIST;
    parse_options.argument_list = jerry_string ((const jerry_char_t *) function_args_p,
                                                (jerry_size_t) strlen (function_args_p),
                                                JERRY_ENCODING_UTF8);
  }

  jerry_value_t snapshot_result = jerry_parse ((jerry_char_t *) source_p, source_length, &parse_options);

  if (!jerry_value_is_exception (snapshot_result))
  {
    jerry_value_t parse_result = snapshot_result;
    snapshot_result =
      jerry_generate_snapshot (parse_result, snapshot_flags, output_buffer, sizeof (output_buffer) / sizeof (uint32_t));
    jerry_value_free (parse_result);
  }

  if (parse_options.options & JERRY_PARSE_HAS_ARGUMENT_LIST)
  {
    jerry_value_free (parse_options.argument_list);
  }

  jerry_value_free (parse_options.source_name);

  if (jerry_value_is_exception (snapshot_result))
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Error: Generating snapshot failed!\n");

    snapshot_result = jerry_exception_value (snapshot_result, true);

    print_unhandled_exception (snapshot_result);

    jerry_value_free (snapshot_result);
    jerry_cleanup ();
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  size_t snapshot_size = (size_t) jerry_value_as_number (snapshot_result);
  jerry_value_free (snapshot_result);

  FILE *snapshot_file_p = fopen (output_file_name_p, "wb");
  if (snapshot_file_p == NULL)
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Error: Unable to write snapshot file: '%s'\n", output_file_name_p);
    jerry_cleanup ();
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  fwrite (output_buffer, sizeof (uint8_t), snapshot_size, snapshot_file_p);
  fclose (snapshot_file_p);

  printf ("Created snapshot file: '%s' (%zu bytes)\n", output_file_name_p, snapshot_size);

  jerry_cleanup ();
  return JERRY_STANDALONE_EXIT_CODE_OK;
} /* process_generate */

/**
 * Literal dump command line option IDs
 */
typedef enum
{
  OPT_LITERAL_DUMP_HELP,
  OPT_LITERAL_DUMP_FORMAT,
  OPT_LITERAL_DUMP_OUT,
} literal_dump_opt_id_t;

/**
 * Literal dump command line options
 */
static const cli_opt_t literal_dump_opts[] = {
  CLI_OPT_DEF (.id = OPT_LITERAL_DUMP_HELP, .opt = "h", .longopt = "help", .help = "print this help and exit"),
  CLI_OPT_DEF (.id = OPT_LITERAL_DUMP_FORMAT,
               .longopt = "format",
               .meta = "[c|list]",
               .help = "specify output format (default: list)"),
  CLI_OPT_DEF (.id = OPT_LITERAL_DUMP_OUT, .opt = "o", .help = "specify output file name (default: literals.[h|list])"),
  CLI_OPT_DEF (.id = CLI_OPT_DEFAULT, .meta = "FILE(S)", .help = "input snapshot files")
};

/**
 * Process 'litdump' command.
 *
 * @return error code (0 - no error)
 */
static int
process_literal_dump (cli_state_t *cli_state_p, /**< cli state */
                      int argc, /**< number of arguments */
                      char *prog_name_p) /**< program name */
{
  uint8_t *input_pos_p = input_buffer;

  cli_change_opts (cli_state_p, literal_dump_opts);

  JERRY_VLA (const uint32_t *, snapshot_buffers, argc);
  JERRY_VLA (size_t, snapshot_buffer_sizes, argc);
  uint32_t number_of_files = 0;
  const char *literals_file_name_p = NULL;
  bool is_c_format = false;

  for (int id = cli_consume_option (cli_state_p); id != CLI_OPT_END; id = cli_consume_option (cli_state_p))
  {
    switch (id)
    {
      case OPT_LITERAL_DUMP_HELP:
      {
        cli_help (prog_name_p, "litdump", literal_dump_opts);
        return JERRY_STANDALONE_EXIT_CODE_OK;
      }
      case OPT_LITERAL_DUMP_FORMAT:
      {
        const char *fromat_str_p = cli_consume_string (cli_state_p);
        if (!strcmp ("c", fromat_str_p))
        {
          is_c_format = true;
        }
        else if (!strcmp ("list", fromat_str_p))
        {
          is_c_format = false;
        }
        else
        {
          jerry_log (JERRY_LOG_LEVEL_ERROR, "Error: Unsupported literal dump format.");
          return JERRY_STANDALONE_EXIT_CODE_FAIL;
        }
        break;
      }
      case OPT_LITERAL_DUMP_OUT:
      {
        literals_file_name_p = cli_consume_string (cli_state_p);
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

          snapshot_buffers[number_of_files] = (const uint32_t *) input_pos_p;
          snapshot_buffer_sizes[number_of_files] = size;

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

  if (number_of_files < 1)
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Error: at least one input file must be specified.\n");
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

#if defined(JERRY_EXTERNAL_CONTEXT) && (JERRY_EXTERNAL_CONTEXT == 1)
  context_init ();
#endif /* defined (JERRY_EXTERNAL_CONTEXT) && (JERRY_EXTERNAL_CONTEXT == 1) */

  jerry_init (JERRY_INIT_EMPTY);

  size_t lit_buf_sz = 0;
  if (number_of_files == 1)
  {
    lit_buf_sz = jerry_get_literals_from_snapshot (snapshot_buffers[0],
                                                   snapshot_buffer_sizes[0],
                                                   literal_buffer,
                                                   JERRY_BUFFER_SIZE,
                                                   is_c_format);
  }
  else
  {
    /* The input contains more than one input snapshot file, so we must merge them first. */
    const char *error_p = NULL;
    size_t merged_snapshot_size = jerry_merge_snapshots (snapshot_buffers,
                                                         snapshot_buffer_sizes,
                                                         number_of_files,
                                                         output_buffer,
                                                         JERRY_BUFFER_SIZE,
                                                         &error_p);

    if (merged_snapshot_size == 0)
    {
      jerry_log (JERRY_LOG_LEVEL_ERROR, "Error: %s\n", error_p);
      jerry_cleanup ();
      return JERRY_STANDALONE_EXIT_CODE_FAIL;
    }

    printf ("Successfully merged the input snapshots (%zu bytes).\n", merged_snapshot_size);

    lit_buf_sz = jerry_get_literals_from_snapshot (output_buffer,
                                                   merged_snapshot_size,
                                                   literal_buffer,
                                                   JERRY_BUFFER_SIZE,
                                                   is_c_format);
  }

  if (lit_buf_sz == 0)
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR,
               "Error: Literal saving failed! No literals were found in the input snapshot(s).\n");
    jerry_cleanup ();
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  if (literals_file_name_p == NULL)
  {
    literals_file_name_p = is_c_format ? "literals.h" : "literals.list";
  }

  FILE *file_p = fopen (literals_file_name_p, "wb");

  if (file_p == NULL)
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Error: cannot open file: '%s'\n", literals_file_name_p);
    jerry_cleanup ();
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  fwrite (literal_buffer, sizeof (uint8_t), lit_buf_sz, file_p);
  fclose (file_p);

  printf ("Literals are saved into '%s' (%zu bytes).\n", literals_file_name_p, lit_buf_sz);

  jerry_cleanup ();
  return JERRY_STANDALONE_EXIT_CODE_OK;
} /* process_literal_dump */

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
static const cli_opt_t merge_opts[] = {
  CLI_OPT_DEF (.id = OPT_MERGE_HELP, .opt = "h", .longopt = "help", .help = "print this help and exit"),
  CLI_OPT_DEF (.id = OPT_MERGE_OUT, .opt = "o", .help = "specify output file name (default: js.snapshot)"),
  CLI_OPT_DEF (.id = CLI_OPT_DEFAULT, .meta = "FILE", .help = "input snapshot files, minimum two")
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
  uint8_t *input_pos_p = input_buffer;

  cli_change_opts (cli_state_p, merge_opts);

  JERRY_VLA (const uint32_t *, merge_buffers, argc);
  JERRY_VLA (size_t, merge_buffer_sizes, argc);
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
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Error: at least two input files must be passed.\n");
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

#if defined(JERRY_EXTERNAL_CONTEXT) && (JERRY_EXTERNAL_CONTEXT == 1)
  context_init ();
#endif /* defined (JERRY_EXTERNAL_CONTEXT) && (JERRY_EXTERNAL_CONTEXT == 1) */

  jerry_init (JERRY_INIT_EMPTY);

  const char *error_p = NULL;
  size_t merged_snapshot_size = jerry_merge_snapshots (merge_buffers,
                                                       merge_buffer_sizes,
                                                       number_of_files,
                                                       output_buffer,
                                                       JERRY_BUFFER_SIZE,
                                                       &error_p);

  if (merged_snapshot_size == 0)
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Error: %s\n", error_p);
    jerry_cleanup ();
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  FILE *file_p = fopen (output_file_name_p, "wb");

  if (file_p == NULL)
  {
    jerry_log (JERRY_LOG_LEVEL_ERROR, "Error: cannot open file: '%s'\n", output_file_name_p);
    jerry_cleanup ();
    return JERRY_STANDALONE_EXIT_CODE_FAIL;
  }

  fwrite (output_buffer, 1u, merged_snapshot_size, file_p);
  fclose (file_p);

  printf ("Merge is completed. Merged snapshot is saved into '%s' (%zu bytes).\n",
          output_file_name_p,
          merged_snapshot_size);

  jerry_cleanup ();
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
static const cli_opt_t main_opts[] = {
  CLI_OPT_DEF (.id = OPT_HELP, .opt = "h", .longopt = "help", .help = "print this help and exit"),
  CLI_OPT_DEF (.id = CLI_OPT_DEFAULT, .meta = "COMMAND", .help = "specify the command")
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
          "  litdump\n"
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
  cli_state_t cli_state = cli_init (main_opts, argc, argv);

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
        else if (!strcmp ("litdump", command_p))
        {
          return process_literal_dump (&cli_state, argc, argv[0]);
        }
        else if (!strcmp ("generate", command_p))
        {
          return process_generate (&cli_state, argc, argv[0]);
        }

        jerry_log (JERRY_LOG_LEVEL_ERROR, "Error: unknown command: %s\n\n", command_p);
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
