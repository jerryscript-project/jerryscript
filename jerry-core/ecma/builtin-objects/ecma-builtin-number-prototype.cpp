/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged.
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
#include "fdlibm-math.h"
#include "jrt.h"
#include "jrt-libc-includes.h"

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_NUMBER_BUILTIN

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
 * Helper for rounding numbers
 *
 * @return rounded number
 */
static uint64_t
ecma_builtin_number_prototype_helper_round (uint64_t digits, /**< actual number **/
                                            int32_t round_num) /**< number of digits to round off **/
{
  int8_t digit = 0;

  /* Remove unneeded precision digits. */
  while (round_num > 0)
  {
    digit = (int8_t) (digits % 10);
    digits /= 10;
    round_num--;
  }

  /* Round the last digit up if neccessary */
  if (digit >= 5)
  {
    digits++;
  }

  return digits;
} /* ecma_builtin_number_prototype_helper_round */

/**
 * The Number.prototype object's 'toString' routine
 *
 * See also:
 *          ECMA-262 v5, 15.7.4.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_number_prototype_object_to_string (ecma_value_t this_arg, /**< this argument */
                                                const ecma_value_t* arguments_list_p, /**< arguments list */
                                                ecma_length_t arguments_list_len) /**< number of arguments */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (this_value, ecma_builtin_number_prototype_object_value_of (this_arg), ret_value);
  ecma_number_t this_arg_number = *ecma_get_number_from_value (this_value);

  if (arguments_list_len == 0
      || ecma_number_is_nan (this_arg_number)
      || ecma_number_is_infinity (this_arg_number)
      || ecma_number_is_zero (this_arg_number)
      || (arguments_list_len > 0 && ecma_is_value_undefined (arguments_list_p[0])))
  {
    ecma_string_t *ret_str_p = ecma_new_ecma_string_from_number (this_arg_number);

    ret_value = ecma_make_normal_completion_value (ecma_make_string_value (ret_str_p));
  }
  else
  {
    const lit_utf8_byte_t digit_chars[36] =
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
      ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_RANGE));
    }
    else if (radix == 10)
    {
      ecma_string_t *ret_str_p = ecma_new_ecma_string_from_number (this_arg_number);

      ret_value = ecma_make_normal_completion_value (ecma_make_string_value (ret_str_p));
    }
    else
    {
      uint64_t digits;
      int32_t num_digits;
      int32_t exponent;
      bool is_negative = false;
      bool should_round = false;

      if (ecma_number_is_negative (this_arg_number))
      {
        this_arg_number = -this_arg_number;
        is_negative = true;
      }

      ecma_number_to_decimal (this_arg_number, &digits, &num_digits, &exponent);

      exponent = exponent - num_digits;
      bool is_scale_negative = false;

      /* Calculate the scale of the number in the specified radix. */
      int scale = (int) -floor ((log (10) / log (radix)) * exponent);

      if (scale < 0)
      {
        is_scale_negative = true;
        scale = -scale;
      }

      int buff_size;

      if (is_scale_negative)
      {
        buff_size = (int) floor (log (this_arg_number) / log (radix)) + 1;
      }
      else
      {
        buff_size = scale + ECMA_NUMBER_FRACTION_WIDTH + 2;
      }

      if (is_negative)
      {
        buff_size++;
      }

      /* Normalize the number, so that it is as close to 0 exponent as possible. */
      for (int i = 0; i < scale; i++)
      {
        if (is_scale_negative)
        {
          this_arg_number /= (ecma_number_t) radix;
        }
        else
        {
          this_arg_number *= (ecma_number_t) radix;
        }
      }

      uint64_t whole = (uint64_t) this_arg_number;
      ecma_number_t fraction = this_arg_number - (ecma_number_t) whole;

      if (!ecma_number_is_zero (fraction) && is_scale_negative)
      {
        /* Add one extra digit for rounding. */
        buff_size++;
        should_round = true;
      }

      MEM_DEFINE_LOCAL_ARRAY (buff, buff_size, lit_utf8_byte_t);
      int buff_index = 0;

      /* Calculate digits for whole part. */
      while (whole > 0)
      {
        buff[buff_index++] = (lit_utf8_byte_t) (whole % radix);
        whole /= radix;
      }

      /* Calculate where we have to put the radix point. */
      int point = is_scale_negative ? buff_index + scale : buff_index - scale;

      /* Reverse the digits, since they are backwards. */
      for (int i = 0; i < buff_index / 2; i++)
      {
        lit_utf8_byte_t swap = buff[i];
        buff[i] = buff[buff_index - i - 1];
        buff[buff_index - i - 1] = swap;
      }

      int required_digits = buff_size;
      if (is_negative)
      {
        required_digits--;
      }

      if (!is_scale_negative)
      {
        /* Leave space for leading zeros / radix point. */
        required_digits -= scale + 1;
      }

      /* Calculate digits for fractional part. */
      while (buff_index < required_digits && (fraction != 0 || is_scale_negative))
      {
        fraction *= (ecma_number_t) radix;
        lit_utf8_byte_t digit = (lit_utf8_byte_t) floor (fraction);

        buff[buff_index++] = digit;
        fraction -= (ecma_number_t) floor (fraction);
      }

      if (should_round)
      {
        /* Round off last digit. */
        if (buff[buff_index - 1] > radix / 2)
        {
          buff[buff_index - 2]++;
        }

        buff_index--;

        /* Propagate carry. */
        for (int i = buff_index - 1; i > 0 && buff[i] >= radix; i--)
        {
          buff[i] = (lit_utf8_byte_t) (buff[i] - radix);
          buff[i - 1]++;
        }

        /* Carry propagated over the whole number, need to add a leading digit. */
        if (buff[0] >= radix)
        {
          memmove (buff + 1, buff, (size_t) buff_index);
          buff_index++;
          buff[0] = 1;
        }
      }

      /* Remove trailing zeros from fraction. */
      while (buff_index - 1 > point && buff[buff_index - 1] == 0)
      {
        buff_index--;
      }

      /* Add leading zeros in case place of radix point is negative. */
      if (point <= 0)
      {
        memmove (buff - point + 1, buff, (size_t) buff_index);
        buff_index += -point + 1;

        for (int i = 0; i < -point + 1; i++)
        {
          buff[i] = 0;
        }

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
        memmove (buff + point + 1, buff + point,  (size_t) buff_index);
        buff[point] = '.';
        buff_index++;
      }

      /* Add negative sign if necessary. */
      if (is_negative)
      {
        memmove (buff + 1, buff, (size_t) buff_index);
        buff_index++;
        buff[0] = '-';
      }

      JERRY_ASSERT (buff_index <= buff_size);
      ecma_string_t* str_p = ecma_new_ecma_string_from_utf8 (buff, (lit_utf8_size_t) buff_index);
      ret_value = ecma_make_normal_completion_value (ecma_make_string_value (str_p));
      MEM_FINALIZE_LOCAL_ARRAY (buff);
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
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
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
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_number_prototype_object_value_of (ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_value_number (this_arg))
  {
    return ecma_make_normal_completion_value (ecma_copy_value (this_arg, true));
  }
  else if (ecma_is_value_object (this_arg))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (this_arg);

    if (ecma_object_get_class_name (obj_p) == LIT_MAGIC_STRING_NUMBER_UL)
    {
      ecma_property_t *prim_value_prop_p = ecma_get_internal_property (obj_p,
                                                                       ECMA_INTERNAL_PROPERTY_PRIMITIVE_NUMBER_VALUE);

      ecma_number_t *prim_value_num_p = ECMA_GET_NON_NULL_POINTER (ecma_number_t,
                                                                   prim_value_prop_p->u.internal_property.value);

      ecma_number_t *ret_num_p = ecma_alloc_number ();
      *ret_num_p = *prim_value_num_p;

      return ecma_make_normal_completion_value (ecma_make_number_value (ret_num_p));
    }
  }

  return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
} /* ecma_builtin_number_prototype_object_value_of */

