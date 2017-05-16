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

#include "cli.h"

/*
 * Fixed layout settings
 */

/**
 * Wrap lines at:
 */
#define CLI_LINE_LENGTH 80

/**
 * Indent various lines with:
 */
#define CLI_LINE_INDENT 2

/**
 * Tab stop (for multi-column display) at:
 */
#define CLI_LINE_TAB 24

/*
 * Print helper functions
 */

/**
 * Pad with spaces.
 */
static void
cli_print_pad (int cnt) /**< number of spaces to print */
{
  for (int i = 0; i < cnt; i++)
  {
    printf (" ");
  }
} /* cli_print_pad */

/**
 * Print the prefix of a string.
 */
static void
cli_print_prefix (const char *str, /**< string to print */
                  int len) /**< length of the prefix to print */
{
  for (int i = 0; i < len; i++)
  {
    printf ("%c", *str++);
  }
} /* cli_print_prefix */

/**
 * Print a help message wrapped into the second column.
 */
static void
cli_print_help (const char *help) /**< the help message to print */
{
  while (help != NULL && *help != 0)
  {
    int length = -1;
    int i = 0;
    for (; i < CLI_LINE_LENGTH - CLI_LINE_TAB && help[i] != 0; i++)
    {
      if (help[i] == ' ')
      {
        length = i;
      }
    }
    if (length < 0 || i < CLI_LINE_LENGTH - CLI_LINE_TAB)
    {
      length = i;
    }

    cli_print_prefix (help, length);

    help += length;
    while (*help == ' ' && *help != 0)
    {
      help++;
    }

    if (*help != 0)
    {
      printf ("\n");
      cli_print_pad (CLI_LINE_TAB);
    }
  }
} /* cli_print_help */

/*
 * Command line option handling
 */

/**
 * Initialize a command line option processor.
 *
 * @return the state that should be passed iteratively to cli_opt_process.
 */
cli_opt_state_t
cli_opt_init (const cli_opt_t *opts, /**< array of option definitions, terminated by CLI_OPT_END */
              int argc, /**< number of command line arguments */
              char **argv) /**< array of command line arguments */
{
  return (cli_opt_state_t)
  {
    .opts = opts,
    .argc = argc,
    .argv = argv,
    .opt = NULL,
    .arg = NULL
  };
} /* cli_opt_init */

/**
 * Perform one step of the command line option processor.
 *
 * @return the ID of the option that was found. (The definition of the found
 *         option (if any) is available via state->opt, while the option
 *         string and its arguments are available via state->arg[0..].)
 *         CLI_OPT_END signals that all command line arguments are consumed.
 */
int
cli_opt_process (cli_opt_state_t *state) /**< state of the command line option processor */
{
  if (state->argc <= 0)
  {
    state->opt = NULL;
    state->arg = NULL;
    return CLI_OPT_END;
  }

  state->arg = state->argv;

  for (const cli_opt_t *o = state->opts; o->id != CLI_OPT_END; o++)
  {
    state->opt = o;

    if (o->id == CLI_OPT_POSITIONAL && (state->arg[0][0] != '-' || !strcmp (state->arg[0], "-")))
    {
      state->argc--;
      state->argv++;
      return CLI_OPT_POSITIONAL;
    }
    else if ((o->opt != NULL && !strcmp (o->opt, state->arg[0]))
        || (o->longopt != NULL && !strcmp (o->longopt, state->arg[0])))
    {
      if (state->argc > o->argc)
      {
        state->argc -= 1 + o->argc;
        state->argv += 1 + o->argc;
        return o->id;
      }
      else
      {
        state->argv += state->argc;
        state->argc = 0;
        return CLI_OPT_INCOMPLETE;
      }
    }
  }

  state->opt = NULL;
  state->argc--;
  state->argv++;
  return CLI_OPT_UNKNOWN;
} /* cli_opt_process */

/**
 * Print usage summary of options.
 */
void
cli_opt_usage (const char *progname, /**< program name, typically argv[0] */
               const cli_opt_t *opts) /**< array of command line option definitions, terminated by CLI_OPT_END */
{
  int length = (int) strlen (progname);
  printf ("%s", progname);

  for (const cli_opt_t *o = opts; o->id != CLI_OPT_END; o++)
  {
    const char *opt = o->opt != NULL ? o->opt : o->longopt;
    opt = opt != NULL ? opt : o->meta;

    int opt_length = (int) strlen (opt);
    if (o->argc > 0)
    {
      opt_length += o->meta != NULL ? 1 + (int) strlen (o->meta) : o->argc * 2;
    }
    opt_length += o->quant == CLI_QUANT_Q || o->quant == CLI_QUANT_A ? 2 : 0;
    opt_length += o->quant == CLI_QUANT_P || o->quant == CLI_QUANT_A ? 3 : 0;

    if (length + 1 + opt_length >= CLI_LINE_LENGTH)
    {
      length = CLI_LINE_INDENT - 1;
      printf ("\n");
      cli_print_pad (length);
    }
    length += opt_length;

    printf (" ");

    if (o->quant == CLI_QUANT_Q || o->quant == CLI_QUANT_A)
    {
      printf ("[");
    }

    printf ("%s", opt);

    if (o->argc > 0)
    {
      if (o->meta != NULL)
      {
        printf (" %s", o->meta);
      }
      else
      {
        for (int i = 0; i < o->argc; i++)
        {
          printf (" _");
        }
      }
    }

    if (o->quant == CLI_QUANT_Q || o->quant == CLI_QUANT_A)
    {
      printf ("]");
    }

    if (o->quant == CLI_QUANT_P || o->quant == CLI_QUANT_A)
    {
      printf ("...");
    }
  }

  printf ("\n");
} /* cli_opt_usage */

