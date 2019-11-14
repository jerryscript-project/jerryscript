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
#if ENABLED (JERRY_ES2015)
  ecma_value_t new_pattern = ECMA_VALUE_EMPTY;
  ecma_value_t new_flags = ECMA_VALUE_EMPTY;
#endif /* ENABLED (JERRY_ES2015) */

  if (arguments_list_len > 0)
  {
    /* pattern string or RegExp object */
    pattern_value = arguments_list_p[0];

    if (arguments_list_len > 1)
    {
      flags_value = arguments_list_p[1];
    }
  }

#if ENABLED (JERRY_ES2015)
  ecma_value_t regexp_value = ecma_op_is_regexp (pattern_value);

  if (ECMA_IS_VALUE_ERROR (regexp_value))
  {
    return regexp_value;
  }

  bool is_regexp = regexp_value == ECMA_VALUE_TRUE;
#else /* !ENABLED (JERRY_ES2015) */
  bool is_regexp = ecma_object_is_regexp_object (pattern_value);
#endif /* ENABLED (JERRY_ES2015) */

  if (is_regexp)
  {
#if ENABLED (JERRY_ES2015)
    ecma_object_t *pattern_obj_p = ecma_get_object_from_value (pattern_value);

    new_pattern = ecma_op_object_get_by_magic_id (pattern_obj_p, LIT_MAGIC_STRING_SOURCE);

    if (ECMA_IS_VALUE_ERROR (new_pattern))
    {
      return new_pattern;
    }

    pattern_value = new_pattern;

    if (ecma_is_value_undefined (flags_value))
    {
      new_flags = ecma_op_object_get_by_magic_id (pattern_obj_p, LIT_MAGIC_STRING_FLAGS);

      if (ECMA_IS_VALUE_ERROR (new_flags))
      {
        ecma_free_value (new_pattern);
        return new_flags;
      }

      flags_value = new_flags;
    }
#else /* !ENABLED (JERRY_ES2015) */
    if (ecma_is_value_undefined (flags_value))
    {
      return ecma_copy_value (pattern_value);
    }

    return ecma_raise_type_error (ECMA_ERR_MSG ("Invalid argument of RegExp call."));
#endif /* ENABLED (JERRY_ES2015) */
  }

  ecma_string_t *pattern_string_p = ecma_regexp_read_pattern_str_helper (pattern_value);
  ecma_value_t ret_value = ECMA_VALUE_ERROR;

  if (pattern_string_p == NULL)
  {
    goto cleanup;
  }

  uint16_t flags = 0;

  if (!ecma_is_value_undefined (flags_value))
  {
    ecma_string_t *flags_string_p = ecma_op_to_string (flags_value);

    if (JERRY_UNLIKELY (flags_string_p == NULL))
    {
      ecma_deref_ecma_string (pattern_string_p);
      goto cleanup;
    }

    ret_value = ecma_regexp_parse_flags (flags_string_p, &flags);
    ecma_deref_ecma_string (flags_string_p);

    if (ECMA_IS_VALUE_ERROR (ret_value))
    {
      ecma_deref_ecma_string (pattern_string_p);
      goto cleanup;
    }
    JERRY_ASSERT (ecma_is_value_empty (ret_value));
  }

  ret_value = ecma_op_create_regexp_object (pattern_string_p, flags);
  ecma_deref_ecma_string (pattern_string_p);

cleanup:
#if ENABLED (JERRY_ES2015)
  ecma_free_value (new_pattern);
  ecma_free_value (new_flags);
#endif /* ENABLED (JERRY_ES2015) */

  return ret_value;
} /* ecma_builtin_regexp_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_BUILTIN_REGEXP) */
