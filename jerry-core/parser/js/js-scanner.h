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

#ifndef JS_SCANNER_H
#define JS_SCANNER_H

/** \addtogroup parser Parser
 * @{
 *
 * \addtogroup jsparser JavaScript
 * @{
 *
 * \addtogroup jsparser_scanner Scanner
 * @{
 */

/**
 * Allowed types for scanner_info_t structures.
 */
typedef enum
{
  SCANNER_TYPE_END, /**< mark the last info block */
  SCANNER_TYPE_END_ARGUMENTS, /**< mark the end of function arguments
                               *   (only present if a function script is parsed) */
  SCANNER_TYPE_WHILE, /**< while statement */
  SCANNER_TYPE_FOR, /**< for statement */
  SCANNER_TYPE_FOR_IN, /**< for-in statement */
#if ENABLED (JERRY_ES2015_FOR_OF)
  SCANNER_TYPE_FOR_OF, /**< for-of statement */
#endif /* ENABLED (JERRY_ES2015_FOR_OF) */
  SCANNER_TYPE_SWITCH, /**< switch statement */
  SCANNER_TYPE_CASE, /**< case statement */
#if ENABLED (JERRY_ES2015_ARROW_FUNCTION)
  SCANNER_TYPE_ARROW, /**< arrow function */
#endif /* ENABLED (JERRY_ES2015_ARROW_FUNCTION) */
} scanner_info_type_t;

/**
 * Source code location which can be used to change the position of parsing.
 */
typedef struct
{
  const uint8_t *source_p; /**< next source byte */
  parser_line_counter_t line; /**< token start line */
  parser_line_counter_t column; /**< token start column */
} scanner_location_t;

/**
 * Scanner info blocks which provides information for the parser.
 */
typedef struct scanner_info_t
{
  struct scanner_info_t *next_p; /**< next info structure */
  const uint8_t *source_p; /**< triggering position of this scanner info */
  uint8_t type; /**< type of the scanner info */
} scanner_info_t;

/**
 * Scanner info extended with a location.
 */
typedef struct
{
  scanner_info_t info; /**< header */
  scanner_location_t location; /**< location */
} scanner_location_info_t;

/**
 * Scanner info for "for" statements.
 */
typedef struct
{
  scanner_info_t info; /**< header */
  scanner_location_t expression_location; /**< location of expression start */
  scanner_location_t end_location; /**< location of expression end */
} scanner_for_info_t;

/**
 * Case statement list for scanner_switch_info_t structure.
 */
typedef struct scanner_case_info_t
{
  struct scanner_case_info_t *next_p; /**< next case statement info */
  scanner_location_t location; /**< location of case statement */
} scanner_case_info_t;

/**
 * Scanner info for "switch" statements.
 */
typedef struct
{
  scanner_info_t info; /**< header */
  scanner_case_info_t *case_p; /**< list of switch cases */
} scanner_switch_info_t;

/**
 * @}
 * @}
 * @}
 */

#endif /* !JS_SCANNER_H */
