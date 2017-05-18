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

/*
 * Types for CLI
 */

/**
 * Command line option definition
 */
typedef struct
{
  int id; /**< unique ID of the option (CLI_OPT_END, CLI_OPT_POSITIONAL, or anything >= 0) */
  const char *opt; /**< short option variant (in the form of "-x") */
  const char *longopt; /**< long option variant (in the form of "--xxx") */
  int argc; /**< number of arguments of the option */
  const char *meta; /**< name(s) of the argument(s) of the option, for display only */
  int quant; /**< quantifier of the option (CLI_QUANT_{Q,A,P,1} for ?,*,+,1), for display only */
  const char *help; /**< descriptive help message of the option */
} cli_opt_t;

/**
 * Quantifiers of command line options
 */
typedef enum
{
  CLI_QUANT_Q, /**< ? (Question mark: optional, zero or one) */
  CLI_QUANT_A, /**< * (Asterisk, star: zero or one or more) */
  CLI_QUANT_P, /**< + (Plus sign: one or more) */
  CLI_QUANT_1  /**< 1 (One: one) */
} cli_opt_quant_t;

/**
 * Common command line option IDs
 */
typedef enum
{
  CLI_OPT_POSITIONAL = -1, /**< positional "option" */
  CLI_OPT_END = -2, /**< end of options marker */
  CLI_OPT_INCOMPLETE = -3, /**< incomplete option (too few arguments) */
  CLI_OPT_UNKNOWN = -4 /**< unknown option */
} cli_opt_id_t;

/**
 * State of the command line option processor.
 * No fields should be accessed other than arg and opt.
 */
typedef struct
{
  const cli_opt_t *opts;
  int argc;
  char **argv;
  const cli_opt_t *opt; /**< found option or NULL */
  char **arg; /**< array of strings for the last processed option */
} cli_opt_state_t;

/**
 * Sub-command definition
 */
typedef struct
{
  int id;  /**< unique ID of the sub-command (CLI_CMD_END, or anything >= 0) */
  const char *cmd; /**< sub-command name (in the form of "xxx") */
  const cli_opt_t *opts;  /**< array of associated command line option definitions, for display only */
  const char *help; /**< descriptive help message of the sub-command */
} cli_cmd_t;

/**
 * Common sub-command IDs
 */
typedef enum
{
  CLI_CMD_END = -1, /**< end of sub-commands marker */
  CLI_CMD_NONE = -1, /**< no sub-command */
  CLI_CMD_UNKNOWN = -2 /**< unknown sub-command */
} cli_cmd_id_t;

/**
 * State of the sub-command processor.
 * No fields should be accessed other than arg and cmd.
 */
typedef struct
{
  const cli_cmd_t *cmds;
  int argc;
  char **argv;
  const cli_cmd_t *cmd; /**< found command or NULL */
  char **arg; /**< array of strings for the processed command */
} cli_cmd_state_t;

/*
 * Functions for CLI
 */

cli_opt_state_t cli_opt_init (const cli_opt_t *opts, int argc, char **argv);
int cli_opt_process (cli_opt_state_t *state);
void cli_opt_usage (const char *progname, const cli_opt_t *opts);
void cli_opt_help (const cli_opt_t *opts);

cli_cmd_state_t cli_cmd_init (const cli_cmd_t *cmds, int argc, char **argv);
int cli_cmd_process (cli_cmd_state_t *state);
void cli_cmd_help (const cli_cmd_t *cmds);
void cli_cmd_usage (const char *progname, const cli_cmd_t *cmds);

/*
 * Useful macros for CLI
 */

/**
 * Macro for writing command line option definition struct literals
 */
#define CLI_OPT_DEF(...) /*(cli_opt_t)*/ { __VA_ARGS__ }

/**
 * Macro for writing sub-command definition struct literals
 */
#define CLI_CMD_DEF(...) /*(cli_cmd_t)*/ { __VA_ARGS__ }

/**
 * Declare a char VLA and concatenate a program name and a sub-command name
 * (separated by a single space) into the new array. Useful for printing command
 * line option usage summary for sub-commands.
 *
 * @param CMDNAME name of the new array variable.
 * @param PROGNAME string containing the name of the program.
 * @param CMD string continaing the name of the sub-command.
 */
#define CLI_CMD_NAME(CMDNAME, PROGNAME, CMD) \
  char CMDNAME[strlen ((PROGNAME)) + strlen ((CMD)) + 2]; \
  strncpy (CMDNAME, (PROGNAME), strlen ((PROGNAME))); \
  CMDNAME[strlen ((PROGNAME))] = ' '; \
  strncpy (CMDNAME + strlen ((PROGNAME)) + 1, (CMD), strlen ((CMD)) + 1)

#endif /* CLI_H */
