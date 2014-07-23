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

#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-reference.h"
#include "globals.h"

/** \addtogroup ecma ---TODO---
 * @{
 */

/**
 * \addtogroup references ECMA-Reference
 * @{
 */

/**
 * Resolve syntactic reference to ECMA-reference.
 *
 * Warning: string pointed by name_p
 *          must not be freed or reused
 *          until the reference is freed.
 *
 * @return ECMA-reference
 *         Returned value must be freed through ecma_free_reference.
 */
ecma_reference_t
ecma_op_get_identifier_reference(ecma_object_t *lex_env_p, /**< lexical environment */
                              ecma_char_t *name_p, /**< identifier's name */
                              bool is_strict) /**< strict reference flag */
{
  JERRY_ASSERT( lex_env_p != NULL );

  ecma_object_t *lex_env_iter_p = lex_env_p;

  while ( lex_env_iter_p != NULL )
  {
    ecma_completion_value_t completion_value;
    completion_value = ecma_op_has_binding( lex_env_iter_p, name_p);

    if ( ecma_is_completion_value_normal_true( completion_value) )
    {
      return ecma_make_reference( ecma_make_object_value( lex_env_iter_p),
                                 name_p,
                                 is_strict);
    } else
    {
      JERRY_ASSERT( ecma_is_completion_value_normal_false( completion_value) );
    }

    lex_env_iter_p = ecma_get_pointer( lex_env_iter_p->u.lexical_environment.outer_reference_p);
  }

  return ecma_make_reference( ecma_make_simple_value( ECMA_SIMPLE_VALUE_UNDEFINED),
                             name_p,
                             is_strict);
} /* ecma_op_get_identifier_reference */

/**
 * ECMA-reference constructor.
 *
 * Warning: string pointed by name_p
 *          must not be freed or reused
 *          until the reference is freed.
 *
 * @return ECMA-reference
 *         Returned value must be freed through ecma_free_reference.
 */
ecma_reference_t
ecma_make_reference(ecma_value_t base, /**< base value */
                   ecma_char_t *name_p, /**< referenced name */
                   bool is_strict) /**< strict reference flag */
{
  return (ecma_reference_t) { .base = ecma_copy_value( base),
                              .referenced_name_p = name_p,
                              .is_strict = is_strict };
} /* ecma_make_reference */

/**
 * Free specified ECMA-reference.
 *
 * Warning:
 *         freeing invalidates all copies of the reference.
 */
void
ecma_free_reference( const ecma_reference_t ref) /**< reference */
{
  ecma_free_value( ref.base);
} /* ecma_free_reference */

/**
 * @}
 * @}
 */
