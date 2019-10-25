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
#include "ecma-array-object.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"
#include "lit-char-helpers.h"

#if ENABLED (JERRY_BUILTIN_REGEXP)
#include "ecma-regexp-object.h"
#include "re-compiler.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-regexp-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID regexp_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup regexpprototype ECMA RegExp.prototype object built-in
 * @{
 */

#if ENABLED (JERRY_ES2015)
/**
 * Helper function to retrieve the flags associated with a RegExp object
 *
 * @return ECMA_VALUE_ERROR - if 'this' is not a RegExp object
 *         ECMA_VALUE_EMPTY - otherwise
 */
static ecma_value_t
ecma_builtin_regexp_prototype_flags_helper (ecma_value_t this, /**< this value */
                                            uint16_t *flags_p) /**< [out] flags */
{
  if (!ecma_is_value_object (this)
      || !ecma_object_class_is (ecma_get_object_from_value (this), LIT_MAGIC_STRING_REGEXP_UL))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Incompatible type"));
  }

  ecma_extended_object_t *re_obj_p = (ecma_extended_object_t *) ecma_get_object_from_value (this);
  re_compiled_code_t *bc_p = ECMA_GET_INTERNAL_VALUE_ANY_POINTER (re_compiled_code_t,
                                                                  re_obj_p->u.class_prop.u.value);

  if (bc_p != NULL)
  {
    *flags_p = bc_p->header.status_flags;
  }

  return ECMA_VALUE_EMPTY;
} /* ecma_builtin_regexp_prototype_flags_helper */

/**
 * The RegExp.prototype object's 'flags' accessor property
 *
 * See also:
 *          ECMA-262 v6, 21.2.5.3
 *
 * @return ECMA_VALUE_ERROR - if 'this' is not a RegExp object
 *         string value     - otherwise
 *
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_regexp_prototype_get_flags (ecma_value_t this_arg) /**< this argument */
{
  uint16_t flags = RE_FLAG_EMPTY;
  ecma_value_t ret_value = ecma_builtin_regexp_prototype_flags_helper (this_arg, &flags);
  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    return ret_value;
  }

  ecma_stringbuilder_t result = ecma_stringbuilder_create ();

  if (flags & RE_FLAG_GLOBAL)
  {
    ecma_stringbuilder_append_byte (&result, LIT_CHAR_LOWERCASE_G);
  }

  if (flags & RE_FLAG_IGNORE_CASE)
  {
    ecma_stringbuilder_append_byte (&result, LIT_CHAR_LOWERCASE_I);
  }

  if (flags & RE_FLAG_MULTILINE)
  {
    ecma_stringbuilder_append_byte (&result, LIT_CHAR_LOWERCASE_M);
  }

  return ecma_make_string_value (ecma_stringbuilder_finalize (&result));
} /* ecma_builtin_regexp_prototype_get_flags */

/**
 * The RegExp.prototype object's 'source' accessor property
 *
 * See also:
 *          ECMA-262 v6, 21.2.5.10
 *
 * @return ECMA_VALUE_ERROR - if 'this' is not a RegExp object
 *         string value     - otherwise
 *
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_regexp_prototype_get_source (ecma_value_t this_arg) /**< this argument */
{
  if (!ecma_is_value_object (this_arg)
      || !ecma_object_class_is (ecma_get_object_from_value (this_arg), LIT_MAGIC_STRING_REGEXP_UL))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Incompatible type"));
  }

  ecma_extended_object_t *re_obj_p = (ecma_extended_object_t *) ecma_get_object_from_value (this_arg);
  re_compiled_code_t *bc_p = ECMA_GET_INTERNAL_VALUE_ANY_POINTER (re_compiled_code_t,
                                                                  re_obj_p->u.class_prop.u.value);

  if (bc_p != NULL)
  {
    ecma_ref_ecma_string (ecma_get_string_from_value (bc_p->source));
    return bc_p->source;
  }

  return ecma_make_string_value (ecma_get_magic_string (LIT_MAGIC_STRING_EMPTY_NON_CAPTURE_GROUP));
} /* ecma_builtin_regexp_prototype_get_source */

