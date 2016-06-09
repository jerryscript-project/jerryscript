/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2015-2016 University of Szeged.
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

#ifndef JS_LEXER_H
#define JS_LEXER_H

/** \addtogroup parser Parser
 * @{
 *
 * \addtogroup jsparser JavaScript
 * @{
 *
 * \addtogroup jsparser_lexer Lexer
 * @{
 */

/**
 * Lexer token types.
 */
typedef enum
{
  LEXER_EOS,                     /**< end of source */

  /* Primary expressions */
  LEXER_LITERAL,                 /**< literal token */
  LEXER_KEYW_THIS,               /**< this */
  LEXER_LIT_TRUE,                /**< true (not a keyword!) */
  LEXER_LIT_FALSE,               /**< false (not a keyword!) */
  LEXER_LIT_NULL,                /**< null (not a keyword!) */

  /* Unary operators
   * IMPORTANT: update CBC_UNARY_OP_TOKEN_TO_OPCODE and
   *            CBC_UNARY_LVALUE_OP_TOKEN_TO_OPCODE after changes. */
#define LEXER_IS_UNARY_OP_TOKEN(token_type) \
  ((token_type) >= LEXER_PLUS && (token_type) <= LEXER_DECREASE)
#define LEXER_IS_UNARY_LVALUE_OP_TOKEN(token_type) \
  ((token_type) >= LEXER_KEYW_DELETE && (token_type) <= LEXER_DECREASE)

  LEXER_PLUS,                    /**< "+" */
  LEXER_NEGATE,                  /**< "-" */
  LEXER_LOGICAL_NOT,             /**< "!" */
  LEXER_BIT_NOT,                 /**< "~" */
  LEXER_KEYW_VOID,               /**< void */
  LEXER_KEYW_TYPEOF,             /**< typeof */
  LEXER_KEYW_DELETE,             /**< delete */
  LEXER_INCREASE,                /**< "++" */
  LEXER_DECREASE,                /**< "--" */

  /* Binary operators
   * IMPORTANT: update CBC_BINARY_OP_TOKEN_TO_OPCODE,
   *            CBC_BINARY_LVALUE_OP_TOKEN_TO_OPCODE and
   *            parser_binary_precedence_table after changes. */
#define LEXER_IS_BINARY_OP_TOKEN(token_type) \
  ((token_type) >= LEXER_ASSIGN && (token_type) <= LEXER_MODULO)
#define LEXER_IS_BINARY_LVALUE_TOKEN(token_type) \
  ((token_type) >= LEXER_ASSIGN && (token_type) <= LEXER_ASSIGN_BIT_XOR)
#define LEXER_FIRST_BINARY_OP LEXER_ASSIGN

  LEXER_ASSIGN,                  /**< "=" (prec: 3) */
  LEXER_ASSIGN_ADD,              /**< "+=" (prec: 3) */
  LEXER_ASSIGN_SUBTRACT,         /**< "-=" (prec: 3) */
  LEXER_ASSIGN_MULTIPLY,         /**< "*=" (prec: 3) */
  LEXER_ASSIGN_DIVIDE,           /**< "/=" (prec: 3) */
  LEXER_ASSIGN_MODULO,           /**< "%=" (prec: 3) */
  LEXER_ASSIGN_LEFT_SHIFT,       /**< "<<=" (prec: 3) */
  LEXER_ASSIGN_RIGHT_SHIFT,      /**< ">>=" (prec: 3) */
  LEXER_ASSIGN_UNS_RIGHT_SHIFT,  /**< ">>>=" (prec: 3) */
  LEXER_ASSIGN_BIT_AND,          /**< "&=" (prec: 3) */
  LEXER_ASSIGN_BIT_OR,           /**< "|=" (prec: 3) */
  LEXER_ASSIGN_BIT_XOR,          /**< "^=" (prec: 3) */
  LEXER_QUESTION_MARK,           /**< "?" (prec: 4) */
  LEXER_LOGICAL_OR,              /**< "||" (prec: 5) */
  LEXER_LOGICAL_AND,             /**< "&&" (prec: 6) */
  LEXER_BIT_OR,                  /**< "|" (prec: 7) */
  LEXER_BIT_XOR,                 /**< "^" (prec: 8) */
  LEXER_BIT_AND,                 /**< "&" (prec: 9) */
  LEXER_EQUAL,                   /**< "==" (prec: 10) */
  LEXER_NOT_EQUAL,               /**< "!=" (prec: 10) */
  LEXER_STRICT_EQUAL,            /**< "===" (prec: 10) */
  LEXER_STRICT_NOT_EQUAL,        /**< "!==" (prec: 10) */
  LEXER_LESS,                    /**< "<" (prec: 11) */
  LEXER_GREATER,                 /**< ">" (prec: 11) */
  LEXER_LESS_EQUAL,              /**< "<=" (prec: 11) */
  LEXER_GREATER_EQUAL,           /**< ">=" (prec: 11) */
  LEXER_KEYW_IN,                 /**< in (prec: 11) */
  LEXER_KEYW_INSTANCEOF,         /**< instanceof (prec: 11) */
  LEXER_LEFT_SHIFT,              /**< "<<" (prec: 12) */
  LEXER_RIGHT_SHIFT,             /**< ">>" (prec: 12) */
  LEXER_UNS_RIGHT_SHIFT,         /**< ">>>" (prec: 12) */
  LEXER_ADD,                     /**< "+" (prec: 13) */
  LEXER_SUBTRACT,                /**< "-" (prec: 13) */
  LEXER_MULTIPLY,                /**< "*" (prec: 14) */
  LEXER_DIVIDE,                  /**< "/" (prec: 14) */
  LEXER_MODULO,                  /**< "%" (prec: 14) */

  LEXER_LEFT_BRACE,              /**< "{" */
  LEXER_LEFT_PAREN,              /**< "(" */
  LEXER_LEFT_SQUARE,             /**< "[" */
  LEXER_RIGHT_BRACE,             /**< "}" */
  LEXER_RIGHT_PAREN,             /**<_")" */
  LEXER_RIGHT_SQUARE,            /**< "]" */
  LEXER_DOT,                     /**< "." */
  LEXER_SEMICOLON,               /**< ";" */
  LEXER_COLON,                   /**< ":" */
  LEXER_COMMA,                   /**< "," */

  LEXER_KEYW_BREAK,              /**< break */
  LEXER_KEYW_DO,                 /**< do */
  LEXER_KEYW_CASE,               /**< case  */
  LEXER_KEYW_ELSE,               /**< else */
  LEXER_KEYW_NEW,                /**< new */
  LEXER_KEYW_VAR,                /**< var */
  LEXER_KEYW_CATCH,              /**< catch */
  LEXER_KEYW_FINALLY,            /**< finally */
  LEXER_KEYW_RETURN,             /**< return */
  LEXER_KEYW_CONTINUE,           /**< continue */
  LEXER_KEYW_FOR,                /**< for */
  LEXER_KEYW_SWITCH,             /**< switch */
  LEXER_KEYW_WHILE,              /**< while */
  LEXER_KEYW_DEBUGGER,           /**< debugger */
  LEXER_KEYW_FUNCTION,           /**< function */
  LEXER_KEYW_WITH,               /**< with */
  LEXER_KEYW_DEFAULT,            /**< default */
  LEXER_KEYW_IF,                 /**< if */
  LEXER_KEYW_THROW,              /**< throw */
  LEXER_KEYW_TRY,                /**< try */

  /* These are virtual tokens. */
  LEXER_EXPRESSION_START,        /**< expression start */
  LEXER_PROPERTY_GETTER,         /**< property getter function */
  LEXER_PROPERTY_SETTER,         /**< property setter function */
  LEXER_COMMA_SEP_LIST,          /**< comma separated bracketed expression list */
  LEXER_SCAN_SWITCH,             /**< special value for switch pre-scan */

  /* Future reserved words: these keywords
   * must form a group after all other keywords. */
#define LEXER_FIRST_FUTURE_RESERVED_WORD LEXER_KEYW_CLASS
  LEXER_KEYW_CLASS,              /**< class */
  LEXER_KEYW_ENUM,               /**< enum */
  LEXER_KEYW_EXTENDS,            /**< extends */
  LEXER_KEYW_SUPER,              /**< super */
  LEXER_KEYW_CONST,              /**< const */
  LEXER_KEYW_EXPORT,             /**< export */
  LEXER_KEYW_IMPORT,             /**< import */

  /* Future strict reserved words: these keywords
   * must form a group after future reserved words. */
#define LEXER_FIRST_FUTURE_STRICT_RESERVED_WORD LEXER_KEYW_IMPLEMENTS
  LEXER_KEYW_IMPLEMENTS,         /**< implements */
  LEXER_KEYW_LET,                /**< let */
  LEXER_KEYW_PRIVATE,            /**< private */
  LEXER_KEYW_PUBLIC,             /**< public */
  LEXER_KEYW_YIELD,              /**< yield */
  LEXER_KEYW_INTERFACE,          /**< interface */
  LEXER_KEYW_PACKAGE,            /**< package */
  LEXER_KEYW_PROTECTED,          /**< protected */
  LEXER_KEYW_STATIC,             /**< static */
} lexer_token_type_t;

