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

#include "ecma-bigint.h"
#include "ecma-big-uint.h"
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
  ECMA_BUILTIN_BIGINT_START = 0,
  ECMA_BUILTIN_BIGINT_AS_INT_N,
  ECMA_BUILTIN_BIGINT_AS_U_INT_N,
};

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-bigint.inc.h"
#define BUILTIN_UNDERSCORED_ID bigint
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
 *          ECMA-262 v5, 11.0
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_bigint_object_as_int_n (ecma_value_t this_arg, /**< 'this' argument */
                                     ecma_value_t bits, /**< number of bits */
                                     ecma_value_t bigint, /**< bigint number */
                                     bool is_signed) /**< The operation is signed */
{
  JERRY_UNUSED (this_arg);

  ecma_number_t bit_length_to_index;
  ecma_value_t bit_value = ecma_op_to_index (bits, &bit_length_to_index);

  if (ECMA_IS_VALUE_ERROR (bit_value))
  {
    return bits;
  }

  if (bit_length_to_index == 0)
  {
    return ECMA_BIGINT_ZERO;
  }

  ecma_number_t number;
  ecma_value_t bigint_value = ecma_op_to_numeric (bigint, &number, ECMA_TO_NUMERIC_ALLOW_BIGINT);

  if (bigint_value == ECMA_BIGINT_ZERO)
  {
    return ECMA_BIGINT_ZERO;
  }

  uint32_t whole_part = (ecma_value_t) bit_length_to_index / (sizeof (uint32_t) * sizeof (uint64_t));
  uint32_t remainder = (ecma_value_t) bit_length_to_index % (sizeof (uint32_t) * sizeof (uint64_t));

  uint32_t input_bit_length = 1;

  if (whole_part == 0)
  {
    input_bit_length = (remainder > 0) ? (ecma_value_t) bit_length_to_index : (sizeof (uint32_t) * sizeof (uint64_t));
  }
  else
  {
    input_bit_length = (remainder > 0) ? (uint32_t) ((whole_part + 1) * ((sizeof (uint32_t) * sizeof (uint64_t))))
                                       : (uint32_t) (whole_part * (sizeof (uint32_t) * sizeof (uint64_t)));
  }

  uint32_t input_bit_byte_size = (ecma_value_t) bit_length_to_index / sizeof (uint64_t);

  if ((ecma_value_t) bit_length_to_index % sizeof (uint64_t) != 0)
  {
    input_bit_byte_size += 1;
  }

  ecma_extended_primitive_t * input_bigint_p = ecma_get_extended_primitive_from_value (bigint_value);

  if (JERRY_UNLIKELY (input_bigint_p == NULL))
  {
    return 0;
  }

  uint32_t bigint_byte_size = ECMA_BIGINT_GET_SIZE (input_bigint_p);
  uint8_t input_bigint_sign = input_bigint_p->u.bigint_sign_and_size & ECMA_BIGINT_SIGN;

  uint32_t min_size = ((input_bit_length / sizeof (uint64_t)) < bigint_byte_size)
                        ? (input_bit_length / sizeof (uint64_t))
                        : bigint_byte_size;

  if (input_bigint_sign && (input_bit_byte_size > bigint_byte_size))
  {
    min_size = ((input_bit_length / sizeof (uint64_t)) > bigint_byte_size) ? (input_bit_length / sizeof (uint64_t))
                                                                           : bigint_byte_size;
  }

  if (min_size < sizeof (uint32_t))
  {
    min_size = sizeof (uint32_t);
  }

  ecma_extended_primitive_t * result_p = ecma_bigint_create (min_size);

  if (JERRY_UNLIKELY (result_p == NULL))
  {
    ecma_deref_bigint (input_bigint_p);
    return 0;
  }

  ecma_bigint_digit_t *first_digit_p = ECMA_BIGINT_GET_DIGITS (input_bigint_p, 0);
  ecma_bigint_digit_t *last_digit_p = ECMA_BIGINT_GET_DIGITS (input_bigint_p, bigint_byte_size);

  /* Calc the leading zeros of the input_bigint */
  ecma_bigint_digit_t zeros;

  do
  {
    zeros = ecma_big_uint_count_leading_zero (*first_digit_p);
    ++first_digit_p;
  }
  while (first_digit_p < last_digit_p);

  uint32_t bits_of_bigint = (uint32_t) (bigint_byte_size * sizeof (uint64_t)) - zeros;

  uint32_t exact_size = 1;

  if ((bits_of_bigint > (sizeof (uint32_t) * sizeof (uint64_t)) - 1) || input_bigint_sign)
  {
    exact_size = (input_bit_byte_size < bigint_byte_size) ? input_bit_byte_size : bigint_byte_size;

    if ((input_bigint_sign) && (input_bit_byte_size > bigint_byte_size))
    {
      exact_size = (input_bit_byte_size > bigint_byte_size) ? input_bit_byte_size : bigint_byte_size;
    }
  }
  else
  {
    exact_size = bigint_byte_size;
  }

  if (input_bigint_sign)
  {
    bits_of_bigint += 1;
  }

  if (bits_of_bigint > (bit_length_to_index - 1) || (input_bigint_sign))
  {
    ecma_bigint_digit_t *digits_p = ECMA_BIGINT_GET_DIGITS (input_bigint_p, 0);

    ecma_bigint_digit_t *digits_end_p = ECMA_BIGINT_GET_DIGITS (input_bigint_p, exact_size);

    ecma_bigint_digit_t *result_number_p = ECMA_BIGINT_GET_DIGITS (result_p, 0);

    int32_t first_cell = 0;

    uint32_t surplus_bits = (whole_part > 0) ? (uint32_t) (whole_part * sizeof (uint32_t) * sizeof (uint64_t))
                                             : (uint32_t) (sizeof (uint32_t) * sizeof (uint64_t));

    uint32_t mask_bit = (whole_part == 0) ? (uint32_t) bit_length_to_index
                                          : (uint32_t) bit_length_to_index - surplus_bits;

    if (mask_bit == 0)
    {
      mask_bit = (sizeof (uint32_t) * sizeof (uint64_t)) - 1;
    }

    uint32_t check_sign_mask = (uint32_t) 1 << (mask_bit - 1);

    uint32_t mask = (uint32_t) 1 << mask_bit;

    if (mask == 0)
    {
      mask = ~((ecma_bigint_digit_t) 0);
    }
    else if (is_signed && mask > 1)
    {
      mask -= 1;
    }
    else
    {
      mask -= 1;
    }

    uint32_t last_cell = (exact_size >= sizeof (uint32_t)) ? (uint32_t) (min_size / sizeof (uint32_t)) - 1 : 0;

    uint8_t positive_num = 0;
    uint32_t num_representation = 0;

    if (is_signed)
    {
      if ((input_bigint_sign) && ((~digits_p[last_cell] + 1) & check_sign_mask) == 0)
      {
        positive_num = 1;
      }

      if (((digits_p[last_cell]) & check_sign_mask) == 0)
      {
        num_representation = 1;
      }

      do
      {
        *result_number_p++ = (num_representation) ? *digits_p++ : ~(*digits_p++);
        first_cell--;
      }
      while (digits_p < digits_end_p);
    }
    else
    {
      do
      {
        *result_number_p++ = (input_bigint_sign) ? ~(*digits_p++) : *digits_p++;
        first_cell--;
      }
      while (digits_p < digits_end_p);
    }

    int16_t equal_bit_s = 0;

    if (remainder != 0)
    {
      equal_bit_s = -1;
    }

    int32_t last_cell_negative = (last_cell != 0) ? ((int32_t) last_cell * (-1)) : -1;

    uint8_t helper_value = 0;

    if (!is_signed)
    {
      if (input_bigint_sign)
      {
        helper_value = 1;
      }
    }
    else
    {
      if ((((digits_p[-1]) & check_sign_mask) > 0) || (result_number_p[-1] & check_sign_mask) > 0)
      {
        helper_value = 1;
      }
    }
    if (helper_value == 1)
    {
      result_number_p[first_cell] += 1;

      if (result_number_p[first_cell] == 0)
      {
        do
        {
          result_number_p[++first_cell] += 1;
        }
        while (first_cell != equal_bit_s);

        first_cell = last_cell_negative;
      }
    }

    result_number_p[-1] &= mask;

    uint32_t surplus = (min_size - exact_size) / (sizeof (uint32_t) / sizeof (ecma_char_t));

    uint32_t new_size = result_p->u.bigint_sign_and_size;

    if ((min_size - exact_size) % (sizeof (uint32_t) / sizeof (ecma_char_t)) > 0 && surplus == 0)
    {
      surplus += (uint32_t) (sizeof (uint32_t) / sizeof (ecma_char_t));
    }
    else
    {
      surplus = (uint32_t) (surplus * (sizeof (uint32_t) / sizeof (ecma_char_t)));
    }

    if (min_size / sizeof (uint64_t) < 1)
    {
      surplus = 0;
    }

    if (is_signed)
    {
      if ((result_p->u.bigint_sign_and_size) > exact_size && min_size > sizeof (uint32_t)
           && result_number_p[last_cell_negative] < 1)
      {
        new_size -= surplus;
      }

      new_size += 1;

      if (positive_num || (((digits_p[-1]) & check_sign_mask) == 0 && !(input_bigint_sign)))
      {
        new_size -= 1;
      }
    }

    while (first_cell != 0)
    {
      if (result_number_p[first_cell] != 0)
      {
        break;
      }

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

    uint32_t size_check_number = sizeof (uint32_t);

    if (zero_section_cnt >= 1)
    {
      size_check_number = new_size - ((uint32_t) zero_section_cnt * size_check_number);
      new_size = (size_check_number < sizeof (uint32_t)) ? (uint32_t) (sizeof (uint64_t) - size_check_number) 
                                                         : size_check_number;
    }

    if (new_size < result_p->u.bigint_sign_and_size)
    {
      result_p->refs_and_type = ECMA_EXTENDED_PRIMITIVE_REF_ONE | ECMA_TYPE_BIGINT;

      uint32_t new_size_remainder = new_size % sizeof (uint32_t);

      ecma_extended_primitive_t * new_result_p = ecma_bigint_create (new_size - new_size_remainder);

      new_result_p->u.bigint_sign_and_size += new_size_remainder;
      memcpy (new_result_p +1, result_p +1, new_size - new_size_remainder);

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

  return ecma_raise_type_error (ECMA_ERR_MSG ("BigInt function is not a constructor"));
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
  JERRY_UNUSED (this_arg);
  JERRY_UNUSED (arguments_number);

  switch (builtin_routine_id)
  {
    case ECMA_BUILTIN_BIGINT_AS_INT_N:
    case ECMA_BUILTIN_BIGINT_AS_U_INT_N:
    {
      bool is_signed = (builtin_routine_id == ECMA_BUILTIN_BIGINT_AS_INT_N);
      return ecma_builtin_bigint_object_as_int_n (this_arg, arguments_list_p[0], arguments_list_p[1], is_signed);
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
