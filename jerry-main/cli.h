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

#ifndef CLI_H
#define CLI_H

#include <string.h>

/**
 * Command line option definition.
 */
typedef struct
{
  int id; /**< unique ID of the option (CLI_OPT_DEFAULT, or anything >= 0) */
  const char *opt; /**< short option variant (in the form of "x" without dashes) */
  const char *longopt; /**< long option variant (in the form of "xxx" without dashes) */
  const char *meta; /**< name(s) of the argument(s) of the option, for display only */
  const char *help; /**< descriptive help message of the option */
} cli_opt_t;

/**
 * Special marker for default option which also marks the end of the option list.
 */
#define CLI_OPT_DEFAULT -1

/**
 * Returned by cli_consume_option () when no more options are available
 * or an error occured.
 */
#define CLI_OPT_END -2

/**
 * State of the sub-command processor.
 * No fields should be accessed other than error and arg.
 */
typedef struct
{
  /* Public fields. */
  const char *error; /**< public field for error message */
  const char *arg; /**< last processed argument as string */

  /* Private fields. */
  int argc; /**< remaining number of arguments */
  char **argv; /**< remaining arguments */
  const cli_opt_t *opts; /**< options */
} cli_state_t;

/**
 * Macro for writing command line option definition struct literals.
 */
#define CLI_OPT_DEF(...) /*(cli_opt_t)*/ { __VA_ARGS__ }

/*
 * Functions for CLI.
 */

cli_state_t cli_init (const cli_opt_t *options, int argc, char **argv);
int cli_consume_option (cli_state_t *state);
const char * cli_consume_string (cli_state_t *state);
int cli_consume_int (cli_state_t *state);
void cli_help (const char *progname, const cli_opt_t *options);

#endif /* CLI_H */
