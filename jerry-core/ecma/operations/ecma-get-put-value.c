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

/**
 * Implementation of ECMA GetValue and PutValue
 */

#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-objects.h"
#include "ecma-function-object.h"
#include "ecma-objects-general.h"
#include "ecma-try-catch-macro.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup lexicalenvironment Lexical environment
 * @{
 */

/**
 * GetValue operation part
 *
 * See also: ECMA-262 v5, 8.7.1, sections 3 and 5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_get_value_lex_env_base (ecma_object_t *lex_env_p, /**< lexical environment */
                                ecma_object_t **ref_base_lex_env_p, /**< [out] reference's base (lexical environment) */
                                ecma_string_t *name_p) /**< variable name */
{
  JERRY_ASSERT (lex_env_p != NULL
                && ecma_is_lexical_environment (lex_env_p));

  while (true)
  {
    switch (ecma_get_lex_env_type (lex_env_p))
    {
      case ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE:
      {
        ecma_property_t *property_p = ecma_find_named_property (lex_env_p, name_p);

        if (property_p != NULL)
        {
          *ref_base_lex_env_p = lex_env_p;
          return ecma_copy_value (ECMA_PROPERTY_VALUE_PTR (property_p)->value);
        }
        break;
      }
#if ENABLED (JERRY_ES2015)
      case ECMA_LEXICAL_ENVIRONMENT_SUPER_OBJECT_BOUND:
      {
        break;
      }
#endif /* ENABLED (JERRY_ES2015) */
      default:
      {
        JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND);

        ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

        ecma_value_t result = ecma_op_object_find (binding_obj_p, name_p);

        if (ecma_is_value_found (result))
        {
          *ref_base_lex_env_p = lex_env_p;
          return result;
        }

        break;
      }
    }

    if (lex_env_p->u2.outer_reference_cp == JMEM_CP_NULL)
    {
      break;
    }

    lex_env_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, lex_env_p->u2.outer_reference_cp);
  }

  *ref_base_lex_env_p = NULL;
#if ENABLED (JERRY_ERROR_MESSAGES)
  return ecma_raise_standard_error_with_format (ECMA_ERROR_REFERENCE,
                                                "% is not defined",
                                                ecma_make_string_value (name_p));
#else /* ENABLED (JERRY_ERROR_MESSAGES) */
  return ecma_raise_reference_error (NULL);
#endif /* ENABLED (JERRY_ERROR_MESSAGES) */

} /* ecma_op_get_value_lex_env_base */

/**
 * GetValue operation part (object base).
 *
 * See also: ECMA-262 v5, 8.7.1, section 4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_get_value_object_base (ecma_value_t base_value, /**< base value */
                               ecma_string_t *property_name_p) /**< property name */
{
  ecma_object_t *obj_p;

  if (JERRY_UNLIKELY (ecma_is_value_object (base_value)))
  {
    obj_p = ecma_get_object_from_value (base_value);
  }
  else
  {
    ecma_builtin_id_t id = ECMA_BUILTIN_ID_OBJECT_PROTOTYPE;

    if (JERRY_LIKELY (ecma_is_value_string (base_value)))
    {
      ecma_string_t *string_p = ecma_get_string_from_value (base_value);

      if (ecma_string_is_length (property_name_p))
      {
        return ecma_make_uint32_value (ecma_string_get_length (string_p));
      }

      uint32_t index = ecma_string_get_array_index (property_name_p);

      if (index != ECMA_STRING_NOT_ARRAY_INDEX
          && index < ecma_string_get_length (string_p))
      {
        ecma_char_t char_at_idx = ecma_string_get_char_at_pos (string_p, index);
        return ecma_make_string_value (ecma_new_ecma_string_from_code_unit (char_at_idx));
      }

#if ENABLED (JERRY_BUILTIN_STRING)
      id = ECMA_BUILTIN_ID_STRING_PROTOTYPE;
#endif /* ENABLED (JERRY_BUILTIN_STRING) */
    }
    else if (ecma_is_value_number (base_value))
    {
#if ENABLED (JERRY_BUILTIN_NUMBER)
      id = ECMA_BUILTIN_ID_NUMBER_PROTOTYPE;
#endif /* ENABLED (JERRY_BUILTIN_NUMBER) */
    }
#if ENABLED (JERRY_ES2015)
    else if (ecma_is_value_symbol (base_value))
    {
      id = ECMA_BUILTIN_ID_SYMBOL_PROTOTYPE;
    }
#endif /* ENABLED (JERRY_ES2015) */
    else
    {
      JERRY_ASSERT (ecma_is_value_boolean (base_value));
#if ENABLED (JERRY_BUILTIN_BOOLEAN)
      id = ECMA_BUILTIN_ID_BOOLEAN_PROTOTYPE;
#endif /* ENABLED (JERRY_BUILTIN_BOOLEAN) */
    }

    obj_p = ecma_builtin_get (id);
  }

  return ecma_op_object_get_with_receiver (obj_p, property_name_p, base_value);
} /* ecma_op_get_value_object_base */

