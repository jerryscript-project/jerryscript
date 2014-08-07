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
ecma_completion_value_t
ecma_op_check_object_coercible( ecma_value_t value) /**< ecma-value */
{
  switch ( (ecma_type_t)value.value_type )
  {
    case ECMA_TYPE_SIMPLE:
      {
        if ( ecma_is_value_undefined( value) )
          {
            return ecma_make_throw_value( ecma_new_standard_error( ECMA_ERROR_TYPE));
          }
        else if ( ecma_is_value_boolean( value) )
          {
            break;
          }
        else
          {
            JERRY_UNREACHABLE();
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

  return ecma_make_empty_completion_value();
} /* ecma_op_check_object_coercible */

/**
 * SameValue operation.
 *
 * See also:
 *          ECMA-262 v5, 9.12
 *
 * @return true - if the value are same according to ECMA-defined SameValue algorithm,
 *         false - otherwise.
 */
bool
ecma_op_same_value( ecma_value_t x, /**< ecma-value */
                    ecma_value_t y) /**< ecma-value */
{
  const bool is_x_undefined = ecma_is_value_undefined( x);
  const bool is_x_null = ecma_is_value_null( x);
  const bool is_x_boolean = ecma_is_value_boolean( x);
  const bool is_x_number = ( x.value_type == ECMA_TYPE_NUMBER );
  const bool is_x_string = ( x.value_type == ECMA_TYPE_STRING );
  const bool is_x_object = ( x.value_type == ECMA_TYPE_OBJECT );

  const bool is_y_undefined = ecma_is_value_undefined( y);
  const bool is_y_null = ecma_is_value_null( y);
  const bool is_y_boolean = ecma_is_value_boolean( y);
  const bool is_y_number = ( y.value_type == ECMA_TYPE_NUMBER );
  const bool is_y_string = ( y.value_type == ECMA_TYPE_STRING );
  const bool is_y_object = ( y.value_type == ECMA_TYPE_OBJECT );

  const bool is_types_equal = ( ( is_x_undefined && is_y_undefined )
                                || ( is_x_null && is_y_null )
                                || ( is_x_boolean && is_y_boolean )
                                || ( is_x_number && is_y_number )
                                || ( is_x_string && is_y_string )
                                || ( is_x_object && is_y_object ) );

  if ( !is_types_equal )
    {
      return false;
    }

  if ( is_x_undefined
       || is_x_null )
    {
      return true;
    }

  if ( is_x_number )
    {
      TODO( Implement according to ECMA );

      ecma_number_t *x_num_p = (ecma_number_t*)ecma_get_pointer( x.value);
      ecma_number_t *y_num_p = (ecma_number_t*)ecma_get_pointer( y.value);

      return ( *x_num_p == *y_num_p );
    }

  if ( is_x_string )
    {
      ecma_array_first_chunk_t* x_str_p = (ecma_array_first_chunk_t*)( ecma_get_pointer(x.value) );
      ecma_array_first_chunk_t* y_str_p = (ecma_array_first_chunk_t*)( ecma_get_pointer(y.value) );

      return ecma_compare_ecma_string_to_ecma_string( x_str_p, y_str_p);  
    }

  if ( is_x_boolean )
    {
      return ( ecma_is_value_true( x) == ecma_is_value_true( y) );
    }

  JERRY_ASSERT( is_x_object );

  return ( ecma_get_pointer( x.value) == ecma_get_pointer( y.value) );
} /* ecma_op_same_value */

/**
 * ToPrimitive operation.
 *
 * See also:
 *          ECMA-262 v5, 9.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_to_primitive( ecma_value_t value, /**< ecma-value */
                      ecma_preferred_type_hint_t preferred_type) /**< preferred type hint */
{
  switch ( (ecma_type_t)value.value_type )
  {
    case ECMA_TYPE_SIMPLE:
    case ECMA_TYPE_NUMBER:
    case ECMA_TYPE_STRING:
      {
        return ecma_make_completion_value( ECMA_COMPLETION_TYPE_NORMAL,
                                           ecma_copy_value( value, true),
                                           ECMA_TARGET_ID_RESERVED);
      }
    case ECMA_TYPE_OBJECT:
      {
        JERRY_UNIMPLEMENTED_REF_UNUSED_VARS(preferred_type);
      }
    case ECMA_TYPE__COUNT:
      {
        JERRY_UNREACHABLE();
      }
  }

  JERRY_UNREACHABLE();
} /* ecma_op_to_primitive */

/**
 * ToBoolean operation.
 *
 * See also:
 *          ECMA-262 v5, 9.2
 *
 * @return completion value
 *         Returned value is simple and so need not be freed.
 *         However, ecma_free_completion_value may be called for it, but it is a no-op.
 */
ecma_completion_value_t
ecma_op_to_boolean( ecma_value_t value) /**< ecma-value */
{
  switch ( (ecma_type_t)value.value_type )
  {
    case ECMA_TYPE_NUMBER:
      {
        ecma_number_t *num_p = ecma_get_pointer( value.value);

        TODO( Implement according to ECMA );

        return ecma_make_simple_completion_value( ( *num_p == 0 ) ? ECMA_SIMPLE_VALUE_FALSE
                                                                  : ECMA_SIMPLE_VALUE_TRUE );

        break;
      }
    case ECMA_TYPE_SIMPLE:
      {
        if ( ecma_is_value_boolean (value ) )
        {
          return ecma_make_simple_completion_value( value.value);
        } else if ( ecma_is_value_undefined (value)
                    || ecma_is_value_null( value) )
        {
          return ecma_make_simple_completion_value( ECMA_SIMPLE_VALUE_FALSE);
        } else
        {
          JERRY_UNREACHABLE();
        }

        break;
      }
    case ECMA_TYPE_STRING:
      {
        ecma_array_first_chunk_t *str_p = ecma_get_pointer( value.value);

        return ecma_make_simple_completion_value( ( str_p->header.unit_number == 0 ) ? ECMA_SIMPLE_VALUE_FALSE
                                                                                     : ECMA_SIMPLE_VALUE_TRUE );

        break;
      }
    case ECMA_TYPE_OBJECT:
      {
        return ecma_make_simple_completion_value( ECMA_SIMPLE_VALUE_TRUE);

        break;
      }
    case ECMA_TYPE__COUNT:
      {
        JERRY_UNREACHABLE();
      }
  }

  JERRY_UNREACHABLE();
} /* ecma_op_to_boolean */

/**
 * ToNumber operation.
 *
 * See also:
 *          ECMA-262 v5, 9.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_to_number( ecma_value_t value) /**< ecma-value */
{
  switch ( (ecma_type_t)value.value_type )
  {
    case ECMA_TYPE_NUMBER:
      {
        return ecma_make_completion_value( ECMA_COMPLETION_TYPE_NORMAL,
                                           ecma_copy_value( value, true),
                                           ECMA_TARGET_ID_RESERVED);
      }
    case ECMA_TYPE_SIMPLE:
    case ECMA_TYPE_STRING:
      {
        JERRY_UNIMPLEMENTED();
      }
    case ECMA_TYPE_OBJECT:
      {
        ecma_completion_value_t completion_to_primitive = ecma_op_to_primitive( value, ECMA_PREFERRED_TYPE_NUMBER);
        JERRY_ASSERT( ecma_is_completion_value_normal( completion_to_primitive) );

        ecma_completion_value_t completion_to_number = ecma_op_to_number( completion_to_primitive.value);
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
ecma_completion_value_t
ecma_op_to_object( ecma_value_t value) /**< ecma-value */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS( value);
} /* ecma_op_to_object */

/**
 * @}
 * @}
 */
