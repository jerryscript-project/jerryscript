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

#ifndef CONFIG_DISABLE_REGEXP_BUILTIN

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
 * Check if a RegExp opcode is a capture group or not
 */
#define RE_IS_CAPTURE_GROUP(x) (((x) < RE_OP_NON_CAPTURE_GROUP_START) ? 1 : 0)

/**
 * Parse RegExp flags (global, ignoreCase, multiline)
 *
 * See also: ECMA-262 v5, 15.10.4.1
 *
 * @return empty ecma value - if parsed successfully
 *         error ecma value - otherwise
 *
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
re_parse_regexp_flags (ecma_string_t *flags_str_p, /**< Input string with flags */
                       uint16_t *flags_p) /**< [out] parsed flag bits */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_STRING_TO_UTF8_STRING (flags_str_p, flags_start_p, flags_start_size);

  const lit_utf8_byte_t *flags_str_curr_p = flags_start_p;
  const lit_utf8_byte_t *flags_str_end_p = flags_start_p + flags_start_size;

  while (flags_str_curr_p < flags_str_end_p
         && ecma_is_value_empty (ret_value))
  {
    switch (*flags_str_curr_p++)
    {
      case 'g':
      {
        if (*flags_p & RE_FLAG_GLOBAL)
        {
          ret_value = ecma_raise_syntax_error (ECMA_ERR_MSG ("Invalid RegExp flags."));
        }
        *flags_p |= RE_FLAG_GLOBAL;
        break;
      }
      case 'i':
      {
        if (*flags_p & RE_FLAG_IGNORE_CASE)
        {
          ret_value = ecma_raise_syntax_error (ECMA_ERR_MSG ("Invalid RegExp flags."));
        }
        *flags_p |= RE_FLAG_IGNORE_CASE;
        break;
      }
      case 'm':
      {
        if (*flags_p & RE_FLAG_MULTILINE)
        {
          ret_value = ecma_raise_syntax_error (ECMA_ERR_MSG ("Invalid RegExp flags."));
        }
        *flags_p |= RE_FLAG_MULTILINE;
        break;
      }
      default:
      {
        ret_value = ecma_raise_syntax_error (ECMA_ERR_MSG ("Invalid RegExp flags."));
        break;
      }
    }
  }

  ECMA_FINALIZE_UTF8_STRING (flags_start_p, flags_start_size);

  return ret_value;
} /* re_parse_regexp_flags  */

/**
 * Set a data property value for a regexp object.
 */
static void
re_set_data_property (ecma_object_t *re_object_p, /**< RegExp object */
                      ecma_string_t *property_name_p, /**< property name */
                      uint8_t prop_attributes, /**< property attributes */
                      ecma_value_t value) /**< property value */
{
  ecma_property_ref_t property_ref;
  ecma_property_t property = ecma_op_object_get_own_property (re_object_p,
                                                              property_name_p,
                                                              &property_ref,
                                                              ECMA_PROPERTY_GET_VALUE);

  if (property == ECMA_PROPERTY_TYPE_NOT_FOUND)
  {
    property_ref.value_p = ecma_create_named_data_property (re_object_p,
                                                            property_name_p,
                                                            prop_attributes,
                                                            NULL);
  }
  else
  {
    JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (property) == ECMA_PROPERTY_TYPE_NAMEDDATA
                  && !ecma_is_property_configurable (property));
  }

  ecma_named_data_property_assign_value (re_object_p, property_ref.value_p, value);
} /* re_set_data_property */

/**
 * Initializes the source, global, ignoreCase, multiline, and lastIndex properties of RegExp instance.
 */
void
re_initialize_props (ecma_object_t *re_obj_p, /**< RegExp object */
                     ecma_string_t *source_p, /**< source string */
                     uint16_t flags) /**< flags */
{
 /* Set source property. ECMA-262 v5, 15.10.7.1 */
  ecma_string_t *magic_string_p;

  magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_SOURCE);
  re_set_data_property (re_obj_p,
                        magic_string_p,
                        ECMA_PROPERTY_FIXED,
                        ecma_make_string_value (source_p));
  ecma_deref_ecma_string (magic_string_p);

  /* Set global property. ECMA-262 v5, 15.10.7.2 */
  magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_GLOBAL);
  re_set_data_property (re_obj_p,
                        magic_string_p,
                        ECMA_PROPERTY_FIXED,
                        ecma_make_boolean_value (flags & RE_FLAG_GLOBAL));
  ecma_deref_ecma_string (magic_string_p);

  /* Set ignoreCase property. ECMA-262 v5, 15.10.7.3 */
  magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_IGNORECASE_UL);
  re_set_data_property (re_obj_p,
                        magic_string_p,
                        ECMA_PROPERTY_FIXED,
                        ecma_make_boolean_value (flags & RE_FLAG_IGNORE_CASE));
  ecma_deref_ecma_string (magic_string_p);

  /* Set multiline property. ECMA-262 v5, 15.10.7.4 */
  magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_MULTILINE);
  re_set_data_property (re_obj_p,
                        magic_string_p,
                        ECMA_PROPERTY_FIXED,
                        ecma_make_boolean_value (flags & RE_FLAG_MULTILINE));
  ecma_deref_ecma_string (magic_string_p);

  /* Set lastIndex property. ECMA-262 v5, 15.10.7.5 */
  magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL);
  re_set_data_property (re_obj_p,
                        magic_string_p,
                        ECMA_PROPERTY_FLAG_WRITABLE,
                        ecma_make_integer_value (0));
  ecma_deref_ecma_string (magic_string_p);
} /* re_initialize_props */

/**
 * RegExp object creation operation.
 *
 * See also: ECMA-262 v5, 15.10.4.1
 *
 * @return constructed RegExp object
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_create_regexp_object_from_bytecode (re_compiled_code_t *bytecode_p) /**< RegExp bytecode */
{
  JERRY_ASSERT (bytecode_p != NULL);

  ecma_object_t *re_prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_REGEXP_PROTOTYPE);

  ecma_object_t *object_p = ecma_create_object (re_prototype_obj_p,
                                                sizeof (ecma_extended_object_t),
                                                ECMA_OBJECT_TYPE_CLASS);

  ecma_deref_object (re_prototype_obj_p);

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

  /* Set the internal [[Class]] property */
  ext_object_p->u.class_prop.class_id = LIT_MAGIC_STRING_REGEXP_UL;

  /* Set bytecode internal property. */
  ECMA_SET_INTERNAL_VALUE_POINTER (ext_object_p->u.class_prop.u.value, bytecode_p);
  ecma_bytecode_ref ((ecma_compiled_code_t *) bytecode_p);

  /* Initialize RegExp object properties */
  re_initialize_props (object_p,
                       ECMA_GET_NON_NULL_POINTER (ecma_string_t, bytecode_p->pattern_cp),
                       bytecode_p->header.status_flags);

  return ecma_make_object_value (object_p);
} /* ecma_op_create_regexp_object_from_bytecode */

/**
 * RegExp object creation operation.
 *
 * See also: ECMA-262 v5, 15.10.4.1
 *
 * @return constructed RegExp object - if pattern and flags were parsed successfully
 *         error ecma value          - otherwise
 *
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_create_regexp_object (ecma_string_t *pattern_p, /**< input pattern */
                              ecma_string_t *flags_str_p) /**< flags */
{
  JERRY_ASSERT (pattern_p != NULL);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  uint16_t flags = 0;

  if (flags_str_p != NULL)
  {
    ECMA_TRY_CATCH (empty, re_parse_regexp_flags (flags_str_p, &flags), ret_value);
    ECMA_FINALIZE (empty);

    if (!ecma_is_value_empty (ret_value))
    {
      return ret_value;
    }
  }

  ecma_object_t *re_prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_REGEXP_PROTOTYPE);

  ecma_object_t *object_p = ecma_create_object (re_prototype_obj_p,
                                                sizeof (ecma_extended_object_t),
                                                ECMA_OBJECT_TYPE_CLASS);

  ecma_deref_object (re_prototype_obj_p);

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  ext_object_p->u.class_prop.class_id = LIT_MAGIC_STRING_UNDEFINED;

  re_initialize_props (object_p, pattern_p, flags);

  /* Compile bytecode. */
  const re_compiled_code_t *bc_p = NULL;
  ECMA_TRY_CATCH (empty, re_compile_bytecode (&bc_p, pattern_p, flags), ret_value);

  /* Set [[Class]] and bytecode internal properties. */
  ext_object_p->u.class_prop.class_id = LIT_MAGIC_STRING_REGEXP_UL;
  ECMA_SET_INTERNAL_VALUE_POINTER (ext_object_p->u.class_prop.u.value, bc_p);

  ret_value = ecma_make_object_value (object_p);

  ECMA_FINALIZE (empty);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    ecma_deref_object (object_p);
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
inline ecma_char_t __attr_always_inline___
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
      ecma_length_t size = lit_char_to_upper_case (ch, u, LIT_MAXIMUM_OTHER_CASE_LENGTH);

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
 * @return true  - if matched
 *         false - otherwise
 *
 *         May raise error, so returned value must be freed with ecma_free_value
 */
static ecma_value_t
re_match_regexp (re_matcher_ctx_t *re_ctx_p, /**< RegExp matcher context */
                 uint8_t *bc_p, /**< pointer to the current RegExp bytecode */
                 const lit_utf8_byte_t *str_p, /**< input string pointer */
                 const lit_utf8_byte_t **out_str_p) /**< [out] matching substring iterator */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  re_opcode_t op;

  const lit_utf8_byte_t *str_curr_p = str_p;

  while ((op = re_get_opcode (&bc_p)))
  {
    switch (op)
    {
      case RE_OP_MATCH:
      {
        JERRY_TRACE_MSG ("Execute RE_OP_MATCH: match\n");
        *out_str_p = str_curr_p;
        ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
        return ret_value; /* match */
      }
      case RE_OP_CHAR:
      {
        if (str_curr_p >= re_ctx_p->input_end_p)
        {
          return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        bool is_ignorecase = re_ctx_p->flags & RE_FLAG_IGNORE_CASE;
        ecma_char_t ch1 = (ecma_char_t) re_get_char (&bc_p); /* Already canonicalized. */
        ecma_char_t ch2 = re_canonicalize (lit_utf8_read_next (&str_curr_p), is_ignorecase);
        JERRY_TRACE_MSG ("Character matching %d to %d: ", ch1, ch2);

        if (ch1 != ch2)
        {
          JERRY_TRACE_MSG ("fail\n");
          return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        JERRY_TRACE_MSG ("match\n");

        break; /* tail merge */
      }
      case RE_OP_PERIOD:
      {
        if (str_curr_p >= re_ctx_p->input_end_p)
        {
          return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        ecma_char_t ch = lit_utf8_read_next (&str_curr_p);
        JERRY_TRACE_MSG ("Period matching '.' to %u: ", (unsigned int) ch);

        if (lit_char_is_line_terminator (ch))
        {
          JERRY_TRACE_MSG ("fail\n");
          return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        JERRY_TRACE_MSG ("match\n");
        break; /* tail merge */
      }
      case RE_OP_ASSERT_START:
      {
        JERRY_TRACE_MSG ("Execute RE_OP_ASSERT_START: ");

        if (str_curr_p <= re_ctx_p->input_start_p)
        {
          JERRY_TRACE_MSG ("match\n");
          break; /* tail merge */
        }

        if (!(re_ctx_p->flags & RE_FLAG_MULTILINE))
        {
          JERRY_TRACE_MSG ("fail\n");
          return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        if (lit_char_is_line_terminator (lit_utf8_peek_prev (str_curr_p)))
        {
          JERRY_TRACE_MSG ("match\n");
          break; /* tail merge */
        }

        JERRY_TRACE_MSG ("fail\n");
        return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_ASSERT_END:
      {
        JERRY_TRACE_MSG ("Execute RE_OP_ASSERT_END: ");

        if (str_curr_p >= re_ctx_p->input_end_p)
        {
          JERRY_TRACE_MSG ("match\n");
          break; /* tail merge */
        }

        if (!(re_ctx_p->flags & RE_FLAG_MULTILINE))
        {
          JERRY_TRACE_MSG ("fail\n");
          return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        if (lit_char_is_line_terminator (lit_utf8_peek_next (str_curr_p)))
        {
          JERRY_TRACE_MSG ("match\n");
          break; /* tail merge */
        }

        JERRY_TRACE_MSG ("fail\n");
        return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
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
          JERRY_TRACE_MSG ("Execute RE_OP_ASSERT_WORD_BOUNDARY: ");
          if (is_wordchar_left == is_wordchar_right)
          {
            JERRY_TRACE_MSG ("fail\n");
            return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }
        else
        {
          JERRY_ASSERT (op == RE_OP_ASSERT_NOT_WORD_BOUNDARY);
          JERRY_TRACE_MSG ("Execute RE_OP_ASSERT_NOT_WORD_BOUNDARY: ");

          if (is_wordchar_left != is_wordchar_right)
          {
            JERRY_TRACE_MSG ("fail\n");
            return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }

        JERRY_TRACE_MSG ("match\n");
        break; /* tail merge */
      }
      case RE_OP_LOOKAHEAD_POS:
      case RE_OP_LOOKAHEAD_NEG:
      {
        ecma_value_t match_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
        const lit_utf8_byte_t *sub_str_p = NULL;

        uint32_t array_size = re_ctx_p->num_of_captures + re_ctx_p->num_of_non_captures;
        JMEM_DEFINE_LOCAL_ARRAY (saved_bck_p, array_size, lit_utf8_byte_t *);

        size_t size = (size_t) (array_size) * sizeof (lit_utf8_byte_t *);
        memcpy (saved_bck_p, re_ctx_p->saved_p, size);

        do
        {
          uint32_t offset = re_get_value (&bc_p);

          if (!sub_str_p)
          {
            match_value = re_match_regexp (re_ctx_p, bc_p, str_curr_p, &sub_str_p);
            if (ECMA_IS_VALUE_ERROR (match_value))
            {
              break;
            }
          }
          bc_p += offset;
        }
        while (re_get_opcode (&bc_p) == RE_OP_ALTERNATIVE);

        if (!ECMA_IS_VALUE_ERROR (match_value))
        {
          JERRY_TRACE_MSG ("Execute RE_OP_LOOKAHEAD_POS/NEG: ");
          ecma_free_value (match_value);
          if ((op == RE_OP_LOOKAHEAD_POS && sub_str_p)
              || (op == RE_OP_LOOKAHEAD_NEG && !sub_str_p))
          {
            JERRY_TRACE_MSG ("match\n");
            match_value = re_match_regexp (re_ctx_p, bc_p, str_curr_p, &sub_str_p);
          }
          else
          {
            JERRY_TRACE_MSG ("fail\n");
            match_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }

        if (!ECMA_IS_VALUE_ERROR (match_value))
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

        JMEM_FINALIZE_LOCAL_ARRAY (saved_bck_p);
        return match_value;
      }
      case RE_OP_CHAR_CLASS:
      case RE_OP_INV_CHAR_CLASS:
      {
        uint32_t num_of_ranges;
        bool is_match;

        JERRY_TRACE_MSG ("Execute RE_OP_CHAR_CLASS/RE_OP_INV_CHAR_CLASS, ");
        if (str_curr_p >= re_ctx_p->input_end_p)
        {
          JERRY_TRACE_MSG ("fail\n");
          return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        bool is_ignorecase = re_ctx_p->flags & RE_FLAG_IGNORE_CASE;
        ecma_char_t curr_ch = re_canonicalize (lit_utf8_read_next (&str_curr_p), is_ignorecase);

        num_of_ranges = re_get_value (&bc_p);
        is_match = false;

        while (num_of_ranges)
        {
          ecma_char_t ch1 = re_canonicalize (re_get_char (&bc_p), is_ignorecase);
          ecma_char_t ch2 = re_canonicalize (re_get_char (&bc_p), is_ignorecase);
          JERRY_TRACE_MSG ("num_of_ranges=%u, ch1=%u, ch2=%u, curr_ch=%u; ",
                           (unsigned int) num_of_ranges, (unsigned int) ch1,
                           (unsigned int) ch2, (unsigned int) curr_ch);

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
            JERRY_TRACE_MSG ("fail\n");
            return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }
        else
        {
          JERRY_ASSERT (op == RE_OP_INV_CHAR_CLASS);
          if (is_match)
          {
            JERRY_TRACE_MSG ("fail\n");
            return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }
        JERRY_TRACE_MSG ("match\n");
        break; /* tail merge */
      }
      case RE_OP_BACKREFERENCE:
      {
        uint32_t backref_idx;

        backref_idx = re_get_value (&bc_p);
        JERRY_TRACE_MSG ("Execute RE_OP_BACKREFERENCE (idx: %u): ", (unsigned int) backref_idx);
        backref_idx *= 2;  /* backref n -> saved indices [n*2, n*2+1] */
        JERRY_ASSERT (backref_idx >= 2 && backref_idx + 1 < re_ctx_p->num_of_captures);

        if (!re_ctx_p->saved_p[backref_idx] || !re_ctx_p->saved_p[backref_idx + 1])
        {
          JERRY_TRACE_MSG ("match\n");
          break; /* capture is 'undefined', always matches! */
        }

        const lit_utf8_byte_t *sub_str_p = re_ctx_p->saved_p[backref_idx];

        while (sub_str_p < re_ctx_p->saved_p[backref_idx + 1])
        {
          ecma_char_t ch1, ch2;

          if (str_curr_p >= re_ctx_p->input_end_p)
          {
            JERRY_TRACE_MSG ("fail\n");
            return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }

          ch1 = lit_utf8_read_next (&sub_str_p);
          ch2 = lit_utf8_read_next (&str_curr_p);

          if (ch1 != ch2)
          {
            JERRY_TRACE_MSG ("fail\n");
            return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }
        JERRY_TRACE_MSG ("match\n");
        break; /* tail merge */
      }
      case RE_OP_SAVE_AT_START:
      {
        uint8_t *old_bc_p;

        JERRY_TRACE_MSG ("Execute RE_OP_SAVE_AT_START\n");
        const lit_utf8_byte_t *old_start_p = re_ctx_p->saved_p[RE_GLOBAL_START_IDX];
        re_ctx_p->saved_p[RE_GLOBAL_START_IDX] = str_curr_p;

        do
        {
          uint32_t offset = re_get_value (&bc_p);
          const lit_utf8_byte_t *sub_str_p = NULL;
          ecma_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_curr_p, &sub_str_p);

          if (ecma_is_value_true (match_value))
          {
            *out_str_p = sub_str_p;
            return match_value; /* match */
          }
          else if (ECMA_IS_VALUE_ERROR (match_value))
          {
            return match_value;
          }

          bc_p += offset;
          old_bc_p = bc_p;
        }
        while (re_get_opcode (&bc_p) == RE_OP_ALTERNATIVE);
        bc_p = old_bc_p;

        re_ctx_p->saved_p[RE_GLOBAL_START_IDX] = old_start_p;
        return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_SAVE_AND_MATCH:
      {
        JERRY_TRACE_MSG ("End of pattern is reached: match\n");
        re_ctx_p->saved_p[RE_GLOBAL_END_IDX] = str_curr_p;
        *out_str_p = str_curr_p;
        return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE); /* match */
      }
      case RE_OP_ALTERNATIVE:
      {
        /*
        *  Alternatives should be jump over, when alternative opcode appears.
        */
        uint32_t offset = re_get_value (&bc_p);
        JERRY_TRACE_MSG ("Execute RE_OP_ALTERNATIVE");
        bc_p += offset;

        while (*bc_p == RE_OP_ALTERNATIVE)
        {
          JERRY_TRACE_MSG (", jump: %u", (unsigned int) offset);
          bc_p++;
          offset = re_get_value (&bc_p);
          bc_p += offset;
        }

        JERRY_TRACE_MSG ("\n");
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
        const lit_utf8_byte_t *old_start_p = NULL;
        const lit_utf8_byte_t *sub_str_p = NULL;
        uint8_t *old_bc_p;

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
        ecma_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_curr_p, &sub_str_p);

        if (ecma_is_value_true (match_value))
        {
          *out_str_p = sub_str_p;
          return match_value; /* match */
        }
        else if (ECMA_IS_VALUE_ERROR (match_value))
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
        const lit_utf8_byte_t *sub_str_p = NULL;
        uint8_t *old_bc_p;
        uint8_t *end_bc_p = NULL;
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

        const lit_utf8_byte_t *old_start_p = re_ctx_p->saved_p[start_idx];
        old_iteration_cnt = re_ctx_p->num_of_iterations_p[iter_idx];
        re_ctx_p->saved_p[start_idx] = str_curr_p;
        re_ctx_p->num_of_iterations_p[iter_idx] = 0;

        do
        {
          offset = re_get_value (&bc_p);
          ecma_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_curr_p, &sub_str_p);

          if (ecma_is_value_true (match_value))
          {
            *out_str_p = sub_str_p;
            return match_value; /* match */
          }
          else if (ECMA_IS_VALUE_ERROR (match_value))
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
          ecma_value_t match_value = re_match_regexp (re_ctx_p, end_bc_p, str_curr_p, &sub_str_p);

          if (ecma_is_value_true (match_value))
          {
            *out_str_p = sub_str_p;
            return match_value; /* match */
          }
          else if (ECMA_IS_VALUE_ERROR (match_value))
          {
            return match_value;
          }
        }

        re_ctx_p->saved_p[start_idx] = old_start_p;
        return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_CAPTURE_NON_GREEDY_GROUP_END:
      case RE_OP_NON_CAPTURE_NON_GREEDY_GROUP_END:
      {
        uint32_t end_idx, iter_idx, min, max;
        uint8_t *old_bc_p;

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
          const lit_utf8_byte_t *old_end_p = re_ctx_p->saved_p[end_idx];
          re_ctx_p->saved_p[end_idx] = str_curr_p;

          const lit_utf8_byte_t *sub_str_p = NULL;
          ecma_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_curr_p, &sub_str_p);

          if (ecma_is_value_true (match_value))
          {
            *out_str_p = sub_str_p;
            return match_value; /* match */
          }
          else if (ECMA_IS_VALUE_ERROR (match_value))
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
        const lit_utf8_byte_t *old_start_p = NULL;
        const lit_utf8_byte_t *old_end_p = NULL;
        const lit_utf8_byte_t *sub_str_p = NULL;
        uint8_t *old_bc_p;

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
          return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
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
          ecma_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_curr_p, &sub_str_p);

          if (ecma_is_value_true (match_value))
          {
            *out_str_p = sub_str_p;
            return match_value; /* match */
          }
          else if (ECMA_IS_VALUE_ERROR (match_value))
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

            ecma_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_curr_p, &sub_str_p);

            if (ecma_is_value_true (match_value))
            {
              *out_str_p = sub_str_p;
              return match_value; /* match */
            }
            else if (ECMA_IS_VALUE_ERROR (match_value))
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
          ecma_value_t match_value = re_match_regexp (re_ctx_p, old_bc_p, str_curr_p, &sub_str_p);

          if (ecma_is_value_true (match_value))
          {
            *out_str_p = sub_str_p;
            return match_value; /* match */
          }
          else if (ECMA_IS_VALUE_ERROR (match_value))
          {
            return match_value;
          }
        }

        /* restore if fails */
        re_ctx_p->saved_p[end_idx] = old_end_p;
        re_ctx_p->num_of_iterations_p[iter_idx]--;
        return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_NON_GREEDY_ITERATOR:
      {
        uint32_t min, max, offset, num_of_iter;
        const lit_utf8_byte_t *sub_str_p = NULL;

        min = re_get_value (&bc_p);
        max = re_get_value (&bc_p);

        offset = re_get_value (&bc_p);
        JERRY_TRACE_MSG ("Non-greedy iterator, min=%lu, max=%lu, offset=%ld\n",
                         (unsigned long) min, (unsigned long) max, (long) offset);

        num_of_iter = 0;
        while (num_of_iter <= max)
        {
          if (num_of_iter >= min)
          {
            ecma_value_t match_value = re_match_regexp (re_ctx_p, bc_p + offset, str_curr_p, &sub_str_p);

            if (ecma_is_value_true (match_value))
            {
              *out_str_p = sub_str_p;
              return match_value; /* match */
            }
            else if (ECMA_IS_VALUE_ERROR (match_value))
            {
              return match_value;
            }
          }

          ecma_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_curr_p, &sub_str_p);

          if (!ecma_is_value_true (match_value))
          {
            if (ECMA_IS_VALUE_ERROR (match_value))
            {
              return match_value;
            }

            break;
          }

          str_curr_p = sub_str_p;
          num_of_iter++;
        }
        return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_GREEDY_ITERATOR:
      {
        uint32_t min, max, offset, num_of_iter;
        const lit_utf8_byte_t *sub_str_p = NULL;

        min = re_get_value (&bc_p);
        max = re_get_value (&bc_p);

        offset = re_get_value (&bc_p);
        JERRY_TRACE_MSG ("Greedy iterator, min=%lu, max=%lu, offset=%ld\n",
                         (unsigned long) min, (unsigned long) max, (long) offset);

        num_of_iter = 0;

        while (num_of_iter < max)
        {
          ecma_value_t match_value = re_match_regexp (re_ctx_p, bc_p, str_curr_p, &sub_str_p);

          if (!ecma_is_value_true (match_value))
          {
            if (ECMA_IS_VALUE_ERROR (match_value))
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
          ecma_value_t match_value = re_match_regexp (re_ctx_p, bc_p + offset, str_curr_p, &sub_str_p);

          if (ecma_is_value_true (match_value))
          {
            *out_str_p = sub_str_p;
            return match_value; /* match */
          }
          else if (ECMA_IS_VALUE_ERROR (match_value))
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
        return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      default:
      {
        JERRY_TRACE_MSG ("UNKNOWN opcode (%u)!\n", (unsigned int) op);
        return ecma_raise_common_error (ECMA_ERR_MSG ("Unknown RegExp opcode."));
      }
    }
  }

  JERRY_UNREACHABLE ();
  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
} /* re_match_regexp */

/**
 * Define the necessary properties for the result array (index, input, length).
 */
void
re_set_result_array_properties (ecma_object_t *array_obj_p, /**< result array */
                                ecma_string_t *input_str_p, /**< input string */
                                uint32_t num_of_elements, /**< Number of array elements */
                                int32_t index) /**< index of matching */
{
  /* Set index property of the result array */
  ecma_string_t *result_prop_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_INDEX);
  {
    ecma_builtin_helper_def_prop (array_obj_p,
                                  result_prop_str_p,
                                  ecma_make_int32_value (index),
                                  true, /* Writable */
                                  true, /* Enumerable */
                                  true, /* Configurable */
                                  true); /* Failure handling */
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
  result_prop_str_p = ecma_new_ecma_length_string ();
  {
    ecma_property_descriptor_t array_item_prop_desc = ecma_make_empty_property_descriptor ();
    array_item_prop_desc.is_value_defined = true;

    array_item_prop_desc.value = ecma_make_uint32_value (num_of_elements);

    ecma_op_object_define_own_property (array_obj_p,
                                        result_prop_str_p,
                                        &array_item_prop_desc,
                                        true);
  }
  ecma_deref_ecma_string (result_prop_str_p);
} /* re_set_result_array_properties */

/**
 * RegExp helper function to start the recursive matching algorithm
 * and create the result Array object
 *
 * See also:
 *          ECMA-262 v5, 15.10.6.2
 *
 * @return array object - if matched
 *         null         - otherwise
 *
 *         May raise error.
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_regexp_exec_helper (ecma_value_t regexp_value, /**< RegExp object */
                         ecma_value_t input_string, /**< input string */
                         bool ignore_global) /**< ignore global flag */
{
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  JERRY_ASSERT (ecma_is_value_object (regexp_value));
  JERRY_ASSERT (ecma_is_value_string (input_string));

  ecma_object_t *regexp_object_p = ecma_get_object_from_value (regexp_value);

  JERRY_ASSERT (ecma_object_class_is (regexp_object_p, LIT_MAGIC_STRING_REGEXP_UL));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) regexp_object_p;
  re_compiled_code_t *bc_p = ECMA_GET_INTERNAL_VALUE_POINTER (re_compiled_code_t,
                                                              ext_object_p->u.class_prop.u.value);

  if (bc_p == NULL)
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Incompatible type"));
  }

  ecma_string_t *input_string_p = ecma_get_string_from_value (input_string);
  ECMA_STRING_TO_UTF8_STRING (input_string_p, input_buffer_p, input_buffer_size);

  re_matcher_ctx_t re_ctx;
  const lit_utf8_byte_t *input_curr_p = input_buffer_p;

  re_ctx.input_start_p = input_curr_p;
  const lit_utf8_byte_t *input_end_p = re_ctx.input_start_p + input_buffer_size;
  re_ctx.input_end_p = input_end_p;

  /* 1. Read bytecode header and init regexp matcher context. */
  re_ctx.flags = bc_p->header.status_flags;

  if (ignore_global)
  {
    re_ctx.flags &= (uint16_t) ~RE_FLAG_GLOBAL;
  }

  JERRY_TRACE_MSG ("Exec with flags [global: %d, ignoreCase: %d, multiline: %d]\n",
                   re_ctx.flags & RE_FLAG_GLOBAL,
                   re_ctx.flags & RE_FLAG_IGNORE_CASE,
                   re_ctx.flags & RE_FLAG_MULTILINE);

  re_ctx.num_of_captures = bc_p->num_of_captures;
  JERRY_ASSERT (re_ctx.num_of_captures % 2 == 0);
  re_ctx.num_of_non_captures = bc_p->num_of_non_captures;

  JMEM_DEFINE_LOCAL_ARRAY (saved_p, re_ctx.num_of_captures + re_ctx.num_of_non_captures, const lit_utf8_byte_t *);

  for (uint32_t i = 0; i < re_ctx.num_of_captures + re_ctx.num_of_non_captures; i++)
  {
    saved_p[i] = NULL;
  }
  re_ctx.saved_p = saved_p;

  uint32_t num_of_iter_length = (re_ctx.num_of_captures / 2) + (re_ctx.num_of_non_captures - 1);
  JMEM_DEFINE_LOCAL_ARRAY (num_of_iter_p, num_of_iter_length, uint32_t);

  for (uint32_t i = 0; i < num_of_iter_length; i++)
  {
    num_of_iter_p[i] = 0u;
  }

  bool is_match = false;
  re_ctx.num_of_iterations_p = num_of_iter_p;
  int32_t index = 0;
  ecma_length_t input_str_len;

  input_str_len = lit_utf8_string_length (input_buffer_p, input_buffer_size);

  if (input_buffer_p && (re_ctx.flags & RE_FLAG_GLOBAL))
  {
    ecma_string_t *magic_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL);
    ecma_value_t lastindex_value = ecma_op_object_get_own_data_prop (regexp_object_p, magic_str_p);

    ECMA_OP_TO_NUMBER_TRY_CATCH (lastindex_num, lastindex_value, ret_value)

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

    ecma_fast_free_value (lastindex_value);

    ecma_deref_ecma_string (magic_str_p);
  }

  /* 2. Try to match */
  const lit_utf8_byte_t *sub_str_p = NULL;
  uint8_t *bc_start_p = (uint8_t *) (bc_p + 1);

  while (ecma_is_value_empty (ret_value))
  {
    if (index < 0 || index > (int32_t) input_str_len)
    {
      if (re_ctx.flags & RE_FLAG_GLOBAL)
      {
        ecma_string_t *magic_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL);
        ecma_op_object_put (regexp_object_p, magic_str_p, ecma_make_integer_value (0), true);
        ecma_deref_ecma_string (magic_str_p);
      }

      is_match = false;
      break;
    }
    else
    {
      ECMA_TRY_CATCH (match_value, re_match_regexp (&re_ctx,
                                                    bc_start_p,
                                                    input_curr_p,
                                                    &sub_str_p),
                                                    ret_value);

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
    ecma_number_t lastindex_num;

    if (sub_str_p != NULL
        && input_buffer_p != NULL)
    {
      lastindex_num = lit_utf8_string_length (input_buffer_p,
                                              (lit_utf8_size_t) (sub_str_p - input_buffer_p));
    }
    else
    {
      lastindex_num = ECMA_NUMBER_ZERO;
    }

    ecma_op_object_put (regexp_object_p, magic_str_p, ecma_make_number_value (lastindex_num), true);
    ecma_deref_ecma_string (magic_str_p);
  }

  /* 3. Fill the result array or return with 'undefiend' */
  if (ecma_is_value_empty (ret_value))
  {
    if (is_match)
    {
      ecma_value_t result_array = ecma_op_create_array_object (0, 0, false);
      ecma_object_t *result_array_obj_p = ecma_get_object_from_value (result_array);

      ecma_string_t *input_str_p = ecma_new_ecma_string_from_utf8 (input_buffer_p, input_buffer_size);
      re_set_result_array_properties (result_array_obj_p, input_str_p, re_ctx.num_of_captures / 2, index);
      ecma_deref_ecma_string (input_str_p);

      for (uint32_t i = 0; i < re_ctx.num_of_captures; i += 2)
      {
        ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (i / 2);
        ecma_value_t capture_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

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

          capture_value = ecma_make_string_value (capture_str_p);
        }

        ecma_property_value_t *prop_value_p;
        prop_value_p = ecma_create_named_data_property (result_array_obj_p,
                                                        index_str_p,
                                                        ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                        NULL);
        prop_value_p->value = capture_value;

        JERRY_ASSERT (!ecma_is_value_object (capture_value));
        ecma_deref_ecma_string (index_str_p);
      }

      ret_value = result_array;
    }
    else
    {
      ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_NULL);
    }
  }

  JMEM_FINALIZE_LOCAL_ARRAY (num_of_iter_p);
  JMEM_FINALIZE_LOCAL_ARRAY (saved_p);
  ECMA_FINALIZE_UTF8_STRING (input_buffer_p, input_buffer_size);

  return ret_value;
} /* ecma_regexp_exec_helper */

/**
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */
