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
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-objects.h"
#include "ecma-regexp-object.h"
#include "re-compiler.h"
#include "stdio.h"

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

/**
 * RegExp object creation operation.
 *
 * See also: ECMA-262 v5, 15.10.4.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_create_regexp_object (ecma_string_t *pattern, /**< input pattern */
                              ecma_string_t *flags __attr_unused___) /**< flags */
{
  JERRY_ASSERT (pattern != NULL);

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
                                      ecma_make_string_value (ecma_copy_or_ref_ecma_string (pattern)));

  /* Set global property. ECMA-262 v5, 15.10.7.2*/
  magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_GLOBAL);
  ecma_property_t *global_prop_p = ecma_create_named_data_property (obj_p,
                                                                    magic_string_p,
                                                                    false, false, false);
  ecma_deref_ecma_string (magic_string_p);
  ecma_set_named_data_property_value (global_prop_p, ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE));

  /* Set ignoreCase property. ECMA-262 v5, 15.10.7.3*/
  magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_IGNORECASE);
  ecma_property_t *ignorecase_prop_p = ecma_create_named_data_property (obj_p,
                                                                        magic_string_p,
                                                                        false, false, false);
  ecma_deref_ecma_string (magic_string_p);
  ecma_set_named_data_property_value (ignorecase_prop_p, ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE));

  /* Set multiline property. ECMA-262 v5, 15.10.7.4*/
  magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_MULTILINE);
  ecma_property_t *multiline_prop_p = ecma_create_named_data_property (obj_p,
                                                                       magic_string_p,
                                                                       false, false, false);
  ecma_deref_ecma_string (magic_string_p);
  ecma_set_named_data_property_value (multiline_prop_p, ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE));

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
  regexp_compile_bytecode (bytecode, pattern);

  return ecma_make_normal_completion_value (ecma_make_object_value (obj_p));
} /* ecma_op_create_regexp_object */

static const ecma_char_t*
regexp_match (re_matcher_ctx *re_ctx __attr_unused___,
              regexp_bytecode_t *bc_p,
              const ecma_char_t *str_p)
{
  regexp_opcode_t op;

  while ((op = get_opcode (&bc_p)))
  {
    switch (op)
    {
      case RE_OP_MATCH:
      {
        fprintf (stderr, "RegExp match\n");
        return str_p;
      }
      case RE_OP_CHAR:
      {
        uint32_t ch1 = get_value (&bc_p);
        uint32_t ch2 = (uint32_t) *str_p;
        str_p++;
        fprintf (stderr, "Character matching %d to %d\n", ch1, ch2);

        if (ch1 != ch2)
        {
          return NULL;
        }
        break;
      }
      case RE_OP_SAVE_AT_START:
      {
        const ecma_char_t *old_start;
        const ecma_char_t *sub_str_p;

        old_start = re_ctx->saved_p[RE_GLOBAL_START_IDX];
        re_ctx->saved_p[RE_GLOBAL_START_IDX] = str_p;

        sub_str_p = regexp_match (re_ctx, bc_p, str_p);
        if (sub_str_p)
        {
          return sub_str_p;
        }

        re_ctx->saved_p[RE_GLOBAL_START_IDX] = old_start;
        return NULL;
      }
      case RE_OP_SAVE_AND_MATCH:
      {
        re_ctx->saved_p[RE_GLOBAL_END_IDX] = str_p;
        return str_p;
      }
      default:
      {
        fprintf (stderr, "UNKNOWN opcode!\n");
        // FIXME: throw an internal error
        return NULL;
      }
    }
  }

  // FIXME: throw an internal error
  fprintf (stderr, "Should not get here!\n");
  return NULL;
} /* ecma_regexp_match */

ecma_completion_value_t
ecma_regexp_exec_helper (regexp_bytecode_t *bc_p, const ecma_char_t *str_p)
{
  re_matcher_ctx re_ctx;
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1. Read bytecode header and init regexp matcher context. */
  re_ctx.flags = (uint8_t) get_value (&bc_p);
  re_ctx.num_of_captures = get_value (&bc_p);
  JERRY_ASSERT (re_ctx.num_of_captures % 2 == 0);
  re_ctx.num_of_non_captures = get_value (&bc_p);

  MEM_DEFINE_LOCAL_ARRAY (saved_p, re_ctx.num_of_captures, const ecma_char_t*);
  bool match = 0;
  re_ctx.saved_p = saved_p;

  /* 2. Try to match */
  while (str_p && *str_p != '\0')
  {
    if (regexp_match (&re_ctx, bc_p, str_p) != NULL)
    {
      match = 1;
      break;
    }
    str_p++;
  }

  /* 3. Fill the result array or return with 'undefiend' */
  if (match)
  {
    ecma_completion_value_t new_array = ecma_op_create_array_object (0, 0, false);
    ecma_object_t *new_array_p = ecma_get_object_from_completion_value (new_array);

    uint32_t length = re_ctx.num_of_captures / 2;
    for (uint32_t i = 0; i < length; i++)
    {
      ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (i);

      ecma_length_t capture_str_len = static_cast<ecma_length_t> (re_ctx.saved_p[i+1] - re_ctx.saved_p[i]);
      ecma_string_t *capture_str_p = ecma_new_ecma_string (re_ctx.saved_p[i], capture_str_len);

      ecma_op_object_put (new_array_p, index_str_p, ecma_make_string_value (capture_str_p), true);
      ecma_deref_ecma_string (index_str_p);
      ecma_deref_ecma_string (capture_str_p);
    }
    ret_value = new_array;
  }
  else
  {
    ret_value = ecma_make_normal_completion_value (ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED));
  }

  MEM_FINALIZE_LOCAL_ARRAY (saved_p);

  return ret_value;
} /* ecma_regexp_exec_helper */

/**
 * @}
 * @}
 */
