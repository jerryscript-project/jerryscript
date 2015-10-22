/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-objects.h"
#include "jrt.h"

/** \addtogroup ecma ECMA
 * @{
 */

/**
 * \addtogroup lexicalenvironment Lexical environment
 * @{
 */

/**
 * \addtogroup globallexicalenvironment Global lexical environment
 * @{
 */

/**
 * Global lexical environment
 *
 * See also: ECMA-262 v5, 10.2.3
 */
ecma_object_t* ecma_global_lex_env_p = NULL;

/**
 * Initialize Global environment
 */
void
ecma_init_environment (void)
{
#ifdef CONFIG_ECMA_GLOBAL_ENVIRONMENT_DECLARATIVE
  ecma_global_lex_env_p = ecma_create_decl_lex_env (NULL);
#else /* !CONFIG_ECMA_GLOBAL_ENVIRONMENT_DECLARATIVE */
  ecma_object_t *glob_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_GLOBAL);

  ecma_global_lex_env_p = ecma_create_object_lex_env (NULL, glob_obj_p, false);

  ecma_deref_object (glob_obj_p);
#endif /* !CONFIG_ECMA_GLOBAL_ENVIRONMENT_DECLARATIVE */
} /* ecma_init_environment */

/**
 * Finalize Global environment
 */
void
ecma_finalize_environment (void)
{
  ecma_deref_object (ecma_global_lex_env_p);
  ecma_global_lex_env_p = NULL;
} /* ecma_finalize_environment */

/**
 * Get reference to Global lexical environment
 *
 * @return pointer to the object's instance
 */
ecma_object_t*
ecma_get_global_environment (void)
{
  ecma_ref_object (ecma_global_lex_env_p);

  return ecma_global_lex_env_p;
} /* ecma_get_global_environment */

/**
 * Figure out whether the lexical environment is global.
 *
 * @return true - if lexical environment is object-bound and corresponding object is global object,
 *         false - otherwise.
 */
bool
ecma_is_lexical_environment_global (ecma_object_t *lex_env_p) /**< lexical environment */
{
  JERRY_ASSERT (lex_env_p != NULL
                && ecma_is_lexical_environment (lex_env_p));

  return (lex_env_p == ecma_global_lex_env_p);
} /* ecma_is_lexical_environment_global */

/**
 * @}
 */

/**
 * HasBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 *
 * @return true / false
 */
bool
ecma_op_has_binding (ecma_object_t *lex_env_p, /**< lexical environment */
                     ecma_string_t *name_p) /**< argument N */
{
  JERRY_ASSERT (lex_env_p != NULL
                && ecma_is_lexical_environment (lex_env_p));

  if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
  {
    ecma_property_t *property_p = ecma_find_named_property (lex_env_p, name_p);

    return (property_p != NULL);
  }
  else
  {
    JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND);

    ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

    return (ecma_op_object_get_property (binding_obj_p, name_p) != NULL);
  }
} /* ecma_op_has_binding */

/**
 * CreateMutableBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_create_mutable_binding (ecma_object_t *lex_env_p, /**< lexical environment */
                                ecma_string_t *name_p, /**< argument N */
                                bool is_deletable) /**< argument D */
{
  JERRY_ASSERT (lex_env_p != NULL
                && ecma_is_lexical_environment (lex_env_p));
  JERRY_ASSERT (name_p != NULL);

  if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
  {
    ecma_create_named_data_property (lex_env_p,
                                     name_p,
                                     true, false, is_deletable);
  }
  else
  {
    JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND);

    ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

    ecma_completion_value_t completion;
    completion = ecma_builtin_helper_def_prop (binding_obj_p,
                                               name_p,
                                               ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED),
                                               true, /* Writable */
                                               true, /* Enumerable */
                                               is_deletable, /* Configurable */
                                               true); /* Failure handling */

    if (ecma_is_completion_value_throw (completion))
    {
      return completion;
    }
    else
    {
      JERRY_ASSERT (ecma_is_completion_value_normal_true (completion)
                    || ecma_is_completion_value_normal_false (completion));
    }
  }

  return ecma_make_empty_completion_value ();
} /* ecma_op_create_mutable_binding */

