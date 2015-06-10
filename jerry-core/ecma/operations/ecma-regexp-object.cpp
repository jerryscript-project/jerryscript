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
#include "ecma-array-object.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-objects.h"
#include "ecma-regexp-object.h"
#include "ecma-try-catch-macro.h"
#include "jrt-libc-includes.h"
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

#define RE_GLOBAL_START_IDX 0
#define RE_GLOBAL_END_IDX   1

/* flags */
#define RE_FLAG_GLOBAL              (1 << 0)
#define RE_FLAG_IGNORE_CASE         (1 << 1)
#define RE_FLAG_MULTILINE           (1 << 2)

/**
 * Parse RegExp flags (global, ignoreCase, multiline)
 */
static ecma_completion_value_t
parse_regexp_flags (ecma_string_t *flags_str_p, /**< Input string with flags */
                    uint8_t *flags) /**< Output: parsed flag bits */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  int32_t chars = ecma_string_get_length (flags_str_p);
  MEM_DEFINE_LOCAL_ARRAY (flags_start_p, chars + 1, ecma_char_t);
  ssize_t zt_str_size = (ssize_t) sizeof (ecma_char_t) * (chars + 1);
  ecma_string_to_zt_string (flags_str_p, flags_start_p, zt_str_size);

  ecma_char_t *flags_char_p = flags_start_p;
  while (flags_char_p
         && *flags_char_p != '\0'
         && ecma_is_completion_value_empty (ret_value))
  {
    ecma_char_t ch = *flags_char_p;
    switch (ch)
    {
      case 'g':
      {
        if (*flags & RE_FLAG_GLOBAL)
        {
          SYNTAX_ERROR_OBJ (ret_value, "Invalid RegExp flags.");
        }
        *flags |= RE_FLAG_GLOBAL;
        break;
      }
      case 'i':
      {
        if (*flags & RE_FLAG_IGNORE_CASE)
        {
          SYNTAX_ERROR_OBJ (ret_value, "Invalid RegExp flags.");
        }
        *flags |= RE_FLAG_IGNORE_CASE;
        break;
      }
      case 'm':
      {
        if (*flags & RE_FLAG_MULTILINE)
        {
          SYNTAX_ERROR_OBJ (ret_value, "Invalid RegExp flags.");
        }
        *flags |= RE_FLAG_MULTILINE;
        break;
      }
      default:
      {
        SYNTAX_ERROR_OBJ (ret_value, "Invalid RegExp flags.");
        break;
      }
    }
    flags_char_p++;
  }

  MEM_FINALIZE_LOCAL_ARRAY (flags_start_p);

  return ret_value;
} /* parse_regexp_flags  */

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
    ECMA_TRY_CATCH (empty, parse_regexp_flags (flags_str_p, &flags), ret_value);
    ECMA_FINALIZE (empty);

    if (!ecma_is_completion_value_empty (ret_value))
    {
      return ret_value;
    }
  }

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN
  ecma_object_t *regexp_prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_REGEXP_PROTOTYPE);
#else /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */
  ecma_object_t *regexp_prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);
