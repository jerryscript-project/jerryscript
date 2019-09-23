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

#include "ecma-alloc.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-regexp-object.h"
#include "ecma-try-catch-macro.h"

#if ENABLED (JERRY_BUILTIN_REGEXP)

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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_regexp_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                   ecma_length_t arguments_list_len) /**< number of arguments */
{
  return ecma_builtin_regexp_dispatch_construct (arguments_list_p, arguments_list_len);
} /* ecma_builtin_regexp_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in RegExp object
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_regexp_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                        ecma_length_t arguments_list_len) /**< number of arguments */
{
  ecma_value_t pattern_value = ECMA_VALUE_UNDEFINED;
  ecma_value_t flags_value = ECMA_VALUE_UNDEFINED;

  if (arguments_list_len > 0)
  {
    /* pattern string or RegExp object */
    pattern_value = arguments_list_p[0];

    if (arguments_list_len > 1)
    {
      flags_value = arguments_list_p[1];
    }
  }

  if (ecma_is_value_object (pattern_value)
      && ecma_object_class_is (ecma_get_object_from_value (pattern_value), LIT_MAGIC_STRING_REGEXP_UL))
  {
    if (ecma_is_value_undefined (flags_value))
    {
      return ecma_copy_value (pattern_value);
    }

    return ecma_raise_type_error (ECMA_ERR_MSG ("Invalid argument of RegExp call."));
  }

  ecma_string_t *pattern_string_p = ecma_regexp_read_pattern_str_helper (pattern_value);

  if (pattern_string_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  uint16_t flags = 0;
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  if (!ecma_is_value_undefined (flags_value))
  {
    ecma_string_t *flags_string_p = ecma_op_to_string (flags_value);

    if (JERRY_UNLIKELY (flags_string_p == NULL))
    {
      ecma_deref_ecma_string (pattern_string_p);
      return ECMA_VALUE_ERROR;
    }

    ret_value = ecma_regexp_parse_flags (flags_string_p, &flags);
    ecma_deref_ecma_string (flags_string_p);

    if (ECMA_IS_VALUE_ERROR (ret_value))
    {
      ecma_deref_ecma_string (pattern_string_p);
      return ret_value;
    }
    JERRY_ASSERT (ecma_is_value_empty (ret_value));
  }

  ret_value = ecma_op_create_regexp_object (pattern_string_p, flags);
  ecma_deref_ecma_string (pattern_string_p);
  return ret_value;
} /* ecma_builtin_regexp_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_BUILTIN_REGEXP) */
