/* Copyright 2014 Samsung Electronics Co., Ltd.
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

#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-global-object.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-objects-properties.h"
#include "globals.h"

/** \addtogroup ecma ---TODO---
 * @{
 */

/**
 * \addtogroup lexicalenvironment Lexical environment
 * @{
 */

/**
 * Get binding object of object lexical environment.
 *
 * @return pointer to binding object
 */
static ecma_object_t*
ecma_get_lex_env_binding_object (ecma_object_t* obj_lex_env_p) /**< object lexical environment */
{
  JERRY_ASSERT(obj_lex_env_p != NULL
                && obj_lex_env_p->is_lexical_environment
                && obj_lex_env_p->u.lexical_environment.type == ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND);

  ecma_property_t *binding_obj_prop_p = ECMA_GET_POINTER(obj_lex_env_p->properties_p);
  JERRY_ASSERT(binding_obj_prop_p != NULL
                && binding_obj_prop_p->u.internal_property.type == ECMA_INTERNAL_PROPERTY_BINDING_OBJECT);

  return ECMA_GET_POINTER(binding_obj_prop_p->u.internal_property.value);
} /* ecma_get_lex_env_binding_object */

/**
 * HasBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 *
 * @return completion value
 *         Return value is simple and so need not be freed.
 *         However, ecma_free_completion_value may be called for it, but it is a no-op.
 */
ecma_completion_value_t
ecma_op_has_binding (ecma_object_t *lex_env_p, /**< lexical environment */
                    const ecma_char_t *name_p) /**< argument N */
{
  JERRY_ASSERT(lex_env_p != NULL && lex_env_p->is_lexical_environment);

  ecma_simple_value_t has_binding = ECMA_SIMPLE_VALUE_UNDEFINED;

  switch ((ecma_lexical_environment_type_t) lex_env_p->u.lexical_environment.type)
  {
    case ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE:
    {
      ecma_property_t *property_p = ecma_find_named_property (lex_env_p, name_p);

      has_binding = (property_p != NULL) ? ECMA_SIMPLE_VALUE_TRUE
                                           : ECMA_SIMPLE_VALUE_FALSE;

      break;
    }
    case ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND:
    {
      ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

      has_binding = ecma_op_object_has_property (binding_obj_p, name_p) ? ECMA_SIMPLE_VALUE_TRUE
                                                                        : ECMA_SIMPLE_VALUE_FALSE;

      break;
    }
  }

  return ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
                                    ecma_make_simple_value (has_binding),
                                    ECMA_TARGET_ID_RESERVED);
} /* ecma_op_has_binding */

