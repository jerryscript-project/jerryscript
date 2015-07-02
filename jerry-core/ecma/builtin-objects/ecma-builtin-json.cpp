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
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-builtin-helpers.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"
#include "jrt-libc-includes.h"
#include "lit-char-helpers.h"

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_JSON_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-json.inc.h"
#define BUILTIN_UNDERSCORED_ID json
#include "ecma-builtin-internal-routines-template.inc.h"

/*
 * FIXME:
 *       Replace usage of isalpha and isdigit functions in the module with lit_char helpers and remove the functions.
 *
 *       Related issue: #424
 */

/** Checks for an alphabetic character. */
static int
isalpha (int c)
{
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

/** Checks for a digit (0 through 9).  */
static int
isdigit (int c)
{
  return c >= '0' && c <= '9';
}

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup json ECMA JSON object built-in
 * @{
 */

/**
 * JSON token type
 */
typedef enum
{
  invalid_token, /**< error token */
  end_token, /**< end of stream reached */
  number_token, /**< JSON number */
  string_token, /**< JSON string */
  null_token, /**< JSON null primitive value */
  true_token, /**< JSON true primitive value */
  false_token, /**< JSON false primitive value */
  left_brace_token, /**< JSON left brace */
  right_brace_token, /**< JSON right brace */
  left_square_token, /**< JSON left square bracket */
  comma_token, /**< JSON comma */
  colon_token /**< JSON colon */
} ecma_json_token_type_t;

/**
 * JSON token
 */
typedef struct
{
  ecma_json_token_type_t type; /**< type of the current token */
  lit_utf8_byte_t *current_p; /**< current position of the string processed by the parser */
  lit_utf8_byte_t *end_p; /**< end of the string processed by the parser */
  union
  {
    struct
    {
      lit_utf8_byte_t *start_p; /**< when type is string_token, it contains the start of the string */
      lit_utf8_size_t size; /**< when type is string_token, it contains the size of the string */
    } string;
    ecma_number_t number; /**< when type is number_token, it contains the value of the number */
  } u;
} ecma_json_token_t;

/**
 * Compare the string with an ID.
 *
 * @return true if the match is successful
 */
static bool
ecma_builtin_json_check_id (lit_utf8_byte_t *string_p, /**< start position */
                            const char *id_p) /**< string identifier */
{
  /*
   * String comparison must not depend on lit_utf8_byte_t definition.
   */
  JERRY_ASSERT (*string_p == *id_p);

  do
  {
    string_p++;
    id_p++;
    if (*id_p == '\0')
    {
      return !isalpha (*string_p) && !isdigit (*string_p) && *string_p != '$' && *string_p != '_';
    }
  }
  while (*string_p == *id_p);

  return false;
} /* ecma_builtin_json_check_id */

/**
 * Parse and extract string token.
 */
static void
ecma_builtin_json_parse_string (ecma_json_token_t *token_p) /**< token argument */
{
  lit_utf8_byte_t *current_p = token_p->current_p;
  lit_utf8_byte_t *write_p = current_p;

  token_p->u.string.start_p = current_p;

  while (*current_p != '"')
  {
    if (*current_p <= 0x1f)
    {
      return;
    }
    if (*current_p == '\\')
    {
      current_p++;
      switch (*current_p)
      {
        case '"':
        case '/':
        case '\\':
        {
          break;
        }
        case 'b':
        {
          *current_p = '\b';
          break;
        }
        case 'f':
        {
          *current_p = '\f';
          break;
        }
        case 'n':
        {
          *current_p = '\n';
          break;
        }
        case 'r':
        {
          *current_p = '\r';
          break;
        }
        case 't':
        {
          *current_p = '\t';
          break;
        }
        case 'u':
        {
          lit_code_point_t code_point;

          if (!(lit_read_code_point_from_hex (current_p + 1, 4, &code_point)))
          {
            return;
          }

          current_p += 5;
          write_p += lit_code_point_to_utf8 (code_point, write_p);
          continue;
          /* FALLTHRU */
        }
        default:
        {
          return;
        }
      }
    }
    *write_p++ = *current_p++;
  }

  /*
   * Post processing surrogate pairs.
   *
   * The general issue is, that surrogate fragments can come from
   * the original stream and can be constructed by \u sequences
   * as well. We need to construct code points from them.
   *
   * Example: JSON.parse ('"\\ud801\udc00"') === "\ud801\udc00"
   *          The first \u is parsed by JSON, the second is by the lexer.
   *
   * The rewrite happens in-place, since the write pointer is always
   * precede the read-pointer. We also cannot create an UTF8 iterator,
   * because the lit_is_utf8_string_valid assertion may fail.
   */

  lit_utf8_byte_t *read_p = token_p->u.string.start_p;
  lit_utf8_byte_t *read_end_p = write_p;
  write_p = read_p;

  while (read_p < read_end_p)
  {
    lit_code_point_t code_point;
    read_p += lit_read_code_point_from_utf8 (read_p,
                                             (lit_utf8_size_t) (read_end_p - read_p),
                                             &code_point);

    /* The lit_is_code_unit_high_surrogate expects ecma_char_t argument
       so code_points above maximum UTF16 code unit must not be tested. */
    if (read_p < read_end_p
        && code_point <= LIT_UTF16_CODE_UNIT_MAX
        && lit_is_code_unit_high_surrogate ((ecma_char_t) code_point))
    {
      lit_code_point_t next_code_point;
      lit_utf8_size_t next_code_point_size = lit_read_code_point_from_utf8 (read_p,
                                                                            (lit_utf8_size_t) (read_end_p - read_p),
                                                                            &next_code_point);

      if (next_code_point <= LIT_UTF16_CODE_UNIT_MAX
          && lit_is_code_unit_low_surrogate ((ecma_char_t) next_code_point))
      {
        code_point = lit_convert_surrogate_pair_to_code_point ((ecma_char_t) code_point,
                                                               (ecma_char_t) next_code_point);
        read_p += next_code_point_size;
      }
    }
    write_p += lit_code_point_to_utf8 (code_point, write_p);
  }

  JERRY_ASSERT (lit_is_utf8_string_valid (token_p->u.string.start_p,
                                          (lit_utf8_size_t) (write_p - token_p->u.string.start_p)));

  token_p->u.string.size = (lit_utf8_size_t) (write_p - token_p->u.string.start_p);
  token_p->current_p = current_p + 1;
  token_p->type = string_token;
} /* ecma_builtin_json_parse_string */

/**
 * Parse and extract string token.
 */
static void
ecma_builtin_json_parse_number (ecma_json_token_t *token_p) /**< token argument */
{
  lit_utf8_byte_t *current_p = token_p->current_p;
  lit_utf8_byte_t *start_p = current_p;

  if (*current_p == '-')
  {
    current_p++;
  }

  if (*current_p == '0')
  {
    current_p++;
    if (isdigit (*current_p))
    {
      return;
    }
  }
  else if (isdigit (*current_p))
  {
    do
    {
      current_p++;
    }
    while (isdigit (*current_p));
  }

  if (*current_p == '.')
  {
    current_p++;
    if (!isdigit (*current_p))
    {
      return;
    }

    do
    {
      current_p++;
    }
    while (isdigit (*current_p));
  }

  if (*current_p == 'e' || *current_p == 'E')
  {
    current_p++;
    if (*current_p == '+' || *current_p == '-')
    {
      current_p++;
    }

    if (!isdigit (*current_p))
    {
      return;
    }

    do
    {
      current_p++;
    }
    while (isdigit (*current_p));
  }
  token_p->type = number_token;
  token_p->u.number = ecma_utf8_string_to_number (start_p, (lit_utf8_size_t) (current_p - start_p));

  token_p->current_p = current_p;
} /* ecma_builtin_json_parse_number */

/**
 * Parse next token.
 *
 * The function fills the fields of the ecma_json_token_t
 * argument and advances the string pointer.
 */
static void
ecma_builtin_json_parse_next_token (ecma_json_token_t *token_p) /**< token argument */
{
  lit_utf8_byte_t *current_p = token_p->current_p;
  token_p->type = invalid_token;

  /*
   * No need for end check since the string is zero terminated.
   */
  while (*current_p == ' ' || *current_p == '\r'
         || *current_p == '\n' || *current_p == '\t')
  {
    current_p++;
  }

  if (current_p == token_p->end_p)
  {
    token_p->type = end_token;
    return;
  }

  switch (*current_p)
  {
    case '{':
    {
      token_p->type = left_brace_token;
      token_p->current_p = current_p + 1;
      return;
    }
    case '}':
    {
      token_p->type = right_brace_token;
      token_p->current_p = current_p + 1;
      return;
    }
    case '[':
    {
      token_p->type = left_square_token;
      token_p->current_p = current_p + 1;
      return;
    }
    case ',':
    {
      token_p->type = comma_token;
      token_p->current_p = current_p + 1;
      return;
    }
    case ':':
    {
      token_p->type = colon_token;
      token_p->current_p = current_p + 1;
      return;
    }
    case '"':
    {
      token_p->current_p = current_p + 1;
      ecma_builtin_json_parse_string (token_p);
      return;
    }
    case 'n':
    {
      if (ecma_builtin_json_check_id (current_p, "null"))
      {
        token_p->type = null_token;
        token_p->current_p = current_p + 4;
        return;
      }
      break;
    }
    case 't':
    {
      if (ecma_builtin_json_check_id (current_p, "true"))
      {
        token_p->type = true_token;
        token_p->current_p = current_p + 4;
        return;
      }
      break;
    }
    case 'f':
    {
      if (ecma_builtin_json_check_id (current_p, "false"))
      {
        token_p->type = false_token;
        token_p->current_p = current_p + 5;
        return;
      }
      break;
    }
    default:
    {
      if (*current_p == '-' || isdigit (*current_p))
      {
        token_p->current_p = current_p;
        ecma_builtin_json_parse_number (token_p);
        return;
      }
      break;
    }
  }
} /* ecma_builtin_json_parse_next_token */

/**
 * Checks whether the next token is right square token.
 *
 * @return true if it is.
 *
 * Always skips white spaces regardless the next token.
 */
static bool
ecma_builtin_json_check_right_square_token (ecma_json_token_t *token_p) /**< token argument */
{
  lit_utf8_byte_t *current_p = token_p->current_p;

  /*
   * No need for end check since the string is zero terminated.
   */
  while (*current_p == ' ' || *current_p == '\r'
         || *current_p == '\n' || *current_p == '\t')
  {
    current_p++;
  }

  token_p->current_p = current_p;

  if (*current_p == ']')
  {
    token_p->current_p = current_p + 1;
    return true;
  }
  return false;
} /* ecma_builtin_json_check_right_square_token */

/**
 * Utility for defining properties.
 *
 * It silently ignores all errors.
 */
static void
ecma_builtin_json_define_value_property (ecma_object_t *obj_p, /**< this object */
                                         ecma_string_t *property_name_p, /**< property name */
                                         ecma_value_t value) /**< value */
{
  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();
  {
    prop_desc.is_value_defined = true;
    prop_desc.value = value;

    prop_desc.is_writable_defined = true;
    prop_desc.is_writable = true;

    prop_desc.is_enumerable_defined = true;
    prop_desc.is_enumerable = true;

    prop_desc.is_configurable_defined = true;
    prop_desc.is_configurable = true;
  }

  ecma_completion_value_t completion_value = ecma_op_object_define_own_property (obj_p,
                                                                                 property_name_p,
                                                                                 &prop_desc,
                                                                                 false);
  JERRY_ASSERT (ecma_is_completion_value_normal_true (completion_value)
                || ecma_is_completion_value_normal_false (completion_value));
} /* ecma_builtin_json_define_value_property */

/**
 * Parse next value.
 *
 * The function fills the fields of the ecma_json_token_t
 * argument and advances the string pointer.
 *
 * @return ecma_value with the property value
 */
static ecma_value_t
ecma_builtin_json_parse_value (ecma_json_token_t *token_p) /**< token argument */
{
  ecma_builtin_json_parse_next_token (token_p);

  switch (token_p->type)
  {
    case number_token:
    {
      ecma_number_t *number_p = ecma_alloc_number ();
      *number_p = token_p->u.number;
      return ecma_make_number_value (number_p);
    }
    case string_token:
    {
      ecma_string_t *string_p = ecma_new_ecma_string_from_utf8 (token_p->u.string.start_p, token_p->u.string.size);
      return ecma_make_string_value (string_p);
    }
    case null_token:
    {
      return ecma_make_simple_value (ECMA_SIMPLE_VALUE_NULL);
    }
    case true_token:
    {
      return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
    }
    case false_token:
    {
      return ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
    }
    case left_brace_token:
    {
      bool parse_comma = false;
      ecma_object_t *object_p = ecma_op_create_object_object_noarg ();

      while (true)
      {
        ecma_builtin_json_parse_next_token (token_p);

        if (token_p->type == right_brace_token)
        {
          return ecma_make_object_value (object_p);
        }

        if (parse_comma)
        {
          if (token_p->type != comma_token)
          {
            break;
          }
          ecma_builtin_json_parse_next_token (token_p);
        }

        if (token_p->type != string_token)
        {
          break;
        }

        lit_utf8_byte_t *string_start_p = token_p->u.string.start_p;
        lit_utf8_size_t string_size = token_p->u.string.size;
        ecma_builtin_json_parse_next_token (token_p);

        if (token_p->type != colon_token)
        {
          break;
        }

        ecma_value_t value = ecma_builtin_json_parse_value (token_p);

        if (ecma_is_value_undefined (value))
        {
          break;
        }

        ecma_string_t *name_p = ecma_new_ecma_string_from_utf8 (string_start_p, string_size);
        ecma_builtin_json_define_value_property (object_p, name_p, value);
        ecma_deref_ecma_string (name_p);
        ecma_free_value (value, true);
        parse_comma = true;
      }

      /*
       * Parse error occured.
       */
      ecma_deref_object (object_p);
      return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
    case left_square_token:
    {
      bool parse_comma = false;
      uint32_t length = 0;

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ARRAY_BUILTIN
      ecma_object_t *array_prototype_p = ecma_builtin_get (ECMA_BUILTIN_ID_ARRAY_PROTOTYPE);
#else /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ARRAY_BUILTIN */
      ecma_object_t *array_prototype_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);
#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ARRAY_BUILTIN */

      ecma_object_t *array_p = ecma_create_object (array_prototype_p, true, ECMA_OBJECT_TYPE_ARRAY);
      ecma_deref_object (array_prototype_p);

      ecma_property_t *class_prop_p = ecma_create_internal_property (array_p, ECMA_INTERNAL_PROPERTY_CLASS);
      class_prop_p->u.internal_property.value = LIT_MAGIC_STRING_ARRAY_UL;

      while (true)
      {
        if (ecma_builtin_json_check_right_square_token (token_p))
        {
          ecma_string_t *length_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);
          ecma_number_t *length_num_p = ecma_alloc_number ();
          *length_num_p = ecma_uint32_to_number (length);

          ecma_property_t *length_prop_p = ecma_create_named_data_property (array_p,
                                                                            length_magic_string_p,
                                                                            true,
                                                                            false,
                                                                            false);
          ecma_set_named_data_property_value (length_prop_p, ecma_make_number_value (length_num_p));
          ecma_deref_ecma_string (length_magic_string_p);

          return ecma_make_object_value (array_p);
        }

        if (parse_comma)
        {
          ecma_builtin_json_parse_next_token (token_p);
          if (token_p->type != comma_token)
          {
            break;
          }
        }

        ecma_value_t value = ecma_builtin_json_parse_value (token_p);

        if (ecma_is_value_undefined (value))
        {
          break;
        }

        ecma_string_t *name_p = ecma_new_ecma_string_from_uint32 (length);
        ecma_property_t *property_p = ecma_create_named_data_property (array_p, name_p,
                                                                       true, true, true);
        ecma_deref_ecma_string (name_p);

        ecma_named_data_property_assign_value (array_p, property_p, value);
        ecma_free_value (value, true);
        length++;
        parse_comma = true;
      }

      /*
       * Parse error occured.
       */
      ecma_deref_object (array_p);
      return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
    default:
    {
      return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
  }
} /* ecma_builtin_json_parse_value */

/**
 * Abstract operation Walk defined in 15.12.2
 *
 * See also:
 *          ECMA-262 v5, 15.12.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_json_walk (ecma_object_t *reviver_p, /**< reviver function */
                        ecma_object_t *holder_p, /**< holder object */
                        ecma_string_t *name_p) /** < property name */
{
  JERRY_ASSERT (reviver_p);
  JERRY_ASSERT (holder_p);
  JERRY_ASSERT (name_p);

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (value_get,
                  ecma_op_object_get (holder_p, name_p),
                  ret_value);

  if (ecma_is_value_object (value_get))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (value_get);

    uint32_t no_properties = 0;

    /*
     * The following algorithm works with arrays and objects as well.
     */
    for (ecma_property_t *property_p = ecma_get_property_list (object_p);
         property_p != NULL;
         property_p = ECMA_GET_POINTER (ecma_property_t, property_p->next_property_p))
    {
      /*
       * All properties must be named data or internal properties, since we constructed them.
       */
      JERRY_ASSERT (property_p->type == ECMA_PROPERTY_NAMEDDATA
                    || property_p->type == ECMA_PROPERTY_INTERNAL);

      if (property_p->type == ECMA_PROPERTY_NAMEDDATA
          && ecma_is_property_enumerable (property_p))
      {
        no_properties++;
      }
    }

    if (no_properties > 0)
    {
      MEM_DEFINE_LOCAL_ARRAY (property_names_p, no_properties, ecma_string_t *);

      uint32_t property_index = no_properties;
      for (ecma_property_t *property_p = ecma_get_property_list (object_p);
           property_p != NULL;
           property_p = ECMA_GET_POINTER (ecma_property_t, property_p->next_property_p))
      {
        if (property_p->type == ECMA_PROPERTY_NAMEDDATA
            && ecma_is_property_enumerable (property_p))
        {
          ecma_string_t *property_name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                                      property_p->u.named_data_property.name_p);
         /*
          * The property list must be in creation order.
          */
          property_index--;
          property_names_p[property_index] = ecma_copy_or_ref_ecma_string (property_name_p);
        }
      }

      JERRY_ASSERT (property_index == 0);

      for (property_index = 0;
           property_index < no_properties && ecma_is_completion_value_empty (ret_value);
           property_index++)
      {
        ECMA_TRY_CATCH (value_walk,
                        ecma_builtin_json_walk (reviver_p,
                                                object_p,
                                                property_names_p[property_index]),
                        ret_value);

        /*
         * We cannot optimize this function since any members
         * can be changed (deleted) by the reviver function.
         */
        if (ecma_is_value_undefined (value_walk))
        {
          ecma_completion_value_t delete_val = ecma_op_general_object_delete (object_p,
                                                                              property_names_p[property_index],
                                                                              false);
          JERRY_ASSERT (ecma_is_completion_value_normal_true (delete_val)
                        || ecma_is_completion_value_normal_false (delete_val));
        }
        else
        {
          ecma_builtin_json_define_value_property (object_p,
                                                   property_names_p[property_index],
                                                   value_walk);
        }

        ECMA_FINALIZE (value_walk);
      }

      for (uint32_t i = 0; i < no_properties; i++)
      {
        ecma_deref_ecma_string (property_names_p[i]);
      }

      MEM_FINALIZE_LOCAL_ARRAY (property_names_p);
    }
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    ecma_value_t arguments_list[2];

    arguments_list[0] = ecma_make_string_value (name_p);
    arguments_list[1] = value_get;

   /*
    * The completion value can be anything including exceptions.
    */
    ret_value = ecma_op_function_call (reviver_p,
                                       ecma_make_object_value (holder_p),
                                       arguments_list,
                                       2);
  }
  else
  {
    JERRY_ASSERT (ecma_is_completion_value_throw (ret_value));
  }

  ECMA_FINALIZE (value_get);

  return ret_value;
} /* ecma_builtin_json_walk */

