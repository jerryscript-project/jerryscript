/* Copyright JS Foundation and other contributors, http://js.foundation
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

#include "ecma-arraybuffer-object.h"
#include "ecma-atomics-object.h"
#include "ecma-bigint.h"
#include "ecma-builtins.h"
#include "ecma-errors.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-shared-arraybuffer-object.h"
#include "ecma-typedarray-object.h"

#include "jrt.h"

#if JERRY_BUILTIN_ATOMICS

#define ECMA_BUILTINS_INTERNAL

#include "ecma-builtins-internal.h"

/**
 * This object has a custom dispatch function.
 */
#define BUILTIN_CUSTOM_DISPATCH

/**
 * List of built-in routine identifiers.
 */
enum
{
  ECMA_ATOMICS_ROUTINE_START = 0, /**< Special value, should be ignored */
  ECMA_ATOMICS_ROUTINE_ADD, /**< Atomics add routine */
  ECMA_ATOMICS_ROUTINE_AND, /**< Atomics and routine */
  ECMA_ATOMICS_ROUTINE_COMPAREEXCHANGE, /**< Atomics compare exchange routine */
  ECMA_ATOMICS_ROUTINE_EXCHANGE, /**< Atomics exchange routine */
  ECMA_ATOMICS_ROUTINE_ISLOCKFREE, /**< Atomics is lock free routine */
  ECMA_ATOMICS_ROUTINE_LOAD, /**< Atomics load routine */
  ECMA_ATOMICS_ROUTINE_OR, /**< Atomics or routine */
  ECMA_ATOMICS_ROUTINE_STORE, /**< Atomics store routine */
  ECMA_ATOMICS_ROUTINE_SUB, /**< Atomics sub routine */
  ECMA_ATOMICS_ROUTINE_WAIT, /**< Atomics wait routine */
  ECMA_ATOMICS_ROUTINE_NOTIFY, /**< Atomics notify routine */
  ECMA_ATOMICS_ROUTINE_XOR, /**< Atomics xor routine */
};

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-atomics.inc.h"
#define BUILTIN_UNDERSCORED_ID  atomics

#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup atomics ECMA Atomics object built-in
 * @{
 */

/**
 * Convert ecma_number to the appropriate type according to the element type of typedarray.
 */
static ecma_value_t
ecma_convert_number_to_typed_array_type (ecma_number_t num, /**< ecma_number argument */
                                         ecma_typedarray_type_t element_type) /**< element type of typedarray */
{
  uint32_t value = ecma_typedarray_setter_number_to_uint32 (num);

  switch (element_type)
  {
    case ECMA_INT8_ARRAY:
    {
      return ecma_make_number_value ((int8_t) value);
    }
    case ECMA_UINT8_ARRAY:
    {
      return ecma_make_number_value ((uint8_t) value);
    }
    case ECMA_INT16_ARRAY:
    {
      return ecma_make_number_value ((int16_t) value);
    }
    case ECMA_UINT16_ARRAY:
    {
      return ecma_make_number_value ((uint16_t) value);
    }
    case ECMA_INT32_ARRAY:
    {
      return ecma_make_number_value ((int32_t) value);
    }
    default:
    {
      JERRY_ASSERT (element_type == ECMA_UINT32_ARRAY);

      return ecma_make_number_value (value);
    }
  }
} /* ecma_convert_number_to_typed_array_type */

/**
 * The Atomics object's 'compareExchange' routine
 *
 * See also: ES12 25.4.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_atomics_compare_exchange (ecma_value_t typedarray, /**< typedArray argument */
                                       ecma_value_t index, /**< index argument */
                                       ecma_value_t expected_value, /**< expectedValue argument */
                                       ecma_value_t replacement_value) /**< replacementValue argument*/
{
  ecma_value_t buffer = ecma_validate_integer_typedarray (typedarray, false);

  if (ECMA_IS_VALUE_ERROR (buffer))
  {
    return buffer;
  }

  uint32_t idx = ecma_validate_atomic_access (typedarray, index);

  if (idx == ECMA_STRING_NOT_ARRAY_INDEX)
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_object_t *typedarray_p = ecma_get_object_from_value (typedarray);
  ecma_typedarray_info_t target_info = ecma_typedarray_get_info (typedarray_p);

  if (ECMA_ARRAYBUFFER_LAZY_ALLOC (target_info.array_buffer_p))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_typedarray_type_t element_type = target_info.id;
  ecma_value_t expected;
  ecma_value_t replacement;

#if JERRY_BUILTIN_BIGINT
  if (ECMA_TYPEDARRAY_IS_BIGINT_TYPE (element_type))
  {
    expected = ecma_bigint_to_bigint (expected_value, false);

    if (ECMA_IS_VALUE_ERROR (expected))
    {
      return expected;
    }

    if (element_type == ECMA_BIGUINT64_ARRAY)
    {
      uint64_t num;
      bool sign;

      ecma_bigint_get_digits_and_sign (expected, &num, 1, &sign);

      if (sign)
      {
        num = (uint64_t) (-(int64_t) num);
      }

      if (expected != ECMA_BIGINT_ZERO)
      {
        ecma_deref_bigint (ecma_get_extended_primitive_from_value (expected));
      }

      expected = ecma_bigint_create_from_digits (&num, 1, false);
    }

    replacement = ecma_bigint_to_bigint (replacement_value, false);

    if (ECMA_IS_VALUE_ERROR (replacement))
    {
      ecma_free_value (expected);
      return replacement;
    }
  }
  else
#endif /* JERRY_BUILTIN_BIGINT */
  {
    ecma_number_t tmp_exp;
    ecma_number_t tmp_rep;

    expected = ecma_op_to_integer (expected_value, &tmp_exp);

    if (ECMA_IS_VALUE_ERROR (expected))
    {
      return expected;
    }

    expected = ecma_convert_number_to_typed_array_type (tmp_exp, element_type);

    replacement = ecma_op_to_integer (replacement_value, &tmp_rep);

    if (ECMA_IS_VALUE_ERROR (replacement))
    {
      ecma_free_value (expected);
      return replacement;
    }

    replacement = ecma_make_number_value (tmp_rep);
  }

  uint8_t element_size = target_info.element_size;
  uint32_t offset = target_info.offset;
  uint32_t indexed_position = idx * element_size + offset;

  if (ecma_arraybuffer_is_detached (ecma_get_object_from_value (buffer)))
  {
    ecma_free_value (expected);
    ecma_free_value (replacement);
    return ecma_raise_type_error (ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  ecma_typedarray_getter_fn_t typedarray_getter_cb = ecma_get_typedarray_getter_fn (element_type);

  ecma_object_t *buffer_obj_p = ecma_get_object_from_value (buffer);
  lit_utf8_byte_t *pos = ecma_arraybuffer_get_buffer (buffer_obj_p) + indexed_position;
  ecma_value_t stored_value = typedarray_getter_cb (pos);

  // TODO: Handle shared array buffers differently.
  if (ecma_op_same_value (stored_value, expected))
  {
    ecma_typedarray_setter_fn_t typedarray_setter_cb = ecma_get_typedarray_setter_fn (element_type);
    typedarray_setter_cb (ecma_arraybuffer_get_buffer (buffer_obj_p) + indexed_position, replacement);
  }

  ecma_free_value (expected);
  ecma_free_value (replacement);

  return stored_value;
} /* ecma_builtin_atomics_compare_exchange */

/**
 * The Atomics object's 'isLockFree' routine
 *
 * See also: ES11 24.4.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_atomics_is_lock_free (ecma_value_t size) /**< size argument */
{
  JERRY_UNUSED (size);

  return ECMA_VALUE_FALSE;
} /* ecma_builtin_atomics_is_lock_free */

/**
 * The Atomics object's 'store' routine
 *
 * See also: ES11 24.4.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_atomics_store (ecma_value_t typedarray, /**< typedArray argument */
                            ecma_value_t index, /**< index argument */
                            ecma_value_t value) /**< value argument */
{
  /* 1. */
  ecma_value_t buffer = ecma_validate_integer_typedarray (typedarray, false);

  if (ECMA_IS_VALUE_ERROR (buffer))
  {
    return buffer;
  }

  /* 2. */
  uint32_t idx = ecma_validate_atomic_access (typedarray, index);

  if (idx == ECMA_STRING_NOT_ARRAY_INDEX)
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_object_t *typedarray_p = ecma_get_object_from_value (typedarray);
  ecma_typedarray_info_t target_info = ecma_typedarray_get_info (typedarray_p);

  if (ECMA_ARRAYBUFFER_LAZY_ALLOC (target_info.array_buffer_p))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_typedarray_type_t element_type = target_info.id;

  ecma_typedarray_setter_fn_t typedarray_setter_cb = ecma_get_typedarray_setter_fn (element_type);

  ecma_value_t value_to_store;
#if JERRY_BUILTIN_BIGINT
  if (element_type == ECMA_BIGINT64_ARRAY || element_type == ECMA_BIGUINT64_ARRAY)
  {
    value_to_store = ecma_bigint_to_bigint (value, false);

    if (ECMA_IS_VALUE_ERROR (value_to_store))
    {
      return value_to_store;
    }
  }
  else
#endif /* JERRY_BUILTIN_BIGINT */
  {
    ecma_number_t num_int;

    value_to_store = ecma_op_to_integer (value, &num_int);

    if (ECMA_IS_VALUE_ERROR (value_to_store))
    {
      return value_to_store;
    }

    if (ecma_number_is_zero (num_int) && ecma_number_is_negative (num_int))
    {
      num_int = (ecma_number_t) 0;
    }

    value_to_store = ecma_make_number_value (num_int);
  }

  ecma_object_t *buffer_obj_p = ecma_get_object_from_value (buffer);

  if (ecma_arraybuffer_is_detached (buffer_obj_p))
  {
    ecma_free_value (value_to_store);
    return ecma_raise_type_error (ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  uint8_t element_size = target_info.element_size;

  uint32_t offset = target_info.offset;

  uint32_t indexed_position = idx * element_size + offset;

  // TODO: Handle shared array buffers differently.
  typedarray_setter_cb (ecma_arraybuffer_get_buffer (buffer_obj_p) + indexed_position, value_to_store);

  return value_to_store;
} /* ecma_builtin_atomics_store */

/**
 * The Atomics object's 'wait' routine
 *
 * See also: ES11 24.4.11
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_atomics_wait (ecma_value_t typedarray, /**< typedArray argument */
                           ecma_value_t index, /**< index argument */
                           ecma_value_t value, /**< value argument */
                           ecma_value_t timeout) /**< timeout argument */
{
  JERRY_UNUSED (typedarray);
  JERRY_UNUSED (index);
  JERRY_UNUSED (value);
  JERRY_UNUSED (timeout);

  return ecma_make_uint32_value (0);
} /* ecma_builtin_atomics_wait */

/**
 * The Atomics object's 'notify' routine
 *
 * See also: ES11 24.4.12
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_atomics_notify (ecma_value_t typedarray, /**< typedArray argument */
                             ecma_value_t index, /**< index argument */
                             ecma_value_t count) /**< count argument */
{
  JERRY_UNUSED (typedarray);
  JERRY_UNUSED (index);
  JERRY_UNUSED (count);

  return ecma_make_uint32_value (0);
} /* ecma_builtin_atomics_notify */

/**
 * Dispatcher of the built-in's routines
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_atomics_dispatch_routine (uint8_t builtin_routine_id, /**< built-in wide routine identifier */
                                       ecma_value_t this_arg, /**< 'this' argument value */
                                       const ecma_value_t arguments_list_p[], /**< list of arguments
                                                                               *   passed to routine */
                                       uint32_t arguments_number) /**< length of arguments' list */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t arg1 = arguments_list_p[0];
  ecma_value_t arg2 = arguments_list_p[1];
  ecma_value_t arg3 = arguments_list_p[2];
  ecma_value_t arg4 = (arguments_number > 3) ? arguments_list_p[3] : ECMA_VALUE_UNDEFINED;

  ecma_atomics_op_t type;

  switch (builtin_routine_id)
  {
    case ECMA_ATOMICS_ROUTINE_ADD:
    {
      type = ECMA_ATOMICS_ADD;
      break;
    }
    case ECMA_ATOMICS_ROUTINE_AND:
    {
      type = ECMA_ATOMICS_AND;
      break;
    }
    case ECMA_ATOMICS_ROUTINE_COMPAREEXCHANGE:
    {
      return ecma_builtin_atomics_compare_exchange (arg1, arg2, arg3, arg4);
    }
    case ECMA_ATOMICS_ROUTINE_EXCHANGE:
    {
      type = ECMA_ATOMICS_EXCHANGE;
      break;
    }
    case ECMA_ATOMICS_ROUTINE_ISLOCKFREE:
    {
      return ecma_builtin_atomics_is_lock_free (arg1);
    }
    case ECMA_ATOMICS_ROUTINE_LOAD:
    {
      return ecma_atomic_load (arg1, arg2);
    }
    case ECMA_ATOMICS_ROUTINE_OR:
    {
      type = ECMA_ATOMICS_OR;
      break;
    }
    case ECMA_ATOMICS_ROUTINE_STORE:
    {
      return ecma_builtin_atomics_store (arg1, arg2, arg3);
    }
    case ECMA_ATOMICS_ROUTINE_SUB:
    {
      type = ECMA_ATOMICS_SUBTRACT;
      break;
    }
    case ECMA_ATOMICS_ROUTINE_WAIT:
    {
      return ecma_builtin_atomics_wait (arg1, arg2, arg3, arg4);
    }
    case ECMA_ATOMICS_ROUTINE_NOTIFY:
    {
      return ecma_builtin_atomics_notify (arg1, arg2, arg3);
    }
    case ECMA_ATOMICS_ROUTINE_XOR:
    {
      type = ECMA_ATOMICS_XOR;
      break;
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
  return ecma_atomic_read_modify_write (arg1, arg2, arg3, type);
} /* ecma_builtin_atomics_dispatch_routine */

/**
 * @}
 * @}
 * @}
 */

#endif /* JERRY_BUILTIN_ATOMICS */
