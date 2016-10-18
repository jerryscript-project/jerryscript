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

#include "ecma-alloc.h"
#include "ecma-array-object.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"

#ifndef CONFIG_DISABLE_REGEXP_BUILTIN
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

#ifndef CONFIG_DISABLE_ANNEXB_BUILTIN

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
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  if (!ecma_is_value_object (this_arg)
      || !ecma_object_class_is (ecma_get_object_from_value (this_arg), LIT_MAGIC_STRING_REGEXP_UL))
  {
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Incomplete RegExp type"));
  }
  else
  {
    uint16_t flags = 0;

    if (ecma_is_value_object (pattern_arg)
        && ecma_object_class_is (ecma_get_object_from_value (pattern_arg), LIT_MAGIC_STRING_REGEXP_UL))
    {
      if (!ecma_is_value_undefined (flags_arg))
      {
        ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Invalid argument of RegExp compile."));
      }
      else
      {
        /* Compile from existing RegExp pbject. */
        ecma_object_t *target_p = ecma_get_object_from_value (pattern_arg);

        /* Get source. */
        ecma_string_t *magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_SOURCE);
        ecma_value_t source_value = ecma_op_object_get_own_data_prop (target_p, magic_string_p);
        ecma_deref_ecma_string (magic_string_p);
        ecma_string_t *pattern_string_p = ecma_get_string_from_value (source_value);

        /* Get flags. */
        magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_GLOBAL);
        ecma_value_t global_value = ecma_op_object_get_own_data_prop (target_p, magic_string_p);
        ecma_deref_ecma_string (magic_string_p);

        JERRY_ASSERT (ecma_is_value_boolean (global_value));

        if (ecma_is_value_true (global_value))
        {
          flags |= RE_FLAG_GLOBAL;
        }

        magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_IGNORECASE_UL);
        ecma_value_t ignore_case_value = ecma_op_object_get_own_data_prop (target_p, magic_string_p);
        ecma_deref_ecma_string (magic_string_p);

        JERRY_ASSERT (ecma_is_value_boolean (ignore_case_value));

        if (ecma_is_value_true (ignore_case_value))
        {
          flags |= RE_FLAG_IGNORE_CASE;
        }

        magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_MULTILINE);
        ecma_value_t multiline_value = ecma_op_object_get_own_data_prop (target_p, magic_string_p);
        ecma_deref_ecma_string (magic_string_p);

        JERRY_ASSERT (ecma_is_value_boolean (multiline_value));

        if (ecma_is_value_true (multiline_value))
        {
          flags |= RE_FLAG_MULTILINE;
        }

        ECMA_TRY_CATCH (obj_this, ecma_op_to_object (this_arg), ret_value);
        ecma_object_t *this_obj_p = ecma_get_object_from_value (obj_this);

        /* Get bytecode property. */
        ecma_value_t *bc_prop_p = &(((ecma_extended_object_t *) this_obj_p)->u.class_prop.value);

        /* TODO: We currently have to re-compile the bytecode, because
         * we can't copy it without knowing its length. */
        const re_compiled_code_t *new_bc_p = NULL;
        ecma_value_t bc_comp = re_compile_bytecode (&new_bc_p, pattern_string_p, flags);
        /* Should always succeed, since we're compiling from a source that has been compiled previously. */
        JERRY_ASSERT (ecma_is_value_empty (bc_comp));

        ecma_deref_ecma_string (pattern_string_p);

        re_compiled_code_t *old_bc_p = ECMA_GET_INTERNAL_VALUE_POINTER (re_compiled_code_t, *bc_prop_p);

        if (old_bc_p != NULL)
        {
          /* Free the old bytecode */
          ecma_bytecode_deref ((ecma_compiled_code_t *) old_bc_p);
        }

        ECMA_SET_INTERNAL_VALUE_POINTER (*bc_prop_p, new_bc_p);

        re_initialize_props (this_obj_p, pattern_string_p, flags);

        ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

        ECMA_FINALIZE (obj_this);
      }
    }
    else
    {
      ecma_string_t *pattern_string_p = NULL;

      /* Get source string. */
      if (!ecma_is_value_undefined (pattern_arg))
      {
        ECMA_TRY_CATCH (regexp_str_value,
                        ecma_op_to_string (pattern_arg),
                        ret_value);

        if (ecma_string_is_empty (ecma_get_string_from_value (regexp_str_value)))
        {
          pattern_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_EMPTY_NON_CAPTURE_GROUP);
        }
        else
        {
          pattern_string_p = ecma_get_string_from_value (regexp_str_value);
          ecma_ref_ecma_string (pattern_string_p);
        }

        ECMA_FINALIZE (regexp_str_value);
      }
      else
      {
        pattern_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_EMPTY_NON_CAPTURE_GROUP);
      }

      /* Parse flags. */
      if (ecma_is_value_empty (ret_value) && !ecma_is_value_undefined (flags_arg))
      {
        ECMA_TRY_CATCH (flags_str_value,
                        ecma_op_to_string (flags_arg),
                        ret_value);

        ECMA_TRY_CATCH (flags_dummy,
                        re_parse_regexp_flags (ecma_get_string_from_value (flags_str_value), &flags),
                        ret_value);
        ECMA_FINALIZE (flags_dummy);
        ECMA_FINALIZE (flags_str_value);
      }

      if (ecma_is_value_empty (ret_value))
      {
        ECMA_TRY_CATCH (obj_this, ecma_op_to_object (this_arg), ret_value);
        ecma_object_t *this_obj_p = ecma_get_object_from_value (obj_this);

        ecma_value_t *bc_prop_p = &(((ecma_extended_object_t *) this_obj_p)->u.class_prop.value);

        /* Try to compile bytecode from new source. */
        const re_compiled_code_t *new_bc_p = NULL;
        ECMA_TRY_CATCH (bc_dummy,
                        re_compile_bytecode (&new_bc_p, pattern_string_p, flags),
                        ret_value);

        re_compiled_code_t *old_bc_p = ECMA_GET_INTERNAL_VALUE_POINTER (re_compiled_code_t, *bc_prop_p);

        if (old_bc_p != NULL)
        {
          /* Free the old bytecode */
          ecma_bytecode_deref ((ecma_compiled_code_t *) old_bc_p);
        }

        ECMA_SET_INTERNAL_VALUE_POINTER (*bc_prop_p, new_bc_p);
        re_initialize_props (this_obj_p, pattern_string_p, flags);
        ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

        ECMA_FINALIZE (bc_dummy);

        ECMA_FINALIZE (obj_this);
      }

      if (pattern_string_p != NULL)
      {
        ecma_deref_ecma_string (pattern_string_p);
      }
    }
  }

  return ret_value;
} /* ecma_builtin_regexp_prototype_compile */

#endif /* !CONFIG_DISABLE_ANNEXB_BUILTIN */

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
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  if (!ecma_is_value_object (this_arg)
      || !ecma_object_class_is (ecma_get_object_from_value (this_arg), LIT_MAGIC_STRING_REGEXP_UL))
  {
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Incomplete RegExp type"));
  }
  else
  {
    ECMA_TRY_CATCH (obj_this, ecma_op_to_object (this_arg), ret_value);

    ECMA_TRY_CATCH (input_str_value,
                    ecma_op_to_string (arg),
                    ret_value);

    ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);
    ecma_value_t *bytecode_prop_p = &(((ecma_extended_object_t *) obj_p)->u.class_prop.value);

    void *bytecode_p = ECMA_GET_INTERNAL_VALUE_POINTER (void, *bytecode_prop_p);

    if (bytecode_p == NULL)
    {
      /* Missing bytecode means empty RegExp: '/(?:)/', so always return empty string. */
      ecma_string_t *capture_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);

      ecma_value_t arguments_list[1];
      arguments_list[0] = ecma_make_string_value (capture_str_p);

      ret_value = ecma_op_create_array_object (arguments_list, 1, false);

      ecma_deref_ecma_string (capture_str_p);

      re_set_result_array_properties (ecma_get_object_from_value (ret_value),
                                      ecma_get_string_from_value (input_str_value),
                                      1,
                                      0);
    }
    else
    {
      ret_value = ecma_regexp_exec_helper (obj_this, input_str_value, false);
    }

    ECMA_FINALIZE (input_str_value);
    ECMA_FINALIZE (obj_this);
  }

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
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_TRY_CATCH (match_value,
                  ecma_builtin_regexp_prototype_exec (this_arg, arg),
                  ret_value);

  ret_value = ecma_make_boolean_value (!ecma_is_value_null (match_value));

  ECMA_FINALIZE (match_value);

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
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  if (!ecma_is_value_object (this_arg)
      || !ecma_object_class_is (ecma_get_object_from_value (this_arg), LIT_MAGIC_STRING_REGEXP_UL))
  {
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("Incomplete RegExp type"));
  }
  else
  {
    ECMA_TRY_CATCH (obj_this,
                    ecma_op_to_object (this_arg),
                    ret_value);

    ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);

    /* Get RegExp source from the source property */
    ecma_string_t *magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_SOURCE);
    ecma_value_t source_value = ecma_op_object_get_own_data_prop (obj_p, magic_string_p);
    ecma_deref_ecma_string (magic_string_p);

    ecma_string_t *src_sep_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_SLASH_CHAR);
    ecma_string_t *source_str_p = ecma_get_string_from_value (source_value);
    ecma_string_t *output_str_p = ecma_concat_ecma_strings (src_sep_str_p, source_str_p);
    ecma_deref_ecma_string (source_str_p);

    ecma_string_t *concat_p = ecma_concat_ecma_strings (output_str_p, src_sep_str_p);
    ecma_deref_ecma_string (src_sep_str_p);
    ecma_deref_ecma_string (output_str_p);
    output_str_p = concat_p;

    /* Check the global flag */
    magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_GLOBAL);
    ecma_value_t global_value = ecma_op_object_get_own_data_prop (obj_p, magic_string_p);
    ecma_deref_ecma_string (magic_string_p);

    JERRY_ASSERT (ecma_is_value_boolean (global_value));

    if (ecma_is_value_true (global_value))
    {
      ecma_string_t *g_flag_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_G_CHAR);
      concat_p = ecma_concat_ecma_strings (output_str_p, g_flag_str_p);
      ecma_deref_ecma_string (output_str_p);
      ecma_deref_ecma_string (g_flag_str_p);
      output_str_p = concat_p;
    }

    /* Check the ignoreCase flag */
    magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_IGNORECASE_UL);
    ecma_value_t ignore_case_value = ecma_op_object_get_own_data_prop (obj_p, magic_string_p);
    ecma_deref_ecma_string (magic_string_p);

    JERRY_ASSERT (ecma_is_value_boolean (ignore_case_value));

    if (ecma_is_value_true (ignore_case_value))
    {
      ecma_string_t *ic_flag_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_I_CHAR);
      concat_p = ecma_concat_ecma_strings (output_str_p, ic_flag_str_p);
      ecma_deref_ecma_string (output_str_p);
      ecma_deref_ecma_string (ic_flag_str_p);
      output_str_p = concat_p;
    }

    /* Check the multiline flag */
    magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_MULTILINE);
    ecma_value_t multiline_value = ecma_op_object_get_own_data_prop (obj_p, magic_string_p);
    ecma_deref_ecma_string (magic_string_p);

    JERRY_ASSERT (ecma_is_value_boolean (multiline_value));

    if (ecma_is_value_true (multiline_value))
    {
      ecma_string_t *m_flag_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_M_CHAR);
      concat_p = ecma_concat_ecma_strings (output_str_p, m_flag_str_p);
      ecma_deref_ecma_string (output_str_p);
      ecma_deref_ecma_string (m_flag_str_p);
      output_str_p = concat_p;
    }

    ret_value = ecma_make_string_value (output_str_p);

    ECMA_FINALIZE (obj_this);
  }

  return ret_value;
} /* ecma_builtin_regexp_prototype_to_string */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */
