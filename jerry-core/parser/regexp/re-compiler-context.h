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

#ifndef RE_COMPILER_CONTEXT_H
#define RE_COMPILER_CONTEXT_H

#if ENABLED (JERRY_BUILTIN_REGEXP)

#include "re-token.h"

/** \addtogroup parser Parser
 * @{
 *
 * \addtogroup regexparser Regular expression
 * @{
 *
 * \addtogroup regexparser_compiler Compiler
 * @{
 */
#if ENABLED (JERRY_ESNEXT)
 /**
  * RegExp named group instance
  */
typedef struct re_group_name_t
{
  struct re_group_name_t *next_p; /**< next captured named groups*/
  uint32_t capture_index; /**< number of capture groups*/
  uint32_t name_length; /**< length of captured group name*/
  const lit_utf8_byte_t *name_p; /**< name of the captured group*/
} re_group_name_t;
#endif /* ENABLED (JERRY_ESNEXT) */
/**
 * RegExp compiler context
 */
typedef struct
{
#if ENABLED (JERRY_ESNEXT)
  re_group_name_t *group_names_p;                 /**< captured named groups */
#endif /* ENABLED (JERRY_ESNEXT) */
  const lit_utf8_byte_t *input_start_p;           /**< start of input pattern */
  const lit_utf8_byte_t *input_curr_p;            /**< current position in input pattern */
  const lit_utf8_byte_t *input_end_p;             /**< end of input pattern */

  uint8_t *bytecode_start_p;                      /**< start of bytecode block */
  size_t bytecode_size;                           /**< size of bytecode */

  uint32_t captures_count;                        /**< number of capture groups */
  uint32_t non_captures_count;                    /**< number of non-capture groups */

  int groups_count;                               /**< number of groups */
#if ENABLED (JERRY_ESNEXT)
  bool has_reference;                             /**< if had invalid named references  */
#endif /* ENABLED (JERRY_ESNEXT) */
  uint16_t flags;                                 /**< RegExp flags */
  re_token_t token;                               /**< current token */
} re_compiler_ctx_t;

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_BUILTIN_REGEXP) */
#endif /* !RE_COMPILER_CONTEXT_H */
