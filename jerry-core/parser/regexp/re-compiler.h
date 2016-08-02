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

#ifndef RE_COMPILER_H
#define RE_COMPILER_H

#ifndef CONFIG_DISABLE_REGEXP_BUILTIN

#include "ecma-globals.h"
#include "re-bytecode.h"
#include "re-parser.h"

/** \addtogroup parser Parser
 * @{
 *
 * \addtogroup regexparser Regular expression
 * @{
 *
 * \addtogroup regexparser_compiler Compiler
 * @{
 */

/**
 * Context of RegExp compiler
 */
typedef struct
{
  uint16_t flags;                    /**< RegExp flags */
  uint32_t num_of_captures;          /**< number of capture groups */
  uint32_t num_of_non_captures;      /**< number of non-capture groups */
  uint32_t highest_backref;          /**< highest backreference */
  re_bytecode_ctx_t *bytecode_ctx_p; /**< pointer of RegExp bytecode context */
  re_token_t current_token;          /**< current token */
  re_parser_ctx_t *parser_ctx_p;     /**< pointer of RegExp parser context */
} re_compiler_ctx_t;

ecma_value_t
re_compile_bytecode (const re_compiled_code_t **, ecma_string_t *, uint16_t);

void re_cache_gc_run ();

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */
#endif /* !RE_COMPILER_H */
