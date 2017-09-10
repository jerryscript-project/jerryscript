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

#ifndef ECMA_REGEXP_OBJECT_H
#define ECMA_REGEXP_OBJECT_H

#ifndef CONFIG_DISABLE_REGEXP_BUILTIN

#include "ecma-globals.h"
#include "re-compiler.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaregexpobject ECMA RegExp object related routines
 * @{
 */

/**
 * RegExp flags
 */
typedef enum
{
  RE_FLAG_GLOBAL = (1u << 1),      /**< ECMA-262 v5, 15.10.7.2 */
  RE_FLAG_IGNORE_CASE = (1u << 2), /**< ECMA-262 v5, 15.10.7.3 */
  RE_FLAG_MULTILINE = (1u << 3)    /**< ECMA-262 v5, 15.10.7.4 */
} re_flags_t;

/**
 * RegExp executor context
 */
typedef struct
{
  const lit_utf8_byte_t **saved_p;      /**< saved result string pointers, ECMA 262 v5, 15.10.2.1, State */
  const lit_utf8_byte_t *input_start_p; /**< start of input pattern string */
  const lit_utf8_byte_t *input_end_p;   /**< end of input pattern string */
  uint32_t num_of_captures;             /**< number of capture groups */
  uint32_t num_of_non_captures;         /**< number of non-capture groups */
  uint32_t *num_of_iterations_p;        /**< number of iterations */
  uint16_t flags;                       /**< RegExp flags */
} re_matcher_ctx_t;

ecma_value_t ecma_op_create_regexp_object_from_bytecode (re_compiled_code_t *bytecode_p);
ecma_value_t ecma_op_create_regexp_object (ecma_string_t *pattern_p, ecma_string_t *flags_str_p);
ecma_value_t ecma_regexp_exec_helper (ecma_value_t regexp_value, ecma_value_t input_string, bool ignore_global);
ecma_char_t re_canonicalize (ecma_char_t ch, bool is_ignorecase);
void re_set_result_array_properties (ecma_object_t *array_obj_p, ecma_string_t *input_str_p, uint32_t num_of_elements,
                                     int32_t index);
ecma_value_t re_parse_regexp_flags (ecma_string_t *flags_str_p, uint16_t *flags_p);
void re_initialize_props (ecma_object_t *re_obj_p, ecma_string_t *source_p, uint16_t flags);

/**
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */
#endif /* !ECMA_REGEXP_OBJECT_H */