/**
 * The RegExp.prototype object's 'global' accessor property
 *
 * See also:
 *          ECMA-262 v6, 21.2.5.4
 *
 * @return ECMA_VALUE_ERROR - if 'this' is not a RegExp object
 *         ECMA_VALUE_TRUE  - if 'global' flag is set
 *         ECMA_VALUE_FALSE - otherwise
 *
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_regexp_prototype_get_global (ecma_value_t this_arg) /**< this argument */
{
  uint16_t flags = RE_FLAG_EMPTY;
  ecma_value_t ret_value = ecma_builtin_regexp_prototype_flags_helper (this_arg, &flags);
  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    return ret_value;
  }

  return ecma_make_boolean_value (flags & RE_FLAG_GLOBAL);
} /* ecma_builtin_regexp_prototype_get_global */

/**
 * The RegExp.prototype object's 'ignoreCase' accessor property
 *
 * See also:
 *          ECMA-262 v6, 21.2.5.5
 *
 * @return ECMA_VALUE_ERROR - if 'this' is not a RegExp object
 *         ECMA_VALUE_TRUE  - if 'ignoreCase' flag is set
 *         ECMA_VALUE_FALSE - otherwise
 *
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_regexp_prototype_get_ignorecase (ecma_value_t this_arg) /**< this argument */
{
  uint16_t flags = RE_FLAG_EMPTY;
  ecma_value_t ret_value = ecma_builtin_regexp_prototype_flags_helper (this_arg, &flags);
  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    return ret_value;
  }

  return ecma_make_boolean_value (flags & RE_FLAG_IGNORE_CASE);
} /* ecma_builtin_regexp_prototype_get_ignorecase */

/**
 * The RegExp.prototype object's 'multiline' accessor property
 *
 * See also:
 *          ECMA-262 v6, 21.2.5.7
 *
 * @return ECMA_VALUE_ERROR - if 'this' is not a RegExp object
 *         ECMA_VALUE_TRUE  - if 'multiline' flag is set
 *         ECMA_VALUE_FALSE - otherwise
 *
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_regexp_prototype_get_multiline (ecma_value_t this_arg) /**< this argument */
{
  uint16_t flags = RE_FLAG_EMPTY;
  ecma_value_t ret_value = ecma_builtin_regexp_prototype_flags_helper (this_arg, &flags);
  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    return ret_value;
  }

  return ecma_make_boolean_value (flags & RE_FLAG_MULTILINE);
} /* ecma_builtin_regexp_prototype_get_multiline */
#endif /* ENABLED (JERRY_ES2015) */

#if ENABLED (JERRY_BUILTIN_ANNEXB)

