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
#include "ecma-builtin-helpers.h"
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

/**
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

/**
 * Parse RegExp flags (global, ignoreCase, multiline)
 *
 * See also: ECMA-262 v5, 15.10.4.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
re_parse_regexp_flags (ecma_string_t *flags_str_p, /**< Input string with flags */
                       uint8_t *flags_p) /**< Output: parsed flag bits */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  lit_utf8_size_t flags_str_size = ecma_string_get_size (flags_str_p);
  MEM_DEFINE_LOCAL_ARRAY (flags_start_p, flags_str_size, lit_utf8_byte_t);

  ssize_t sz = ecma_string_to_utf8_string (flags_str_p, flags_start_p, (ssize_t) flags_str_size);
  JERRY_ASSERT (sz >= 0);

  lit_utf8_byte_t *flags_str_curr_p = flags_start_p;
  const lit_utf8_byte_t *flags_str_end_p = flags_start_p + flags_str_size;

  while (flags_str_curr_p < flags_str_end_p
         && ecma_is_completion_value_empty (ret_value))
  {
    switch (*flags_str_curr_p++)
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
  }

  MEM_FINALIZE_LOCAL_ARRAY (flags_start_p);

  return ret_value;
} /* re_parse_regexp_flags  */

/*
 * Initializes the source, global, ignoreCase, multiline, and lastIndex properties of RegExp instance.
 */
void
re_initialize_props (ecma_object_t *re_obj_p, /**< RegExp obejct */
                     ecma_string_t *source_p, /**< source string */
                     uint8_t flags) /**< flags */
{
 /* Set source property. ECMA-262 v5, 15.10.7.1 */
  ecma_string_t *magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_SOURCE);
  ecma_property_t *prop_p = ecma_find_named_property (re_obj_p, magic_string_p);

  if (prop_p == NULL)
  {
    prop_p = ecma_create_named_data_property (re_obj_p,
                                              magic_string_p,
                                              false, false, false);
  }

  ecma_deref_ecma_string (magic_string_p);
  JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDDATA);
  ecma_named_data_property_assign_value (re_obj_p,
                                         prop_p,
                                         ecma_make_string_value (source_p));

  ecma_simple_value_t prop_value;

  /* Set global property. ECMA-262 v5, 15.10.7.2 */
  magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_GLOBAL);
  prop_p = ecma_find_named_property (re_obj_p, magic_string_p);

  if (prop_p == NULL)
  {
    prop_p = ecma_create_named_data_property (re_obj_p,
                                              magic_string_p,
                                              false, false, false);
  }

  ecma_deref_ecma_string (magic_string_p);
  prop_value = (flags & RE_FLAG_GLOBAL) ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE;
  JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDDATA);
  ecma_set_named_data_property_value (prop_p, ecma_make_simple_value (prop_value));

  /* Set ignoreCase property. ECMA-262 v5, 15.10.7.3 */
  magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_IGNORECASE_UL);
  prop_p = ecma_find_named_property (re_obj_p, magic_string_p);

  if (prop_p == NULL)
  {
    prop_p = ecma_create_named_data_property (re_obj_p,
                                              magic_string_p,
                                              false, false, false);
  }

  ecma_deref_ecma_string (magic_string_p);
  prop_value = (flags & RE_FLAG_IGNORE_CASE) ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE;
  JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDDATA);
  ecma_set_named_data_property_value (prop_p, ecma_make_simple_value (prop_value));

  /* Set multiline property. ECMA-262 v5, 15.10.7.4 */
  magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_MULTILINE);
  prop_p = ecma_find_named_property (re_obj_p, magic_string_p);

  if (prop_p == NULL)
  {
    prop_p = ecma_create_named_data_property (re_obj_p,
                                              magic_string_p,
                                              false, false, false);
  }

  ecma_deref_ecma_string (magic_string_p);
  prop_value = (flags & RE_FLAG_MULTILINE) ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE;
  JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDDATA);
  ecma_set_named_data_property_value (prop_p, ecma_make_simple_value (prop_value));

  /* Set lastIndex property. ECMA-262 v5, 15.10.7.5 */
  magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL);
  prop_p = ecma_find_named_property (re_obj_p, magic_string_p);

  if (prop_p == NULL)
  {
    prop_p = ecma_create_named_data_property (re_obj_p,
                                              magic_string_p,
                                              true, false, false);
  }

  ecma_deref_ecma_string (magic_string_p);

  ecma_number_t *lastindex_num_p = ecma_alloc_number ();
  *lastindex_num_p = ECMA_NUMBER_ZERO;
  JERRY_ASSERT (prop_p->type == ECMA_PROPERTY_NAMEDDATA);
  ecma_named_data_property_assign_value (re_obj_p, prop_p, ecma_make_number_value (lastindex_num_p));
  ecma_dealloc_number (lastindex_num_p);
} /* re_initialize_props */

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

  re_initialize_props (obj_p, pattern_p, flags);

  /* Set bytecode internal property. */
  ecma_property_t *bytecode_prop_p;
  bytecode_prop_p = ecma_create_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_REGEXP_BYTECODE);

  /* Compile bytecode. */
  re_bytecode_t *bc_p = NULL;
  ECMA_TRY_CATCH (empty, re_compile_bytecode (&bc_p, pattern_p, flags), ret_value);

  ECMA_SET_POINTER (bytecode_prop_p->u.internal_property.value, bc_p);
  ret_value = ecma_make_normal_completion_value (ecma_make_object_value (obj_p));

  ECMA_FINALIZE (empty);

  if (ecma_is_completion_value_throw (ret_value))
  {
    ecma_deref_object (obj_p);
  }

  return ret_value;
} /* ecma_op_create_regexp_object */