/**
 * Print detailed help for options.
 */
void
cli_opt_help (const cli_opt_t *opts) /**< array of command line option definitions, terminated by CLI_OPT_END */
{
  for (const cli_opt_t *o = opts; o->id != CLI_OPT_END; o++)
  {
    int length = CLI_LINE_INDENT;
    cli_print_pad (length);

    if (o->opt != NULL)
    {
      printf ("%s", o->opt);
      length += (int) strlen (o->opt);
    }

    if (o->opt != NULL && o->longopt != NULL)
    {
      printf (", ");
      length += 2;
    }

    if (o->longopt != NULL)
    {
      printf ("%s", o->longopt);
      length += (int) strlen (o->longopt);
    }

    if (o->opt == NULL && o->longopt == NULL)
    {
      printf ("%s", o->meta);
      length += (int) strlen (o->meta);
    }
    else if (o->argc > 0)
    {
      if (o->meta != NULL)
      {
        printf (" %s", o->meta);
        length += 1 + (int) strlen (o->meta);
      }
      else
      {
        for (int i = 0; i < o->argc; i++)
        {
          printf (" _");
        }
        length += o->argc * 2;
      }
    }

    if (o->help != NULL)
    {
      if (length >= CLI_LINE_TAB)
      {
        printf ("\n");
        length = 0;
      }
      cli_print_pad (CLI_LINE_TAB - length);
      length = CLI_LINE_TAB;

      cli_print_help (o->help);
    }

    printf ("\n");
  }
} /* cli_opt_help */

/*
 * Sub-command handling
 */

/**
 * Initialize a sub-command processor.
 *
 * @return the state that should be passed to cli_cmd_process.
 */
cli_cmd_state_t
cli_cmd_init (const cli_cmd_t *cmds, /**< array of sub-command definitions, terminated by CLI_CMD_END */
              int argc, /**< number of command line arguments */
              char **argv) /**< array of command line arguments */
{
  return (cli_cmd_state_t)
  {
    .cmds = cmds,
    .argc = argc,
    .argv = argv,
    .cmd = NULL,
    .arg = NULL
  };
} /* cli_cmd_init */

/**
 * Process first element of the command line and determine whether it is a
 * defined sub-command or not.
 *
 * @return the ID of the sub-command that was found.
 */
int
cli_cmd_process (cli_cmd_state_t *state) /**< state of the sub-command processor */
{
  if (state->argc <= 0 || state->argv[0][0] == '-')
  {
    state->cmd = NULL;
    state->arg = NULL;
    return CLI_CMD_NONE;
  }

  state->arg = state->argv;

  for (const cli_cmd_t *c = state->cmds; c->id != CLI_CMD_END; c++)
  {
    state->cmd = c;

    if (!strcmp (c->cmd, state->argv[0]))
    {
      state->argc--;
      state->argv++;
      state->cmd = c;
      return c->id;
    }
  }

  state->cmd = NULL;
  state->argc--;
  state->argv++;
  return CLI_CMD_UNKNOWN;
} /* cli_cmd_process */

/**
 * Print usage summary of all sub-commands.
 */
void
cli_cmd_usage (const char *progname, /**< program name, typically argv[0] */
               const cli_cmd_t *cmds) /**< array of sub-command definitions, terminated by CLI_CMD_END */
{
  for (const cli_cmd_t *c = cmds; c->id != CLI_CMD_END; c++)
  {
    if (c->cmd != NULL)
    {
      CLI_CMD_NAME (cmdname, progname, c->cmd);
      cli_opt_usage (cmdname, c->opts);
    }
  }
} /* cli_cmd_usage */

/**
 * Print help of all sub-commands.
 */
void
cli_cmd_help (const cli_cmd_t *cmds) /**< array of sub-command definitions, terminated by CLI_CMD_END */
{
  for (const cli_cmd_t *c = cmds; c->id != CLI_CMD_END; c++)
  {
    int length = CLI_LINE_INDENT;
    cli_print_pad (length);

    printf ("%s", c->cmd);
    length += (int) strlen (c->cmd);

    if (c->help != NULL)
    {
      if (length >= CLI_LINE_TAB)
      {
        printf ("\n");
        length = 0;
      }
      cli_print_pad (CLI_LINE_TAB - length);
      length = CLI_LINE_TAB;

      cli_print_help (c->help);
    }

    printf ("\n");
  }
} /* cli_cmd_help */