/**
 * The RegExp.prototype object's 'compile' routine
 *
 * See also:
 *          ECMA-262 v5, B.2.5.1
 *
 * @return undefined        - if compiled successfully
 *         error ecma value - otherwise
 *
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_regexp_prototype_compile (ecma_value_t this_arg, /**< this argument */
                                       ecma_value_t pattern_arg, /**< pattern or RegExp object */
                                       ecma_value_t flags_arg) /**< flags */
{
  if (!ecma_is_value_object (this_arg)
      || !ecma_object_class_is (ecma_get_object_from_value (this_arg), LIT_MAGIC_STRING_REGEXP_UL)
      /* The builtin RegExp.prototype object does not have [[RegExpMatcher]] internal slot */
      || ecma_get_object_from_value (this_arg) == ecma_builtin_get (ECMA_BUILTIN_ID_REGEXP_PROTOTYPE))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Incompatible type"));
  }

  uint16_t flags = 0;

  if (ecma_is_value_object (pattern_arg)
      && ecma_object_class_is (ecma_get_object_from_value (pattern_arg), LIT_MAGIC_STRING_REGEXP_UL)
      && ecma_get_object_from_value (pattern_arg) != ecma_builtin_get (ECMA_BUILTIN_ID_REGEXP_PROTOTYPE))
  {
    if (!ecma_is_value_undefined (flags_arg))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("Invalid argument"));
    }

    /* Compile from existing RegExp object. */
    ecma_extended_object_t *target_p = (ecma_extended_object_t *) ecma_get_object_from_value (pattern_arg);
    re_compiled_code_t *target_bc_p = ECMA_GET_INTERNAL_VALUE_ANY_POINTER (re_compiled_code_t,
                                                                           target_p->u.class_prop.u.value);

    ecma_object_t *this_object_p = ecma_get_object_from_value (this_arg);
    ecma_extended_object_t *current_p = (ecma_extended_object_t *) this_object_p;

    re_compiled_code_t *current_bc_p = ECMA_GET_INTERNAL_VALUE_ANY_POINTER (re_compiled_code_t,
                                                                            current_p->u.class_prop.u.value);

    JERRY_ASSERT (current_bc_p != NULL);
    ecma_bytecode_deref ((ecma_compiled_code_t *) current_bc_p);

    JERRY_ASSERT (target_bc_p != NULL);
    ecma_bytecode_ref ((ecma_compiled_code_t *) target_bc_p);
    ECMA_SET_INTERNAL_VALUE_POINTER (current_p->u.class_prop.u.value, target_bc_p);
    ecma_regexp_initialize_props (this_object_p,
                                  ecma_get_string_from_value (target_bc_p->source),
                                  target_bc_p->header.status_flags);
    return ecma_copy_value (this_arg);
  }

  ecma_string_t *pattern_string_p = ecma_regexp_read_pattern_str_helper (pattern_arg);

  /* Get source string. */
  if (pattern_string_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  /* Parse flags. */
  if (!ecma_is_value_undefined (flags_arg))
  {
    ecma_string_t *flags_str_p = ecma_op_to_string (flags_arg);
    if (JERRY_UNLIKELY (flags_str_p == NULL))
    {
      ecma_deref_ecma_string (pattern_string_p);
      return ECMA_VALUE_ERROR;
    }

    ecma_value_t parsed_flags_val = ecma_regexp_parse_flags (flags_str_p, &flags);
    ecma_deref_ecma_string (flags_str_p);

    if (ECMA_IS_VALUE_ERROR (parsed_flags_val))
    {
      ecma_deref_ecma_string (pattern_string_p);
      return parsed_flags_val;
    }
  }

  /* Try to compile bytecode from new source. */
  const re_compiled_code_t *new_bc_p = NULL;
  ecma_value_t bc_val = re_compile_bytecode (&new_bc_p, pattern_string_p, flags);
  if (ECMA_IS_VALUE_ERROR (bc_val))
  {
    ecma_deref_ecma_string (pattern_string_p);
    return bc_val;
  }

  ecma_object_t *this_obj_p = ecma_get_object_from_value (this_arg);
  ecma_value_t *bc_prop_p = &(((ecma_extended_object_t *) this_obj_p)->u.class_prop.u.value);

  re_compiled_code_t *old_bc_p = ECMA_GET_INTERNAL_VALUE_ANY_POINTER (re_compiled_code_t, *bc_prop_p);

  JERRY_ASSERT (old_bc_p != NULL);
  ecma_bytecode_deref ((ecma_compiled_code_t *) old_bc_p);

  ECMA_SET_INTERNAL_VALUE_POINTER (*bc_prop_p, new_bc_p);
  ecma_regexp_initialize_props (this_obj_p, pattern_string_p, flags);
  ecma_deref_ecma_string (pattern_string_p);

  return ecma_copy_value (this_arg);
} /* ecma_builtin_regexp_prototype_compile */

#endif /* ENABLED (JERRY_BUILTIN_ANNEXB) */