#define LEXER_NEWLINE_LS_PS_BYTE_1 0xe2
#define LEXER_NEWLINE_LS_PS_BYTE_23(source) \
  ((source)[1] == LIT_UTF8_2_BYTE_CODE_POINT_MIN && ((source)[2] | 0x1) == 0xa9)
#define LEXER_UTF8_4BYTE_START 0xf0

#define LEXER_IS_LEFT_BRACKET(type) \
  ((type) == LEXER_LEFT_BRACE || (type) == LEXER_LEFT_PAREN || (type) == LEXER_LEFT_SQUARE)

#define LEXER_IS_RIGHT_BRACKET(type) \
  ((type) == LEXER_RIGHT_BRACE || (type) == LEXER_RIGHT_PAREN || (type) == LEXER_RIGHT_SQUARE)

#define LEXER_UNARY_OP_TOKEN_TO_OPCODE(token_type) \
   ((((token_type) - LEXER_PLUS) * 2) + CBC_PLUS)

#define LEXER_UNARY_LVALUE_OP_TOKEN_TO_OPCODE(token_type) \
   ((((token_type) - LEXER_INCREASE) * 6) + CBC_PRE_INCR)

#define LEXER_BINARY_OP_TOKEN_TO_OPCODE(token_type) \
   ((cbc_opcode_t) ((((token_type) - LEXER_BIT_OR) * 3) + CBC_BIT_OR))