/**
 * The JSON object's 'parse' routine
 *
 * See also:
 *          ECMA-262 v5, 15.12.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_json_parse (ecma_value_t this_arg __attr_unused___, /**< 'this' argument */
                         ecma_value_t arg1, /**< string argument */
                         ecma_value_t arg2) /**< reviver argument */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ECMA_TRY_CATCH (string,
                  ecma_op_to_string (arg1),
                  ret_value);

  ecma_string_t *string_p = ecma_get_string_from_value (string);
  ecma_length_t string_size = (uint32_t) ecma_string_get_size (string_p);
  size_t buffer_size = sizeof (lit_utf8_byte_t) * (string_size + 1);

  MEM_DEFINE_LOCAL_ARRAY (str_start_p, buffer_size, lit_utf8_byte_t);

  ecma_string_to_utf8_string (string_p, str_start_p, (ssize_t) buffer_size);
  str_start_p[string_size] = LIT_BYTE_NULL;

  ecma_json_token_t token;
  token.current_p = str_start_p;
  token.end_p = str_start_p + string_size;

  ecma_value_t final_result = ecma_builtin_json_parse_value (&token);

  if (!ecma_is_value_undefined (final_result))
  {
    ecma_builtin_json_parse_next_token (&token);

    if (token.type != end_token)
    {
      ecma_free_value (final_result, true);
      final_result = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
  }

  if (ecma_is_value_undefined (final_result))
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_SYNTAX));
  }
  else
  {
    if (ecma_op_is_callable (arg2))
    {
      ecma_object_t *object_p = ecma_op_create_object_object_noarg ();
      ecma_string_t *name_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
      ecma_property_t *property_p = ecma_create_named_data_property (object_p,
                                                                     name_p,
                                                                     true,
                                                                     true,
                                                                     true);

      ecma_named_data_property_assign_value (object_p, property_p, final_result);
      ecma_free_value (final_result, true);

      ret_value = ecma_builtin_json_walk (ecma_get_object_from_value (arg2),
                                          object_p,
                                          name_p);
      ecma_deref_object (object_p);
      ecma_deref_ecma_string (name_p);
    }
    else
    {
      ret_value = ecma_make_normal_completion_value (final_result);
    }
  }

  MEM_FINALIZE_LOCAL_ARRAY (str_start_p);

  ECMA_FINALIZE (string);
  return ret_value;
} /* ecma_builtin_json_parse */

static ecma_completion_value_t
ecma_builtin_json_str (ecma_string_t *key_p, ecma_object_t *holder_p, stringify_context_t *context_p);

static ecma_completion_value_t
ecma_builtin_json_object (ecma_object_t *obj_p, stringify_context_t *context_p);

static ecma_completion_value_t
ecma_builtin_json_array (ecma_object_t *obj_p, stringify_context_t *context_p);

/**
 * The JSON object's 'stringify' routine
 *
 * See also:
 *          ECMA-262 v5, 15.12.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_json_stringify (ecma_value_t this_arg __attr_unused___, /**< 'this' argument */
                             ecma_value_t arg1,  /**< value */
                             ecma_value_t arg2,  /**< replacer */
                             ecma_value_t arg3)  /**< space */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  stringify_context_t context_p;

  /* 1. */
  context_p.occurence_stack.block_start_p = NULL;
  context_p.occurence_stack.block_end_p = NULL;
  context_p.occurence_stack.current_p = NULL;

  /* 2. */
  context_p.indent_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);

  /* 3. */
  context_p.property_list.block_start_p = NULL;
  context_p.property_list.block_end_p = NULL;
  context_p.property_list.current_p = NULL;

  context_p.replacer_function_p = NULL;

  /* 4. */
  if (ecma_is_value_object (arg2))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (arg2);

    /* 4.a */
    if (ecma_op_is_callable (arg2))
    {
      context_p.replacer_function_p = obj_p;
    }
    /* 4.b */
    else if (ecma_object_get_class_name (obj_p) == LIT_MAGIC_STRING_ARRAY_UL)
    {
      ecma_string_t *length_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);

      ECMA_TRY_CATCH (array_length,
                      ecma_op_object_get (obj_p, length_str_p),
                      ret_value);

      ECMA_OP_TO_NUMBER_TRY_CATCH (array_length_num,
                                   array_length,
                                   ret_value);

      uint32_t array_length = ecma_number_to_uint32 (array_length_num);
      uint32_t index = 0;

      /* 4.b.ii */
      while ((index < array_length) && ecma_is_completion_value_empty (ret_value))
      {
        ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (index);

        ECMA_TRY_CATCH (value,
                        ecma_op_object_get (obj_p, index_str_p),
                        ret_value);

        /* 4.b.ii.1 */
        ecma_value_t item = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);

        /* 4.b.ii.2 */
        if (ecma_is_value_string (value))
        {
          item = ecma_copy_value (value, true);
        }
        /* 4.b.ii.3 */
        else if (ecma_is_value_number (value))
        {
          ECMA_TRY_CATCH (str_val,
                          ecma_op_to_string (value),
                          ret_value);

          item = ecma_copy_value (str_val, true);

          ECMA_FINALIZE (str_val);
        }
        /* 4.b.ii.4 */
        else if (ecma_is_value_object (value))
        {
          ecma_object_t *obj_val_p = ecma_get_object_from_value (value);
          lit_magic_string_id_t class_name = ecma_object_get_class_name (obj_val_p);

          /* 4.b.ii.4.a */
          if (class_name == LIT_MAGIC_STRING_NUMBER_UL
              || class_name == LIT_MAGIC_STRING_STRING_UL)
          {
            ECMA_TRY_CATCH (val,
                            ecma_op_to_string (value),
                            ret_value);

            item = ecma_copy_value (val, true);

            ECMA_FINALIZE (val);
          }
        }

        /* 4.b.ii.5 */
        if (!ecma_is_value_undefined (item))
        {
          ecma_string_t *item_str_p = ecma_get_string_from_value (item);

          if (!list_has_ecma_string_element (&context_p.property_list, item_str_p))
          {
            list_append (&context_p.property_list, item_str_p);
          }
          else
          {
            ecma_deref_ecma_string (item_str_p);
          }
        }

        ECMA_FINALIZE (value);

        ecma_deref_ecma_string (index_str_p);

        index++;
      }

      ECMA_OP_TO_NUMBER_FINALIZE (array_length_num);
      ECMA_FINALIZE (array_length);

      ecma_deref_ecma_string (length_str_p);
    }
  }

  ecma_value_t space = ecma_copy_value (arg3, true);

  /* 5. */
  if (ecma_is_value_object (arg3))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (arg3);
    lit_magic_string_id_t class_name = ecma_object_get_class_name (obj_p);

    /* 5.a */
    if (class_name == LIT_MAGIC_STRING_NUMBER_UL)
    {
      ECMA_TRY_CATCH (val,
                      ecma_op_to_number (arg3),
                      ret_value);

      ecma_free_value (space, true);
      space = ecma_copy_value (val, true);

      ECMA_FINALIZE (val);
    }
    /* 5.b */
    else if (class_name == LIT_MAGIC_STRING_STRING_UL)
    {
      ECMA_TRY_CATCH (val,
                      ecma_op_to_string (arg3),
                      ret_value);

      ecma_free_value (space, true);
      space = ecma_copy_value (val, true);

      ECMA_FINALIZE (val);
    }
  }

  /* 6. */
  if (ecma_is_value_number (space))
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (array_length_num,
                                 arg3,
                                 ret_value);

    /* 6.a */
    int32_t num_of_spaces = ecma_number_to_int32 (array_length_num);
    int32_t space = (num_of_spaces > 10) ? 10 : num_of_spaces;

    /* 6.b */
    if (space < 1)
    {
      context_p.gap_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
    }
    else
    {
      MEM_DEFINE_LOCAL_ARRAY (space_buff, space, char);

      for (int32_t i = 0; i < space; i++)
      {
        space_buff[i] = ' ';
      }

      context_p.gap_str_p = ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) space_buff, (lit_utf8_size_t) space);

      MEM_FINALIZE_LOCAL_ARRAY (space_buff);
    }

    ECMA_OP_TO_NUMBER_FINALIZE (array_length_num);
  }
  /* 7. */
  else if (ecma_is_value_string (space))
  {
    ecma_string_t *space_str_p = ecma_get_string_from_value (space);
    ecma_length_t num_of_chars = ecma_string_get_length (space_str_p);

    if (num_of_chars < 10)
    {
      context_p.gap_str_p = ecma_copy_or_ref_ecma_string (space_str_p);
    }
    else
    {
      ecma_length_t string_len = ecma_string_get_length (space_str_p);

      MEM_DEFINE_LOCAL_ARRAY (zt_string_buff, string_len, lit_utf8_byte_t);

      size_t string_buf_size = (size_t) (string_len) * sizeof (lit_utf8_byte_t);
      ssize_t bytes_copied = ecma_string_to_utf8_string (space_str_p,
                                                         zt_string_buff,
                                                         (ssize_t) string_buf_size);
      JERRY_ASSERT (bytes_copied > 0);

      /* Buffer for the first 10 characters. */
      MEM_DEFINE_LOCAL_ARRAY (space_buff, 10, lit_utf8_byte_t);

      for (uint32_t i = 0; i < 10; i++)
      {
        space_buff[i] = zt_string_buff[i];
      }

      context_p.gap_str_p = ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) space_buff, 10);

      MEM_FINALIZE_LOCAL_ARRAY (space_buff);
      MEM_FINALIZE_LOCAL_ARRAY (zt_string_buff);
    }
  }
  /* 8. */
  else
  {
    context_p.gap_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
  }

  ecma_free_value (space, true);

  /* 9. */
  ecma_object_t *obj_wrapper_p = ecma_create_object (NULL, true, ECMA_OBJECT_TYPE_GENERAL);
  ecma_string_t *empty_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);

  /* 10. */
  ecma_completion_value_t put_comp_val = ecma_op_object_put (obj_wrapper_p,
                                                             empty_str_p,
                                                             arg1,
                                                             false);

  JERRY_ASSERT (ecma_is_completion_value_normal_true (put_comp_val));
  ecma_free_completion_value (put_comp_val);

  /* 11. */
  ECMA_TRY_CATCH (str_val,
                  ecma_builtin_json_str (empty_str_p, obj_wrapper_p, &context_p),
                  ret_value);

  ret_value = ecma_make_normal_completion_value (ecma_copy_value (str_val, true));

  ECMA_FINALIZE (str_val);

  ecma_deref_object (obj_wrapper_p);
  ecma_deref_ecma_string (empty_str_p);

  ecma_deref_ecma_string (context_p.indent_str_p);
  ecma_deref_ecma_string (context_p.gap_str_p);

  free_list_with_ecma_string_content (&context_p.property_list);
  free_list (&context_p.occurence_stack);

  return ret_value;
} /* ecma_builtin_json_stringify */

/**
 * Abstract operation 'Quote' defined in 15.12.3
 *
 * See also:
 *          ECMA-262 v5, 15.12.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_json_quote (ecma_string_t *string_p) /**< string that should be quoted*/
{
  /* 1. */
  ecma_string_t *quote_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_DOUBLE_QUOTE_CHAR);
  ecma_string_t *product_str_p = ecma_copy_or_ref_ecma_string (quote_str_p);
  ecma_string_t *tmp_str_p;

  ecma_length_t string_len = ecma_string_get_length (string_p);

  MEM_DEFINE_LOCAL_ARRAY (zt_string_buff, string_len, lit_utf8_byte_t);

  size_t string_buf_size = (size_t) (string_len) * sizeof (lit_utf8_byte_t);
  ssize_t bytes_copied = ecma_string_to_utf8_string (string_p,
                                                     zt_string_buff,
                                                     (ssize_t) string_buf_size);
  JERRY_ASSERT (bytes_copied > 0 || !string_len);

  /* 2. */
  for (ecma_length_t i = 0; i < string_len; i++)
  {
    lit_utf8_byte_t c = zt_string_buff[i];

    /* 2.a */
    if (c == '\\' || c == '\"')
    {
      ecma_string_t *backslash_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_BACKSLASH_CHAR);

      /* 2.a.i */
      tmp_str_p = ecma_concat_ecma_strings (product_str_p, backslash_str_p);
      ecma_deref_ecma_string (product_str_p);
      ecma_deref_ecma_string (backslash_str_p);
      product_str_p = tmp_str_p;

      /* 2.a.ii */
      ecma_string_t *c_str_p = ecma_new_ecma_string_from_utf8 (&c, 1);

      tmp_str_p = ecma_concat_ecma_strings (product_str_p, c_str_p);
      ecma_deref_ecma_string (product_str_p);
      ecma_deref_ecma_string (c_str_p);
      product_str_p = tmp_str_p;
    }
    /* 2.b */
    else if (c == '\b' || c == '\f' || c == '\n' || c == '\r' || c == '\t')
    {
      ecma_string_t *backslash_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_BACKSLASH_CHAR);

      /* 2.b.i */
      tmp_str_p = ecma_concat_ecma_strings (product_str_p, backslash_str_p);
      ecma_deref_ecma_string (product_str_p);
      ecma_deref_ecma_string (backslash_str_p);
      product_str_p = tmp_str_p;

      /* 2.b.ii */
      lit_utf8_byte_t abbrev = ' ';

      switch (c)
      {
        case '\b':
        {
          abbrev = 'b';
          break;
        }
        case '\f':
        {
          abbrev = 'f';
          break;
        }
        case '\n':
        {
          abbrev = 'n';
          break;
        }
        case '\r':
        {
          abbrev = 'r';
          break;
        }
        case '\t':
        {
          abbrev = 't';
          break;
        }
      }

      /* 2.b.iii */
      ecma_string_t *abbrev_str_p = ecma_new_ecma_string_from_utf8 (&abbrev, 1);

      tmp_str_p = ecma_concat_ecma_strings (product_str_p, abbrev_str_p);
      ecma_deref_ecma_string (product_str_p);
      ecma_deref_ecma_string (abbrev_str_p);
      product_str_p = tmp_str_p;
    }
    /* 2.c */
    else if (c < LIT_CHAR_SP)
    {
      ecma_string_t *backslash_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_BACKSLASH_CHAR);

      /* 2.c.i */
      tmp_str_p = ecma_concat_ecma_strings (product_str_p, backslash_str_p);
      ecma_deref_ecma_string (product_str_p);
      ecma_deref_ecma_string (backslash_str_p);
      product_str_p = tmp_str_p;

      /* 2.c.ii */
      lit_utf8_byte_t u_ch = 'u';
      ecma_string_t *u_ch_str_p = ecma_new_ecma_string_from_utf8 (&u_ch, 1);

      tmp_str_p = ecma_concat_ecma_strings (product_str_p, u_ch_str_p);
      ecma_deref_ecma_string (product_str_p);
      ecma_deref_ecma_string (u_ch_str_p);
      product_str_p = tmp_str_p;

      /* 2.c.iii */
      ecma_string_t *hex_str_p = ecma_builtin_helper_json_create_hex_digit_ecma_string (c);

      /* 2.c.iv */
      tmp_str_p = ecma_concat_ecma_strings (product_str_p, hex_str_p);
      ecma_deref_ecma_string (product_str_p);
      ecma_deref_ecma_string (hex_str_p);
      product_str_p = tmp_str_p;
    }
    /* 2.d */
    else
    {
      ecma_string_t *c_str_p = ecma_new_ecma_string_from_utf8 (&c, 1);

      tmp_str_p = ecma_concat_ecma_strings (product_str_p, c_str_p);
      ecma_deref_ecma_string (product_str_p);
      ecma_deref_ecma_string (c_str_p);
      product_str_p = tmp_str_p;
    }
  }

  MEM_FINALIZE_LOCAL_ARRAY (zt_string_buff);

  /* 3. */
  tmp_str_p = ecma_concat_ecma_strings (product_str_p, quote_str_p);
  ecma_deref_ecma_string (product_str_p);
  ecma_deref_ecma_string (quote_str_p);
  product_str_p = tmp_str_p;

  /* 4. */
  return ecma_make_normal_completion_value (ecma_make_string_value (product_str_p));
} /* ecma_builtin_json_quote */

