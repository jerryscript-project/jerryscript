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
#include "jrt.h"

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
  ecma_number_t this_arg_number;

  if (ecma_is_value_number (this_arg))
  {
    ecma_number_t *this_arg_number_p = ecma_get_number_from_value (this_arg);

    this_arg_number = *this_arg_number_p;
  }
  else if (ecma_is_value_object (this_arg))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (this_arg);

    if (ecma_object_get_class_name (obj_p) == ECMA_MAGIC_STRING_NUMBER_UL)
    {
      ecma_property_t *prim_value_prop_p = ecma_get_internal_property (obj_p,
                                                                       ECMA_INTERNAL_PROPERTY_PRIMITIVE_NUMBER_VALUE);

      ecma_number_t *prim_value_num_p = ECMA_GET_NON_NULL_POINTER (ecma_number_t,
                                                                   prim_value_prop_p->u.internal_property.value);
      this_arg_number = *prim_value_num_p;
    }
    else
    {
      return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
  }
  else
  {
    return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }

  if (arguments_list_len == 0)
  {
    ecma_string_t *ret_str_p = ecma_new_ecma_string_from_number (this_arg_number);

    return ecma_make_normal_completion_value (ecma_make_string_value (ret_str_p));
  }
  {
    ECMA_BUILTIN_CP_UNIMPLEMENTED (arguments_list_p);
  }
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

    if (ecma_object_get_class_name (obj_p) == ECMA_MAGIC_STRING_NUMBER_UL)
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

  ECMA_OP_TO_NUMBER_TRY_CATCH (this_num, this_arg, ret_value);
  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  /* 1. */
  int32_t frac_digits = ecma_number_to_int32 (arg_num);

  /* 2. */
  if (frac_digits < 0 || frac_digits > 20)
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_RANGE));
  }
  else
  {
    /* 4. */
    if (ecma_number_is_nan (this_num))
    {
      ecma_string_t *nan_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING_NAN);
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
        ecma_string_t *infinity_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING_INFINITY_UL);

        if (is_negative)
        {
          ecma_string_t *neg_str_p = ecma_new_ecma_string ((const ecma_char_t *) "-");
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
          MEM_DEFINE_LOCAL_ARRAY (buff, buffer_size, ecma_char_t);

          ecma_char_t* p = buff;

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

              *p = (ecma_char_t) ((ecma_char_t) digit + '0');
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

              *p = (ecma_char_t) ((ecma_char_t) digit + '0');
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

              *p = (ecma_char_t) ((ecma_char_t) digit + '0');
              p++;
            }
          }

          JERRY_ASSERT (p - buff < buffer_size);
          /* String terminator. */
          *p = 0;
          ecma_string_t* str = ecma_new_ecma_string ((ecma_char_t *) buff);

          ret_value = ecma_make_normal_completion_value (ecma_make_string_value (str));
          MEM_FINALIZE_LOCAL_ARRAY (buff);
        }
      }
    }
  }

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);
  ECMA_OP_TO_NUMBER_FINALIZE (this_num);
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
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
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
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, arg);
} /* ecma_builtin_number_prototype_object_to_precision */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_NUMBER_BUILTIN */