/**
 * CreateMutableBinding operation.
 *
 * see also: ecma-262 v5, 10.2.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_create_mutable_binding (ecma_object_t *lex_env_p, /**< lexical environment */
                               const ecma_char_t *name_p, /**< argument N */
                               bool is_deletable) /**< argument D */
{
  JERRY_ASSERT(lex_env_p != NULL && lex_env_p->is_lexical_environment);
  JERRY_ASSERT(name_p != NULL);

  switch ((ecma_lexical_environment_type_t) lex_env_p->u.lexical_environment.type)
  {
    case ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE:
    {
      JERRY_ASSERT(ecma_is_completion_value_normal_false (ecma_op_has_binding (lex_env_p, name_p)));

      ecma_create_named_data_property (lex_env_p,
                                       name_p,
                                       ECMA_PROPERTY_WRITABLE,
                                       ECMA_PROPERTY_NOT_ENUMERABLE,
                                       is_deletable ? ECMA_PROPERTY_CONFIGURABLE
                                                    : ECMA_PROPERTY_NOT_CONFIGURABLE);


      break;
    }
    case ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND:
    {
      ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

      JERRY_ASSERT(!ecma_op_object_has_property (binding_obj_p, name_p));

      ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();
        {
          prop_desc.is_value_defined = true;
          prop_desc.value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

          prop_desc.is_writable_defined = true;
          prop_desc.writable = ECMA_PROPERTY_WRITABLE;

          prop_desc.is_enumerable_defined = true;
          prop_desc.enumerable = ECMA_PROPERTY_ENUMERABLE;

          prop_desc.is_configurable_defined = true;
          prop_desc.configurable = is_deletable ? ECMA_PROPERTY_CONFIGURABLE
                                                : ECMA_PROPERTY_NOT_CONFIGURABLE;
        }

      ecma_completion_value_t completion = ecma_op_object_define_own_property (binding_obj_p,
                                                                               name_p,
                                                                               prop_desc,
                                                                               true);

      if (!(ecma_is_completion_value_normal_true (completion)
              || ecma_is_completion_value_normal_false (completion)))
        {
          JERRY_ASSERT(ecma_is_completion_value_throw (completion));

          return completion;
        }
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
                            const ecma_char_t *name_p, /**< argument N */
                            ecma_value_t value, /**< argument V */
                            bool is_strict) /**< argument S */
{
  JERRY_ASSERT(lex_env_p != NULL && lex_env_p->is_lexical_environment);
  JERRY_ASSERT(name_p != NULL);

  JERRY_ASSERT(ecma_is_completion_value_normal_true (ecma_op_has_binding (lex_env_p, name_p)));

  switch ((ecma_lexical_environment_type_t) lex_env_p->u.lexical_environment.type)
  {
    case ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE:
    {
      ecma_property_t *property_p = ecma_get_named_data_property (lex_env_p, name_p);

      if (property_p->u.named_data_property.writable == ECMA_PROPERTY_WRITABLE)
        {
          ecma_free_value (property_p->u.named_data_property.value, false);
          property_p->u.named_data_property.value = ecma_copy_value (value, false);

          ecma_gc_update_may_ref_younger_object_flag_by_value (lex_env_p, value);
        }
      else if (is_strict)
        {
          return ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
        }

      break;
    }
    case ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND:
    {
      ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

      ecma_completion_value_t completion = ecma_op_object_put (binding_obj_p,
                                                               name_p,
                                                               value,
                                                               is_strict);
      if (!(ecma_is_completion_value_normal_true (completion)
              || ecma_is_completion_value_normal_false (completion)))
        {
          JERRY_ASSERT(ecma_is_completion_value_throw (completion));

          return completion;
        }

      break;
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
                          const ecma_char_t *name_p, /**< argument N */
                          bool is_strict) /**< argument S */
{
  JERRY_ASSERT(lex_env_p != NULL && lex_env_p->is_lexical_environment);
  JERRY_ASSERT(name_p != NULL);

  JERRY_ASSERT(ecma_is_completion_value_normal_true (ecma_op_has_binding (lex_env_p, name_p)));

  switch ((ecma_lexical_environment_type_t) lex_env_p->u.lexical_environment.type)
  {
    case ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE:
    {
      ecma_property_t *property_p = ecma_get_named_data_property (lex_env_p, name_p);

      ecma_value_t prop_value = property_p->u.named_data_property.value;

      /* is the binding mutable? */
      if (property_p->u.named_data_property.writable == ECMA_PROPERTY_WRITABLE)
      {
        return ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
                                           ecma_copy_value (prop_value, true),
                                           ECMA_TARGET_ID_RESERVED);
      } else if (ecma_is_value_empty (prop_value))
      {
        /* unitialized immutable binding */
        if (is_strict)
        {
          return ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_REFERENCE));
        } else
        {
          return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
        }
      }

      break;
    }
    case ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND:
    {
      ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

      bool has_prop = ecma_op_object_has_property (binding_obj_p, name_p);

      if (!has_prop)
        {
          if (is_strict)
            {
              return ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_REFERENCE));
            }
          else
            {
              return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
            }
        }

      return ecma_op_object_get (binding_obj_p, name_p);
    }
  }

  JERRY_UNREACHABLE();
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
                       const ecma_char_t *name_p) /**< argument N */
{
  JERRY_ASSERT(lex_env_p != NULL && lex_env_p->is_lexical_environment);
  JERRY_ASSERT(name_p != NULL);

  switch ((ecma_lexical_environment_type_t) lex_env_p->u.lexical_environment.type)
  {
    case ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE:
      {
        ecma_property_t *prop_p = ecma_find_named_property (lex_env_p, name_p);
        ecma_simple_value_t ret_val;

        if (prop_p == NULL)
        {
          ret_val = ECMA_SIMPLE_VALUE_TRUE;
        } else
        {
          JERRY_ASSERT(prop_p->type == ECMA_PROPERTY_NAMEDDATA);

          if (prop_p->u.named_data_property.configurable == ECMA_PROPERTY_NOT_CONFIGURABLE)
          {
            ret_val = ECMA_SIMPLE_VALUE_FALSE;
          } else
          {
            ecma_delete_property (lex_env_p, prop_p);

            ret_val = ECMA_SIMPLE_VALUE_TRUE;
          }
        }

        return ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
                                           ecma_make_simple_value (ret_val),
                                           ECMA_TARGET_ID_RESERVED);
      }
    case ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND:
    {
      ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

      return ecma_op_object_delete (binding_obj_p, name_p, false);
    }
  }

  JERRY_UNREACHABLE();
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
  JERRY_ASSERT(lex_env_p != NULL && lex_env_p->is_lexical_environment);

  switch ((ecma_lexical_environment_type_t) lex_env_p->u.lexical_environment.type)
  {
    case ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE:
      {
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
      }
    case ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND:
    {
      ecma_property_t *provide_this_prop_p = ecma_get_internal_property (lex_env_p,
                                                                         ECMA_INTERNAL_PROPERTY_PROVIDE_THIS);

      bool provide_this = provide_this_prop_p->u.internal_property.value;

      if (provide_this)
        {
          ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);
          ecma_ref_object (binding_obj_p);

          return ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
                                             ecma_make_object_value (binding_obj_p),
                                             ECMA_TARGET_ID_RESERVED);
        }
      else
        {
          return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
        }
    }
  }

  JERRY_UNREACHABLE();
} /* ecma_op_implicit_this_value */

/**
 * CreateImmutableBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 */
void
ecma_op_create_immutable_binding (ecma_object_t *lex_env_p, /**< lexical environment */
                                 const ecma_char_t *name_p) /**< argument N */
{
  JERRY_ASSERT(lex_env_p != NULL && lex_env_p->is_lexical_environment);

  switch ((ecma_lexical_environment_type_t) lex_env_p->u.lexical_environment.type)
  {
    case ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE:
      {
        JERRY_ASSERT(ecma_is_completion_value_normal_false (ecma_op_has_binding (lex_env_p, name_p)));

        /*
         * Warning:
         *         Whether immutable bindings are deletable seems not to be defined by ECMA v5.
         */
        ecma_property_t *prop_p = ecma_create_named_data_property (lex_env_p,
                                                                   name_p,
                                                                   ECMA_PROPERTY_NOT_WRITABLE,
                                                                   ECMA_PROPERTY_NOT_ENUMERABLE,
                                                                   ECMA_PROPERTY_NOT_CONFIGURABLE);

        JERRY_ASSERT(ecma_is_value_undefined (prop_p->u.named_data_property.value));

        prop_p->u.named_data_property.value.value_type = ECMA_TYPE_SIMPLE;
        prop_p->u.named_data_property.value.value = ECMA_SIMPLE_VALUE_EMPTY;
      }
    case ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND:
    {
      JERRY_UNREACHABLE();
    }
  }

  JERRY_UNREACHABLE();
} /* ecma_op_create_immutable_binding */

/**
 * InitializeImmutableBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 */
void
ecma_op_initialize_immutable_binding (ecma_object_t *lex_env_p, /**< lexical environment */
                                     const ecma_char_t *name_p, /**< argument N */
                                     ecma_value_t value) /**< argument V */
{
  JERRY_ASSERT(lex_env_p != NULL && lex_env_p->is_lexical_environment);

  switch ((ecma_lexical_environment_type_t) lex_env_p->u.lexical_environment.type)
  {
    case ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE:
      {
        JERRY_ASSERT(ecma_is_completion_value_normal_true (ecma_op_has_binding (lex_env_p, name_p)));

        ecma_property_t *prop_p = ecma_get_named_data_property (lex_env_p, name_p);

        /* The binding must be unitialized immutable binding */
        JERRY_ASSERT(prop_p->u.named_data_property.writable == ECMA_PROPERTY_NOT_WRITABLE
                      && ecma_is_value_empty (prop_p->u.named_data_property.value));

        prop_p->u.named_data_property.value = ecma_copy_value (value, false);

        ecma_gc_update_may_ref_younger_object_flag_by_value (lex_env_p, value);
      }
    case ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND:
    {
      JERRY_UNREACHABLE();
    }
  }

  JERRY_UNREACHABLE();
} /* ecma_op_initialize_immutable_binding */

/**
 * The Global Environment constructor.
 *
 * See also: ECMA-262 v5, 10.2.3
 *
 * @return pointer to created lexical environment
 */
ecma_object_t*
ecma_op_create_global_environment (void)
{
  ecma_object_t *glob_obj_p = ecma_op_create_global_object ();

  ecma_object_t *glob_env_p = ecma_create_object_lex_env (NULL, glob_obj_p, false);

  ecma_deref_object (glob_obj_p);

  return glob_env_p;
} /* ecma_op_create_global_environment */

/**
 * @}
 * @}
 */
