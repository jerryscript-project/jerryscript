/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged
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
#include "ecma-eval.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"
#include "lit-char-helpers.h"
#include "lit-magic-strings.h"
#include "lit-strings.h"
#include "vm.h"
#include "jrt-libc-includes.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-global.inc.h"
#define BUILTIN_UNDERSCORED_ID global
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup global ECMA Global object built-in
 * @{
 */

/**
 * The Global object's 'eval' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_eval (ecma_value_t this_arg __attr_unused___, /**< this argument */
                                 ecma_value_t x) /**< routine's first argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  bool is_direct_eval = vm_is_direct_eval_form_call ();

  /* See also: ECMA-262 v5, 10.1.1 */
  bool is_called_from_strict_mode_code;
  if (is_direct_eval)
  {
    is_called_from_strict_mode_code = vm_is_strict_mode ();
  }
  else
  {
    is_called_from_strict_mode_code = false;
  }

  if (!ecma_is_value_string (x))
  {
    /* step 1 */
    ret_value = ecma_make_normal_completion_value (ecma_copy_value (x, true));
  }
  else
  {
    /* steps 2 to 8 */
    ret_value = ecma_op_eval (ecma_get_string_from_value (x),
                              is_direct_eval,
                              is_called_from_strict_mode_code);
  }

  return ret_value;
} /* ecma_builtin_global_object_eval */

/**
 * The Global object's 'parseInt' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_parse_int (ecma_value_t this_arg __attr_unused___, /**< this argument */
                                      ecma_value_t string, /**< routine's first argument */
                                      ecma_value_t radix) /**< routine's second argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (string_var, ecma_op_to_string (string), ret_value);

  ecma_string_t *number_str_p = ecma_get_string_from_value (string_var);
  lit_utf8_size_t str_size = ecma_string_get_size (number_str_p);

  if (str_size > 0)
  {
    MEM_DEFINE_LOCAL_ARRAY (utf8_string_buff, str_size, lit_utf8_byte_t);

    ssize_t bytes_copied = ecma_string_to_utf8_string (number_str_p,
                                                       utf8_string_buff,
                                                       (ssize_t) str_size);
    JERRY_ASSERT (bytes_copied >= 0);
    utf8_string_buff[str_size] = LIT_BYTE_NULL;

    /* 2. Remove leading whitespace. */
    ecma_length_t start = str_size;
    ecma_length_t end = str_size;
    for (ecma_length_t i = 0; i < end; i++)
    {
      if (!lit_char_is_white_space (utf8_string_buff[i])
          && !lit_char_is_line_terminator (utf8_string_buff[i]))
      {
        start = i;
        break;
      }
    }

    /* 3. */
    int sign = 1;

    /* 4. */
    if (utf8_string_buff[start] == LIT_CHAR_MINUS)
    {
      sign = -1;
    }

    /* 5. */
    if (utf8_string_buff[start] == LIT_CHAR_MINUS || utf8_string_buff[start] == LIT_CHAR_PLUS)
    {
      start++;
    }

    /* 6. */
    ECMA_OP_TO_NUMBER_TRY_CATCH (radix_num, radix, ret_value);
    int32_t rad = ecma_number_to_int32 (radix_num);

    /* 7.*/
    bool strip_prefix = true;

    /* 8. */
    if (rad != 0)
    {
      /* 8.a */
      if (rad < 2 || rad > 36)
      {
        ecma_number_t *ret_num_p = ecma_alloc_number ();
        *ret_num_p = ecma_number_make_nan ();
        ret_value = ecma_make_normal_completion_value (ecma_make_number_value (ret_num_p));
      }
      /* 8.b */
      else if (rad != 16)
      {
        strip_prefix = false;
      }
    }
    /* 9. */
    else
    {
      rad = 10;
    }

    if (ecma_is_completion_value_empty (ret_value))
    {
      /* 10. */
      if (strip_prefix)
      {
        if (end - start >= 2
            && utf8_string_buff[start] == LIT_CHAR_0
            && (utf8_string_buff[start + 1] == LIT_CHAR_LOWERCASE_X
                || utf8_string_buff[start + 1] == LIT_CHAR_UPPERCASE_X))
        {
          start += 2;

          rad = 16;
        }
      }

      /* 11. Check if characters are in [0, Radix - 1]. We also convert them to number values in the process. */
      for (lit_utf8_size_t i = start; i < end; i++)
      {
        if ((utf8_string_buff[i]) >= LIT_CHAR_LOWERCASE_A && utf8_string_buff[i] <= LIT_CHAR_LOWERCASE_Z)
        {
          utf8_string_buff[i] = (lit_utf8_byte_t) (utf8_string_buff[i] - LIT_CHAR_LOWERCASE_A + 10);
        }
        else if (utf8_string_buff[i] >= LIT_CHAR_UPPERCASE_A && utf8_string_buff[i] <= LIT_CHAR_UPPERCASE_Z)
        {
          utf8_string_buff[i] = (lit_utf8_byte_t) (utf8_string_buff[i] - LIT_CHAR_UPPERCASE_A + 10);
        }
        else if (lit_char_is_decimal_digit (utf8_string_buff[i]))
        {
          utf8_string_buff[i] = (lit_utf8_byte_t) (utf8_string_buff[i] - LIT_CHAR_0);
        }
        else
        {
          /* Not a valid number char, set value to radix so it fails to pass as a valid character. */
          utf8_string_buff[i] = (lit_utf8_byte_t) rad;
        }

        if (!(utf8_string_buff[i] < rad))
        {
          end = i;
          break;
        }
      }

      /* 12. */
      if (end - start == 0)
      {
        ecma_number_t *ret_num_p = ecma_alloc_number ();
        *ret_num_p = ecma_number_make_nan ();
        ret_value = ecma_make_normal_completion_value (ecma_make_number_value (ret_num_p));
      }
    }

    if (ecma_is_completion_value_empty (ret_value))
    {
      ecma_number_t *value_p = ecma_alloc_number ();
      *value_p = 0;
      ecma_number_t multiplier = 1.0f;

      /* 13. and 14. */
      for (int32_t i = (int32_t) end - 1; i >= (int32_t) start; i--)
      {
        *value_p += (ecma_number_t) utf8_string_buff[i] * multiplier;
        multiplier *= (ecma_number_t) rad;
      }

      /* 15. */
      if (sign < 0)
      {
        *value_p *= (ecma_number_t) sign;
      }

      ret_value = ecma_make_normal_completion_value (ecma_make_number_value (value_p));
    }

    ECMA_OP_TO_NUMBER_FINALIZE (radix_num);
    MEM_FINALIZE_LOCAL_ARRAY (utf8_string_buff);
  }
  else
  {
    ecma_number_t *ret_num_p = ecma_alloc_number ();
    *ret_num_p = ecma_number_make_nan ();
    ret_value = ecma_make_normal_completion_value (ecma_make_number_value (ret_num_p));
  }

  ECMA_FINALIZE (string_var);
  return ret_value;
} /* ecma_builtin_global_object_parse_int */

