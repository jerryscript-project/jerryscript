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

#include <math.h>

#include "ecma-alloc.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-string-object.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"
#include "jrt-libc-includes.h"

#ifndef CONFIG_DISABLE_NUMBER_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-number-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID number_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup numberprototype ECMA Number.prototype object built-in
 * @{
 */

/**
 * Helper for stringifying numbers
 *
 * @return the length of the generated string representation
 */
static lit_utf8_size_t
ecma_builtin_number_prototype_helper_to_string (lit_utf8_byte_t *digits_p, /**< number as string in decimal form */
                                                lit_utf8_size_t num_digits, /**< length of the string representation */
                                                int32_t exponent, /**< decimal exponent */
                                                lit_utf8_byte_t *to_digits_p, /**< [out] buffer to write */
                                                lit_utf8_size_t to_num_digits) /**< requested number of digits */
{
  lit_utf8_byte_t *p = to_digits_p;

  if (exponent <= 0)
  {
    /* Add zero to the integer part. */
    *p++ = '0';
    to_num_digits--;

    if (to_num_digits > 0)
    {
      *p++ = '.';

      /* Add leading zeros to the fraction part. */
      for (int i = 0; i < -exponent && to_num_digits > 0; i++)
      {
        *p++ = '0';
        to_num_digits--;
      }
    }
  }
  else
  {
    /* Add significant digits of the integer part. */
    lit_utf8_size_t to_copy = JERRY_MIN (num_digits, to_num_digits);
    to_copy = JERRY_MIN (to_copy, (lit_utf8_size_t) exponent);
    memmove (p, digits_p, (size_t) to_copy);
    p += to_copy;
    to_num_digits -= to_copy;
    digits_p += to_copy;
    num_digits -= to_copy;
    exponent -= (int32_t) to_copy;

    /* Add zeros before decimal point. */
    while (exponent > 0 && to_num_digits > 0)
    {
      JERRY_ASSERT (num_digits == 0);
      *p++ = '0';
      to_num_digits--;
      exponent--;
    }

    if (to_num_digits > 0)
    {
      *p++ = '.';
    }
  }

  if (to_num_digits > 0)
  {
    /* Add significant digits of the fraction part. */
    lit_utf8_size_t to_copy = JERRY_MIN (num_digits, to_num_digits);
    memmove (p, digits_p, (size_t) to_copy);
    p += to_copy;
    to_num_digits -= to_copy;

    /* Add trailing zeros. */
    while (to_num_digits > 0)
    {
      *p++ = '0';
      to_num_digits--;
    }
  }

  return (lit_utf8_size_t) (p - to_digits_p);
} /* ecma_builtin_number_prototype_helper_to_string */

static inline lit_utf8_size_t __attr_always_inline___
ecma_builtin_binary_floating_number_to_string (lit_utf8_byte_t *digits_p, /**< number as string
                                                                           * in binary-floating point number */
                                               int32_t exponent, /**< decimal exponent */
                                               lit_utf8_byte_t *to_digits_p, /**< [out] buffer to write */
                                               lit_utf8_size_t to_num_digits) /**< requested number of digits */
{
  lit_utf8_byte_t *p = to_digits_p;
  /* Add significant digits of the decimal part. */
  while (exponent > 0)
  {
    *p++ = *digits_p++;
    exponent--;
    to_num_digits--;
  }

  if (to_num_digits > 0)
  {
    *p++ = '.';
  }

  if (to_num_digits > 0)
  {
    /* Add significant digits of the fraction part and fill the remaining digits with zero */
    while (to_num_digits > 0)
    {
      *p++ = (*digits_p == 0 ? '0' : *digits_p++);
      to_num_digits--;
    }
  }

  return (lit_utf8_size_t) (p - to_digits_p);
} /* ecma_builtin_binary_floating_number_to_string */

/**
 * Helper for rounding numbers
 *
 * @return rounded number
 */
static inline lit_utf8_size_t __attr_always_inline___
ecma_builtin_number_prototype_helper_round (lit_utf8_byte_t *digits_p, /**< [in,out] number as a string in decimal
                                                                        *   form */
                                            lit_utf8_size_t num_digits, /**< length of the string representation */
                                            int32_t round_num, /**< number of digits to keep */
                                            int32_t *exponent_p, /**< [in, out] decimal exponent */
                                            bool zero) /**< true if digits_p represents zero */
{
  if (round_num < 1)
  {
    return 0;
  }

  if ((lit_utf8_size_t) round_num >= num_digits || zero)
  {
    return num_digits;
  }

  if (digits_p[round_num] >= '5')
  {
    digits_p[round_num] = '0';

    int i = 1;

    /* Handle carry number. */
    for (; i <= round_num; i++)
    {
      if (++digits_p[round_num - i] <= '9')
      {
        break;
      }
      digits_p[round_num - i] = '0';
    }

    /* Prepend highest digit */
    if (i > round_num)
    {
      memmove (digits_p + 1, digits_p, num_digits);
      digits_p[0] = '1';
      *exponent_p += 1;
    }
  }

  return (lit_utf8_size_t) round_num;
} /* ecma_builtin_number_prototype_helper_round */

/**
 * The Number.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.7.4.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_number_prototype_object_to_string (ecma_value_t this_arg, /**< this argument */
                                                const ecma_value_t *arguments_list_p, /**< arguments list */
                                                ecma_length_t arguments_list_len) /**< number of arguments */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  ECMA_TRY_CATCH (this_value, ecma_builtin_number_prototype_object_value_of (this_arg), ret_value);
  ecma_number_t this_arg_number = ecma_get_number_from_value (this_value);

  if (arguments_list_len == 0
      || ecma_number_is_nan (this_arg_number)
      || ecma_number_is_infinity (this_arg_number)
      || ecma_number_is_zero (this_arg_number)
      || (arguments_list_len > 0 && ecma_is_value_undefined (arguments_list_p[0])))
  {
    ecma_string_t *ret_str_p = ecma_new_ecma_string_from_number (this_arg_number);

    ret_value = ecma_make_string_value (ret_str_p);
  }
  else
  {
    static const lit_utf8_byte_t digit_chars[36] =
    {
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
      'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
      'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
      'u', 'v', 'w', 'x', 'y', 'z'
    };

    ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arguments_list_p[0], ret_value);

    uint32_t radix = ecma_number_to_uint32 (arg_num);

    if (radix < 2 || radix > 36)
    {
      ret_value = ecma_raise_range_error (ECMA_ERR_MSG ("Radix must be between 2 and 36."));
    }
    else if (radix == 10)
    {
      ecma_string_t *ret_str_p = ecma_new_ecma_string_from_number (this_arg_number);

      ret_value = ecma_make_string_value (ret_str_p);
    }
    else
    {
      int buff_size = 0;

      bool is_number_negative = false;
      if (ecma_number_is_negative (this_arg_number))
      {
        /* ecma_number_to_decimal can't handle negative numbers, so we get rid of the sign. */
        this_arg_number = -this_arg_number;
        is_number_negative = true;

        /* Add space for the sign in the result. */
        buff_size += 1;
      }

      /* Decompose the number. */
      lit_utf8_byte_t digits[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER];
      int32_t exponent;
      lit_utf8_size_t digit_count = ecma_number_to_decimal (this_arg_number, digits, &exponent);

      /*
       * The 'exponent' given by 'ecma_number_to_decimal' specifies where the decimal point is located
       * compared to the first digit in 'digits'.
       * For example: 120 -> '12', exp: 3 and 0.012 -> '12', exp: -1
       * We convert it to be location of the decimal point compared to the last digit of 'digits':
       * 120 -> 12 * 10^1 and 0.012 -> 12 * 10^-3
       */
      exponent = exponent - (int32_t) digit_count;

      /* 'magnitude' will be the magnitude of the number in the specific radix. */
      int magnitude;
      int required_digits;
      if (exponent >= 0)
      {
        /*
         * If the exponent is non-negative that means we won't have a fractional part, and can calculate
         * exactly how many digits we will have. This could be done via a mathematic formula, but in rare
         * cases that can cause incorrect results due to precision issues, so we use a loop instead.
         */
        magnitude = 0;
        double counter = this_arg_number;
        while (counter >= radix)
        {
          counter /= radix;
          magnitude++;
        }

        /*
         * The magnitude will only tell us how many digits we have after the first one, so we add one extra.
         * In this case we won't be needing a radix point, so we don't need to worry about space for it.
         */
        required_digits = magnitude + 1;
      }
      else
      {
        /*
         * We can't know exactly how many digits we will need, since the number may be non-terminating in the
         * new radix, so we will have to estimate it. We do this by first calculating how many zeros we will
         * need in the specific radix before we hit a significant digit. This is calculated from the decimal
         * exponent, which we negate so that we get a positive number in the end.
         */
        magnitude = (int) floor ((log (10) / log (radix)) * -exponent);

        /*
         * We also need to add space for significant digits. The worst case is radix == 2, since this will
         * require the most digits. In this case, the upper limit to the number of significant digits we can have is
         * ECMA_NUMBER_FRACTION_WIDTH + 1. This should be sufficient for any number.
         */
        required_digits = magnitude + ECMA_NUMBER_FRACTION_WIDTH + 1;

        /*
         * We add an exta slot for the radix point. It is also likely that we will need extra space for a
         * leading zero before the radix point. It's better to add space for that here as well, even if we may not
         * need it, since later we won't be able to do so.
         */
        buff_size += 2;
      }

      /*
       * Here we normalize the number so that it is as close to 0 as possible, which will prevent us from losing
       * precision in case of extreme numbers when we later split the number into integer and fractional parts.
       * This has to be done in the specific radix, otherwise it messes up the result, so we use magnitude instead.
       */
      if (exponent > 0)
      {
        for (int i = 0; i < magnitude; i++)
        {
          this_arg_number /= (ecma_number_t) radix;
        }
      }
      else if (exponent < 0)
      {
        for (int i = 0; i < magnitude; i++)
        {
          this_arg_number *= (ecma_number_t) radix;
        }
      }

      /* Split the number into an integer and a fractional part, since we have to handle them separately. */
      uint64_t whole = (uint64_t) this_arg_number;
      ecma_number_t fraction = this_arg_number - (ecma_number_t) whole;

      bool should_round = false;
      if (!ecma_number_is_zero (fraction) && exponent >= 0)
      {
        /*
         * If the exponent is non-negative, and we get a non-zero fractional part, that means
         * the normalization might have introduced a small error, in which case we have to correct it by rounding.
         * We'll add one extra significant digit which we will later use to round.
         */
        required_digits += 1;
        should_round = true;
      }

      /* Get the total required buffer size and allocate the buffer. */
      buff_size += required_digits;
      JMEM_DEFINE_LOCAL_ARRAY (buff, buff_size, lit_utf8_byte_t);
      int buff_index = 0;

      /* Calculate digits for whole part. */
      while (whole > 0)
      {
        JERRY_ASSERT (buff_index < buff_size && buff_index < required_digits);
        buff[buff_index++] = (lit_utf8_byte_t) (whole % radix);
        whole /= radix;
      }

      /* The digits are backwards, we need to reverse them. */
      for (int i = 0; i < buff_index / 2; i++)
      {
        lit_utf8_byte_t swap = buff[i];
        buff[i] = buff[buff_index - i - 1];
        buff[buff_index - i - 1] = swap;
      }

      /*
       * Calculate where we have to put the radix point relative to the beginning of
       * the new digits. If the exponent is non-negative this will be right after the number.
       */
      int point = exponent >= 0 ? magnitude + 1: buff_index - magnitude;

      if (point < 0)
      {
        /*
         * In this case the radix point will be before the first digit,
         * so we need to leave space for leading zeros.
         */
        JERRY_ASSERT (exponent < 0);
        required_digits += point;
      }

      JERRY_ASSERT (required_digits <= buff_size);

      /* Calculate digits for fractional part. */
      while (buff_index < required_digits)
      {
        fraction *= (ecma_number_t) radix;
        lit_utf8_byte_t digit = (lit_utf8_byte_t) floor (fraction);

        buff[buff_index++] = digit;
        fraction -= (ecma_number_t) floor (fraction);
      }

      if (should_round)
      {
        /* Consume last digit for rounding. */
        buff_index--;
        if (buff[buff_index] > radix / 2)
        {
          /* We should be rounding up. */
          buff[buff_index - 1]++;

          /* Propagate carry forward in the digits. */
          for (int i = buff_index - 1; i > 0 && buff[i] >= radix; i--)
          {
            buff[i] = (lit_utf8_byte_t) (buff[i] - radix);
            buff[i - 1]++;
          }

          if (buff[0] >= radix)
          {
            /*
             * Carry propagated over the whole number, we need to add a new leading digit.
             * We can use the place of the original rounded digit, we just need to shift everything
             * right by one.
             */
            memmove (buff + 1, buff, (size_t) buff_index);
            buff_index++;
            buff[0] = 1;
          }
        }
      }

      /* Remove trailing zeros. */
      while (buff_index - 1 > point && buff[buff_index - 1] == 0)
      {
        buff_index--;
      }

      /* Add leading zeros in case place of radix point is negative. */
      if (point <= 0)
      {
        /* We will have 'point' amount of zeros after the radix point, and +1 before. */
        int zero_count = -point + 1;
        memmove (buff + zero_count, buff, (size_t) buff_index);
        buff_index += zero_count;

        for (int i = 0; i < zero_count; i++)
        {
          buff[i] = 0;
        }

        /* We now need to place the radix point after the first zero. */
        point = 1;
      }

      /* Convert digits to characters. */
      for (int i = 0; i < buff_index; i++)
      {
        buff[i] = digit_chars[buff[i]];
      }

      /* Place radix point to the required position. */
      if (point < buff_index)
      {
        memmove (buff + point + 1, buff + point, (size_t) (buff_index - point));
        buff[point] = '.';
        buff_index++;
      }

      /* Add negative sign if necessary. */
      if (is_number_negative)
      {
        memmove (buff + 1, buff, (size_t) buff_index);
        buff[0] = '-';
        buff_index++;
      }

      JERRY_ASSERT (buff_index <= buff_size);
      ecma_string_t *str_p = ecma_new_ecma_string_from_utf8 (buff, (lit_utf8_size_t) buff_index);
      ret_value = ecma_make_string_value (str_p);
      JMEM_FINALIZE_LOCAL_ARRAY (buff);
    }
    ECMA_OP_TO_NUMBER_FINALIZE (arg_num);
  }
  ECMA_FINALIZE (this_value);
  return ret_value;
} /* ecma_builtin_number_prototype_object_to_string */

