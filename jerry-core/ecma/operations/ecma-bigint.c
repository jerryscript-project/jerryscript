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
#include "ecma-exceptions.h"
#include "ecma-helpers.h"
#include "lit-char-helpers.h"

#if ENABLED (JERRY_BUILTIN_BIGINT)

/**
 * Raise a not enough memory error
 *
 * @return ECMA_VALUE_ERROR
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_bigint_raise_memory_error (void)
{
  return ecma_raise_range_error (ECMA_ERR_MSG ("Cannot allocate memory for a BigInt value"));
} /* ecma_bigint_raise_memory_error */

/**
 * Parse a string and create a BigInt value
 *
 * @return ecma BigInt value or ECMA_VALUE_ERROR
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_bigint_parse_string (const lit_utf8_byte_t *string_p, /**< string represenation of the BigInt */
                          lit_utf8_size_t size) /**< string size */
{
  ecma_bigint_digit_t radix = 10;
  uint32_t sign = 0;

  if (size >= 3 && string_p[0] == LIT_CHAR_0)
  {
    if (string_p[1] == LIT_CHAR_LOWERCASE_X || string_p[1] == LIT_CHAR_UPPERCASE_X)
    {
      radix = 16;
      string_p += 2;
      size -= 2;
    }
    else if (string_p[1] == LIT_CHAR_LOWERCASE_B || string_p[1] == LIT_CHAR_UPPERCASE_B)
    {
      radix = 2;
      string_p += 2;
      size -= 2;
    }
  }
  else if (size >= 2)
  {
    if (string_p[0] == LIT_CHAR_PLUS)
    {
      size--;
      string_p++;
    }
    else if (string_p[0] == LIT_CHAR_MINUS)
    {
      sign = ECMA_BIGINT_SIGN;
      size--;
      string_p++;
    }
  }
  else if (size == 0)
  {
    return ecma_raise_syntax_error (ECMA_ERR_MSG ("BigInt cannot be constructed from empty string"));
  }

  const lit_utf8_byte_t *string_end_p = string_p + size;

  while (string_p < string_end_p && *string_p == LIT_CHAR_0)
  {
    string_p++;
  }

  ecma_extended_primitive_t *result_p = NULL;

  if (string_p == string_end_p)
  {
    result_p = ecma_bigint_create (0);
  }
  else
  {
    do
    {
      ecma_bigint_digit_t digit = radix;

      if (*string_p >= LIT_CHAR_0 && *string_p <= LIT_CHAR_9)
      {
        digit = (ecma_bigint_digit_t) (*string_p - LIT_CHAR_0);
      }
      else
      {
        lit_utf8_byte_t character = (lit_utf8_byte_t) LEXER_TO_ASCII_LOWERCASE (*string_p);

        if (character >= LIT_CHAR_LOWERCASE_A && character <= LIT_CHAR_LOWERCASE_F)
        {
          digit = (ecma_bigint_digit_t) (character - (LIT_CHAR_LOWERCASE_A - 10));
        }
      }

      if (digit >= radix)
      {
        if (result_p != NULL)
        {
          ecma_deref_bigint (result_p);
        }
        return ecma_raise_syntax_error (ECMA_ERR_MSG ("String cannot be converted to BigInt value"));
      }

      result_p = ecma_big_uint_mul_digit (result_p, radix, digit);

      if (JERRY_UNLIKELY (result_p == NULL))
      {
        break;
      }
    }
    while (++string_p < string_end_p);
  }

  if (JERRY_UNLIKELY (result_p == NULL))
  {
    return ecma_bigint_raise_memory_error ();
  }

  result_p->u.bigint_sign_and_size |= sign;
  return ecma_make_extended_primitive_value (result_p, ECMA_TYPE_BIGINT);
} /* ecma_bigint_parse_string */

/**
 * Create a string representation for a BigInt value
 *
 * @return ecma string or ECMA_VALUE_ERROR
 *         Returned value must be freed with ecma_free_value.
 */
