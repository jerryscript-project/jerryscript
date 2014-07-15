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

/**
 * Implementation of ECMA GetValue and PutValue
 */

#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-operations.h"

/** \addtogroup ecma ---TODO---
 * @{
 * 
 * \addtogroup ecmaoperations ECMA-defined operations
 * @{
 */

/**
 * Resolve syntactic reference to ECMA-reference.
 *
 * Warning: string pointed by name_p
 *          must not be freed or reused
 *          until the reference is freed.
 *
 * @return ECMA-reference (if base value is an object, upon return
 *         it's reference counter is increased by one).
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

    JERRY_ASSERT( completion_value.type == ECMA_COMPLETION_TYPE_NORMAL );

    if ( ecma_IsValueTrue( completion_value.value) )
    {
      ecma_RefObject( lex_env_iter_p);

      return (ecma_Reference_t) { .base = ecma_MakeObjectValue( lex_env_iter_p),
                                  .referenced_name_p = name_p,
                                  .is_strict = is_strict };
    }

    lex_env_iter_p = ecma_GetPointer( lex_env_iter_p->u.m_LexicalEnvironment.m_pOuterReference);
  }

  return (ecma_Reference_t) { .base = ecma_MakeObjectValue( NULL),
                              .referenced_name_p = NULL,
                              .is_strict = is_strict };
} /* ecma_OpGetIdentifierReference */

/**
 * GetValue operation.
 *
 * See also: ECMA-262 v5, 8.7.1
 */
ecma_CompletionValue_t
ecma_OpGetValue( ecma_Reference_t *ref_p) /**< ECMA-reference */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS( ref_p);
} /* ecma_OpGetValue */

/**
 * SetValue operation.
 *
 * See also: ECMA-262 v5, 8.7.1
 */
ecma_CompletionValue_t
ecma_OpSetValue(ecma_Reference_t *ref_p, /**< ECMA-reference */
                ecma_Value_t value) /**< ECMA-value */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS( ref_p, value);
} /* ecma_OpSetValue */

/**
 * @}
 * @}
 */
