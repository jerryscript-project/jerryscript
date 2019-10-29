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

#ifndef JS_PARSER_H
#define JS_PARSER_H

#include "ecma-globals.h"

/** \addtogroup parser Parser
 * @{
 *
 * \addtogroup jsparser JavaScript
 * @{
 *
 * \addtogroup jsparser_parser Parser
 * @{
 */

/**
 * Error codes.
 */
typedef enum
{
  PARSER_ERR_NO_ERROR,                                /**< no error */

  PARSER_ERR_OUT_OF_MEMORY,                           /**< out of memory */
  PARSER_ERR_LITERAL_LIMIT_REACHED,                   /**< maximum number of literals reached */
  PARSER_ERR_SCOPE_STACK_LIMIT_REACHED,               /**< maximum depth of scope stack reached */
  PARSER_ERR_ARGUMENT_LIMIT_REACHED,                  /**< maximum number of function arguments reached */
  PARSER_ERR_STACK_LIMIT_REACHED,                     /**< maximum function stack size reached */

  PARSER_ERR_INVALID_CHARACTER,                       /**< unexpected character */
  PARSER_ERR_INVALID_HEX_DIGIT,                       /**< invalid hexadecimal digit */
  PARSER_ERR_INVALID_ESCAPE_SEQUENCE,                 /**< invalid escape sequence */
  PARSER_ERR_INVALID_UNICODE_ESCAPE_SEQUENCE,         /**< invalid unicode escape sequence */
  PARSER_ERR_INVALID_IDENTIFIER_START,                /**< character cannot be start of an identifier */
  PARSER_ERR_INVALID_IDENTIFIER_PART,                 /**< character cannot be part of an identifier */

  PARSER_ERR_INVALID_NUMBER,                          /**< invalid number literal */
  PARSER_ERR_MISSING_EXPONENT,                        /**< missing exponent */
  PARSER_ERR_IDENTIFIER_AFTER_NUMBER,                 /**< identifier start after number */

  PARSER_ERR_INVALID_REGEXP,                          /**< invalid regular expression */
  PARSER_ERR_UNKNOWN_REGEXP_FLAG,                     /**< unknown regexp flag */
  PARSER_ERR_DUPLICATED_REGEXP_FLAG,                  /**< duplicated regexp flag */
  PARSER_ERR_UNSUPPORTED_REGEXP,                      /**< regular expression is not supported */

  PARSER_ERR_IDENTIFIER_TOO_LONG,                     /**< too long identifier */
  PARSER_ERR_STRING_TOO_LONG,                         /**< too long string literal */
  PARSER_ERR_NUMBER_TOO_LONG,                         /**< too long number literal */
  PARSER_ERR_REGEXP_TOO_LONG,                         /**< too long regexp literal */

  PARSER_ERR_UNTERMINATED_MULTILINE_COMMENT,          /**< unterminated multiline comment */
  PARSER_ERR_UNTERMINATED_STRING,                     /**< unterminated string literal */
  PARSER_ERR_UNTERMINATED_REGEXP,                     /**< unterminated regexp literal */

  PARSER_ERR_NEWLINE_NOT_ALLOWED,                     /**< newline is not allowed */
  PARSER_ERR_OCTAL_NUMBER_NOT_ALLOWED,                /**< octal numbers are not allowed in strict mode */
  PARSER_ERR_OCTAL_ESCAPE_NOT_ALLOWED,                /**< octal escape sequences are not allowed in strict mode */
  PARSER_ERR_STRICT_IDENT_NOT_ALLOWED,                /**< identifier name is reserved in strict mode */
  PARSER_ERR_EVAL_NOT_ALLOWED,                        /**< eval is not allowed here in strict mode */
  PARSER_ERR_ARGUMENTS_NOT_ALLOWED,                   /**< arguments is not allowed here in strict mode */
  PARSER_ERR_DELETE_IDENT_NOT_ALLOWED,                /**< identifier delete is not allowed in strict mode */
  PARSER_ERR_EVAL_CANNOT_ASSIGNED,                    /**< eval cannot be assigned in strict mode */
  PARSER_ERR_ARGUMENTS_CANNOT_ASSIGNED,               /**< arguments cannot be assigned in strict mode */
  PARSER_ERR_WITH_NOT_ALLOWED,                        /**< with statement is not allowed in strict mode */
  PARSER_ERR_MULTIPLE_DEFAULTS_NOT_ALLOWED,           /**< multiple default cases are not allowed */
  PARSER_ERR_DEFAULT_NOT_IN_SWITCH,                   /**< default statement is not in switch block */
  PARSER_ERR_CASE_NOT_IN_SWITCH,                      /**< case statement is not in switch block */

  PARSER_ERR_LEFT_PAREN_EXPECTED,                     /**< left paren expected */
  PARSER_ERR_LEFT_BRACE_EXPECTED,                     /**< left brace expected */
  PARSER_ERR_RIGHT_PAREN_EXPECTED,                    /**< right paren expected */
  PARSER_ERR_RIGHT_SQUARE_EXPECTED,                   /**< right square expected */

  PARSER_ERR_COLON_EXPECTED,                          /**< colon expected */
  PARSER_ERR_COLON_FOR_CONDITIONAL_EXPECTED,          /**< colon expected for conditional expression */
  PARSER_ERR_SEMICOLON_EXPECTED,                      /**< semicolon expected */
  PARSER_ERR_IN_EXPECTED,                             /**< in keyword expected */
  PARSER_ERR_WHILE_EXPECTED,                          /**< while expected for do-while loop */
  PARSER_ERR_CATCH_FINALLY_EXPECTED,                  /**< catch or finally expected */
  PARSER_ERR_ARRAY_ITEM_SEPARATOR_EXPECTED,           /**< array item separator expected */
  PARSER_ERR_OBJECT_ITEM_SEPARATOR_EXPECTED,          /**< object item separator expected */
  PARSER_ERR_IDENTIFIER_EXPECTED,                     /**< identifier expected */
  PARSER_ERR_EXPRESSION_EXPECTED,                     /**< expression expected */
  PARSER_ERR_PRIMARY_EXP_EXPECTED,                    /**< primary expression expected */
  PARSER_ERR_STATEMENT_EXPECTED,                      /**< statement expected */
  PARSER_ERR_PROPERTY_IDENTIFIER_EXPECTED,            /**< property identifier expected */
  PARSER_ERR_ARGUMENT_LIST_EXPECTED,                  /**< argument list expected */
  PARSER_ERR_NO_ARGUMENTS_EXPECTED,                   /**< property getters must have no arguments */
  PARSER_ERR_ONE_ARGUMENT_EXPECTED,                   /**< property setters must have one argument */

  PARSER_ERR_INVALID_EXPRESSION,                      /**< invalid expression */
  PARSER_ERR_INVALID_SWITCH,                          /**< invalid switch body */
  PARSER_ERR_INVALID_BREAK,                           /**< break must be inside a loop or switch */
  PARSER_ERR_INVALID_BREAK_LABEL,                     /**< break target not found */
  PARSER_ERR_INVALID_CONTINUE,                        /**< continue must be inside a loop */
  PARSER_ERR_INVALID_CONTINUE_LABEL,                  /**< continue target not found */
  PARSER_ERR_INVALID_RETURN,                          /**< return must be inside a function */
  PARSER_ERR_INVALID_RIGHT_SQUARE,                    /**< right square must terminate a block */
  PARSER_ERR_DUPLICATED_LABEL,                        /**< duplicated label */
  PARSER_ERR_OBJECT_PROPERTY_REDEFINED,               /**< property of object literal redefined */
#if ENABLED (JERRY_ES2015)
  PARSER_ERR_VARIABLE_REDECLARED,                     /**< a variable redeclared */
  PARSER_ERR_MISSING_ASSIGN_AFTER_CONST,              /**< an assignment is required after a const declaration */

  PARSER_ERR_MULTIPLE_CLASS_CONSTRUCTORS,             /**< multiple class constructor */
  PARSER_ERR_CLASS_CONSTRUCTOR_AS_ACCESSOR,           /**< class constructor cannot be an accessor */
  PARSER_ERR_CLASS_STATIC_PROTOTYPE,                  /**< static method name 'prototype' is not allowed */
  PARSER_ERR_UNEXPECTED_SUPER_REFERENCE,              /**< unexpected super keyword */

  PARSER_ERR_RIGHT_BRACE_EXPECTED,                    /**< right brace expected */
  PARSER_ERR_OF_EXPECTED,                             /**< of keyword expected */

  PARSER_ERR_FORMAL_PARAM_AFTER_REST_PARAMETER,       /**< formal parameter after rest parameter */
  PARSER_ERR_REST_PARAMETER_DEFAULT_INITIALIZER,      /**< rest parameter default initializer */
  PARSER_ERR_DUPLICATED_ARGUMENT_NAMES,               /**< duplicated argument names */
#endif /* ENABLED (JERRY_ES2015) */
#if ENABLED (JERRY_ES2015_MODULE_SYSTEM)
  PARSER_ERR_FILE_NOT_FOUND,                          /**< file not found*/
  PARSER_ERR_FROM_EXPECTED,                           /**< from expected */
  PARSER_ERR_FROM_COMMA_EXPECTED,                     /**< from or comma expected */
  PARSER_ERR_AS_EXPECTED,                             /**< as expected */
  PARSER_ERR_STRING_EXPECTED,                         /**< string literal expected */
  PARSER_ERR_MODULE_UNEXPECTED,                       /**< unexpected import or export statement */
  PARSER_ERR_LEFT_BRACE_MULTIPLY_LITERAL_EXPECTED,    /**< left brace or multiply or literal expected */
  PARSER_ERR_LEFT_BRACE_MULTIPLY_EXPECTED,            /**< left brace or multiply expected */
  PARSER_ERR_RIGHT_BRACE_COMMA_EXPECTED,              /**< right brace or comma expected */
  PARSER_ERR_DUPLICATED_EXPORT_IDENTIFIER,            /**< duplicated export identifier name */
  PARSER_ERR_DUPLICATED_IMPORT_BINDING,               /**< duplicated import binding name */
#endif /* ENABLED (JERRY_ES2015_MODULE_SYSTEM) */

  PARSER_ERR_NON_STRICT_ARG_DEFINITION                /**< non-strict argument definition */
} parser_error_t;

/**
 * Source code line counter type.
 */
typedef uint32_t parser_line_counter_t;

/**
 * Error code location.
 */
typedef struct
{
  parser_error_t error;                               /**< error code */
  parser_line_counter_t line;                         /**< line where the error occured */
  parser_line_counter_t column;                       /**< column where the error occured */
} parser_error_location_t;

/* Note: source must be a valid UTF-8 string */
ecma_value_t parser_parse_script (const uint8_t *arg_list_p, size_t arg_list_size,
                                  const uint8_t *source_p, size_t source_size,
                                  uint32_t parse_opts, ecma_compiled_code_t **bytecode_data_p);

#if ENABLED (JERRY_ERROR_MESSAGES)
const char *parser_error_to_string (parser_error_t);
#endif /* ENABLED (JERRY_ERROR_MESSAGES) */

/**
 * @}
 * @}
 * @}
 */

#endif /* !JS_PARSER_H */
