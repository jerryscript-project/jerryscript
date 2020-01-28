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

#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lcache.h"
#include "ecma-lex-env.h"
#include "ecma-objects.h"
#include "ecma-reference.h"
#include "jrt.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup references ECMA-Reference
 * @{
 */

/**
 * Resolve syntactic reference.
 *
 * @return ECMA_OBJECT_POINTER_ERROR - if the operation fails
 *         pointer to lexical environment - if the reference's base is resolved sucessfully,
 *         NULL - otherwise.
 */
ecma_object_t *
ecma_op_resolve_reference_base (ecma_object_t *lex_env_p, /**< starting lexical environment */
                                ecma_string_t *name_p) /**< identifier's name */
{
  JERRY_ASSERT (lex_env_p != NULL);

  while (true)
  {
#if ENABLED (JERRY_ES2015)
    if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_SUPER_OBJECT_BOUND)
    {
      JERRY_ASSERT (lex_env_p->u2.outer_reference_cp != JMEM_CP_NULL);
      lex_env_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, lex_env_p->u2.outer_reference_cp);
    }
#endif /* ENABLED (JERRY_ES2015) */

    ecma_value_t has_binding = ecma_op_has_binding (lex_env_p, name_p);

#if ENABLED (JERRY_ES2015_BUILTIN_PROXY)
    if (ECMA_IS_VALUE_ERROR (has_binding))
    {
      return ECMA_OBJECT_POINTER_ERROR;
    }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROXY) */

    if (ecma_is_value_true (has_binding))
    {
      return lex_env_p;
    }

    if (lex_env_p->u2.outer_reference_cp == JMEM_CP_NULL)
    {
      return NULL;
    }

    lex_env_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, lex_env_p->u2.outer_reference_cp);
  }
} /* ecma_op_resolve_reference_base */

#if ENABLED (JERRY_ES2015)
/**
 * Resolve super reference.
 *
 * @return value of the reference
 */
ecma_object_t *
ecma_op_resolve_super_reference_value (ecma_object_t *lex_env_p) /**< starting lexical environment */
{
  while (true)
  {
    JERRY_ASSERT (lex_env_p != NULL);

    if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_SUPER_OBJECT_BOUND)
    {
      return ecma_get_lex_env_binding_object (lex_env_p);
    }

    JERRY_ASSERT (lex_env_p->u2.outer_reference_cp != JMEM_CP_NULL);

    lex_env_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, lex_env_p->u2.outer_reference_cp);
  }
} /* ecma_op_resolve_super_reference_value */

/**
 * Helper method for HasBindig operation
 *
 * See also:
 *         ECMA-262 v6, 8.1.1.2.1 steps 7-9;
 *
 * @return ECMA_VALUE_TRUE - if the property is unscopable
 *         ECMA_VALUE_FALSE - if a the property is not unscopable
 *         ECMA_VALUE_ERROR - otherwise
 */
ecma_value_t
ecma_op_is_prop_unscopable (ecma_object_t *lex_env_p, /**< lexical environment */
                            ecma_string_t *prop_name_p) /**< property's name */
{
  if (lex_env_p == ecma_get_global_scope ())
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

  ecma_value_t unscopables = ecma_op_object_get_by_symbol_id (binding_obj_p, LIT_GLOBAL_SYMBOL_UNSCOPABLES);

  if (ECMA_IS_VALUE_ERROR (unscopables))
  {
    return unscopables;
  }

  if (ecma_is_value_object (unscopables))
  {
    ecma_object_t *unscopables_obj_p = ecma_get_object_from_value (unscopables);
    ecma_value_t get_unscopables_value = ecma_op_object_get (unscopables_obj_p, prop_name_p);
    ecma_deref_object (unscopables_obj_p);

    if (ECMA_IS_VALUE_ERROR (get_unscopables_value))
    {
      return get_unscopables_value;
    }

    bool is_blocked = ecma_op_to_boolean (get_unscopables_value);

    ecma_free_value (get_unscopables_value);

    return ecma_make_boolean_value (is_blocked);
  }

  ecma_free_value (unscopables);

  return ECMA_VALUE_FALSE;
} /* ecma_op_is_prop_unscopable */
#endif /* ENABLED (JERRY_ES2015) */

/**
 * Resolve value corresponding to reference.
 *
 * @return value of the reference
 */
ecma_value_t
ecma_op_resolve_reference_value (ecma_object_t *lex_env_p, /**< starting lexical environment */
                                 ecma_string_t *name_p) /**< identifier's name */
{
  JERRY_ASSERT (lex_env_p != NULL);

  while (true)
  {
    ecma_lexical_environment_type_t lex_env_type = ecma_get_lex_env_type (lex_env_p);

    if (lex_env_type == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
    {
      ecma_property_t *property_p = ecma_find_named_property (lex_env_p, name_p);

      if (property_p != NULL)
      {
        ecma_property_value_t *property_value_p = ECMA_PROPERTY_VALUE_PTR (property_p);

#if ENABLED (JERRY_ES2015)
        if (JERRY_UNLIKELY (property_value_p->value == ECMA_VALUE_UNINITIALIZED))
        {
          return ecma_raise_reference_error (ECMA_ERR_MSG ("Variables declared by let/const must be"
                                                           " initialized before reading their value."));
        }
#endif /* ENABLED (JERRY_ES2015) */

        return ecma_fast_copy_value (property_value_p->value);
      }
    }
    else if (lex_env_type == ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND)
    {
      ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

#if ENABLED (JERRY_ES2015)
      bool lcache_lookup_allowed = (lex_env_p == ecma_get_global_environment ());
#else /* !ENABLED (JERRY_ES2015)*/
      bool lcache_lookup_allowed = true;
#endif /* ENABLED (JERRY_ES2015) */

      if (lcache_lookup_allowed)
      {
#if ENABLED (JERRY_LCACHE)
        ecma_property_t *property_p = ecma_lcache_lookup (binding_obj_p, name_p);

        if (property_p != NULL)
        {
          ecma_property_value_t *prop_value_p = ECMA_PROPERTY_VALUE_PTR (property_p);

          if (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA)
          {
            return ecma_fast_copy_value (prop_value_p->value);
          }

          JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR);

          ecma_getter_setter_pointers_t *get_set_pair_p = ecma_get_named_accessor_property (prop_value_p);

          if (get_set_pair_p->getter_cp == JMEM_CP_NULL)
          {
            return ECMA_VALUE_UNDEFINED;
          }

          ecma_object_t *getter_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, get_set_pair_p->getter_cp);

          ecma_value_t base_value = ecma_make_object_value (binding_obj_p);
          return ecma_op_function_call (getter_p, base_value, NULL, 0);
        }
#endif /* ENABLED (JERRY_LCACHE) */
      }

      ecma_value_t prop_value = ecma_op_object_find (binding_obj_p, name_p);

      if (ECMA_IS_VALUE_ERROR (prop_value))
      {
        return prop_value;
      }

      if (ecma_is_value_found (prop_value))
      {
#if ENABLED (JERRY_ES2015)
        ecma_value_t blocked = ecma_op_is_prop_unscopable (lex_env_p, name_p);

        if (ECMA_IS_VALUE_ERROR (blocked))
        {
          ecma_free_value (prop_value);
          return blocked;
        }

        if (ecma_is_value_true (blocked))
        {
          ecma_free_value (prop_value);
        }
        else
        {
          return prop_value;
        }
#else /* !ENABLED (JERRY_ES2015) */
        return prop_value;
#endif /* ENABLED (JERRY_ES2015) */
      }
    }
    else
    {
#if ENABLED (JERRY_ES2015)
      JERRY_ASSERT (lex_env_type == ECMA_LEXICAL_ENVIRONMENT_SUPER_OBJECT_BOUND);
#else /* !ENABLED (JERRY_ES2015) */
      JERRY_UNREACHABLE ();
#endif /* ENABLED (JERRY_ES2015) */
    }

    if (lex_env_p->u2.outer_reference_cp == JMEM_CP_NULL)
    {
      break;
    }

    lex_env_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, lex_env_p->u2.outer_reference_cp);
  }

#if ENABLED (JERRY_ERROR_MESSAGES)
  ecma_value_t name_val = ecma_make_string_value (name_p);
  ecma_value_t error_value = ecma_raise_standard_error_with_format (ECMA_ERROR_REFERENCE,
                                                                    "% is not defined",
                                                                    name_val);
#else /* ENABLED (JERRY_ERROR_MESSAGES) */
  ecma_value_t error_value = ecma_raise_reference_error (NULL);
#endif /* !ENABLED (JERRY_ERROR_MESSAGES) */
  return error_value;
} /* ecma_op_resolve_reference_value */

/**
 * @}
 * @}
 */
