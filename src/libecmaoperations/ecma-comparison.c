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

#include "ecma-comparison.h"
#include "ecma-globals.h"
#include "globals.h"

/** \addtogroup ecma ---TODO---
 * @{
 * 
 * \addtogroup ecmacomparison ECMA comparison
 * @{
 */

/**
 * ECMA abstract equality comparison routine.
 *
 * See also: ECMA-262 v5, 11.9.3
 *
 * @return true - if values are equal,
 *         false - otherwise.
 */
bool
ecma_abstract_equality_compare(ecma_Value_t x, /**< first operand */
                               ecma_Value_t y) /**< second operand */
{
  const bool is_x_undefined = ecma_IsValueUndefined( x);
  const bool is_x_null = ecma_IsValueNull( x);
  const bool is_x_boolean = ecma_IsValueBoolean( x);
  const bool is_x_number = ( x.m_ValueType == ECMA_TYPE_NUMBER );
  const bool is_x_string = ( x.m_ValueType == ECMA_TYPE_STRING );
  const bool is_x_object = ( x.m_ValueType == ECMA_TYPE_OBJECT );

  const bool is_y_undefined = ecma_IsValueUndefined( y);
  const bool is_y_null = ecma_IsValueNull( y);
  const bool is_y_boolean = ecma_IsValueBoolean( y);
  const bool is_y_number = ( y.m_ValueType == ECMA_TYPE_NUMBER );
  const bool is_y_string = ( y.m_ValueType == ECMA_TYPE_STRING );
  const bool is_y_object = ( y.m_ValueType == ECMA_TYPE_OBJECT );

  const bool is_types_equal = ( ( is_x_undefined && is_y_undefined )
                                || ( is_x_null && is_y_null )
                                || ( is_x_boolean && is_y_boolean )
                                || ( is_x_number && is_y_number )
                                || ( is_x_string && is_y_string )
                                || ( is_x_object && is_y_object ) );

  if ( is_types_equal )
  {
    // 1.

    if ( is_x_undefined
         || is_x_null )
    {
      // a., b.
      return true;
    } else if ( is_x_number )
    { // c.
      ecma_Number_t x_num = *(ecma_Number_t*)( ecma_GetPointer(x.m_Value) );
      ecma_Number_t y_num = *(ecma_Number_t*)( ecma_GetPointer(y.m_Value) );

      TODO( Implement according to ECMA );

      return (x_num == y_num);
    } else if ( is_x_string )
    { // d.
      ecma_ArrayFirstChunk_t* x_str = (ecma_ArrayFirstChunk_t*)( ecma_GetPointer(x.m_Value) );
      ecma_ArrayFirstChunk_t* y_str = (ecma_ArrayFirstChunk_t*)( ecma_GetPointer(y.m_Value) );

      return ecma_CompareEcmaStringToEcmaString( x_str, y_str);
    } else if ( is_x_boolean )
    { // e.
      return ( x.m_Value == y.m_Value );
    } else
    { // f.
      JERRY_ASSERT( is_x_object );
      
      return ( x.m_Value == y.m_Value );
    }
  } else if ( ( is_x_null && is_y_undefined )
              || ( is_x_undefined && is_y_null ) )
  { // 2., 3.
    return true;
  } else
  {
    JERRY_UNIMPLEMENTED();
  }
} /* ecma_abstract_equality_compare */

/**
 * @}
 * @}
 */
