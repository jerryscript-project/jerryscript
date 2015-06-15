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
ecma_builtin_global_object_parse_int (ecma_value_t this_arg __attr_unused___, /**< this argument */
                                      ecma_value_t string, /**< routine's first argument */
                                      ecma_value_t radix) /**< routine's second argument */
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

  /* 2. Remove leading whitespace. */
  int32_t start = string_len;
  int32_t end = string_len;
  for (int i = 0; i < end; i++)
  {
    if (!(isspace (zt_string_buff[i])))
    {
      start = i;
      break;
    }
  }

  /* 3. */
  int sign = 1;

  /* 4. */
  if (zt_string_buff[start] == '-')
  {
    sign = -1;
  }

  /* 5. */
  if (zt_string_buff[start] == '-' || zt_string_buff[start] == '+')
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
          && zt_string_buff[start] == '0'
          && (zt_string_buff[start + 1] == 'x' || zt_string_buff[start + 1] == 'X'))
      {
        start += 2;

        rad = 16;
      }
    }

    /* 11. Check if characters are in [0, Radix - 1]. We also convert them to number values in the process. */
    for (int i = start; i < end; i++)
    {
      if ((zt_string_buff[i]) >= 'a' && zt_string_buff[i] <= 'z')
      {
        zt_string_buff[i] = (ecma_char_t) (zt_string_buff[i] - 'a' + 10);
      }
      else if (zt_string_buff[i] >= 'A' && zt_string_buff[i] <= 'Z')
      {
        zt_string_buff[i] = (ecma_char_t) (zt_string_buff[i] - 'A' + 10);
      }
      else if (isdigit (zt_string_buff[i]))
      {
        zt_string_buff[i] = (ecma_char_t) (zt_string_buff[i] - '0');
      }
      else
      {
        /* Not a valid number char, set value to radix so it fails to pass as a valid character. */
        zt_string_buff[i] = (ecma_char_t) rad;
      }

      if (!(zt_string_buff[i] < rad))
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
    for (int i = end - 1; i >= start; i--)
    {
      *value_p += (ecma_number_t) zt_string_buff[i] * multiplier;
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
  MEM_FINALIZE_LOCAL_ARRAY (zt_string_buff);
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
ecma_builtin_global_object_parse_float (ecma_value_t this_arg, /**< this argument */
                                        ecma_value_t string) /**< routine's first argument */
{
  ECMA_BUILTIN_CP_UNIMPLEMENTED (this_arg, string);
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