/**
 * Abstract operation 'Str' defined in 15.12.3
 *
 * See also:
 *          ECMA-262 v5, 15.12.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_json_str (ecma_string_t *key_p, /**< property key*/
                       ecma_object_t *holder_p, /**< the object*/
                       stringify_context_t *context_p) /**< context*/
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 1. */
  ECMA_TRY_CATCH (value,
                  ecma_op_object_get (holder_p, key_p),
                  ret_value);

  ecma_value_t my_val = ecma_copy_value (value, true);

  /* 2. */
  if (ecma_is_value_object (my_val))
  {
    ecma_object_t *value_obj_p = ecma_get_object_from_value (my_val);
    ecma_string_t *to_json_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_TO_JSON_UL);

    /* 2.a */
    ECMA_TRY_CATCH (toJSON,
                    ecma_op_object_get (value_obj_p, to_json_str_p),
                    ret_value);

    /* 2.b */
    if (ecma_op_is_callable (toJSON))
    {
      ecma_value_t key_value = ecma_make_string_value (key_p);
      ecma_value_t call_args[] = { key_value };
      ecma_object_t *toJSON_obj_p = ecma_get_object_from_value (toJSON);

      ECMA_TRY_CATCH (func_ret_val,
                      ecma_op_function_call (toJSON_obj_p, my_val, call_args, 1),
                      ret_value);

      ecma_free_value (my_val, true);
      my_val = ecma_copy_value (func_ret_val, true);

      ECMA_FINALIZE (func_ret_val);
    }

    ECMA_FINALIZE (toJSON);

    ecma_deref_ecma_string (to_json_str_p);
  }

  /* 3. */
  if (context_p->replacer_function_p && ecma_is_completion_value_empty (ret_value))
  {
    ecma_value_t holder_value = ecma_make_object_value (holder_p);
    ecma_value_t key_value = ecma_make_string_value (key_p);
    ecma_value_t call_args[] = { key_value, my_val };

    ECMA_TRY_CATCH (func_ret_val,
                    ecma_op_function_call (context_p->replacer_function_p, holder_value, call_args, 2),
                    ret_value);

    ecma_free_value (my_val, true);
    my_val = ecma_copy_value (func_ret_val, true);

    ECMA_FINALIZE (func_ret_val);
  }

  /* 4. */
  if (ecma_is_value_object (my_val) && ecma_is_completion_value_empty (ret_value))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (my_val);
    lit_magic_string_id_t class_name = ecma_object_get_class_name (obj_p);

    /* 4.a */
    if (class_name == LIT_MAGIC_STRING_NUMBER_UL)
    {
      ECMA_TRY_CATCH (val,
                      ecma_op_to_number (my_val),
                      ret_value);

      ecma_free_value (my_val, true);
      my_val = ecma_copy_value (val, true);

      ECMA_FINALIZE (val);
    }
    /* 4.b */
    else if (class_name == LIT_MAGIC_STRING_STRING_UL)
    {
      ECMA_TRY_CATCH (val,
                      ecma_op_to_string (my_val),
                      ret_value);

      ecma_free_value (my_val, true);
      my_val = ecma_copy_value (val, true);

      ECMA_FINALIZE (val);
    }
    /* 4.c */
    else if (class_name == LIT_MAGIC_STRING_BOOLEAN_UL)
    {
      ECMA_TRY_CATCH (val,
                      ecma_op_to_primitive (my_val, ECMA_PREFERRED_TYPE_NO),
                      ret_value);

      ecma_free_value (my_val, true);
      my_val = ecma_copy_value (val, true);

      ECMA_FINALIZE (val);
    }
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    /* 5. - 7. */
    if (ecma_is_value_null (my_val) || ecma_is_value_boolean (my_val))
    {
      ret_value = ecma_op_to_string (my_val);
      JERRY_ASSERT (ecma_is_completion_value_normal (ret_value));
    }
    /* 8. */
    else if (ecma_is_value_string (my_val))
    {
      ecma_string_t *value_str_p = ecma_get_string_from_value (my_val);
      ret_value = ecma_builtin_json_quote (value_str_p);
    }
    /* 9. */
    else if (ecma_is_value_number (my_val))
    {
      ecma_number_t num_value_p = *ecma_get_number_from_value (my_val);

      /* 9.a */
      if (!ecma_number_is_infinity (num_value_p))
      {
        ret_value = ecma_op_to_string (my_val);
        JERRY_ASSERT (ecma_is_completion_value_normal (ret_value));
      }
      else
      {
        /* 9.b */
        ecma_string_t *null_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_NULL);
        ret_value = ecma_make_normal_completion_value (ecma_make_string_value (null_str_p));
      }
    }
    /* 10. */
    else if (ecma_is_value_object (my_val) && !ecma_op_is_callable (my_val))
    {
      ecma_object_t *obj_p = ecma_get_object_from_value (my_val);
      lit_magic_string_id_t class_name = ecma_object_get_class_name (obj_p);

      /* 10.a */
      if (class_name == LIT_MAGIC_STRING_ARRAY_UL)
      {
        ECMA_TRY_CATCH (val,
                        ecma_builtin_json_array (obj_p, context_p),
                        ret_value);

        ret_value = ecma_make_normal_completion_value (ecma_copy_value (val, true));

        ECMA_FINALIZE (val);
      }
      /* 10.b */
      else
      {
        ECMA_TRY_CATCH (val,
                        ecma_builtin_json_object (obj_p, context_p),
                        ret_value);

        ret_value = ecma_make_normal_completion_value (ecma_copy_value (val, true));

        ECMA_FINALIZE (val);
      }
    }
    else
    {
      /* 11. */
      ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
  }

  ecma_free_value (my_val, true);
  ECMA_FINALIZE (value);

  return ret_value;
} /* ecma_builtin_json_str */

