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
 * Descriptor for storing a binding literal on stack.
 */
typedef struct
{
  lexer_lit_location_t *literal_p; /**< binding literal */
} scanner_binding_literal_t;

/**
 * Flags for type member of lexer_lit_location_t structure in the literal pool.
 */
typedef enum
{
  SCANNER_LITERAL_IS_ARG = (1 << 0), /**< literal is argument */
  SCANNER_LITERAL_IS_VAR = (1 << 1), /**< literal is var */
#if ENABLED (JERRY_ES2015)
  /** literal is a destructured argument binding of a possible arrow function */
  SCANNER_LITERAL_IS_ARROW_DESTRUCTURED_ARG = SCANNER_LITERAL_IS_VAR,
#endif /* ENABLED (JERRY_ES2015) */
  SCANNER_LITERAL_IS_FUNC = (1 << 2), /**< literal is function */
#if ENABLED (JERRY_ES2015)
  /** a destructured argument binding of a possible arrow function cannot be stored in a register */
  SCANNER_LITERAL_ARROW_DESTRUCTURED_ARG_NO_REG = SCANNER_LITERAL_IS_FUNC,
#endif /* ENABLED (JERRY_ES2015) */
  SCANNER_LITERAL_NO_REG = (1 << 3), /**< literal cannot be stored in a register */
  SCANNER_LITERAL_IS_LET = (1 << 4), /**< literal is let */
#if ENABLED (JERRY_ES2015)
  /** literal is a function declared in this block (prevents declaring let/const with the same name) */
  SCANNER_LITERAL_IS_FUNC_DECLARATION = SCANNER_LITERAL_IS_LET,
#endif /* ENABLED (JERRY_ES2015) */
  SCANNER_LITERAL_IS_CONST = (1 << 5), /**< literal is const */
#if ENABLED (JERRY_ES2015)
  /** literal is a destructured argument binding */
  SCANNER_LITERAL_IS_DESTRUCTURED_ARG = SCANNER_LITERAL_IS_CONST,
  /** literal is a local function */
  SCANNER_LITERAL_IS_FUNC_LOCAL = SCANNER_LITERAL_IS_CONST,
  SCANNER_LITERAL_IS_USED = (1 << 6), /**< literal is used */
#endif /* ENABLED (JERRY_ES2015) */
} scanner_literal_type_flags_t;

/*
 * Known combinations:
 *
 *  SCANNER_LITERAL_IS_FUNC | SCANNER_LITERAL_IS_FUNC_DECLARATION :
 *         function declared in this block, might be let or var
 *  SCANNER_LITERAL_IS_FUNC | SCANNER_LITERAL_IS_FUNC_LOCAL :
 *         function is visible only in this block
 *  SCANNER_LITERAL_IS_LOCAL :
 *         module import on global scope, catch block variable otherwise
 * SCANNER_LITERAL_IS_ARG | SCANNER_LITERAL_IS_FUNC :
 *         a function argument which is reassigned to a function later
 *  SCANNER_LITERAL_IS_ARG | SCANNER_LITERAL_IS_DESTRUCTURED_ARG :
 *         destructured binding argument
 *  SCANNER_LITERAL_IS_ARG | SCANNER_LITERAL_IS_DESTRUCTURED_ARG | SCANNER_LITERAL_IS_FUNC :
 *         destructured binding argument which is reassigned to a function later
 */

/**
 * Literal is a local declration (let, const, catch variable, etc.)
 */
#define SCANNER_LITERAL_IS_LOCAL (SCANNER_LITERAL_IS_LET | SCANNER_LITERAL_IS_CONST)

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

#if ENABLED (JERRY_ES2015)

/**
 * Types of scanner destructuring bindings.
 */
typedef enum
{
  /* Update SCANNER_NEEDS_BINDING_LIST after changing these values. */
  SCANNER_BINDING_NONE, /**< not a destructuring binding expression */
  SCANNER_BINDING_VAR, /**< destructuring var binding */
  SCANNER_BINDING_LET, /**< destructuring let binding */
  SCANNER_BINDING_CONST, /**< destructuring const binding */
  SCANNER_BINDING_ARG, /**< destructuring arg binding */
  SCANNER_BINDING_ARROW_ARG, /**< possible destructuring arg binding of an arrow function */
  SCANNER_BINDING_CATCH, /**< destructuring catch binding */
} scanner_binding_type_t;

/**
 * Check whether a binding list is needed for the binding pattern.
 */
#define SCANNER_NEEDS_BINDING_LIST(type) ((type) >= SCANNER_BINDING_LET)

/**
 * Scanner binding items for destructuring binding patterns.
 */
typedef struct scanner_binding_item_t
{
  struct scanner_binding_item_t *next_p; /**< next binding in the list */
  lexer_lit_location_t *literal_p; /**< binding literal */
} scanner_binding_item_t;

/**
 * Scanner binding lists for destructuring binding patterns.
 */
typedef struct scanner_binding_list_t
{
  struct scanner_binding_list_t *prev_p; /**< prev list */
  scanner_binding_item_t *items_p; /**< list of bindings */
  bool is_nested; /**< is nested binding declaration */
} scanner_binding_list_t;

#endif /* ENABLED (JERRY_ES2015) */

/**
 * Flags for scanner_literal_pool_t structure.
 */
typedef enum
{
  SCANNER_LITERAL_POOL_FUNCTION = (1 << 0), /**< literal pool represents a function */
  SCANNER_LITERAL_POOL_BLOCK = (1 << 1), /**< literal pool represents a code block */
  SCANNER_LITERAL_POOL_NO_REG = (1 << 2), /**< variable declarations cannot be kept in registers */
#if ENABLED (JERRY_ES2015)
  SCANNER_LITERAL_POOL_NO_VAR_REG = (1 << 3), /**< non let/const declarations cannot be kept in registers */
#endif /* ENABLED (JERRY_ES2015) */
  SCANNER_LITERAL_POOL_NO_ARGUMENTS = (1 << 4), /**< arguments object should not be constructed */
  SCANNER_LITERAL_POOL_IN_WITH = (1 << 5), /**< literal pool is in a with statement */
#if ENABLED (JERRY_ES2015_MODULE_SYSTEM)
  SCANNER_LITERAL_POOL_IN_EXPORT = (1 << 6), /**< the declared variables are exported by the module system */
#endif /* ENABLED (JERRY_ES2015_MODULE_SYSTEM) */
#if ENABLED (JERRY_ES2015)
  SCANNER_LITERAL_POOL_GENERATOR = (1 << 7), /**< generator function */
#endif /* ENABLED (JERRY_ES2015) */
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
#if ENABLED (JERRY_ES2015)
  uint8_t binding_type; /**< current destructuring binding type */
  scanner_binding_list_t *active_binding_list_p; /**< currently active binding list */
#endif /* ENABLED (JERRY_ES2015) */
  scanner_literal_pool_t *active_literal_pool_p; /**< currently active literal pool */
  scanner_switch_statement_t active_switch_statement; /**< currently active switch statement */
  scanner_info_t *end_arguments_p; /**< position of end arguments */
};

void scanner_raise_error (parser_context_t *context_p);
#if ENABLED (JERRY_ES2015)
void scanner_raise_redeclaration_error (parser_context_t *context_p);
#endif /* ENABLED (JERRY_ES2015) */

void *scanner_malloc (parser_context_t *context_p, size_t size);
void scanner_free (void *ptr, size_t size);

size_t scanner_get_stream_size (scanner_info_t *info_p, size_t size);
scanner_info_t *scanner_insert_info (parser_context_t *context_p, const uint8_t *source_p, size_t size);
scanner_info_t *scanner_insert_info_before (parser_context_t *context_p, const uint8_t *source_p,
                                            scanner_info_t *start_info_p, size_t size);
scanner_literal_pool_t *scanner_push_literal_pool (parser_context_t *context_p, scanner_context_t *scanner_context_p,
                                                   uint16_t status_flags);
void scanner_pop_literal_pool (parser_context_t *context_p, scanner_context_t *scanner_context_p);
#if ENABLED (JERRY_ES2015)
void scanner_construct_global_block (parser_context_t *context_p, scanner_context_t *scanner_context_p);
#endif /* ENABLED (JERRY_ES2015) */
void scanner_filter_arguments (parser_context_t *context_p, scanner_context_t *scanner_context_p);
lexer_lit_location_t *scanner_add_custom_literal (parser_context_t *context_p, scanner_literal_pool_t *literal_pool_p,
                                                  const lexer_lit_location_t *literal_location_p);
lexer_lit_location_t *scanner_add_literal (parser_context_t *context_p, scanner_context_t *scanner_context_p);
void scanner_add_reference (parser_context_t *context_p, scanner_context_t *scanner_context_p);
void scanner_append_argument (parser_context_t *context_p, scanner_context_t *scanner_context_p);
#if ENABLED (JERRY_ES2015)
bool scanner_scope_find_let_declaration (parser_context_t *context_p, lexer_lit_location_t *literal_p);
void scanner_detect_invalid_var (parser_context_t *context_p, scanner_context_t *scanner_context_p,
                                 lexer_lit_location_t *var_literal_p);
void scanner_detect_invalid_let (parser_context_t *context_p, lexer_lit_location_t *let_literal_p);
#endif /* ENABLED (JERRY_ES2015) */
void scanner_detect_eval_call (parser_context_t *context_p, scanner_context_t *scanner_context_p);

#if ENABLED (JERRY_ES2015)
void scanner_push_destructuring_pattern (parser_context_t *context_p, scanner_context_t *scanner_context_p,
                                         uint8_t binding_type, bool is_nested);
void scanner_pop_binding_list (scanner_context_t *scanner_context_p);
void scanner_append_hole (parser_context_t *context_p, scanner_context_t *scanner_context_p);
#endif /* ENABLED (JERRY_ES2015) */

/**
 * @}
 * @}
 * @}
 */

#endif /* !JS_SCANNER_INTERNAL_H */
