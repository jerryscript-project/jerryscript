/* Copyright 2015 Samsung Electronics Co., Ltd.
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
#include "ecma-builtin-helpers.h"
#include "ecma-conversion.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-try-catch-macro.h"
#include "lit-char-helpers.h"

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-date.inc.h"
#define BUILTIN_UNDERSCORED_ID date
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup date ECMA Date object built-in
 * @{
 */

/**
 * Helper function to try to parse a part of a date string
 *
 * @return NaN if cannot read from string, ToNumber() otherwise
 */
static ecma_number_t
ecma_date_parse_date_chars (lit_utf8_iterator_t *iter, /**< iterator of the utf8 string */
                            uint32_t num_of_chars) /**< number of characters to read and convert */
{
  JERRY_ASSERT (num_of_chars > 0);

  lit_utf8_size_t copy_size = 0;
  const lit_utf8_byte_t *str_start_p = iter->buf_p + iter->buf_pos.offset;

  while (num_of_chars--)
  {
    if (lit_utf8_iterator_is_eos (iter)
        || !lit_char_is_decimal_digit (lit_utf8_iterator_peek_next (iter)))
    {
      return ecma_number_make_nan ();
    }

    copy_size += lit_get_unicode_char_size_by_utf8_first_byte (*(iter->buf_p + iter->buf_pos.offset));
    lit_utf8_iterator_incr (iter);
  }

  return ecma_utf8_string_to_number (str_start_p, copy_size);
} /* ecma_date_parse_date_chars */

/**
  * Calculate MakeDate(MakeDay(yr, m, dt), MakeTime(h, min, s, milli)) for Date constructor and UTC
  *
  * See also:
  *          ECMA-262 v5, 15.9.3.1
  *          ECMA-262 v5, 15.9.4.3
  *
  * @return result of MakeDate(MakeDay(yr, m, dt), MakeTime(h, min, s, milli))
  */
static ecma_completion_value_t
ecma_date_construct_helper (const ecma_value_t *args, /**< arguments passed to the Date constructor */
                            ecma_length_t args_len) /**< number of arguments */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  ecma_number_t *prim_value_p = ecma_alloc_number ();
  *prim_value_p = ecma_number_make_nan ();

  ECMA_TRY_CATCH (year_value, ecma_op_to_number (args[0]), ret_value);
  ECMA_TRY_CATCH (month_value, ecma_op_to_number (args[1]), ret_value);

  ecma_number_t year = *ecma_get_number_from_value (year_value);
  ecma_number_t month = *ecma_get_number_from_value (month_value);
  ecma_number_t date = ECMA_NUMBER_ONE;
  ecma_number_t hours = ECMA_NUMBER_ZERO;
  ecma_number_t minutes = ECMA_NUMBER_ZERO;
  ecma_number_t seconds = ECMA_NUMBER_ZERO;
  ecma_number_t milliseconds = ECMA_NUMBER_ZERO;

  /* 3. */
  if (args_len >= 3 && ecma_is_completion_value_empty (ret_value))
  {
    ECMA_TRY_CATCH (date_value, ecma_op_to_number (args[2]), ret_value);
    date = *ecma_get_number_from_value (date_value);
    ECMA_FINALIZE (date_value);
  }

  /* 4. */
  if (args_len >= 4 && ecma_is_completion_value_empty (ret_value))
  {
    ECMA_TRY_CATCH (hours_value, ecma_op_to_number (args[3]), ret_value);
    hours = *ecma_get_number_from_value (hours_value);
    ECMA_FINALIZE (hours_value);
  }

  /* 5. */
  if (args_len >= 5 && ecma_is_completion_value_empty (ret_value))
  {
    ECMA_TRY_CATCH (minutes_value, ecma_op_to_number (args[4]), ret_value);
    minutes = *ecma_get_number_from_value (minutes_value);
    ECMA_FINALIZE (minutes_value);
  }

  /* 6. */
  if (args_len >= 6 && ecma_is_completion_value_empty (ret_value))
  {
    ECMA_TRY_CATCH (seconds_value, ecma_op_to_number (args[5]), ret_value);
    seconds = *ecma_get_number_from_value (seconds_value);
    ECMA_FINALIZE (seconds_value);
  }

  /* 7. */
  if (args_len >= 7 && ecma_is_completion_value_empty (ret_value))
  {
    ECMA_TRY_CATCH (milliseconds_value, ecma_op_to_number (args[6]), ret_value);
    milliseconds = *ecma_get_number_from_value (milliseconds_value);
    ECMA_FINALIZE (milliseconds_value);
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    if (!ecma_number_is_nan (year))
    {
      /* 8. */
      int32_t y = ecma_number_to_int32 (year);

      if (y >= 0 && y <= 99)
      {
        year = (ecma_number_t) (1900 + y);
      }
      else
      {
        year = (ecma_number_t) y;
      }
    }

    *prim_value_p = ecma_date_make_date (ecma_date_make_day (year,
                                                             month,
                                                             date),
                                         ecma_date_make_time (hours,
                                                              minutes,
                                                              seconds,
                                                              milliseconds));
  }

  ECMA_FINALIZE (month_value);
  ECMA_FINALIZE (year_value);

  if (ecma_is_completion_value_empty (ret_value))
  {
    ret_value = ecma_make_normal_completion_value (ecma_make_number_value (prim_value_p));
  }

  return ret_value;
} /* ecma_date_construct_helper */

/**
 * The Date object's 'parse' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.4.2
 *          ECMA-262 v5, 15.9.1.15
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_parse (ecma_value_t this_arg __attr_unused___, /**< this argument */
                         ecma_value_t arg) /**< string */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  ecma_number_t *date_num_p = ecma_alloc_number ();
  *date_num_p = ecma_number_make_nan ();

  /* Date Time String fromat (ECMA-262 v5, 15.9.1.15) */
  ECMA_TRY_CATCH (date_str_value,
                  ecma_op_to_string (arg),
                  ret_value);

  ecma_string_t *date_str_p = ecma_get_string_from_value (date_str_value);

  lit_utf8_size_t date_str_size = ecma_string_get_size (date_str_p);
  MEM_DEFINE_LOCAL_ARRAY (date_start_p, date_str_size, lit_utf8_byte_t);

  ecma_string_to_utf8_string (date_str_p, date_start_p, (ssize_t) date_str_size);
  lit_utf8_iterator_t iter = lit_utf8_iterator_create (date_start_p, date_str_size);

  /* 1. read year */
  ecma_number_t year = ecma_date_parse_date_chars (&iter, 4);

  if (!ecma_number_is_nan (year)
      && year >= 0)
  {
    ecma_number_t month = ECMA_NUMBER_ONE;
    ecma_number_t day = ECMA_NUMBER_ONE;
    ecma_number_t time = ECMA_NUMBER_ZERO;

    /* 2. read month if any */
    if (!lit_utf8_iterator_is_eos (&iter)
        && lit_utf8_iterator_peek_next (&iter) == '-')
    {
      /* eat up '-' */
      lit_utf8_iterator_incr (&iter);
      month = ecma_date_parse_date_chars (&iter, 2);

      if (month > 12 || month < 1)
      {
        month = ecma_number_make_nan ();
      }
    }

    /* 3. read day if any */
    if (!lit_utf8_iterator_is_eos (&iter)
        && lit_utf8_iterator_peek_next (&iter) == '-')
    {
      /* eat up '-' */
      lit_utf8_iterator_incr (&iter);
      day = ecma_date_parse_date_chars (&iter, 2);

      if (day < 1 || day > 31)
      {
        day = ecma_number_make_nan ();
      }
    }

    /* 4. read time if any */
    if (!lit_utf8_iterator_is_eos (&iter)
        && lit_utf8_iterator_peek_next (&iter) == 'T')
    {
      ecma_number_t hours = ECMA_NUMBER_ZERO;
      ecma_number_t minutes = ECMA_NUMBER_ZERO;
      ecma_number_t seconds = ECMA_NUMBER_ZERO;
      ecma_number_t milliseconds = ECMA_NUMBER_ZERO;

      ecma_length_t num_of_visited_chars = lit_utf8_iterator_get_index (&iter);
      ecma_length_t date_str_len = lit_utf8_string_length (iter.buf_p, iter.buf_size) - 1;

      if ((date_str_len - num_of_visited_chars) >= 5)
      {
        /* eat up 'T' */
        lit_utf8_iterator_incr (&iter);

        /* 4.1 read hours and minutes */
        hours = ecma_date_parse_date_chars (&iter, 2);

        if (hours < 0 || hours > 24)
        {
          hours = ecma_number_make_nan ();
        }
        else if (hours == 24)
        {
          hours = ECMA_NUMBER_ZERO;
        }

        /* eat up ':' */
        lit_utf8_iterator_incr (&iter);

        minutes = ecma_date_parse_date_chars (&iter, 2);

        if (minutes < 0 || minutes > 59)
        {
          minutes = ecma_number_make_nan ();
        }

        /* 4.2 read seconds if any */
        if (!lit_utf8_iterator_is_eos (&iter) && lit_utf8_iterator_peek_next (&iter) == ':')
        {
          /* eat up ':' */
          lit_utf8_iterator_incr (&iter);
          seconds = ecma_date_parse_date_chars (&iter, 2);

          if (seconds < 0 || seconds > 59)
          {
            seconds = ecma_number_make_nan ();
          }

          /* 4.3 read milliseconds if any */
          if (!lit_utf8_iterator_is_eos (&iter) && lit_utf8_iterator_peek_next (&iter) == '.')
          {
            /* eat up '.' */
            lit_utf8_iterator_incr (&iter);
            milliseconds = ecma_date_parse_date_chars (&iter, 3);

            if (milliseconds < 0)
            {
              milliseconds = ecma_number_make_nan ();
            }
          }
        }

        time = ecma_date_make_time (hours, minutes, seconds, milliseconds);
      }
      else
      {
        time = ecma_number_make_nan ();
      }

      /* 4.4 read timezone if any */
      if (!lit_utf8_iterator_is_eos (&iter)
          && lit_utf8_iterator_peek_next (&iter) == 'Z'
          && !ecma_number_is_nan (time))
      {
        lit_utf8_iterator_incr (&iter);
        time = ecma_date_utc (ecma_date_make_time (hours,
                                                   minutes,
                                                   seconds,
                                                   milliseconds));
      }
      else if (!lit_utf8_iterator_is_eos (&iter)
               && (lit_utf8_iterator_peek_next (&iter) == '+'
                   || lit_utf8_iterator_peek_next (&iter) == '-'))
      {
        ecma_length_t num_of_visited_chars = lit_utf8_iterator_get_index (&iter);
        ecma_length_t date_str_len = lit_utf8_string_length (iter.buf_p, iter.buf_size) - 1;

        if ((date_str_len - num_of_visited_chars) == 5)
        {
          bool is_negative = false;

          if (lit_utf8_iterator_peek_next (&iter) == '-')
          {
            is_negative = true;
          }

          /* eat up '+/-' */
          lit_utf8_iterator_incr (&iter);

          /* read hours and minutes */
          hours = ecma_date_parse_date_chars (&iter, 2);

          if (hours < 0 || hours > 24)
          {
            hours = ecma_number_make_nan ();
          }
          else if (hours == 24)
          {
            hours = ECMA_NUMBER_ZERO;
          }

          /* eat up ':' */
          lit_utf8_iterator_incr (&iter);

          minutes = ecma_date_parse_date_chars (&iter, 2);

          if (minutes < 0 || minutes > 59)
          {
            minutes = ecma_number_make_nan ();
          }

          if (is_negative)
          {
            time += ecma_date_make_time (hours, minutes, ECMA_NUMBER_ZERO, ECMA_NUMBER_ZERO);
          }
          else
          {
            time -= ecma_date_make_time (hours, minutes, ECMA_NUMBER_ZERO, ECMA_NUMBER_ZERO);
          }
        }
      }
    }

    if (lit_utf8_iterator_is_eos (&iter))
    {
      ecma_number_t date = ecma_date_make_day (year, month - 1, day);
      *date_num_p = ecma_date_make_date (date, time);
    }
  }

  ret_value = ecma_make_normal_completion_value (ecma_make_number_value (date_num_p));

  MEM_FINALIZE_LOCAL_ARRAY (date_start_p);
  ECMA_FINALIZE (date_str_value);

  return ret_value;
} /* ecma_builtin_date_parse */

/**
 * The Date object's 'UTC' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.4.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_utc (ecma_value_t this_arg __attr_unused___, /**< this argument */
                       const ecma_value_t args[], /**< arguments list */
                       ecma_length_t args_number) /**< number of arguments */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (args_number < 2)
  {
    /* Note:
     *      When the UTC function is called with fewer than two arguments,
     *      the behaviour is implementation-dependent, so just return NaN.
     */
    ecma_number_t *nan_p = ecma_alloc_number ();
    *nan_p = ecma_number_make_nan ();
    return ecma_make_normal_completion_value (ecma_make_number_value (nan_p));
  }

  ECMA_TRY_CATCH (time_value, ecma_date_construct_helper (args, args_number), ret_value);

  ecma_number_t *time_p = ecma_get_number_from_value (time_value);
  ecma_number_t *time_clip_p = ecma_alloc_number ();
  *time_clip_p = ecma_date_time_clip (*time_p);
  ret_value = ecma_make_normal_completion_value (ecma_make_number_value (time_clip_p));

  ECMA_FINALIZE (time_value);

  return ret_value;
} /* ecma_builtin_date_utc */

/**
 * The Date object's 'now' routine
 *
 * See also:
 *          ECMA-262 v5, 15.9.4.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_date_now (ecma_value_t this_arg __attr_unused___) /**< this argument */
{
  /*
   * FIXME:
   *        Get the real system time. ex: gettimeofday() on Linux
   *        Introduce system macros at first.
   */
  ecma_number_t *now_num_p = ecma_alloc_number ();
  *now_num_p = ECMA_NUMBER_ZERO;
  return ecma_make_normal_completion_value (ecma_make_number_value (now_num_p));
} /* ecma_builtin_date_now */

/**
 * Handle calling [[Call]] of built-in Date object
 *
 * See also:
 *          ECMA-262 v5, 15.9.2.1
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_date_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                 ecma_length_t arguments_list_len) /**< number of arguments */
{
  /* FIXME:
   *       Fix this, after Date.prototype.toString is finished.
   */
  return ecma_builtin_date_dispatch_construct (arguments_list_p, arguments_list_len);
} /* ecma_builtin_date_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in Date object
 *
 * See also:
 *          ECMA-262 v5, 15.9.3.1
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_date_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                      ecma_length_t arguments_list_len) /**< number of arguments */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  ecma_number_t *prim_value_num_p = NULL;

  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_DATE_PROTOTYPE);
  ecma_object_t *obj_p = ecma_create_object (prototype_obj_p,
                                             true,
                                             ECMA_OBJECT_TYPE_GENERAL);
  ecma_deref_object (prototype_obj_p);

  if (arguments_list_len == 0)
  {
    ECMA_TRY_CATCH (parse_res_value,
                    ecma_builtin_date_now (ecma_make_object_value (obj_p)),
                    ret_value);

    prim_value_num_p = ecma_alloc_number ();
    *prim_value_num_p = *ecma_get_number_from_value (parse_res_value);

    ECMA_FINALIZE (parse_res_value)
  }
  else if (arguments_list_len == 1)
  {
    ECMA_TRY_CATCH (prim_comp_value,
                    ecma_op_to_primitive (arguments_list_p[0], ECMA_PREFERRED_TYPE_NUMBER),
                    ret_value);

    if (ecma_is_value_string (prim_comp_value))
    {
      ECMA_TRY_CATCH (parse_res_value,
                      ecma_builtin_date_parse (ecma_make_object_value (obj_p), prim_comp_value),
                      ret_value);

      prim_value_num_p = ecma_alloc_number ();
      *prim_value_num_p = *ecma_get_number_from_value (parse_res_value);

      ECMA_FINALIZE (parse_res_value);
    }
    else
    {
      ECMA_TRY_CATCH (prim_value, ecma_op_to_number (arguments_list_p[0]), ret_value);

      prim_value_num_p = ecma_alloc_number ();
      *prim_value_num_p = *ecma_get_number_from_value (prim_value);

      ECMA_FINALIZE (prim_value);
    }

    ECMA_FINALIZE (prim_comp_value);
  }
  else if (arguments_list_len >= 2)
  {
    ECMA_TRY_CATCH (time_value,
                    ecma_date_construct_helper (arguments_list_p, arguments_list_len),
                    ret_value);

    ecma_number_t *time_p = ecma_get_number_from_value (time_value);
    prim_value_num_p = ecma_alloc_number ();
    *prim_value_num_p = ecma_date_time_clip (ecma_date_utc (*time_p));

    ECMA_FINALIZE (time_value);
  }
  else
  {
    prim_value_num_p = ecma_alloc_number ();
    *prim_value_num_p = ecma_number_make_nan ();
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    if (!ecma_number_is_nan (*prim_value_num_p) && ecma_number_is_infinity (*prim_value_num_p))
    {
      *prim_value_num_p = ecma_number_make_nan ();
    }

    ecma_property_t *class_prop_p = ecma_create_internal_property (obj_p,
                                                                   ECMA_INTERNAL_PROPERTY_CLASS);
    class_prop_p->u.internal_property.value = LIT_MAGIC_STRING_DATE_UL;

    ecma_property_t *prim_value_prop_p = ecma_create_internal_property (obj_p,
                                                                        ECMA_INTERNAL_PROPERTY_PRIMITIVE_NUMBER_VALUE);
    ECMA_SET_POINTER (prim_value_prop_p->u.internal_property.value, prim_value_num_p);

    ret_value = ecma_make_normal_completion_value (ecma_make_object_value (obj_p));
  }
  else
  {
    JERRY_ASSERT (ecma_is_completion_value_throw (ret_value));
    ecma_deref_object (obj_p);
  }

  return ret_value;
} /* ecma_builtin_date_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN */
