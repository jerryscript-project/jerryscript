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

#include "ecma-exceptions.h"
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
 *
 * @return completion value
 *         Return value is simple and so need not be freed.
 *         However, ecma_free_completion_value may be called for it, but it is a no-op.
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

      has_binding = ( property_p != NULL ) ? ECMA_SIMPLE_VALUE_TRUE
                                           : ECMA_SIMPLE_VALUE_FALSE;

      break;
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
 *
 * @return completion value
 *         Return value is simple and so need not be freed.
 *         However, ecma_free_completion_value may be called for it, but it is a no-op.
 */
ecma_CompletionValue_t
ecma_OpCreateMutableBinding(ecma_Object_t *lex_env_p, /**< lexical environment */
                            ecma_Char_t *name_p, /**< argument N */
                            bool is_deletable) /**< argument D */
{
  JERRY_ASSERT( lex_env_p != NULL && lex_env_p->m_IsLexicalEnvironment );
  JERRY_ASSERT( name_p != NULL );

  switch ( (ecma_LexicalEnvironmentType_t) lex_env_p->u.m_LexicalEnvironment.m_Type )
  {
    case ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE:
    {
      JERRY_ASSERT( ecma_IsCompletionValueNormalFalse( ecma_OpHasBinding( lex_env_p, name_p)) );

      ecma_CreateNamedProperty( lex_env_p,
                                name_p,
                                ECMA_PROPERTY_WRITABLE,
                                ECMA_PROPERTY_NOT_ENUMERABLE,
                                is_deletable ? ECMA_PROPERTY_CONFIGURABLE
                                             : ECMA_PROPERTY_NOT_CONFIGURABLE); 


      break;
    }
    case ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND:
    {
      JERRY_UNIMPLEMENTED();
    }
  }  

  return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                   ecma_MakeSimpleValue( ECMA_SIMPLE_VALUE_EMPTY),
                                   ECMA_TARGET_ID_RESERVED);
} /* ecma_OpCreateMutableBinding */

/**
 * SetMutableBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_CompletionValue_t
ecma_OpSetMutableBinding(ecma_Object_t *lex_env_p, /**< lexical environment */
                         ecma_Char_t *name_p, /**< argument N */
                         ecma_Value_t value, /**< argument V */
                         bool is_strict) /**< argument S */
{
  JERRY_ASSERT( lex_env_p != NULL && lex_env_p->m_IsLexicalEnvironment );
  JERRY_ASSERT( name_p != NULL );

  JERRY_ASSERT( ecma_IsCompletionValueNormalTrue( ecma_OpHasBinding( lex_env_p, name_p)) );

  switch ( (ecma_LexicalEnvironmentType_t) lex_env_p->u.m_LexicalEnvironment.m_Type )
  {
    case ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE:
    {
      ecma_Property_t *property_p = ecma_GetNamedDataProperty( lex_env_p, name_p);    

      if ( property_p->u.m_NamedDataProperty.m_Writable == ECMA_PROPERTY_WRITABLE )
      {
        ecma_FreeValue( property_p->u.m_NamedDataProperty.m_Value);
        property_p->u.m_NamedDataProperty.m_Value = ecma_CopyValue( value);
      } else if ( is_strict )
      {
        return ecma_MakeThrowValue( ecma_NewStandardError( ECMA_ERROR_TYPE));
      }

      break;
    }
    case ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND:
    {
      JERRY_UNIMPLEMENTED();
    }
  }  

  return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                   ecma_MakeSimpleValue( ECMA_SIMPLE_VALUE_EMPTY),
                                   ECMA_TARGET_ID_RESERVED);
} /* ecma_OpSetMutableBinding */

/**
 * GetBindingValue operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_CompletionValue_t
ecma_OpGetBindingValue(ecma_Object_t *lex_env_p, /**< lexical environment */
                       ecma_Char_t *name_p, /**< argument N */
                       bool is_strict) /**< argument S */
{
  JERRY_ASSERT( lex_env_p != NULL && lex_env_p->m_IsLexicalEnvironment );
  JERRY_ASSERT( name_p != NULL );

  JERRY_ASSERT( ecma_IsCompletionValueNormalTrue( ecma_OpHasBinding( lex_env_p, name_p)) );

  switch ( (ecma_LexicalEnvironmentType_t) lex_env_p->u.m_LexicalEnvironment.m_Type )
  {
    case ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE:
    {
      ecma_Property_t *property_p = ecma_GetNamedDataProperty( lex_env_p, name_p);    

      ecma_Value_t prop_value = property_p->u.m_NamedDataProperty.m_Value;

      /* is the binding mutable? */
      if ( property_p->u.m_NamedDataProperty.m_Writable == ECMA_PROPERTY_WRITABLE )
      {
        return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                         ecma_CopyValue( prop_value),
                                         ECMA_TARGET_ID_RESERVED);
      } else if ( prop_value.m_ValueType == ECMA_TYPE_SIMPLE
                  && prop_value.m_Value == ECMA_SIMPLE_VALUE_EMPTY )
      {
        /* unitialized immutable binding */
        if ( is_strict )
        {
          return ecma_MakeThrowValue( ecma_NewStandardError( ECMA_ERROR_REFERENCE));
        } else
        {
          return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                           ecma_MakeSimpleValue( ECMA_SIMPLE_VALUE_UNDEFINED),
                                           ECMA_TARGET_ID_RESERVED);
        }
      }

      break;
    }
    case ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND:
    {
      JERRY_UNIMPLEMENTED();
    }
  }  

  JERRY_UNREACHABLE();
} /* ecma_OpGetBindingValue */

/**
 * DeleteBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 *
 * @return completion value
 *         Return value is simple and so need not be freed.
 *         However, ecma_free_completion_value may be called for it, but it is a no-op.
 */
ecma_CompletionValue_t
ecma_OpDeleteBinding(ecma_Object_t *lex_env_p, /**< lexical environment */
                     ecma_Char_t *name_p) /**< argument N */
{
  JERRY_ASSERT( lex_env_p != NULL && lex_env_p->m_IsLexicalEnvironment );
  JERRY_ASSERT( name_p != NULL );

  switch ( (ecma_LexicalEnvironmentType_t) lex_env_p->u.m_LexicalEnvironment.m_Type )
  {
    case ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE:
      {
        ecma_Property_t *prop_p = ecma_FindNamedProperty( lex_env_p, name_p);
        ecma_SimpleValue_t ret_val;

        if ( prop_p == NULL )
        {
          ret_val = ECMA_SIMPLE_VALUE_TRUE;
        } else
        {
          JERRY_ASSERT( prop_p->m_Type == ECMA_PROPERTY_NAMEDDATA );

          if ( prop_p->u.m_NamedDataProperty.m_Configurable == ECMA_PROPERTY_NOT_CONFIGURABLE )
          {
            ret_val = ECMA_SIMPLE_VALUE_FALSE;
          } else
          {
            ecma_DeleteProperty( lex_env_p, prop_p);

            ret_val = ECMA_SIMPLE_VALUE_TRUE;
          }
        }

        return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                         ecma_MakeSimpleValue( ret_val),
                                         ECMA_TARGET_ID_RESERVED);
      }
    case ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND:
    {
      JERRY_UNIMPLEMENTED();
    }
  }  

  JERRY_UNREACHABLE();
} /* ecma_OpDeleteBinding */

/**
 * ImplicitThisValue operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_CompletionValue_t
ecma_OpImplicitThisValue( ecma_Object_t *lex_env_p) /**< lexical environment */
{
  JERRY_ASSERT( lex_env_p != NULL && lex_env_p->m_IsLexicalEnvironment );

  switch ( (ecma_LexicalEnvironmentType_t) lex_env_p->u.m_LexicalEnvironment.m_Type )
  {
    case ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE:
      {
        return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                         ecma_MakeSimpleValue( ECMA_SIMPLE_VALUE_UNDEFINED),
                                         ECMA_TARGET_ID_RESERVED);
      }
    case ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND:
    {
      JERRY_UNIMPLEMENTED();
    }
  }  

  JERRY_UNREACHABLE();
} /* ecma_OpImplicitThisValue */

/**
 * CreateImmutableBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 */
void
ecma_OpCreateImmutableBinding(ecma_Object_t *lex_env_p, /**< lexical environment */
                              ecma_Char_t *name_p) /**< argument N */
{
  JERRY_ASSERT( lex_env_p != NULL && lex_env_p->m_IsLexicalEnvironment );

  switch ( (ecma_LexicalEnvironmentType_t) lex_env_p->u.m_LexicalEnvironment.m_Type )
  {
    case ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE:
      {
        JERRY_ASSERT( ecma_IsCompletionValueNormalFalse( ecma_OpHasBinding( lex_env_p, name_p)) );

        /*
         * Warning:
         *         Whether immutable bindings are deletable seems not to be defined by ECMA v5.
         */
        ecma_Property_t *prop_p = ecma_CreateNamedProperty( lex_env_p,
                                                            name_p,
                                                            ECMA_PROPERTY_NOT_WRITABLE,
                                                            ECMA_PROPERTY_NOT_ENUMERABLE,
                                                            ECMA_PROPERTY_NOT_CONFIGURABLE);

        JERRY_ASSERT( prop_p->u.m_NamedDataProperty.m_Value.m_ValueType == ECMA_TYPE_SIMPLE );

        prop_p->u.m_NamedDataProperty.m_Value.m_Value = ECMA_SIMPLE_VALUE_EMPTY;
      }
    case ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND:
    {
      JERRY_UNREACHABLE();
    }
  }  

  JERRY_UNREACHABLE();
} /* ecma_OpCreateImmutableBinding */

/**
 * InitializeImmutableBinding operation.
 *
 * See also: ECMA-262 v5, 10.2.1
 */
void
ecma_OpInitializeImmutableBinding(ecma_Object_t *lex_env_p, /**< lexical environment */
                                  ecma_Char_t *name_p, /**< argument N */
                                  ecma_Value_t value) /**< argument V */
{
  JERRY_ASSERT( lex_env_p != NULL && lex_env_p->m_IsLexicalEnvironment );

  switch ( (ecma_LexicalEnvironmentType_t) lex_env_p->u.m_LexicalEnvironment.m_Type )
  {
    case ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE:
      {
        JERRY_ASSERT( ecma_IsCompletionValueNormalTrue( ecma_OpHasBinding( lex_env_p, name_p)) );

        ecma_Property_t *prop_p = ecma_GetNamedDataProperty( lex_env_p, name_p);

        /* The binding must be unitialized immutable binding */
        JERRY_ASSERT( prop_p->u.m_NamedDataProperty.m_Writable == ECMA_PROPERTY_NOT_WRITABLE
                      && prop_p->u.m_NamedDataProperty.m_Value.m_ValueType == ECMA_TYPE_SIMPLE
                      && prop_p->u.m_NamedDataProperty.m_Value.m_Value == ECMA_SIMPLE_VALUE_EMPTY );

        prop_p->u.m_NamedDataProperty.m_Value = ecma_CopyValue( value);
      }
    case ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND:
    {
      JERRY_UNREACHABLE();
    }
  }  

  JERRY_UNREACHABLE();
} /* ecma_OpInitializeImmutableBinding */

/**
 * @}
 * @}
 */
