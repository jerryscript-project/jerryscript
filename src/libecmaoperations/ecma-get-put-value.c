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
 * GetValue operation.
 *
 * See also: ECMA-262 v5, 8.7.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_CompletionValue_t
ecma_op_get_value( ecma_Reference_t *ref_p) /**< ECMA-reference */
{
  const ecma_Value_t base = ref_p->base;
  const bool is_unresolvable_reference = ecma_IsValueUndefined( base);
  const bool has_primitive_base = ( ecma_IsValueBoolean( base)
                                    || base.m_ValueType == ECMA_TYPE_NUMBER
                                    || base.m_ValueType == ECMA_TYPE_STRING );
  const bool is_property_reference = has_primitive_base || ( base.m_ValueType == ECMA_TYPE_OBJECT );
                                     
  // GetValue_3
  if ( is_unresolvable_reference )
  {
    return ecma_MakeThrowValue( ecma_NewStandardError( ECMA_ERROR_REFERENCE));
  }

  // GetValue_4
  if ( is_property_reference )
  {
    if ( !has_primitive_base ) // GetValue_4.a
    {
      ecma_Object_t *obj_p = ecma_GetPointer( base.m_Value);
      JERRY_ASSERT( obj_p != NULL && !obj_p->m_IsLexicalEnvironment );
      
      // GetValue_4.b case 1
      /* return [[Get]]( base as this, ref_p->referenced_name_p) */
      JERRY_UNIMPLEMENTED();
    } else
    { // GetValue_4.b case 2
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
           return [[Call]]( getter, base as this);
         }
       }
      */
      JERRY_UNIMPLEMENTED();
    }
  } else
  {
    // GetValue_5
    ecma_Object_t *lex_env_p = ecma_GetPointer( base.m_Value);

    JERRY_ASSERT( lex_env_p != NULL && lex_env_p->m_IsLexicalEnvironment );

    return ecma_OpGetBindingValue( lex_env_p, ref_p->referenced_name_p, ref_p->is_strict);
  }
} /* ecma_op_get_value */

/**
 * SetValue operation.
 *
 * See also: ECMA-262 v5, 8.7.1

 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_CompletionValue_t
ecma_op_put_value(ecma_Reference_t *ref_p, /**< ECMA-reference */
                  ecma_Value_t value) /**< ECMA-value */
{
  const ecma_Value_t base = ref_p->base;
  const bool is_unresolvable_reference = ecma_IsValueUndefined( base);
  const bool has_primitive_base = ( ecma_IsValueBoolean( base)
                                    || base.m_ValueType == ECMA_TYPE_NUMBER
                                    || base.m_ValueType == ECMA_TYPE_STRING );
  const bool is_property_reference = has_primitive_base || ( base.m_ValueType == ECMA_TYPE_OBJECT );

  if ( is_unresolvable_reference ) // PutValue_3
  {
    if ( ref_p->is_strict ) // PutValue_3.a
    {
      return ecma_MakeThrowValue( ecma_NewStandardError( ECMA_ERROR_REFERENCE));
    } else // PutValue_3.b
    {
      /*
       ecma_Object_t *global_object_p = ecma_GetGlobalObject();

       return global_object_p->[[Put]]( ref_p->referenced_name_p, value, false);
      */

      JERRY_UNIMPLEMENTED();      
    }
  } else if ( is_property_reference ) // PutValue_4
  {
    if ( !has_primitive_base ) // PutValue_4.a
    {
      // PutValue_4.b case 1

      /* return [[Put]]( base as this, ref_p->referenced_name_p, value, ref_p->is_strict); */
      JERRY_UNIMPLEMENTED();      
    } else
    {
      // PutValue_4.b case 2

      /*
       // PutValue_sub_1
       ecma_Object_t *obj_p = ecma_ToObject( base);
       JERRY_ASSERT( obj_p != NULL && !obj_p->m_IsLexicalEnvironment );

       // PutValue_sub_2
       if ( !obj_p->[[CanPut]]( ref_p->referenced_name_p) )
       {
         // PutValue_sub_2.a
         if ( ref_p->is_strict )
         {
           return ecma_MakeThrowValue( ecma_NewStandardError( ECMA_ERROR_TYPE));
         } else
         { // PutValue_sub_2.b
           return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                            ecma_MakeSimpleValue( ECMA_SIMPLE_VALUE_EMPTY),
                                            ECMA_TARGET_ID_RESERVED);
         }
       }

       // PutValue_sub_3
       ecma_Property_t *own_prop = obj_p->[[GetOwnProperty]]( ref_p->referenced_name_p);

       // PutValue_sub_4
       if ( ecma_OpIsDataDescriptor( own_prop) )
       {
         // PutValue_sub_4.a
         if ( ref_p->is_strict )
         {
           return ecma_MakeThrowValue( ecma_NewStandardError( ECMA_ERROR_TYPE));
         } else
         { // PutValue_sub_4.b
           return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                            ecma_MakeSimpleValue( ECMA_SIMPLE_VALUE_EMPTY),
                                            ECMA_TARGET_ID_RESERVED);
         }
       }

       // PutValue_sub_5
       ecma_Property_t *prop = obj_p->[[GetProperty]]( ref_p->referenced_name_p);

       // PutValue_sub_6
       if ( ecma_OpIsAccessorDescriptor( prop) )
       {
         // PutValue_sub_6.a
         ecma_Object_t *setter = ecma_GetPointer( property->u.m_NamedAccessorProperty.m_pSet);
         JERRY_ASSERT( setter != NULL );

         // PutValue_sub_6.b
         return [[Call]]( setter, base as this, value);
       } else // PutValue_sub_7
       {
         // PutValue_sub_7.a
         if ( ref_p->is_strict )
         {
           return ecma_MakeThrowValue( ecma_NewStandardError( ECMA_ERROR_TYPE));
         }
       }

       // PutValue_sub_8
       return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                        ecma_MakeSimpleValue( ECMA_SIMPLE_VALUE_EMPTY),
                                        ECMA_TARGET_ID_RESERVED);
       */

      JERRY_UNIMPLEMENTED();
    }
  } else
  {
    // PutValue_7
    ecma_Object_t *lex_env_p = ecma_GetPointer( base.m_Value);

    JERRY_ASSERT( lex_env_p != NULL && lex_env_p->m_IsLexicalEnvironment );

    return ecma_OpSetMutableBinding( lex_env_p, ref_p->referenced_name_p, value, ref_p->is_strict);
  }
} /* ecma_op_put_value */

/**
 * @}
 * @}
 */
