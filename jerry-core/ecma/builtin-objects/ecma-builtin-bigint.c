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

#include "ecma-big-uint.h"
#include "ecma-bigint.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"

#if JERRY_BUILTIN_BIGINT

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
  ECMA_BUILTIN_BIGINT_START = 0, /**< Special value, should be ignored */
  ECMA_BUILTIN_BIGINT_AS_INT_N, /**< The 'asIntN' routine of the BigInt object */
  ECMA_BUILTIN_BIGINT_AS_U_INT_N, /**< The 'asUintN' routine of the BigInt object */
};

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-bigint.inc.h"
#define BUILTIN_UNDERSCORED_ID  bigint
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup bigint ECMA BigInt object built-in
 * @{
 */

/**
 * The BigInt object's 'asIntN' and 'asUintN' routines
 *
 * See also:
 *          ECMA-262 v15, 21.2.2
 *
 * @return ecma value
 *         Truncated BigInt value for the given number of LSBs
 *         as an integer or unsigned integer.
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_bigint_object_as_int_n (ecma_value_t bits, /**< number of bits */
                                     ecma_value_t bigint, /**< bigint number */
                                     bool is_signed) /**< The operation is signed */
{
  ecma_number_t input_bits;
  ecma_value_t bit_value = ecma_op_to_index (bits, &input_bits);

  if (ECMA_IS_VALUE_ERROR (bit_value))
  {
    return bit_value;
  }

  ecma_value_t bigint_value = ecma_bigint_to_bigint (bigint, false);

  if (ECMA_IS_VALUE_ERROR (bigint_value))
  {
    return bigint_value;
  }

  if (input_bits == 0 || bigint_value == ECMA_BIGINT_ZERO)
  {
    ecma_free_value (bigint_value);
    return ECMA_BIGINT_ZERO;
  }

  ecma_extended_primitive_t *input_bigint_p = ecma_get_extended_primitive_from_value (bigint_value);
  uint32_t bigint_size = ECMA_BIGINT_GET_SIZE (input_bigint_p);

  if (input_bits >= UINT32_MAX)
  {
    return bigint_value;
  }

  uint8_t input_bigint_sign = input_bigint_p->u.bigint_sign_and_size & ECMA_BIGINT_SIGN;
  const uint32_t size_of_divisor_in_bits = sizeof (uint32_t) * JERRY_BITSINBYTE;
  uint32_t whole_part = (uint32_t) input_bits / size_of_divisor_in_bits;
  uint32_t remainder = (uint32_t) input_bits % size_of_divisor_in_bits;
  uint32_t input_bit_length =
    (whole_part == 0 || remainder == 0) ? (uint32_t) input_bits : (whole_part + 1) * size_of_divisor_in_bits;

  uint32_t input_byte_size = (uint32_t) input_bits / JERRY_BITSINBYTE;

  if ((uint32_t) input_bits % JERRY_BITSINBYTE != 0)
  {
    input_byte_size += 1;
  }

  const uint32_t input_bits_in_byte = (uint32_t) input_bit_length / JERRY_BITSINBYTE;
  uint32_t min_size = (input_bits_in_byte < bigint_size) ? input_bits_in_byte : bigint_size;

  if (input_bigint_sign && (input_byte_size > bigint_size))
  {
    min_size = (input_bits_in_byte > bigint_size) ? input_bits_in_byte : bigint_size;
  }

  if (min_size < sizeof (uint32_t))
  {
    min_size = sizeof (uint32_t);
  }

  ecma_extended_primitive_t *result_p = ecma_bigint_create ((uint32_t) min_size);

  if (JERRY_UNLIKELY (result_p == NULL))
  {
    ecma_deref_bigint (input_bigint_p);
    return ECMA_VALUE_ERROR;
  }

  ecma_bigint_digit_t *last_digit_p = ECMA_BIGINT_GET_DIGITS (input_bigint_p, bigint_size);

  /* Calculate the leading zeros of the input_bigint */

  ecma_bigint_digit_t zeros = ecma_big_uint_count_leading_zero (last_digit_p[-1]);
  uint32_t bits_of_bigint = (uint32_t) (bigint_size * JERRY_BITSINBYTE) - zeros;
  uint32_t exact_size =
    (input_bigint_sign || ((bits_of_bigint > (size_of_divisor_in_bits - 1)) && (input_byte_size < bigint_size)))
      ? (uint32_t) input_byte_size
      : bigint_size;

  if (input_bigint_sign)
  {
    bits_of_bigint += 1;
  }

  if (bits_of_bigint > (input_bits - 1) || input_bigint_sign)
  {
    ecma_bigint_digit_t *digits_p = ECMA_BIGINT_GET_DIGITS (input_bigint_p, 0);
    ecma_bigint_digit_t *digits_end_p = ECMA_BIGINT_GET_DIGITS (input_bigint_p, exact_size);
    ecma_bigint_digit_t *result_number_p = ECMA_BIGINT_GET_DIGITS (result_p, 0);
    int32_t first_cell = 0;
    uint32_t surplus_bits =
      (whole_part > 0) ? (uint32_t) (whole_part * size_of_divisor_in_bits) : (uint32_t) size_of_divisor_in_bits;
    uint32_t mask_bit = (whole_part == 0) ? (uint32_t) input_bits : (uint32_t) input_bits - surplus_bits;

    if (mask_bit == 0)
    {
      mask_bit = size_of_divisor_in_bits - 1;
    }

    uint32_t check_sign_mask = (uint32_t) 1 << (mask_bit - 1);
    uint32_t mask = ((uint32_t) 1 << mask_bit) - 1;
    uint32_t last_cell = (exact_size >= sizeof (uint32_t)) ? (uint32_t) (min_size / sizeof (uint32_t)) - 1 : 0;
    bool is_positive = false;
    bool is_representation_positive = false;

    if (is_signed)
    {
      if (input_bigint_sign && ((~digits_p[last_cell] + 1) & check_sign_mask) == 0)
      {
        is_positive = true;
      }

      if ((digits_p[last_cell] & check_sign_mask) == 0)
      {
        is_representation_positive = true;
      }
    }

    do
    {
      *result_number_p++ =
        (is_representation_positive || (!is_signed && !input_bigint_sign)) ? *digits_p++ : ~(*digits_p++);
      first_cell--;
    } while (digits_p < digits_end_p);

    int16_t equal_bits = 0;

    if (remainder != 0)
    {
      equal_bits = -1;
    }

    int32_t last_cell_negative = (last_cell != 0) ? ((int32_t) last_cell * (-1)) : -1;
    bool is_zero_values = false;

    if (!is_signed)
    {
      if (input_bigint_sign)
      {
        is_zero_values = true;
      }
    }
    else
    {
      if (((digits_p[-1] & check_sign_mask) > 0) || (result_number_p[-1] & check_sign_mask) > 0)
      {
        is_zero_values = true;
      }
    }

    if (is_zero_values)
    {
      result_number_p[first_cell] += 1;

      if (result_number_p[first_cell] == 0)
      {
        do
        {
          result_number_p[++first_cell] += 1;
        } while (first_cell != equal_bits);

        first_cell = last_cell_negative;
      }
    }

    result_number_p[-1] &= mask;
    uint32_t surplus = (uint32_t) (min_size - exact_size) / sizeof (ecma_char_t);
    uint32_t new_size = result_p->u.bigint_sign_and_size;

    if ((min_size - exact_size) % (sizeof (ecma_char_t)) > 0 && surplus == 0)
    {
      surplus += (uint32_t) sizeof (ecma_char_t);
    }
    else
    {
      surplus = (uint32_t) (surplus * sizeof (ecma_char_t));
    }

    if (min_size / JERRY_BITSINBYTE < 1)
    {
      surplus = 0;
    }

    if (is_signed)
    {
      if (result_p->u.bigint_sign_and_size > exact_size && min_size > sizeof (uint32_t)
          && result_number_p[last_cell_negative] < 1)
      {
        new_size -= surplus;
      }

      new_size += 1;

      if (is_positive || ((digits_p[-1] & check_sign_mask) == 0 && !input_bigint_sign))
      {
        new_size -= 1;
      }
    }

    while (first_cell != 0 && result_number_p[first_cell] == 0)
    {
      first_cell++;
    }

    if (first_cell == 0)
    {
      ecma_deref_bigint (result_p);
      ecma_deref_bigint (input_bigint_p);

      return ECMA_BIGINT_ZERO;
    }

    last_cell_negative = first_cell + (int32_t) last_cell;
    int16_t zero_section_cnt = 0;

    while (last_cell_negative > first_cell)
    {
      if (result_number_p[last_cell_negative] == 0)
      {
        zero_section_cnt++;
      }

      last_cell_negative--;
    }

    uint32_t size_limit = sizeof (uint32_t);

    if (zero_section_cnt >= 1)
    {
      size_limit = new_size - (uint32_t) zero_section_cnt * size_limit;
      new_size = (size_limit < sizeof (uint32_t)) ? (uint32_t) (JERRY_BITSINBYTE - size_limit) : size_limit;
    }

    if (new_size < result_p->u.bigint_sign_and_size)
    {
      result_p->refs_and_type = ECMA_EXTENDED_PRIMITIVE_REF_ONE | ECMA_TYPE_BIGINT;
      uint32_t new_size_remainder = new_size % sizeof (uint32_t);
      ecma_extended_primitive_t *new_result_p = ecma_bigint_create (new_size - new_size_remainder);

      new_result_p->u.bigint_sign_and_size += new_size_remainder;
      memcpy (new_result_p + 1, result_p + 1, new_size - new_size_remainder);

      ecma_deref_bigint (result_p);
      ecma_deref_bigint (input_bigint_p);

      return ecma_make_extended_primitive_value (new_result_p, ECMA_TYPE_BIGINT);
    }

    result_p->u.bigint_sign_and_size = new_size;
    result_p->refs_and_type = ECMA_EXTENDED_PRIMITIVE_REF_ONE | ECMA_TYPE_BIGINT;

    ecma_deref_bigint (input_bigint_p);
    return ecma_make_extended_primitive_value (result_p, ECMA_TYPE_BIGINT);
  }

  memcpy (result_p + 1, input_bigint_p + 1, exact_size);
  result_p->refs_and_type = ECMA_EXTENDED_PRIMITIVE_REF_ONE | ECMA_TYPE_BIGINT;

  if (input_bigint_p->u.bigint_sign_and_size & ECMA_BIGINT_SIGN)
  {
    ecma_deref_bigint (input_bigint_p);
    return ecma_bigint_negate (result_p);
  }

  ecma_deref_bigint (input_bigint_p);
  return ecma_make_extended_primitive_value (result_p, ECMA_TYPE_BIGINT);
} /* ecma_builtin_bigint_object_as_int_n */

/**
 * Handle calling [[Call]] of built-in BigInt object
 *
 * See also:
 *          ECMA-262 v11, 20.2.1.1
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_bigint_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                   uint32_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_value_t value = (arguments_list_len == 0) ? ECMA_VALUE_UNDEFINED : arguments_list_p[0];
  return ecma_bigint_to_bigint (value, true);
} /* ecma_builtin_bigint_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in BigInt object
 *
 * See also:
 *          ECMA-262 v11, 20.2.1
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_bigint_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                        uint32_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_raise_type_error (ECMA_ERR_BIGINT_FUNCTION_NOT_CONSTRUCTOR);
} /* ecma_builtin_bigint_dispatch_construct */

/**
 * Dispatcher of the built-in's routines
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_bigint_dispatch_routine (uint8_t builtin_routine_id, /**< built-in wide routine identifier */
                                      ecma_value_t this_arg, /**< 'this' argument value */
                                      const ecma_value_t arguments_list_p[], /**< list of arguments
                                                                              *   passed to routine */
                                      uint32_t arguments_number) /**< length of arguments' list */
{
  JERRY_UNUSED_2 (this_arg, arguments_number);

  switch (builtin_routine_id)
  {
    case ECMA_BUILTIN_BIGINT_AS_INT_N:
    {
      return ecma_builtin_bigint_object_as_int_n (arguments_list_p[0], arguments_list_p[1], true);
    }
    case ECMA_BUILTIN_BIGINT_AS_U_INT_N:
    {
      return ecma_builtin_bigint_object_as_int_n (arguments_list_p[0], arguments_list_p[1], false);
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* ecma_builtin_bigint_dispatch_routine */

/**
 * @}
 * @}
 * @}
 */

#endif /* JERRY_BUILTIN_BIGINT */