/**
 * The Number.prototype object's 'toLocaleString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.7.4.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_number_prototype_object_to_locale_string (ecma_value_t this_arg) /**< this argument */
{
  return ecma_builtin_number_prototype_object_to_string (this_arg, NULL, 0);
} /* ecma_builtin_number_prototype_object_to_locale_string */

/**
 * The Number.prototype object's 'valueOf' routine
 *
 * See also:
 *          ECMA-262 v5, 15.7.4.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_number_prototype_object_value_of (ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_value_number (this_arg))
  {
    return ecma_copy_value (this_arg);
  }
  else if (ecma_is_value_object (this_arg))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (this_arg);

    if (ecma_object_class_is (object_p, LIT_MAGIC_STRING_NUMBER_UL))
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

      JERRY_ASSERT (ecma_is_value_number (ext_object_p->u.class_prop.u.value));

      return ecma_copy_value (ext_object_p->u.class_prop.u.value);
    }
  }

  return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a Number or a Number object."));
} /* ecma_builtin_number_prototype_object_value_of */

/**
 * The Number.prototype object's 'toFixed' routine
 *
 * See also:
 *          ECMA-262 v5, 15.7.4.5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_number_prototype_object_to_fixed (ecma_value_t this_arg, /**< this argument */
                                               ecma_value_t arg) /**< routine's argument */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  ECMA_TRY_CATCH (this_value, ecma_builtin_number_prototype_object_value_of (this_arg), ret_value);
  ecma_number_t this_num = ecma_get_number_from_value (this_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  /* 2. */
  if (arg_num <= -1 || arg_num >= 21)
  {
    ret_value = ecma_raise_range_error (ECMA_ERR_MSG ("Fraction digits must be between 0 and 20."));
  }
  else
  {
    /* 4. */
    if (ecma_number_is_nan (this_num))
    {
      ecma_string_t *nan_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_NAN);
      ret_value = ecma_make_string_value (nan_str_p);
    }
    else
    {
      /* 6. */
      bool is_negative = false;
      if (ecma_number_is_negative (this_num))
      {
        is_negative = ecma_number_is_zero (this_num) ? false : true;
        this_num *= -1;
      }

      /* We handle infinities separately. */
      if (ecma_number_is_infinity (this_num))
      {
        lit_magic_string_id_t id = (is_negative ? LIT_MAGIC_STRING_NEGATIVE_INFINITY_UL
                                                : LIT_MAGIC_STRING_INFINITY_UL);

        ret_value = ecma_make_string_value (ecma_get_magic_string (id));
      }
      else
      {
        /* Get the parameters of the number if non-zero. */
        lit_utf8_byte_t digits[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER];
        lit_utf8_size_t num_digits;
        int32_t exponent;
        int32_t frac_digits = ecma_number_to_int32 (arg_num);

        if (!ecma_number_is_zero (this_num))
        {
          num_digits = ecma_number_to_binary_floating_point_number (this_num, digits, &exponent);
        }
        else
        {
          for (int32_t i = 0; i <= frac_digits; i++)
          {
            digits[i] = '0';
          }
          num_digits = (lit_utf8_size_t) frac_digits + 1;
          exponent = 1;
        }

        /* 7. */
        if (exponent > 21)
        {
          ret_value = ecma_builtin_number_prototype_object_to_string (this_arg, NULL, 0);
        }
        /* 8. */
        else
        {
          /* 1. */
          num_digits = ecma_builtin_number_prototype_helper_round (digits,
                                                                   num_digits + 1,
                                                                   exponent + frac_digits,
                                                                   &exponent,
                                                                   ecma_number_is_zero (this_num) ? true : false);

          /* Buffer that is used to construct the string. */
          int buffer_size = (exponent > 0) ? exponent + frac_digits + 2 : frac_digits + 3;

          if (is_negative)
          {
            buffer_size++;
          }

          JERRY_ASSERT (buffer_size > 0);
          JMEM_DEFINE_LOCAL_ARRAY (buff, buffer_size, lit_utf8_byte_t);

          lit_utf8_byte_t *p = buff;

          if (is_negative)
          {
            *p++ = '-';
          }

          lit_utf8_size_t to_num_digits = ((exponent > 0) ? (lit_utf8_size_t) (exponent + frac_digits)
                                                          : (lit_utf8_size_t) (frac_digits + 1));
          p += ecma_builtin_binary_floating_number_to_string (digits,
                                                              exponent,
                                                              p,
                                                              to_num_digits);

          JERRY_ASSERT (p - buff < buffer_size);
          /* String terminator. */
          *p = 0;
          ecma_string_t *str = ecma_new_ecma_string_from_utf8 (buff, (lit_utf8_size_t) (p - buff));

          ret_value = ecma_make_string_value (str);
          JMEM_FINALIZE_LOCAL_ARRAY (buff);
        }
      }
    }
  }

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);
  ECMA_FINALIZE (this_value);
  return ret_value;
} /* ecma_builtin_number_prototype_object_to_fixed */

/**
 * The Number.prototype object's 'toExponential' routine
 *
 * See also:
 *          ECMA-262 v5, 15.7.4.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_number_prototype_object_to_exponential (ecma_value_t this_arg, /**< this argument */
                                                     ecma_value_t arg) /**< routine's argument */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  /* 1. */
  ECMA_TRY_CATCH (this_value, ecma_builtin_number_prototype_object_value_of (this_arg), ret_value);
  ecma_number_t this_num = ecma_get_number_from_value (this_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  /* 7. */
  if (arg_num <= -1.0 || arg_num >= 21.0)
  {
    ret_value = ecma_raise_range_error (ECMA_ERR_MSG ("Fraction digits must be between 0 and 20."));
  }
  else
  {
    /* 3. */
    if (ecma_number_is_nan (this_num))
    {
      ecma_string_t *nan_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_NAN);
      ret_value = ecma_make_string_value (nan_str_p);
    }
    else
    {
      /* 5. */
      bool is_negative = false;
      if (ecma_number_is_negative (this_num) && !ecma_number_is_zero (this_num))
      {
        is_negative = true;
        this_num *= -1;
      }

      /* 6. */
      if (ecma_number_is_infinity (this_num))
      {
        lit_magic_string_id_t id = (is_negative ? LIT_MAGIC_STRING_NEGATIVE_INFINITY_UL
                                                : LIT_MAGIC_STRING_INFINITY_UL);

        ret_value = ecma_make_string_value (ecma_get_magic_string (id));
      }
      else
      {
        /* Get the parameters of the number if non zero. */
        lit_utf8_byte_t digits[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER];
        lit_utf8_size_t num_digits;
        int32_t exponent;

        if (!ecma_number_is_zero (this_num))
        {
          num_digits = ecma_number_to_decimal (this_num, digits, &exponent);
        }
        else
        {
          digits[0] = '0';
          num_digits = 1;
          exponent = 1;
        }

        int32_t frac_digits;
        if (ecma_is_value_undefined (arg))
        {
          frac_digits = (int32_t) num_digits - 1;
        }
        else
        {
          frac_digits = ecma_number_to_int32 (arg_num);
        }

        num_digits = ecma_builtin_number_prototype_helper_round (digits, num_digits, frac_digits + 1, &exponent, false);

        /* frac_digits + 2 characters for number, 5 characters for exponent, 1 for \0. */
        int buffer_size = frac_digits + 2 + 5 + 1;

        if (is_negative)
        {
          /* +1 character for sign. */
          buffer_size++;
        }

        JMEM_DEFINE_LOCAL_ARRAY (buff, buffer_size, lit_utf8_byte_t);

        lit_utf8_byte_t *actual_char_p = buff;

        if (is_negative)
        {
          *actual_char_p++ = '-';
        }

        actual_char_p += ecma_builtin_number_prototype_helper_to_string (digits,
                                                                         num_digits,
                                                                         1,
                                                                         actual_char_p,
                                                                         (lit_utf8_size_t) (frac_digits + 1));

        *actual_char_p++ = 'e';

        exponent--;
        if (exponent < 0)
        {
          exponent *= -1;
          *actual_char_p++ = '-';
        }
        else
        {
          *actual_char_p++ = '+';
        }

        /* Add exponent digits. */
        actual_char_p += ecma_uint32_to_utf8_string ((uint32_t) exponent, actual_char_p, 3);

        JERRY_ASSERT (actual_char_p - buff < buffer_size);
        *actual_char_p = '\0';
        ecma_string_t *str = ecma_new_ecma_string_from_utf8 (buff, (lit_utf8_size_t) (actual_char_p - buff));
        ret_value = ecma_make_string_value (str);
        JMEM_FINALIZE_LOCAL_ARRAY (buff);
      }
    }
  }

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);
  ECMA_FINALIZE (this_value);
  return ret_value;
} /* ecma_builtin_number_prototype_object_to_exponential */

