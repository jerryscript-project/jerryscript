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
#include "ecma-builtin-helpers.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-globals.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-regexp-object.h"
#include "lit-char-helpers.h"

#if ENABLED (JERRY_BUILTIN_REGEXP)
#include "ecma-regexp-object.h"
#include "re-compiler.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/**
 * This object has a custom dispatch function.
 */
 #define BUILTIN_CUSTOM_DISPATCH

/**
 * List of built-in routine identifiers.
 */
enum
{
  /** These routines must be in this order */
  ECMA_REGEXP_PROTOTYPE_ROUTINE_START = 0,
  ECMA_REGEXP_PROTOTYPE_ROUTINE_EXEC,
#if ENABLED (JERRY_BUILTIN_ANNEXB)
  ECMA_REGEXP_PROTOTYPE_ROUTINE_COMPILE,
#endif /* ENABLED (JERRY_BUILTIN_ANNEXB) */

  ECMA_REGEXP_PROTOTYPE_ROUTINE_TEST,
  ECMA_REGEXP_PROTOTYPE_ROUTINE_TO_STRING,
#if ENABLED (JERRY_ESNEXT)
  ECMA_REGEXP_PROTOTYPE_ROUTINE_GET_SOURCE,
  ECMA_REGEXP_PROTOTYPE_ROUTINE_GET_FLAGS,

  ECMA_REGEXP_PROTOTYPE_ROUTINE_GET_GLOBAL,
  ECMA_REGEXP_PROTOTYPE_ROUTINE_GET_IGNORE_CASE,
  ECMA_REGEXP_PROTOTYPE_ROUTINE_GET_MULTILINE,
  ECMA_REGEXP_PROTOTYPE_ROUTINE_GET_STICKY,
  ECMA_REGEXP_PROTOTYPE_ROUTINE_GET_UNICODE,
  ECMA_REGEXP_PROTOTYPE_ROUTINE_GET_DOT_ALL,

  ECMA_REGEXP_PROTOTYPE_ROUTINE_SYMBOL_SEARCH,
  ECMA_REGEXP_PROTOTYPE_ROUTINE_SYMBOL_MATCH,
  ECMA_REGEXP_PROTOTYPE_ROUTINE_SYMBOL_REPLACE,
  ECMA_REGEXP_PROTOTYPE_ROUTINE_SYMBOL_SPLIT,
#endif /* ENABLED (JERRY_ESNEXT) */
};

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

#if ENABLED (JERRY_ESNEXT)
/**
 * Helper function to retrieve the flags associated with a RegExp object
 *
 * @return ECMA_VALUE_{TRUE,FALSE} depends on whether the given flag is present.
 */
