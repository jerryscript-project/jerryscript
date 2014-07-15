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

#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "globals.h"

/** \addtogroup ecma ---TODO---
 * @{
 */

/**
 * \addtogroup lexicalenvironment Lexical environment
 * @{
 */

/**
 * HasBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 */
ecma_CompletionValue_t
ecma_OpHasBinding(ecma_Object_t *lex_env_p, /**< lexical environment */
                  ecma_Char_t *name_p) /**< argument N */
{
  JERRY_ASSERT( lex_env_p != NULL && lex_env_p->m_IsLexicalEnvironment );

  ecma_SimpleValue_t has_binding = ECMA_SIMPLE_VALUE_UNDEFINED;

  switch ( (ecma_LexicalEnvironmentType_t) lex_env_p->u.m_LexicalEnvironment.m_Type )
  {
    case ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE:
    {
      ecma_Property_t *property_p = ecma_FindNamedProperty( lex_env_p, name_p);    

      has_binding = ( property_p != NULL ) ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE;
    }
    case ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND:
    {
      JERRY_UNIMPLEMENTED();
    }
  }  

  return ecma_MakeCompletionValue(ECMA_COMPLETION_TYPE_NORMAL,
                                  ecma_MakeSimpleValue( has_binding),
		                  ECMA_TARGET_ID_RESERVED);
} /* ecma_OpHasBinding */

/**
 * CreateMutableBinding operation.
 *
 * see also: ecma-262 v5, 10.2.1
 */
ecma_CompletionValue_t
ecma_OpCreateMutableBinding(ecma_Object_t *lex_env_p, /**< lexical environment */
                            ecma_Char_t *name_p, /**< argument N */
                            bool is_deletable) /**< argument D */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS( lex_env_p, name_p, is_deletable);
} /* ecma_OpCreateMutableBinding */

/**
 * SetMutableBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 */
ecma_CompletionValue_t
ecma_OpSetMutableBinding(ecma_Object_t *lex_env_p, /**< lexical environment */
                         ecma_Char_t *name_p, /**< argument N */
                         ecma_Value_t value, /**< argument V */
                         bool is_strict) /**< argument S */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS( lex_env_p, name_p, value, is_strict);
} /* ecma_OpSetMutableBinding */

/**
 * GetBindingValue operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 */
ecma_CompletionValue_t
ecma_OpGetBindingValue(ecma_Object_t *lex_env_p, /**< lexical environment */
                       ecma_Char_t *name_p, /**< argument N */
                       bool is_strict) /**< argument S */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS( lex_env_p, name_p, is_strict);
} /* ecma_OpGetBindingValue */

/**
 * DeleteBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 */
ecma_CompletionValue_t
ecma_OpDeleteBinding(ecma_Object_t *lex_env_p, /**< lexical environment */
                     ecma_Char_t *name_p) /**< argument N */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS( lex_env_p, name_p);
} /* ecma_OpDeleteBinding */

/**
 * ImplicitThisValue operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 */
ecma_CompletionValue_t
ecma_OpImplicitThisValue( ecma_Object_t *lex_env_p) /**< lexical environment */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS( lex_env_p);
} /* ecma_OpImplicitThisValue */

/**
 * CreateImmutableBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 */
ecma_CompletionValue_t
ecma_OpCreateImmutableBinding(ecma_Object_t *lex_env_p, /**< lexical environment */
                              ecma_Char_t *name_p) /**< argument N */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS( lex_env_p, name_p);
} /* ecma_OpCreateImmutableBinding */

/**
 * InitializeImmutableBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 */
ecma_CompletionValue_t
ecma_OpInitializeImmutableBinding(ecma_Object_t *lex_env_p, /**< lexical environment */
                                  ecma_Char_t *name_p, /**< argument N */
                                  ecma_Value_t value) /**< argument V */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS( lex_env_p, name_p, value);
} /* ecma_OpInitializeImmutableBinding */

/**
 * @}
 * @}
 */
