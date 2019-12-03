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
#include "ecma-builtins.h"
#include "ecma-builtin-helpers.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-objects.h"
#include "ecma-function-object.h"
#include "ecma-regexp-object.h"
#include "ecma-try-catch-macro.h"
#include "jcontext.h"
#include "jrt-libc-includes.h"
#include "lit-char-helpers.h"
#include "re-compiler.h"

#if ENABLED (JERRY_BUILTIN_REGEXP)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaregexpobject ECMA RegExp object related routines
 * @{
 */

/**
 * Index of the global capturing group
 */
#define RE_GLOBAL_CAPTURE 0

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
ecma_regexp_parse_flags (ecma_string_t *flags_str_p, /**< Input string with flags */
                         uint16_t *flags_p) /**< [out] parsed flag bits */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;
  uint16_t result_flags = RE_FLAG_EMPTY;

  ECMA_STRING_TO_UTF8_STRING (flags_str_p, flags_start_p, flags_start_size);

  const lit_utf8_byte_t *flags_str_curr_p = flags_start_p;
  const lit_utf8_byte_t *flags_str_end_p = flags_start_p + flags_start_size;

  while (flags_str_curr_p < flags_str_end_p)
  {
    ecma_regexp_flags_t flag;
    switch (*flags_str_curr_p++)
    {
      case 'g':
      {
        flag = RE_FLAG_GLOBAL;
        break;
      }
      case 'i':
      {
        flag = RE_FLAG_IGNORE_CASE;
        break;
      }
      case 'm':
      {
        flag = RE_FLAG_MULTILINE;
        break;
      }
      case 'y':
      {
        flag = RE_FLAG_STICKY;
        break;
      }
      case 'u':
      {
        flag = RE_FLAG_UNICODE;
        break;
      }
      default:
      {
        flag = RE_FLAG_EMPTY;
        break;
      }
    }

    if (flag == RE_FLAG_EMPTY || (result_flags & flag) != 0)
    {
      ret_value = ecma_raise_syntax_error (ECMA_ERR_MSG ("Invalid RegExp flags."));
      break;
    }

    result_flags = (uint16_t) (result_flags | flag);
  }

  ECMA_FINALIZE_UTF8_STRING (flags_start_p, flags_start_size);

  *flags_p = result_flags;
  return ret_value;
} /* ecma_regexp_parse_flags */

/*
 * Create the properties of a RegExp instance.
 */
static void
ecma_regexp_create_props (ecma_object_t *re_object_p) /**< RegExp object */
{
#if !ENABLED (JERRY_ES2015)
  ecma_create_named_data_property (re_object_p,
                                   ecma_get_magic_string (LIT_MAGIC_STRING_SOURCE),
                                   ECMA_PROPERTY_FIXED,
                                   NULL);
  ecma_create_named_data_property (re_object_p,
                                   ecma_get_magic_string (LIT_MAGIC_STRING_GLOBAL),
                                   ECMA_PROPERTY_FIXED,
                                   NULL);
  ecma_create_named_data_property (re_object_p,
                                   ecma_get_magic_string (LIT_MAGIC_STRING_IGNORECASE_UL),
                                   ECMA_PROPERTY_FIXED,
                                   NULL);
  ecma_create_named_data_property (re_object_p,
                                   ecma_get_magic_string (LIT_MAGIC_STRING_MULTILINE),
                                   ECMA_PROPERTY_FIXED,
                                   NULL);
#endif /* !ENABLED (JERRY_ES2015) */
  ecma_create_named_data_property (re_object_p,
                                   ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL),
                                   ECMA_PROPERTY_FLAG_WRITABLE,
                                   NULL);
} /* ecma_regexp_create_props */

/*
 * Helper function to assign a value to a property
 */
static void
ecma_regexp_helper_assign_prop (ecma_object_t *re_object_p, /**< RegExp object */
                                lit_magic_string_id_t prop_id, /**< property name ide */
                                ecma_value_t value) /**< value */
{
  ecma_property_ref_t property_ref;
  ecma_op_object_get_own_property (re_object_p,
                                   ecma_get_magic_string (prop_id),
                                   &property_ref,
                                   ECMA_PROPERTY_GET_VALUE);
  ecma_named_data_property_assign_value (re_object_p,
                                         property_ref.value_p,
                                         value);
} /* ecma_regexp_helper_assign_prop */

/**
 * Initializes the properties of a RegExp instance.
 */
void
ecma_regexp_initialize_props (ecma_object_t *re_object_p, /**< RegExp object */
                              ecma_string_t *source_p, /**< source string */
                              uint16_t flags) /**< flags */
{
#if !ENABLED (JERRY_ES2015)
  ecma_regexp_helper_assign_prop (re_object_p,
                                  LIT_MAGIC_STRING_SOURCE,
                                  ecma_make_string_value (source_p));

  ecma_regexp_helper_assign_prop (re_object_p,
                                  LIT_MAGIC_STRING_GLOBAL,
                                  ecma_make_boolean_value (flags & RE_FLAG_GLOBAL));

  ecma_regexp_helper_assign_prop (re_object_p,
                                  LIT_MAGIC_STRING_IGNORECASE_UL,
                                  ecma_make_boolean_value (flags & RE_FLAG_IGNORE_CASE));

  ecma_regexp_helper_assign_prop (re_object_p,
                                  LIT_MAGIC_STRING_MULTILINE,
                                  ecma_make_boolean_value (flags & RE_FLAG_MULTILINE));
#else /* ENABLED (JERRY_ES2015) */
  JERRY_UNUSED (source_p);
  JERRY_UNUSED (flags);
#endif /* !ENABLED (JERRY_ES2015) */

  ecma_regexp_helper_assign_prop (re_object_p,
                                  LIT_MAGIC_STRING_LASTINDEX_UL,
                                  ecma_make_uint32_value (0));
} /* ecma_regexp_initialize_props */

#if ENABLED (JERRY_ES2015)
/**
 * Helper function to get current code point and advance the string pointer.
 *
 * @return lit_code_point_t current code point
 */
static lit_code_point_t
ecma_regexp_unicode_advance (const lit_utf8_byte_t **str_p, /**< reference to string pointer */
                             const lit_utf8_byte_t *end_p) /**< string end pointer */
{
  JERRY_ASSERT (str_p != NULL);
  const lit_utf8_byte_t *current_p = *str_p;

  lit_code_point_t ch = lit_utf8_read_next (&current_p);
  if (lit_is_code_point_utf16_high_surrogate ((ecma_char_t) ch)
      && current_p < end_p)
  {
    const ecma_char_t next_ch = lit_utf8_peek_next (current_p);
    if (lit_is_code_point_utf16_low_surrogate (next_ch))
    {
      lit_utf8_incr (&current_p);
      ch = lit_convert_surrogate_pair_to_code_point ((ecma_char_t) ch, next_ch);
    }
  }

  *str_p = current_p;
  return ch;
} /* ecma_regexp_unicode_advance */
#endif /* ENABLED (JERRY_ES2015) */

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

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

  /* Set the internal [[Class]] property */
  ext_object_p->u.class_prop.class_id = LIT_MAGIC_STRING_REGEXP_UL;

  /* Set bytecode internal property. */
  ECMA_SET_INTERNAL_VALUE_POINTER (ext_object_p->u.class_prop.u.value, bytecode_p);
  ecma_bytecode_ref ((ecma_compiled_code_t *) bytecode_p);

  /* Create and initialize RegExp object properties */
  ecma_regexp_create_props (object_p);
  ecma_regexp_initialize_props (object_p,
                                ecma_get_string_from_value (bytecode_p->source),
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
                              uint16_t flags) /**< flags */
{
  JERRY_ASSERT (pattern_p != NULL);

  ecma_object_t *re_prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_REGEXP_PROTOTYPE);

  ecma_object_t *object_p = ecma_create_object (re_prototype_obj_p,
                                                sizeof (ecma_extended_object_t),
                                                ECMA_OBJECT_TYPE_CLASS);
  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  ext_object_p->u.class_prop.class_id = LIT_MAGIC_STRING_UNDEFINED;

  ecma_regexp_create_props (object_p);
  ecma_regexp_initialize_props (object_p, pattern_p, flags);

  /* Compile bytecode. */
  const re_compiled_code_t *bc_p = NULL;
  ecma_value_t ret_value = re_compile_bytecode (&bc_p, pattern_p, flags);
  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    ecma_deref_object (object_p);
    return ret_value;
  }

  JERRY_ASSERT (ecma_is_value_empty (ret_value));

  /* Set [[Class]] and bytecode internal properties. */
  ext_object_p->u.class_prop.class_id = LIT_MAGIC_STRING_REGEXP_UL;
  ECMA_SET_INTERNAL_VALUE_POINTER (ext_object_p->u.class_prop.u.value, bc_p);

  return ecma_make_object_value (object_p);
} /* ecma_op_create_regexp_object */

/**
 * Canonicalize a character
 *
 * @return ecma_char_t canonicalized character
 */