/**
 * The Number.prototype object's 'toPrecision' routine
 *
 * See also:
 *          ECMA-262 v5, 15.7.4.7
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_number_prototype_object_to_precision (ecma_value_t this_arg, /**< this argument */
                                                   ecma_value_t arg) /**< routine's argument */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  /* 1. */
  ECMA_TRY_CATCH (this_value, ecma_builtin_number_prototype_object_value_of (this_arg), ret_value);
  ecma_number_t this_num = ecma_get_number_from_value (this_value);

  /* 2. */
  if (ecma_is_value_undefined (arg))
  {
    ret_value = ecma_builtin_number_prototype_object_to_string (this_arg, NULL, 0);
  }
  else
  {
    /* 3. */
    ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

    /* 4. */
    if (ecma_number_is_nan (this_num))
    {
      ecma_string_t *nan_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_NAN);
      ret_value = ecma_make_string_value (nan_str_p);
    }
    else
    {
      /* 6. */
      bool is_negative = false;
      if (ecma_number_is_negative (this_num) && !ecma_number_is_zero (this_num))
      {
        is_negative = true;
        this_num *= -1;
      }

      /* 7. */
      if (ecma_number_is_infinity (this_num))
      {
        lit_magic_string_id_t id = (is_negative ? LIT_MAGIC_STRING_NEGATIVE_INFINITY_UL
                                                : LIT_MAGIC_STRING_INFINITY_UL);

        ret_value = ecma_make_string_value (ecma_get_magic_string (id));
      }
      /* 8. */
      else if (arg_num < 1.0 || arg_num >= 22.0)
      {
        ret_value = ecma_raise_range_error (ECMA_ERR_MSG ("Precision must be between 1 and 21."));
      }
      else
      {
        /* Get the parameters of the number if non-zero. */
        lit_utf8_byte_t digits[ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER];
        lit_utf8_size_t num_digits;
        int32_t exponent;

        if (!ecma_number_is_zero (this_num))
        {
          num_digits = ecma_number_to_decimal (this_num, digits, &exponent);
        }
        else
        {
          digits[0] = '0';
          num_digits = 1;
          exponent = 1;
        }

        int32_t precision = ecma_number_to_int32 (arg_num);

        num_digits = ecma_builtin_number_prototype_helper_round (digits, num_digits, precision, &exponent, false);

        int buffer_size;
        if (exponent  < -5 || exponent > precision)
        {
          /* Exponential notation, precision + 1 digits for number, 5 for exponent, 1 for \0 */
          buffer_size = precision + 1 + 5 + 1;
        }
        else if (exponent <= 0)
        {
          /* Fixed notation, -exponent + 2 digits for leading zeros, precision digits, 1 for \0 */
          buffer_size = -exponent + 2 + precision + 1;
        }
        else
        {
          /* Fixed notation, precision + 1 digits for number, 1 for \0 */
          buffer_size = precision + 1 + 1;
        }

        if (is_negative)
        {
          buffer_size++;
        }

        JMEM_DEFINE_LOCAL_ARRAY (buff, buffer_size, lit_utf8_byte_t);
        lit_utf8_byte_t *actual_char_p = buff;

        if (is_negative)
        {
          *actual_char_p++ = '-';
        }

        /* 10.c, Exponential notation.*/
        if (exponent < -5 || exponent > precision)
        {
          actual_char_p  += ecma_builtin_number_prototype_helper_to_string (digits,
                                                                            num_digits,
                                                                            1,
                                                                            actual_char_p,
                                                                            (lit_utf8_size_t) precision);

          *actual_char_p++ = 'e';

          exponent--;
          if (exponent < 0)
          {
            exponent *= -1;
            *actual_char_p++ = '-';
          }
          else
          {
            *actual_char_p++ = '+';
          }

          /* Add exponent digits. */
          actual_char_p += ecma_uint32_to_utf8_string ((uint32_t) exponent, actual_char_p, 3);
        }
        /* Fixed notation. */
        else
        {
          lit_utf8_size_t to_num_digits = ((exponent <= 0) ? (lit_utf8_size_t) (1 - exponent + precision)
                                                           : (lit_utf8_size_t) precision);
          actual_char_p += ecma_builtin_number_prototype_helper_to_string (digits,
                                                                           num_digits,
                                                                           exponent,
                                                                           actual_char_p,
                                                                           to_num_digits);

        }

        JERRY_ASSERT (actual_char_p - buff < buffer_size);
        *actual_char_p = '\0';
        ecma_string_t *str_p = ecma_new_ecma_string_from_utf8 (buff, (lit_utf8_size_t) (actual_char_p - buff));

        ret_value = ecma_make_string_value (str_p);
        JMEM_FINALIZE_LOCAL_ARRAY (buff);
      }
    }
    ECMA_OP_TO_NUMBER_FINALIZE (arg_num);
  }
  ECMA_FINALIZE (this_value);

  return ret_value;
} /* ecma_builtin_number_prototype_object_to_precision */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_NUMBER_BUILTIN */