static ecma_value_t
ecma_builtin_regexp_prototype_flags_helper (ecma_extended_object_t *re_obj_p, /**< this object */
                                            uint16_t builtin_routine_id) /**< id of the flag */
{
  re_compiled_code_t *bc_p = ECMA_GET_INTERNAL_VALUE_POINTER (re_compiled_code_t,
                                                              re_obj_p->u.class_prop.u.value);

  uint16_t flags = bc_p->header.status_flags;

  static const uint8_t re_flags[] =
  {
    RE_FLAG_GLOBAL,
    RE_FLAG_IGNORE_CASE,
    RE_FLAG_MULTILINE,
    RE_FLAG_STICKY,
    RE_FLAG_UNICODE,
    RE_FLAG_DOTALL,
  };

  uint16_t offset = (uint16_t) (builtin_routine_id - ECMA_REGEXP_PROTOTYPE_ROUTINE_GET_GLOBAL);
  return ecma_make_boolean_value ((flags & re_flags[offset]) != 0);
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
ecma_builtin_regexp_prototype_get_flags (ecma_object_t *object_p) /**< this object */
{
  static const lit_magic_string_id_t flag_lit_ids[] =
  {
    LIT_MAGIC_STRING_GLOBAL,
    LIT_MAGIC_STRING_IGNORECASE_UL,
    LIT_MAGIC_STRING_MULTILINE,
    LIT_MAGIC_STRING_DOTALL,
    LIT_MAGIC_STRING_UNICODE,
    LIT_MAGIC_STRING_STICKY
  };

  static const lit_utf8_byte_t flag_chars[] =
  {
    LIT_CHAR_LOWERCASE_G,
    LIT_CHAR_LOWERCASE_I,
    LIT_CHAR_LOWERCASE_M,
    LIT_CHAR_LOWERCASE_S,
    LIT_CHAR_LOWERCASE_U,
    LIT_CHAR_LOWERCASE_Y
  };

  ecma_stringbuilder_t builder = ecma_stringbuilder_create ();
  for (uint32_t i = 0; i < sizeof (flag_lit_ids) / sizeof (lit_magic_string_id_t); i++)
  {
    ecma_value_t result = ecma_op_object_get_by_magic_id (object_p, flag_lit_ids[i]);
    if (ECMA_IS_VALUE_ERROR (result))
    {
      ecma_stringbuilder_destroy (&builder);
      return result;
    }

    if (ecma_op_to_boolean (result))
    {
      ecma_stringbuilder_append_byte (&builder, flag_chars[i]);
    }

    ecma_free_value (result);
  }

  return ecma_make_string_value (ecma_stringbuilder_finalize (&builder));
} /* ecma_builtin_regexp_prototype_get_flags */

/**
 * The EscapeRegExpPattern method.
 *
 * See also:
 *          ECMA-262 v6, 21.2.3.2.4
 *
 * @return ecma_value_t
 */
static ecma_value_t
ecma_op_escape_regexp_pattern (ecma_string_t *pattern_str_p) /**< RegExp pattern */
{
  ecma_stringbuilder_t builder = ecma_stringbuilder_create ();

  ECMA_STRING_TO_UTF8_STRING (pattern_str_p, pattern_start_p, pattern_start_size);

  const lit_utf8_byte_t *pattern_str_curr_p = pattern_start_p;
  const lit_utf8_byte_t *pattern_str_end_p = pattern_start_p + pattern_start_size;

  while (pattern_str_curr_p < pattern_str_end_p)
  {
    ecma_char_t c = lit_cesu8_read_next (&pattern_str_curr_p);

    switch (c)
    {
      case LIT_CHAR_SLASH:
      {
        ecma_stringbuilder_append_raw (&builder, (const lit_utf8_byte_t *) "\\/", 2);
        break;
      }
      case LIT_CHAR_LF:
      {
        ecma_stringbuilder_append_raw (&builder, (const lit_utf8_byte_t *) "\\n", 2);
        break;
      }
      case LIT_CHAR_CR:
      {
        ecma_stringbuilder_append_raw (&builder, (const lit_utf8_byte_t *) "\\r", 2);
        break;
      }
      case LIT_CHAR_LS:
      {
        ecma_stringbuilder_append_raw (&builder, (const lit_utf8_byte_t *) "\\u2028", 6);
        break;
      }
      case LIT_CHAR_PS:
      {
        ecma_stringbuilder_append_raw (&builder, (const lit_utf8_byte_t *) "\\u2029", 6);
        break;
      }
      case LIT_CHAR_BACKSLASH:
      {
        JERRY_ASSERT (pattern_str_curr_p < pattern_str_end_p);
        ecma_stringbuilder_append_char (&builder, LIT_CHAR_BACKSLASH);
        ecma_stringbuilder_append_char (&builder, lit_cesu8_read_next (&pattern_str_curr_p));
        break;
      }
      default:
      {
        ecma_stringbuilder_append_char (&builder, c);
        break;
      }
    }
  }

  ECMA_FINALIZE_UTF8_STRING (pattern_start_p, pattern_start_size);

  return ecma_make_string_value (ecma_stringbuilder_finalize (&builder));
} /* ecma_op_escape_regexp_pattern */

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
ecma_builtin_regexp_prototype_get_source (ecma_extended_object_t *re_obj_p) /**< this argument */
{
  re_compiled_code_t *bc_p = ECMA_GET_INTERNAL_VALUE_POINTER (re_compiled_code_t,
                                                              re_obj_p->u.class_prop.u.value);

  return ecma_op_escape_regexp_pattern (ecma_get_string_from_value (bc_p->source));
} /* ecma_builtin_regexp_prototype_get_source */
#endif /* ENABLED (JERRY_ESNEXT) */

#if ENABLED (JERRY_BUILTIN_ANNEXB)
/**
 * The RegExp.prototype object's 'compile' routine
 *
 * See also:
 *          ECMA-262 v11, B.2.5.1
 *
 * @return undefined        - if compiled successfully
 *         error ecma value - otherwise
 *
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_regexp_prototype_compile (ecma_value_t this_arg, /**< this */
                                       ecma_value_t pattern_arg, /**< pattern or RegExp object */
                                       ecma_value_t flags_arg) /**< flags */
{
#if !ENABLED (JERRY_ESNEXT)
  if (ecma_get_object_from_value (this_arg) == ecma_builtin_get (ECMA_BUILTIN_ID_REGEXP_PROTOTYPE))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("'this' is not a RegExp object"));
  }