/**
 * The RegExp.prototype object's 'exec' routine
 *
 * See also:
 *          ECMA-262 v5, 15.10.6.2
 *
 * @return array object containing the results - if the matched
 *         null                                - otherwise
 *
 *         May raise error, so returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_regexp_prototype_exec (ecma_value_t this_arg, /**< this argument */
                                    ecma_value_t arg) /**< routine's argument */
{
  if (!ecma_is_value_object (this_arg)
      || !ecma_object_class_is (ecma_get_object_from_value (this_arg), LIT_MAGIC_STRING_REGEXP_UL))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Incomplete RegExp type"));
  }

  ecma_value_t obj_this = ecma_op_to_object (this_arg);
  if (ECMA_IS_VALUE_ERROR (obj_this))
  {
    return obj_this;
  }

  ecma_string_t *input_str_p = ecma_op_to_string (arg);
  if (JERRY_UNLIKELY (input_str_p == NULL))
  {
    ecma_free_value (obj_this);
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t ret_value = ecma_regexp_exec_helper (obj_this, ecma_make_string_value (input_str_p), false);

  ecma_free_value (obj_this);
  ecma_deref_ecma_string (input_str_p);

  return ret_value;
} /* ecma_builtin_regexp_prototype_exec */

/**
 * The RegExp.prototype object's 'test' routine
 *
 * See also:
 *          ECMA-262 v5, 15.10.6.3
 *
 * @return true  - if match is not null
 *         false - otherwise
 *
 *         May raise error, so returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_regexp_prototype_test (ecma_value_t this_arg, /**< this argument */
                                    ecma_value_t arg) /**< routine's argument */
{
  ecma_value_t result = ecma_builtin_regexp_prototype_exec (this_arg, arg);

  if (ECMA_IS_VALUE_ERROR (result))
  {
    return result;
  }

  ecma_value_t ret_value = ecma_make_boolean_value (!ecma_is_value_null (result));
  ecma_free_value (result);

  return ret_value;
} /* ecma_builtin_regexp_prototype_test */

/**
 * The RegExp.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.10.6.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_regexp_prototype_to_string (ecma_value_t this_arg) /**< this argument */
{
  if (!ecma_is_value_object (this_arg)
      || !ecma_object_class_is (ecma_get_object_from_value (this_arg), LIT_MAGIC_STRING_REGEXP_UL))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Incompatible type"));
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (this_arg);
  ecma_extended_object_t *re_obj_p = (ecma_extended_object_t *) obj_p;

  re_compiled_code_t *bc_p = ECMA_GET_INTERNAL_VALUE_ANY_POINTER (re_compiled_code_t,
                                                                  re_obj_p->u.class_prop.u.value);

  ecma_string_t *source_p;
  uint16_t flags;

  if (bc_p != NULL)
  {
    source_p = ecma_get_string_from_value (bc_p->source);
    flags = bc_p->header.status_flags;
  }
  else
  {
    source_p = ecma_get_magic_string (LIT_MAGIC_STRING_EMPTY_NON_CAPTURE_GROUP);
    flags = RE_FLAG_EMPTY;
  }

  ecma_stringbuilder_t result = ecma_stringbuilder_create ();
  ecma_stringbuilder_append_byte (&result, LIT_CHAR_SLASH);
  ecma_stringbuilder_append (&result, source_p);
  ecma_stringbuilder_append_byte (&result, LIT_CHAR_SLASH);

  if (flags & RE_FLAG_GLOBAL)
  {
    ecma_stringbuilder_append_byte (&result, LIT_CHAR_LOWERCASE_G);
  }

  if (flags & RE_FLAG_IGNORE_CASE)
  {
    ecma_stringbuilder_append_byte (&result, LIT_CHAR_LOWERCASE_I);
  }

  if (flags & RE_FLAG_MULTILINE)
  {
    ecma_stringbuilder_append_byte (&result, LIT_CHAR_LOWERCASE_M);
  }

  return ecma_make_string_value (ecma_stringbuilder_finalize (&result));
} /* ecma_builtin_regexp_prototype_to_string */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_BUILTIN_REGEXP) */
