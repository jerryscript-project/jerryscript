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
#include "jrt-bit-fields.h"

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
 * The implementation-defined Global object's 'print' routine
 *
 * The routine converts all of its arguments to strings and outputs them using 'jerry_port_console'.
 *
 * Code points, with except of NUL character, that are representable with one utf8-byte
 * are outputted as is, using "%c" format argument, and other code points are outputted as "\uhhll",
 * where hh and ll are values of code point's high and low bytes, correspondingly.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_global_object_print (ecma_value_t this_arg, /**< this argument */
                                  const ecma_value_t args[], /**< arguments list */
                                  ecma_length_t args_number) /**< number of arguments */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  for (ecma_length_t arg_index = 0;
       ecma_is_value_empty (ret_value) && arg_index < args_number;
       arg_index++)
  {
    ECMA_TRY_CATCH (str_value,
                    ecma_op_to_string (args[arg_index]),
                    ret_value);

    ecma_string_t *str_p = ecma_get_string_from_value (str_value);

    lit_utf8_size_t utf8_str_size = ecma_string_get_size (str_p);

    JMEM_DEFINE_LOCAL_ARRAY (utf8_str_p,
                             utf8_str_size,
                             lit_utf8_byte_t);

    ecma_string_to_utf8_bytes (str_p, utf8_str_p, utf8_str_size);

    const lit_utf8_byte_t *utf8_str_curr_p = utf8_str_p;
    const lit_utf8_byte_t *utf8_str_end_p = utf8_str_p + utf8_str_size;

    while (utf8_str_curr_p < utf8_str_end_p)
    {
      ecma_char_t code_unit = lit_utf8_read_next (&utf8_str_curr_p);

      if (code_unit == LIT_CHAR_NULL)
      {
        jerry_port_console ("\\u0000");
      }
      else if (code_unit <= LIT_UTF8_1_BYTE_CODE_POINT_MAX)
      {
        jerry_port_console ("%c", (char) code_unit);
      }
      else
      {
        JERRY_STATIC_ASSERT (sizeof (code_unit) == 2,
                             size_of_code_point_must_be_equal_to_2_bytes);

        uint32_t byte_high = (uint32_t) JRT_EXTRACT_BIT_FIELD (ecma_char_t, code_unit,
                                                               JERRY_BITSINBYTE,
                                                               JERRY_BITSINBYTE);
        uint32_t byte_low = (uint32_t) JRT_EXTRACT_BIT_FIELD (ecma_char_t, code_unit,
                                                              0,
                                                              JERRY_BITSINBYTE);

        jerry_port_console ("\\u%02x%02x", (unsigned int) byte_high, (unsigned int) byte_low);
      }
    }

    if (arg_index < args_number - 1)
    {
      jerry_port_console (" ");
    }

    JMEM_FINALIZE_LOCAL_ARRAY (utf8_str_p);

    ECMA_FINALIZE (str_value);
  }

  jerry_port_console ("\n");

  if (ecma_is_value_empty (ret_value))
  {
    ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  }

  return ret_value;
} /* ecma_builtin_global_object_print */

/**
 * The Global object's 'eval' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_global_object_eval (ecma_value_t this_arg, /**< this argument */
                                 ecma_value_t x) /**< routine's first argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

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
    ret_value = ecma_copy_value (x);
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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_global_object_parse_int (ecma_value_t this_arg, /**< this argument */
                                      ecma_value_t string, /**< routine's first argument */
                                      ecma_value_t radix) /**< routine's second argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (string_var, ecma_op_to_string (string), ret_value);

  ecma_string_t *number_str_p = ecma_get_string_from_value (string_var);
  ECMA_STRING_TO_UTF8_STRING (number_str_p, string_buff, string_buff_size);

  if (string_buff_size > 0)
  {
    const lit_utf8_byte_t *string_curr_p = (lit_utf8_byte_t *) string_buff;
    const lit_utf8_byte_t *string_end_p = string_buff + string_buff_size;

    /* 2. Remove leading whitespace. */

    const lit_utf8_byte_t *start_p = string_end_p;
    const lit_utf8_byte_t *end_p = start_p;

    while (string_curr_p < string_end_p)
    {
      ecma_char_t current_char = lit_utf8_read_next (&string_curr_p);

      if (!lit_char_is_white_space (current_char)
          && !lit_char_is_line_terminator (current_char))
      {
        lit_utf8_decr (&string_curr_p);
        start_p = string_curr_p;
        break;
      }
    }

    if (string_curr_p < string_end_p)
    {
      /* 3. */
      int sign = 1;

      /* 4. */
      ecma_char_t current = lit_utf8_read_next (&string_curr_p);
      if (current == LIT_CHAR_MINUS)
      {
        sign = -1;
      }

      /* 5. */
      if (current == LIT_CHAR_MINUS || current == LIT_CHAR_PLUS)
      {
        start_p = string_curr_p;
        if (string_curr_p < string_end_p)
        {
          current = lit_utf8_read_next (&string_curr_p);
        }
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
          ret_value = ecma_make_nan_value ();
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

      if (ecma_is_value_empty (ret_value))
      {
        /* 10. */
        if (strip_prefix)
        {
          if (end_p - start_p >= 2 && current == LIT_CHAR_0)
          {
            ecma_char_t next = *string_curr_p;
            if (next == LIT_CHAR_LOWERCASE_X || next == LIT_CHAR_UPPERCASE_X)
            {
              /* Skip the 'x' or 'X' characters. */
              start_p = ++string_curr_p;
              rad = 16;
            }
          }
        }

        /* 11. Check if characters are in [0, Radix - 1]. We also convert them to number values in the process. */
        string_curr_p = start_p;
        while (string_curr_p < string_end_p)
        {
          ecma_char_t current_char = *string_curr_p++;
          int32_t current_number;

          if ((current_char >= LIT_CHAR_LOWERCASE_A && current_char <= LIT_CHAR_LOWERCASE_Z))
          {
            current_number = current_char - LIT_CHAR_LOWERCASE_A + 10;
          }
          else if ((current_char >= LIT_CHAR_UPPERCASE_A && current_char <= LIT_CHAR_UPPERCASE_Z))
          {
            current_number = current_char - LIT_CHAR_UPPERCASE_A + 10;
          }
          else if (lit_char_is_decimal_digit (current_char))
          {
            current_number = current_char - LIT_CHAR_0;
          }
          else
          {
            /* Not a valid number char, set value to radix so it fails to pass as a valid character. */
            current_number = rad;
          }

          if (!(current_number < rad))
          {
            end_p = --string_curr_p;
            break;
          }
        }

        /* 12. */
        if (end_p == start_p)
        {
          ret_value = ecma_make_nan_value ();
        }
      }

      if (ecma_is_value_empty (ret_value))
      {
        ecma_number_t value = ECMA_NUMBER_ZERO;
        ecma_number_t multiplier = 1.0f;

        /* 13. and 14. */
        string_curr_p = end_p;

        while (string_curr_p > start_p)
        {
          ecma_char_t current_char = *(--string_curr_p);
          ecma_number_t current_number = ECMA_NUMBER_MINUS_ONE;

          if ((current_char >= LIT_CHAR_LOWERCASE_A && current_char <= LIT_CHAR_LOWERCASE_Z))
          {
            current_number =  (ecma_number_t) current_char - LIT_CHAR_LOWERCASE_A + 10;
          }
          else if ((current_char >= LIT_CHAR_UPPERCASE_A && current_char <= LIT_CHAR_UPPERCASE_Z))
          {
            current_number =  (ecma_number_t) current_char - LIT_CHAR_UPPERCASE_A + 10;
          }
          else if (lit_char_is_decimal_digit (current_char))
          {
            current_number =  (ecma_number_t) current_char - LIT_CHAR_0;
          }
          else
          {
            JERRY_UNREACHABLE ();
          }

          value += current_number * multiplier;
          multiplier *= (ecma_number_t) rad;
        }

        /* 15. */
        if (sign < 0)
        {
          value *= (ecma_number_t) sign;
        }

        ret_value = ecma_make_number_value (value);
      }

      ECMA_OP_TO_NUMBER_FINALIZE (radix_num);
    }
    else
    {
      ret_value = ecma_make_nan_value ();
    }

  }
  else
  {
    ret_value = ecma_make_nan_value ();
  }

  ECMA_FINALIZE_UTF8_STRING (string_buff, string_buff_size);
  ECMA_FINALIZE (string_var);
  return ret_value;
} /* ecma_builtin_global_object_parse_int */

/**
 * The Global object's 'parseFloat' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_global_object_parse_float (ecma_value_t this_arg, /**< this argument */
                                        ecma_value_t string) /**< routine's first argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (string_var, ecma_op_to_string (string), ret_value);

  ecma_string_t *number_str_p = ecma_get_string_from_value (string_var);
  ECMA_STRING_TO_UTF8_STRING (number_str_p, string_buff, string_buff_size);

  if (string_buff_size > 0)
  {
    const lit_utf8_byte_t *str_curr_p = string_buff;
    const lit_utf8_byte_t *str_end_p = string_buff + string_buff_size;

    const lit_utf8_byte_t *start_p = str_end_p;
    const lit_utf8_byte_t *end_p = str_end_p;

    /* 2. Find first non whitespace char and set starting position. */
    while (str_curr_p < str_end_p)
    {
      ecma_char_t current_char = lit_utf8_read_next (&str_curr_p);

      if (!lit_char_is_white_space (current_char)
          && !lit_char_is_line_terminator (current_char))
      {
        lit_utf8_decr (&str_curr_p);
        start_p = str_curr_p;
        break;
      }
    }

    bool sign = false;
    ecma_char_t current;

    if (str_curr_p < str_end_p)
    {
      /* Check if sign is present. */
      current = *str_curr_p;
      if (current == LIT_CHAR_MINUS)
      {
        sign = true;
      }

      if (current == LIT_CHAR_MINUS || current == LIT_CHAR_PLUS)
      {
        /* Set starting position to be after the sign character. */
        start_p = ++str_curr_p;
      }
    }

    const lit_utf8_byte_t *infinity_str_p = lit_get_magic_string_utf8 (LIT_MAGIC_STRING_INFINITY_UL);
    lit_utf8_byte_t *infinity_str_curr_p = (lit_utf8_byte_t *) infinity_str_p;
    lit_utf8_byte_t *infinity_str_end_p = infinity_str_curr_p + sizeof (*infinity_str_p);

    /* Check if string is equal to "Infinity". */
    while (str_curr_p < str_end_p
           && *str_curr_p++ == *infinity_str_curr_p++)
    {
      if (infinity_str_curr_p == infinity_str_end_p)
      {
        /* String matched Infinity. */
        ret_value = ecma_make_number_value (ecma_number_make_infinity (sign));
        break;
      }
    }

    /* Reset to starting position. */
    str_curr_p = start_p;

    if (ecma_is_value_empty (ret_value) && str_curr_p < str_end_p)
    {
      current = *str_curr_p;

      bool has_whole_part = false;
      bool has_fraction_part = false;

      /* Check digits of whole part. */
      if (lit_char_is_decimal_digit (current))
      {
        has_whole_part = true;
        str_curr_p++;

        while (str_curr_p < str_end_p)
        {
          current = *str_curr_p++;
          if (!lit_char_is_decimal_digit (current))
          {
            str_curr_p--;
            break;
          }
        }
      }


      /* Set end position to the end of whole part. */
      end_p = str_curr_p;
      if (str_curr_p < str_end_p)
      {
        current = *str_curr_p;

        /* Check decimal point. */
        if (current == LIT_CHAR_DOT)
        {
          str_curr_p++;

          if (str_curr_p < str_end_p)
          {
            current = *str_curr_p;

            if (lit_char_is_decimal_digit (current))
            {
              has_fraction_part = true;

              /* Check digits of fractional part. */
              while (str_curr_p < str_end_p)
              {
                current = *str_curr_p++;
                if (!lit_char_is_decimal_digit (current))
                {
                  str_curr_p--;
                  break;
                }
              }

              /* Set end position to end of fraction part. */
              end_p = str_curr_p;
            }
          }
        }
      }


      if (str_curr_p < str_end_p)
      {
        current = *str_curr_p++;
      }

      /* Check exponent. */
      if ((current == LIT_CHAR_LOWERCASE_E || current == LIT_CHAR_UPPERCASE_E)
          && (has_whole_part || has_fraction_part)
          && str_curr_p < str_end_p)
      {
        current = *str_curr_p++;

        /* Check sign of exponent. */
        if ((current == LIT_CHAR_PLUS || current == LIT_CHAR_MINUS)
             && str_curr_p < str_end_p)
        {
          current = *str_curr_p++;
        }

        if (lit_char_is_decimal_digit (current))
        {
          /* Check digits of exponent part. */
          while (str_curr_p < str_end_p)
          {
            current = *str_curr_p++;
            if (!lit_char_is_decimal_digit (current))
            {
              str_curr_p--;
              break;
            }
          }

          /* Set end position to end of exponent part. */
          end_p = str_curr_p;
        }
      }

      /* String did not contain a valid number. */
      if (start_p == end_p)
      {
        ret_value = ecma_make_nan_value ();
      }
      else
      {
        /* 5. */
        ecma_number_t ret_num = ecma_utf8_string_to_number (start_p,
                                                            (lit_utf8_size_t) (end_p - start_p));

        if (sign)
        {
          ret_num *= ECMA_NUMBER_MINUS_ONE;
        }

        ret_value = ecma_make_number_value (ret_num);
      }
    }
    /* String ended after sign character, or was empty after removing leading whitespace. */
    else if (ecma_is_value_empty (ret_value))
    {
      ret_value = ecma_make_nan_value ();
    }
  }
  /* String length is zero. */
  else
  {
    ret_value = ecma_make_nan_value ();
  }

  ECMA_FINALIZE_UTF8_STRING (string_buff, string_buff_size);
  ECMA_FINALIZE (string_var);

  return ret_value;
} /* ecma_builtin_global_object_parse_float */

/**
 * The Global object's 'isNaN' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.2.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_global_object_is_nan (ecma_value_t this_arg, /**< this argument */
                                   ecma_value_t arg) /**< routine's first argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  bool is_nan = ecma_number_is_nan (arg_num);

  ret_value = ecma_make_simple_value (is_nan ? ECMA_SIMPLE_VALUE_TRUE
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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_global_object_is_finite (ecma_value_t this_arg, /**< this argument */
                                      ecma_value_t arg) /**< routine's first argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_OP_TO_NUMBER_TRY_CATCH (arg_num, arg, ret_value);

  bool is_finite = !(ecma_number_is_nan (arg_num)
                     || ecma_number_is_infinity (arg_num));

  ret_value = ecma_make_simple_value (is_finite ? ECMA_SIMPLE_VALUE_TRUE
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
                                            const uint8_t *bitset) /**< character set */
{
  JERRY_ASSERT (character < 128);
  return (bitset[character >> 3] & (1u << (character & 0x7))) != 0;
} /* ecma_builtin_global_object_character_is_in */

/*
 * Unescaped URI characters bitset:
 *   One bit for each character between 0 - 127.
 *   Bit is set if the character is in the unescaped URI set.
 */
static const uint8_t unescaped_uri_set[16] =
{
  0x0, 0x0, 0x0, 0x0, 0xda, 0xff, 0xff, 0xaf,
  0xff, 0xff, 0xff, 0x87, 0xfe, 0xff, 0xff, 0x47
};

/*
 * Unescaped URI component characters bitset:
 *   One bit for each character between 0 - 127.
 *   Bit is set if the character is in the unescaped component URI set.
 */
static const uint8_t unescaped_uri_component_set[16] =
{
  0x0, 0x0, 0x0, 0x0, 0x82, 0x67, 0xff, 0x3,
  0xfe, 0xff, 0xff, 0x87, 0xfe, 0xff, 0xff, 0x47
};

/*
 * Format is a percent sign followed by two hex digits.
 */
#define URI_ENCODED_BYTE_SIZE (3)

/*
 * These two types shows whether the byte is present in
 * the original stream or decoded from a %xx sequence.
 */
#define URI_DECODE_ORIGINAL_BYTE 0
#define URI_DECODE_DECODED_BYTE 1

/**
 * Helper function to decode URI.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_global_object_decode_uri_helper (ecma_value_t uri, /**< uri argument */
                                              const uint8_t *reserved_uri_bitset) /**< reserved characters bitset */
{
  JERRY_UNUSED (uri);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_TRY_CATCH (string,
                  ecma_op_to_string (uri),
                  ret_value);

  JERRY_ASSERT (ecma_is_value_string (string));

  ecma_string_t *input_string_p = ecma_get_string_from_value (string);
  lit_utf8_size_t input_size = ecma_string_get_size (input_string_p);

  JMEM_DEFINE_LOCAL_ARRAY (input_start_p,
                           input_size + 1,
                           lit_utf8_byte_t);

  ecma_string_to_utf8_bytes (input_string_p, input_start_p, input_size);

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

    ecma_char_t decoded_byte;

    if (!lit_read_code_unit_from_hex (input_char_p + 1, 2, &decoded_byte))
    {
      ret_value = ecma_raise_uri_error (ECMA_ERR_MSG ("Invalid hexadecimal value."));
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
    else if ((decoded_byte & LIT_UTF8_4_BYTE_MASK) == LIT_UTF8_4_BYTE_MARKER)
    {
      output_size += 3;
    }
    else
    {
      output_size++;
    }
  }

  if (ecma_is_value_empty (ret_value))
  {
    JMEM_DEFINE_LOCAL_ARRAY (output_start_p,
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

      ecma_char_t decoded_byte;

      if (!lit_read_code_unit_from_hex (input_char_p + 1, 2, &decoded_byte))
      {
        ret_value = ecma_raise_uri_error (ECMA_ERR_MSG ("Invalid hexadecimal value."));
        break;
      }

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
          *output_char_p++ = (lit_utf8_byte_t) decoded_byte;
        }
      }
      else
      {
        uint32_t bytes_count;

        if ((decoded_byte & LIT_UTF8_2_BYTE_MASK) == LIT_UTF8_2_BYTE_MARKER)
        {
          bytes_count = 2;
        }
        else if ((decoded_byte & LIT_UTF8_3_BYTE_MASK) == LIT_UTF8_3_BYTE_MARKER)
        {
          bytes_count = 3;
        }
        else if ((decoded_byte & LIT_UTF8_4_BYTE_MASK) == LIT_UTF8_4_BYTE_MARKER)
        {
          bytes_count = 4;
        }
        else
        {
          ret_value = ecma_raise_uri_error (ECMA_ERR_MSG ("Invalid UTF8 character."));
          break;
        }

        lit_utf8_byte_t octets[LIT_UTF8_MAX_BYTES_IN_CODE_POINT];
        octets[0] = (lit_utf8_byte_t) decoded_byte;
        bool is_valid = true;

        for (uint32_t i = 1; i < bytes_count; i++)
        {
          if (input_char_p >= input_end_p || *input_char_p != '%')
          {
            is_valid = false;
            break;
          }
          else
          {
            ecma_char_t chr;

            if (!lit_read_code_unit_from_hex (input_char_p + 1, 2, &chr)
                || ((chr & LIT_UTF8_EXTRA_BYTE_MASK) != LIT_UTF8_EXTRA_BYTE_MARKER))
            {
              is_valid = false;
              break;
            }

            octets[i] = (lit_utf8_byte_t) chr;
            input_char_p += URI_ENCODED_BYTE_SIZE;
          }
        }

        if (!is_valid
            || !lit_is_valid_utf8_string (octets, bytes_count))
        {
          ret_value = ecma_raise_uri_error (ECMA_ERR_MSG ("Invalid UTF8 string."));
          break;
        }

        lit_code_point_t cp;
        lit_read_code_point_from_utf8 (octets, bytes_count, &cp);

        if (lit_is_code_point_utf16_high_surrogate (cp)
            || lit_is_code_point_utf16_low_surrogate (cp))
        {
          ret_value = ecma_raise_uri_error (ECMA_ERR_MSG ("Invalid UTF8 codepoint."));
          break;
        }

        output_char_p += lit_code_point_to_cesu8 (cp, output_char_p);
      }
    }

    if (ecma_is_value_empty (ret_value))
    {
      JERRY_ASSERT (output_start_p + output_size == output_char_p);

      if (lit_is_valid_cesu8_string (output_start_p, output_size))
      {
        ecma_string_t *output_string_p = ecma_new_ecma_string_from_utf8 (output_start_p, output_size);
        ret_value = ecma_make_string_value (output_string_p);
      }
      else
      {
        ret_value = ecma_raise_uri_error (ECMA_ERR_MSG ("Invalid CESU8 string."));
      }
    }

    JMEM_FINALIZE_LOCAL_ARRAY (output_start_p);
  }

  JMEM_FINALIZE_LOCAL_ARRAY (input_start_p);

  ECMA_FINALIZE (string);
  return ret_value;
} /* ecma_builtin_global_object_decode_uri_helper */

/**
 * The Global object's 'decodeURI' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.3.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_global_object_decode_uri (ecma_value_t this_arg, /**< this argument */
                                       ecma_value_t encoded_uri) /**< routine's first argument */
{
  JERRY_UNUSED (this_arg);
  return ecma_builtin_global_object_decode_uri_helper (encoded_uri, unescaped_uri_set);
} /* ecma_builtin_global_object_decode_uri */

/**
 * The Global object's 'decodeURIComponent' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.3.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_global_object_decode_uri_component (ecma_value_t this_arg, /**< this argument */
                                                 ecma_value_t encoded_uri_component) /**< routine's
                                                                                      *   first argument */
{
  JERRY_UNUSED (this_arg);
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

  dest_p[0] = LIT_CHAR_PERCENT;
  ecma_char_t hex_digit = (ecma_char_t) (byte >> 4);
  dest_p[1] = (lit_utf8_byte_t) ((hex_digit > 9) ? (hex_digit + ('A' - 10)) : (hex_digit + '0'));
  hex_digit = (lit_utf8_byte_t) (byte & 0xf);
  dest_p[2] = (lit_utf8_byte_t) ((hex_digit > 9) ? (hex_digit + ('A' - 10)) : (hex_digit + '0'));
} /* ecma_builtin_global_object_byte_to_hex */

/**
 * Helper function to encode URI.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_global_object_encode_uri_helper (ecma_value_t uri, /**< uri argument */
                                              const uint8_t *unescaped_uri_bitset_p) /**< unescaped bitset */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_TRY_CATCH (string,
                  ecma_op_to_string (uri),
                  ret_value);

  JERRY_ASSERT (ecma_is_value_string (string));

  ecma_string_t *input_string_p = ecma_get_string_from_value (string);
  lit_utf8_size_t input_size = ecma_string_get_size (input_string_p);

  JMEM_DEFINE_LOCAL_ARRAY (input_start_p,
                           input_size,
                           lit_utf8_byte_t);

  ecma_string_to_utf8_bytes (input_string_p, input_start_p, input_size);

  /*
   * The URI encoding has two major phases: first we validate the input,
   * and compute the length of the output, then we encode the input.
   */

  lit_utf8_byte_t *input_char_p = input_start_p;
  const lit_utf8_byte_t *input_end_p = input_start_p + input_size;
  lit_utf8_size_t output_length = 0;
  lit_code_point_t cp;
  ecma_char_t ch;
  lit_utf8_byte_t octets[LIT_UTF8_MAX_BYTES_IN_CODE_POINT];
  memset (octets, LIT_BYTE_NULL, LIT_UTF8_MAX_BYTES_IN_CODE_POINT);

  while (input_char_p < input_end_p)
  {
    /* Input validation, we need to reject stray surrogates. */
    input_char_p += lit_read_code_unit_from_utf8 (input_char_p, &ch);

    if (lit_is_code_point_utf16_low_surrogate (ch))
    {
      ret_value = ecma_raise_uri_error (ECMA_ERR_MSG ("Unicode surrogate pair missing."));
      break;
    }

    cp = ch;

    if (lit_is_code_point_utf16_high_surrogate (ch))
    {
      if (input_char_p == input_end_p)
      {
        ret_value = ecma_raise_uri_error (ECMA_ERR_MSG ("Unicode surrogate pair missing."));
        break;
      }

      ecma_char_t next_ch;
      lit_utf8_size_t read_size = lit_read_code_unit_from_utf8 (input_char_p, &next_ch);

      if (lit_is_code_point_utf16_low_surrogate (next_ch))
      {
        cp = lit_convert_surrogate_pair_to_code_point (ch, next_ch);
        input_char_p += read_size;
      }
      else
      {
        ret_value = ecma_raise_uri_error (ECMA_ERR_MSG ("Unicode surrogate pair missing."));
        break;
      }
    }

    lit_utf8_size_t utf_size = lit_code_point_to_utf8 (cp, octets);

    if (utf_size == 1)
    {
      if (ecma_builtin_global_object_character_is_in (octets[0], unescaped_uri_bitset_p))
      {
        output_length++;
      }
      else
      {
        output_length += URI_ENCODED_BYTE_SIZE;
      }
    }
    else
    {
      output_length += utf_size * URI_ENCODED_BYTE_SIZE;
    }
  }

  if (ecma_is_value_empty (ret_value))
  {
    JMEM_DEFINE_LOCAL_ARRAY (output_start_p,
                             output_length,
                             lit_utf8_byte_t);

    lit_utf8_byte_t *output_char_p = output_start_p;
    input_char_p = input_start_p;

    while (input_char_p < input_end_p)
    {
      /* Input decode. */
      input_char_p += lit_read_code_unit_from_utf8 (input_char_p, &ch);
      cp = ch;

      if (lit_is_code_point_utf16_high_surrogate (ch))
      {
        ecma_char_t next_ch;
        lit_utf8_size_t read_size = lit_read_code_unit_from_utf8 (input_char_p, &next_ch);

        if (lit_is_code_point_utf16_low_surrogate (next_ch))
        {
          cp = lit_convert_surrogate_pair_to_code_point (ch, next_ch);
          input_char_p += read_size;
        }
      }

      lit_utf8_size_t utf_size = lit_code_point_to_utf8 (cp, octets);

      if (utf_size == 1)
      {
        if (ecma_builtin_global_object_character_is_in (octets[0], unescaped_uri_bitset_p))
        {
          *output_char_p++ = octets[0];
        }
        else
        {
          ecma_builtin_global_object_byte_to_hex (output_char_p, octets[0]);
          output_char_p += URI_ENCODED_BYTE_SIZE;
        }
      }
      else
      {
        for (uint32_t i = 0; i < utf_size; i++)
        {
          ecma_builtin_global_object_byte_to_hex (output_char_p, octets[i]);
          output_char_p += URI_ENCODED_BYTE_SIZE;
        }
      }
    }

    JERRY_ASSERT (output_start_p + output_length == output_char_p);

    ecma_string_t *output_string_p = ecma_new_ecma_string_from_utf8 (output_start_p, output_length);

    ret_value = ecma_make_string_value (output_string_p);

    JMEM_FINALIZE_LOCAL_ARRAY (output_start_p);
  }

  JMEM_FINALIZE_LOCAL_ARRAY (input_start_p);

  ECMA_FINALIZE (string);
  return ret_value;
} /* ecma_builtin_global_object_encode_uri_helper */

/**
 * The Global object's 'encodeURI' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.3.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_global_object_encode_uri (ecma_value_t this_arg, /**< this argument */
                                       ecma_value_t uri) /**< routine's first argument */
{
  JERRY_UNUSED (this_arg);
  return ecma_builtin_global_object_encode_uri_helper (uri, unescaped_uri_set);
} /* ecma_builtin_global_object_encode_uri */

/**
 * The Global object's 'encodeURIComponent' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.3.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_global_object_encode_uri_component (ecma_value_t this_arg, /**< this argument */
                                                 ecma_value_t uri_component) /**< routine's first argument */
{
  JERRY_UNUSED (this_arg);
  return ecma_builtin_global_object_encode_uri_helper (uri_component, unescaped_uri_component_set);
} /* ecma_builtin_global_object_encode_uri_component */

#ifndef CONFIG_DISABLE_ANNEXB_BUILTIN

/*
 * Maximum value of a byte.
 */
#define ECMA_ESCAPE_MAXIMUM_BYTE_VALUE (255)

/*
 * Format is a percent sign followed by lowercase u and four hex digits.
 */
#define ECMA_ESCAPE_ENCODED_UNICODE_CHARACTER_SIZE (6)

/*
 * Escape characters bitset:
 *   One bit for each character between 0 - 127.
 *   Bit is set if the character does not need to be converted to %xx form.
 *   These characters are: a-z A-Z 0-9 @ * _ + - . /
 */
static const uint8_t ecma_escape_set[16] =
{
  0x0, 0x0, 0x0, 0x0, 0x0, 0xec, 0xff, 0x3,
  0xff, 0xff, 0xff, 0x87, 0xfe, 0xff, 0xff, 0x7
};

/**
 * The Global object's 'escape' routine
 *
 * See also:
 *          ECMA-262 v5, B.2.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_global_object_escape (ecma_value_t this_arg, /**< this argument */
                                   ecma_value_t arg) /**< routine's first argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_TRY_CATCH (string,
                  ecma_op_to_string (arg),
                  ret_value);

  ecma_string_t *input_string_p = ecma_get_string_from_value (string);
  lit_utf8_size_t input_size = ecma_string_get_size (input_string_p);

  JMEM_DEFINE_LOCAL_ARRAY (input_start_p,
                           input_size,
                           lit_utf8_byte_t);

  ecma_string_to_utf8_bytes (input_string_p, input_start_p, input_size);

  /*
   * The escape routine has two major phases: first we compute
   * the length of the output, then we encode the input.
   */
  const lit_utf8_byte_t *input_curr_p = input_start_p;
  const lit_utf8_byte_t *input_end_p = input_start_p + input_size;
  lit_utf8_size_t output_length = 0;

  while (input_curr_p < input_end_p)
  {
    ecma_char_t chr = lit_utf8_read_next (&input_curr_p);

    if (chr <= LIT_UTF8_1_BYTE_CODE_POINT_MAX)
    {
      if (ecma_builtin_global_object_character_is_in ((uint32_t) chr, ecma_escape_set))
      {
        output_length++;
      }
      else
      {
        output_length += URI_ENCODED_BYTE_SIZE;
      }
    }
    else if (chr > ECMA_ESCAPE_MAXIMUM_BYTE_VALUE)
    {
      output_length += ECMA_ESCAPE_ENCODED_UNICODE_CHARACTER_SIZE;
    }
    else
    {
      output_length += URI_ENCODED_BYTE_SIZE;
    }
  }

  JMEM_DEFINE_LOCAL_ARRAY (output_start_p,
                           output_length,
                           lit_utf8_byte_t);

  lit_utf8_byte_t *output_char_p = output_start_p;

  input_curr_p = input_start_p;

  while (input_curr_p < input_end_p)
  {
    ecma_char_t chr = lit_utf8_read_next (&input_curr_p);

    if (chr <= LIT_UTF8_1_BYTE_CODE_POINT_MAX)
    {
      if (ecma_builtin_global_object_character_is_in ((uint32_t) chr, ecma_escape_set))
      {
        *output_char_p = (lit_utf8_byte_t) chr;
        output_char_p++;
      }
      else
      {
        ecma_builtin_global_object_byte_to_hex (output_char_p, (lit_utf8_byte_t) chr);
        output_char_p += URI_ENCODED_BYTE_SIZE;
      }
    }
    else if (chr > ECMA_ESCAPE_MAXIMUM_BYTE_VALUE)
    {
      /*
       * Although ecma_builtin_global_object_byte_to_hex inserts a percent (%) sign
       * the follow-up changes overwrites it. We call this function twice to
       * produce four hexadecimal characters (%uxxxx format).
       */
      ecma_builtin_global_object_byte_to_hex (output_char_p + 3, (lit_utf8_byte_t) (chr & 0xff));
      ecma_builtin_global_object_byte_to_hex (output_char_p + 1, (lit_utf8_byte_t) (chr >> JERRY_BITSINBYTE));
      output_char_p[0] = LIT_CHAR_PERCENT;
      output_char_p[1] = LIT_CHAR_LOWERCASE_U;
      output_char_p += ECMA_ESCAPE_ENCODED_UNICODE_CHARACTER_SIZE;
    }
    else
    {
      ecma_builtin_global_object_byte_to_hex (output_char_p, (lit_utf8_byte_t) chr);
      output_char_p += URI_ENCODED_BYTE_SIZE;
    }
  }

  JERRY_ASSERT (output_start_p + output_length == output_char_p);

  ecma_string_t *output_string_p = ecma_new_ecma_string_from_utf8 (output_start_p, output_length);

  ret_value = ecma_make_string_value (output_string_p);

  JMEM_FINALIZE_LOCAL_ARRAY (output_start_p);

  JMEM_FINALIZE_LOCAL_ARRAY (input_start_p);

  ECMA_FINALIZE (string);
  return ret_value;
} /* ecma_builtin_global_object_escape */

/**
 * The Global object's 'unescape' routine
 *
 * See also:
 *          ECMA-262 v5, B.2.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_global_object_unescape (ecma_value_t this_arg, /**< this argument */
                                     ecma_value_t arg) /**< routine's first argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  /* 1. */
  ECMA_TRY_CATCH (string, ecma_op_to_string (arg), ret_value);
  ecma_string_t *input_string_p = ecma_get_string_from_value (string);
  /* 2. */
  lit_utf8_size_t input_size = ecma_string_get_size (input_string_p);

  /* 3. */
  JMEM_DEFINE_LOCAL_ARRAY (input_start_p, input_size, lit_utf8_byte_t);
  ecma_string_to_utf8_bytes (input_string_p, input_start_p, input_size);

  const lit_utf8_byte_t *input_curr_p = input_start_p;
  const lit_utf8_byte_t *input_end_p = input_start_p + input_size;
  /* 4. */
  /* The length of input string is always greater than output string
   * so we re-use the input string buffer.
   * The %xx is three byte long, and the maximum encoded value is 0xff,
   * which maximum encoded length is two byte. Similar to this, the maximum
   * encoded length of %uxxxx is four byte. */
  lit_utf8_byte_t *output_char_p = input_start_p;

  /* The state of parsing that tells us where we are in an escape pattern.
   * 0    we are outside of pattern,
   * 1    found '%', start of pattern,
   * 2    found first hex digit of '%xy' pattern
   * 3    found valid '%xy' pattern
   * 4    found 'u', start of '%uwxyz' pattern
   * 5-7  found hex digits of '%uwxyz' pattern
   * 8    found valid '%uwxyz' pattern
   */
  uint8_t status = 0;
  ecma_char_t hex_digits = 0;
  /* 5. */
  while (input_curr_p < input_end_p)
  {
    /* 6. */
    ecma_char_t chr = lit_utf8_read_next (&input_curr_p);

    /* 7-8. */
    if (status == 0 && chr == LIT_CHAR_PERCENT)
    {
      /* Found '%' char, start of escape sequence. */
      status = 1;
    }
    /* 9-10. */
    else if (status == 1 && chr == LIT_CHAR_LOWERCASE_U)
    {
      /* Found 'u' char after '%'. */
      status = 4;
    }
    else if (status > 0 && lit_char_is_hex_digit (chr))
    {
      /* Found hexadecimal digit in escape sequence. */
      hex_digits = (ecma_char_t) (hex_digits * 16 + (ecma_char_t) lit_char_hex_to_int (chr));
      status++;
    }

    /* 11-17. Found valid '%uwxyz' or '%xy' escape. */
    if (status == 8 || status == 3)
    {
      output_char_p -= (status == 3) ? 2 : 5;
      status = 0;
      chr = hex_digits;
      hex_digits = 0;
    }

    /* Copying character. */
    lit_utf8_size_t lit_size = lit_code_unit_to_utf8 (chr, output_char_p);
    output_char_p += lit_size;
    JERRY_ASSERT (output_char_p <= input_curr_p);
  }

  lit_utf8_size_t output_length = (lit_utf8_size_t) (output_char_p - input_start_p);
  ecma_string_t *output_string_p = ecma_new_ecma_string_from_utf8 (input_start_p, output_length);
  ret_value = ecma_make_string_value (output_string_p);

  JMEM_FINALIZE_LOCAL_ARRAY (input_start_p);

  ECMA_FINALIZE (string);
  return ret_value;
} /* ecma_builtin_global_object_unescape */

#endif /* !CONFIG_DISABLE_ANNEXB_BUILTIN */

/**
 * @}
 * @}
 * @}
 */