#endif /* !ENABLED (JERRY_ESNEXT) */

  ecma_object_t *this_obj_p = ecma_get_object_from_value (this_arg);
  ecma_extended_object_t *re_obj_p = (ecma_extended_object_t *) this_obj_p;
  re_compiled_code_t *old_bc_p = ECMA_GET_INTERNAL_VALUE_POINTER (re_compiled_code_t,
                                                                  re_obj_p->u.class_prop.u.value);

  ecma_value_t ret_value;

  if (ecma_object_is_regexp_object (pattern_arg))
  {
    if (!ecma_is_value_undefined (flags_arg))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("Invalid argument"));
    }

    ecma_extended_object_t *pattern_obj_p = (ecma_extended_object_t *) ecma_get_object_from_value (pattern_arg);
    re_compiled_code_t *bc_p = ECMA_GET_INTERNAL_VALUE_POINTER (re_compiled_code_t,
                                                                pattern_obj_p->u.class_prop.u.value);

    ret_value = ecma_op_create_regexp_from_bytecode (this_obj_p, bc_p);
  }
  else
  {
    ret_value = ecma_op_create_regexp_from_pattern (this_obj_p, pattern_arg, flags_arg);
  }

  if (!ECMA_IS_VALUE_ERROR (ret_value))
  {
    ecma_value_t status = ecma_builtin_helper_def_prop (this_obj_p,
                                                        ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL),
                                                        ecma_make_uint32_value (0),
                                                        ECMA_PROPERTY_FLAG_WRITABLE | ECMA_PROP_IS_THROW);

    ecma_bytecode_deref ((ecma_compiled_code_t *) old_bc_p);

    if (ECMA_IS_VALUE_ERROR (status))
    {
      return status;
    }

    ecma_ref_object (this_obj_p);
  }

  return ret_value;
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

  ecma_value_t ret_value = ecma_regexp_exec_helper (ecma_get_object_from_value (obj_this), input_str_p);

  ecma_free_value (obj_this);
  ecma_deref_ecma_string (input_str_p);

  return ret_value;
} /* ecma_builtin_regexp_prototype_exec */

/**
 * The RegExp.prototype object's 'test' routine
 *
 * See also:
 *          ECMA-262 v5, 15.10.6.3
 *          ECMA-262 v6, 21.2.5.13
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
#if ENABLED (JERRY_ESNEXT)
  ecma_string_t *arg_str_p = ecma_op_to_string (arg);

  if (JERRY_UNLIKELY (arg_str_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t result = ecma_op_regexp_exec (this_arg, arg_str_p);

  ecma_deref_ecma_string (arg_str_p);

  if (ECMA_IS_VALUE_ERROR (result))
  {
    return result;
  }
#else /* !ENABLED (JERRY_ESNEXT) */
  ecma_value_t result = ecma_builtin_regexp_prototype_exec (this_arg, arg);

  if (ECMA_IS_VALUE_ERROR (result))
  {
    return result;
  }
