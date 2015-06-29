/* Copyright 2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged.
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

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN

#include "ecma-globals.h"
#include "re-compiler.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaregexpobject ECMA RegExp object related routines
 * @{
 */

#define RE_EXECUTE_RECURSION_LIMIT  1000  /* Limit of RegExp executor recursion depth */
#define RE_EXECUTE_MATCH_LIMIT      10000 /* Limit of RegExp execetur matching steps */

/**
 * RegExp executor context
 *
 * FIXME:
 *       Add comments with description of the structure members
 */
typedef struct
{
  const lit_utf8_byte_t **saved_p;
  const lit_utf8_byte_t *input_start_p;
  const lit_utf8_byte_t *input_end_p;
  uint32_t match_limit;
  uint32_t recursion_depth;
  uint32_t num_of_captures;
  uint32_t num_of_non_captures;
  uint32_t *num_of_iterations;
  uint8_t flags;
} re_matcher_ctx_t;

extern ecma_completion_value_t
ecma_op_create_regexp_object (ecma_string_t *pattern_p, ecma_string_t *flags_str_p);

extern ecma_completion_value_t
ecma_regexp_exec_helper (ecma_object_t *obj_p,
                         re_bytecode_t *bc_p,
                         const lit_utf8_byte_t *str_p,
                         lit_utf8_size_t str_size);

/**
 * @}
 * @}
 */

#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */
#endif /* !ECMA_REGEXP_OBJECT_H */