/**
 * RegExp Canonicalize abstract operation
 *
 * See also: ECMA-262 v5, 15.10.2.8
 *
 * @return ecma_char_t canonicalized character
 */
ecma_char_t __attr_always_inline___
re_canonicalize (ecma_char_t ch, /**< character */
                 bool is_ignorecase) /**< IgnoreCase flag */
{
  ecma_char_t ret_value = ch;

  if (is_ignorecase)
  {
    if (ch < 128)
    {
      /* ASCII fast path. */
      if (ch >= LIT_CHAR_LOWERCASE_A && ch <= LIT_CHAR_LOWERCASE_Z)
      {
        ret_value = (ecma_char_t) (ch - (LIT_CHAR_LOWERCASE_A - LIT_CHAR_UPPERCASE_A));
      }
    }
    else
    {
      /* 2. */
      ecma_char_t u[LIT_MAXIMUM_OTHER_CASE_LENGTH];
      lit_utf8_size_t size = lit_char_to_upper_case (ch, u, LIT_MAXIMUM_OTHER_CASE_LENGTH);

      /* 3. */
      if (size == 1)
      {
        /* 4. */
        ecma_char_t cu = u[0];
        /* 5. */
        if (cu >= 128)
        {
          /* 6. */
          ret_value = cu;
        }
      }
    }
  }

  return ret_value;
} /* re_canonicalize */

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
                 lit_utf8_byte_t *str_p, /**< input string pointer */
                 lit_utf8_byte_t **out_str_p) /**< Output: matching substring iterator */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  re_opcode_t op;

  lit_utf8_byte_t *str_curr_p = str_p;

  while ((op = re_get_opcode (&bc_p)))
  {
    switch (op)
    {
      case RE_OP_MATCH:
      {
        JERRY_DDLOG ("Execute RE_OP_MATCH: match\n");
        *out_str_p = str_curr_p;
        ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
        return ret_value; /* match */
      }
      case RE_OP_CHAR:
      {
        if (str_curr_p >= re_ctx_p->input_end_p)
        {
          return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        bool is_ignorecase = re_ctx_p->flags & RE_FLAG_IGNORE_CASE;
        ecma_char_t ch1 = (ecma_char_t) re_get_value (&bc_p); /* Already canonicalized. */
        ecma_char_t ch2 = re_canonicalize (lit_utf8_read_next (&str_curr_p), is_ignorecase);
        JERRY_DDLOG ("Character matching %d to %d: ", ch1, ch2);

        if (ch1 != ch2)
        {
          JERRY_DDLOG ("fail\n");
          return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        JERRY_DDLOG ("match\n");

        break; /* tail merge */
      }
      case RE_OP_PERIOD:
      {
        if (str_curr_p >= re_ctx_p->input_end_p)
        {
          return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        ecma_char_t ch = lit_utf8_read_next (&str_curr_p);
        JERRY_DDLOG ("Period matching '.' to %d: ", (uint32_t) ch);

        if (lit_char_is_line_terminator (ch))
        {
          JERRY_DDLOG ("fail\n");
          return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        JERRY_DDLOG ("match\n");
        break; /* tail merge */
      }
      case RE_OP_ASSERT_START:
      {
        JERRY_DDLOG ("Execute RE_OP_ASSERT_START: ");

        if (str_curr_p <= re_ctx_p->input_start_p)
        {
          JERRY_DDLOG ("match\n");
          break;
        }

        if (!(re_ctx_p->flags & RE_FLAG_MULTILINE))
        {
          JERRY_DDLOG ("fail\n");
          return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        if (lit_char_is_line_terminator (lit_utf8_peek_prev (str_curr_p)))
        {
          JERRY_DDLOG ("match\n");
          break;
        }

        JERRY_DDLOG ("fail\n");
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_ASSERT_END:
      {
        JERRY_DDLOG ("Execute RE_OP_ASSERT_END: ");

        if (str_curr_p >= re_ctx_p->input_end_p)
        {
          JERRY_DDLOG ("match\n");
          break; /* tail merge */
        }

        if (!(re_ctx_p->flags & RE_FLAG_MULTILINE))
        {
          JERRY_DDLOG ("fail\n");
          return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        if (lit_char_is_line_terminator (lit_utf8_peek_next (str_curr_p)))
        {
          JERRY_DDLOG ("match\n");
          break; /* tail merge */
        }

        JERRY_DDLOG ("fail\n");
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_ASSERT_WORD_BOUNDARY:
      case RE_OP_ASSERT_NOT_WORD_BOUNDARY:
      {
        bool is_wordchar_left, is_wordchar_right;

        if (str_curr_p <= re_ctx_p->input_start_p)
        {
          is_wordchar_left = false;  /* not a wordchar */
        }
        else
        {
          is_wordchar_left = lit_char_is_word_char (lit_utf8_peek_prev (str_curr_p));
        }

        if (str_curr_p >= re_ctx_p->input_end_p)
        {
          is_wordchar_right = false;  /* not a wordchar */
        }
        else
        {
          is_wordchar_right = lit_char_is_word_char (lit_utf8_peek_next (str_curr_p));
        }

        if (op == RE_OP_ASSERT_WORD_BOUNDARY)
        {
          JERRY_DDLOG ("Execute RE_OP_ASSERT_WORD_BOUNDARY: ");
          if (is_wordchar_left == is_wordchar_right)
          {
            JERRY_DDLOG ("fail\n");
            return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }
        else
        {
          JERRY_ASSERT (op == RE_OP_ASSERT_NOT_WORD_BOUNDARY);
          JERRY_DDLOG ("Execute RE_OP_ASSERT_NOT_WORD_BOUNDARY: ");

          if (is_wordchar_left != is_wordchar_right)
          {
            JERRY_DDLOG ("fail\n");
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
        lit_utf8_byte_t *sub_str_p = NULL;

        uint32_t array_size = re_ctx_p->num_of_captures + re_ctx_p->num_of_non_captures;
        MEM_DEFINE_LOCAL_ARRAY (saved_bck_p, array_size, lit_utf8_byte_t *);

        size_t size = (size_t) (array_size) * sizeof (lit_utf8_byte_t *);
        memcpy (saved_bck_p, re_ctx_p->saved_p, size);

        do
        {
          uint32_t offset = re_get_value (&bc_p);

          if (!sub_str_p)
          {
            match_value = re_match_regexp (re_ctx_p, bc_p, str_curr_p, &sub_str_p);
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
            match_value = re_match_regexp (re_ctx_p, bc_p, str_curr_p, &sub_str_p);
          }
          else
          {
            JERRY_DDLOG ("fail\n");
            match_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }

        if (!ecma_is_completion_value_throw (match_value))
        {
          if (ecma_is_value_true (match_value))
          {
            *out_str_p = sub_str_p;
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
        uint32_t num_of_ranges;
        bool is_match;

        JERRY_DDLOG ("Execute RE_OP_CHAR_CLASS/RE_OP_INV_CHAR_CLASS, ");
        if (str_curr_p >= re_ctx_p->input_end_p)
        {
          JERRY_DDLOG ("fail\n");
          return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        bool is_ignorecase = re_ctx_p->flags & RE_FLAG_IGNORE_CASE;
        ecma_char_t curr_ch = re_canonicalize (lit_utf8_read_next (&str_curr_p), is_ignorecase);

        num_of_ranges = re_get_value (&bc_p);
        is_match = false;

        while (num_of_ranges)
        {
          ecma_char_t ch1 = re_canonicalize ((ecma_char_t) re_get_value (&bc_p), is_ignorecase);
          ecma_char_t ch2 = re_canonicalize ((ecma_char_t) re_get_value (&bc_p), is_ignorecase);
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
            return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }
        else
        {
          JERRY_ASSERT (op == RE_OP_INV_CHAR_CLASS);
          if (is_match)
          {
            JERRY_DDLOG ("fail\n");
            return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }
        JERRY_DDLOG ("match\n");
        break; /* tail merge */
      }
      case RE_OP_BACKREFERENCE:
      {
        uint32_t backref_idx;

        backref_idx = re_get_value (&bc_p);
        JERRY_DDLOG ("Execute RE_OP_BACKREFERENCE (idx: %d): ", backref_idx);
        backref_idx *= 2;  /* backref n -> saved indices [n*2, n*2+1] */
        JERRY_ASSERT (backref_idx >= 2 && backref_idx + 1 < re_ctx_p->num_of_captures);

        if (!re_ctx_p->saved_p[backref_idx] || !re_ctx_p->saved_p[backref_idx + 1])
        {
          JERRY_DDLOG ("match\n");
          break; /* capture is 'undefined', always matches! */
        }

        lit_utf8_byte_t *sub_str_p = re_ctx_p->saved_p[backref_idx];

        while (sub_str_p < re_ctx_p->saved_p[backref_idx + 1])
        {
          ecma_char_t ch1, ch2;

          if (str_curr_p >= re_ctx_p->input_end_p)
          {
            JERRY_DDLOG ("fail\n");
            return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }

          ch1 = lit_utf8_read_next (&sub_str_p);
          ch2 = lit_utf8_read_next (&str_curr_p);

          if (ch1 != ch2)
          {
            JERRY_DDLOG ("fail\n");
            return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }
        JERRY_DDLOG ("match\n");
        break; /* tail merge */
      }
      case RE_OP_SAVE_AT_START:
      {
        re_bytecode_t *old_bc_p;

        JERRY_DDLOG ("Execute RE_OP_SAVE_AT_START\n");
        lit_utf8_byte_t *old_start_p = re_ctx_p->saved_p[RE_GLOBAL_START_IDX];
        re_ctx_p->saved_p[RE_GLOBAL_START_IDX] = str_curr_p;

        do
        {
          uint32_t offset = re_get_value (&bc_p);
          lit_utf8_byte_t *sub_str_p = NULL;
          ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_curr_p, &sub_str_p);
          if (ecma_is_value_true (match_value))
          {
            *out_str_p = sub_str_p;
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
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_SAVE_AND_MATCH:
      {
        JERRY_DDLOG ("End of pattern is reached: match\n");
        re_ctx_p->saved_p[RE_GLOBAL_END_IDX] = str_curr_p;
        *out_str_p = str_curr_p;
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE); /* match */
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
        lit_utf8_byte_t *old_start_p = NULL;
        lit_utf8_byte_t *sub_str_p = NULL;
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
          re_ctx_p->saved_p[start_idx] = str_curr_p;
        }
        else
        {
          JERRY_ASSERT (start_idx < re_ctx_p->num_of_non_captures);
          iter_idx = start_idx + (re_ctx_p->num_of_captures / 2) - 1;
          start_idx += re_ctx_p->num_of_captures;
        }
        re_ctx_p->num_of_iterations_p[iter_idx] = 0;

        /* Jump all over to the end of the END opcode. */
        bc_p += offset;

        /* Try to match after the close paren if zero is allowed */
        ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_curr_p, &sub_str_p);

        if (ecma_is_value_true (match_value))
        {
          *out_str_p = sub_str_p;
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
        lit_utf8_byte_t *sub_str_p = NULL;
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

        lit_utf8_byte_t *old_start_p = re_ctx_p->saved_p[start_idx];
        old_iteration_cnt = re_ctx_p->num_of_iterations_p[iter_idx];
        re_ctx_p->saved_p[start_idx] = str_curr_p;
        re_ctx_p->num_of_iterations_p[iter_idx] = 0;

        do
        {
          offset = re_get_value (&bc_p);
          ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_curr_p, &sub_str_p);
          if (ecma_is_value_true (match_value))
          {
            *out_str_p = sub_str_p;
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
        re_ctx_p->num_of_iterations_p[iter_idx] = old_iteration_cnt;

        /* Try to match after the close paren if zero is allowed. */
        if (op == RE_OP_CAPTURE_GREEDY_ZERO_GROUP_START
            || op == RE_OP_NON_CAPTURE_GREEDY_ZERO_GROUP_START)
        {
          JERRY_ASSERT (end_bc_p);
          ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, end_bc_p, str_curr_p, &sub_str_p);

          if (ecma_is_value_true (match_value))
          {
            *out_str_p = sub_str_p;
            return match_value; /* match */
          }
          else if (ecma_is_completion_value_throw (match_value))
          {
            return match_value;
          }
        }

        re_ctx_p->saved_p[start_idx] = old_start_p;
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_CAPTURE_NON_GREEDY_GROUP_END:
      case RE_OP_NON_CAPTURE_NON_GREEDY_GROUP_END:
      {
        uint32_t end_idx, iter_idx, min, max;
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

        re_ctx_p->num_of_iterations_p[iter_idx]++;

        if (re_ctx_p->num_of_iterations_p[iter_idx] >= min
            && re_ctx_p->num_of_iterations_p[iter_idx] <= max)
        {
          lit_utf8_byte_t *old_end_p = re_ctx_p->saved_p[end_idx];
          re_ctx_p->saved_p[end_idx] = str_curr_p;

          lit_utf8_byte_t *sub_str_p = NULL;
          ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_curr_p, &sub_str_p);
          if (ecma_is_value_true (match_value))
          {
            *out_str_p = sub_str_p;
            return match_value; /* match */
          }
          else if (ecma_is_completion_value_throw (match_value))
          {
            return match_value;
          }

          re_ctx_p->saved_p[end_idx] = old_end_p;
        }
        re_ctx_p->num_of_iterations_p[iter_idx]--;
        bc_p = old_bc_p;

        /* If non-greedy fails and try to iterate... */
        /* FALLTHRU */
      }
      case RE_OP_CAPTURE_GREEDY_GROUP_END:
      case RE_OP_NON_CAPTURE_GREEDY_GROUP_END:
      {
        uint32_t start_idx, end_idx, iter_idx, min, max, offset;
        lit_utf8_byte_t *old_start_p = NULL;
        lit_utf8_byte_t *old_end_p = NULL;
        lit_utf8_byte_t *sub_str_p = NULL;
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
        if (re_ctx_p->num_of_iterations_p[iter_idx] >= min
            && str_curr_p== re_ctx_p->saved_p[start_idx])
        {
          return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        re_ctx_p->num_of_iterations_p[iter_idx]++;

        old_bc_p = bc_p; /* Save the bytecode end position of the END opcodes for matching after it. */
        old_end_p = re_ctx_p->saved_p[end_idx];
        re_ctx_p->saved_p[end_idx] = str_curr_p;

        if (re_ctx_p->num_of_iterations_p[iter_idx] < max)
        {
          bc_p -= offset;
          offset = re_get_value (&bc_p);

          old_start_p = re_ctx_p->saved_p[start_idx];
          re_ctx_p->saved_p[start_idx] = str_curr_p;
          ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_curr_p, &sub_str_p);

          if (ecma_is_value_true (match_value))
          {
            *out_str_p = sub_str_p;
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
            re_ctx_p->saved_p[start_idx] = str_curr_p;

            ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_curr_p, &sub_str_p);

            if (ecma_is_value_true (match_value))
            {
              *out_str_p = sub_str_p;
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

        if (re_ctx_p->num_of_iterations_p[iter_idx] >= min
            && re_ctx_p->num_of_iterations_p[iter_idx] <= max)
        {
          /* Try to match the rest of the bytecode. */
          ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, old_bc_p, str_curr_p, &sub_str_p);

          if (ecma_is_value_true (match_value))
          {
            *out_str_p = sub_str_p;
            return match_value; /* match */
          }
          else if (ecma_is_completion_value_throw (match_value))
          {
            return match_value;
          }
        }

        /* restore if fails */
        re_ctx_p->saved_p[end_idx] = old_end_p;
        re_ctx_p->num_of_iterations_p[iter_idx]--;
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_NON_GREEDY_ITERATOR:
      {
        uint32_t min, max, offset, num_of_iter;
        lit_utf8_byte_t *sub_str_p = NULL;

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
            ecma_completion_value_t match_value = re_match_regexp (re_ctx_p,
                                                                   bc_p + offset,
                                                                   str_curr_p,
                                                                   &sub_str_p);

            if (ecma_is_value_true (match_value))
            {
              *out_str_p = sub_str_p;
              return match_value; /* match */
            }
            else if (ecma_is_completion_value_throw (match_value))
            {
              return match_value;
            }
          }

          ecma_completion_value_t match_value = re_match_regexp (re_ctx_p,
                                                                 bc_p,
                                                                 str_curr_p,
                                                                 &sub_str_p);

          if (!ecma_is_value_true (match_value))
          {
            if (ecma_is_completion_value_throw (match_value))
            {
              return match_value;
            }

            break;
          }

          str_curr_p = sub_str_p;
          num_of_iter++;
        }
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_GREEDY_ITERATOR:
      {
        uint32_t min, max, offset, num_of_iter;
        lit_utf8_byte_t *sub_str_p = NULL;

        min = re_get_value (&bc_p);
        max = re_get_value (&bc_p);

        offset = re_get_value (&bc_p);
        JERRY_DDLOG ("Greedy iterator, min=%lu, max=%lu, offset=%ld\n",
                     (unsigned long) min, (unsigned long) max, (long) offset);

        num_of_iter = 0;

        while (num_of_iter < max)
        {
          ecma_completion_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_curr_p, &sub_str_p);

          if (!ecma_is_value_true (match_value))
          {
            if (ecma_is_completion_value_throw (match_value))
            {
              return match_value;
            }

            break;
          }

          str_curr_p = sub_str_p;
          num_of_iter++;
        }

        while (num_of_iter >= min)
        {
          ecma_completion_value_t match_value = re_match_regexp (re_ctx_p,
                                                                 bc_p + offset,
                                                                 str_curr_p,
                                                                 &sub_str_p);

          if (ecma_is_value_true (match_value))
          {
            *out_str_p = sub_str_p;
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

          lit_utf8_read_prev (&str_curr_p);
          num_of_iter--;
        }
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      default:
      {
        JERRY_DDLOG ("UNKNOWN opcode (%d)!\n", (uint32_t) op);
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
void
re_set_result_array_properties (ecma_object_t *array_obj_p, /**< result array */
                                ecma_string_t *input_str_p, /**< input string */
                                uint32_t num_of_elements, /**< Number of array elements */
                                int32_t index) /** index of matching */
{
  /* Set index property of the result array */
  ecma_string_t *result_prop_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_INDEX);
  {
    ecma_number_t *num_p = ecma_alloc_number ();
    *num_p = (ecma_number_t) index;

    ecma_builtin_helper_def_prop (array_obj_p,
                                  result_prop_str_p,
                                  ecma_make_number_value (num_p),
                                  true, /* Writable */
                                  true, /* Enumerable */
                                  true, /* Configurable */
                                  true); /* Failure handling */

    ecma_dealloc_number (num_p);
  }
  ecma_deref_ecma_string (result_prop_str_p);

  /* Set input property of the result array */
  result_prop_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_INPUT);

  ecma_builtin_helper_def_prop (array_obj_p,
                                result_prop_str_p,
                                ecma_make_string_value (input_str_p),
                                true, /* Writable */
                                true, /* Enumerable */
                                true, /* Configurable */
                                true); /* Failure handling */

  ecma_deref_ecma_string (result_prop_str_p);

  /* Set length property of the result array */
  result_prop_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);
  {

    ecma_property_descriptor_t array_item_prop_desc = ecma_make_empty_property_descriptor ();
    array_item_prop_desc.is_value_defined = true;

    ecma_number_t *num_p = ecma_alloc_number ();
    *num_p = (ecma_number_t) (num_of_elements);
    array_item_prop_desc.value = ecma_make_number_value (num_p);

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
ecma_regexp_exec_helper (ecma_value_t regexp_value, /**< RegExp object */
                         ecma_value_t input_string, /**< input string */
                         bool ignore_global) /**< ignore global flag */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  JERRY_ASSERT (ecma_is_value_object (regexp_value));
  JERRY_ASSERT (ecma_is_value_string (input_string));

  ecma_object_t *regexp_object_p = ecma_get_object_from_value (regexp_value);

  JERRY_ASSERT (ecma_object_get_class_name (regexp_object_p) == LIT_MAGIC_STRING_REGEXP_UL);

  ecma_property_t *bytecode_prop_p = ecma_get_internal_property (regexp_object_p,
                                                                 ECMA_INTERNAL_PROPERTY_REGEXP_BYTECODE);
  re_bytecode_t *bc_p = ECMA_GET_POINTER (re_bytecode_t, bytecode_prop_p->u.internal_property.value);

  ecma_string_t *input_string_p = ecma_get_string_from_value (input_string);
  lit_utf8_size_t input_string_size = ecma_string_get_size (input_string_p);

  MEM_DEFINE_LOCAL_ARRAY (input_buffer_p, input_string_size, lit_utf8_byte_t);

  ssize_t sz = ecma_string_to_utf8_string (input_string_p, input_buffer_p, (ssize_t) input_string_size);
  JERRY_ASSERT (sz >= 0);

  lit_utf8_byte_t *input_curr_p = input_buffer_p;

  if (!input_string_size)
  {
    input_curr_p = (lit_utf8_byte_t *) lit_get_magic_string_utf8 (LIT_MAGIC_STRING__EMPTY);
  }
  lit_utf8_byte_t *input_end_p = input_buffer_p + input_string_size;

  re_matcher_ctx_t re_ctx;
  re_ctx.input_start_p = input_buffer_p;
  re_ctx.input_end_p = input_buffer_p + input_string_size;

  /* 1. Read bytecode header and init regexp matcher context. */
  re_ctx.flags = (uint8_t) re_get_value (&bc_p);

  if (ignore_global)
  {
    re_ctx.flags &= (uint8_t) ~RE_FLAG_GLOBAL;
  }

  JERRY_DDLOG ("Exec with flags [global: %d, ignoreCase: %d, multiline: %d]\n",
               re_ctx.flags & RE_FLAG_GLOBAL,
               re_ctx.flags & RE_FLAG_IGNORE_CASE,
               re_ctx.flags & RE_FLAG_MULTILINE);

  re_ctx.num_of_captures = re_get_value (&bc_p);
  JERRY_ASSERT (re_ctx.num_of_captures % 2 == 0);
  re_ctx.num_of_non_captures = re_get_value (&bc_p);


  MEM_DEFINE_LOCAL_ARRAY (saved_p, re_ctx.num_of_captures + re_ctx.num_of_non_captures, lit_utf8_byte_t *);

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
  re_ctx.num_of_iterations_p = num_of_iter_p;
  int32_t index = 0;
  ecma_length_t input_str_len = lit_utf8_string_length (input_buffer_p, input_string_size);

  if (input_buffer_p && (re_ctx.flags & RE_FLAG_GLOBAL))
  {
    ecma_string_t *magic_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL);
    ecma_property_t *lastindex_prop_p = ecma_op_object_get_property (regexp_object_p, magic_str_p);

    ECMA_OP_TO_NUMBER_TRY_CATCH (lastindex_num, lastindex_prop_p->u.named_data_property.value, ret_value)
    index = ecma_number_to_int32 (lastindex_num);

    if (input_curr_p < input_end_p
        && index <= (int32_t) input_str_len
        && index > 0)
    {
      for (int i = 0; i < index; i++)
      {
        lit_utf8_incr (&input_curr_p);
      }
    }
    ECMA_OP_TO_NUMBER_FINALIZE (lastindex_num);
    ecma_deref_ecma_string (magic_str_p);
  }

  /* 2. Try to match */
  lit_utf8_byte_t *sub_str_p = NULL;

  while (ecma_is_completion_value_empty (ret_value))
  {
    if (index < 0 || index > (int32_t) input_str_len)
    {
      if (re_ctx.flags & RE_FLAG_GLOBAL)
      {
        ecma_string_t *magic_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL);
        ecma_number_t *lastindex_num_p = ecma_alloc_number ();
        *lastindex_num_p = ECMA_NUMBER_ZERO;
        ecma_op_object_put (regexp_object_p, magic_str_p, ecma_make_number_value (lastindex_num_p), true);
        ecma_dealloc_number (lastindex_num_p);
        ecma_deref_ecma_string (magic_str_p);
      }

      is_match = false;
      break;
    }
    else
    {
      ECMA_TRY_CATCH (match_value, re_match_regexp (&re_ctx, bc_p, input_curr_p, &sub_str_p), ret_value);
      if (ecma_is_value_true (match_value))
      {
        is_match = true;
        break;
      }

      if (input_curr_p < input_end_p)
      {
        lit_utf8_incr (&input_curr_p);
      }
      index++;

      ECMA_FINALIZE (match_value);
    }
  }

  if (input_curr_p && (re_ctx.flags & RE_FLAG_GLOBAL))
  {
    ecma_string_t *magic_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL);
    ecma_number_t *lastindex_num_p = ecma_alloc_number ();

    if (sub_str_p)
    {
      *lastindex_num_p = lit_utf8_string_length (input_buffer_p,
                                                  (lit_utf8_size_t) (sub_str_p - input_buffer_p));
    }
    else
    {
      *lastindex_num_p = ECMA_NUMBER_ZERO;
    }

    ecma_op_object_put (regexp_object_p, magic_str_p, ecma_make_number_value (lastindex_num_p), true);
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

      ecma_string_t *input_str_p = ecma_new_ecma_string_from_utf8 (input_buffer_p, input_string_size);
      re_set_result_array_properties (result_array_obj_p, input_str_p, re_ctx.num_of_captures / 2, index);
      ecma_deref_ecma_string (input_str_p);

      for (uint32_t i = 0; i < re_ctx.num_of_captures; i += 2)
      {
        ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (i / 2);

        if (((re_ctx.saved_p[i] && re_ctx.saved_p[i + 1])
             && re_ctx.saved_p[i + 1] >= re_ctx.saved_p[i]))
        {
          ecma_length_t capture_str_len;
          capture_str_len = (ecma_length_t) (re_ctx.saved_p[i + 1] - re_ctx.saved_p[i]);
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
      ret_value = ecma_make_normal_completion_value (ecma_make_simple_value (ECMA_SIMPLE_VALUE_NULL));
    }
  }

  MEM_FINALIZE_LOCAL_ARRAY (num_of_iter_p);
  MEM_FINALIZE_LOCAL_ARRAY (saved_p);
  MEM_FINALIZE_LOCAL_ARRAY (input_buffer_p);

  return ret_value;
} /* ecma_regexp_exec_helper */

/**
 * @}
 * @}
 */

#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */
