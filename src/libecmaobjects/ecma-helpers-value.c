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
ecma_IsUndefinedValue( ecma_Value_t value) /**< ecma-value */
{
  return ( value.m_ValueType == ECMA_TYPE_SIMPLE && value.m_Value == ECMA_SIMPLE_VALUE_UNDEFINED );
} /* ecma_IsUndefinedValue */

/**
 * Check if the value is null.
 *
 * @return true - if the value contains ecma-null simple value,
 *         false - otherwise.
 */
bool
ecma_IsNullValue( ecma_Value_t value) /**< ecma-value */
{
  return ( value.m_ValueType == ECMA_TYPE_SIMPLE && value.m_Value == ECMA_SIMPLE_VALUE_NULL );
} /* ecma_IsNullValue */

/**
 * Check if the value is boolean.
 *
 * @return true - if the value contains ecma-true or ecma-false simple values,
 *         false - otherwise.
 */
bool
ecma_IsBooleanValue( ecma_Value_t value) /**< ecma-value */
{
  return ( ( value.m_ValueType == ECMA_TYPE_SIMPLE && value.m_Value == ECMA_SIMPLE_VALUE_FALSE )
           || ( value.m_ValueType == ECMA_TYPE_SIMPLE && value.m_Value == ECMA_SIMPLE_VALUE_TRUE ) );
} /* ecma_IsBooleanValue */

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
  JERRY_ASSERT( ecma_IsBooleanValue( value) );

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
 * @}
 * @}
 */