/**
 * The Number.prototype object's 'toFixed' routine
 *
 * See also:
 *          ECMA-262 v5, 15.7.4.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_number_prototype_object_to_fixed (ecma_value_t this_arg, /**< this argument */
                                               ecma_value_t arg) /**< routine's argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (this_value, ecma_builtin_number_prototype_object_value_of (this_arg), ret_value);
  ecma_number_t this_num = *ecma_get_number_from_value (this_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  /* 2. */
  if (arg_num <= -1 || arg_num >= 21)
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_RANGE));
  }
  else
  {
    /* 4. */
    if (ecma_number_is_nan (this_num))
    {
      ecma_string_t *nan_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_NAN);
      ret_value = ecma_make_normal_completion_value (ecma_make_string_value (nan_str_p));
    }
    else
    {
      bool is_negative = false;

      /* 6. */
      if (ecma_number_is_negative (this_num))
      {
        is_negative = true;
        this_num *= -1;
      }

      /* We handle infinities separately. */
      if (ecma_number_is_infinity (this_num))
      {
        ecma_string_t *infinity_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_INFINITY_UL);

        if (is_negative)
        {
          ecma_string_t *neg_str_p = ecma_new_ecma_string_from_utf8 ((const lit_utf8_byte_t *) "-", 1);
          ecma_string_t *neg_inf_str_p = ecma_concat_ecma_strings (neg_str_p, infinity_str_p);
          ecma_deref_ecma_string (infinity_str_p);
          ecma_deref_ecma_string (neg_str_p);
          ret_value = ecma_make_normal_completion_value (ecma_make_string_value (neg_inf_str_p));
        }
        else
        {
          ret_value = ecma_make_normal_completion_value (ecma_make_string_value (infinity_str_p));
        }
      }
      else
      {
        uint64_t digits = 0;
        int32_t num_digits = 0;
        int32_t exponent = 1;

        /* 1. */
        int32_t frac_digits = ecma_number_to_int32 (arg_num);

        /* Get the parameters of the number if non-zero. */
        if (!ecma_number_is_zero (this_num))
        {
          ecma_number_to_decimal (this_num, &digits, &num_digits, &exponent);
        }

        digits = ecma_builtin_number_prototype_helper_round (digits, num_digits - exponent - frac_digits);

        /* 7. */
        if (exponent > 21)
        {
          ret_value = ecma_builtin_number_prototype_object_to_string (this_arg, NULL, 0);
        }
        /* 8. */
        else
        {
          /* Buffer that is used to construct the string. */
          int buffer_size = (exponent > 0) ? exponent + frac_digits + 2 : frac_digits + 3;

          if (is_negative)
          {
            buffer_size++;
          }

          JERRY_ASSERT (buffer_size > 0);
          MEM_DEFINE_LOCAL_ARRAY (buff, buffer_size, lit_utf8_byte_t);

          lit_utf8_byte_t *p = buff;

          if (is_negative)
          {
            *p++ = '-';
          }

          int8_t digit = 0;
          uint64_t s = 1;

          /* Calculate the magnitude of the number. This is used to get the digits from left to right. */
          while (s <= digits)
          {
            s *= 10;
          }

          if (exponent <= 0)
          {
            /* Add leading zeros. */
            *p++ = '0';

            if (frac_digits != 0)
            {
              *p++ = '.';
            }

            for (int i = 0; i < -exponent && i < frac_digits; i++)
            {
              *p++ = '0';
            }

            /* Add significant digits. */
            for (int i = -exponent; i < frac_digits; i++)
            {
              digit = 0;
              s /= 10;

              while (digits >= s && s > 0)
              {
                digits -= s;
                digit++;
              }

              *p = (lit_utf8_byte_t) ((lit_utf8_byte_t) digit + '0');
              p++;
            }
          }
          else
          {
            /* Add significant digits. */
            for (int i = 0; i < exponent; i++)
            {
              digit = 0;
              s /= 10;

              while (digits >= s && s > 0)
              {
                digits -= s;
                digit++;
              }

              *p = (lit_utf8_byte_t) ((lit_utf8_byte_t) digit + '0');
              p++;
            }

            /* Add the decimal point after whole part. */
            if (frac_digits != 0)
            {
              *p++ = '.';
            }

            /* Add neccessary fracion digits. */
            for (int i = 0; i < frac_digits; i++)
            {
              digit = 0;
              s /= 10;

              while (digits >= s && s > 0)
              {
                digits -= s;
                digit++;
              }

              *p = (lit_utf8_byte_t) ((lit_utf8_byte_t) digit + '0');
              p++;
            }
          }

          JERRY_ASSERT (p - buff < buffer_size);
          /* String terminator. */
          *p = 0;
          ecma_string_t* str = ecma_new_ecma_string_from_utf8 (buff, (lit_utf8_size_t) (p - buff));

          ret_value = ecma_make_normal_completion_value (ecma_make_string_value (str));
          MEM_FINALIZE_LOCAL_ARRAY (buff);
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
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_number_prototype_object_to_exponential (ecma_value_t this_arg, /**< this argument */
                                                     ecma_value_t arg) /**< routine's argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (this_value, ecma_builtin_number_prototype_object_value_of (this_arg), ret_value);
  ecma_number_t this_num = *ecma_get_number_from_value (this_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  /* 7. */
  if (arg_num <= -1.0 || arg_num >= 21.0)
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_RANGE));
  }
  else
  {
    /* 3. */
    if (ecma_number_is_nan (this_num))
    {
      ecma_string_t *nan_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_NAN);
      ret_value = ecma_make_normal_completion_value (ecma_make_string_value (nan_str_p));
    }
    else
    {
      bool is_negative = false;

      /* 5. */
      if (ecma_number_is_negative (this_num) && !ecma_number_is_zero (this_num))
      {
        is_negative = true;
        this_num *= -1;
      }

      /* 6. */
      if (ecma_number_is_infinity (this_num))
      {
        ecma_string_t *infinity_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_INFINITY_UL);

        if (is_negative)
        {
          ecma_string_t *neg_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_MINUS_CHAR);
          ecma_string_t *neg_inf_str_p = ecma_concat_ecma_strings (neg_str_p, infinity_str_p);
          ecma_deref_ecma_string (infinity_str_p);
          ecma_deref_ecma_string (neg_str_p);
          ret_value = ecma_make_normal_completion_value (ecma_make_string_value (neg_inf_str_p));
        }
        else
        {
          ret_value = ecma_make_normal_completion_value (ecma_make_string_value (infinity_str_p));
        }
      }
      else
      {
        uint64_t digits = 0;
        int32_t num_digits = 0;
        int32_t exponent = 1;

        if (!ecma_number_is_zero (this_num))
        {
          /* Get the parameters of the number if non zero. */
          ecma_number_to_decimal (this_num, &digits, &num_digits, &exponent);
        }

        int32_t frac_digits;
        if (ecma_is_value_undefined (arg))
        {
          frac_digits = num_digits - 1;
        }
        else
        {
          frac_digits = ecma_number_to_int32 (arg_num);
        }

        digits = ecma_builtin_number_prototype_helper_round (digits, num_digits - frac_digits - 1);

        /* frac_digits + 2 characters for number, 5 characters for exponent, 1 for \0. */
        int buffer_size = frac_digits + 2 + 5 + 1;

        if (is_negative)
        {
          /* +1 character for sign. */
          buffer_size++;
        }

        MEM_DEFINE_LOCAL_ARRAY (buff, buffer_size, lit_utf8_byte_t);

        int digit = 0;
        uint64_t scale = 1;

        /* Calculate the magnitude of the number. This is used to get the digits from left to right. */
        while (scale <= digits)
        {
          scale *= 10;
        }

        lit_utf8_byte_t *actual_char_p = buff;

        if (is_negative)
        {
          *actual_char_p++ = '-';
        }

        /* Add significant digits. */
        for (int i = 0; i <= frac_digits; i++)
        {
          digit = 0;
          scale /= 10;
          while (digits >= scale && scale > 0)
          {
            digits -= scale;
            digit++;
          }

          *actual_char_p = (lit_utf8_byte_t) (digit + '0');
          actual_char_p++;

          if (i == 0 && frac_digits != 0)
          {
            *actual_char_p++ = '.';
          }
        }

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

        /* Get magnitude of exponent. */
        int32_t scale_expt = 1;
        while (scale_expt <= exponent)
        {
          scale_expt *= 10;
        }
        scale_expt /= 10;

        /* Add exponent digits. */
        if (exponent == 0)
        {
          *actual_char_p++ = '0';
        }
        else
        {
          while (scale_expt > 0)
          {
            digit = exponent / scale_expt;
            exponent %= scale_expt;
            *actual_char_p++ = (lit_utf8_byte_t) (digit + '0');
            scale_expt /= 10;
          }
        }

        JERRY_ASSERT (actual_char_p - buff < buffer_size);
        *actual_char_p = '\0';
        ecma_string_t *str = ecma_new_ecma_string_from_utf8 (buff, (lit_utf8_size_t) (actual_char_p - buff));
        ret_value = ecma_make_normal_completion_value (ecma_make_string_value (str));
        MEM_FINALIZE_LOCAL_ARRAY (buff);
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
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_number_prototype_object_to_precision (ecma_value_t this_arg, /**< this argument */
                                                   ecma_value_t arg) /**< routine's argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (this_value, ecma_builtin_number_prototype_object_value_of (this_arg), ret_value);
  ecma_number_t this_num = *ecma_get_number_from_value (this_value);

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
      ret_value = ecma_make_normal_completion_value (ecma_make_string_value (nan_str_p));
    }
    else
    {
      bool is_negative = false;

      /* 6. */
      if (ecma_number_is_negative (this_num) && !ecma_number_is_zero (this_num))
      {
        is_negative = true;
        this_num *= -1;
      }

      /* 7. */
      if (ecma_number_is_infinity (this_num))
      {
        ecma_string_t *infinity_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_INFINITY_UL);

        if (is_negative)
        {
          ecma_string_t *neg_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_MINUS_CHAR);
          ecma_string_t *neg_inf_str_p = ecma_concat_ecma_strings (neg_str_p, infinity_str_p);
          ecma_deref_ecma_string (infinity_str_p);
          ecma_deref_ecma_string (neg_str_p);
          ret_value = ecma_make_normal_completion_value (ecma_make_string_value (neg_inf_str_p));
        }
        else
        {
          ret_value = ecma_make_normal_completion_value (ecma_make_string_value (infinity_str_p));
        }
      }
      /* 8. */
      else if (arg_num < 1.0 || arg_num >= 22.0)
      {
        ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_RANGE));
      }
      else
      {
        uint64_t digits = 0;
        int32_t num_digits = 0;
        int32_t exponent = 1;

        int32_t precision = ecma_number_to_int32 (arg_num);

        /* Get the parameters of the number if non-zero. */
        if (!ecma_number_is_zero (this_num))
        {
          ecma_number_to_decimal (this_num, &digits, &num_digits, &exponent);
        }

        digits = ecma_builtin_number_prototype_helper_round (digits, num_digits - precision);

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

        MEM_DEFINE_LOCAL_ARRAY (buff, buffer_size, lit_utf8_byte_t);
        lit_utf8_byte_t *actual_char_p = buff;

        uint64_t scale = 1;

        /* Calculate the magnitude of the number. This is used to get the digits from left to right. */
        while (scale <= digits)
        {
          scale *= 10;
        }

        if (is_negative)
        {
          *actual_char_p++ = '-';
        }

        int digit = 0;

        /* 10.c, Exponential notation.*/
        if (exponent < -5 || exponent > precision)
        {
          /* Add significant digits. */
          for (int i = 1; i <= precision; i++)
          {
            digit = 0;
            scale /= 10;
            while (digits >= scale && scale > 0)
            {
              digits -= scale;
              digit++;
            }

            *actual_char_p++ = (lit_utf8_byte_t) (digit + '0');

            if (i == 1 && i != precision)
            {
              *actual_char_p++ = '.';
            }
          }

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

          /* Get magnitude of exponent. */
          int32_t scale_expt = 1;
          while (scale_expt <= exponent)
          {
            scale_expt *= 10;
          }
          scale_expt /= 10;

          /* Add exponent digits. */
          if (exponent == 0)
          {
            *actual_char_p++ = '0';
          }
          else
          {
            while (scale_expt > 0)
            {
              digit = exponent / scale_expt;
              exponent %= scale_expt;
              *actual_char_p++ = (lit_utf8_byte_t) (digit + '0');
              scale_expt /= 10;
            }
          }
        }
        /* Fixed notation. */
        else
        {
          /* Add leading zeros if neccessary. */
          if (exponent <= 0)
          {
            *actual_char_p++ = '0';
            *actual_char_p++ = '.';
            for (int i = exponent; i < 0; i++)
            {
              *actual_char_p++ = '0';
            }
          }

          /* Add significant digits. */
          for (int i = 1; i <= precision; i++)
          {
            digit = 0;
            scale /= 10;
            while (digits >= scale && scale > 0)
            {
              digits -= scale;
              digit++;
            }

            *actual_char_p++ = (lit_utf8_byte_t) (digit + '0');

            if (i == exponent && i != precision)
            {
              *actual_char_p++ = '.';
            }
          }
        }

        JERRY_ASSERT (actual_char_p - buff < buffer_size);
        *actual_char_p = '\0';
        ecma_string_t *str_p = ecma_new_ecma_string_from_utf8 (buff, (lit_utf8_size_t) (actual_char_p - buff));

        ret_value = ecma_make_normal_completion_value (ecma_make_string_value (str_p));
        MEM_FINALIZE_LOCAL_ARRAY (buff);
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

#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_NUMBER_BUILTIN */
