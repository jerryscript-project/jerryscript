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
 *         Returned value must be freed through ecma_FreeReference.
 */
ecma_Reference_t
ecma_OpGetIdentifierReference(ecma_Object_t *lex_env_p, /**< lexical environment */
                              ecma_Char_t *name_p, /**< identifier's name */
                              bool is_strict) /**< strict reference flag */
{
  JERRY_ASSERT( lex_env_p != NULL );

  ecma_Object_t *lex_env_iter_p = lex_env_p;

  while ( lex_env_iter_p != NULL )
  {
    ecma_CompletionValue_t completion_value;
    completion_value = ecma_OpHasBinding( lex_env_iter_p, name_p);

    if ( ecma_IsCompletionValueNormalTrue( completion_value) )
    {
      return ecma_MakeReference( ecma_MakeObjectValue( lex_env_iter_p),
                                 name_p,
                                 is_strict);
    } else
    {
      JERRY_ASSERT( ecma_IsCompletionValueNormalFalse( completion_value) );
    }

    lex_env_iter_p = ecma_GetPointer( lex_env_iter_p->u.m_LexicalEnvironment.m_pOuterReference);
  }

  return ecma_MakeReference( ecma_MakeSimpleValue( ECMA_SIMPLE_VALUE_UNDEFINED),
                             name_p,
                             is_strict);
} /* ecma_OpGetIdentifierReference */

/**
 * ECMA-reference constructor.
 *
 * Warning: string pointed by name_p
 *          must not be freed or reused
 *          until the reference is freed.
 *
 * @return ECMA-reference
 *         Returned value must be freed through ecma_FreeReference.
 */
ecma_Reference_t
ecma_MakeReference(ecma_Value_t base, /**< base value */
                   ecma_Char_t *name_p, /**< referenced name */
                   bool is_strict) /**< strict reference flag */
{
  return (ecma_Reference_t) { .base = ecma_CopyValue( base),
                              .referenced_name_p = name_p,
                              .is_strict = is_strict };
} /* ecma_MakeReference */

/**
 * Free specified ECMA-reference.
 *
 * Warning:
 *         freeing invalidates all copies of the reference.
 */
void
ecma_FreeReference( const ecma_Reference_t ref) /**< reference */
{
  ecma_FreeValue( ref.base);
} /* ecma_FreeReference */

/**
 * @}
 * @}
 */