/**
 * Abstract operation 'JO' defined in 15.12.3
 *
 * See also:
 *          ECMA-262 v5, 15.12.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_json_object (ecma_object_t *obj_p, /**< the object*/
                          stringify_context_t *context_p) /**< context*/
{
  /* 1. */
  if (list_has_element (&context_p->occurence_stack, obj_p))
  {
    return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 2. */
  list_append (&context_p->occurence_stack, obj_p);

  /* 3. */
  ecma_string_t *stepback_p = context_p->indent_str_p;

  /* 4. */
  context_p->indent_str_p = ecma_concat_ecma_strings (context_p->indent_str_p, context_p->gap_str_p);

  list_ctx_t property_keys;
  {
    property_keys.block_start_p = NULL;
    property_keys.block_end_p = NULL;
    property_keys.current_p = NULL;
  }

  /* 5. */
  if (!list_is_empty (&context_p->property_list))
  {
    property_keys = context_p->property_list;
  }
  /* 6. */
  else
  {
    for (ecma_property_t *property_p = ecma_get_property_list (obj_p);
         property_p != NULL;
         property_p = ECMA_GET_POINTER (ecma_property_t, property_p->next_property_p))
    {
      if (property_p->type == ECMA_PROPERTY_NAMEDDATA && ecma_is_property_enumerable (property_p))
      {
        ecma_string_t *prop_key = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                           property_p->u.named_data_property.name_p);

        JERRY_ASSERT (prop_key != NULL);

        list_append (&property_keys, ecma_copy_or_ref_ecma_string (prop_key));
      }
    }
  }

  /* 7. */
  list_ctx_t partial;
  {
    partial.block_start_p = NULL;
    partial.block_end_p = NULL;
    partial.current_p = NULL;
  }

  /* 8. */
  for (void **key_p = property_keys.block_start_p;
       key_p < property_keys.current_p && ecma_is_completion_value_empty (ret_value);
       key_p++)
  {
    /* 8.a */
    ECMA_TRY_CATCH (str_val,
                    ecma_builtin_json_str ((ecma_string_t *) *key_p, obj_p, context_p),
                    ret_value);

    /* 8.b */
    if (!ecma_is_value_undefined (str_val))
    {
      ecma_string_t *colon_p = ecma_get_magic_string (LIT_MAGIC_STRING_COLON_CHAR);
      ecma_string_t *value_str_p = ecma_get_string_from_value (str_val);
      ecma_string_t *tmp_str_p;

      /* 8.b.i */
      ecma_completion_value_t str_comp_val = ecma_builtin_json_quote ((ecma_string_t *) *key_p);
      JERRY_ASSERT (ecma_is_completion_value_normal (str_comp_val));

      ecma_string_t *member_str_p = ecma_get_string_from_value (str_comp_val);

      /* 8.b.ii */
      tmp_str_p = ecma_concat_ecma_strings (member_str_p, colon_p);
      ecma_free_completion_value (str_comp_val);
      ecma_deref_ecma_string (colon_p);
      member_str_p = tmp_str_p;

      /* 8.b.iii */
      bool is_gap_empty = (ecma_string_get_length (context_p->gap_str_p) == 0);

      if (!is_gap_empty)
      {
        ecma_string_t *space_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_SPACE_CHAR);

        tmp_str_p = ecma_concat_ecma_strings (member_str_p, space_str_p);
        ecma_deref_ecma_string (member_str_p);
        ecma_deref_ecma_string (space_str_p);
        member_str_p = tmp_str_p;
      }

      /* 8.b.iv */
      tmp_str_p = ecma_concat_ecma_strings (member_str_p, value_str_p);
      ecma_deref_ecma_string (member_str_p);
      member_str_p = tmp_str_p;

      /* 8.b.v */
      list_append (&partial, member_str_p);
    }

    ECMA_FINALIZE (str_val);
  }

  if (list_is_empty (&context_p->property_list))
  {
    free_list_with_ecma_string_content (&property_keys);
  }

  if (!ecma_is_completion_value_empty (ret_value))
  {
    free_list_with_ecma_string_content (&partial);
    ecma_deref_ecma_string (stepback_p);
    return ret_value;
  }

  /* 9. */
  if (list_is_empty (&partial))
  {
    ecma_string_t *left_brace_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LEFT_BRACE_CHAR);
    ecma_string_t *right_brace_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_RIGHT_BRACE_CHAR);

    ecma_string_t *final_str_p = ecma_concat_ecma_strings (left_brace_str_p, right_brace_str_p);
    ret_value = ecma_make_normal_completion_value (ecma_make_string_value (final_str_p));

    ecma_deref_ecma_string (left_brace_str_p);
    ecma_deref_ecma_string (right_brace_str_p);
  }
  /* 10. */
  else
  {
    bool is_gap_empty = (ecma_string_get_length (context_p->gap_str_p) == 0);

    /* 10.a */
    if (is_gap_empty)
    {
      ecma_string_t *left_brace_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LEFT_BRACE_CHAR);
      ecma_string_t *right_brace_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_RIGHT_BRACE_CHAR);

      ret_value = ecma_builtin_helper_json_create_non_formatted_json (left_brace_str_p,
                                                                      right_brace_str_p,
                                                                      &partial);

      ecma_deref_ecma_string (left_brace_str_p);
      ecma_deref_ecma_string (right_brace_str_p);
    }
    /* 10.b */
    else
    {
      ecma_string_t *left_brace_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LEFT_BRACE_CHAR);
      ecma_string_t *right_brace_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_RIGHT_BRACE_CHAR);

      ret_value = ecma_builtin_helper_json_create_formatted_json (left_brace_str_p,
                                                                  right_brace_str_p,
                                                                  stepback_p,
                                                                  &partial,
                                                                  context_p);

      ecma_deref_ecma_string (left_brace_str_p);
      ecma_deref_ecma_string (right_brace_str_p);
    }
  }

  free_list_with_ecma_string_content (&partial);

  /* 11. */
  list_remove_last_element (&context_p->occurence_stack);

  /* 12. */
  ecma_deref_ecma_string (context_p->indent_str_p);
  context_p->indent_str_p = stepback_p;

  /* 13. */
  return ret_value;
} /* ecma_builtin_json_object */

/**
 * Abstract operation 'JA' defined in 15.12.3
 *
 * See also:
 *          ECMA-262 v5, 15.12.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_json_array (ecma_object_t *obj_p, /**< the array object*/
                         stringify_context_t *context_p) /**< context*/
{
  /* 1. */
  if (list_has_element (&context_p->occurence_stack, obj_p))
  {
    return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 2. */
  list_append (&context_p->occurence_stack, obj_p);

  /* 3. */
  ecma_string_t *stepback_p = context_p->indent_str_p;

  /* 4. */
  context_p->indent_str_p = ecma_concat_ecma_strings (context_p->indent_str_p, context_p->gap_str_p);

  /* 5. */
  list_ctx_t partial;
  {
    partial.block_start_p = NULL;
    partial.block_end_p = NULL;
    partial.current_p = NULL;
  }

  ecma_string_t *length_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);

  /* 6. */
  ECMA_TRY_CATCH (array_length,
                  ecma_op_object_get (obj_p, length_str_p),
                  ret_value);

  ECMA_OP_TO_NUMBER_TRY_CATCH (array_length_num,
                               array_length,
                               ret_value);

  uint32_t array_length = ecma_number_to_uint32 (array_length_num);

  /* 7. - 8. */
  for (uint32_t index = 0;
       index < array_length && ecma_is_completion_value_empty (ret_value);
       index++)
  {

    /* 8.a */
    ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (index);

    ECMA_TRY_CATCH (str_val,
                    ecma_builtin_json_str (index_str_p, obj_p, context_p),
                    ret_value);

    /* 8.b */
    if (ecma_is_value_undefined (str_val))
    {
      list_append (&partial, ecma_get_magic_string (LIT_MAGIC_STRING_NULL));
    }
    /* 8.c */
    else
    {
      ecma_string_t *str_val_p = ecma_get_string_from_value (str_val);
      list_append (&partial, ecma_copy_or_ref_ecma_string (str_val_p));
    }

    ECMA_FINALIZE (str_val);
    ecma_deref_ecma_string (index_str_p);
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    /* 9. */
    if (list_is_empty (&partial))
    {
      ecma_string_t *left_square_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LEFT_SQUARE_CHAR);
      ecma_string_t *right_square_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_RIGHT_SQUARE_CHAR);

      ecma_string_t *final_str_p = ecma_concat_ecma_strings (left_square_str_p, right_square_str_p);
      ret_value = ecma_make_normal_completion_value (ecma_make_string_value (final_str_p));

      ecma_deref_ecma_string (left_square_str_p);
      ecma_deref_ecma_string (right_square_str_p);
    }
    /* 10. */
    else
    {
      bool is_gap_empty = (ecma_string_get_length (context_p->gap_str_p) == 0);
      /* 10.a */
      if (is_gap_empty)
      {
        ecma_string_t *left_square_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LEFT_SQUARE_CHAR);
        ecma_string_t *right_square_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_RIGHT_SQUARE_CHAR);

        ret_value = ecma_builtin_helper_json_create_non_formatted_json (left_square_str_p,
                                                                        right_square_str_p,
                                                                        &partial);

        ecma_deref_ecma_string (left_square_str_p);
        ecma_deref_ecma_string (right_square_str_p);
      }
      /* 10.b */
      else
      {
        ecma_string_t *left_square_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_LEFT_SQUARE_CHAR);
        ecma_string_t *right_square_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_RIGHT_SQUARE_CHAR);

        ret_value = ecma_builtin_helper_json_create_formatted_json (left_square_str_p,
                                                                    right_square_str_p,
                                                                    stepback_p,
                                                                    &partial,
                                                                    context_p);

        ecma_deref_ecma_string (left_square_str_p);
        ecma_deref_ecma_string (right_square_str_p);
      }
    }
  }

  ECMA_OP_TO_NUMBER_FINALIZE (array_length_num);
  ECMA_FINALIZE (array_length);

  ecma_deref_ecma_string (length_str_p);
  free_list_with_ecma_string_content (&partial);

  /* 11. */
  list_remove_last_element (&context_p->occurence_stack);

  /* 12. */
  ecma_deref_ecma_string (context_p->indent_str_p);
  context_p->indent_str_p = stepback_p;

  /* 13. */
  return ret_value;
} /* ecma_builtin_json_array */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_JSON_BUILTIN */
