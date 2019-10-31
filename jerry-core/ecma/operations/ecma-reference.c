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
 * @return if reference was resolved successfully,
 *          pointer to lexical environment - reference's base,
 *         else - NULL.
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

    if (ecma_op_has_binding (lex_env_p, name_p))
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

      ecma_value_t prop_value = ecma_op_object_find (binding_obj_p, name_p);

      if (ecma_is_value_found (prop_value))
      {
        return prop_value;
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