#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */

  ecma_object_t *obj_p = ecma_create_object (regexp_prototype_obj_p, true, ECMA_OBJECT_TYPE_GENERAL);
  ecma_deref_object (regexp_prototype_obj_p);

  /* Set source property. ECMA-262 v5, 15.10.7.1 */
  ecma_string_t *magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_SOURCE);
  ecma_property_t *source_prop_p = ecma_create_named_data_property (obj_p,
                                                                    magic_string_p,
                                                                    false, false, false);
  ecma_deref_ecma_string (magic_string_p);
  ecma_set_named_data_property_value (source_prop_p,
                                      ecma_make_string_value (ecma_copy_or_ref_ecma_string (pattern_p)));

  ecma_simple_value_t prop_value;

  /* Set global property. ECMA-262 v5, 15.10.7.2*/
  magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_GLOBAL);
  ecma_property_t *global_prop_p = ecma_create_named_data_property (obj_p,
                                                                    magic_string_p,
                                                                    false, false, false);
  ecma_deref_ecma_string (magic_string_p);
  prop_value = flags & RE_FLAG_GLOBAL ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE;
  ecma_set_named_data_property_value (global_prop_p, ecma_make_simple_value (prop_value));

  /* Set ignoreCase property. ECMA-262 v5, 15.10.7.3*/
  magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_IGNORECASE);
  ecma_property_t *ignorecase_prop_p = ecma_create_named_data_property (obj_p,
                                                                        magic_string_p,
                                                                        false, false, false);
  ecma_deref_ecma_string (magic_string_p);
  prop_value = flags & RE_FLAG_IGNORE_CASE ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE;
  ecma_set_named_data_property_value (ignorecase_prop_p, ecma_make_simple_value (prop_value));


  /* Set multiline property. ECMA-262 v5, 15.10.7.4*/
  magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_MULTILINE);
  ecma_property_t *multiline_prop_p = ecma_create_named_data_property (obj_p,
                                                                       magic_string_p,
                                                                       false, false, false);
  ecma_deref_ecma_string (magic_string_p);
  prop_value = flags & RE_FLAG_MULTILINE ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE;
  ecma_set_named_data_property_value (multiline_prop_p, ecma_make_simple_value (prop_value));

  /* Set lastIndex property. ECMA-262 v5, 15.10.7.5*/
  magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LASTINDEX);
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
  ECMA_TRY_CATCH (empty, regexp_compile_bytecode (bytecode, pattern_p, flags), ret_value);
  ret_value = ecma_make_normal_completion_value (ecma_make_object_value (obj_p));
  ECMA_FINALIZE (empty);

  if (ecma_is_completion_value_throw (ret_value))
  {
    ecma_deref_object (obj_p);
  }

  return ret_value;
} /* ecma_op_create_regexp_object */

static const ecma_char_t*
utf8_backtrack (const ecma_char_t* str_p)
{
  /* FIXME: fix this, when unicode support is finished! */
  return --str_p;
}

/**
 * Helper to get an input character and increase string pointer.
 */
static ecma_char_t
get_input_char (const ecma_char_t** char_p)
{
  /* FIXME: fix this, when unicode support is finished! */
  const ecma_char_t ch = **char_p;
  (*char_p)++;
  return ch;
}

/**
 * Helper to get current input character, won't increase string pointer.
 */
static ecma_char_t
lookup_input_char (const ecma_char_t* str_p)
{
  /* FIXME: fix this, when unicode support is finished! */
  return *str_p;
}

/**
 * Helper to get previous input character, won't decrease string pointer.
 */
static ecma_char_t
lookup_prev_char (const ecma_char_t* str_p)
{
  /* FIXME: fix this, when unicode support is finished! */
  return *(--str_p);
}

/**
 * Helper to check that a unicode character is a word character or not.
 */
static bool
regexp_is_wordchar (const ecma_char_t ch)
{
  if ((ch >= 'a' && ch <= 'z')
      || (ch >= 'A' && ch <= 'Z')
      || (ch >= '0' && ch <= '9')
      || ch == '_')
  {
    return true;
  }
  return false;
}

/**
 * Recursive function for RegExp matching
 */
static ecma_completion_value_t
regexp_match (re_matcher_ctx *re_ctx_p, /**< RegExp matcher context */
              re_bytecode_t *bc_p, /**< pointer to the current RegExp bytecode */
              const ecma_char_t *str_p, /**< pointer to the current input character */
              const ecma_char_t **res)
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  re_opcode_t op;

  if (re_ctx_p->recursion_depth >= RE_EXECUTE_RECURSION_LIMIT)
  {
    RANGE_ERROR_OBJ (ret_value, "RegExp executor recursion limit is exceeded.");
    return ret_value;
  }
  re_ctx_p->recursion_depth++;

  while ((op = get_opcode (&bc_p)))
  {
    if (re_ctx_p->steps_count >= RE_EXECUTE_STEPS_LIMIT)
    {
      RANGE_ERROR_OBJ (ret_value, "RegExp executor steps limit is exceeded.");
      return ret_value;
    }
    re_ctx_p->steps_count++;

    switch (op)
    {
      case RE_OP_MATCH:
      {
        JERRY_DDLOG ("Execute RE_OP_MATCH: match\n");
        *res = str_p;
        re_ctx_p->recursion_depth--;
        ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
        return ret_value; /* match */
      }
      case RE_OP_CHAR:
      {
        uint32_t ch1 = get_value (&bc_p);
        uint32_t ch2 = get_input_char (&str_p);
        JERRY_DDLOG ("Character matching %d to %d: ", ch1, ch2);

        if (ch2 == '\0' || ch1 != ch2)
        {
          JERRY_DDLOG ("fail\n");
          re_ctx_p->recursion_depth--;
          return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }
        JERRY_DDLOG ("match\n");
        break;
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
        break;
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

        if (ecma_char_is_line_terminator (lookup_prev_char (str_p)))
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
          break;
        }

        if (!(re_ctx_p->flags & RE_FLAG_MULTILINE))
        {
          JERRY_DDLOG ("fail\n");
          re_ctx_p->recursion_depth--;
          return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        if (ecma_char_is_line_terminator (lookup_input_char (str_p)))
        {
          JERRY_DDLOG ("match\n");
          break;
        }

        JERRY_DDLOG ("fail\n");
        re_ctx_p->recursion_depth--;
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_ASSERT_WORD_BOUNDARY:
      case RE_OP_ASSERT_NOT_WORD_BOUNDARY:
      {
        bool wordchar1, wordchar2;

        if (str_p <= re_ctx_p->input_start_p)
        {
          wordchar1 = false;  /* not a wordchar */
        }
        else
        {
          wordchar1 = regexp_is_wordchar (lookup_prev_char (str_p));
        }

        if (str_p >= re_ctx_p->input_end_p)
        {
          wordchar2 = false;  /* not a wordchar */
        }
        else
        {
          wordchar2 = regexp_is_wordchar (lookup_input_char (str_p));
        }

        if (op == RE_OP_ASSERT_WORD_BOUNDARY)
        {
          JERRY_DDLOG ("Execute RE_OP_ASSERT_WORD_BOUNDARY at %c: ", *str_p);
          if (wordchar1 == wordchar2)
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

          if (wordchar1 != wordchar2)
          {
            JERRY_DDLOG ("fail\n");
            re_ctx_p->recursion_depth--;
            return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }

        JERRY_DDLOG ("match\n");
        break;
      }
      case RE_OP_LOOKPOS:
      case RE_OP_LOOKNEG:
      {
        const ecma_char_t *sub_str_p = NULL;

        // save the save_p array
        size_t size = (size_t) (re_ctx_p->num_of_captures) * sizeof (const ecma_char_t *);
        void* saved_bck_void_p = mem_heap_alloc_block (size, MEM_HEAP_ALLOC_SHORT_TERM);
        const ecma_char_t **saved_bck_p = static_cast <const ecma_char_t **> (saved_bck_void_p);

        memcpy (saved_bck_p, re_ctx_p->saved_p, size);

        do
        {
          uint32_t offset = get_value (&bc_p);
          if (!sub_str_p)
          {
            ecma_completion_value_t match_value = regexp_match (re_ctx_p, bc_p, str_p, &sub_str_p);
            if (ecma_is_completion_value_throw (match_value))
            {
              mem_heap_free_block (saved_bck_p);
              return match_value;
            }
          }
          bc_p += offset;
        }
        while (get_opcode (&bc_p) == RE_OP_ALTERNATIVE);

        if (op == RE_OP_LOOKPOS)
        {
          JERRY_DDLOG ("Execute RE_OP_LOOKPOS: ");
          if (!sub_str_p)
          {
            JERRY_DDLOG ("fail\n");
            // restore saves
            memcpy (re_ctx_p->saved_p, saved_bck_p, size);
            mem_heap_free_block (saved_bck_p);
            re_ctx_p->recursion_depth--;
            return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
          JERRY_DDLOG ("match\n");
        }
        else
        {
          JERRY_DDLOG ("Execute RE_OP_LOOKNEG: ");
          if (sub_str_p)
          {
            JERRY_DDLOG ("fail\n");
            // restore saves
            memcpy (re_ctx_p->saved_p, saved_bck_p, size);
            mem_heap_free_block (saved_bck_p);
            re_ctx_p->recursion_depth--;
            return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
          JERRY_DDLOG ("match\n");
        }

        ecma_completion_value_t match_value = regexp_match (re_ctx_p, bc_p, str_p, &sub_str_p);
        if (ecma_is_value_true (match_value))
        {
          *res = sub_str_p;
          mem_heap_free_block (saved_bck_p);
          re_ctx_p->recursion_depth--;
          return match_value; /* match */
        }
        else if (ecma_is_completion_value_throw (match_value))
        {
          mem_heap_free_block (saved_bck_p);
          return match_value;
        }

        // restore saves
        memcpy (re_ctx_p->saved_p, saved_bck_p, size);
        mem_heap_free_block (saved_bck_p);
        re_ctx_p->recursion_depth--;
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_CHAR_CLASS:
      case RE_OP_INV_CHAR_CLASS:
      {
        uint32_t curr_ch, q;
        bool match;

        JERRY_DDLOG ("Execute RE_OP_CHAR_CLASS/RE_OP_INV_CHAR_CLASS, ");

        if (str_p >= re_ctx_p->input_end_p)
        {
          JERRY_DDLOG ("fail\n");
          re_ctx_p->recursion_depth--;
          return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
        }

        curr_ch = get_input_char (&str_p);

        q = get_value (&bc_p);
        match = false;
        while (q)
        {
          uint32_t ch1, ch2;
          ch1 = (uint32_t) get_value (&bc_p);
          ch2 = (uint32_t) get_value (&bc_p);
          JERRY_DDLOG ("q=%d, c1=%d, c2=%d, c3=%d; ", q, ch1, ch2, curr_ch);

          if (curr_ch >= ch1 && curr_ch <= ch2)
          {
            /* We must read all the ranges from bytecode. */
            match = true;
          }
          q--;
        }

        if (op == RE_OP_CHAR_CLASS)
        {
          if (!match)
          {
            JERRY_DDLOG ("fail\n");
            re_ctx_p->recursion_depth--;
            return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }
        else
        {
          JERRY_ASSERT (op == RE_OP_INV_CHAR_CLASS);
          if (match)
          {
            JERRY_DDLOG ("fail\n");
            re_ctx_p->recursion_depth--;
            return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
          }
        }
        JERRY_DDLOG ("match\n");
        break;
      }
      case RE_OP_BACKREFERENCE:
      {
        uint32_t backref_idx;
        const ecma_char_t *sub_str_p;

        backref_idx = get_value (&bc_p);
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
        break;
      }
      case RE_OP_SAVE_AT_START:
      {
        const ecma_char_t *old_start_p;
        re_bytecode_t *old_bc_p;

        JERRY_DDLOG ("Execute RE_OP_SAVE_AT_START\n");
        old_start_p = re_ctx_p->saved_p[RE_GLOBAL_START_IDX];
        re_ctx_p->saved_p[RE_GLOBAL_START_IDX] = str_p;
        do
        {
          uint32_t offset = get_value (&bc_p);
          const ecma_char_t *sub_str_p;
          ecma_completion_value_t match_value = regexp_match (re_ctx_p, bc_p, str_p, &sub_str_p);
          if (ecma_is_value_true (match_value))
          {
            *res = sub_str_p;
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
        while (get_opcode (&bc_p) == RE_OP_ALTERNATIVE);
        bc_p = old_bc_p;

        re_ctx_p->saved_p[RE_GLOBAL_START_IDX] = old_start_p;
        re_ctx_p->recursion_depth--;
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_SAVE_AND_MATCH:
      {
        JERRY_DDLOG ("End of pattern is reached: match\n");
        re_ctx_p->saved_p[RE_GLOBAL_END_IDX] = str_p;
        *res = str_p;
        re_ctx_p->recursion_depth--;
        return ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE); /* match */
      }
      case RE_OP_ALTERNATIVE:
      {
        /*
        *  Alternatives should be jump over, when alternative opcode appears.
        */
        uint32_t offset = get_value (&bc_p);
        JERRY_DDLOG ("Execute RE_OP_ALTERNATIVE");
        bc_p += offset;
        while (*bc_p == RE_OP_ALTERNATIVE)
        {
          JERRY_DDLOG (", jump: %d");
          bc_p++;
          offset = get_value (&bc_p);
          bc_p += offset;
        }
        JERRY_DDLOG ("\n");
        break;
      }
      case RE_OP_CAPTURE_NON_GREEDY_ZERO_GROUP_START:
      case RE_OP_NON_CAPTURE_NON_GREEDY_ZERO_GROUP_START:
      {
        /*
        *  On non-greedy iterations we have to execute the bytecode
        *  after the group first, if zero iteration is allowed.
        */
        uint32_t start_idx, iter_idx, offset;
        const ecma_char_t *old_start_p;
        const ecma_char_t *sub_str_p;
        re_bytecode_t *old_bc_p;

        old_bc_p = bc_p; /* save the bytecode start position of the group start */
        start_idx = get_value (&bc_p);
        offset = get_value (&bc_p);

        if (IS_CAPTURE_GROUP (op))
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
        ecma_completion_value_t match_value = regexp_match (re_ctx_p, bc_p, str_p, &sub_str_p);
        if (ecma_is_value_true (match_value))
        {
          *res = sub_str_p;
          re_ctx_p->recursion_depth--;
          return match_value; /* match */
        }
        else if (ecma_is_completion_value_throw (match_value))
        {
          return match_value;
        }
        if (IS_CAPTURE_GROUP (op))
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
        const ecma_char_t *old_start_p;
        const ecma_char_t *sub_str_p;
        re_bytecode_t *old_bc_p;
        re_bytecode_t *end_bc_p = NULL;

        start_idx = get_value (&bc_p);
        if (op != RE_OP_CAPTURE_GROUP_START
            && op != RE_OP_NON_CAPTURE_GROUP_START)
        {
          offset = get_value (&bc_p);
          end_bc_p = bc_p + offset;
        }

        if (IS_CAPTURE_GROUP (op))
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
          offset = get_value (&bc_p);
          ecma_completion_value_t match_value = regexp_match (re_ctx_p, bc_p, str_p, &sub_str_p);
          if (ecma_is_value_true (match_value))
          {
            *res = sub_str_p;
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
        while (get_opcode (&bc_p) == RE_OP_ALTERNATIVE);
        bc_p = old_bc_p;
        re_ctx_p->num_of_iterations[iter_idx] = old_iteration_cnt;

        /* Try to match after the close paren if zero is allowed. */
        if (op == RE_OP_CAPTURE_GREEDY_ZERO_GROUP_START
            || op == RE_OP_NON_CAPTURE_GREEDY_ZERO_GROUP_START)
        {
          JERRY_ASSERT (end_bc_p);
          ecma_completion_value_t match_value = regexp_match (re_ctx_p, end_bc_p, str_p, &sub_str_p);
          if (ecma_is_value_true (match_value))
          {
            *res = sub_str_p;
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
        const ecma_char_t *old_end_p;
        re_bytecode_t *old_bc_p;

        /*
        *  On non-greedy iterations we have to execute the bytecode
        *  after the group first. Try to iterate only if it fails.
        */
        old_bc_p = bc_p; /* save the bytecode start position of the group end */
        end_idx = get_value (&bc_p);
        min = get_value (&bc_p);
        max = get_value (&bc_p);
        get_value (&bc_p); /* start offset */

        if (IS_CAPTURE_GROUP (op))
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

          const ecma_char_t *sub_str_p;
          ecma_completion_value_t match_value = regexp_match (re_ctx_p, bc_p, str_p, &sub_str_p);
          if (ecma_is_value_true (match_value))
          {
            *res = sub_str_p;
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
        const ecma_char_t *old_start_p;
        const ecma_char_t *old_end_p;
        const ecma_char_t *sub_str_p;
        re_bytecode_t *old_bc_p;

        end_idx = get_value (&bc_p);
        min = get_value (&bc_p);
        max = get_value (&bc_p);
        offset = get_value (&bc_p);

        if (IS_CAPTURE_GROUP (op))
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
          offset = get_value (&bc_p);

          old_start_p = re_ctx_p->saved_p[start_idx];
          re_ctx_p->saved_p[start_idx] = str_p;
          ecma_completion_value_t match_value = regexp_match (re_ctx_p, bc_p, str_p, &sub_str_p);
          if (ecma_is_value_true (match_value))
          {
            *res = sub_str_p;
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
            offset = get_value (&bc_p);

            old_start_p = re_ctx_p->saved_p[start_idx];
            re_ctx_p->saved_p[start_idx] = str_p;

            ecma_completion_value_t match_value = regexp_match (re_ctx_p, bc_p, str_p, &sub_str_p);
            if (ecma_is_value_true (match_value))
            {
              *res = sub_str_p;
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
          ecma_completion_value_t match_value = regexp_match (re_ctx_p, old_bc_p, str_p, &sub_str_p);
          if (ecma_is_value_true (match_value))
          {
            *res = sub_str_p;
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
        uint32_t min, max, offset, q;
        const ecma_char_t *sub_str_p;

        min = get_value (&bc_p);
        max = get_value (&bc_p);

        offset = get_value (&bc_p);
        JERRY_DDLOG ("Non-greedy iterator, min=%lu, max=%lu, offset=%ld\n",
                     (unsigned long) min, (unsigned long) max, (long) offset);

        q = 0;
        while (q <= max)
        {
          if (q >= min)
          {
            ecma_completion_value_t match_value = regexp_match (re_ctx_p, bc_p + offset, str_p, &sub_str_p);
            if (ecma_is_value_true (match_value))
            {
              *res = sub_str_p;
              re_ctx_p->recursion_depth--;
              return match_value; /* match */
            }
            else if (ecma_is_completion_value_throw (match_value))
            {
              return match_value;
            }
          }

          ecma_completion_value_t match_value = regexp_match (re_ctx_p, bc_p, str_p, &sub_str_p);
          if (!ecma_is_value_true (match_value))
          {
            break;
          }
          else if (ecma_is_completion_value_throw (match_value))
          {
            return match_value;
          }
          str_p = sub_str_p;
          q++;
        }
        re_ctx_p->recursion_depth--;
        return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE); /* fail */
      }
      case RE_OP_GREEDY_ITERATOR:
      {
        uint32_t min, max, offset, q;
        const ecma_char_t *sub_str_p;

        min = get_value (&bc_p);
        max = get_value (&bc_p);

        offset = get_value (&bc_p);
        JERRY_DDLOG ("Greedy iterator, min=%lu, max=%lu, offset=%ld\n",
                     (unsigned long) min, (unsigned long) max, (long) offset);

        q = 0;
        while (q < max)
        {
          ecma_completion_value_t match_value = regexp_match (re_ctx_p, bc_p, str_p, &sub_str_p);
          if (!ecma_is_value_true (match_value))
          {
            break;
          }
          else if (ecma_is_completion_value_throw (match_value))
          {
            return match_value;
          }
          str_p = sub_str_p;
          q++;
        }

        while (q >= min)
        {
          ecma_completion_value_t match_value = regexp_match (re_ctx_p, bc_p + offset, str_p, &sub_str_p);
          if (ecma_is_value_true (match_value))
          {
            *res = sub_str_p;
            re_ctx_p->recursion_depth--;
            return match_value; /* match */
          }
          else if (ecma_is_completion_value_throw (match_value))
          {
            return match_value;
          }
          if (q == min)
          {
            break;
          }

          str_p = utf8_backtrack (str_p);
          q--;
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
 * RegExp helper function
 */
ecma_completion_value_t
ecma_regexp_exec_helper (re_bytecode_t *bc_p, /**< start of the RegExp bytecode */
                         const ecma_char_t *str_p) /**< start of the input string */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  re_matcher_ctx re_ctx;
  re_ctx.input_start_p = str_p;
  re_ctx.input_end_p = str_p + strlen ((char *) str_p);
  re_ctx.steps_count = 0;
  re_ctx.recursion_depth = 0;

  /* 1. Read bytecode header and init regexp matcher context. */
  re_ctx.flags = (uint8_t) get_value (&bc_p);
  re_ctx.num_of_captures = get_value (&bc_p);
  JERRY_ASSERT (re_ctx.num_of_captures % 2 == 0);
  re_ctx.num_of_non_captures = get_value (&bc_p);

  MEM_DEFINE_LOCAL_ARRAY (saved_p, re_ctx.num_of_captures + re_ctx.num_of_non_captures, const ecma_char_t*);
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

  bool match = false;
  re_ctx.num_of_iterations = num_of_iter_p;

  /* 2. Try to match */
  while (str_p && str_p <= re_ctx.input_end_p && ecma_is_completion_value_empty (ret_value))
  {
    const ecma_char_t *sub_str_p;
    ECMA_TRY_CATCH (match_value, regexp_match (&re_ctx, bc_p, str_p, &sub_str_p), ret_value);
    if (ecma_is_value_true (match_value))
    {
      match = true;
      break;
    }
    str_p++;
    ECMA_FINALIZE (match_value);
  }

  /* 3. Fill the result array or return with 'undefiend' */
  if (ecma_is_completion_value_empty (ret_value))
  {
    if (match)
    {
      ecma_completion_value_t new_array = ecma_op_create_array_object (0, 0, false);
      ecma_object_t *new_array_p = ecma_get_object_from_completion_value (new_array);

      for (uint32_t i = 0; i < re_ctx.num_of_captures; i += 2)
      {
        ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (i / 2);
        if (re_ctx.saved_p[i] && re_ctx.saved_p[i + 1] && re_ctx.saved_p[i + 1] >= re_ctx.saved_p[i])
        {
          ecma_length_t capture_str_len = static_cast<ecma_length_t> (re_ctx.saved_p[i + 1] - re_ctx.saved_p[i]);
          ecma_string_t *capture_str_p;
          if (capture_str_len > 0)
          {
            capture_str_p = ecma_new_ecma_string (re_ctx.saved_p[i], capture_str_len);
          }
          else
          {
            capture_str_p = ecma_get_magic_string (ECMA_MAGIC_STRING__EMPTY);
          }
          ecma_op_object_put (new_array_p, index_str_p, ecma_make_string_value (capture_str_p), true);
          ecma_deref_ecma_string (capture_str_p);
        }
        else
        {
          ecma_op_object_put (new_array_p, index_str_p, ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED), true);
        }
        ecma_deref_ecma_string (index_str_p);
      }
      ret_value = new_array;
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
