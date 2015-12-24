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

#ifndef JSP_EARLY_ERROR_H
#define JSP_EARLY_ERROR_H

#include "jrt-libc-includes.h"
#include "lexer.h"
#include "opcodes-dumper.h"

#ifndef JERRY_NDEBUG
#define PARSE_ERROR_PRINT_PLACE(TYPE, LOCUS) do { \
  size_t line, column; \
  lexer_locus_to_line_and_column ((LOCUS), &line, &column); \
  lexer_dump_line (line); \
  printf ("\n"); \
  for (size_t i = 0; i < column; i++) { \
    jerry_port_putchar (' '); \
  } \
  printf ("^\n"); \
  printf ("%s: Ln %lu, Col %lu: ", TYPE, (unsigned long) (line + 1), (unsigned long) (column + 1)); \
} while (0)
#define PARSE_ERROR(type, MESSAGE, LOCUS) do { \
  locus __loc = LOCUS; \
  PARSE_ERROR_PRINT_PLACE ("ERROR", __loc); \
  printf ("%s\n", MESSAGE); \
  jsp_early_error_raise_error (type); \
} while (0)
#define PARSE_ERROR_VARG(type, MESSAGE, LOCUS, ...) do { \
  locus __loc = LOCUS; \
  PARSE_ERROR_PRINT_PLACE ("ERROR", __loc); \
  printf (MESSAGE, __VA_ARGS__); \
  printf ("\n"); \
  jsp_early_error_raise_error (type); \
} while (0)
#else /* JERRY_NDEBUG */
#define PARSE_ERROR(type, MESSAGE, LOCUS) do { \
  locus __attr_unused___ unused_value = LOCUS; jsp_early_error_raise_error (type); \
} while (0)
#define PARSE_ERROR_VARG(type, MESSAGE, LOCUS, ...) do { \
  locus __attr_unused___ unused_value = LOCUS; jsp_early_error_raise_error (type); \
} while (0)
#endif /* JERRY_NDEBUG */

typedef enum __attr_packed___
{
  PROP_DATA,
  PROP_SET,
  PROP_GET
} prop_type;

/**
 * Early error types (ECMA-262 v5, 16)
 */
typedef enum
{
  JSP_EARLY_ERROR__NO_ERROR, /** initializer value (indicates that no error occured) */
  JSP_EARLY_ERROR_SYNTAX, /**< SyntaxError */
  JSP_EARLY_ERROR_REFERENCE /**< ReferenceError */
} jsp_early_error_t;

void jsp_early_error_init (void);
void jsp_early_error_free (void);

void jsp_early_error_start_checking_of_prop_names (void);
void jsp_early_error_add_prop_name (jsp_operand_t, prop_type);
void jsp_early_error_check_for_duplication_of_prop_names (bool, locus);

void jsp_early_error_emit_error_on_eval_and_arguments (lit_literal_t, locus);
void jsp_early_error_check_for_eval_and_arguments_in_strict_mode (jsp_operand_t, bool, locus);

void jsp_early_error_check_delete (bool, locus);

jmp_buf *jsp_early_error_get_early_error_longjmp_label (void);
void __attribute__((noreturn)) jsp_early_error_raise_error (jsp_early_error_t);
jsp_early_error_t jsp_early_error_get_type (void);

#endif /* JSP_EARLY_ERROR_H */
