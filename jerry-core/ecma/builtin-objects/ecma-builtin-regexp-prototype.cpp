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

#include "ecma-alloc.h"
#include "ecma-array-object.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN
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
 * \addtogroup regexp ECMA RegExp.prototype object built-in
 * @{
 */

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ANNEXB_BUILTIN

/**
 * The RegExp.prototype object's 'compile' routine
 *
 * See also:
 *          ECMA-262 v5, B.2.5.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_regexp_prototype_compile (ecma_value_t this_arg, /**< this argument */
                                       ecma_value_t pattern_arg, /**< pattern or RegExp object */
                                       ecma_value_t flags_arg) /**< flags */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  ecma_object_t *this_obj_p = ecma_get_object_from_value (this_arg);

  if (ecma_object_get_class_name (this_obj_p) != LIT_MAGIC_STRING_REGEXP_UL)
  {
    /* Compile can only be called on RegExp objects. */
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    ecma_string_t *pattern_string_p = NULL;
    uint8_t flags = 0;

    if (ecma_is_value_object (pattern_arg)
        && ecma_object_get_class_name (ecma_get_object_from_value (pattern_arg)) == LIT_MAGIC_STRING_REGEXP_UL)
    {
      if (!ecma_is_value_undefined (flags_arg))
      {
        ret_value = ecma_raise_type_error ("Invalid argument of RegExp compile.");
      }
      else
      {
        /* Compile from existing RegExp pbject. */
        ecma_object_t *target_p = ecma_get_object_from_value (pattern_arg);

        /* Get source. */
        ecma_string_t *magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_SOURCE);
        ecma_property_t *prop_p = ecma_get_named_data_property (target_p, magic_string_p);
        pattern_string_p = ecma_get_string_from_value (ecma_get_named_data_property_value (prop_p));
        ecma_deref_ecma_string (magic_string_p);

        /* Get flags. */
        magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_GLOBAL);
        prop_p = ecma_get_named_data_property (target_p, magic_string_p);

        if (ecma_is_value_true (ecma_get_named_data_property_value (prop_p)))
        {
          flags |= RE_FLAG_GLOBAL;
        }

        ecma_deref_ecma_string (magic_string_p);
        magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_IGNORECASE_UL);
        prop_p = ecma_get_named_data_property (target_p, magic_string_p);

        if (ecma_is_value_true (ecma_get_named_data_property_value (prop_p)))
        {
          flags |= RE_FLAG_IGNORE_CASE;
        }

        ecma_deref_ecma_string (magic_string_p);
        magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_MULTILINE);
        prop_p = ecma_get_named_data_property (target_p, magic_string_p);

        if (ecma_is_value_true (ecma_get_named_data_property_value (prop_p)))
        {
          flags |= RE_FLAG_MULTILINE;
        }

        ecma_deref_ecma_string (magic_string_p);

        /* Get bytecode property. */
        ecma_property_t *bc_prop_p = ecma_get_internal_property (this_obj_p,
                                                                 ECMA_INTERNAL_PROPERTY_REGEXP_BYTECODE);

        FIXME ("We currently have to re-compile the bytecode, because we can't copy it without knowing its length.")
        re_bytecode_t *new_bc_p = NULL;
        ecma_completion_value_t bc_comp = re_compile_bytecode (&new_bc_p, pattern_string_p, flags);
        /* Should always succeed, since we're compiling from a source that has been compiled previously. */
        JERRY_ASSERT (ecma_is_completion_value_empty (bc_comp));

        re_bytecode_t *old_bc_p = ECMA_GET_POINTER (re_bytecode_t,
                                                    bc_prop_p->u.internal_property.value);
        if (old_bc_p != NULL)
        {
          mem_heap_free_block (old_bc_p);
        }

        ECMA_SET_POINTER (bc_prop_p->u.internal_property.value, new_bc_p);

        re_initialize_props (this_obj_p, pattern_string_p, flags);

        ret_value = ecma_make_normal_completion_value (ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED));
      }
    }
    else
    {
      /* Get source string. */
      if (!ecma_is_value_undefined (pattern_arg))
      {
        ECMA_TRY_CATCH (regexp_str_value,
                        ecma_op_to_string (pattern_arg),
                        ret_value);

        if (ecma_string_get_length (ecma_get_string_from_value (regexp_str_value)) == 0)
        {
          pattern_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_EMPTY_NON_CAPTURE_GROUP);
        }
        else
        {
          pattern_string_p = ecma_copy_or_ref_ecma_string (ecma_get_string_from_value (regexp_str_value));
        }

        ECMA_FINALIZE (regexp_str_value);
      }
      else
      {
        pattern_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_EMPTY_NON_CAPTURE_GROUP);
      }

      /* Parse flags. */
      if (ecma_is_completion_value_empty (ret_value) && !ecma_is_value_undefined (flags_arg))
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

      if (ecma_is_completion_value_empty (ret_value))
      {
        ecma_property_t *bc_prop_p = ecma_get_internal_property (this_obj_p,
                                                                 ECMA_INTERNAL_PROPERTY_REGEXP_BYTECODE);
        /* Try to compile bytecode from new source. */
        re_bytecode_t *new_bc_p = NULL;
        ECMA_TRY_CATCH (bc_dummy, re_compile_bytecode (&new_bc_p, pattern_string_p, flags), ret_value);


        re_bytecode_t *old_bc_p = ECMA_GET_POINTER (re_bytecode_t,
                                                    bc_prop_p->u.internal_property.value);

        if (old_bc_p != NULL)
        {
          /* Replace old bytecode with new one. */
          mem_heap_free_block (old_bc_p);
        }

        ECMA_SET_POINTER (bc_prop_p->u.internal_property.value, new_bc_p);
        re_initialize_props (this_obj_p, pattern_string_p, flags);
        ret_value = ecma_make_normal_completion_value (ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED));

        ECMA_FINALIZE (bc_dummy);
      }

      if (pattern_string_p != NULL)
      {
        ecma_deref_ecma_string (pattern_string_p);
      }
    }
  }

  return ret_value;
} /* ecma_builtin_regexp_prototype_compile */

#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ANNEXB_BUILTIN */

/**
 * The RegExp.prototype object's 'exec' routine
 *
 * See also:
 *          ECMA-262 v5, 15.10.6.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_regexp_prototype_exec (ecma_value_t this_arg, /**< this argument */
                                    ecma_value_t arg) /**< routine's argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (!ecma_is_value_object (this_arg)
      || ecma_object_get_class_name (ecma_get_object_from_value (this_arg)) != LIT_MAGIC_STRING_REGEXP_UL)
  {
    ret_value = ecma_raise_type_error ("Incomplete RegExp type");
  }
  else
  {
    ECMA_TRY_CATCH (obj_this, ecma_op_to_object (this_arg), ret_value);

    ECMA_TRY_CATCH (input_str_value,
                    ecma_op_to_string (arg),
                    ret_value);

    ecma_property_t *bytecode_prop_p;
    ecma_object_t *obj_p = ecma_get_object_from_completion_value (obj_this);
    bytecode_prop_p = ecma_get_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_REGEXP_BYTECODE);

    void *bytecode_p = ECMA_GET_POINTER (void, bytecode_prop_p->u.internal_property.value);

    if (bytecode_p == NULL)
    {
      /* Missing bytecode means empty RegExp: '/(?:)/', so always return empty string. */
      ecma_completion_value_t result_array = ecma_op_create_array_object (0, 0, false);
      ecma_object_t *result_array_obj_p = ecma_get_object_from_completion_value (result_array);
      ecma_string_t *input_str_p = ecma_get_string_from_value (input_str_value);
      re_set_result_array_properties (result_array_obj_p,
                                      input_str_p,
                                      1,
                                      0);

      ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (0);
      ecma_string_t *capture_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);

      ECMA_TRY_CATCH (put_res_value,
                      ecma_op_object_put (result_array_obj_p,
                                          index_str_p,
                                          ecma_make_string_value (capture_str_p),
                                          true),
                      ret_value);

      ret_value = result_array;

      ECMA_FINALIZE (put_res_value)

      ecma_deref_ecma_string (capture_str_p);
      ecma_deref_ecma_string (index_str_p);
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
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_regexp_prototype_test (ecma_value_t this_arg, /**< this argument */
                                    ecma_value_t arg) /**< routine's argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (match_value,
                  ecma_builtin_regexp_prototype_exec (this_arg, arg),
                  ret_value);

  if (ecma_is_value_null (match_value))
  {
    ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE);
  }
  else
  {
    ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
  }

  ECMA_FINALIZE (match_value);

  return ret_value;
} /* ecma_builtin_regexp_prototype_test */

/**
 * The RegExp.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.10.6.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_regexp_prototype_to_string (ecma_value_t this_arg) /**< this argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (!ecma_is_value_object (this_arg)
      || ecma_object_get_class_name (ecma_get_object_from_value (this_arg)) != LIT_MAGIC_STRING_REGEXP_UL)
  {
    ret_value = ecma_raise_type_error ("Incomplete RegExp type");
  }
  else
  {
    ECMA_TRY_CATCH (obj_this,
                    ecma_op_to_object (this_arg),
                    ret_value);

    ecma_object_t *obj_p = ecma_get_object_from_value (obj_this);

    /* Get RegExp source from the source property */
    ecma_string_t *magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_SOURCE);
    ecma_property_t *source_prop_p = ecma_op_object_get_property (obj_p, magic_string_p);
    ecma_deref_ecma_string (magic_string_p);

    ecma_string_t *src_sep_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_SLASH_CHAR);
    ecma_string_t *source_str_p = ecma_get_string_from_value (source_prop_p->u.named_data_property.value);
    ecma_string_t *output_str_p = ecma_concat_ecma_strings (src_sep_str_p, ecma_copy_or_ref_ecma_string (source_str_p));
    ecma_deref_ecma_string (source_str_p);

    ecma_string_t *concat_p = ecma_concat_ecma_strings (output_str_p, src_sep_str_p);
    ecma_deref_ecma_string (src_sep_str_p);
    ecma_deref_ecma_string (output_str_p);
    output_str_p = concat_p;

    /* Check the global flag */
    magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_GLOBAL);
    ecma_property_t *global_prop_p = ecma_op_object_get_property (obj_p, magic_string_p);
    ecma_deref_ecma_string (magic_string_p);

    if (ecma_is_value_true (global_prop_p->u.named_data_property.value))
    {
      ecma_string_t *g_flag_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_G_CHAR);
      concat_p = ecma_concat_ecma_strings (output_str_p, g_flag_str_p);
      ecma_deref_ecma_string (output_str_p);
      ecma_deref_ecma_string (g_flag_str_p);
      output_str_p = concat_p;
    }

    /* Check the ignoreCase flag */
    magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_IGNORECASE_UL);
    ecma_property_t *ignorecase_prop_p = ecma_op_object_get_property (obj_p, magic_string_p);
    ecma_deref_ecma_string (magic_string_p);

    if (ecma_is_value_true (ignorecase_prop_p->u.named_data_property.value))
    {
      ecma_string_t *ic_flag_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_I_CHAR);
      concat_p = ecma_concat_ecma_strings (output_str_p, ic_flag_str_p);
      ecma_deref_ecma_string (output_str_p);
      ecma_deref_ecma_string (ic_flag_str_p);
      output_str_p = concat_p;
    }

    /* Check the global flag */
    magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_MULTILINE);
    ecma_property_t *multiline_prop_p = ecma_op_object_get_property (obj_p, magic_string_p);
    ecma_deref_ecma_string (magic_string_p);

    if (ecma_is_value_true (multiline_prop_p->u.named_data_property.value))
    {
      ecma_string_t *m_flag_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_M_CHAR);
      concat_p = ecma_concat_ecma_strings (output_str_p, m_flag_str_p);
      ecma_deref_ecma_string (output_str_p);
      ecma_deref_ecma_string (m_flag_str_p);
      output_str_p = concat_p;
    }

    ret_value = ecma_make_normal_completion_value (ecma_make_string_value (output_str_p));

    ECMA_FINALIZE (obj_this);
  }

  return ret_value;
} /* ecma_builtin_regexp_prototype_to_string */

/**
 * @}
 * @}
 * @}
 */

#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */
