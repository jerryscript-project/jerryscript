/* Copyright 2014 Samsung Electronics Co., Ltd.
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

#ifndef PARSE_ERROR_H
#define PARSE_ERROR_H

#define PARSE_ERROR(MESSAGE, LOCUS) do { \
  size_t line, column; \
  lexer_locus_to_line_and_column ((size_t) (LOCUS), &line, &column); \
  lexer_dump_line (line); \
  __printf ("\n"); \
  for (size_t i = 0; i < column; i++) { \
    __putchar (' '); \
  } \
  __printf ("^\n"); \
  __printf ("ERROR: Ln %d, Col %d: %s\n", line + 1, column + 1, MESSAGE); \
  __exit (-1); \
} while (0)
#define PARSE_WARN(MESSAGE, LOCUS) do { \
  size_t line, column; \
  lexer_locus_to_line_and_column ((size_t) (LOCUS), &line, &column); \
  __printf ("WARNING: Ln %d, Col %d: %s\n", line + 1, column + 1, MESSAGE); \
} while (0)
#define PARSE_ERROR_VARG(MESSAGE, LOCUS, ...) do { \
  size_t line, column; \
  lexer_locus_to_line_and_column ((size_t) (LOCUS), &line, &column); \
  lexer_dump_line (line); \
  __printf ("\n"); \
  for (size_t i = 0; i < column; i++) { \
    __putchar (' '); \
  } \
  __printf ("^\n"); \
  __printf ("ERROR: Ln %d, Col %d: ", line + 1, column + 1); \
  __printf (MESSAGE, __VA_ARGS__); \
  __printf ("\n"); \
  __exit (-1); \
} while (0)
#define PARSE_SORRY(MESSAGE, LOCUS) do { \
  size_t line, column; \
  lexer_locus_to_line_and_column ((size_t) (LOCUS), &line, &column); \
  lexer_dump_line (line); \
  __printf ("\n"); \
  for (size_t i = 0; i < column; i++) { \
    __putchar (' '); \
  } \
    __printf ("^\n"); \
  __printf ("SORRY, Unimplemented: Ln %d, Col %d: %s\n", line + 1, column + 1, MESSAGE); \
  __exit (-1); \
} while (0)

#endif /* PARSE_ERROR_H */
