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

#ifndef JS_SCANNER_INTERNAL_H
#define JS_SCANNER_INTERNAL_H

/* \addtogroup parser Parser
 * @{
 *
 * \addtogroup jsparser JavaScript
 * @{
 *
 * \addtogroup jsparser_scanner Scanner
 * @{
 */

/**
 * Generic descriptor which stores only the start position.
 */
typedef struct
{
  const uint8_t *source_p; /**< start source byte */
} scanner_source_start_t;

/**
 * Flags for type member of lexer_lit_location_t structure in the literal pool.
 */
typedef enum
{
  SCANNER_LITERAL_IS_ARG = (1 << 0), /**< literal is argument */
  SCANNER_LITERAL_IS_LOCAL = (1 << 1), /**< literal is local (similar to let,
                                        *   but var is allowed with the same identifier) */
  SCANNER_LITERAL_IS_VAR = (1 << 2), /**< literal is var */
  SCANNER_LITERAL_IS_FUNC = (1 << 3), /**< literal is function */
  SCANNER_LITERAL_NO_REG = (1 << 4), /**< literal cannot be stored in register */
} scanner_literal_type_flags_t;

/**
 * For statement descriptor.
 */
typedef struct
{
  /** shared fields of for statements */
  union
  {
    const uint8_t *source_p; /**< start source byte */
    scanner_for_info_t *for_info_p; /**< for info */
  } u;
} scanner_for_statement_t;

/**
 * Switch statement descriptor.
 */
typedef struct
{
  scanner_case_info_t **last_case_p; /**< last case info */
} scanner_switch_statement_t;

/**
 * Flags for scanner_literal_pool_t structure.
 */
typedef enum
{
  SCANNER_LITERAL_POOL_FUNCTION = (1 << 0), /**< literal pool represents a function */
  SCANNER_LITERAL_POOL_BLOCK = (1 << 1), /**< literal pool represents a code block */
  SCANNER_LITERAL_POOL_NO_REG = (1 << 2), /**< variable declarations cannot be kept in registers */
  SCANNER_LITERAL_POOL_NO_ARGUMENTS = (1 << 3), /**< arguments object should not be constructed */
  SCANNER_LITERAL_POOL_IN_WITH = (1 << 4), /**< literal pool is in a with statement */
} scanner_literal_pool_flags_t;

/**
 * Define a function where no arguments are allowed.
 */
#define SCANNER_LITERAL_POOL_FUNCTION_WITHOUT_ARGUMENTS \
  (SCANNER_LITERAL_POOL_FUNCTION | SCANNER_LITERAL_POOL_NO_ARGUMENTS)

/**
 * Local literal pool.
 */
typedef struct scanner_literal_pool_t
{
  struct scanner_literal_pool_t *prev_p; /**< previous literal pool */
  const uint8_t *source_p; /**< source position where the final data needs to be inserted */
  parser_list_t literal_pool; /**< list of literal */
  uint16_t status_flags; /**< combination of scanner_literal_pool_flags_t flags */
  uint16_t no_declarations; /**< size of scope stack required during parsing */
} scanner_literal_pool_t;

/**
 * Scanner context.
 */
struct scanner_context_t
{
  uint8_t mode; /**< scanner mode */
#if ENABLED (JERRY_DEBUGGER)
  uint8_t debugger_enabled; /**< debugger is enabled */
#endif /* ENABLED (JERRY_DEBUGGER) */
  scanner_literal_pool_t *active_literal_pool_p; /**< currently active literal pool */
  scanner_switch_statement_t active_switch_statement; /**< currently active switch statement */
  scanner_info_t *end_arguments_p; /**< position of end arguments */
};

size_t scanner_get_stream_size (scanner_info_t *info_p, size_t size);
scanner_info_t *scanner_insert_info (parser_context_t *context_p, const uint8_t *source_p, size_t size);
scanner_info_t *scanner_insert_info_before (parser_context_t *context_p, const uint8_t *source_p,
                                            scanner_info_t *start_info_p, size_t size);
scanner_literal_pool_t *scanner_push_literal_pool (parser_context_t *context_p, scanner_context_t *scanner_context_p,
                                                   uint16_t status_flags);
void scanner_pop_literal_pool (parser_context_t *context_p, scanner_context_t *scanner_context_p);
void scanner_filter_arguments (parser_context_t *context_p, scanner_context_t *scanner_context_p);
lexer_lit_location_t *scanner_add_custom_literal (parser_context_t *context_p, scanner_literal_pool_t *literal_pool_p,
                                                  const lexer_lit_location_t *literal_location_p);
lexer_lit_location_t *scanner_add_literal (parser_context_t *context_p, scanner_context_t *scanner_context_p);
void scanner_add_reference (parser_context_t *context_p, scanner_context_t *scanner_context_p);
void scanner_append_argument (parser_context_t *context_p, scanner_context_t *scanner_context_p);
void scanner_detect_eval_call (parser_context_t *context_p, scanner_context_t *scanner_context_p);

/**
 * @}
 * @}
 * @}
 */

#endif /* !JS_SCANNER_INTERNAL_H */
