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

#include "ecma-exceptions.h"
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

  return (ecma_Reference_t) { .base = ecma_MakeSimpleValue( ECMA_SIMPLE_VALUE_UNDEFINED),
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
  const ecma_Value_t base = ref_p->base;
  const bool is_unresolvable_reference = ecma_IsUndefinedValue( base);
  const bool has_primitive_base = ( ecma_IsBooleanValue( base)
                                    || base.m_ValueType == ECMA_TYPE_NUMBER
                                    || base.m_ValueType == ECMA_TYPE_STRING );
  const bool is_property_reference = has_primitive_base || ( base.m_ValueType == ECMA_TYPE_OBJECT );
                                     
  if ( is_unresolvable_reference )
  {
    return ecma_MakeThrowValue( ecma_NewStandardError( ECMA_ERROR_REFERENCE));
  }

  if ( is_property_reference )
  {
    if ( !has_primitive_base )
    {
      ecma_Object_t *obj_p = ecma_GetPointer( base.m_Value);
      JERRY_ASSERT( obj_p != NULL && !obj_p->m_IsLexicalEnvironment );
      
      /* return [[Get]]( obj_p as this, ref_p->referenced_name_p) */
      JERRY_UNIMPLEMENTED();
    } else
    { 
      /*
       ecma_Object_t *obj_p = ecma_ToObject( base);
       JERRY_ASSERT( obj_p != NULL && !obj_p->m_IsLexicalEnvironment );
       ecma_Property_t *property = obj_p->[[GetProperty]]( ref_p->referenced_name_p);
       if ( property->m_Type == ECMA_PROPERTY_NAMEDDATA )
       {
         return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                          property->u.m_NamedDataProperty.m_Value,
                                          ECMA_TARGET_ID_RESERVED);
       } else
       {
         JERRY_ASSERT( property->m_Type == ECMA_PROPERTY_NAMEDACCESSOR );

         ecma_Object_t *getter = ecma_GetPointer( property->u.m_NamedAccessorProperty.m_pGet);

         if ( getter == NULL )
         {
           return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                            ecma_MakeSimpleValue( ECMA_SIMPLE_VALUE_UNDEFINED),
                                            ECMA_TARGET_ID_RESERVED);
         } else
         {
           [[Call]]( getter, base as this);
         }
       }
      */
      JERRY_UNIMPLEMENTED();
    }
  } else
  {
    ecma_Object_t *lex_env_p = ecma_GetPointer( base.m_Value);

    JERRY_ASSERT( lex_env_p != NULL && lex_env_p->m_IsLexicalEnvironment );

    return ecma_OpGetBindingValue( lex_env_p, ref_p->referenced_name_p, ref_p->is_strict);
  }
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