ecma_string_t *
ecma_bigint_to_string (ecma_value_t value, /**< BigInt value */
                       ecma_bigint_digit_t radix) /**< conversion radix */
{
  JERRY_ASSERT (ecma_is_value_bigint (value));

  ecma_extended_primitive_t *bigint_p = ecma_get_extended_primitive_from_value (value);

  if (ECMA_BIGINT_GET_SIZE (bigint_p) == 0)
  {
    return ecma_new_ecma_string_from_code_unit (LIT_CHAR_0);
  }

  uint32_t char_start_p, char_size_p;
  lit_utf8_byte_t *string_buffer_p = ecma_big_uint_to_string (bigint_p, radix, &char_start_p, &char_size_p);

  if (JERRY_UNLIKELY (string_buffer_p == NULL))
  {
    ecma_raise_range_error (ECMA_ERR_MSG ("Cannot allocate memory for a string representation of a BigInt value"));
    return NULL;
  }

  JERRY_ASSERT (char_start_p > 0);

  if (bigint_p->u.bigint_sign_and_size & ECMA_BIGINT_SIGN)
  {
    string_buffer_p[--char_start_p] = LIT_CHAR_MINUS;
  }

  ecma_string_t *string_p;
  string_p = ecma_new_ecma_string_from_utf8 (string_buffer_p + char_start_p, char_size_p - char_start_p);

  jmem_heap_free_block (string_buffer_p, char_size_p);
  return string_p;
} /* ecma_bigint_to_string */

/**
 * Negate a non-zero BigInt value
 *
 * @return ecma BigInt value or ECMA_VALUE_ERROR
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_bigint_negate (ecma_extended_primitive_t *value_p) /**< BigInt value */
{
  uint32_t size = ECMA_BIGINT_GET_SIZE (value_p);

  JERRY_ASSERT (size > 0 && ECMA_BIGINT_GET_LAST_DIGIT (value_p, size) != 0);

  ecma_extended_primitive_t *result_p = ecma_bigint_create (size);

  if (JERRY_UNLIKELY (result_p == NULL))
  {
    return ecma_bigint_raise_memory_error ();
  }

  memcpy (result_p + 1, value_p + 1, size);
  result_p->refs_and_type = ECMA_EXTENDED_PRIMITIVE_REF_ONE | ECMA_TYPE_BIGINT;
  result_p->u.bigint_sign_and_size = value_p->u.bigint_sign_and_size ^ ECMA_BIGINT_SIGN;

  return ecma_make_extended_primitive_value (result_p, ECMA_TYPE_BIGINT);
} /* ecma_bigint_negate */

/**
 * Add/subtract right BigInt value to/from left BigInt value
 *
 * @return ecma BigInt value or ECMA_VALUE_ERROR
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_bigint_add_sub (ecma_value_t left_value, /**< left BigInt value */
                     ecma_value_t right_value, /**< right BigInt value */
                     bool is_add) /**< true if add operation should be performed */
{
  JERRY_ASSERT (ecma_is_value_bigint (left_value) && ecma_is_value_bigint (right_value));

  ecma_extended_primitive_t *left_p = ecma_get_extended_primitive_from_value (left_value);
  ecma_extended_primitive_t *right_p = ecma_get_extended_primitive_from_value (right_value);
  uint32_t left_size = ECMA_BIGINT_GET_SIZE (left_p);
  uint32_t right_size = ECMA_BIGINT_GET_SIZE (right_p);

  if (right_size == 0)
  {
    ecma_ref_extended_primitive (left_p);
    return left_value;
  }

  if (left_size == 0)
  {
    if (!is_add)
    {
      return ecma_bigint_negate (right_p);
    }

    ecma_ref_extended_primitive (right_p);
    return right_value;
  }

  uint32_t sign = is_add ? 0 : ECMA_BIGINT_SIGN;

  if (((left_p->u.bigint_sign_and_size ^ right_p->u.bigint_sign_and_size) & ECMA_BIGINT_SIGN) == sign)
  {
    ecma_extended_primitive_t *result_p = ecma_big_uint_add (left_p, right_p);

    if (JERRY_UNLIKELY (result_p == NULL))
    {
      return ecma_bigint_raise_memory_error ();
    }

    result_p->u.bigint_sign_and_size |= left_p->u.bigint_sign_and_size & ECMA_BIGINT_SIGN;
    return ecma_make_extended_primitive_value (result_p, ECMA_TYPE_BIGINT);
  }

  int compare_result = ecma_big_uint_compare (left_p, right_p);
  ecma_extended_primitive_t *result_p;

  if (compare_result == 0)
  {
    sign = 0;
    result_p = ecma_bigint_create (0);
  }
  else if (compare_result > 0)
  {
    sign = left_p->u.bigint_sign_and_size & ECMA_BIGINT_SIGN;
    result_p = ecma_big_uint_sub (left_p, right_p);
  }
  else
  {
    sign = right_p->u.bigint_sign_and_size & ECMA_BIGINT_SIGN;

    if (!is_add)
    {
      sign ^= ECMA_BIGINT_SIGN;
    }

    result_p = ecma_big_uint_sub (right_p, left_p);
  }

  if (JERRY_UNLIKELY (result_p == NULL))
  {
    return ecma_bigint_raise_memory_error ();
  }

  result_p->u.bigint_sign_and_size |= sign;
  return ecma_make_extended_primitive_value (result_p, ECMA_TYPE_BIGINT);
} /* ecma_bigint_add_sub */

/**
 * Multiply two BigInt values
 *
 * @return ecma BigInt value or ECMA_VALUE_ERROR
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_bigint_mul (ecma_value_t left_value, /**< left BigInt value */
                 ecma_value_t right_value) /**< right BigInt value */
{
  JERRY_ASSERT (ecma_is_value_bigint (left_value) && ecma_is_value_bigint (right_value));

  ecma_extended_primitive_t *left_p = ecma_get_extended_primitive_from_value (left_value);
  ecma_extended_primitive_t *right_p = ecma_get_extended_primitive_from_value (right_value);
  uint32_t left_size = ECMA_BIGINT_GET_SIZE (left_p);
  uint32_t right_size = ECMA_BIGINT_GET_SIZE (right_p);

  if (left_size == 0)
  {
    ecma_ref_extended_primitive (left_p);
    return left_value;
  }

  if (right_size == 0)
  {
    ecma_ref_extended_primitive (right_p);
    return right_value;
  }

  if (left_size == sizeof (ecma_bigint_digit_t)
      && ECMA_BIGINT_GET_LAST_DIGIT (left_p, sizeof (ecma_bigint_digit_t)) == 1)
  {
    if (left_p->u.bigint_sign_and_size & ECMA_BIGINT_SIGN)
    {
      return ecma_bigint_negate (right_p);
    }

    ecma_ref_extended_primitive (right_p);
    return right_value;
  }

  if (right_size == sizeof (ecma_bigint_digit_t)
      && ECMA_BIGINT_GET_LAST_DIGIT (right_p, sizeof (ecma_bigint_digit_t)) == 1)
  {
    if (right_p->u.bigint_sign_and_size & ECMA_BIGINT_SIGN)
    {
      return ecma_bigint_negate (left_p);
    }

    ecma_ref_extended_primitive (left_p);
    return left_value;
  }

  ecma_extended_primitive_t *result_p = ecma_big_uint_mul (left_p, right_p);

  if (JERRY_UNLIKELY (result_p == NULL))
  {
    return ecma_bigint_raise_memory_error ();
  }

  uint32_t sign = (left_p->u.bigint_sign_and_size ^ right_p->u.bigint_sign_and_size) & ECMA_BIGINT_SIGN;
  result_p->u.bigint_sign_and_size |= sign;
  return ecma_make_extended_primitive_value (result_p, ECMA_TYPE_BIGINT);
} /* ecma_bigint_mul */

/**
 * Divide two BigInt values
 *
 * @return ecma BigInt value or ECMA_VALUE_ERROR
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_bigint_div_mod (ecma_value_t left_value, /**< left BigInt value */
                     ecma_value_t right_value, /**< right BigInt value */
                     bool is_mod) /**< true if return with remainder */
{
  JERRY_ASSERT (ecma_is_value_bigint (left_value) && ecma_is_value_bigint (right_value));

  ecma_extended_primitive_t *left_p = ecma_get_extended_primitive_from_value (left_value);
  ecma_extended_primitive_t *right_p = ecma_get_extended_primitive_from_value (right_value);
  uint32_t left_size = ECMA_BIGINT_GET_SIZE (left_p);
  uint32_t right_size = ECMA_BIGINT_GET_SIZE (right_p);

  if (right_size == 0)
  {
    return ecma_raise_range_error (ECMA_ERR_MSG ("BigInt division by zero"));
  }

  if (left_size == 0)
  {
    ecma_ref_extended_primitive (left_p);
    return left_value;
  }

  int compare_result = ecma_big_uint_compare (left_p, right_p);
  ecma_extended_primitive_t *result_p;

  if (compare_result < 0)
  {
    if (is_mod)
    {
      ecma_ref_extended_primitive (left_p);
      return left_value;
    }
    else
    {
      result_p = ecma_bigint_create (0);
    }
  }
  else if (compare_result == 0)
  {
    if (is_mod)
    {
      result_p = ecma_bigint_create (0);
    }
    else
    {
      result_p = ecma_bigint_create (sizeof (ecma_bigint_digit_t));

      if (result_p != NULL)
      {
        *ECMA_BIGINT_GET_DIGITS (result_p, 0) = 1;
      }
    }
  }
  else
  {
    result_p = ecma_big_uint_div_mod (left_p, right_p, is_mod);
  }

  if (JERRY_UNLIKELY (result_p == NULL))
  {
    return ecma_bigint_raise_memory_error ();
  }

  if (ECMA_BIGINT_GET_SIZE (result_p) == 0)
  {
    return ecma_make_extended_primitive_value (result_p, ECMA_TYPE_BIGINT);
  }

  if (is_mod)
  {
    result_p->u.bigint_sign_and_size |= left_p->u.bigint_sign_and_size & ECMA_BIGINT_SIGN;
  }
  else
  {
    uint32_t sign = (left_p->u.bigint_sign_and_size ^ right_p->u.bigint_sign_and_size) & ECMA_BIGINT_SIGN;
    result_p->u.bigint_sign_and_size |= sign;
  }

  return ecma_make_extended_primitive_value (result_p, ECMA_TYPE_BIGINT);
} /* ecma_bigint_div_mod */

/**
 * Shift left BigInt value to left or right
 *
 * @return ecma BigInt value or ECMA_VALUE_ERROR
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_bigint_shift (ecma_value_t left_value, /**< left BigInt value */
                   ecma_value_t right_value, /**< right BigInt value */
                   bool is_left) /**< true if left shift operation should be performed */
{
  JERRY_ASSERT (ecma_is_value_bigint (left_value) && ecma_is_value_bigint (right_value));

  ecma_extended_primitive_t *left_p = ecma_get_extended_primitive_from_value (left_value);
  ecma_extended_primitive_t *right_p = ecma_get_extended_primitive_from_value (right_value);
  uint32_t right_size = ECMA_BIGINT_GET_SIZE (right_p);

  if (right_size == 0 ||  ECMA_BIGINT_GET_SIZE (left_p) == 0)
  {
    ecma_ref_extended_primitive (left_p);
    return left_value;
  }

  if (right_p->u.bigint_sign_and_size & ECMA_BIGINT_SIGN)
  {
    is_left = !is_left;
  }

  if (right_size > sizeof (ecma_bigint_digit_t))
  {
    if (is_left)
    {
      return ecma_bigint_raise_memory_error ();
    }

    return ecma_make_extended_primitive_value (ecma_bigint_create (0), ECMA_TYPE_BIGINT);
  }

  ecma_extended_primitive_t *result_p;
  ecma_bigint_digit_t shift = ECMA_BIGINT_GET_LAST_DIGIT (right_p, sizeof (ecma_bigint_digit_t));

  if (is_left)
  {
    result_p = ecma_big_uint_shift_left (left_p, shift);
  }
  else
  {
    result_p = ecma_big_uint_shift_right (left_p, shift);
  }

  if (JERRY_UNLIKELY (result_p == NULL))
  {
    return ecma_bigint_raise_memory_error ();
  }

  result_p->u.bigint_sign_and_size |= left_p->u.bigint_sign_and_size & ECMA_BIGINT_SIGN;
  return ecma_make_extended_primitive_value (result_p, ECMA_TYPE_BIGINT);
} /* ecma_bigint_shift */
#endif /* ENABLED (JERRY_BUILTIN_BIGINT) */