#define LEXER_BINARY_LVALUE_OP_TOKEN_TO_OPCODE(token_type) \
   ((cbc_opcode_t) ((((token_type) - LEXER_ASSIGN_ADD) * 2) + CBC_ASSIGN_ADD))

/**
 * Lexer literal object types.
 */
typedef enum
{
  LEXER_LITERAL_OBJECT_ANY,                 /**< unspecified object type */
  LEXER_LITERAL_OBJECT_EVAL,                /**< reference is equal to eval */
  LEXER_LITERAL_OBJECT_ARGUMENTS,           /**< reference is equal to arguments */
} lexer_literal_object_type_t;

/**
 * Lexer number types.
 */
typedef enum
{
  LEXER_NUMBER_DECIMAL,                     /**< decimal number */
  LEXER_NUMBER_HEXADECIMAL,                 /**< hexadecimal number */
  LEXER_NUMBER_OCTAL,                       /**< octal number */
} lexer_number_type_t;

/**
 * Lexer character (string / identifier) literal data.
 */
typedef struct
{
  const uint8_t *char_p;                     /**< start of identifier or string token */
  uint16_t length;                           /**< length or index of a literal */
  uint8_t type;                              /**< type of the current literal */
  uint8_t has_escape;                        /**< has escape sequences */
} lexer_lit_location_t;

/**
 * Range of input string which processing is postponed.
 */
typedef struct
{
  const uint8_t *source_p;                   /**< next source byte */
  const uint8_t *source_end_p;               /**< last source byte */
  parser_line_counter_t line;                /**< token start line */
  parser_line_counter_t column;              /**< token start column */
} lexer_range_t;

/**
 * Lexer token.
 */
typedef struct
{
  uint8_t type;                              /**< token type */
  uint8_t literal_is_reserved;               /**< future reserved keyword
                                              *   (when char_literal.type is LEXER_IDENT_LITERAL) */
  uint8_t extra_value;                       /**< helper value for different purposes */
  uint8_t was_newline;                       /**< newline occured before this token */
  parser_line_counter_t line;                /**< token start line */
  parser_line_counter_t column;              /**< token start column */
  lexer_lit_location_t lit_location;         /**< extra data for character literals */
} lexer_token_t;

/**
 * Literal data set by lexer_construct_literal_object.
 */
typedef struct
{
  lexer_literal_t *literal_p;                /**< pointer to the literal object */
  uint16_t index;                            /**< literal index */
  uint8_t type;                              /**< literal object type */
} lexer_lit_object_t;

/**
 * @}
 * @}
 * @}
 */

#endif /* !JS_LEXER_H */