/**
 * PutValue operation part
 *
 * See also: ECMA-262 v5, 8.7.2, sections 3 and 5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_put_value_lex_env_base (ecma_object_t *lex_env_p, /**< lexical environment */
                                ecma_string_t *name_p, /**< variable name */
                                bool is_strict, /**< flag indicating strict mode */
                                ecma_value_t value) /**< ECMA-value */
{
  JERRY_ASSERT (lex_env_p != NULL
                && ecma_is_lexical_environment (lex_env_p));

  while (true)
  {
    switch (ecma_get_lex_env_type (lex_env_p))
    {
      case ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE:
      {
        ecma_property_t *property_p = ecma_find_named_property (lex_env_p, name_p);

        if (property_p != NULL)
        {
          if (ecma_is_property_writable (*property_p))
          {
            ecma_property_value_t *property_value_p = ECMA_PROPERTY_VALUE_PTR (property_p);

#if ENABLED (JERRY_ES2015)
            if (JERRY_UNLIKELY (property_value_p->value == ECMA_VALUE_UNINITIALIZED))
            {
              return ecma_raise_reference_error (ECMA_ERR_MSG ("Variables declared by let/const must be"
                                                               " initialized before writing their value."));
            }
#endif /* ENABLED (JERRY_ES2015) */

            ecma_named_data_property_assign_value (lex_env_p, property_value_p, value);
          }
          else if (is_strict)
          {
            return ecma_raise_type_error (ECMA_ERR_MSG ("Binding cannot be set."));
          }
          return ECMA_VALUE_EMPTY;
        }
        break;
      }
#if ENABLED (JERRY_ES2015)
      case ECMA_LEXICAL_ENVIRONMENT_SUPER_OBJECT_BOUND:
      {
        break;
      }
#endif /* ENABLED (JERRY_ES2015) */
      default:
      {
        JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND);

        ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

        if (ecma_op_object_has_property (binding_obj_p, name_p))
        {
          ecma_value_t completion = ecma_op_object_put (binding_obj_p,
                                                        name_p,
                                                        value,
                                                        is_strict);

          if (ECMA_IS_VALUE_ERROR (completion))
          {
            return completion;
          }

          JERRY_ASSERT (ecma_is_value_boolean (completion));
          return ECMA_VALUE_EMPTY;
        }

        break;
      }
    }

    if (lex_env_p->u2.outer_reference_cp == JMEM_CP_NULL)
    {
      break;
    }

    lex_env_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, lex_env_p->u2.outer_reference_cp);
  }

  if (is_strict)
  {
#if ENABLED (JERRY_ERROR_MESSAGES)
    return ecma_raise_standard_error_with_format (ECMA_ERROR_REFERENCE,
                                                  "% is not defined",
                                                  ecma_make_string_value (name_p));
#else /* !ENABLED (JERRY_ERROR_MESSAGES) */
    return ecma_raise_reference_error (NULL);
#endif /* ENABLED (JERRY_ERROR_MESSAGES) */
  }

  ecma_value_t completion = ecma_op_object_put (ecma_builtin_get_global (),
                                                name_p,
                                                value,
                                                false);

  JERRY_ASSERT (ecma_is_value_boolean (completion));

  return ECMA_VALUE_EMPTY;
} /* ecma_op_put_value_lex_env_base */

/**
 * @}
 * @}
 */
