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
 * Number value constructor
 */
ecma_Value_t
ecma_MakeNumberValue( ecma_Number_t* num_p) /**< number to reference in value */
{
  JERRY_ASSERT( num_p != NULL );

  ecma_Value_t number_value;

  number_value.m_ValueType = ECMA_TYPE_NUMBER;
  ecma_SetPointer( number_value.m_Value, num_p);

  return number_value;
} /* ecma_MakeNumberValue */

/**
 * String value constructor
 */
ecma_Value_t
ecma_make_string_value( ecma_ArrayFirstChunk_t* ecma_string_p) /**< string to reference in value */
{
  JERRY_ASSERT( ecma_string_p != NULL );

  ecma_Value_t string_value;

  string_value.m_ValueType = ECMA_TYPE_STRING;
  ecma_SetPointer( string_value.m_Value, ecma_string_p);

  return string_value;
} /* ecma_make_string_value */

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
 * TODO:
 *      reference counter in strings
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
 * Free the ecma-value
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
 *
 * @return completion value
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
 *
 * @return 'throw' completion value
 */
ecma_CompletionValue_t
ecma_MakeThrowValue( ecma_Object_t *exception_p) /**< an object */
{
  JERRY_ASSERT( exception_p != NULL && !exception_p->m_IsLexicalEnvironment );

  ecma_Value_t exception = ecma_MakeObjectValue( exception_p);

  return ecma_MakeCompletionValue(ECMA_COMPLETION_TYPE_THROW,
                                  exception,
                                  ECMA_TARGET_ID_RESERVED);                                  
} /* ecma_MakeThrowValue */

/**
 * Empty completion value constructor.
 *
 * @return (normal, empty, reserved) completion value.
 */
ecma_CompletionValue_t
ecma_make_empty_completion_value( void)
{
  return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                   ecma_MakeSimpleValue( ECMA_SIMPLE_VALUE_EMPTY),
                                   ECMA_TARGET_ID_RESERVED);
} /* ecma_make_empty_completion_value */

/**
 * Copy ecma-completion value.
 *
 * @return (source.type, ecma_CopyValue( source.value), source.target).
 */
ecma_CompletionValue_t
ecma_copy_completion_value( ecma_CompletionValue_t value) /**< completion value */
{
  return ecma_MakeCompletionValue( value.type,
                                   ecma_CopyValue( value.value),
                                   value.target);
} /* ecma_copy_completion_value */

/**
 * Free the completion value.
 */
void
ecma_free_completion_value( ecma_CompletionValue_t completion_value) /**< completion value */
{
  switch ( completion_value.type )
    {
    case ECMA_COMPLETION_TYPE_NORMAL:
    case ECMA_COMPLETION_TYPE_THROW:
    case ECMA_COMPLETION_TYPE_RETURN:
      ecma_FreeValue( completion_value.value);
      break;
    case ECMA_COMPLETION_TYPE_CONTINUE:
    case ECMA_COMPLETION_TYPE_BREAK:
    case ECMA_COMPLETION_TYPE_EXIT:
      JERRY_ASSERT( completion_value.value.m_ValueType == ECMA_TYPE_SIMPLE );
      break;
    }
} /* ecma_free_completion_value */

/**
 * Check if the completion value is normal value.
 *
 * @return true - if the completion type is normal,
 *         false - otherwise.
 */
bool
ecma_is_completion_value_normal( ecma_CompletionValue_t value) /**< completion value */
{
  return ( value.type == ECMA_COMPLETION_TYPE_NORMAL );
} /* ecma_is_completion_value_normal */

/**
 * Check if the completion value is throw value.
 *
 * @return true - if the completion type is throw,
 *         false - otherwise.
 */
bool
ecma_is_completion_value_throw( ecma_CompletionValue_t value) /**< completion value */
{
  return ( value.type == ECMA_COMPLETION_TYPE_THROW );
} /* ecma_is_completion_value_throw */

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