lit_code_point_t
ecma_regexp_canonicalize_char (lit_code_point_t ch) /**< character */
{
  if (JERRY_LIKELY (ch <= LIT_UTF8_1_BYTE_CODE_POINT_MAX))
  {
    if (ch >= LIT_CHAR_LOWERCASE_A && ch <= LIT_CHAR_LOWERCASE_Z)
    {
      return (ecma_char_t) (ch - (LIT_CHAR_LOWERCASE_A - LIT_CHAR_UPPERCASE_A));
    }

    return ch;
  }

#if ENABLED (JERRY_ES2015)
  /* TODO: Implement case folding for code points in the upper planes. */
  if (JERRY_UNLIKELY (ch > LIT_UTF16_CODE_UNIT_MAX))
  {
    return ch;
  }
#endif /* ENABLED (JERRY_ES2015) */

  ecma_char_t u[LIT_MAXIMUM_OTHER_CASE_LENGTH];
  const ecma_length_t size = lit_char_to_upper_case ((ecma_char_t) ch, u, LIT_MAXIMUM_OTHER_CASE_LENGTH);

  /* 3. */
  if (size != 1)
  {
    return ch;
  }
  /* 4. */
  const ecma_char_t cu = u[0];
  /* 5. */
  if (cu >= 128)
  {
    /* 6. */
    return cu;
  }

  return ch;
} /* ecma_regexp_canonicalize_char */

/**
 * RegExp Canonicalize abstract operation
 *
 * See also: ECMA-262 v5, 15.10.2.8
 *
 * @return ecma_char_t canonicalized character
 */
inline lit_code_point_t JERRY_ATTR_ALWAYS_INLINE
ecma_regexp_canonicalize (lit_code_point_t ch, /**< character */
                          bool is_ignorecase) /**< IgnoreCase flag */
{
  if (is_ignorecase)
  {
    return ecma_regexp_canonicalize_char (ch);
  }

  return ch;
} /* ecma_regexp_canonicalize */

/**
 * Recursive function for RegExp matching.
 *
 * See also:
 *          ECMA-262 v5, 15.10.2.1
 *
 * @return true  - if matched
 *         false - otherwise
 */
static const lit_utf8_byte_t *
ecma_regexp_match (ecma_regexp_ctx_t *re_ctx_p, /**< RegExp matcher context */
                   const uint8_t *bc_p, /**< pointer to the current RegExp bytecode */
                   const lit_utf8_byte_t *str_curr_p) /**< input string pointer */
{
#if (JERRY_STACK_LIMIT != 0)
  if (JERRY_UNLIKELY (ecma_get_current_stack_usage () > CONFIG_MEM_STACK_LIMIT))
  {
    return ECMA_RE_OUT_OF_STACK;
  }
#endif /* JERRY_STACK_LIMIT != 0 */

  while (true)
  {
    re_opcode_t op = re_get_opcode (&bc_p);

    switch (op)
    {
      case RE_OP_MATCH:
      {
        JERRY_TRACE_MSG ("Execute RE_OP_MATCH: match\n");
        return str_curr_p;
      }
      case RE_OP_CHAR:
      {
        if (str_curr_p >= re_ctx_p->input_end_p)
        {
          return NULL; /* fail */
        }

        const bool is_ignorecase = re_ctx_p->flags & RE_FLAG_IGNORE_CASE;
        lit_code_point_t ch1 = re_get_char (&bc_p); /* Already canonicalized. */
        lit_code_point_t ch2 = lit_utf8_read_next (&str_curr_p);

#if ENABLED (JERRY_ES2015)
        if (re_ctx_p->flags & RE_FLAG_UNICODE
            && lit_is_code_point_utf16_high_surrogate (ch2)
            && str_curr_p < re_ctx_p->input_end_p)
        {
          const ecma_char_t next_ch = lit_utf8_peek_next (str_curr_p);
          if (lit_is_code_point_utf16_low_surrogate (next_ch))
          {
            lit_utf8_incr (&str_curr_p);
            ch2 = lit_convert_surrogate_pair_to_code_point ((ecma_char_t) ch2, next_ch);
          }
        }
#endif /* ENABLED (JERRY_ES2015) */

        ch2 = ecma_regexp_canonicalize (ch2, is_ignorecase);
        JERRY_TRACE_MSG ("Character matching %d to %d: ", ch1, ch2);

        if (ch1 != ch2)
        {
          JERRY_TRACE_MSG ("fail\n");
          return NULL; /* fail */
        }

        JERRY_TRACE_MSG ("match\n");
        break; /* tail merge */
      }
      case RE_OP_PERIOD:
      {
        if (str_curr_p >= re_ctx_p->input_end_p)
        {
          return NULL; /* fail */
        }

        const ecma_char_t ch = lit_utf8_read_next (&str_curr_p);
        JERRY_TRACE_MSG ("Period matching '.' to %u: ", (unsigned int) ch);

        if (lit_char_is_line_terminator (ch))
        {
          JERRY_TRACE_MSG ("fail\n");
          return NULL; /* fail */
        }

#if ENABLED (JERRY_ES2015)
        if (re_ctx_p->flags & RE_FLAG_UNICODE
            && lit_is_code_point_utf16_high_surrogate (ch)
            && str_curr_p < re_ctx_p->input_end_p)
        {
          const ecma_char_t next_ch = lit_utf8_peek_next (str_curr_p);
          if (lit_is_code_point_utf16_low_surrogate (next_ch))
          {
            lit_utf8_incr (&str_curr_p);
          }
        }
#endif /* ENABLED (JERRY_ES2015) */

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
          return NULL; /* fail */
        }

        if (lit_char_is_line_terminator (lit_utf8_peek_prev (str_curr_p)))
        {
          JERRY_TRACE_MSG ("match\n");
          break; /* tail merge */
        }

        JERRY_TRACE_MSG ("fail\n");
        return NULL; /* fail */
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
          return NULL; /* fail */
        }

        if (lit_char_is_line_terminator (lit_utf8_peek_next (str_curr_p)))
        {
          JERRY_TRACE_MSG ("match\n");
          break; /* tail merge */
        }

        JERRY_TRACE_MSG ("fail\n");
        return NULL; /* fail */
      }
      case RE_OP_ASSERT_WORD_BOUNDARY:
      case RE_OP_ASSERT_NOT_WORD_BOUNDARY:
      {
        const bool is_wordchar_left = ((str_curr_p > re_ctx_p->input_start_p)
                                       && lit_char_is_word_char (lit_utf8_peek_prev (str_curr_p)));

        const bool is_wordchar_right = ((str_curr_p < re_ctx_p->input_end_p)
                                        && lit_char_is_word_char (lit_utf8_peek_next (str_curr_p)));

        if (op == RE_OP_ASSERT_WORD_BOUNDARY)
        {
          JERRY_TRACE_MSG ("Execute RE_OP_ASSERT_WORD_BOUNDARY: ");
          if (is_wordchar_left == is_wordchar_right)
          {
            JERRY_TRACE_MSG ("fail\n");
            return NULL; /* fail */
          }
        }
        else
        {
          JERRY_ASSERT (op == RE_OP_ASSERT_NOT_WORD_BOUNDARY);
          JERRY_TRACE_MSG ("Execute RE_OP_ASSERT_NOT_WORD_BOUNDARY: ");

          if (is_wordchar_left != is_wordchar_right)
          {
            JERRY_TRACE_MSG ("fail\n");
            return NULL; /* fail */
          }
        }

        JERRY_TRACE_MSG ("match\n");
        break; /* tail merge */
      }
      case RE_OP_LOOKAHEAD_POS:
      case RE_OP_LOOKAHEAD_NEG:
      {
        const lit_utf8_byte_t *matched_p = NULL;
        const size_t captures_size = re_ctx_p->captures_count * sizeof (ecma_regexp_capture_t);
        ecma_regexp_capture_t *saved_captures_p = (ecma_regexp_capture_t *) jmem_heap_alloc_block (captures_size);
        memcpy (saved_captures_p, re_ctx_p->captures_p, captures_size);

        do
        {
          const uint32_t offset = re_get_value (&bc_p);

          if (matched_p == NULL)
          {
            matched_p = ecma_regexp_match (re_ctx_p, bc_p, str_curr_p);

            if (ECMA_RE_STACK_LIMIT_REACHED (matched_p))
            {
              jmem_heap_free_block (saved_captures_p, captures_size);
              return matched_p;
            }
          }
          bc_p += offset;
        }
        while (re_get_opcode (&bc_p) == RE_OP_ALTERNATIVE);

        JERRY_TRACE_MSG ("Execute RE_OP_LOOKAHEAD_POS/NEG: ");
        if ((op == RE_OP_LOOKAHEAD_POS && matched_p != NULL)
            || (op == RE_OP_LOOKAHEAD_NEG && matched_p == NULL))
        {
          JERRY_TRACE_MSG ("match\n");
          matched_p = ecma_regexp_match (re_ctx_p, bc_p, str_curr_p);
        }
        else
        {
          JERRY_TRACE_MSG ("fail\n");
          matched_p = NULL; /* fail */
        }

        if (matched_p == NULL)
        {
          /* restore saved */
          memcpy (re_ctx_p->captures_p, saved_captures_p, captures_size);
        }

        jmem_heap_free_block (saved_captures_p, captures_size);
        return matched_p;
      }
      case RE_OP_CHAR_CLASS:
      case RE_OP_INV_CHAR_CLASS:
      {
        JERRY_TRACE_MSG ("Execute RE_OP_CHAR_CLASS/RE_OP_INV_CHAR_CLASS, ");
        if (str_curr_p >= re_ctx_p->input_end_p)
        {
          JERRY_TRACE_MSG ("fail\n");
          return NULL; /* fail */
        }

        uint32_t range_count = re_get_value (&bc_p);
        const bool is_ignorecase = re_ctx_p->flags & RE_FLAG_IGNORE_CASE;
        bool is_match = false;

#if ENABLED (JERRY_ES2015)
        if (re_ctx_p->flags & RE_FLAG_UNICODE)
        {
          lit_code_point_t curr_ch = ecma_regexp_unicode_advance (&str_curr_p,
                                                                  re_ctx_p->input_end_p);
          curr_ch = ecma_regexp_canonicalize (curr_ch, is_ignorecase);

          while (range_count-- > 0)
          {
            const lit_code_point_t ch1 = re_get_value (&bc_p);
            if (curr_ch < ch1)
            {
              bc_p += sizeof (uint32_t);
              continue;
            }

            const lit_code_point_t ch2 = re_get_value (&bc_p);
            is_match = (curr_ch <= ch2);
            if (is_match)
            {
              /* Skip the remaining ranges in the bytecode. */
              bc_p += range_count * 2 * sizeof (uint32_t);
              break;
            }
          }
        }
        else
        {
#endif /* ENABLED (JERRY_ES2015) */
          const ecma_char_t curr_ch = (ecma_char_t) ecma_regexp_canonicalize (lit_utf8_read_next (&str_curr_p),
                                                                              is_ignorecase);

          while (range_count-- > 0)
          {
            const ecma_char_t ch1 = re_get_char (&bc_p);
            if (curr_ch < ch1)
            {
              bc_p += sizeof (ecma_char_t);
              continue;
            }

            const ecma_char_t ch2 = re_get_char (&bc_p);
            is_match = (curr_ch <= ch2);
            if (is_match)
            {
              /* Skip the remaining ranges in the bytecode. */
              bc_p += range_count * 2 * sizeof (ecma_char_t);
              break;
            }
          }
#if ENABLED (JERRY_ES2015)
        }
#endif /* ENABLED (JERRY_ES2015) */

        JERRY_ASSERT (op == RE_OP_CHAR_CLASS || op == RE_OP_INV_CHAR_CLASS);

        if ((op == RE_OP_CHAR_CLASS) != is_match)
        {
          JERRY_TRACE_MSG ("fail\n");
          return NULL; /* fail */
        }

        JERRY_TRACE_MSG ("match\n");
        break; /* tail merge */
      }
      case RE_OP_BACKREFERENCE:
      {
        const uint32_t backref_idx = re_get_value (&bc_p);
        JERRY_TRACE_MSG ("Execute RE_OP_BACKREFERENCE (idx: %u): ", (unsigned int) backref_idx);
        JERRY_ASSERT (backref_idx >= 1 && backref_idx < re_ctx_p->captures_count);
        const ecma_regexp_capture_t capture = re_ctx_p->captures_p[backref_idx];

        if (capture.begin_p == NULL || capture.end_p == NULL)
        {
          JERRY_TRACE_MSG ("match\n");
          break; /* capture is 'undefined', always matches! */
        }

        const lit_utf8_size_t capture_size = (lit_utf8_size_t) (capture.end_p - capture.begin_p);

        if (str_curr_p + capture_size > re_ctx_p->input_end_p)
        {
          JERRY_TRACE_MSG ("fail\n");
          return NULL; /* fail */
        }

        if (memcmp (str_curr_p, capture.begin_p, capture_size))
        {
          JERRY_TRACE_MSG ("fail\n");
          return NULL; /* fail */
        }

        str_curr_p += capture_size;
        JERRY_TRACE_MSG ("match\n");
        break; /* tail merge */
      }
      case RE_OP_SAVE_AT_START:
      {
        JERRY_TRACE_MSG ("Execute RE_OP_SAVE_AT_START\n");
        re_ctx_p->captures_p[RE_GLOBAL_CAPTURE].begin_p = str_curr_p;

        do
        {
          const uint32_t offset = re_get_value (&bc_p);
          const lit_utf8_byte_t *const matched_p = ecma_regexp_match (re_ctx_p, bc_p, str_curr_p);

          if (matched_p != NULL)
          {
            return matched_p; /* match */
          }

          bc_p += offset;
        }
        while (re_get_opcode (&bc_p) == RE_OP_ALTERNATIVE);
        bc_p -= sizeof (uint8_t);

        return NULL; /* fail */
      }
      case RE_OP_SAVE_AND_MATCH:
      {
        JERRY_TRACE_MSG ("End of pattern is reached: match\n");
        re_ctx_p->captures_p[RE_GLOBAL_CAPTURE].end_p = str_curr_p;
        return str_curr_p; /* match */
      }
      case RE_OP_ALTERNATIVE:
      {
        /*
        *  Alternatives should be jumped over, when an alternative opcode appears.
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
        const lit_utf8_byte_t *old_begin_p = NULL;
        const uint8_t *const bc_start_p = bc_p; /* save the bytecode start position of the group start */
        const uint32_t start_idx = re_get_value (&bc_p);
        const uint32_t offset = re_get_value (&bc_p);

        uint32_t *iterator_p;
        if (RE_IS_CAPTURE_GROUP (op))
        {
          JERRY_ASSERT (start_idx < re_ctx_p->captures_count);
          re_ctx_p->captures_p[start_idx].begin_p = str_curr_p;
          iterator_p = &(re_ctx_p->iterations_p[start_idx - 1]);
        }
        else
        {
          JERRY_ASSERT (start_idx < re_ctx_p->non_captures_count);
          iterator_p = &(re_ctx_p->iterations_p[start_idx + re_ctx_p->captures_count - 1]);
        }
        *iterator_p = 0;

        /* Jump all over to the end of the END opcode. */
        bc_p += offset;

        /* Try to match after the close paren if zero is allowed */
        const lit_utf8_byte_t *matched_p = ecma_regexp_match (re_ctx_p, bc_p, str_curr_p);

        if (matched_p != NULL)
        {
          return str_curr_p; /* match */
        }

        if (RE_IS_CAPTURE_GROUP (op))
        {
          re_ctx_p->captures_p[start_idx].begin_p = old_begin_p;
        }

        bc_p = bc_start_p;
        /* FALLTHRU */
      }
      case RE_OP_CAPTURE_GROUP_START:
      case RE_OP_CAPTURE_GREEDY_ZERO_GROUP_START:
      case RE_OP_NON_CAPTURE_GROUP_START:
      case RE_OP_NON_CAPTURE_GREEDY_ZERO_GROUP_START:
      {
        const uint8_t *bc_end_p = NULL;
        const uint32_t start_idx = re_get_value (&bc_p);

        if (op != RE_OP_CAPTURE_GROUP_START
            && op != RE_OP_NON_CAPTURE_GROUP_START)
        {
          const uint32_t offset = re_get_value (&bc_p);
          bc_end_p = bc_p + offset;
        }

        const lit_utf8_byte_t **group_begin_p;
        uint32_t *iterator_p;
        if (RE_IS_CAPTURE_GROUP (op))
        {
          JERRY_ASSERT (start_idx < re_ctx_p->captures_count);
          group_begin_p = &(re_ctx_p->captures_p[start_idx].begin_p);
          iterator_p = &(re_ctx_p->iterations_p[start_idx - 1]);
        }
        else
        {
          JERRY_ASSERT (start_idx < re_ctx_p->non_captures_count);
          group_begin_p = &(re_ctx_p->non_captures_p[start_idx].str_p);
          iterator_p = &(re_ctx_p->iterations_p[start_idx + re_ctx_p->captures_count - 1]);
        }

        const lit_utf8_byte_t *const old_begin_p = *group_begin_p;
        const uint32_t old_iter_count = *iterator_p;
        *group_begin_p = str_curr_p;
        *iterator_p = 0;

        do
        {
          const uint32_t offset = re_get_value (&bc_p);
          const lit_utf8_byte_t *const matched_p = ecma_regexp_match (re_ctx_p, bc_p, str_curr_p);

          if (matched_p != NULL)
          {
            return matched_p; /* match */
          }

          bc_p += offset;
        }
        while (re_get_opcode (&bc_p) == RE_OP_ALTERNATIVE);

        bc_p -= sizeof (uint8_t);
        *iterator_p = old_iter_count;

        /* Try to match after the close paren if zero is allowed. */
        if (op == RE_OP_CAPTURE_GREEDY_ZERO_GROUP_START
            || op == RE_OP_NON_CAPTURE_GREEDY_ZERO_GROUP_START)
        {
          JERRY_ASSERT (bc_end_p);
          const lit_utf8_byte_t *const matched_p = ecma_regexp_match (re_ctx_p, bc_end_p, str_curr_p);

          if (matched_p != NULL)
          {
            return matched_p; /* match */
          }
        }

        *group_begin_p = old_begin_p;
        return NULL; /* fail */
      }
      case RE_OP_CAPTURE_NON_GREEDY_GROUP_END:
      case RE_OP_NON_CAPTURE_NON_GREEDY_GROUP_END:
      {
        /*
        *  On non-greedy iterations we have to execute the bytecode
        *  after the group first. Try to iterate only if it fails.
        */
        const uint8_t *const bc_start_p = bc_p; /* save the bytecode start position of the group end */
        const uint32_t end_idx = re_get_value (&bc_p);
        const uint32_t min = re_get_value (&bc_p);
        const uint32_t max = re_get_value (&bc_p);
        re_get_value (&bc_p); /* start offset */

        const lit_utf8_byte_t **group_end_p;
        uint32_t *iterator_p;
        if (RE_IS_CAPTURE_GROUP (op))
        {
          JERRY_ASSERT (end_idx < re_ctx_p->captures_count);
          group_end_p = &(re_ctx_p->captures_p[end_idx].end_p);
          iterator_p = &(re_ctx_p->iterations_p[end_idx - 1]);
        }
        else
        {
          JERRY_ASSERT (end_idx < re_ctx_p->non_captures_count);
          group_end_p = &(re_ctx_p->non_captures_p[end_idx].str_p);
          iterator_p = &(re_ctx_p->iterations_p[end_idx + re_ctx_p->captures_count - 1]);
        }

        (*iterator_p)++;

        if (*iterator_p >= min && *iterator_p <= max)
        {
          const lit_utf8_byte_t *const old_end_p = *group_end_p;
          *group_end_p = str_curr_p;

          const lit_utf8_byte_t *const matched_p = ecma_regexp_match (re_ctx_p, bc_p, str_curr_p);

          if (matched_p != NULL)
          {
            return matched_p; /* match */
          }

          *group_end_p = old_end_p;
        }
        (*iterator_p)--;
        bc_p = bc_start_p;

        /* Non-greedy fails, try to iterate. */
        /* FALLTHRU */
      }
      case RE_OP_CAPTURE_GREEDY_GROUP_END:
      case RE_OP_NON_CAPTURE_GREEDY_GROUP_END:
      {
        const uint32_t end_idx = re_get_value (&bc_p);
        const uint32_t min = re_get_value (&bc_p);
        const uint32_t max = re_get_value (&bc_p);
        uint32_t offset = re_get_value (&bc_p);

        const lit_utf8_byte_t **group_begin_p;
        const lit_utf8_byte_t **group_end_p;
        uint32_t *iterator_p;

        if (RE_IS_CAPTURE_GROUP (op))
        {
          JERRY_ASSERT (end_idx < re_ctx_p->captures_count);
          group_begin_p = &(re_ctx_p->captures_p[end_idx].begin_p);
          group_end_p = &(re_ctx_p->captures_p[end_idx].end_p);
          iterator_p = &(re_ctx_p->iterations_p[end_idx - 1]);
        }
        else
        {
          JERRY_ASSERT (end_idx <= re_ctx_p->non_captures_count);
          group_begin_p = &(re_ctx_p->non_captures_p[end_idx].str_p);
          group_end_p = &(re_ctx_p->non_captures_p[end_idx].str_p);
          iterator_p = &(re_ctx_p->iterations_p[end_idx + re_ctx_p->captures_count - 1]);
        }

        /* Check the empty iteration if the minimum number of iterations is reached. */
        if (*iterator_p >= min && str_curr_p == *group_begin_p)
        {
          return NULL; /* fail */
        }

        (*iterator_p)++;

        const uint8_t *const bc_start_p = bc_p; /* Save the bytecode end position of the END opcodes. */
        const lit_utf8_byte_t *const old_end_p = *group_end_p;
        *group_end_p = str_curr_p;

        if (*iterator_p < max)
        {
          bc_p -= offset;
          offset = re_get_value (&bc_p);

          const lit_utf8_byte_t *const old_begin_p = *group_begin_p;
          *group_begin_p = str_curr_p;

          const lit_utf8_byte_t *matched_p = ecma_regexp_match (re_ctx_p, bc_p, str_curr_p);

          if (matched_p != NULL)
          {
            return matched_p; /* match */
          }

          /* Try to match alternatives if any. */
          bc_p += offset;
          while (*bc_p == RE_OP_ALTERNATIVE)
          {
            bc_p++; /* RE_OP_ALTERNATIVE */
            offset = re_get_value (&bc_p);

            *group_begin_p = str_curr_p;

            matched_p = ecma_regexp_match (re_ctx_p, bc_p, str_curr_p);

            if (matched_p != NULL)
            {
              return matched_p; /* match */
            }

            bc_p += offset;
          }

          *group_begin_p = old_begin_p;
        }

        if (*iterator_p >= min && *iterator_p <= max)
        {
          /* Try to match the rest of the bytecode. */
          const lit_utf8_byte_t *const matched_p = ecma_regexp_match (re_ctx_p, bc_start_p, str_curr_p);

          if (matched_p != NULL)
          {
            return matched_p; /* match */
          }
        }

        /* restore if fails */
        *group_end_p = old_end_p;
        (*iterator_p)--;
        return NULL; /* fail */
      }
      case RE_OP_NON_GREEDY_ITERATOR:
      {
        const uint32_t min = re_get_value (&bc_p);
        const uint32_t max = re_get_value (&bc_p);

        const uint32_t offset = re_get_value (&bc_p);
        JERRY_TRACE_MSG ("Non-greedy iterator, min=%lu, max=%lu, offset=%ld\n",
                         (unsigned long) min, (unsigned long) max, (long) offset);

        uint32_t iter_count = 0;
        while (iter_count <= max)
        {
          if (iter_count >= min)
          {
            const lit_utf8_byte_t *const matched_p = ecma_regexp_match (re_ctx_p, bc_p + offset, str_curr_p);

            if (matched_p != NULL)
            {
              return matched_p; /* match */
            }
          }

          const lit_utf8_byte_t *const matched_p = ecma_regexp_match (re_ctx_p, bc_p, str_curr_p);

          if (ECMA_RE_STACK_LIMIT_REACHED (matched_p))
          {
            return matched_p;
          }

          if (matched_p == NULL)
          {
            break;
          }

          str_curr_p = matched_p;
          iter_count++;
        }

        return NULL; /* fail */
      }
      default:
      {
        JERRY_ASSERT (op == RE_OP_GREEDY_ITERATOR);

        const uint32_t min = re_get_value (&bc_p);
        const uint32_t max = re_get_value (&bc_p);

        const uint32_t offset = re_get_value (&bc_p);
        JERRY_TRACE_MSG ("Greedy iterator, min=%lu, max=%lu, offset=%ld\n",
                         (unsigned long) min, (unsigned long) max, (long) offset);

        uint32_t iter_count = 0;
        while (iter_count < max)
        {
          const lit_utf8_byte_t *const matched_p = ecma_regexp_match (re_ctx_p, bc_p, str_curr_p);

          if (ECMA_RE_STACK_LIMIT_REACHED (matched_p))
          {
            return matched_p;
          }

          if (matched_p == NULL)
          {
            break;
          }

          str_curr_p = matched_p;
          iter_count++;
        }

        if (iter_count >= min)
        {
          while (true)
          {
            const lit_utf8_byte_t *const matched_p = ecma_regexp_match (re_ctx_p, bc_p + offset, str_curr_p);

            if (matched_p != NULL)
            {
              return matched_p; /* match */
            }

            if (iter_count == min)
            {
              break;
            }

            lit_utf8_read_prev (&str_curr_p);
            iter_count--;
          }
        }

        return NULL; /* fail */
      }
    }
  }
} /* ecma_regexp_match */

