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

#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-reference.h"
#include "jrt.h"

/** \addtogroup ecma ECMA
 * @{
 */

/**
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
ecma_object_t*
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
 * Resolve syntactic reference to ECMA-reference.
 *
 * @return ECMA-reference
 *         Returned value must be freed through ecma_free_reference.
 */
ecma_reference_t
ecma_op_get_identifier_reference (ecma_object_t *lex_env_p, /**< lexical environment */
                                  ecma_string_t *name_p, /**< identifier's name */
                                  bool is_strict) /**< strict reference flag */
{
  JERRY_ASSERT (lex_env_p != NULL);

  ecma_object_t *base_lex_env_p = ecma_op_resolve_reference_base (lex_env_p, name_p);

  if (base_lex_env_p != NULL)
  {
    return ecma_make_reference (ecma_make_object_value (base_lex_env_p),
                                name_p,
                                is_strict);
  }
  else
  {
    return ecma_make_reference (ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED),
                                name_p,
                                is_strict);
  }
} /* ecma_op_get_identifier_reference */

/**
 * ECMA-reference constructor.
 *
 * @return ECMA-reference
 *         Returned value must be freed through ecma_free_reference.
 */
ecma_reference_t
ecma_make_reference (ecma_value_t base, /**< base value */
                     ecma_string_t *name_p, /**< referenced name */
                     bool is_strict) /**< strict reference flag */
{
  name_p = ecma_copy_or_ref_ecma_string (name_p);

  ecma_reference_t ref;
  ref.base = ecma_copy_value (base, true);
  ref.is_strict = (is_strict != 0);

  ECMA_SET_POINTER (ref.referenced_name_cp, name_p);

  return ref;
} /* ecma_make_reference */

/**
 * Free specified ECMA-reference.
 *
 * Warning:
 *         freeing invalidates all copies of the reference.
 */
void
ecma_free_reference (ecma_reference_t ref) /**< reference */
{
  ecma_free_value (ref.base, true);
  ecma_deref_ecma_string (ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                     ref.referenced_name_cp));
} /* ecma_free_reference */

/**
 * @}
 * @}
 */
