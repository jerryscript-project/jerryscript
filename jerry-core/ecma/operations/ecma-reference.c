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
                                 ecma_string_t *name_p) /**< identifier's name */
{
  JERRY_ASSERT (lex_env_p != NULL);

  while (lex_env_p != NULL)
  {
    if (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE)
    {
      ecma_property_t *property_p = ecma_find_named_property (lex_env_p, name_p);

      if (property_p != NULL)
      {
        return ecma_fast_copy_value (ECMA_PROPERTY_VALUE_PTR (property_p)->value);
      }
    }
    else
    {
      JERRY_ASSERT (ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_OBJECT_BOUND
                    || ecma_get_lex_env_type (lex_env_p) == ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND);

      ecma_object_t *binding_obj_p = ecma_get_lex_env_binding_object (lex_env_p);

      ecma_value_t prop_value = ecma_op_object_find (binding_obj_p, name_p);

      if (ecma_is_value_found (prop_value))
      {
        return prop_value;
      }
    }

    lex_env_p = ecma_get_lex_env_outer_reference (lex_env_p);
  }

  return ecma_raise_reference_error (ECMA_ERR_MSG ("Cannot resolve reference."));
} /* ecma_op_resolve_reference_value */

/**
 * @}
 * @}
 */
