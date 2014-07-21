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

/** \addtogroup ecma ---TODO---
 * @{
 * 
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

#include "ecma-alloc.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "globals.h"

/**
 * Check if the value is undefined.
 *
 * @return true - if the value contains ecma-undefined simple value,
 *         false - otherwise.
 */
bool
ecma_IsValueUndefined( ecma_Value_t value) /**< ecma-value */
{
  return ( value.m_ValueType == ECMA_TYPE_SIMPLE && value.m_Value == ECMA_SIMPLE_VALUE_UNDEFINED );
} /* ecma_IsValueUndefined */

/**
 * Check if the value is null.
 *
 * @return true - if the value contains ecma-null simple value,
 *         false - otherwise.
 */
bool
ecma_IsValueNull( ecma_Value_t value) /**< ecma-value */
{
  return ( value.m_ValueType == ECMA_TYPE_SIMPLE && value.m_Value == ECMA_SIMPLE_VALUE_NULL );
} /* ecma_IsValueNull */

/**
 * Check if the value is boolean.
 *
 * @return true - if the value contains ecma-true or ecma-false simple values,
 *         false - otherwise.
 */
bool
ecma_IsValueBoolean( ecma_Value_t value) /**< ecma-value */
{
  return ( ( value.m_ValueType == ECMA_TYPE_SIMPLE && value.m_Value == ECMA_SIMPLE_VALUE_FALSE )
           || ( value.m_ValueType == ECMA_TYPE_SIMPLE && value.m_Value == ECMA_SIMPLE_VALUE_TRUE ) );
} /* ecma_IsValueBoolean */

/**
 * Check if the value is true.
 *
 * Warning:
 *         value must be boolean
 *
 * @return true - if the value contains ecma-true simple value,
 *         false - otherwise.
 */
bool
ecma_IsValueTrue( ecma_Value_t value) /**< ecma-value */
{
  JERRY_ASSERT( ecma_IsValueBoolean( value) );

  return ( value.m_ValueType == ECMA_TYPE_SIMPLE && value.m_Value == ECMA_SIMPLE_VALUE_TRUE );
} /* ecma_IsValueTrue */

/**
 * Simple value constructor
 */
ecma_Value_t
ecma_MakeSimpleValue( ecma_SimpleValue_t value) /**< simple value */
{
  return (ecma_Value_t) { .m_ValueType = ECMA_TYPE_SIMPLE, .m_Value = value };
} /* ecma_MakeSimpleValue */

/**
 * Object value constructor
 */
ecma_Value_t
ecma_MakeObjectValue( ecma_Object_t* object_p) /**< object to reference in value */
{
  JERRY_ASSERT( object_p != NULL );

  ecma_Value_t object_value;

  object_value.m_ValueType = ECMA_TYPE_OBJECT;
  ecma_SetPointer( object_value.m_Value, object_p);

  return object_value;
} /* ecma_MakeObjectValue */

/**
 * Copy ecma-value.
 *
 * Note:
 *  Operation algorithm.
 *   switch (valuetype)
 *    case simple:
 *      simply return the value as it was passed;
 *    case number/string:
 *      copy the number/string
 *      and return new ecma-value
 *      pointing to copy of the number/string;
 *    case object;
 *      increase reference counter of the object
 *      and return the value as it was passed.
 *
 * @return See note.
 */
ecma_Value_t
ecma_CopyValue( const ecma_Value_t value) /**< ecma-value */
{
  ecma_Value_t value_copy;

  switch ( (ecma_Type_t)value.m_ValueType )
  {
    case ECMA_TYPE_SIMPLE:
      {
        value_copy = value;

        break;
      }
    case ECMA_TYPE_NUMBER:
      {
        ecma_Number_t *num_p = ecma_GetPointer( value.m_Value);
        JERRY_ASSERT( num_p != NULL );

        ecma_Number_t *number_copy_p = ecma_AllocNumber();
        *number_copy_p = *num_p;

        value_copy = (ecma_Value_t) { .m_ValueType = ECMA_TYPE_NUMBER };
        ecma_SetPointer( value_copy.m_Value, number_copy_p);

        break;
      }
    case ECMA_TYPE_STRING:
      {
        ecma_ArrayFirstChunk_t *string_p = ecma_GetPointer( value.m_Value);
        JERRY_ASSERT( string_p != NULL );

        ecma_ArrayFirstChunk_t *string_copy_p = ecma_DuplicateEcmaString( string_p);

        value_copy = (ecma_Value_t) { .m_ValueType = ECMA_TYPE_STRING };
        ecma_SetPointer( value_copy.m_Value, string_copy_p);

        break;
      }
    case ECMA_TYPE_OBJECT:
      {
        ecma_Object_t *obj_p = ecma_GetPointer( value.m_Value);
        JERRY_ASSERT( obj_p != NULL );

        ecma_RefObject( obj_p);

        value_copy = value;

        break;
      }
    case ECMA_TYPE__COUNT:
      {
        JERRY_UNREACHABLE();
      }
  }

  return value_copy;
} /* ecma_CopyValue */

/**
 * Free memory used for the value
 */
void
ecma_FreeValue( ecma_Value_t value) /**< value description */
{
  switch ( (ecma_Type_t) value.m_ValueType )
  {
    case ECMA_TYPE_SIMPLE:
      {
        /* doesn't hold additional memory */
        break;
      }

    case ECMA_TYPE_NUMBER:
      {
        ecma_Number_t *pNumber = ecma_GetPointer( value.m_Value);
        ecma_DeallocNumber( pNumber);
        break;
      }

    case ECMA_TYPE_STRING:
      {
        ecma_ArrayFirstChunk_t *pString = ecma_GetPointer( value.m_Value);
        ecma_FreeArray( pString);
        break;
      }

    case ECMA_TYPE_OBJECT:
      {
        ecma_DerefObject( ecma_GetPointer( value.m_Value));
        break;
      }

    case ECMA_TYPE__COUNT:
      {
        JERRY_UNREACHABLE();
      }
  }
} /* ecma_FreeValue */

/**
 * Completion value constructor
 */
ecma_CompletionValue_t
ecma_MakeCompletionValue(ecma_CompletionType_t type, /**< type */
                         ecma_Value_t value, /**< value */
                         uint8_t target) /**< target */
{
  return (ecma_CompletionValue_t) { .type = type, .value = value, .target = target };
} /* ecma_MakeCompletionValue */

/**
 * Throw completion value constructor.
 */
ecma_CompletionValue_t
ecma_MakeThrowValue( ecma_Object_t *exception_p) /**< an object */
{
  JERRY_ASSERT( exception_p != NULL && !exception_p->m_IsLexicalEnvironment );

  ecma_Value_t exception;
  exception.m_ValueType = ECMA_TYPE_OBJECT;
  ecma_SetPointer( exception.m_Value, exception_p);

  return ecma_MakeCompletionValue(ECMA_COMPLETION_TYPE_THROW,
                                  exception,
                                  ECMA_TARGET_ID_RESERVED);                                  
} /* ecma_MakeThrowValue */

/**
 * Check if the completion value is specified normal simple value.
 *
 * @return true - if the completion type is normal and
 *                value contains specified simple ecma-value,
 *         false - otherwise.
 */
bool
ecma_is_completion_value_normal_simple_value(ecma_CompletionValue_t value, /**< completion value */
                                             ecma_SimpleValue_t simple_value) /**< simple value to check for equality with */
{
  return ( value.type == ECMA_COMPLETION_TYPE_NORMAL
           && value.value.m_ValueType == ECMA_TYPE_SIMPLE
           && value.value.m_Value == simple_value );
} /* ecma_is_completion_value_normal_simple_value */

/**
 * Check if the completion value is normal true.
 *
 * @return true - if the completion type is normal and
 *                value contains ecma-true simple value,
 *         false - otherwise.
 */
bool
ecma_IsCompletionValueNormalTrue( ecma_CompletionValue_t value) /**< completion value */
{
  return ecma_is_completion_value_normal_simple_value( value, ECMA_SIMPLE_VALUE_TRUE);
} /* ecma_IsCompletionValueNormalTrue */

/**
 * Check if the completion value is normal false.
 *
 * @return true - if the completion type is normal and
 *                value contains ecma-false simple value,
 *         false - otherwise.
 */
bool
ecma_IsCompletionValueNormalFalse( ecma_CompletionValue_t value) /**< completion value */
{
  return ecma_is_completion_value_normal_simple_value( value, ECMA_SIMPLE_VALUE_FALSE);
} /* ecma_IsCompletionValueNormalFalse */

/**
 * @}
 * @}
 */
