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
 * Generic descriptor which stores only the start position.
 */
typedef struct
{
  const uint8_t *source_p; /**< start source byte */
} scanner_source_start_t;

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
 * Scanner context.
 */
typedef struct
{
  uint8_t mode; /**< scanner mode */
  scanner_switch_statement_t active_switch_statement; /**< currently active switch statement */
} scanner_context_t;

/**
 * @}
 * @}
 * @}
 */

#endif /* !JS_SCANNER_INTERNAL_H */
