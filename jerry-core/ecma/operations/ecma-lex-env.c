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

#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-objects.h"
#include "jcontext.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup lexicalenvironment Lexical environment
 * @{
 *
 * \addtogroup globallexicalenvironment Global lexical environment
 * @{
 */

/**
 * Initialize Global environment
 */
void
ecma_init_global_environment (void)
{
  ecma_object_t *glob_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_GLOBAL);

  ecma_object_t *global_lex_env_p = ecma_create_object_lex_env (NULL,
                                                                glob_obj_p,
                                                                ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND);
  ECMA_SET_NON_NULL_POINTER (JERRY_CONTEXT (ecma_global_env_cp), global_lex_env_p);
#if ENABLED (JERRY_ES2015)
  ECMA_SET_NON_NULL_POINTER (JERRY_CONTEXT (ecma_global_scope_cp), global_lex_env_p);
#endif /* ENABLED (JERRY_ES2015) */
} /* ecma_init_global_environment */

/**
 * Finalize Global environment
 */
void
ecma_finalize_global_environment (void)
{
#if ENABLED (JERRY_ES2015)
  if (JERRY_CONTEXT (ecma_global_scope_cp) != JERRY_CONTEXT (ecma_global_env_cp))
  {
    ecma_deref_object (ECMA_GET_NON_NULL_POINTER (ecma_object_t, JERRY_CONTEXT (ecma_global_scope_cp)));
  }
  JERRY_CONTEXT (ecma_global_scope_cp) = JMEM_CP_NULL;
#endif /* ENABLED (JERRY_ES2015) */
  ecma_deref_object (ECMA_GET_NON_NULL_POINTER (ecma_object_t, JERRY_CONTEXT (ecma_global_env_cp)));
  JERRY_CONTEXT (ecma_global_env_cp) = JMEM_CP_NULL;
} /* ecma_finalize_global_environment */

/**
 * Get reference to Global lexical environment
 * without increasing its reference count.
 *
 * @return pointer to the object's instance
 */
ecma_object_t *
ecma_get_global_environment (void)
{
  JERRY_ASSERT (JERRY_CONTEXT (ecma_global_env_cp) != JMEM_CP_NULL);
  return ECMA_GET_NON_NULL_POINTER (ecma_object_t, JERRY_CONTEXT (ecma_global_env_cp));
} /* ecma_get_global_environment */

#if ENABLED (JERRY_ES2015)
/**
 * Create the global lexical block on top of the global environment.
 */
void
ecma_create_global_lexical_block (void)
{
  if (JERRY_CONTEXT (ecma_global_scope_cp) == JERRY_CONTEXT (ecma_global_env_cp))
  {
    ecma_object_t *global_scope_p = ecma_create_decl_lex_env (ecma_get_global_environment ());
    global_scope_p->type_flags_refs |= (uint16_t) ECMA_OBJECT_FLAG_BLOCK;
    ECMA_SET_NON_NULL_POINTER (JERRY_CONTEXT (ecma_global_scope_cp), global_scope_p);
  }
} /* ecma_create_global_lexical_block */
#endif /* ENABLED (JERRY_ES2015) */

/**
 * Get reference to Global lexical scope
 * without increasing its reference count.
 *
 * @return pointer to the object's instance
 */
ecma_object_t *
ecma_get_global_scope (void)
{
#if ENABLED (JERRY_ES2015)
  JERRY_ASSERT (JERRY_CONTEXT (ecma_global_scope_cp) != JMEM_CP_NULL);
  return ECMA_GET_NON_NULL_POINTER (ecma_object_t, JERRY_CONTEXT (ecma_global_scope_cp));
#else /* !ENABLED (JERRY_ES2015) */
  return ecma_get_global_environment ();
#endif /* !ENABLED (JERRY_ES2015) */
} /* ecma_get_global_scope */

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
ecma_value_t
ecma_op_has_binding (ecma_object_t *lex_env_p, /**< lexical environment */
                     ecma_string_t *name_p) /**< argument N */
{
  JERRY_ASSERT (lex_env_p != NULL
                && ecma_is_lexical_environment (lex_env_p));

  ecma_lexical_environment_type_t lex_env_type = ecma_get_lex_env_type (lex_env_p);

  if (lex_env_type == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
  {
    ecma_property_t *property_p = ecma_find_named_property (lex_env_p, name_p);

    return ecma_make_boolean_value (property_p != NULL);
  }

  JERRY_ASSERT (lex_env_type == ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND);

  ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

  return ecma_op_object_has_property (binding_obj_p, name_p);
} /* ecma_op_has_binding */

/**
 * CreateMutableBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_create_mutable_binding (ecma_object_t *lex_env_p, /**< lexical environment */
                                ecma_string_t *name_p, /**< argument N */
                                bool is_deletable) /**< argument D */
{
  JERRY_ASSERT (lex_env_p != NULL
                && ecma_is_lexical_environment (lex_env_p));
  JERRY_ASSERT (name_p != NULL);

  if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
  {
    uint8_t prop_attributes = ECMA_PROPERTY_FLAG_WRITABLE;

    if (is_deletable)
    {
      prop_attributes = (uint8_t) (prop_attributes | ECMA_PROPERTY_FLAG_CONFIGURABLE);
    }

    ecma_create_named_data_property (lex_env_p,
                                     name_p,
                                     prop_attributes,
                                     NULL);
  }
  else
  {
    JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND);

    ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

    if (!ecma_op_ordinary_object_is_extensible (binding_obj_p))
    {
      return ECMA_VALUE_EMPTY;
    }

    const uint32_t flags = ECMA_PROPERTY_ENUMERABLE_WRITABLE | ECMA_IS_THROW;

    ecma_value_t completion = ecma_builtin_helper_def_prop (binding_obj_p,
                                                            name_p,
                                                            ECMA_VALUE_UNDEFINED,
                                                            is_deletable ? flags | ECMA_PROPERTY_FLAG_CONFIGURABLE
                                                                         : flags);

    if (ECMA_IS_VALUE_ERROR (completion))
    {
      return completion;
    }
    else
    {
      JERRY_ASSERT (ecma_is_value_boolean (completion));
    }
  }

  return ECMA_VALUE_EMPTY;
} /* ecma_op_create_mutable_binding */

/**
 * SetMutableBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
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
    ecma_property_t *property_p = ecma_find_named_property (lex_env_p, name_p);

    JERRY_ASSERT (property_p != NULL
                  && ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA);

    if (ecma_is_property_writable (*property_p))
    {
      ecma_named_data_property_assign_value (lex_env_p, ECMA_PROPERTY_VALUE_PTR (property_p), value);
    }
#if ENABLED (JERRY_ES2015)
    else if (ecma_is_property_enumerable (*property_p))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("Constant bindings cannot be reassigned."));
    }
#endif /* ENABLED (JERRY_ES2015) */
    else if (is_strict)
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("Binding cannot be set."));
    }
  }
  else
  {
    JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND);

    ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

    ecma_value_t completion = ecma_op_object_put (binding_obj_p,
                                                  name_p,
                                                  value,
                                                  is_strict);

    if (ECMA_IS_VALUE_ERROR (completion))
    {
      return completion;
    }

    JERRY_ASSERT (ecma_is_value_boolean (completion));
  }

  return ECMA_VALUE_EMPTY;
} /* ecma_op_set_mutable_binding */

/**
 * GetBindingValue operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_get_binding_value (ecma_object_t *lex_env_p, /**< lexical environment */
                           ecma_string_t *name_p, /**< argument N */
                           bool is_strict) /**< argument S */
{
  JERRY_ASSERT (lex_env_p != NULL
                && ecma_is_lexical_environment (lex_env_p));
  JERRY_ASSERT (name_p != NULL);

  if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
  {
    ecma_property_value_t *prop_value_p = ecma_get_named_data_property (lex_env_p, name_p);

    return ecma_copy_value (prop_value_p->value);
  }
  else
  {
    JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND);

    ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

    ecma_value_t result = ecma_op_object_find (binding_obj_p, name_p);

    if (ECMA_IS_VALUE_ERROR (result))
    {
      return result;
    }

    if (!ecma_is_value_found (result))
    {
      if (is_strict)
      {
        result = ecma_raise_reference_error (ECMA_ERR_MSG ("Binding does not exist or is uninitialised."));
      }
      else
      {
        result = ECMA_VALUE_UNDEFINED;
      }
    }

    return result;
  }
} /* ecma_op_get_binding_value */

/**
 * DeleteBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 *
 * @return ecma value
 *         Return ECMA_VALUE_ERROR - if the operation fails
 *         ECMA_VALUE_{TRUE/FALSE} - depends on whether the binding can be deleted
 */
ecma_value_t
ecma_op_delete_binding (ecma_object_t *lex_env_p, /**< lexical environment */
                        ecma_string_t *name_p) /**< argument N */
{
  JERRY_ASSERT (lex_env_p != NULL
                && ecma_is_lexical_environment (lex_env_p));
  JERRY_ASSERT (name_p != NULL);

  if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
  {
    ecma_property_t *prop_p = ecma_find_named_property (lex_env_p, name_p);
    ecma_value_t ret_val;

    if (prop_p == NULL)
    {
      ret_val = ECMA_VALUE_TRUE;
    }
    else
    {
      JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (*prop_p) == ECMA_PROPERTY_TYPE_NAMEDDATA);

      if (!ecma_is_property_configurable (*prop_p))
      {
        ret_val = ECMA_VALUE_FALSE;
      }
      else
      {
        ecma_delete_property (lex_env_p, ECMA_PROPERTY_VALUE_PTR (prop_p));

        ret_val = ECMA_VALUE_TRUE;
      }
    }

    return ret_val;
  }
  else
  {
    JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND);

    ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

    return ecma_op_object_delete (binding_obj_p, name_p, false);
  }
} /* ecma_op_delete_binding */

/**
 * ImplicitThisValue operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_implicit_this_value (ecma_object_t *lex_env_p) /**< lexical environment */
{
  JERRY_ASSERT (lex_env_p != NULL
                && ecma_is_lexical_environment (lex_env_p));

  if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
  {
    return ECMA_VALUE_UNDEFINED;
  }
  else
  {
    JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND);

    ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);
    ecma_ref_object (binding_obj_p);

    return ecma_make_object_value (binding_obj_p);
  }
} /* ecma_op_implicit_this_value */

/**
 * CreateImmutableBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 */
void
ecma_op_create_immutable_binding (ecma_object_t *lex_env_p, /**< lexical environment */
                                  ecma_string_t *name_p, /**< argument N */
                                  ecma_value_t value) /**< argument V */
{
  JERRY_ASSERT (lex_env_p != NULL
                && ecma_is_lexical_environment (lex_env_p));
  JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE);

  /*
   * Warning:
   *         Whether immutable bindings are deletable seems not to be defined by ECMA v5.
   */
  ecma_property_value_t *prop_value_p = ecma_create_named_data_property (lex_env_p,
                                                                         name_p,
                                                                         ECMA_PROPERTY_FIXED,
                                                                         NULL);

  prop_value_p->value = ecma_copy_value_if_not_object (value);
} /* ecma_op_create_immutable_binding */

#if ENABLED (JERRY_ES2015)
/**
 * InitializeBinding operation.
 *
 * See also: ECMA-262 v6, 8.1.1.1.4
 */
void
ecma_op_initialize_binding (ecma_object_t *lex_env_p, /**< lexical environment */
                            ecma_string_t *name_p, /**< argument N */
                            ecma_value_t value) /**< argument V */
{
  JERRY_ASSERT (lex_env_p != NULL
                && ecma_is_lexical_environment (lex_env_p));
  JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE);

  ecma_property_t *prop_p = ecma_find_named_property (lex_env_p, name_p);
  JERRY_ASSERT (prop_p != NULL);
  JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (*prop_p) == ECMA_PROPERTY_TYPE_NAMEDDATA);

  ecma_property_value_t *prop_value_p = ECMA_PROPERTY_VALUE_PTR (prop_p);
  JERRY_ASSERT (prop_value_p->value == ECMA_VALUE_UNINITIALIZED);

  prop_value_p->value = ecma_copy_value_if_not_object (value);
} /* ecma_op_initialize_binding */

/**
 * BindThisValue operation for an empty lexical environment
 *
 * See also: ECMA-262 v6, 8.1.1.3.1
 */
void
ecma_op_init_this_binding (ecma_object_t *lex_env_p, /**< lexical environment */
                           ecma_value_t this_binding) /**< this binding value */
{
  JERRY_ASSERT (lex_env_p != NULL);
  JERRY_ASSERT (ecma_is_value_object (this_binding) || this_binding == ECMA_VALUE_UNINITIALIZED);
  ecma_string_t *prop_name_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_THIS_BINDING_VALUE);

  ecma_property_value_t *prop_value_p = ecma_create_named_data_property (lex_env_p,
                                                                         prop_name_p,
                                                                         ECMA_PROPERTY_FIXED,
                                                                         NULL);
  prop_value_p->value = this_binding;
} /* ecma_op_init_this_binding */

/**
 * GetThisEnvironment operation.
 *
 * See also: ECMA-262 v6, 8.3.2
 *
 * @return property pointer for the internal [[ThisBindingValue]] property
 */
ecma_property_t *
ecma_op_get_this_property (ecma_object_t *lex_env_p) /**< lexical environment */
{
  JERRY_ASSERT (lex_env_p != NULL);

  ecma_string_t *prop_name_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_THIS_BINDING_VALUE);
  while (true)
  {
    if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
    {
      ecma_property_t *prop_p = ecma_find_named_property (lex_env_p, prop_name_p);

      if (prop_p != NULL)
      {
        return prop_p;
      }
    }

    JERRY_ASSERT (lex_env_p->u2.outer_reference_cp != JMEM_CP_NULL);
    lex_env_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, lex_env_p->u2.outer_reference_cp);
  }
} /* ecma_op_get_this_property */

/**
 * GetThisBinding operation.
 *
 * See also: ECMA-262 v6, 8.1.1.3.4
 *
 * @return ECMA_VALUE_ERROR - if the operation fails
 *         ecma-object - otherwise
 */
ecma_value_t
ecma_op_get_this_binding (ecma_object_t *lex_env_p) /**< lexical environment */
{
  JERRY_ASSERT (lex_env_p != NULL);

  ecma_property_t *prop_p = ecma_op_get_this_property (lex_env_p);
  JERRY_ASSERT (prop_p != NULL);

  ecma_value_t this_value = ECMA_PROPERTY_VALUE_PTR (prop_p)->value;

  if (this_value == ECMA_VALUE_UNINITIALIZED)
  {
    return ecma_raise_reference_error (ECMA_ERR_MSG ("Must call super constructor in derived class before "
                                                     "accessing 'this' or returning from it."));
  }

  ecma_ref_object (ecma_get_object_from_value (this_value));

  return this_value;
} /* ecma_op_get_this_binding */

/**
 * BindThisValue operation.
 *
 * See also: ECMA-262 v6, 8.1.1.3.1
 */
void
ecma_op_bind_this_value (ecma_property_t *prop_p, /**< [[ThisBindingValue]] internal property */
                         ecma_value_t this_binding) /**< this binding value */
{
  JERRY_ASSERT (prop_p != NULL);
  JERRY_ASSERT (ecma_is_value_object (this_binding));
  JERRY_ASSERT (!ecma_op_this_binding_is_initialized (prop_p));

  ECMA_PROPERTY_VALUE_PTR (prop_p)->value = this_binding;
} /* ecma_op_bind_this_value */

/**
 * Get the environment record [[ThisBindingStatus]] internal property.
 *
 * See also: ECMA-262 v6, 8.1.1.3
 *
 * @return true - if the status is "initialzed"
 *         false - otherwise
 */
bool
ecma_op_this_binding_is_initialized (ecma_property_t *prop_p) /**< [[ThisBindingValue]] internal property */
{
  JERRY_ASSERT (prop_p != NULL);

  return ECMA_PROPERTY_VALUE_PTR (prop_p)->value != ECMA_VALUE_UNINITIALIZED;
} /* ecma_op_this_binding_is_initialized */

#endif /* ENABLED (JERRY_ES2015) */

/**
 * @}
 * @}
 */
