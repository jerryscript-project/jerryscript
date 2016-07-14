/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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

  ecma_object_t *lex_env_iter_p = lex_env_p;

  while (lex_env_iter_p != NULL)
  {
    if (ecma_op_has_binding (lex_env_iter_p, name_p))
    {
      return lex_env_iter_p;
    }

    lex_env_iter_p = ecma_get_lex_env_outer_reference (lex_env_iter_p);
  }

  return NULL;
} /* ecma_op_resolve_reference_base */

/**
 * Resolve value corresponding to reference.
 *
 * @return value of the reference
 */
ecma_value_t
ecma_op_resolve_reference_value (ecma_object_t *lex_env_p, /**< starting lexical environment */
                                 ecma_string_t *name_p, /**< identifier's name */
                                 bool is_strict) /**< strict mode */
{
  JERRY_ASSERT (lex_env_p != NULL);

  while (lex_env_p != NULL)
  {
    if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
    {
      ecma_property_t *property_p = ecma_find_named_property (lex_env_p, name_p);

      if (property_p != NULL)
      {
        ecma_value_t prop_value = ecma_get_named_data_property_value (property_p);

        /* is the binding mutable? */
        if (unlikely (!ecma_is_property_writable (property_p)
                      && ecma_is_value_empty (prop_value)))
        {
          /* unitialized mutable binding */
          if (is_strict)
          {
            return ecma_raise_reference_error (ECMA_ERR_MSG (""));
          }
          else
          {
            return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
          }
        }
        return ecma_fast_copy_value (prop_value);
      }
    }
    else
    {
      JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_OBJECT_BOUND
                    || ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND);

      ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

      ecma_property_t *property_p = ecma_op_object_get_property (binding_obj_p, name_p);

      if (likely (property_p != NULL))
      {
        if (ECMA_PROPERTY_GET_TYPE (property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA)
        {
          return ecma_fast_copy_value (ecma_get_named_data_property_value (property_p));
        }

        ecma_object_t *getter_p = ecma_get_named_accessor_property_getter (property_p);

        if (getter_p == NULL)
        {
          return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
        }

        return ecma_op_function_call (getter_p,
                                      ecma_make_object_value (binding_obj_p),
                                      NULL,
                                      0);
      }
    }

    lex_env_p = ecma_get_lex_env_outer_reference (lex_env_p);
  }

  return ecma_raise_reference_error (ECMA_ERR_MSG (""));
} /* ecma_op_resolve_reference_value */

/**
 * @}
 * @}
 */
