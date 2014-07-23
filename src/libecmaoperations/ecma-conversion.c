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
 * Implementation of ECMA-defined conversion routines
 */

#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-globals.h"

/** \addtogroup ecma ---TODO---
 * @{
 * 
 * \addtogroup ecmaconversion ECMA conversion routines
 * @{
 */

/**
 * CheckObjectCoercible operation.
 *
 * See also:
 *          ECMA-262 v5, 9.10
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_CompletionValue_t
ecma_op_check_object_coercible( ecma_Value_t value) /**< ecma-value */
{
  switch ( (ecma_Type_t)value.ValueType )
  {
    case ECMA_TYPE_SIMPLE:
      {
        switch ( (ecma_SimpleValue_t)value.Value )
        {
          case ECMA_SIMPLE_VALUE_UNDEFINED:
          case ECMA_SIMPLE_VALUE_NULL:
            {
              return ecma_MakeThrowValue( ecma_NewStandardError( ECMA_ERROR_TYPE));
            }
          case ECMA_SIMPLE_VALUE_FALSE:
          case ECMA_SIMPLE_VALUE_TRUE:
            {
              break;
            }
          case ECMA_SIMPLE_VALUE_EMPTY:
          case ECMA_SIMPLE_VALUE_ARRAY_REDIRECT:
          case ECMA_SIMPLE_VALUE__COUNT:
            {
              JERRY_UNREACHABLE();
            }
        }

        break;
      }
    case ECMA_TYPE_NUMBER:
    case ECMA_TYPE_STRING:
    case ECMA_TYPE_OBJECT:
      {
        break;
      }
    case ECMA_TYPE__COUNT:
      {
        JERRY_UNREACHABLE();
      }
  }

  return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                   ecma_MakeSimpleValue( ECMA_SIMPLE_VALUE_EMPTY),
                                   ECMA_TARGET_ID_RESERVED);
} /* ecma_op_check_object_coercible */

/**
 * ToPrimitive operation.
 *
 * See also:
 *          ECMA-262 v5, 9.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_CompletionValue_t
ecma_op_to_primitive( ecma_Value_t value) /**< ecma-value */
{
  switch ( (ecma_Type_t)value.ValueType )
  {
    case ECMA_TYPE_SIMPLE:
    case ECMA_TYPE_NUMBER:
    case ECMA_TYPE_STRING:
      {
        return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                         ecma_CopyValue( value),
                                         ECMA_TARGET_ID_RESERVED);
      }
    case ECMA_TYPE_OBJECT:
      {
        JERRY_UNIMPLEMENTED();
      }
    case ECMA_TYPE__COUNT:
      {
        JERRY_UNREACHABLE();
      }
  }

  JERRY_UNREACHABLE();
} /* ecma_op_to_primitive */

/**
 * ToNumber operation.
 *
 * See also:
 *          ECMA-262 v5, 9.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_CompletionValue_t
ecma_op_to_number( ecma_Value_t value) /**< ecma-value */
{
  switch ( (ecma_Type_t)value.ValueType )
  {
    case ECMA_TYPE_NUMBER:
      {
        return ecma_MakeCompletionValue( ECMA_COMPLETION_TYPE_NORMAL,
                                         ecma_CopyValue( value),
                                         ECMA_TARGET_ID_RESERVED);
      }
    case ECMA_TYPE_SIMPLE:
    case ECMA_TYPE_STRING:
      {
        JERRY_UNIMPLEMENTED();
      }
    case ECMA_TYPE_OBJECT:
      {
        ecma_CompletionValue_t completion_to_primitive = ecma_op_to_primitive( value);
        JERRY_ASSERT( ecma_is_completion_value_normal( completion_to_primitive) );

        ecma_CompletionValue_t completion_to_number = ecma_op_to_number( completion_to_primitive.value);
        ecma_free_completion_value( completion_to_primitive);

        return completion_to_number;
      }
    case ECMA_TYPE__COUNT:
      {
        JERRY_UNREACHABLE();
      }
  }

  JERRY_UNREACHABLE();
} /* ecma_op_to_number */

/**
 * ToObject operation.
 *
 * See also:
 *          ECMA-262 v5, 9.9
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_CompletionValue_t
ecma_op_to_object( ecma_Value_t value) /**< ecma-value */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS( value);
} /* ecma_op_to_object */

/**
 * @}
 * @}
 */