#endif /* ENABLED (JERRY_ESNEXT) */

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
ecma_builtin_regexp_prototype_to_string (ecma_object_t *object_p) /**< this object */
{
#if ENABLED (JERRY_ESNEXT)
  ecma_value_t result = ecma_op_object_get_by_magic_id (object_p, LIT_MAGIC_STRING_SOURCE);
  if (ECMA_IS_VALUE_ERROR (result))
  {
    return result;
  }

  ecma_string_t *source_p = ecma_op_to_string (result);
  ecma_free_value (result);

  if (source_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  result = ecma_op_object_get_by_magic_id (object_p, LIT_MAGIC_STRING_FLAGS);
  if (ECMA_IS_VALUE_ERROR (result))
  {
    ecma_deref_ecma_string (source_p);
    return result;
  }

  ecma_string_t *flags_p = ecma_op_to_string (result);
  ecma_free_value (result);

  if (flags_p == NULL)
  {
    ecma_deref_ecma_string (source_p);
    return ECMA_VALUE_ERROR;
  }

  ecma_stringbuilder_t builder = ecma_stringbuilder_create ();
  ecma_stringbuilder_append_byte (&builder, LIT_CHAR_SLASH);
  ecma_stringbuilder_append (&builder, source_p);
  ecma_stringbuilder_append_byte (&builder, LIT_CHAR_SLASH);
  ecma_stringbuilder_append (&builder, flags_p);

  ecma_deref_ecma_string (source_p);
  ecma_deref_ecma_string (flags_p);

  return ecma_make_string_value (ecma_stringbuilder_finalize (&builder));
#else /* !ENABLED (JERRY_ESNEXT) */
  ecma_extended_object_t *re_obj_p = (ecma_extended_object_t *) object_p;

  re_compiled_code_t *bc_p = ECMA_GET_INTERNAL_VALUE_POINTER (re_compiled_code_t,
                                                              re_obj_p->u.class_prop.u.value);

  ecma_string_t *source_p = ecma_get_string_from_value (bc_p->source);
  uint16_t flags = bc_p->header.status_flags;

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
#endif /* ENABLED (JERRY_ESNEXT) */
} /* ecma_builtin_regexp_prototype_to_string */

#if ENABLED (JERRY_ESNEXT)
/**
 * Helper function to determine if method is the builtin exec method
 *
 * @return true, if function is the builtin exec method
 *         false, otherwise
 */
JERRY_ALWAYS_INLINE(bool)
ecma_builtin_is_regexp_exec (ecma_extended_object_t *obj_p)
{
  return (ecma_get_object_is_builtin ((ecma_object_t *) obj_p)
          && obj_p->u.built_in.routine_id == ECMA_REGEXP_PROTOTYPE_ROUTINE_EXEC);
} /* ecma_builtin_is_regexp_exec */
#endif /* ENABLED (JERRY_ESNEXT) */

/**
 * Dispatcher of the Regexp built-in's routines
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_regexp_prototype_dispatch_routine (uint8_t builtin_routine_id, /**< built-in wide routine identifier */
                                                ecma_value_t this_arg, /**< 'this' argument value */
                                                const ecma_value_t arguments_list_p[], /**< list of arguments
                                                                                        *   passed to routine */
                                                uint32_t arguments_number) /**< length of arguments' list */
{
  JERRY_UNUSED (arguments_number);

#if !ENABLED (JERRY_ESNEXT)
  bool require_regexp = builtin_routine_id <= ECMA_REGEXP_PROTOTYPE_ROUTINE_TO_STRING;
#else /* ENABLED (JERRY_ESNEXT) */
  bool require_regexp = builtin_routine_id < ECMA_REGEXP_PROTOTYPE_ROUTINE_TEST;
#endif /* ENABLED (JERRY_ESNEXT) */

  ecma_object_t *obj_p = NULL;

  if (ecma_is_value_object (this_arg))
  {
    /* 2. */
    obj_p = ecma_get_object_from_value (this_arg);

    if (require_regexp && !ecma_object_class_is (obj_p, LIT_MAGIC_STRING_REGEXP_UL))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("'this' is not a RegExp object"));
    }
  }
  /* 1. */
  else
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("'this' is not an object"));
  }

  JERRY_ASSERT (obj_p != NULL);

  switch (builtin_routine_id)
  {
#if ENABLED (JERRY_BUILTIN_ANNEXB)
    case ECMA_REGEXP_PROTOTYPE_ROUTINE_COMPILE:
    {
      return ecma_builtin_regexp_prototype_compile (this_arg, arguments_list_p[0], arguments_list_p[1]);
    }
#endif /* ENABLED (JERRY_BUILTIN_ANNEXB) */
    case ECMA_REGEXP_PROTOTYPE_ROUTINE_TEST:
    {
      return ecma_builtin_regexp_prototype_test (this_arg, arguments_list_p[0]);
    }
    case ECMA_REGEXP_PROTOTYPE_ROUTINE_EXEC:
    {
      return ecma_builtin_regexp_prototype_exec (this_arg, arguments_list_p[0]);
    }
    case ECMA_REGEXP_PROTOTYPE_ROUTINE_TO_STRING:
    {
      return ecma_builtin_regexp_prototype_to_string (obj_p);
    }
#if ENABLED (JERRY_ESNEXT)
    case ECMA_REGEXP_PROTOTYPE_ROUTINE_SYMBOL_SEARCH:
    {
      return ecma_regexp_search_helper (this_arg, arguments_list_p[0]);
    }
    case ECMA_REGEXP_PROTOTYPE_ROUTINE_SYMBOL_MATCH:
    {
      return ecma_regexp_match_helper (this_arg, arguments_list_p[0]);
    }
    case ECMA_REGEXP_PROTOTYPE_ROUTINE_SYMBOL_REPLACE:
    {
      return ecma_regexp_replace_helper (this_arg, arguments_list_p[0], arguments_list_p[1]);
    }
    case ECMA_REGEXP_PROTOTYPE_ROUTINE_SYMBOL_SPLIT:
    {
      return ecma_regexp_split_helper (this_arg, arguments_list_p[0], arguments_list_p[1]);
    }
    case ECMA_REGEXP_PROTOTYPE_ROUTINE_GET_FLAGS:
    {
      return ecma_builtin_regexp_prototype_get_flags (obj_p);
    }
    case ECMA_REGEXP_PROTOTYPE_ROUTINE_GET_SOURCE:
    {
      if (!ecma_object_class_is (obj_p, LIT_MAGIC_STRING_REGEXP_UL))
      {
        if (ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_REGEXP_PROTOTYPE))
        {
          return ecma_make_magic_string_value (LIT_MAGIC_STRING_EMPTY_NON_CAPTURE_GROUP);
        }

        return ecma_raise_type_error (ECMA_ERR_MSG ("'this' is not a RegExp object"));
      }

      ecma_extended_object_t *re_obj_p = (ecma_extended_object_t *) obj_p;
      return ecma_builtin_regexp_prototype_get_source (re_obj_p);
    }
    case ECMA_REGEXP_PROTOTYPE_ROUTINE_GET_GLOBAL:
    case ECMA_REGEXP_PROTOTYPE_ROUTINE_GET_IGNORE_CASE:
    case ECMA_REGEXP_PROTOTYPE_ROUTINE_GET_MULTILINE:
    case ECMA_REGEXP_PROTOTYPE_ROUTINE_GET_STICKY:
    case ECMA_REGEXP_PROTOTYPE_ROUTINE_GET_UNICODE:
    case ECMA_REGEXP_PROTOTYPE_ROUTINE_GET_DOT_ALL:
    {
      if (!ecma_object_class_is (obj_p, LIT_MAGIC_STRING_REGEXP_UL))
      {
        if (ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_REGEXP_PROTOTYPE))
        {
          return ECMA_VALUE_UNDEFINED;
        }

        return ecma_raise_type_error (ECMA_ERR_MSG ("'this' is not a RegExp object"));
      }

      ecma_extended_object_t *re_obj_p = (ecma_extended_object_t *) obj_p;
      return ecma_builtin_regexp_prototype_flags_helper (re_obj_p, builtin_routine_id);
    }
#endif /* ENABLED (JERRY_ESNEXT) */
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* ecma_builtin_regexp_prototype_dispatch_routine */
/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_BUILTIN_REGEXP) */
