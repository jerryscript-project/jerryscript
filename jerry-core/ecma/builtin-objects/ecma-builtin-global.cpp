/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"
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
ecma_builtin_global_object_eval (ecma_value_t this_arg, /**< this argument */
                                 ecma_value_t x) /**< routine's first argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, x);
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
ecma_builtin_global_object_parse_int (ecma_value_t this_arg, /**< this argument */
                                      ecma_value_t string, /**< routine's first argument */
                                      ecma_value_t radix) /**< routine's second argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, string, radix);
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
  int32_t string_len = ecma_string_get_length (number_str_p);

  MEM_DEFINE_LOCAL_ARRAY (zt_string_buff, string_len + 1, ecma_char_t);

  size_t string_buf_size = (size_t) (string_len + 1) * sizeof (ecma_char_t);
  ssize_t bytes_copied = ecma_string_to_zt_string (number_str_p,
                                                   zt_string_buff,
                                                   (ssize_t) string_buf_size);
  JERRY_ASSERT (bytes_copied > 0);

  /* 2. Find first non whitespace char. */
  int32_t start = 0;
  for (int i = 0; i < string_len; i++)
  {
    if (!isspace (zt_string_buff[i]))
    {
      start = i;
      break;
    }
  }

  bool sign = false;

  /* Check if sign is present. */
  if (zt_string_buff[start] == '-')
  {
    sign = true;
    start++;
  }
  else if (zt_string_buff[start] == '+')
  {
    start++;
  }

  ecma_number_t *ret_num_p = ecma_alloc_number ();

  /* Check if string is equal to "Infinity". */
  const ecma_char_t *infinity_zt_str_p = ecma_get_magic_string_zt (ECMA_MAGIC_STRING_INFINITY_UL);

  for (int i = 0; infinity_zt_str_p[i] == zt_string_buff[start + i]; i++)
  {
    if (infinity_zt_str_p[i + 1] == 0)
    {
      *ret_num_p = ecma_number_make_infinity (sign);
      ret_value = ecma_make_normal_completion_value (ecma_make_number_value (ret_num_p));
      break;
    }
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    int32_t current = start;
    int32_t end = string_len;
    bool has_whole_part = false;
    bool has_fraction_part = false;

    if (isdigit (zt_string_buff[current]))
    {
      has_whole_part = true;

      /* Check digits of whole part. */
      for (int i = current; i < string_len; i++, current++)
      {
        if (!isdigit (zt_string_buff[current]))
        {
          break;
        }
      }
    }

    end = current;

    /* Check decimal point. */
    if (zt_string_buff[current] == '.')
    {
      current++;

      if (isdigit (zt_string_buff[current]))
      {
        has_fraction_part = true;

        /* Check digits of fractional part. */
        for (int i = current; i < string_len; i++, current++)
        {
          if (!isdigit (zt_string_buff[current]))
          {
            break;
          }
        }

        end = current;
      }
    }

    /* Check exponent. */
    if ((zt_string_buff[current] == 'e' || zt_string_buff[current] == 'E')
        && (has_whole_part || has_fraction_part))
    {
      current++;

      /* Check sign of exponent. */
      if (zt_string_buff[current] == '-' || zt_string_buff[current] == '+')
      {
        current++;
      }

      if (isdigit (zt_string_buff[current]))
      {

        /* Check digits of exponent part. */
        for (int i = current; i < string_len; i++, current++)
        {
          if (!isdigit (zt_string_buff[current]))
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
      if (end < string_len)
      {
        /* 4. End of valid number, terminate the string. */
        zt_string_buff[end] = '\0';
      }

      /* 5. */
      *ret_num_p = ecma_zt_string_to_number (zt_string_buff + start);

      if (sign)
      {
        *ret_num_p *= -1;
      }

      ret_value = ecma_make_normal_completion_value (ecma_make_number_value (ret_num_p));
    }
  }

  MEM_FINALIZE_LOCAL_ARRAY (zt_string_buff);
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
 * The Global object's 'decodeURI' routine
 *
 * See also:
 *          ECMA-262 v5, 15.1.3.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_global_object_decode_uri (ecma_value_t this_arg, /**< this argument */
                                       ecma_value_t encoded_uri) /**< routine's first argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, encoded_uri);
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
ecma_builtin_global_object_decode_uri_component (ecma_value_t this_arg, /**< this argument */
                                                 ecma_value_t encoded_uri_component) /**< routine's
                                                                                      *   first argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, encoded_uri_component);
} /* ecma_builtin_global_object_decode_uri_component */

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
ecma_builtin_global_object_encode_uri (ecma_value_t this_arg, /**< this argument */
                                       ecma_value_t uri) /**< routine's first argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, uri);
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
ecma_builtin_global_object_encode_uri_component (ecma_value_t this_arg, /**< this argument */
                                                 ecma_value_t uri_component) /**< routine's first argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, uri_component);
} /* ecma_builtin_global_object_encode_uri_component */

/**
 * @}
 * @}
 * @}
 */
