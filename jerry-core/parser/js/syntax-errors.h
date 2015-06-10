/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#ifndef SYNTAX_ERRORS_H
#define SYNTAX_ERRORS_H

#include "opcodes-dumper.h"
#include "lexer.h"

#ifndef JERRY_NDEBUG
#define PARSE_ERROR(MESSAGE, LOCUS) do { \
  size_t line, column; \
  lexer_locus_to_line_and_column ((locus) (LOCUS), &line, &column); \
  lexer_dump_line (line); \
  printf ("\n"); \
  for (size_t i = 0; i < column; i++) { \
    putchar (' '); \
  } \
  printf ("^\n"); \
  printf ("ERROR: Ln %lu, Col %lu: %s\n", (unsigned long) (line + 1), (unsigned long) (column + 1), MESSAGE); \
  jerry_fatal (ERR_PARSER); \
} while (0)
#define PARSE_WARN(MESSAGE, LOCUS) do { \
  size_t line, column; \
  lexer_locus_to_line_and_column ((locus) (LOCUS), &line, &column); \
  printf ("WARNING: Ln %lu, Col %lu: %s\n", (unsigned long) (line + 1), (unsigned long) (column + 1), MESSAGE); \
} while (0)
#define PARSE_ERROR_VARG(MESSAGE, LOCUS, ...) do { \
  size_t line, column; \
  lexer_locus_to_line_and_column ((locus) (LOCUS), &line, &column); \
  lexer_dump_line (line); \
  printf ("\n"); \
  for (size_t i = 0; i < column; i++) { \
    putchar (' '); \
  } \
  printf ("^\n"); \
  printf ("ERROR: Ln %lu, Col %lu: ", (unsigned long) (line + 1), (unsigned long) (column + 1)); \
  printf (MESSAGE, __VA_ARGS__); \
  printf ("\n"); \
  jerry_fatal (ERR_PARSER); \
} while (0)
#define PARSE_SORRY(MESSAGE, LOCUS) do { \
  size_t line, column; \
  lexer_locus_to_line_and_column ((locus) (LOCUS), &line, &column); \
  lexer_dump_line (line); \
  printf ("\n"); \
  for (size_t i = 0; i < column; i++) { \
    putchar (' '); \
  } \
    printf ("^\n"); \
  printf ("SORRY, Unimplemented: Ln %lu, Col %lu: %s\n", \
          (unsigned long) (line + 1), (unsigned long) (column + 1), MESSAGE); \
  JERRY_UNIMPLEMENTED ("Unimplemented parser feature."); \
} while (0)
#else /* JERRY_NDEBUG */
#define PARSE_ERROR(MESSAGE, LOCUS) do { \
  jerry_fatal (ERR_PARSER); \
} while (0)
#define PARSE_WARN(MESSAGE, LOCUS) do { \
} while (0)
#define PARSE_ERROR_VARG(MESSAGE, LOCUS, ...) do { \
  jerry_fatal (ERR_PARSER); \
} while (0)
#define PARSE_SORRY(MESSAGE, LOCUS) do { \
  JERRY_UNIMPLEMENTED ("Unimplemented parser feature."); \
} while (0)
#endif /* JERRY_NDEBUG */

typedef enum __attr_packed___
{
  PROP_DATA,
  PROP_SET,
  PROP_GET,
  VARG
} prop_type;

void syntax_init (void);
void syntax_free (void);

void syntax_start_checking_of_prop_names (void);
void syntax_add_prop_name (operand, prop_type);
void syntax_check_for_duplication_of_prop_names (bool, locus);

void syntax_start_checking_of_vargs (void);
void syntax_add_varg (operand);
void syntax_check_for_eval_and_arguments_in_strict_mode (operand, bool, locus);
void syntax_check_for_syntax_errors_in_formal_param_list (bool, locus);

void syntax_check_delete (bool, locus);

#endif /* SYNTAX_ERRORS_H */