/**
 * SetMutableBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_op_set_mutable_binding (ecma_object_t *lex_env_p, /**< lexical environment */
                             ecma_string_t *name_p, /**< argument N */
                             ecma_value_t value, /**< argument V */
                             bool is_strict) /**< argument S */
{
  JERRY_ASSERT (lex_env_p != NULL
                && ecma_is_lexical_environment (lex_env_p));
  JERRY_ASSERT (name_p != NULL);

  if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
  {
#ifndef JERRY_NDEBUG
# ifdef CONFIG_ECMA_COMPACT_PROFILE
    bool is_equal = false;

    ecma_string_t *arguments_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_ARGUMENTS);
    if (ecma_compare_ecma_strings (name_p, arguments_magic_string_p))
    {
      is_equal = true;
    }
    ecma_deref_ecma_string (arguments_magic_string_p);

    JERRY_ASSERT (!is_equal);

    if (is_equal)
    {
      return ecma_make_throw_obj_completion_value (ecma_builtin_get (ECMA_BUILTIN_ID_COMPACT_PROFILE_ERROR));
    }
# endif /* CONFIG_ECMA_COMPACT_PROFILE */
#endif /* !JERRY_NDEBUG */

    ecma_property_t *property_p = ecma_get_named_data_property (lex_env_p, name_p);

    if (ecma_is_property_writable (property_p))
    {
      ecma_named_data_property_assign_value (lex_env_p, property_p, value);
    }
    else if (is_strict)
    {
      return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
  }
  else
  {
    JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND);

    ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

    ecma_completion_value_t completion = ecma_op_object_put (binding_obj_p,
                                                             name_p,
                                                             value,
                                                             is_strict);

    if (ecma_is_completion_value_throw (completion))
    {
      return completion;
    }
    else
    {
      JERRY_ASSERT (ecma_is_completion_value_normal_true (completion)
                    || ecma_is_completion_value_normal_false (completion));
    }
  }

  return ecma_make_empty_completion_value ();
} /* ecma_op_set_mutable_binding */

/**
 * GetBindingValue operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_op_get_binding_value (ecma_object_t *lex_env_p, /**< lexical environment */
                           ecma_string_t *name_p, /**< argument N */
                           bool is_strict) /**< argument S */
{
  JERRY_ASSERT (lex_env_p != NULL
                && ecma_is_lexical_environment (lex_env_p));
  JERRY_ASSERT (name_p != NULL);

  if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
  {
#ifndef JERRY_NDEBUG
# ifdef CONFIG_ECMA_COMPACT_PROFILE
    bool is_equal = false;

    ecma_string_t *arguments_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_ARGUMENTS);
    if (ecma_compare_ecma_strings (name_p, arguments_magic_string_p))
    {
      is_equal = true;
    }
    ecma_deref_ecma_string (arguments_magic_string_p);

    JERRY_ASSERT (!is_equal);

    if (is_equal)
    {
      return ecma_make_throw_obj_completion_value (ecma_builtin_get (ECMA_BUILTIN_ID_COMPACT_PROFILE_ERROR));
    }
# endif /* CONFIG_ECMA_COMPACT_PROFILE */
#endif /* !JERRY_NDEBUG */

    ecma_property_t *property_p = ecma_get_named_data_property (lex_env_p, name_p);

    ecma_value_t prop_value = ecma_get_named_data_property_value (property_p);

    /* is the binding mutable? */
    if (!ecma_is_property_writable (property_p)
        && ecma_is_value_empty (prop_value))
    {
      /* unitialized immutable binding */
      if (is_strict)
      {
        return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_REFERENCE));
      }
      else
      {
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
      }
    }

    return ecma_make_normal_completion_value (ecma_copy_value (prop_value, true));
  }
  else
  {
    JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND);

    ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

    if (ecma_op_object_get_property (binding_obj_p, name_p) == NULL)
    {
      if (is_strict)
      {
        return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_REFERENCE));
      }
      else
      {
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
      }
    }

    return ecma_op_object_get (binding_obj_p, name_p);
  }
} /* ecma_op_get_binding_value */

/**
 * DeleteBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 *
 * @return completion value
 *         Return value is simple and so need not be freed.
 *         However, ecma_free_completion_value may be called for it, but it is a no-op.
 */
ecma_completion_value_t
ecma_op_delete_binding (ecma_object_t *lex_env_p, /**< lexical environment */
                        ecma_string_t *name_p) /**< argument N */
{
  JERRY_ASSERT (lex_env_p != NULL
                && ecma_is_lexical_environment (lex_env_p));
  JERRY_ASSERT (name_p != NULL);


  if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
  {
    ecma_property_t *prop_p = ecma_find_named_property (lex_env_p, name_p);
    ecma_simple_value_t ret_val;

    if (prop_p == NULL)
    {
      ret_val = ECMA_SIMPLE_VALUE_TRUE;
    }
    else
    {
      JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDDATA);

      if (!ecma_is_property_configurable (prop_p))
      {
        ret_val = ECMA_SIMPLE_VALUE_FALSE;
      }
      else
      {
        ecma_delete_property (lex_env_p, prop_p);

        ret_val = ECMA_SIMPLE_VALUE_TRUE;
      }
    }

    return ecma_make_simple_completion_value (ret_val);
  }
  else
  {
    JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND);

    ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

    return ecma_op_object_delete (binding_obj_p, name_p, false);
  }
} /* ecma_op_delete_binding */

/**
 * ImplicitThisValue operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_op_implicit_this_value (ecma_object_t *lex_env_p) /**< lexical environment */
{
  JERRY_ASSERT (lex_env_p != NULL
                && ecma_is_lexical_environment (lex_env_p));

  if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
  {
    return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  }
  else
  {
    JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND);

    if (ecma_get_lex_env_provide_this (lex_env_p))
    {
      ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);
      ecma_ref_object (binding_obj_p);

      return ecma_make_normal_completion_value (ecma_make_object_value (binding_obj_p));
    }
    else
    {
      return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
  }
} /* ecma_op_implicit_this_value */

/**
 * CreateImmutableBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 */
void
ecma_op_create_immutable_binding (ecma_object_t *lex_env_p, /**< lexical environment */
                                  ecma_string_t *name_p) /**< argument N */
{
  JERRY_ASSERT (lex_env_p != NULL
                && ecma_is_lexical_environment (lex_env_p));
  JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE);

  /*
   * Warning:
   *         Whether immutable bindings are deletable seems not to be defined by ECMA v5.
   */
  ecma_property_t *prop_p = ecma_create_named_data_property (lex_env_p,
                                                             name_p,
                                                             false, false, false);

  JERRY_ASSERT (ecma_is_value_undefined (ecma_get_named_data_property_value (prop_p)));

  ecma_set_named_data_property_value (prop_p,
                                      ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY));
} /* ecma_op_create_immutable_binding */

/**
 * InitializeImmutableBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 */
void
ecma_op_initialize_immutable_binding (ecma_object_t *lex_env_p, /**< lexical environment */
                                      ecma_string_t *name_p, /**< argument N */
                                      ecma_value_t value) /**< argument V */
{
  JERRY_ASSERT (lex_env_p != NULL
                && ecma_is_lexical_environment (lex_env_p));
  JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE);

  ecma_property_t *prop_p = ecma_get_named_data_property (lex_env_p, name_p);

  /* The binding must be unitialized immutable binding */
  JERRY_ASSERT (!ecma_is_property_writable (prop_p)
                && ecma_is_value_empty (ecma_get_named_data_property_value (prop_p)));

  ecma_named_data_property_assign_value (lex_env_p, prop_p, value);
} /* ecma_op_initialize_immutable_binding */

/**
 * @}
 * @}
 */