/**
 * The Global object's 'parseFloat' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_parse_float (ecma_value_t this_arg __attr_unused___, /**< this argument */
                                        ecma_value_t string) /**< routine's first argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (string_var, ecma_op_to_string (string), ret_value);

  ecma_string_t *number_str_p = ecma_get_string_from_value (string_var);
  lit_utf8_size_t str_size = ecma_string_get_size (number_str_p);

  MEM_DEFINE_LOCAL_ARRAY (utf8_string_buff, str_size + 1, lit_utf8_byte_t);

  ssize_t bytes_copied = ecma_string_to_utf8_string (number_str_p,
                                                     utf8_string_buff,
                                                     (ssize_t) str_size);
  JERRY_ASSERT (bytes_copied >= 0);
  utf8_string_buff[str_size] = LIT_BYTE_NULL;

  /* 2. Find first non whitespace char. */
  lit_utf8_size_t start = 0;
  for (lit_utf8_size_t i = 0; i < str_size; i++)
  {
    if (!lit_char_is_white_space (utf8_string_buff[i])
        && !lit_char_is_line_terminator (utf8_string_buff[i]))
    {
      start = i;
      break;
    }
  }

  bool sign = false;

  /* Check if sign is present. */
  if (utf8_string_buff[start] == '-')
  {
    sign = true;
    start++;
  }
  else if (utf8_string_buff[start] == '+')
  {
    start++;
  }

  ecma_number_t *ret_num_p = ecma_alloc_number ();

  /* Check if string is equal to "Infinity". */
  const lit_utf8_byte_t *infinity_utf8_str_p = lit_get_magic_string_utf8 (LIT_MAGIC_STRING_INFINITY_UL);

  for (lit_utf8_size_t i = 0; infinity_utf8_str_p[i] == utf8_string_buff[start + i]; i++)
  {
    if (infinity_utf8_str_p[i + 1] == 0)
    {
      *ret_num_p = ecma_number_make_infinity (sign);
      ret_value = ecma_make_normal_completion_value (ecma_make_number_value (ret_num_p));
      break;
    }
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    lit_utf8_size_t current = start;
    lit_utf8_size_t end = str_size;
    bool has_whole_part = false;
    bool has_fraction_part = false;

    if (lit_char_is_decimal_digit (utf8_string_buff[current]))
    {
      has_whole_part = true;

      /* Check digits of whole part. */
      for (lit_utf8_size_t i = current; i < str_size; i++, current++)
      {
        if (!lit_char_is_decimal_digit (utf8_string_buff[current]))
        {
          break;
        }
      }
    }

    end = current;

    /* Check decimal point. */
    if (utf8_string_buff[current] == '.')
    {
      current++;

      if (lit_char_is_decimal_digit (utf8_string_buff[current]))
      {
        has_fraction_part = true;

        /* Check digits of fractional part. */
        for (lit_utf8_size_t i = current; i < str_size; i++, current++)
        {
          if (!lit_char_is_decimal_digit (utf8_string_buff[current]))
          {
            break;
          }
        }

        end = current;
      }
    }

    /* Check exponent. */
    if ((utf8_string_buff[current] == 'e' || utf8_string_buff[current] == 'E')
        && (has_whole_part || has_fraction_part))
    {
      current++;

      /* Check sign of exponent. */
      if (utf8_string_buff[current] == '-' || utf8_string_buff[current] == '+')
      {
        current++;
      }

      if (lit_char_is_decimal_digit (utf8_string_buff[current]))
      {

        /* Check digits of exponent part. */
        for (lit_utf8_size_t i = current; i < str_size; i++, current++)
        {
          if (!lit_char_is_decimal_digit (utf8_string_buff[current]))
          {
            break;
          }
        }

        end = current;
      }
    }

    if (start == end)
    {
      *ret_num_p = ecma_number_make_nan ();
      ret_value = ecma_make_normal_completion_value (ecma_make_number_value (ret_num_p));
    }
    else
    {
      /* 5. */
      *ret_num_p = ecma_utf8_string_to_number (utf8_string_buff + start, end - start);

      if (sign)
      {
        *ret_num_p *= -1;
      }

      ret_value = ecma_make_normal_completion_value (ecma_make_number_value (ret_num_p));
    }
  }

  MEM_FINALIZE_LOCAL_ARRAY (utf8_string_buff);
  ECMA_FINALIZE (string_var);

  return ret_value;
} /* ecma_builtin_global_object_parse_float */

/**
 * The Global object's 'isNaN' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_is_nan (ecma_value_t this_arg __attr_unused___, /**< this argument */
                                   ecma_value_t arg) /**< routine's first argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  bool is_nan = ecma_number_is_nan (arg_num);

  ret_value = ecma_make_simple_completion_value (is_nan ? ECMA_SIMPLE_VALUE_TRUE
                                                        : ECMA_SIMPLE_VALUE_FALSE);

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);

  return ret_value;
} /* ecma_builtin_global_object_is_nan */

/**
 * The Global object's 'isFinite' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.5
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_is_finite (ecma_value_t this_arg __attr_unused___, /**< this argument */
                                      ecma_value_t arg) /**< routine's first argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  bool is_finite = !(ecma_number_is_nan (arg_num)
                     || ecma_number_is_infinity (arg_num));

  ret_value = ecma_make_simple_completion_value (is_finite ? ECMA_SIMPLE_VALUE_TRUE
                                                           : ECMA_SIMPLE_VALUE_FALSE);

  ECMA_OP_TO_NUMBER_FINALIZE (arg_num);

  return ret_value;
} /* ecma_builtin_global_object_is_finite */

/**
 * Helper function to check whether a character is in a character bitset.
 *
 * @return true if the character is in the character bitset.
 */
static bool
ecma_builtin_global_object_character_is_in (uint32_t character, /**< character */
                                            uint8_t *bitset) /**< character set */
{
  JERRY_ASSERT (character < 128);
  return (bitset[character >> 3] & (1 << (character & 0x7))) != 0;
} /* ecma_builtin_global_object_character_is_in */

/*
 * Unescaped URI characters bitset:
 *   One bit for each character between 0 - 127.
 *   Bit is set if the character is in the unescaped URI set.
 */
static uint8_t unescaped_uri_set[16] =
{
  0x0, 0x0, 0x0, 0x0, 0xda, 0xff, 0xff, 0xaf,
  0xff, 0xff, 0xff, 0x87, 0xfe, 0xff, 0xff, 0x47
};

/*
 * Unescaped URI component characters bitset:
 *   One bit for each character between 0 - 127.
 *   Bit is set if the character is in the unescaped component URI set.
 */
static uint8_t unescaped_uri_component_set[16] =
{
  0x0, 0x0, 0x0, 0x0, 0x82, 0x67, 0xff, 0x3,
  0xfe, 0xff, 0xff, 0x87, 0xfe, 0xff, 0xff, 0x47
};

/*
 * Format is a percent sign followed by two hex digits.
 */
#define URI_ENCODED_BYTE_SIZE (3)

/**
 * Helper function to decode URI.
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_decode_uri_helper (ecma_value_t uri __attr_unused___, /**< uri argument */
                                              uint8_t *reserved_uri_bitset) /**< reserved characters bitset */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (string,
                  ecma_op_to_string (uri),
                  ret_value);

  JERRY_ASSERT (ecma_is_value_string (string));

  ecma_string_t *input_string_p = ecma_get_string_from_value (string);
  lit_utf8_size_t input_size = ecma_string_get_size (input_string_p);

  MEM_DEFINE_LOCAL_ARRAY (input_start_p,
                          input_size + 1,
                          lit_utf8_byte_t);

  ecma_string_to_utf8_string (input_string_p,
                              input_start_p,
                              (ssize_t) (input_size));
  input_start_p[input_size] = LIT_BYTE_NULL;

  lit_utf8_byte_t *input_char_p = input_start_p;
  lit_utf8_byte_t *input_end_p = input_start_p + input_size;
  lit_utf8_size_t output_size = 0;

  /*
   * The URI decoding has two major phases: first we validate the input,
   * and compute the length of the output, then we decode the input.
   */

  while (input_char_p < input_end_p)
  {
    /*
     * We expect that the input is a valid UTF-8 sequence,
     * so characters >= 0x80 can be let through.
     */

    if (*input_char_p != '%')
    {
      output_size++;
      input_char_p++;
      continue;
    }

    lit_code_point_t decoded_byte;

    if (!lit_read_code_point_from_hex (input_char_p + 1, 2, &decoded_byte))
    {
      ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_URI));
      break;
    }

    input_char_p += URI_ENCODED_BYTE_SIZE;

    if (decoded_byte <= LIT_UTF8_1_BYTE_CODE_POINT_MAX)
    {
      /*
       * We don't decode those bytes, which are part of reserved_uri_bitset
       * but not part of unescaped_uri_component_set.
       */
      if (ecma_builtin_global_object_character_is_in (decoded_byte, reserved_uri_bitset)
          && !ecma_builtin_global_object_character_is_in (decoded_byte, unescaped_uri_component_set))
      {
        output_size += URI_ENCODED_BYTE_SIZE;
      }
      else
      {
        output_size++;
      }
    }
    else
    {
      output_size++;
    }
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    MEM_DEFINE_LOCAL_ARRAY (output_start_p,
                            output_size,
                            lit_utf8_byte_t);

    input_char_p = input_start_p;
    lit_utf8_byte_t *output_char_p = output_start_p;

    while (input_char_p < input_end_p)
    {
      /* Input decode. */
      if (*input_char_p != '%')
      {
        *output_char_p = *input_char_p;
        output_char_p++;
        input_char_p++;
        continue;
      }

      lit_code_point_t decoded_byte;

      lit_read_code_point_from_hex (input_char_p + 1, 2, &decoded_byte);
      input_char_p += URI_ENCODED_BYTE_SIZE;

      if (decoded_byte <= LIT_UTF8_1_BYTE_CODE_POINT_MAX)
      {
        if (ecma_builtin_global_object_character_is_in (decoded_byte, reserved_uri_bitset)
            && !ecma_builtin_global_object_character_is_in (decoded_byte, unescaped_uri_component_set))
        {
          *output_char_p = '%';
          output_char_p++;
          input_char_p -= 2;
        }
        else
        {
          *output_char_p = (lit_utf8_byte_t) decoded_byte;
          output_char_p++;
        }
      }
      else
      {
        *output_char_p = (lit_utf8_byte_t) decoded_byte;
        output_char_p++;
      }
    }

    JERRY_ASSERT (output_start_p + output_size == output_char_p);

    bool valid_utf8 = lit_is_utf8_string_valid (output_start_p, output_size);

    if (valid_utf8)
    {
      lit_utf8_iterator_t characters = lit_utf8_iterator_create (output_start_p, output_size);
      while (!lit_utf8_iterator_is_eos (&characters))
      {
        ecma_char_t character = lit_utf8_iterator_read_next (&characters);

        /* Surrogate fragments are allowed in JS, but not accepted by URI decoding. */
        if (lit_is_code_unit_low_surrogate (character)
            || lit_is_code_unit_high_surrogate (character))
        {
          valid_utf8 = false;
          break;
        }
      }
    }

    if (valid_utf8)
    {
      ecma_string_t *output_string_p = ecma_new_ecma_string_from_utf8 (output_start_p, output_size);
      ret_value = ecma_make_normal_completion_value (ecma_make_string_value (output_string_p));
    }
    else
    {
      ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_URI));
    }

    MEM_FINALIZE_LOCAL_ARRAY (output_start_p);
  }

  MEM_FINALIZE_LOCAL_ARRAY (input_start_p);

  ECMA_FINALIZE (string);
  return ret_value;
} /* ecma_builtin_global_object_decode_uri_helper */

/**
 * The Global object's 'decodeURI' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.3.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_decode_uri (ecma_value_t this_arg __attr_unused___, /**< this argument */
                                       ecma_value_t encoded_uri) /**< routine's first argument */
{
  return ecma_builtin_global_object_decode_uri_helper (encoded_uri, unescaped_uri_set);
} /* ecma_builtin_global_object_decode_uri */

/**
 * The Global object's 'decodeURIComponent' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.3.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_decode_uri_component (ecma_value_t this_arg __attr_unused___, /**< this argument */
                                                 ecma_value_t encoded_uri_component) /**< routine's
                                                                                      *   first argument */
{
  return ecma_builtin_global_object_decode_uri_helper (encoded_uri_component, unescaped_uri_component_set);
} /* ecma_builtin_global_object_decode_uri_component */

/**
 * Helper function to encode byte as hexadecimal values.
 */
static void
ecma_builtin_global_object_byte_to_hex (lit_utf8_byte_t *dest_p, /**< destination pointer */
                                        uint32_t byte) /**< value */
{
  JERRY_ASSERT (byte < 256);

  dest_p[0] = '%';
  ecma_char_t hex_digit = (ecma_char_t) (byte >> 4);
  dest_p[1] = (lit_utf8_byte_t) ((hex_digit > 9) ? (hex_digit + ('A' - 10)) : (hex_digit + '0'));
  hex_digit = (lit_utf8_byte_t) (byte & 0xf);
  dest_p[2] = (lit_utf8_byte_t) ((hex_digit > 9) ? (hex_digit + ('A' - 10)) : (hex_digit + '0'));
} /* ecma_builtin_global_object_byte_to_hex */

/**
 * Helper function to encode URI.
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_encode_uri_helper (ecma_value_t uri, /**< uri argument */
                                              uint8_t* unescaped_uri_bitset) /**< unescaped bitset */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (string,
                  ecma_op_to_string (uri),
                  ret_value);

  JERRY_ASSERT (ecma_is_value_string (string));

  ecma_string_t *input_string_p = ecma_get_string_from_value (string);
  lit_utf8_size_t input_size = ecma_string_get_size (input_string_p);

  MEM_DEFINE_LOCAL_ARRAY (input_start_p,
                          input_size,
                          lit_utf8_byte_t);

  ecma_string_to_utf8_string (input_string_p,
                              input_start_p,
                              (ssize_t) (input_size));

  /*
   * The URI encoding has two major phases: first we validate the input,
   * and compute the length of the output, then we encode the input.
   */

  lit_utf8_byte_t *input_char_p = input_start_p;
  lit_utf8_byte_t *input_end_p = input_start_p + input_size;
  lit_utf8_size_t output_length = 0;

  while (input_char_p < input_end_p)
  {
    /*
     * We expect that the input is a valid UTF-8 sequence,
     * so we only need to reject stray surrogate pairs.
     */

    /* Input validation. */
    if (*input_char_p <= LIT_UTF8_1_BYTE_CODE_POINT_MAX)
    {
      if (ecma_builtin_global_object_character_is_in (*input_char_p, unescaped_uri_bitset))
      {
        output_length++;
      }
      else
      {
        output_length += URI_ENCODED_BYTE_SIZE;
      }
    }
    else if (*input_char_p == (LIT_UTF8_3_BYTE_MARKER + (LIT_UTF16_HIGH_SURROGATE_MARKER >> 12)))
    {
      /* The next character is in the [0xd000, 0xdfff] range. */
      output_length += URI_ENCODED_BYTE_SIZE;
      input_char_p++;
      JERRY_ASSERT (input_char_p < input_end_p);
      JERRY_ASSERT ((*input_char_p & LIT_UTF8_EXTRA_BYTE_MASK) == LIT_UTF8_EXTRA_BYTE_MARKER);

      /* If this condition is true, the next character is >= LIT_UTF16_HIGH_SURROGATE_MIN. */
      if (*input_char_p & 0x20)
      {
        ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_URI));
        break;
      }
      output_length += URI_ENCODED_BYTE_SIZE;
    }
    else
    {
      output_length += URI_ENCODED_BYTE_SIZE;
    }

    input_char_p++;
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    MEM_DEFINE_LOCAL_ARRAY (output_start_p,
                            output_length,
                            lit_utf8_byte_t);

    lit_utf8_byte_t *output_char_p = output_start_p;
    input_char_p = input_start_p;

    while (input_char_p < input_end_p)
    {
      /* Input decode. */

      if (*input_char_p <= LIT_UTF8_1_BYTE_CODE_POINT_MAX)
      {
        if (ecma_builtin_global_object_character_is_in (*input_char_p, unescaped_uri_bitset))
        {
          *output_char_p++ = *input_char_p;
        }
        else
        {
          ecma_builtin_global_object_byte_to_hex (output_char_p, *input_char_p);
          output_char_p += URI_ENCODED_BYTE_SIZE;
        }
      }
      else
      {
        ecma_builtin_global_object_byte_to_hex (output_char_p, *input_char_p);
        output_char_p += URI_ENCODED_BYTE_SIZE;
      }

      input_char_p++;
    }

    JERRY_ASSERT (output_start_p + output_length == output_char_p);

    ecma_string_t *output_string_p = ecma_new_ecma_string_from_utf8 (output_start_p, output_length);

    ret_value = ecma_make_normal_completion_value (ecma_make_string_value (output_string_p));

    MEM_FINALIZE_LOCAL_ARRAY (output_start_p);
  }

  MEM_FINALIZE_LOCAL_ARRAY (input_start_p);

  ECMA_FINALIZE (string);
  return ret_value;
} /* ecma_builtin_global_object_encode_uri_helper */

/**
 * The Global object's 'encodeURI' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.3.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_encode_uri (ecma_value_t this_arg __attr_unused___, /**< this argument */
                                       ecma_value_t uri) /**< routine's first argument */
{
  return ecma_builtin_global_object_encode_uri_helper (uri, unescaped_uri_set);
} /* ecma_builtin_global_object_encode_uri */

/**
 * The Global object's 'encodeURIComponent' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.3.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_encode_uri_component (ecma_value_t this_arg __attr_unused___, /**< this argument */
                                                 ecma_value_t uri_component) /**< routine's first argument */
{
  return ecma_builtin_global_object_encode_uri_helper (uri_component, unescaped_uri_component_set);
} /* ecma_builtin_global_object_encode_uri_component */

/**
 * @}
 * @}
 * @}
 */