/*
 * Helper function to get the result of a capture
 *
 * @return string value, if capture is defined
 *         undefined, otherwise
 */
ecma_value_t
ecma_regexp_get_capture_value (const ecma_regexp_capture_t *const capture_p) /**< capture */
{
  if (ECMA_RE_IS_CAPTURE_DEFINED (capture_p))
  {
    const lit_utf8_size_t capture_size = (lit_utf8_size_t) (capture_p->end_p - capture_p->begin_p);
    ecma_string_t *const capture_str_p = ecma_new_ecma_string_from_utf8 (capture_p->begin_p, capture_size);
    return ecma_make_string_value (capture_str_p);
  }

  return ECMA_VALUE_UNDEFINED;
} /* ecma_regexp_get_capture_value */

/**
 * Helper function to create a result array from the captures in a regexp context
 *
 * @return ecma value containing the created array object
 */
static ecma_value_t
ecma_regexp_create_result_object (ecma_regexp_ctx_t *re_ctx_p, /**< regexp context */
                                  ecma_string_t *input_string_p, /**< input ecma string */
                                  uint32_t index) /**< match index */
{
  ecma_value_t result_array = ecma_op_create_array_object (0, 0, false);
  ecma_object_t *result_p = ecma_get_object_from_value (result_array);

  for (uint32_t i = 0; i < re_ctx_p->captures_count; i++)
  {
    ecma_value_t capture_value = ecma_regexp_get_capture_value (re_ctx_p->captures_p + i);
    ecma_builtin_helper_def_prop_by_index (result_p,
                                           i,
                                           capture_value,
                                           ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);
    ecma_free_value (capture_value);
  }

  ecma_builtin_helper_def_prop (result_p,
                                ecma_get_magic_string (LIT_MAGIC_STRING_INDEX),
                                ecma_make_uint32_value (index),
                                ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);

  ecma_builtin_helper_def_prop (result_p,
                                ecma_get_magic_string (LIT_MAGIC_STRING_INPUT),
                                ecma_make_string_value (input_string_p),
                                ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);

  return result_array;
} /* ecma_regexp_create_result_object */

/**
 * Helper function to initialize a regexp match context
 */
static void
ecma_regexp_initialize_context (ecma_regexp_ctx_t *ctx_p, /**< regexp context */
                                const re_compiled_code_t *bc_p, /**< regexp bytecode */
                                const lit_utf8_byte_t *input_start_p, /**< pointer to input string */
                                const lit_utf8_byte_t *input_end_p) /**< pointer to end of input string */
{
  JERRY_ASSERT (ctx_p != NULL);
  JERRY_ASSERT (bc_p != NULL);
  JERRY_ASSERT (input_start_p != NULL);
  JERRY_ASSERT (input_end_p >= input_start_p);

  ctx_p->input_start_p = input_start_p;
  ctx_p->input_end_p = input_end_p;

  ctx_p->captures_count = bc_p->captures_count;
  ctx_p->captures_p = jmem_heap_alloc_block (ctx_p->captures_count * sizeof (ecma_regexp_capture_t));
  memset (ctx_p->captures_p, 0, ctx_p->captures_count * sizeof (ecma_regexp_capture_t));

  ctx_p->non_captures_count = bc_p->non_captures_count;
  ctx_p->non_captures_p = jmem_heap_alloc_block (ctx_p->non_captures_count * sizeof (ecma_regexp_non_capture_t));
  memset (ctx_p->non_captures_p, 0, ctx_p->non_captures_count * sizeof (ecma_regexp_non_capture_t));

  const uint32_t iters_length = ctx_p->captures_count + ctx_p->non_captures_count - 1;
  ctx_p->iterations_p = jmem_heap_alloc_block (iters_length * sizeof (uint32_t));
  memset (ctx_p->iterations_p, 0, iters_length * sizeof (uint32_t));
} /* ecma_regexp_initialize_context */

/**
 * Helper function to clean up a regexp context
 */
static void
ecma_regexp_cleanup_context (ecma_regexp_ctx_t *ctx_p) /**< regexp context */
{
  JERRY_ASSERT (ctx_p != NULL);
  jmem_heap_free_block (ctx_p->captures_p, ctx_p->captures_count * sizeof (ecma_regexp_capture_t));
  if (ctx_p->non_captures_p != NULL)
  {
    jmem_heap_free_block (ctx_p->non_captures_p, ctx_p->non_captures_count * sizeof (ecma_regexp_non_capture_t));
  }
  if (ctx_p->iterations_p != NULL)
  {
    const uint32_t iters_length = ctx_p->captures_count + ctx_p->non_captures_count - 1;
    jmem_heap_free_block (ctx_p->iterations_p, iters_length * sizeof (uint32_t));
  }
} /* ecma_regexp_cleanup_context */

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
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  JERRY_ASSERT (ecma_is_value_object (regexp_value));
  JERRY_ASSERT (ecma_is_value_string (input_string));

  ecma_object_t *regexp_object_p = ecma_get_object_from_value (regexp_value);

  JERRY_ASSERT (ecma_object_class_is (regexp_object_p, LIT_MAGIC_STRING_REGEXP_UL));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) regexp_object_p;
  re_compiled_code_t *bc_p = ECMA_GET_INTERNAL_VALUE_ANY_POINTER (re_compiled_code_t,
                                                                  ext_object_p->u.class_prop.u.value);

  ecma_regexp_ctx_t re_ctx;
  ecma_string_t *input_string_p = ecma_get_string_from_value (input_string);

  if (bc_p == NULL)
  {
#if ENABLED (JERRY_ES2015)
    return ecma_raise_type_error (ECMA_ERR_MSG ("Incompatible type"));
#else /* !ENABLED (JERRY_ES2015) */
    /* Missing bytecode means the RegExp object is the RegExp.prototype,
     * which will always result in an empty string match. */
    re_ctx.captures_count = 1;

    re_ctx.captures_p = jmem_heap_alloc_block (sizeof (ecma_regexp_capture_t));
    re_ctx.captures_p->begin_p = lit_get_magic_string_utf8 (LIT_MAGIC_STRING__EMPTY);
    re_ctx.captures_p->end_p = lit_get_magic_string_utf8 (LIT_MAGIC_STRING__EMPTY);

    ret_value = ecma_regexp_create_result_object (&re_ctx, input_string_p, 0);

    jmem_heap_free_block (re_ctx.captures_p, sizeof (ecma_regexp_capture_t));
    return ret_value;
#endif /* ENABLED (JERRY_ES2015) */
  }

  re_ctx.flags = bc_p->header.status_flags;

  if (ignore_global)
  {
    re_ctx.flags &= (uint16_t) ~RE_FLAG_GLOBAL;
  }

  lit_utf8_size_t input_size;
  lit_utf8_size_t input_length;
  uint8_t input_flags = ECMA_STRING_FLAG_IS_ASCII;
  const lit_utf8_byte_t *input_buffer_p = ecma_string_get_chars (input_string_p,
                                                                 &input_size,
                                                                 &input_length,
                                                                 NULL,
                                                                 &input_flags);

  const lit_utf8_byte_t *input_curr_p = input_buffer_p;
  uint32_t index = 0;
  if (re_ctx.flags & (RE_FLAG_GLOBAL | RE_FLAG_STICKY))
  {
    ecma_string_t *lastindex_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL);
    ecma_value_t lastindex_value = ecma_op_object_get_own_data_prop (regexp_object_p, lastindex_str_p);

    ecma_number_t lastindex_num;
    ret_value = ecma_get_number (lastindex_value, &lastindex_num);
    ecma_free_value (lastindex_value);

    if (ECMA_IS_VALUE_ERROR (ret_value))
    {
      goto cleanup_string;
    }

    /* TODO: Replace with ToLength */
    if (lastindex_num < 0.0f)
    {
#if ENABLED (JERRY_ES2015)
      lastindex_num = 0.0f;
#else /* !ENABLED (JERRY_ES2015) */
      lastindex_num = input_length + 1;
#endif /* ENABLED (JERRY_ES2015) */
    }
    index = ecma_number_to_uint32 (lastindex_num);

    if (index > input_length)
    {
      ret_value = ecma_op_object_put (regexp_object_p,
                                      lastindex_str_p,
                                      ecma_make_integer_value (0),
                                      true);

      if (!ECMA_IS_VALUE_ERROR (ret_value))
      {
        JERRY_ASSERT (ecma_is_value_boolean (ret_value));
        /* lastIndex is out of bounds, the match should fail. */
        ret_value = ECMA_VALUE_NULL;
      }

      goto cleanup_string;
    }

    if (index > 0)
    {
      if (input_flags & ECMA_STRING_FLAG_IS_ASCII)
      {
        input_curr_p += index;
      }
      else
      {
        for (uint32_t i = 0; i < index; i++)
        {
          lit_utf8_incr (&input_curr_p);
        }
      }
    }
  }

  const lit_utf8_byte_t *input_end_p = input_buffer_p + input_size;
  ecma_regexp_initialize_context (&re_ctx,
                                  bc_p,
                                  input_buffer_p,
                                  input_end_p);

  /* 2. Try to match */
  uint8_t *bc_start_p = (uint8_t *) (bc_p + 1);
  const lit_utf8_byte_t *matched_p = NULL;

  JERRY_TRACE_MSG ("Exec with flags [%x]\n", re_ctx.flags);

  JERRY_ASSERT (index <= input_length);
  while (true)
  {
    matched_p = ecma_regexp_match (&re_ctx, bc_start_p, input_curr_p);

    if (matched_p != NULL)
    {
      break;
    }

#if ENABLED (JERRY_ES2015)
    if (re_ctx.flags & RE_FLAG_STICKY)
    {
      ecma_value_t put_result = ecma_op_object_put (regexp_object_p,
                                                    ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL),
                                                    ecma_make_uint32_value (0),
                                                    true);
      if (ECMA_IS_VALUE_ERROR (put_result))
      {
        ret_value = put_result;
        goto cleanup_context;
      }

      JERRY_ASSERT (ecma_is_value_boolean (put_result));
      ret_value = ECMA_VALUE_NULL;
      goto cleanup_context;
    }
#endif /* ENABLED (JERRY_ES2015) */

    if (input_curr_p >= input_end_p)
    {
      if (re_ctx.flags & RE_FLAG_GLOBAL)
      {
        ecma_value_t put_result = ecma_op_object_put (regexp_object_p,
                                                      ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL),
                                                      ecma_make_uint32_value (0),
                                                      true);
        if (ECMA_IS_VALUE_ERROR (put_result))
        {
          ret_value = put_result;
          goto cleanup_context;
        }

        JERRY_ASSERT (ecma_is_value_boolean (put_result));
      }

      /* Failed to match, return 'null'. */
      ret_value = ECMA_VALUE_NULL;
      goto cleanup_context;
    }

    JERRY_ASSERT (input_curr_p < input_end_p);

#if ENABLED (JERRY_ES2015)
    if (re_ctx.flags & RE_FLAG_UNICODE)
    {
      index++;
      const lit_code_point_t cp = ecma_regexp_unicode_advance (&input_curr_p,
                                                               input_end_p);

      if (cp > LIT_UTF16_CODE_UNIT_MAX)
      {
        index++;
      }

      continue;
    }
#endif /* ENABLED (JERRY_ES2015) */

    index++;
    lit_utf8_incr (&input_curr_p);
  }

  JERRY_ASSERT (matched_p != NULL);

  if (ECMA_RE_STACK_LIMIT_REACHED (matched_p))
  {
    ret_value = ecma_raise_range_error (ECMA_ERR_MSG ("Stack limit exceeded."));
    goto cleanup_context;
  }

  if (re_ctx.flags & (RE_FLAG_GLOBAL | RE_FLAG_STICKY))
  {
    JERRY_ASSERT (index <= input_length);

    lit_utf8_size_t match_length;
    const lit_utf8_byte_t *match_begin_p = re_ctx.captures_p[0].begin_p;
    const lit_utf8_byte_t *match_end_p = re_ctx.captures_p[0].end_p;

    if (input_flags & ECMA_STRING_FLAG_IS_ASCII)
    {
      match_length = (lit_utf8_size_t) (match_end_p - match_begin_p);
    }
    else
    {
      match_length = lit_utf8_string_length (match_begin_p,
                                             (lit_utf8_size_t) (match_end_p - match_begin_p));
    }

    ecma_value_t put_result = ecma_op_object_put (regexp_object_p,
                                                  ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL),
                                                  ecma_make_uint32_value (index + match_length),
                                                  true);
    if (ECMA_IS_VALUE_ERROR (put_result))
    {
      ret_value = put_result;
      goto cleanup_context;
    }

    JERRY_ASSERT (ecma_is_value_boolean (put_result));
  }

  ret_value = ecma_regexp_create_result_object (&re_ctx, input_string_p, index);

cleanup_context:
  ecma_regexp_cleanup_context (&re_ctx);

cleanup_string:
  if (input_flags & ECMA_STRING_FLAG_MUST_BE_FREED)
  {
    jmem_heap_free_block ((void *) input_buffer_p, input_size);
  }

  return ret_value;
} /* ecma_regexp_exec_helper */

/**
 * Helper function for converting a RegExp pattern parameter to string.
 *
 * See also:
 *         RegExp.compile
 *         RegExp dispatch call
 *
 * @return empty value if success, error value otherwise
 *         Returned value must be freed with ecma_free_value.
 */
ecma_string_t *
ecma_regexp_read_pattern_str_helper (ecma_value_t pattern_arg) /**< the RegExp pattern */
{
  if (!ecma_is_value_undefined (pattern_arg))
  {
    ecma_string_t *pattern_string_p = ecma_op_to_string (pattern_arg);
    if (JERRY_UNLIKELY (pattern_string_p == NULL) || !ecma_string_is_empty (pattern_string_p))
    {
      return pattern_string_p;
    }
  }

  return ecma_get_magic_string (LIT_MAGIC_STRING_EMPTY_NON_CAPTURE_GROUP);
} /* ecma_regexp_read_pattern_str_helper */

/**
 * Fast path for RegExp based replace operation
 *
 * This method assumes the following:
 *   - The RegExp object is a built-in RegExp
 *   - The 'exec' method of the RegExp object is the built-in 'exec' method
 *   - The 'lastIndex' property is writable
 *
 * The standard would normally require us to first execute the regexp and collect the results,
 * and after that iterate over the collected results and replace them.
 * The assumptions above guarantee that during the matching phase there will be no exceptions thrown,
 * which means we can do the match/replace in a single loop, without collecting the results.
 *
 * @return string value if successful
 *         thrown value otherwise
 */
static ecma_value_t
ecma_regexp_replace_helper_fast (ecma_replace_context_t *ctx_p, /**<replace context */
                                 const re_compiled_code_t *bc_p, /**< regexp bytecode */
                                 ecma_string_t *string_p, /**< source string */
                                 ecma_value_t replace_arg) /**< replace argument */
{
  JERRY_ASSERT (bc_p != NULL);
  ecma_value_t result = ECMA_VALUE_EMPTY;
  ecma_regexp_ctx_t re_ctx;
  re_ctx.flags = bc_p->header.status_flags;

  uint8_t string_flags = ECMA_STRING_FLAG_IS_ASCII;
  lit_utf8_size_t string_length;
  ctx_p->string_p = ecma_string_get_chars (string_p,
                                           &(ctx_p->string_size),
                                           &string_length,
                                           NULL,
                                           &string_flags);

  const lit_utf8_byte_t *const string_end_p = ctx_p->string_p + ctx_p->string_size;
  const uint8_t *const bc_start_p = (const uint8_t *) (bc_p + 1);
  const lit_utf8_byte_t *matched_p = NULL;
  const lit_utf8_byte_t *current_p = ctx_p->string_p;
  const lit_utf8_byte_t *last_append_p = current_p;
  JERRY_ASSERT (ctx_p->index <= string_length);

#if ENABLED (JERRY_ES2015)
  /* Global matches always start at index 0, but Sticky matches may have a non-zero lastIndex. */
  if (ctx_p->index > 0)
  {
    if (string_flags & ECMA_STRING_FLAG_IS_ASCII)
    {
      current_p += ctx_p->index;
    }
    else
    {
      ecma_length_t index = ctx_p->index;
      while (index--)
      {
        lit_utf8_incr (&current_p);
      }
    }
  }
#endif /* ENABLED (JERRY_ES2015) */

  ecma_regexp_initialize_context (&re_ctx,
                                  bc_p,
                                  ctx_p->string_p,
                                  string_end_p);

  ctx_p->builder = ecma_stringbuilder_create ();
  ctx_p->capture_count = re_ctx.captures_count;
  ctx_p->u.captures_p = re_ctx.captures_p;

  while (true)
  {
    memset (re_ctx.captures_p, 0, re_ctx.captures_count);
    matched_p = ecma_regexp_match (&re_ctx, bc_start_p, current_p);

    if (matched_p != NULL)
    {
      if (ECMA_RE_STACK_LIMIT_REACHED (matched_p))
      {
        result = ecma_raise_range_error (ECMA_ERR_MSG ("Stack limit exceeded."));
        goto cleanup_builder;
      }

      const lit_utf8_size_t remaining_size = (lit_utf8_size_t) (current_p - last_append_p);
      ecma_stringbuilder_append_raw (&(ctx_p->builder), last_append_p, remaining_size);

      if (ctx_p->replace_str_p != NULL)
      {
        ctx_p->matched_p = current_p;
        const ecma_regexp_capture_t *const global_capture_p = re_ctx.captures_p;
        ctx_p->matched_size = (lit_utf8_size_t) (global_capture_p->end_p - global_capture_p->begin_p);
        ctx_p->match_byte_pos = (lit_utf8_size_t) (current_p - re_ctx.input_start_p);

        ecma_builtin_replace_substitute (ctx_p);
      }
      else
      {
        ecma_collection_t *arguments_p = ecma_new_collection ();

        for (uint32_t i = 0; i < re_ctx.captures_count; i++)
        {
          ecma_value_t capture = ecma_regexp_get_capture_value (re_ctx.captures_p + i);
          ecma_collection_push_back (arguments_p, capture);
        }

        ecma_collection_push_back (arguments_p, ecma_make_uint32_value (ctx_p->index));
        ecma_ref_ecma_string (string_p);
        ecma_collection_push_back (arguments_p, ecma_make_string_value (string_p));
        ecma_object_t *function_p = ecma_get_object_from_value (replace_arg);

        result = ecma_op_function_call (function_p,
                                        ECMA_VALUE_UNDEFINED,
                                        arguments_p->buffer_p,
                                        arguments_p->item_count);

        ecma_collection_free (arguments_p);

        if (ECMA_IS_VALUE_ERROR (result))
        {
          goto cleanup_builder;
        }

        /* 16.m.v */
        ecma_string_t *const replace_result_p = ecma_op_to_string (result);
        ecma_free_value (result);

        if (replace_result_p == NULL)
        {
          result = ECMA_VALUE_ERROR;
          goto cleanup_builder;
        }

        ecma_stringbuilder_append (&(ctx_p->builder), replace_result_p);
        ecma_deref_ecma_string (replace_result_p);
      }

      const ecma_regexp_capture_t *global_capture_p = re_ctx.captures_p;
      last_append_p = global_capture_p->end_p;

      if (!(re_ctx.flags & RE_FLAG_GLOBAL))
      {
        break;
      }

      const lit_utf8_size_t matched_size = (lit_utf8_size_t) (global_capture_p->end_p - global_capture_p->begin_p);
      if (matched_size > 0)
      {
        ctx_p->index += lit_utf8_string_length (current_p, matched_size);
        current_p = last_append_p;
        continue;
      }
    }

    if (current_p >= string_end_p)
    {
      break;
    }

#if ENABLED (JERRY_ES2015)
    if ((re_ctx.flags & RE_FLAG_UNICODE) != 0)
    {
      ctx_p->index++;
      const lit_code_point_t cp = ecma_regexp_unicode_advance (&current_p,
                                                               string_end_p);

      if (cp > LIT_UTF16_CODE_UNIT_MAX)
      {
        ctx_p->index++;
      }

      continue;
    }
#endif /* ENABLED (JERRY_ES2015) */

    ctx_p->index++;
    lit_utf8_incr (&current_p);
  }

  const lit_utf8_size_t trailing_size = (lit_utf8_size_t) (string_end_p - last_append_p);
  ecma_stringbuilder_append_raw (&(ctx_p->builder), last_append_p, trailing_size);

  result = ecma_make_string_value (ecma_stringbuilder_finalize (&(ctx_p->builder)));
  goto cleanup_context;

cleanup_builder:
  ecma_stringbuilder_destroy (&(ctx_p->builder));

cleanup_context:
  ecma_regexp_cleanup_context (&re_ctx);

  if (string_flags & ECMA_STRING_FLAG_MUST_BE_FREED)
  {
    jmem_heap_free_block ((void *) ctx_p->string_p, ctx_p->string_size);
  }

  return result;
} /* ecma_regexp_replace_helper_fast */

/**
 * Helper function for RegExp based replacing
 *
 * See also:
 *          String.prototype.replace
 *          RegExp.prototype[@@replace]
 *
 * @return result string of the replacement, if successful
 *         error value, otherwise
 */
ecma_value_t
ecma_regexp_replace_helper (ecma_value_t this_arg, /**< this argument */
                            ecma_value_t string_arg, /**< source string */
                            ecma_value_t replace_arg) /**< replace string */
{
  /* 2. */
  if (!ecma_is_value_object (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("'this' is not an object."));
  }

  ecma_object_t *this_obj_p = ecma_get_object_from_value (this_arg);

  ecma_replace_context_t replace_ctx;
  replace_ctx.index = 0;

  /* 3. */
  ecma_string_t *string_p = ecma_op_to_string (string_arg);
  if (string_p == NULL)
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t result = ECMA_VALUE_ERROR;

  /* 6. */
  replace_ctx.replace_str_p = NULL;
  if (!ecma_op_is_callable (replace_arg))
  {
    replace_ctx.replace_str_p = ecma_op_to_string (replace_arg);

    if (replace_ctx.replace_str_p == NULL)
    {
      goto cleanup_string;
    }
  }

  /* 8 */
  result = ecma_op_object_get_by_magic_id (this_obj_p, LIT_MAGIC_STRING_GLOBAL);
  if (ECMA_IS_VALUE_ERROR (result))
  {
    goto cleanup_replace;
  }

  const bool global = ecma_op_to_boolean (result);
  ecma_free_value (result);

#if ENABLED (JERRY_ES2015)
  const lit_utf8_size_t string_length = ecma_string_get_length (string_p);
  bool unicode = false;
#endif /* ENABLED (JERRY_ES2015) */

  /* 10. */
  if (global)
  {
#if ENABLED (JERRY_ES2015)
    result = ecma_op_object_get_by_magic_id (this_obj_p, LIT_MAGIC_STRING_UNICODE);
    if (ECMA_IS_VALUE_ERROR (result))
    {
      goto cleanup_replace;
    }

    unicode = ecma_op_to_boolean (result);
    ecma_free_value (result);
#endif /* ENABLED (JERRY_ES2015) */

    result = ecma_op_object_put (this_obj_p,
                                 ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL),
                                 ecma_make_uint32_value (0),
                                 true);
    if (ECMA_IS_VALUE_ERROR (result))
    {
      goto cleanup_replace;
    }

    JERRY_ASSERT (ecma_is_value_boolean (result));
  }

#if !ENABLED (JERRY_ES2015)
  ecma_extended_object_t *re_obj_p = (ecma_extended_object_t *) this_obj_p;
  const re_compiled_code_t *bc_p = ECMA_GET_INTERNAL_VALUE_ANY_POINTER (re_compiled_code_t,
                                                                        re_obj_p->u.class_prop.u.value);
  /* In ES5.1 the RegExp prototype object is a valid regexp, but we don't store bytecode for it to save memory.
   * Handling this would be very awkward, so we temporarily compile bytecode for it. */
  if (ecma_builtin_is (this_obj_p, ECMA_BUILTIN_ID_REGEXP_PROTOTYPE))
  {
    JERRY_ASSERT (bc_p == NULL);
    ecma_value_t compile = re_compile_bytecode (&bc_p,
                                                ecma_get_magic_string (LIT_MAGIC_STRING_EMPTY_NON_CAPTURE_GROUP),
                                                RE_FLAG_EMPTY);
    JERRY_ASSERT (ecma_is_value_empty (compile));
  }

  result = ecma_regexp_replace_helper_fast (&replace_ctx,
                                            bc_p,
                                            string_p,
                                            replace_arg);

  if (ecma_builtin_is (this_obj_p, ECMA_BUILTIN_ID_REGEXP_PROTOTYPE))
  {
    ecma_bytecode_deref ((ecma_compiled_code_t *) bc_p);
  }

  goto cleanup_replace;
#else /* ENABLED (JERRY_ES2015) */
  result = ecma_op_object_get_by_magic_id (this_obj_p, LIT_MAGIC_STRING_EXEC);

  if (ECMA_IS_VALUE_ERROR (result))
  {
    goto cleanup_replace;
  }

  /* Check for fast path. */
  if (ecma_op_is_callable (result))
  {
    ecma_extended_object_t *function_p = (ecma_extended_object_t *) ecma_get_object_from_value (result);
    if (ecma_object_class_is (this_obj_p, LIT_MAGIC_STRING_REGEXP_UL)
        && !ecma_builtin_is (this_obj_p, ECMA_BUILTIN_ID_REGEXP_PROTOTYPE)
        && ecma_builtin_is_regexp_exec (function_p))
    {
      result = ecma_op_object_get_by_magic_id (this_obj_p, LIT_MAGIC_STRING_STICKY);
      if (ECMA_IS_VALUE_ERROR (result))
      {
        goto cleanup_replace;
      }

      const bool sticky = ecma_op_to_boolean (result);
      ecma_free_value (result);

      if (sticky && !global)
      {
        ecma_string_t *lastindex_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL);
        ecma_value_t lastindex_value = ecma_op_object_get_own_data_prop (this_obj_p, lastindex_str_p);

        result = ecma_op_to_length (lastindex_value, &replace_ctx.index);
        ecma_free_value (lastindex_value);

        if (ECMA_IS_VALUE_ERROR (result))
        {
          goto cleanup_replace;
        }

        if (replace_ctx.index > string_length)
        {
          ecma_deref_object ((ecma_object_t *) function_p);

          result = ecma_op_object_put (this_obj_p,
                                       ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL),
                                       ecma_make_uint32_value (0),
                                       true);
          JERRY_ASSERT (ecma_is_value_true (result));

          ecma_ref_ecma_string (string_p);
          result = ecma_make_string_value (string_p);
          goto cleanup_replace;
        }
      }

      ecma_extended_object_t *re_obj_p = (ecma_extended_object_t *) this_obj_p;
      const re_compiled_code_t *bc_p = ECMA_GET_INTERNAL_VALUE_ANY_POINTER (re_compiled_code_t,
                                                                            re_obj_p->u.class_prop.u.value);

      result = ecma_regexp_replace_helper_fast (&replace_ctx,
                                                bc_p,
                                                string_p,
                                                replace_arg);

      ecma_deref_object ((ecma_object_t *) function_p);
      goto cleanup_replace;
    }
  }

  ecma_collection_t *results_p = ecma_new_collection ();

  while (true)
  {
    /* 13.a */
    if (ecma_op_is_callable (result))
    {
      ecma_object_t *const function_p = ecma_get_object_from_value (result);

      ecma_value_t arguments[] = { ecma_make_string_value (string_p) };
      result = ecma_op_function_call (function_p, this_arg, arguments, 1);

      ecma_deref_object (function_p);

      if (ECMA_IS_VALUE_ERROR (result))
      {
        goto cleanup_results;
      }

      if (!ecma_is_value_object (result) && !ecma_is_value_null (result))
      {
        ecma_free_value (result);
        result = ecma_raise_type_error (ECMA_ERR_MSG ("Return value of 'exec' must be an Object or Null"));
        goto cleanup_results;
      }
    }
    else
    {
      ecma_free_value (result);

      if (!ecma_object_class_is (this_obj_p, LIT_MAGIC_STRING_REGEXP_UL))
      {
        result = ecma_raise_type_error (ECMA_ERR_MSG ("'this' is not a valid RegExp object"));
        goto cleanup_results;
      }

      result = ecma_regexp_exec_helper (this_arg, ecma_make_string_value (string_p), false);
    }

    /* 13.c */
    if (ecma_is_value_null (result))
    {
      break;
    }

    /* 13.d.i */
    ecma_collection_push_back (results_p, result);

    if (!global)
    {
      break;
    }

    /* 13.d.iii.1 */
    result = ecma_op_object_get_by_uint32_index (ecma_get_object_from_value (result), 0);
    if (ECMA_IS_VALUE_ERROR (result))
    {
      goto cleanup_results;
    }

    ecma_string_t *match_str_p = ecma_op_to_string (result);
    ecma_free_value (result);

    if (match_str_p == NULL)
    {
      result = ECMA_VALUE_ERROR;
      goto cleanup_results;
    }

    const bool is_empty = ecma_string_is_empty (match_str_p);
    ecma_deref_ecma_string (match_str_p);

    /* 13.d.iii.3 */
    if (is_empty)
    {
      result = ecma_op_object_get_by_magic_id (this_obj_p, LIT_MAGIC_STRING_LASTINDEX_UL);
      if (ECMA_IS_VALUE_ERROR (result))
      {
        goto cleanup_results;
      }

      uint32_t index;
      if (ECMA_IS_VALUE_ERROR (ecma_op_to_length (result, &index)))
      {
        ecma_free_value (result);
        result = ECMA_VALUE_ERROR;
        goto cleanup_results;
      }

      ecma_free_value (result);

      index = ecma_op_advance_string_index (string_p, index, unicode);

      /* 10.d.iii.3.c */
      result = ecma_op_object_put (this_obj_p,
                                   ecma_get_magic_string (LIT_MAGIC_STRING_LASTINDEX_UL),
                                   ecma_make_uint32_value (index),
                                   true);

      if (ECMA_IS_VALUE_ERROR (result))
      {
        goto cleanup_results;
      }

      JERRY_ASSERT (ecma_is_value_boolean (result));
    }

    result = ecma_op_object_get_by_magic_id (this_obj_p, LIT_MAGIC_STRING_EXEC);

    if (ECMA_IS_VALUE_ERROR (result))
    {
      goto cleanup_results;
    }
  }

  uint8_t string_flags = ECMA_STRING_FLAG_IS_ASCII;
  replace_ctx.string_p = ecma_string_get_chars (string_p,
                                                &(replace_ctx.string_size),
                                                NULL,
                                                NULL,
                                                &string_flags);

  /* 14. */
  replace_ctx.builder = ecma_stringbuilder_create ();
  replace_ctx.matched_p = NULL;
  replace_ctx.capture_count = 0;

  /* 15. */
  const lit_utf8_byte_t *source_position_p = replace_ctx.string_p;
  const lit_utf8_byte_t *const string_end_p = replace_ctx.string_p + replace_ctx.string_size;

  /* 16. */
  for (ecma_value_t *current_p = results_p->buffer_p;
       current_p < results_p->buffer_p + results_p->item_count;
       current_p++)
  {
    /* 16.a */
    ecma_object_t *current_object_p = ecma_get_object_from_value (*current_p);

    uint32_t capture_count;
    result = ecma_op_object_get_length (current_object_p, &capture_count);
    if (ECMA_IS_VALUE_ERROR (result))
    {
      goto cleanup_builder;
    }

    /* 16.c */
    capture_count = (capture_count > 0) ? capture_count - 1 : capture_count;

    /* 16.d */
    result = ecma_op_object_get_by_uint32_index (current_object_p, 0);
    if (ECMA_IS_VALUE_ERROR (result))
    {
      goto cleanup_builder;
    }

    ecma_string_t *matched_str_p = ecma_op_to_string (result);
    ecma_free_value (result);

    /* 16.e */
    if (matched_str_p == NULL)
    {
      result = ECMA_VALUE_ERROR;
      goto cleanup_builder;
    }

    /* 16.g */
    result = ecma_op_object_get_by_magic_id (current_object_p, LIT_MAGIC_STRING_INDEX);
    if (ECMA_IS_VALUE_ERROR (result))
    {
      ecma_deref_ecma_string (matched_str_p);
      goto cleanup_builder;
    }

    const ecma_value_t index_value = result;

    ecma_number_t position_num;
    result = ecma_op_to_integer (index_value, &position_num);
    ecma_free_value (index_value);

    if (ECMA_IS_VALUE_ERROR (result))
    {
      ecma_deref_ecma_string (matched_str_p);
      goto cleanup_builder;
    }

    /* 16.i */
    lit_utf8_size_t position = JERRY_MIN ((lit_utf8_size_t) JERRY_MAX (position_num, 0.0f), string_length);

    /* 16.k */
    ecma_collection_t *arguments_p = ecma_new_collection ();
    ecma_collection_push_back (arguments_p, ecma_make_string_value (matched_str_p));

    /* 16.j, l */
    uint32_t n = 1;
    while (n <= capture_count)
    {
      result = ecma_op_object_get_by_uint32_index (current_object_p, n);
      if (ECMA_IS_VALUE_ERROR (result))
      {
        ecma_collection_free (arguments_p);
        goto cleanup_builder;
      }

      /* 16.l.iii */
      if (!ecma_is_value_undefined (result))
      {
        ecma_string_t *capture_str_p = ecma_op_to_string (result);
        ecma_free_value (result);

        if (capture_str_p == NULL)
        {
          ecma_collection_free (arguments_p);
          result = ECMA_VALUE_ERROR;
          goto cleanup_builder;
        }

        result = ecma_make_string_value (capture_str_p);
      }

      /* 16.l.iv */
      ecma_collection_push_back (arguments_p, result);
      n++;
    }

    const bool should_replace = (position >= replace_ctx.index);
    /* 16.p */
    if (should_replace)
    {
      const lit_utf8_byte_t *match_position_p;
      const lit_utf8_size_t matched_str_size = ecma_string_get_size (matched_str_p);
      const lit_utf8_size_t matched_str_length = ecma_string_get_length (matched_str_p);

      if (string_flags & ECMA_STRING_FLAG_IS_ASCII)
      {
        match_position_p = replace_ctx.string_p + position;
      }
      else
      {
        match_position_p = source_position_p;
        lit_utf8_size_t distance = position - replace_ctx.index;
        while (distance--)
        {
          lit_utf8_incr (&match_position_p);
        }
      }

      ecma_stringbuilder_append_raw (&replace_ctx.builder,
                                    source_position_p,
                                    (lit_utf8_size_t) (match_position_p - source_position_p));
      replace_ctx.match_byte_pos = (lit_utf8_size_t) (match_position_p - replace_ctx.string_p);

      source_position_p = JERRY_MIN (match_position_p + matched_str_size, string_end_p);
      replace_ctx.index = JERRY_MIN (position + matched_str_length, string_length);
    }

    /* 16.m */
    if (replace_ctx.replace_str_p == NULL)
    {
      /* 16.m.i-ii.
       * arguments_p already contains <<Matched, cap1, cap2, ..., capN>> */

      /* 16.m.iii */
      ecma_collection_push_back (arguments_p, ecma_make_uint32_value (position));
      ecma_ref_ecma_string (string_p);
      ecma_collection_push_back (arguments_p, ecma_make_string_value (string_p));

      result = ecma_op_function_call (ecma_get_object_from_value (replace_arg),
                                      ECMA_VALUE_UNDEFINED,
                                      arguments_p->buffer_p,
                                      arguments_p->item_count);

      ecma_collection_free (arguments_p);

      if (ECMA_IS_VALUE_ERROR (result))
      {
        goto cleanup_builder;
      }

      /* 16.m.v */
      ecma_string_t *const replace_result_p = ecma_op_to_string (result);
      ecma_free_value (result);

      if (replace_result_p == NULL)
      {
        result = ECMA_VALUE_ERROR;
        goto cleanup_builder;
      }

      /* 16.m/p */
      if (should_replace)
      {
        ecma_stringbuilder_append (&replace_ctx.builder, replace_result_p);
      }

      ecma_deref_ecma_string (replace_result_p);
    }
    else
    {
      /* 16.n/p */
      if (should_replace)
      {
        replace_ctx.u.collection_p = arguments_p;
        ecma_builtin_replace_substitute (&replace_ctx);
      }

      ecma_collection_free (arguments_p);
    }
  }

  /* 18. */
  JERRY_ASSERT (replace_ctx.index <= string_length);
  ecma_stringbuilder_append_raw (&(replace_ctx.builder),
                                 source_position_p,
                                 (lit_utf8_size_t) (string_end_p - source_position_p));

  result = ecma_make_string_value (ecma_stringbuilder_finalize (&replace_ctx.builder));
  goto cleanup_chars;

cleanup_builder:
  ecma_stringbuilder_destroy (&replace_ctx.builder);

cleanup_chars:
  if (string_flags & ECMA_STRING_FLAG_MUST_BE_FREED)
  {
    jmem_heap_free_block ((void *) replace_ctx.string_p, replace_ctx.string_size);
  }

cleanup_results:
  ecma_collection_free (results_p);
#endif /* !ENABLED (JERRY_ES2015) */

cleanup_replace:
  if (replace_ctx.replace_str_p != NULL)
  {
    ecma_deref_ecma_string (replace_ctx.replace_str_p);
  }

cleanup_string:
  ecma_deref_ecma_string (string_p);

  return result;
} /* ecma_regexp_replace_helper */

/**
 * RegExpExec operation
 *
 * See also:
 *          ECMA-262 v6.0, 21.2.5.2.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_regexp_exec (ecma_value_t this_arg, /**< this argument */
                     ecma_string_t *str_p) /**< input string */
{
#if ENABLED (JERRY_ES2015)
  ecma_object_t *arg_obj_p = ecma_get_object_from_value (this_arg);

  ecma_value_t exec = ecma_op_object_get_by_magic_id (arg_obj_p, LIT_MAGIC_STRING_EXEC);

  if (ECMA_IS_VALUE_ERROR (exec))
  {
    return exec;
  }

  if (ecma_op_is_callable (exec))
  {
    ecma_object_t *function_p = ecma_get_object_from_value (exec);
    ecma_value_t arguments[] = { ecma_make_string_value (str_p) };

    ecma_value_t result = ecma_op_function_call (function_p, this_arg, arguments, 1);

    ecma_deref_object (function_p);

    if (ECMA_IS_VALUE_ERROR (result))
    {
      return result;
    }

    if (!ecma_is_value_object (result) && !ecma_is_value_null (result))
    {
      ecma_free_value (result);
      return ecma_raise_type_error (ECMA_ERR_MSG ("Return value of 'exec' must be an Object or Null"));
    }

    return result;
  }
  else
  {
    ecma_free_value (exec);
  }
#endif /* ENABLED (JERRY_ES2015) */

  if (!ecma_object_is_regexp_object (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("'this' is not a valid RegExp object"));
  }

  return ecma_regexp_exec_helper (this_arg, ecma_make_string_value (str_p), false);
} /* ecma_op_regexp_exec */

/**
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_BUILTIN_REGEXP) */
