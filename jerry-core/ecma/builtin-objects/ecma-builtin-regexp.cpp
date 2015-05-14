/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#include "ecma-alloc.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-helpers.h"
#include "ecma-regexp-object.h"
#include "ecma-try-catch-macro.h"

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-regexp.inc.h"
#define BUILTIN_UNDERSCORED_ID regexp
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup regexp ECMA RegExp object built-in
 * @{
 */

/**
 * Handle calling [[Call]] of built-in RegExp object
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_regexp_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                   ecma_length_t arguments_list_len) /**< number of arguments */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (arguments_list_p, arguments_list_len);
} /* ecma_builtin_regexp_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in RegExp object
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_regexp_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                        ecma_length_t arguments_list_len) /**< number of arguments */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  JERRY_ASSERT (arguments_list_len <= 2 && arguments_list_p != NULL);

  ECMA_TRY_CATCH (regexp_str_value,
                  ecma_op_to_string (arguments_list_p[0]),
                  ret_value);

  ecma_string_t *pattern_string_p = ecma_copy_or_ref_ecma_string (ecma_get_string_from_value (regexp_str_value));
  ecma_string_t *flags_string_p = NULL; /* FIXME: get flags from arguments_list_p[1] */
  ret_value = ecma_op_create_regexp_object (pattern_string_p, flags_string_p);

  ecma_deref_ecma_string (pattern_string_p);
  ECMA_FINALIZE (regexp_str_value);

  return ret_value;
} /* ecma_builtin_regexp_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */

#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */
