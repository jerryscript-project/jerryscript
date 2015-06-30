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
#include "ecma-array-object.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-objects.h"
#include "ecma-regexp-object.h"
#include "ecma-try-catch-macro.h"
#include "jrt-libc-includes.h"
#include "lit-char-helpers.h"
#include "re-compiler.h"

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaregexpobject ECMA RegExp object related routines
 * @{
 */

/*
 * RegExp results are stored in an array of string pointers. If N is the number
 * of groups then the length of the array is 2*N, because every group has a start
 * and end. We have to handle those pointers.
 *
 *   [0] RE global start
 *   [1] RE global end
 *   [2] 1st group start
 *   [3] 1st group end
 *   ...
 *   [n]   n/2 th group start
 *   [n+1] n/2 th group end
 */
#define RE_GLOBAL_START_IDX 0
#define RE_GLOBAL_END_IDX   1

/* RegExp flags */
#define RE_FLAG_GLOBAL              (1 << 0) /* ECMA-262 v5, 15.10.7.2 */
#define RE_FLAG_IGNORE_CASE         (1 << 1) /* ECMA-262 v5, 15.10.7.3 */
#define RE_FLAG_MULTILINE           (1 << 2) /* ECMA-262 v5, 15.10.7.4 */

/**
 * Parse RegExp flags (global, ignoreCase, multiline)
 *
 * See also: ECMA-262 v5, 15.10.4.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
re_parse_regexp_flags (ecma_string_t *flags_str_p, /**< Input string with flags */
                       uint8_t *flags_p) /**< Output: parsed flag bits */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  FIXME ("Unicode: properly process non-ascii characters.");
  lit_utf8_size_t flags_str_size = ecma_string_get_size (flags_str_p);
  MEM_DEFINE_LOCAL_ARRAY (flags_start_p, flags_str_size, lit_utf8_byte_t);

  ecma_string_to_utf8_string (flags_str_p, flags_start_p, (ssize_t) flags_str_size);

  lit_utf8_byte_t *flags_char_p = flags_start_p;
  while (flags_char_p < flags_start_p + flags_str_size
         && ecma_is_completion_value_empty (ret_value))
  {
    switch (*flags_char_p)
    {
      case 'g':
      {
        if (*flags_p & RE_FLAG_GLOBAL)
        {
          ret_value = ecma_raise_syntax_error ("Invalid RegExp flags.");
        }
        *flags_p |= RE_FLAG_GLOBAL;
        break;
      }
      case 'i':
      {
        if (*flags_p & RE_FLAG_IGNORE_CASE)
        {
          ret_value = ecma_raise_syntax_error ("Invalid RegExp flags.");
        }
        *flags_p |= RE_FLAG_IGNORE_CASE;
        break;
      }
      case 'm':
      {
        if (*flags_p & RE_FLAG_MULTILINE)
        {
          ret_value = ecma_raise_syntax_error ("Invalid RegExp flags.");
        }
        *flags_p |= RE_FLAG_MULTILINE;
        break;
      }
      default:
      {
        ret_value = ecma_raise_syntax_error ("Invalid RegExp flags.");
        break;
      }
    }
    flags_char_p++;
  }

  MEM_FINALIZE_LOCAL_ARRAY (flags_start_p);

  return ret_value;
} /* re_parse_regexp_flags  */

/**
 * RegExp object creation operation.
 *
 * See also: ECMA-262 v5, 15.10.4.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_create_regexp_object (ecma_string_t *pattern_p, /**< input pattern */
                              ecma_string_t *flags_str_p) /**< flags */
{
  JERRY_ASSERT (pattern_p != NULL);
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  uint8_t flags = 0;
  if (flags_str_p != NULL)
  {
    ECMA_TRY_CATCH (empty, re_parse_regexp_flags (flags_str_p, &flags), ret_value);
    ECMA_FINALIZE (empty);

    if (!ecma_is_completion_value_empty (ret_value))
    {
      return ret_value;
    }
  }

  ecma_object_t *re_prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_REGEXP_PROTOTYPE);

  ecma_object_t *obj_p = ecma_create_object (re_prototype_obj_p, true, ECMA_OBJECT_TYPE_GENERAL);
  ecma_deref_object (re_prototype_obj_p);

  /* Set the internal [[Class]] property */
  ecma_property_t *class_prop_p = ecma_create_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_CLASS);
  class_prop_p->u.internal_property.value = LIT_MAGIC_STRING_REGEXP_UL;

  /* Set source property. ECMA-262 v5, 15.10.7.1 */
  ecma_string_t *magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_SOURCE);
  ecma_property_t *source_prop_p = ecma_create_named_data_property (obj_p,
                                                                    magic_string_p,
                                                                    false, false, false);
  ecma_deref_ecma_string (magic_string_p);
  ecma_set_named_data_property_value (source_prop_p,
                                      ecma_make_string_value (ecma_copy_or_ref_ecma_string (pattern_p)));

  ecma_simple_value_t prop_value;

  /* Set global property. ECMA-262 v5, 15.10.7.2*/
  magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_GLOBAL);
  ecma_property_t *global_prop_p = ecma_create_named_data_property (obj_p,
                                                                    magic_string_p,
                                                                    false, false, false);
  ecma_deref_ecma_string (magic_string_p);
  prop_value = flags & RE_FLAG_GLOBAL ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE;
  ecma_set_named_data_property_value (global_prop_p, ecma_make_simple_value (prop_value));

  /* Set ignoreCase property. ECMA-262 v5, 15.10.7.3*/
  magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_IGNORECASE_UL);
  ecma_property_t *ignorecase_prop_p = ecma_create_named_data_property (obj_p,
                                                                        magic_string_p,
                                                                        false, false, false);
  ecma_deref_ecma_string (magic_string_p);
  prop_value = flags & RE_FLAG_IGNORE_CASE ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE;
  ecma_set_named_data_property_value (ignorecase_prop_p, ecma_make_simple_value (prop_value));


  /* Set multiline property. ECMA-262 v5, 15.10.7.4*/
  magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_MULTILINE);
  ecma_property_t *multiline_prop_p = ecma_create_named_data_property (obj_p,
                                                                       magic_string_p,
                                                                       false, false, false);
  ecma_deref_ecma_string (magic_string_p);
  prop_value = flags & RE_FLAG_MULTILINE ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE;
  ecma_set_named_data_property_value (multiline_prop_p, ecma_make_simple_value (prop_value));

  /* Set lastIndex property. ECMA-262 v5, 15.10.7.5*/
  magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL);
  ecma_property_t *lastindex_prop_p = ecma_create_named_data_property (obj_p,
                                                                       magic_string_p,
                                                                       true, false, false);
  ecma_deref_ecma_string (magic_string_p);

  ecma_number_t *lastindex_num_p = ecma_alloc_number ();
  *lastindex_num_p = ECMA_NUMBER_ZERO;
  ecma_named_data_property_assign_value (obj_p, lastindex_prop_p, ecma_make_number_value (lastindex_num_p));
  ecma_dealloc_number (lastindex_num_p);

  /* Set bytecode internal property. */
  ecma_property_t *bytecode = ecma_create_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_REGEXP_BYTECODE);

  /* Compile bytecode. */
  ECMA_TRY_CATCH (empty, re_compile_bytecode (bytecode, pattern_p, flags), ret_value);
  ret_value = ecma_make_normal_completion_value (ecma_make_object_value (obj_p));
  ECMA_FINALIZE (empty);

  if (ecma_is_completion_value_throw (ret_value))
  {
    ecma_deref_object (obj_p);
  }

  return ret_value;
} /* ecma_op_create_regexp_object */

/**
 * Backtrack a unicode character
 */
static const lit_utf8_byte_t *
utf8_backtrack (const lit_utf8_byte_t *str_p)
{
  /* FIXME: change to string iterator with unicode support, when it would be implemented */
  return --str_p;
} /* utf8_backtrack */

/**
 * Helper to get an input character and increase string pointer.
 */
static ecma_char_t
get_input_char (const lit_utf8_byte_t **char_p)
{
  /* FIXME: change to string iterator with unicode support, when it would be implemented */
  const lit_utf8_byte_t ch = **char_p;
  (*char_p)++;
  return ch;
} /* get_input_char */

/**
 * Helper to get current input character, won't increase string pointer.
 */
static ecma_char_t
lookup_input_char (const lit_utf8_byte_t *str_p)
{
  /* FIXME: change to string iterator with unicode support, when it would be implemented */
  return *str_p;
} /* lookup_input_char */

/**
 * Helper to get previous input character, won't decrease string pointer.
 */
static ecma_char_t
lookup_prev_char (const lit_utf8_byte_t *str_p)
{
  /* FIXME: change to string iterator with unicode support, when it would be implemented */
  return *(--str_p);
} /* lookup_prev_char */

/**
 * Recursive function for RegExp matching. Tests for a regular expression
 * match and returns a MatchResult value.
 *
 * See also:
 *          ECMA-262 v5, 15.10.2.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
re_match_regexp (re_matcher_ctx_t *re_ctx_p, /**< RegExp matcher context */
                 re_bytecode_t *bc_p, /**< pointer to the current RegExp bytecode */
                 const lit_utf8_byte_t *str_p, /**< pointer to the current input character */
                 const lit_utf8_byte_t **res_p) /**< pointer to the matching substring */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  re_opcode_t op;

  if (re_ctx_p->recursion_depth >= RE_EXECUTE_RECURSION_LIMIT)
  {
    ret_value = ecma_raise_range_error ("RegExp executor recursion limit is exceeded.");
    return ret_value;
  }
  re_ctx_p->recursion_depth++;

  while ((op = re_get_opcode (&bc_p)))
  {
    if (re_ctx_p->match_limit >= RE_EXECUTE_MATCH_LIMIT)
    {
      ret_value = ecma_raise_range_error ("RegExp executor steps limit is exceeded.");
      return ret_value;
    }
    re_ctx_p->match_limit++;

    switch (op)
    {
      case RE_OP_MATCH:
      {
        JERRY_DDLOG ("Execute RE_OP_MATCH: match\n");
        *res_p = str_p;
        re_ctx_p->recursion_depth--;
        ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
        return ret_value; /* match */
      }
      case RE_OP_CHAR:
      {
        uint32_t ch1 = re_get_value (&bc_p);
        uint32_t ch2 = get_input_char (&str_p);
        JERRY_DDLOG ("Character matching %d to %d: ", ch1, ch2);

        if (ch2 == '\0' || ch1 != ch2)
        {
          JERRY_DDLOG ("fail\n");
          re_ctx_p->recursion_depth--;
          return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }
        JERRY_DDLOG ("match\n");
        break; /* tail merge */
      }
      case RE_OP_PERIOD:
      {
        uint32_t ch1 = get_input_char (&str_p);
        JERRY_DDLOG ("Period matching '.' to %d: ", ch1);
        if (ch1 == '\n' || ch1 == '\0')
        {
          JERRY_DDLOG ("fail\n");
          re_ctx_p->recursion_depth--;
          return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }
        JERRY_DDLOG ("match\n");
        break; /* tail merge */
      }
      case RE_OP_ASSERT_START:
      {
        JERRY_DDLOG ("Execute RE_OP_ASSERT_START: ");

        if (str_p <= re_ctx_p->input_start_p)
        {
          JERRY_DDLOG ("match\n");
          break;
        }

        if (!(re_ctx_p->flags & RE_FLAG_MULTILINE))
        {
          JERRY_DDLOG ("fail\n");
          re_ctx_p->recursion_depth--;
          return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        if (lit_char_is_line_terminator (lookup_prev_char (str_p)))
        {
          JERRY_DDLOG ("match\n");
          break;
        }

        JERRY_DDLOG ("fail\n");
        re_ctx_p->recursion_depth--;
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_ASSERT_END:
      {
        JERRY_DDLOG ("Execute RE_OP_ASSERT_END: ");

        if (str_p >= re_ctx_p->input_end_p)
        {
          JERRY_DDLOG ("match\n");
          break; /* tail merge */
        }

        if (!(re_ctx_p->flags & RE_FLAG_MULTILINE))
        {
          JERRY_DDLOG ("fail\n");
          re_ctx_p->recursion_depth--;
          return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        if (lit_char_is_line_terminator (lookup_input_char (str_p)))
        {
          JERRY_DDLOG ("match\n");
          break; /* tail merge */
        }

        JERRY_DDLOG ("fail\n");
        re_ctx_p->recursion_depth--;
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_ASSERT_WORD_BOUNDARY:
      case RE_OP_ASSERT_NOT_WORD_BOUNDARY:
      {
        bool is_wordchar_left, is_wordchar_right;

        if (str_p <= re_ctx_p->input_start_p)
        {
          is_wordchar_left = false;  /* not a wordchar */
        }
        else
        {
          is_wordchar_left = lit_char_is_word_char (lookup_prev_char (str_p));
        }

        if (str_p >= re_ctx_p->input_end_p)
        {
          is_wordchar_right = false;  /* not a wordchar */
        }
        else
        {
          is_wordchar_right = lit_char_is_word_char (lookup_input_char (str_p));
        }

        if (op == RE_OP_ASSERT_WORD_BOUNDARY)
        {
          JERRY_DDLOG ("Execute RE_OP_ASSERT_WORD_BOUNDARY at %c: ", *str_p);
          if (is_wordchar_left == is_wordchar_right)
          {
            JERRY_DDLOG ("fail\n");
            re_ctx_p->recursion_depth--;
            return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }
        else
        {
          JERRY_ASSERT (op == RE_OP_ASSERT_NOT_WORD_BOUNDARY);
          JERRY_DDLOG ("Execute RE_OP_ASSERT_NOT_WORD_BOUNDARY at %c: ", *str_p);

          if (is_wordchar_left != is_wordchar_right)
          {
            JERRY_DDLOG ("fail\n");
            re_ctx_p->recursion_depth--;
            return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }

        JERRY_DDLOG ("match\n");
        break; /* tail merge */
      }
      case RE_OP_LOOKAHEAD_POS:
      case RE_OP_LOOKAHEAD_NEG:
      {
        ecma_completion_value_t match_value = ecma_make_empty_completion_value ();
        const lit_utf8_byte_t *sub_str_p = NULL;

        MEM_DEFINE_LOCAL_ARRAY (saved_bck_p, re_ctx_p->num_of_captures, lit_utf8_byte_t *);
        size_t size = (size_t) (re_ctx_p->num_of_captures) * sizeof (const lit_utf8_byte_t *);
        memcpy (saved_bck_p, re_ctx_p->saved_p, size);

        do
        {
          uint32_t offset = re_get_value (&bc_p);
          if (!sub_str_p)
          {
            match_value = re_match_regexp (re_ctx_p, bc_p, str_p, &sub_str_p);
            if (ecma_is_completion_value_throw (match_value))
            {
              break;
            }
          }
          bc_p += offset;
        }
        while (re_get_opcode (&bc_p) == RE_OP_ALTERNATIVE);

        if (!ecma_is_completion_value_throw (match_value))
        {
          JERRY_DDLOG ("Execute RE_OP_LOOKAHEAD_POS/NEG: ");
          ecma_free_completion_value (match_value);
          if ((op == RE_OP_LOOKAHEAD_POS && sub_str_p)
              || (op == RE_OP_LOOKAHEAD_NEG && !sub_str_p))
          {
            JERRY_DDLOG ("match\n");
            match_value = re_match_regexp (re_ctx_p, bc_p, str_p, &sub_str_p);
          }
          else
          {
            JERRY_DDLOG ("fail\n");
            match_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }

        if (!ecma_is_completion_value_throw (match_value))
        {
          re_ctx_p->recursion_depth--;
          if (ecma_is_value_true (match_value))
          {
            *res_p = sub_str_p;
          }
          else
          {
            JERRY_ASSERT (ecma_is_value_boolean (match_value));
            /* restore saved */
            memcpy (re_ctx_p->saved_p, saved_bck_p, size);
          }
        }

        MEM_FINALIZE_LOCAL_ARRAY (saved_bck_p);
        return match_value;
      }
      case RE_OP_CHAR_CLASS:
      case RE_OP_INV_CHAR_CLASS:
      {
        uint32_t curr_ch, num_of_ranges;
        bool is_match;

        JERRY_DDLOG ("Execute RE_OP_CHAR_CLASS/RE_OP_INV_CHAR_CLASS, ");

        if (str_p >= re_ctx_p->input_end_p)
        {
          JERRY_DDLOG ("fail\n");
          re_ctx_p->recursion_depth--;
          return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        curr_ch = get_input_char (&str_p);

        num_of_ranges = re_get_value (&bc_p);
        is_match = false;
        while (num_of_ranges)
        {
          uint32_t ch1, ch2;
          ch1 = (uint32_t) re_get_value (&bc_p);
          ch2 = (uint32_t) re_get_value (&bc_p);
          JERRY_DDLOG ("num_of_ranges=%d, ch1=%d, ch2=%d, curr_ch=%d; ",
                       num_of_ranges, ch1, ch2, curr_ch);

          if (curr_ch >= ch1 && curr_ch <= ch2)
          {
            /* We must read all the ranges from bytecode. */
            is_match = true;
          }
          num_of_ranges--;
        }

        if (op == RE_OP_CHAR_CLASS)
        {
          if (!is_match)
          {
            JERRY_DDLOG ("fail\n");
            re_ctx_p->recursion_depth--;
            return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }
        else
        {
          JERRY_ASSERT (op == RE_OP_INV_CHAR_CLASS);
          if (is_match)
          {
            JERRY_DDLOG ("fail\n");
            re_ctx_p->recursion_depth--;
            return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }
        JERRY_DDLOG ("match\n");
        break; /* tail merge */
      }
      case RE_OP_BACKREFERENCE:
      {
        uint32_t backref_idx;
        const lit_utf8_byte_t *sub_str_p;

        backref_idx = re_get_value (&bc_p);
        JERRY_DDLOG ("Execute RE_OP_BACKREFERENCE (idx: %d): ", backref_idx);
        backref_idx *= 2;  /* backref n -> saved indices [n*2, n*2+1] */
        JERRY_ASSERT (backref_idx >= 2 && backref_idx + 1 < re_ctx_p->num_of_captures);

        if (!re_ctx_p->saved_p[backref_idx] || !re_ctx_p->saved_p[backref_idx + 1])
        {
          JERRY_DDLOG ("match\n");
          break; /* capture is 'undefined', always matches! */
        }

        sub_str_p = re_ctx_p->saved_p[backref_idx];
        while (sub_str_p < re_ctx_p->saved_p[backref_idx + 1])
        {
          uint32_t ch1, ch2;

          if (str_p >= re_ctx_p->input_end_p)
          {
            JERRY_DDLOG ("fail\n");
            re_ctx_p->recursion_depth--;
            return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }

          ch1 = get_input_char (&sub_str_p);
          ch2 = get_input_char (&str_p);

          if (ch1 != ch2)
          {
            JERRY_DDLOG ("fail\n");
            re_ctx_p->recursion_depth--;
            return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }
        JERRY_DDLOG ("match\n");
        break; /* tail merge */
      }
      case RE_OP_SAVE_AT_START:
      {
        const lit_utf8_byte_t *old_start_p;
        re_bytecode_t *old_bc_p;

        JERRY_DDLOG ("Execute RE_OP_SAVE_AT_START\n");
        old_start_p = re_ctx_p->saved_p[RE_GLOBAL_START_IDX];
        re_ctx_p->saved_p[RE_GLOBAL_START_IDX] = str_p;
        do
        {
          uint32_t offset = re_get_value (&bc_p);
          const lit_utf8_byte_t *sub_str_p;
          ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_p, &sub_str_p);
          if (ecma_is_value_true (match_value))
          {
            *res_p = sub_str_p;
            re_ctx_p->recursion_depth--;
            return match_value; /* match */
          }
          else if (ecma_is_completion_value_throw (match_value))
          {
            return match_value;
          }
          bc_p += offset;
          old_bc_p = bc_p;
        }
        while (re_get_opcode (&bc_p) == RE_OP_ALTERNATIVE);
        bc_p = old_bc_p;

        re_ctx_p->saved_p[RE_GLOBAL_START_IDX] = old_start_p;
        re_ctx_p->recursion_depth--;
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_SAVE_AND_MATCH:
      {
        JERRY_DDLOG ("End of pattern is reached: match\n");
        re_ctx_p->saved_p[RE_GLOBAL_END_IDX] = str_p;
        *res_p = str_p;
        re_ctx_p->recursion_depth--;
        return ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE); /* match */
      }
      case RE_OP_ALTERNATIVE:
      {
        /*
        *  Alternatives should be jump over, when alternative opcode appears.
        */
        uint32_t offset = re_get_value (&bc_p);
        JERRY_DDLOG ("Execute RE_OP_ALTERNATIVE");
        bc_p += offset;
        while (*bc_p == RE_OP_ALTERNATIVE)
        {
          JERRY_DDLOG (", jump: %d");
          bc_p++;
          offset = re_get_value (&bc_p);
          bc_p += offset;
        }
        JERRY_DDLOG ("\n");
        break; /* tail merge */
      }
      case RE_OP_CAPTURE_NON_GREEDY_ZERO_GROUP_START:
      case RE_OP_NON_CAPTURE_NON_GREEDY_ZERO_GROUP_START:
      {
        /*
        *  On non-greedy iterations we have to execute the bytecode
        *  after the group first, if zero iteration is allowed.
        */
        uint32_t start_idx, iter_idx, offset;
        const lit_utf8_byte_t *old_start_p;
        const lit_utf8_byte_t *sub_str_p;
        re_bytecode_t *old_bc_p;

        old_bc_p = bc_p; /* save the bytecode start position of the group start */
        start_idx = re_get_value (&bc_p);
        offset = re_get_value (&bc_p);

        if (RE_IS_CAPTURE_GROUP (op))
        {
          JERRY_ASSERT (start_idx <= re_ctx_p->num_of_captures / 2);
          iter_idx = start_idx - 1;
          start_idx *= 2;

          old_start_p = re_ctx_p->saved_p[start_idx];
          re_ctx_p->saved_p[start_idx] = str_p;
        }
        else
        {
          JERRY_ASSERT (start_idx < re_ctx_p->num_of_non_captures);
          iter_idx = start_idx + (re_ctx_p->num_of_captures / 2) - 1;
          start_idx += re_ctx_p->num_of_captures;
        }
        re_ctx_p->num_of_iterations[iter_idx] = 0;

        /* Jump all over to the end of the END opcode. */
        bc_p += offset;

        /* Try to match after the close paren if zero is allowed */
        ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_p, &sub_str_p);
        if (ecma_is_value_true (match_value))
        {
          *res_p = sub_str_p;
          re_ctx_p->recursion_depth--;
          return match_value; /* match */
        }
        else if (ecma_is_completion_value_throw (match_value))
        {
          return match_value;
        }
        if (RE_IS_CAPTURE_GROUP (op))
        {
          re_ctx_p->saved_p[start_idx] = old_start_p;
        }

        bc_p = old_bc_p;
        /* FALLTHRU */
      }
      case RE_OP_CAPTURE_GROUP_START:
      case RE_OP_CAPTURE_GREEDY_ZERO_GROUP_START:
      case RE_OP_NON_CAPTURE_GROUP_START:
      case RE_OP_NON_CAPTURE_GREEDY_ZERO_GROUP_START:
      {
        uint32_t start_idx, iter_idx, old_iteration_cnt, offset;
        const lit_utf8_byte_t *old_start_p;
        const lit_utf8_byte_t *sub_str_p;
        re_bytecode_t *old_bc_p;
        re_bytecode_t *end_bc_p = NULL;

        start_idx = re_get_value (&bc_p);
        if (op != RE_OP_CAPTURE_GROUP_START
            && op != RE_OP_NON_CAPTURE_GROUP_START)
        {
          offset = re_get_value (&bc_p);
          end_bc_p = bc_p + offset;
        }

        if (RE_IS_CAPTURE_GROUP (op))
        {
          JERRY_ASSERT (start_idx <= re_ctx_p->num_of_captures / 2);
          iter_idx = start_idx - 1;
          start_idx *= 2;
        }
        else
        {
          JERRY_ASSERT (start_idx < re_ctx_p->num_of_non_captures);
          iter_idx = start_idx + (re_ctx_p->num_of_captures / 2) - 1;
          start_idx += re_ctx_p->num_of_captures;
        }
        old_start_p = re_ctx_p->saved_p[start_idx];
        old_iteration_cnt = re_ctx_p->num_of_iterations[iter_idx];
        re_ctx_p->saved_p[start_idx] = str_p;
        re_ctx_p->num_of_iterations[iter_idx] = 0;

        do
        {
          offset = re_get_value (&bc_p);
          ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_p, &sub_str_p);
          if (ecma_is_value_true (match_value))
          {
            *res_p = sub_str_p;
            re_ctx_p->recursion_depth--;
            return match_value; /* match */
          }
          else if (ecma_is_completion_value_throw (match_value))
          {
            return match_value;
          }
          bc_p += offset;
          old_bc_p = bc_p;
        }
        while (re_get_opcode (&bc_p) == RE_OP_ALTERNATIVE);
        bc_p = old_bc_p;
        re_ctx_p->num_of_iterations[iter_idx] = old_iteration_cnt;

        /* Try to match after the close paren if zero is allowed. */
        if (op == RE_OP_CAPTURE_GREEDY_ZERO_GROUP_START
            || op == RE_OP_NON_CAPTURE_GREEDY_ZERO_GROUP_START)
        {
          JERRY_ASSERT (end_bc_p);
          ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, end_bc_p, str_p, &sub_str_p);
          if (ecma_is_value_true (match_value))
          {
            *res_p = sub_str_p;
            re_ctx_p->recursion_depth--;
            return match_value; /* match */
          }
          else if (ecma_is_completion_value_throw (match_value))
          {
            return match_value;
          }
        }

        re_ctx_p->saved_p[start_idx] = old_start_p;
        re_ctx_p->recursion_depth--;
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_CAPTURE_NON_GREEDY_GROUP_END:
      case RE_OP_NON_CAPTURE_NON_GREEDY_GROUP_END:
      {
        uint32_t end_idx, iter_idx, min, max;
        const lit_utf8_byte_t *old_end_p;
        re_bytecode_t *old_bc_p;

        /*
        *  On non-greedy iterations we have to execute the bytecode
        *  after the group first. Try to iterate only if it fails.
        */
        old_bc_p = bc_p; /* save the bytecode start position of the group end */
        end_idx = re_get_value (&bc_p);
        min = re_get_value (&bc_p);
        max = re_get_value (&bc_p);
        re_get_value (&bc_p); /* start offset */

        if (RE_IS_CAPTURE_GROUP (op))
        {
          JERRY_ASSERT (end_idx <= re_ctx_p->num_of_captures / 2);
          iter_idx = end_idx - 1;
          end_idx = (end_idx * 2) + 1;
        }
        else
        {
          JERRY_ASSERT (end_idx <= re_ctx_p->num_of_non_captures);
          iter_idx = end_idx + (re_ctx_p->num_of_captures / 2) - 1;
          end_idx += re_ctx_p->num_of_captures;
        }

        re_ctx_p->num_of_iterations[iter_idx]++;
        if (re_ctx_p->num_of_iterations[iter_idx] >= min
            && re_ctx_p->num_of_iterations[iter_idx] <= max)
        {
          old_end_p = re_ctx_p->saved_p[end_idx];
          re_ctx_p->saved_p[end_idx] = str_p;

          const lit_utf8_byte_t *sub_str_p;
          ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_p, &sub_str_p);
          if (ecma_is_value_true (match_value))
          {
            *res_p = sub_str_p;
            re_ctx_p->recursion_depth--;
            return match_value; /* match */
          }
          else if (ecma_is_completion_value_throw (match_value))
          {
            return match_value;
          }

          re_ctx_p->saved_p[end_idx] = old_end_p;
        }
        re_ctx_p->num_of_iterations[iter_idx]--;
        bc_p = old_bc_p;

        /* If non-greedy fails and try to iterate... */
        /* FALLTHRU */
      }
      case RE_OP_CAPTURE_GREEDY_GROUP_END:
      case RE_OP_NON_CAPTURE_GREEDY_GROUP_END:
      {
        uint32_t start_idx, end_idx, iter_idx, min, max, offset;
        const lit_utf8_byte_t *old_start_p;
        const lit_utf8_byte_t *old_end_p;
        const lit_utf8_byte_t *sub_str_p;
        re_bytecode_t *old_bc_p;

        end_idx = re_get_value (&bc_p);
        min = re_get_value (&bc_p);
        max = re_get_value (&bc_p);
        offset = re_get_value (&bc_p);

        if (RE_IS_CAPTURE_GROUP (op))
        {
          JERRY_ASSERT (end_idx <= re_ctx_p->num_of_captures / 2);
          iter_idx = end_idx - 1;
          start_idx = end_idx * 2;
          end_idx = start_idx + 1;
        }
        else
        {
          JERRY_ASSERT (end_idx <= re_ctx_p->num_of_non_captures);
          iter_idx = end_idx + (re_ctx_p->num_of_captures / 2) - 1;
          end_idx += re_ctx_p->num_of_captures;
          start_idx = end_idx;
        }

        /* Check the empty iteration if the minimum number of iterations is reached. */
        if (re_ctx_p->num_of_iterations[iter_idx] >= min
            && str_p == re_ctx_p->saved_p[start_idx])
        {
          re_ctx_p->recursion_depth--;
          return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }
        re_ctx_p->num_of_iterations[iter_idx]++;

        old_bc_p = bc_p; /* Save the bytecode end position of the END opcodes for matching after it. */
        old_end_p = re_ctx_p->saved_p[end_idx];
        re_ctx_p->saved_p[end_idx] = str_p;

        if (re_ctx_p->num_of_iterations[iter_idx] < max)
        {
          bc_p -= offset;
          offset = re_get_value (&bc_p);

          old_start_p = re_ctx_p->saved_p[start_idx];
          re_ctx_p->saved_p[start_idx] = str_p;
          ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_p, &sub_str_p);
          if (ecma_is_value_true (match_value))
          {
            *res_p = sub_str_p;
            re_ctx_p->recursion_depth--;
            return match_value; /* match */
          }
          else if (ecma_is_completion_value_throw (match_value))
          {
            return match_value;
          }

          re_ctx_p->saved_p[start_idx] = old_start_p;

          /* Try to match alternatives if any. */
          bc_p += offset;
          while (*bc_p == RE_OP_ALTERNATIVE)
          {
            bc_p++; /* RE_OP_ALTERNATIVE */
            offset = re_get_value (&bc_p);

            old_start_p = re_ctx_p->saved_p[start_idx];
            re_ctx_p->saved_p[start_idx] = str_p;

            ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_p, &sub_str_p);
            if (ecma_is_value_true (match_value))
            {
              *res_p = sub_str_p;
              re_ctx_p->recursion_depth--;
              return match_value; /* match */
            }
            else if (ecma_is_completion_value_throw (match_value))
            {
              return match_value;
            }

            re_ctx_p->saved_p[start_idx] = old_start_p;
            bc_p += offset;
          }
        }

        if (re_ctx_p->num_of_iterations[iter_idx] >= min
            && re_ctx_p->num_of_iterations[iter_idx] <= max)
        {
          /* Try to match the rest of the bytecode. */
          ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, old_bc_p, str_p, &sub_str_p);
          if (ecma_is_value_true (match_value))
          {
            *res_p = sub_str_p;
            re_ctx_p->recursion_depth--;
            return match_value; /* match */
          }
          else if (ecma_is_completion_value_throw (match_value))
          {
            return match_value;
          }
        }

        /* restore if fails */
        re_ctx_p->saved_p[end_idx] = old_end_p;
        re_ctx_p->num_of_iterations[iter_idx]--;
        re_ctx_p->recursion_depth--;
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_NON_GREEDY_ITERATOR:
      {
        uint32_t min, max, offset, num_of_iter;
        const lit_utf8_byte_t *sub_str_p;

        min = re_get_value (&bc_p);
        max = re_get_value (&bc_p);

        offset = re_get_value (&bc_p);
        JERRY_DDLOG ("Non-greedy iterator, min=%lu, max=%lu, offset=%ld\n",
                     (unsigned long) min, (unsigned long) max, (long) offset);

        num_of_iter = 0;
        while (num_of_iter <= max)
        {
          if (num_of_iter >= min)
          {
            ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, bc_p + offset, str_p, &sub_str_p);
            if (ecma_is_value_true (match_value))
            {
              *res_p = sub_str_p;
              re_ctx_p->recursion_depth--;
              return match_value; /* match */
            }
            else if (ecma_is_completion_value_throw (match_value))
            {
              return match_value;
            }
          }

          ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_p, &sub_str_p);
          if (!ecma_is_value_true (match_value))
          {
            break;
          }
          else if (ecma_is_completion_value_throw (match_value))
          {
            return match_value;
          }
          str_p = sub_str_p;
          num_of_iter++;
        }
        re_ctx_p->recursion_depth--;
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_GREEDY_ITERATOR:
      {
        uint32_t min, max, offset, num_of_iter;
        const lit_utf8_byte_t *sub_str_p;

        min = re_get_value (&bc_p);
        max = re_get_value (&bc_p);

        offset = re_get_value (&bc_p);
        JERRY_DDLOG ("Greedy iterator, min=%lu, max=%lu, offset=%ld\n",
                     (unsigned long) min, (unsigned long) max, (long) offset);

        num_of_iter = 0;
        while (num_of_iter < max)
        {
          ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_p, &sub_str_p);
          if (!ecma_is_value_true (match_value))
          {
            break;
          }
          else if (ecma_is_completion_value_throw (match_value))
          {
            return match_value;
          }
          str_p = sub_str_p;
          num_of_iter++;
        }

        while (num_of_iter >= min)
        {
          ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, bc_p + offset, str_p, &sub_str_p);
          if (ecma_is_value_true (match_value))
          {
            *res_p = sub_str_p;
            re_ctx_p->recursion_depth--;
            return match_value; /* match */
          }
          else if (ecma_is_completion_value_throw (match_value))
          {
            return match_value;
          }
          if (num_of_iter == min)
          {
            break;
          }

          str_p = utf8_backtrack (str_p);
          num_of_iter--;
        }
        re_ctx_p->recursion_depth--;
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      default:
      {
        JERRY_DDLOG ("UNKNOWN opcode (%d)!\n", (uint32_t) op);
        re_ctx_p->recursion_depth--;
        return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_COMMON));
      }
    }
  }

  JERRY_UNREACHABLE ();
  return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
} /* regexp_match */

/**
 * Define the necessary properties for the result array (index, input, length).
 */
static void
re_set_result_array_properties (ecma_object_t *array_obj_p, /**< result array */
                                re_matcher_ctx_t *re_ctx_p, /**< RegExp matcher context */
                                int32_t index) /** index of matching */
{
  /* Set index property of the result array */
  ecma_string_t *result_prop_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_INDEX);
  {
    ecma_property_descriptor_t array_item_prop_desc = ecma_make_empty_property_descriptor ();

    array_item_prop_desc.is_value_defined = true;

    ecma_number_t *num_p = ecma_alloc_number ();
    *num_p = (ecma_number_t) index;
    array_item_prop_desc.value = ecma_make_number_value (num_p);

    array_item_prop_desc.is_writable_defined = true;
    array_item_prop_desc.is_writable = true;

    array_item_prop_desc.is_enumerable_defined = true;
    array_item_prop_desc.is_enumerable = true;

    array_item_prop_desc.is_configurable_defined = true;
    array_item_prop_desc.is_configurable = true;

    ecma_op_object_define_own_property (array_obj_p,
                                        result_prop_str_p,
                                        &array_item_prop_desc,
                                        true);

    ecma_dealloc_number (num_p);
  }
  ecma_deref_ecma_string (result_prop_str_p);

  /* Set input property of the result array */
  result_prop_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_INPUT);
  {
    ecma_property_descriptor_t array_item_prop_desc = ecma_make_empty_property_descriptor ();

    array_item_prop_desc.is_value_defined = true;
    ecma_string_t *input_str_p = ecma_new_ecma_string_from_utf8 (re_ctx_p->input_start_p,
                                                                 (lit_utf8_size_t) (re_ctx_p->input_end_p -
                                                                                    re_ctx_p->input_start_p));
    array_item_prop_desc.value = ecma_make_string_value (input_str_p);

    array_item_prop_desc.is_writable_defined = true;
    array_item_prop_desc.is_writable = true;

    array_item_prop_desc.is_enumerable_defined = true;
    array_item_prop_desc.is_enumerable = true;

    array_item_prop_desc.is_configurable_defined = true;
    array_item_prop_desc.is_configurable = true;

    ecma_op_object_define_own_property (array_obj_p,
                                        result_prop_str_p,
                                        &array_item_prop_desc,
                                        true);

    ecma_deref_ecma_string (input_str_p);
  }
  ecma_deref_ecma_string (result_prop_str_p);

  /* Set length property of the result array */
  result_prop_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);
  {

    ecma_property_descriptor_t array_item_prop_desc = ecma_make_empty_property_descriptor ();
    array_item_prop_desc.is_value_defined = true;

    ecma_number_t *num_p = ecma_alloc_number ();
    *num_p = (ecma_number_t) (re_ctx_p->num_of_captures / 2);
    array_item_prop_desc.value = ecma_make_number_value (num_p);

    array_item_prop_desc.is_writable_defined = false;
    array_item_prop_desc.is_enumerable_defined = false;
    array_item_prop_desc.is_configurable_defined = false;

    ecma_op_object_define_own_property (array_obj_p,
                                        result_prop_str_p,
                                        &array_item_prop_desc,
                                        true);

    ecma_dealloc_number (num_p);
  }
  ecma_deref_ecma_string (result_prop_str_p);
} /* re_set_result_array_properties */

/**
 * RegExp helper function to start the recursive matching algorithm
 * and create the result Array object
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_regexp_exec_helper (ecma_object_t *obj_p, /**< RegExp object */
                         re_bytecode_t *bc_p, /**< start of the RegExp bytecode */
                         const lit_utf8_byte_t *str_p, /**< start of the input string */
                         lit_utf8_size_t str_size) /**< size of the input string */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  lit_utf8_size_t input_size = str_size;
  re_matcher_ctx_t re_ctx;
  re_ctx.input_start_p = str_p;
  re_ctx.input_end_p = str_p + str_size;
  re_ctx.match_limit = 0;
  re_ctx.recursion_depth = 0;

  /* 1. Read bytecode header and init regexp matcher context. */
  re_ctx.flags = (uint8_t) re_get_value (&bc_p);
  JERRY_DDLOG ("Exec with flags [global: %d, ignoreCase: %d, multiline: %d]\n",
               re_ctx.flags & RE_FLAG_GLOBAL,
               re_ctx.flags & RE_FLAG_IGNORE_CASE,
               re_ctx.flags & RE_FLAG_MULTILINE);

  re_ctx.num_of_captures = re_get_value (&bc_p);
  JERRY_ASSERT (re_ctx.num_of_captures % 2 == 0);
  re_ctx.num_of_non_captures = re_get_value (&bc_p);

  MEM_DEFINE_LOCAL_ARRAY (saved_p, re_ctx.num_of_captures + re_ctx.num_of_non_captures, const lit_utf8_byte_t *);
  for (uint32_t i = 0; i < re_ctx.num_of_captures + re_ctx.num_of_non_captures; i++)
  {
    saved_p[i] = NULL;
  }
  re_ctx.saved_p = saved_p;

  uint32_t num_of_iter_length = (re_ctx.num_of_captures / 2) + (re_ctx.num_of_non_captures - 1);
  MEM_DEFINE_LOCAL_ARRAY (num_of_iter_p, num_of_iter_length, uint32_t);
  for (uint32_t i = 0; i < num_of_iter_length; i++)
  {
    num_of_iter_p[i] = 0u;
  }

  bool is_match = false;
  re_ctx.num_of_iterations = num_of_iter_p;
  int32_t index = 0;

  if (re_ctx.flags & RE_FLAG_GLOBAL)
  {
    ecma_string_t *magic_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL);
    ecma_property_t *lastindex_prop_p = ecma_op_object_get_property (obj_p, magic_str_p);
    ecma_number_t *lastindex_num_p = ecma_get_number_from_value (lastindex_prop_p->u.named_data_property.value);
    index = ecma_number_to_int32 (*lastindex_num_p);
    JERRY_ASSERT (str_p != NULL);
    str_p += ecma_number_to_int32 (*lastindex_num_p);
    ecma_deref_ecma_string (magic_str_p);
  }

  /* 2. Try to match */
  const lit_utf8_byte_t *sub_str_p;
  while (str_p && str_p <= re_ctx.input_end_p && ecma_is_completion_value_empty (ret_value))
  {
    if (index < 0 || index > (int32_t) input_size)
    {
      ecma_string_t *magic_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL);
      ecma_number_t *lastindex_num_p = ecma_alloc_number ();
      *lastindex_num_p = ECMA_NUMBER_ZERO;
      ecma_op_object_put (obj_p, magic_str_p, ecma_make_number_value (lastindex_num_p), true);
      ecma_dealloc_number (lastindex_num_p);
      ecma_deref_ecma_string (magic_str_p);

      is_match = false;
      break;
    }
    else
    {
      sub_str_p = NULL;
      ECMA_TRY_CATCH (match_value, re_match_regexp (&re_ctx, bc_p, str_p, &sub_str_p), ret_value);
      if (ecma_is_value_true (match_value))
      {
        is_match = true;
        break;
      }
      str_p++;
      index++;
      ECMA_FINALIZE (match_value);
    }
  }

  if (re_ctx.flags & RE_FLAG_GLOBAL)
  {
    ecma_string_t *magic_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL);
    ecma_number_t *lastindex_num_p = ecma_alloc_number ();
    *lastindex_num_p = ((ecma_number_t) (sub_str_p - re_ctx.input_start_p));
    ecma_op_object_put (obj_p, magic_str_p, ecma_make_number_value (lastindex_num_p), true);
    ecma_dealloc_number (lastindex_num_p);
    ecma_deref_ecma_string (magic_str_p);
  }

  /* 3. Fill the result array or return with 'undefiend' */
  if (ecma_is_completion_value_empty (ret_value))
  {
    if (is_match)
    {
      ecma_completion_value_t result_array = ecma_op_create_array_object (0, 0, false);
      ecma_object_t *result_array_obj_p = ecma_get_object_from_completion_value (result_array);

      re_set_result_array_properties (result_array_obj_p, &re_ctx, index);

      for (uint32_t i = 0; i < re_ctx.num_of_captures; i += 2)
      {
        ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (i / 2);

        if (re_ctx.saved_p[i] && re_ctx.saved_p[i + 1] && re_ctx.saved_p[i + 1] >= re_ctx.saved_p[i])
        {
          ecma_length_t capture_str_len = static_cast<ecma_length_t> (re_ctx.saved_p[i + 1] - re_ctx.saved_p[i]);
          ecma_string_t *capture_str_p;

          if (capture_str_len > 0)
          {
            capture_str_p = ecma_new_ecma_string_from_utf8 (re_ctx.saved_p[i], capture_str_len);
          }
          else
          {
            capture_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
          }
          ecma_op_object_put (result_array_obj_p, index_str_p, ecma_make_string_value (capture_str_p), true);
          ecma_deref_ecma_string (capture_str_p);
        }
        else
        {
          ecma_op_object_put (result_array_obj_p,
                              index_str_p,
                              ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED),
                              true);
        }
        ecma_deref_ecma_string (index_str_p);
      }
      ret_value = result_array;
    }
    else
    {
      ret_value = ecma_make_normal_completion_value (ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED));
    }
  }
  MEM_FINALIZE_LOCAL_ARRAY (num_of_iter_p);
  MEM_FINALIZE_LOCAL_ARRAY (saved_p);

  return ret_value;
} /* ecma_regexp_exec_helper */

/**
 * @}
 * @}
 */

#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */
