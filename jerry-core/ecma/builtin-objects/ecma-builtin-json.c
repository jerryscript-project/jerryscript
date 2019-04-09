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
#include "lit-globals.h"

#if ENABLED (JERRY_BUILTIN_JSON)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-json.inc.h"
#define BUILTIN_UNDERSCORED_ID json
#include "ecma-builtin-internal-routines-template.inc.h"

/**
 * The number of expected hexidecimal characters in a hex escape sequence
 */
#define ECMA_JSON_HEX_ESCAPE_SEQUENCE_LENGTH (4)

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
  const lit_utf8_byte_t *current_p; /**< current position of the string processed by the parser */
  const lit_utf8_byte_t *end_p; /**< end of the string processed by the parser */

  /**
   * Fields depending on type.
   */
  union
  {
    ecma_string_t *string_p; /**< when type is string_token it contains the string */
    ecma_number_t number; /**< when type is number_token, it contains the value of the number */
  } u;
} ecma_json_token_t;

/**
 * Compare the string with an ID.
 *
 * @return true if the match is successful
 */
static bool
ecma_builtin_json_check_id (const lit_utf8_byte_t *string_p, /**< start position */
                            const lit_utf8_byte_t *end_p, /**< input end */
                            const char *string_id_p) /**< string identifier */
{
  /*
   * String comparison must not depend on lit_utf8_byte_t definition.
   */
  JERRY_ASSERT (*string_p == *string_id_p);

  string_p++;
  string_id_p++;

  while (string_p < end_p)
  {
    if (*string_id_p == LIT_CHAR_NULL)
    {
      /* JSON lexer accepts input strings such as falsenull and
       * returns with multiple tokens (false and null in this case).
       * This is different from normal lexers, whose return with a
       * single identifier. The falsenull input will still cause an
       * error eventually, since a separator must always be present
       * between two JSON values regardless of the current expression
       * type. */
      return true;
    }

    if (*string_p != *string_id_p)
    {
      return false;
    }

    string_p++;
    string_id_p++;
  }

  return (*string_id_p == LIT_CHAR_NULL);
} /* ecma_builtin_json_check_id */

/**
 * Parse and extract string token.
 */
static void
ecma_builtin_json_parse_string (ecma_json_token_t *token_p) /**< token argument */
{
  const lit_utf8_byte_t *current_p = token_p->current_p;
  const lit_utf8_byte_t *end_p = token_p->end_p;
  bool has_escape_sequence = false;
  lit_utf8_size_t buffer_size = 0;

  /* First step: syntax checking. */
  while (true)
  {
    if (current_p >= end_p || *current_p <= 0x1f)
    {
      return;
    }

    if (*current_p == LIT_CHAR_DOUBLE_QUOTE)
    {
      break;
    }

    if (*current_p == LIT_CHAR_BACKSLASH)
    {
      current_p++;
      has_escape_sequence = true;

      /* If there is an escape sequence but there's no escapable character just return */
      if (current_p >= end_p)
      {
        return;
      }

      switch (*current_p)
      {
        case LIT_CHAR_DOUBLE_QUOTE:
        case LIT_CHAR_SLASH:
        case LIT_CHAR_BACKSLASH:
        case LIT_CHAR_LOWERCASE_B:
        case LIT_CHAR_LOWERCASE_F:
        case LIT_CHAR_LOWERCASE_N:
        case LIT_CHAR_LOWERCASE_R:
        case LIT_CHAR_LOWERCASE_T:
        {
          break;
        }
        case LIT_CHAR_LOWERCASE_U:
        {
          if ((end_p - current_p <= ECMA_JSON_HEX_ESCAPE_SEQUENCE_LENGTH))
          {
            return;
          }

          ecma_char_t code_unit;
          if (!(lit_read_code_unit_from_hex (current_p + 1, ECMA_JSON_HEX_ESCAPE_SEQUENCE_LENGTH, &code_unit)))
          {
            return;
          }

          current_p += ECMA_JSON_HEX_ESCAPE_SEQUENCE_LENGTH + 1;

          lit_utf8_byte_t char_buffer[LIT_UTF8_MAX_BYTES_IN_CODE_UNIT];
          buffer_size += lit_code_unit_to_utf8 (code_unit, char_buffer);
          continue;
        }
        default:
        {
          return;
        }
      }
    }

    buffer_size++;
    current_p++;
  }

  token_p->type = string_token;

  if (!has_escape_sequence)
  {
    token_p->u.string_p = ecma_new_ecma_string_from_utf8 (token_p->current_p, buffer_size);
    token_p->current_p = current_p + 1;
    return;
  }

  JMEM_DEFINE_LOCAL_ARRAY (buffer_p, buffer_size, lit_utf8_byte_t);

  lit_utf8_byte_t *write_p = buffer_p;
  current_p = token_p->current_p;

  while (*current_p != LIT_CHAR_DOUBLE_QUOTE)
  {
    if (*current_p == LIT_CHAR_BACKSLASH)
    {
      current_p++;

      lit_utf8_byte_t special_character;

      switch (*current_p)
      {
        case LIT_CHAR_LOWERCASE_B:
        {
          special_character = LIT_CHAR_BS;
          break;
        }
        case LIT_CHAR_LOWERCASE_F:
        {
          special_character = LIT_CHAR_FF;
          break;
        }
        case LIT_CHAR_LOWERCASE_N:
        {
          special_character = LIT_CHAR_LF;
          break;
        }
        case LIT_CHAR_LOWERCASE_R:
        {
          special_character = LIT_CHAR_CR;
          break;
        }
        case LIT_CHAR_LOWERCASE_T:
        {
          special_character = LIT_CHAR_TAB;
          break;
        }
        case LIT_CHAR_LOWERCASE_U:
        {
          ecma_char_t code_unit;

          lit_read_code_unit_from_hex (current_p + 1, ECMA_JSON_HEX_ESCAPE_SEQUENCE_LENGTH, &code_unit);

          current_p += ECMA_JSON_HEX_ESCAPE_SEQUENCE_LENGTH + 1;
          write_p += lit_code_unit_to_utf8 (code_unit, write_p);
          continue;
        }
        default:
        {
          special_character = *current_p;
          break;
        }
      }

      *write_p++ = special_character;
      current_p++;
      continue;
    }

    *write_p++ = *current_p++;
  }

  JERRY_ASSERT (write_p == buffer_p + buffer_size);

  token_p->u.string_p = ecma_new_ecma_string_from_utf8 (buffer_p, buffer_size);

  JMEM_FINALIZE_LOCAL_ARRAY (buffer_p);

  token_p->current_p = current_p + 1;
} /* ecma_builtin_json_parse_string */

/**
 * Parse and extract string token.
 */
static void
ecma_builtin_json_parse_number (ecma_json_token_t *token_p) /**< token argument */
{
  const lit_utf8_byte_t *current_p = token_p->current_p;
  const lit_utf8_byte_t *end_p = token_p->end_p;
  const lit_utf8_byte_t *start_p = current_p;

  JERRY_ASSERT (current_p < end_p);

  if (*current_p == LIT_CHAR_MINUS)
  {
    current_p++;
  }

  if (current_p >= end_p)
  {
    return;
  }

  if (*current_p == LIT_CHAR_0)
  {
    current_p++;

    if (current_p < end_p && lit_char_is_decimal_digit (*current_p))
    {
      return;
    }
  }
  else if (lit_char_is_decimal_digit (*current_p))
  {
    do
    {
      current_p++;
    }
    while (current_p < end_p && lit_char_is_decimal_digit (*current_p));
  }

  if (current_p < end_p && *current_p == LIT_CHAR_DOT)
  {
    current_p++;

    if (current_p >= end_p || !lit_char_is_decimal_digit (*current_p))
    {
      return;
    }

    do
    {
      current_p++;
    }
    while (current_p < end_p && lit_char_is_decimal_digit (*current_p));
  }

  if (current_p < end_p && (*current_p == LIT_CHAR_LOWERCASE_E || *current_p == LIT_CHAR_UPPERCASE_E))
  {
    current_p++;

    if (current_p < end_p && (*current_p == LIT_CHAR_PLUS || *current_p == LIT_CHAR_MINUS))
    {
      current_p++;
    }

    if (current_p >= end_p || !lit_char_is_decimal_digit (*current_p))
    {
      return;
    }

    do
    {
      current_p++;
    }
    while (current_p < end_p && lit_char_is_decimal_digit (*current_p));
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
ecma_builtin_json_parse_next_token (ecma_json_token_t *token_p, /**< token argument */
                                    bool parse_string) /**< strings are allowed to parse */
{
  const lit_utf8_byte_t *current_p = token_p->current_p;
  const lit_utf8_byte_t *end_p = token_p->end_p;
  token_p->type = invalid_token;

  while (current_p < end_p
         && (*current_p == LIT_CHAR_SP
             || *current_p == LIT_CHAR_CR
             || *current_p == LIT_CHAR_LF
             || *current_p == LIT_CHAR_TAB))
  {
    current_p++;
  }

  if (current_p == end_p)
  {
    token_p->type = end_token;
    return;
  }

  switch (*current_p)
  {
    case LIT_CHAR_LEFT_BRACE:
    {
      token_p->type = left_brace_token;
      token_p->current_p = current_p + 1;
      return;
    }
    case LIT_CHAR_RIGHT_BRACE:
    {
      token_p->type = right_brace_token;
      token_p->current_p = current_p + 1;
      return;
    }
    case LIT_CHAR_LEFT_SQUARE:
    {
      token_p->type = left_square_token;
      token_p->current_p = current_p + 1;
      return;
    }
    case LIT_CHAR_COMMA:
    {
      token_p->type = comma_token;
      token_p->current_p = current_p + 1;
      return;
    }
    case LIT_CHAR_COLON:
    {
      token_p->type = colon_token;
      token_p->current_p = current_p + 1;
      return;
    }
    case LIT_CHAR_DOUBLE_QUOTE:
    {
      if (parse_string)
      {
        token_p->current_p = current_p + 1;
        ecma_builtin_json_parse_string (token_p);
      }
      return;
    }
    case LIT_CHAR_LOWERCASE_N:
    {
      if (ecma_builtin_json_check_id (current_p, token_p->end_p, "null"))
      {
        token_p->type = null_token;
        token_p->current_p = current_p + 4;
        return;
      }
      break;
    }
    case LIT_CHAR_LOWERCASE_T:
    {
      if (ecma_builtin_json_check_id (current_p, token_p->end_p, "true"))
      {
        token_p->type = true_token;
        token_p->current_p = current_p + 4;
        return;
      }
      break;
    }
    case LIT_CHAR_LOWERCASE_F:
    {
      if (ecma_builtin_json_check_id (current_p, token_p->end_p, "false"))
      {
        token_p->type = false_token;
        token_p->current_p = current_p + 5;
        return;
      }
      break;
    }
    default:
    {
      if (*current_p == LIT_CHAR_MINUS || lit_char_is_decimal_digit (*current_p))
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
  const lit_utf8_byte_t *current_p = token_p->current_p;
  const lit_utf8_byte_t *end_p = token_p->end_p;

  /*
   * No need for end check since the string is zero terminated.
   */
  while (current_p < end_p
         && (*current_p == LIT_CHAR_SP
             || *current_p == LIT_CHAR_CR
             || *current_p == LIT_CHAR_LF
             || *current_p == LIT_CHAR_TAB))
  {
    current_p++;
  }

  token_p->current_p = current_p;

  if (current_p < end_p && *current_p == LIT_CHAR_RIGHT_SQUARE)
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
  ecma_value_t completion_value = ecma_builtin_helper_def_prop (obj_p,
                                                                property_name_p,
                                                                value,
                                                                ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                                false); /* Failure handling */

  JERRY_ASSERT (ecma_is_value_boolean (completion_value));
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
  ecma_builtin_json_parse_next_token (token_p, true);

  switch (token_p->type)
  {
    case number_token:
    {
      return ecma_make_number_value (token_p->u.number);
    }
    case string_token:
    {
      return ecma_make_string_value (token_p->u.string_p);
    }
    case null_token:
    {
      return ECMA_VALUE_NULL;
    }
    case true_token:
    {
      return ECMA_VALUE_TRUE;
    }
    case false_token:
    {
      return ECMA_VALUE_FALSE;
    }
    case left_brace_token:
    {
      bool parse_comma = false;
      ecma_object_t *object_p = ecma_op_create_object_object_noarg ();

      while (true)
      {
        ecma_builtin_json_parse_next_token (token_p, !parse_comma);

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

          ecma_builtin_json_parse_next_token (token_p, true);
        }

        if (token_p->type != string_token)
        {
          break;
        }

        ecma_string_t *name_p = token_p->u.string_p;

        ecma_builtin_json_parse_next_token (token_p, false);

        if (token_p->type != colon_token)
        {
          ecma_deref_ecma_string (name_p);
          break;
        }

        ecma_value_t value = ecma_builtin_json_parse_value (token_p);

        if (ecma_is_value_undefined (value))
        {
          ecma_deref_ecma_string (name_p);
          break;
        }

        ecma_builtin_json_define_value_property (object_p, name_p, value);
        ecma_deref_ecma_string (name_p);
        ecma_free_value (value);

        parse_comma = true;
      }

      /*
       * Parse error occured.
       */
      ecma_deref_object (object_p);
      return ECMA_VALUE_UNDEFINED;
    }
    case left_square_token:
    {
      bool parse_comma = false;
      uint32_t length = 0;

      ecma_value_t array_construction = ecma_op_create_array_object (NULL, 0, false);
      JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (array_construction));

      ecma_object_t *array_p = ecma_get_object_from_value (array_construction);

      while (true)
      {
        if (ecma_builtin_json_check_right_square_token (token_p))
        {
          return ecma_make_object_value (array_p);
        }

        if (parse_comma)
        {
          ecma_builtin_json_parse_next_token (token_p, false);

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

        ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (length);

        ecma_value_t completion = ecma_builtin_helper_def_prop (array_p,
                                                                index_str_p,
                                                                value,
                                                                ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                                false); /* Failure handling */

        JERRY_ASSERT (ecma_is_value_true (completion));

        ecma_deref_ecma_string (index_str_p);

        ecma_free_value (value);

        length++;
        parse_comma = true;
      }

      /*
       * Parse error occured.
       */
      ecma_deref_object (array_p);
      return ECMA_VALUE_UNDEFINED;
    }
    default:
    {
      return ECMA_VALUE_UNDEFINED;
    }
  }
} /* ecma_builtin_json_parse_value */

/**
 * Abstract operation Walk defined in 15.12.2
 *
 * See also:
 *          ECMA-262 v5, 15.12.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_json_walk (ecma_object_t *reviver_p, /**< reviver function */
                        ecma_object_t *holder_p, /**< holder object */
                        ecma_string_t *name_p) /**< property name */
{
  JERRY_ASSERT (reviver_p);
  JERRY_ASSERT (holder_p);
  JERRY_ASSERT (name_p);

  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  ECMA_TRY_CATCH (value_get,
                  ecma_op_object_get (holder_p, name_p),
                  ret_value);

  if (ecma_is_value_object (value_get))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (value_get);

    ecma_collection_header_t *props_p = ecma_op_object_get_property_names (object_p, ECMA_LIST_ENUMERABLE);

    ecma_value_t *ecma_value_p = ecma_collection_iterator_init (props_p);

    while (ecma_value_p != NULL && ecma_is_value_empty (ret_value))
    {
      ecma_string_t *property_name_p = ecma_get_string_from_value (*ecma_value_p);
      ecma_value_p = ecma_collection_iterator_next (ecma_value_p);

      ECMA_TRY_CATCH (value_walk,
                      ecma_builtin_json_walk (reviver_p,
                                              object_p,
                                              property_name_p),
                      ret_value);

      /*
       * We cannot optimize this function since any members
       * can be changed (deleted) by the reviver function.
       */
      if (ecma_is_value_undefined (value_walk))
      {
        ecma_value_t delete_val = ecma_op_general_object_delete (object_p,
                                                                 property_name_p,
                                                                 false);
        JERRY_ASSERT (ecma_is_value_boolean (delete_val));
      }
      else
      {
        ecma_builtin_json_define_value_property (object_p,
                                                 property_name_p,
                                                 value_walk);
      }

      ECMA_FINALIZE (value_walk);
    }

    ecma_free_values_collection (props_p, 0);
  }

  if (ecma_is_value_empty (ret_value))
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
    JERRY_ASSERT (ECMA_IS_VALUE_ERROR (ret_value));
  }

  ECMA_FINALIZE (value_get);

  return ret_value;
} /* ecma_builtin_json_walk */

/**
 * Function to set a string token from the given arguments, fills its fields and advances the string pointer.
 *
 * @return ecma_value_t containing an object or an error massage
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_json_parse_buffer (const lit_utf8_byte_t * str_start_p, /**< String to parse */
                                lit_utf8_size_t string_size) /**< size of the string */
{
  ecma_json_token_t token;
  token.current_p = str_start_p;
  token.end_p = str_start_p + string_size;

  ecma_value_t final_result = ecma_builtin_json_parse_value (&token);

  if (!ecma_is_value_undefined (final_result))
  {
    ecma_builtin_json_parse_next_token (&token, false);

    if (token.type != end_token)
    {
      ecma_free_value (final_result);
      final_result = ECMA_VALUE_UNDEFINED;
    }
  }
  return final_result;
} /*ecma_builtin_json_parse_buffer*/

/**
 * The JSON object's 'parse' routine
 *
 * See also:
 *          ECMA-262 v5, 15.12.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_json_parse (ecma_value_t this_arg, /**< 'this' argument */
                         ecma_value_t arg1, /**< string argument */
                         ecma_value_t arg2) /**< reviver argument */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  ECMA_TRY_CATCH (string,
                  ecma_op_to_string (arg1),
                  ret_value);

  const ecma_string_t *string_p = ecma_get_string_from_value (string);

  ECMA_STRING_TO_UTF8_STRING (string_p, str_start_p, string_size);

  ecma_value_t final_result = ecma_builtin_json_parse_buffer (str_start_p, string_size);

  if (ecma_is_value_undefined (final_result))
  {
    ret_value = ecma_raise_syntax_error (ECMA_ERR_MSG ("JSON string parse error."));
  }
  else
  {
    if (ecma_op_is_callable (arg2))
    {
      ecma_object_t *object_p = ecma_op_create_object_object_noarg ();

      ecma_property_value_t *prop_value_p;
      prop_value_p = ecma_create_named_data_property (object_p,
                                                      ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY),
                                                      ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                      NULL);

      ecma_named_data_property_assign_value (object_p, prop_value_p, final_result);
      ecma_free_value (final_result);

      ret_value = ecma_builtin_json_walk (ecma_get_object_from_value (arg2),
                                          object_p,
                                          ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY));
      ecma_deref_object (object_p);
    }
    else
    {
      ret_value = final_result;
    }
  }

  ECMA_FINALIZE_UTF8_STRING (str_start_p, string_size);

  ECMA_FINALIZE (string);
  return ret_value;
} /* ecma_builtin_json_parse */

static ecma_value_t
ecma_builtin_json_str (ecma_string_t *key_p, ecma_object_t *holder_p, ecma_json_stringify_context_t *context_p);

static ecma_value_t
ecma_builtin_json_object (ecma_object_t *obj_p, ecma_json_stringify_context_t *context_p);

static ecma_value_t
ecma_builtin_json_array (ecma_object_t *obj_p, ecma_json_stringify_context_t *context_p);

/**
 * Helper function to stringify an object in JSON format representing an ecma_value.
 *
 *  @return ecma_value_t string created from an abject formating by a given context
 *          Returned value must be freed with ecma_free_value.
 *
 */
static ecma_value_t ecma_builtin_json_str_helper (const ecma_value_t arg1, /**< object argument */
                                                  ecma_json_stringify_context_t context) /**< context argument */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;
  ecma_object_t *obj_wrapper_p = ecma_op_create_object_object_noarg ();
  ecma_string_t *empty_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
  ecma_value_t put_comp_val = ecma_builtin_helper_def_prop (obj_wrapper_p,
                                                            empty_str_p,
                                                            arg1,
                                                            ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                            false);

  JERRY_ASSERT (ecma_is_value_true (put_comp_val));

  ret_value = ecma_builtin_json_str (empty_str_p, obj_wrapper_p, &context);

  ecma_free_value (put_comp_val);
  ecma_deref_ecma_string (empty_str_p);
  ecma_deref_object (obj_wrapper_p);

  return ret_value;
} /* ecma_builtin_json_str_helper */

/**
 * Function to create a json formated string from an object
 *
 * @return ecma_value_t containing a json string
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_json_string_from_object (const ecma_value_t arg1) /**< object argument */
{
  ecma_json_stringify_context_t context;
  context.occurence_stack_last_p = NULL;
  context.indent_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
  context.property_list_p = ecma_new_values_collection ();
  context.replacer_function_p = NULL;
  context.gap_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);

  ecma_value_t ret_value = ecma_builtin_json_str_helper (arg1, context);

  ecma_deref_ecma_string (context.gap_str_p);
  ecma_deref_ecma_string (context.indent_str_p);
  ecma_free_values_collection (context.property_list_p, 0);
  return ret_value;
} /*ecma_builtin_json_string_from_object*/

/**
 * The JSON object's 'stringify' routine
 *
 * See also:
 *          ECMA-262 v5, 15.12.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_json_stringify (ecma_value_t this_arg, /**< 'this' argument */
                             ecma_value_t arg1,  /**< value */
                             ecma_value_t arg2,  /**< replacer */
                             ecma_value_t arg3)  /**< space */
{
  JERRY_UNUSED (this_arg);
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  ecma_json_stringify_context_t context;

  /* 1. */
  context.occurence_stack_last_p = NULL;

  /* 2. */
  context.indent_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);

  /* 3. */
  context.property_list_p = ecma_new_values_collection ();

  context.replacer_function_p = NULL;

  /* 4. */
  if (ecma_is_value_object (arg2))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (arg2);

    /* 4.a */
    if (ecma_op_is_callable (arg2))
    {
      context.replacer_function_p = obj_p;
    }
    /* 4.b */
    else if (ecma_object_get_class_name (obj_p) == LIT_MAGIC_STRING_ARRAY_UL)
    {
      ECMA_TRY_CATCH (array_length,
                      ecma_op_object_get_by_magic_id (obj_p, LIT_MAGIC_STRING_LENGTH),
                      ret_value);

      ECMA_OP_TO_NUMBER_TRY_CATCH (array_length_num,
                                   array_length,
                                   ret_value);

      uint32_t index = 0;

      /* 4.b.ii */
      while ((index < ecma_number_to_uint32 (array_length_num)) && ecma_is_value_empty (ret_value))
      {
        ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (index);

        ECMA_TRY_CATCH (value,
                        ecma_op_object_get (obj_p, index_str_p),
                        ret_value);

        /* 4.b.ii.1 */
        ecma_value_t item = ECMA_VALUE_UNDEFINED;

        /* 4.b.ii.2 */
        if (ecma_is_value_string (value))
        {
          item = ecma_copy_value (value);
        }
        /* 4.b.ii.3 */
        else if (ecma_is_value_number (value))
        {
          ECMA_TRY_CATCH (str_val,
                          ecma_op_to_string (value),
                          ret_value);

          item = ecma_copy_value (str_val);

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

            item = ecma_copy_value (val);

            ECMA_FINALIZE (val);
          }
        }

        /* 4.b.ii.5 */
        if (!ecma_is_value_undefined (item))
        {
          if (!ecma_has_string_value_in_collection (context.property_list_p, item))
          {
            ecma_append_to_values_collection (context.property_list_p, item, 0);
            ecma_deref_ecma_string (ecma_get_string_from_value (item));
          }
          else
          {
            ecma_free_value (item);
          }
        }

        ECMA_FINALIZE (value);

        ecma_deref_ecma_string (index_str_p);

        index++;
      }

      ECMA_OP_TO_NUMBER_FINALIZE (array_length_num);
      ECMA_FINALIZE (array_length);
    }
  }

  if (ecma_is_value_empty (ret_value))
  {
    ecma_value_t space = ecma_copy_value (arg3);

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

        ecma_free_value (space);
        space = ecma_copy_value (val);

        ECMA_FINALIZE (val);
      }
      /* 5.b */
      else if (class_name == LIT_MAGIC_STRING_STRING_UL)
      {
        ECMA_TRY_CATCH (val,
                        ecma_op_to_string (arg3),
                        ret_value);

        ecma_free_value (space);
        space = ecma_copy_value (val);

        ECMA_FINALIZE (val);
      }
    }

    if (ecma_is_value_empty (ret_value))
    {
      /* 6. */
      if (ecma_is_value_number (space))
      {
        ECMA_OP_TO_NUMBER_TRY_CATCH (array_length_num,
                                     arg3,
                                     ret_value);

        /* 6.a */
        int32_t num_of_spaces = ecma_number_to_int32 (array_length_num);
        num_of_spaces = (num_of_spaces > 10) ? 10 : num_of_spaces;

        /* 6.b */
        if (num_of_spaces < 1)
        {
          context.gap_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
        }
        else
        {
          JMEM_DEFINE_LOCAL_ARRAY (space_buff, num_of_spaces, char);

          for (int32_t i = 0; i < num_of_spaces; i++)
          {
            space_buff[i] = LIT_CHAR_SP;
          }

          context.gap_str_p = ecma_new_ecma_string_from_utf8 ((lit_utf8_byte_t *) space_buff,
                                                              (lit_utf8_size_t) num_of_spaces);

          JMEM_FINALIZE_LOCAL_ARRAY (space_buff);
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
          ecma_ref_ecma_string (space_str_p);
          context.gap_str_p = space_str_p;
        }
        else
        {
          context.gap_str_p = ecma_string_substr (space_str_p, 0, 10);
        }
      }
      /* 8. */
      else
      {
        context.gap_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
      }
    }

    ecma_free_value (space);

    if (ecma_is_value_empty (ret_value))
    {
      /* 9. */
      ret_value = ecma_builtin_json_str_helper (arg1, context);
    }

    ecma_deref_ecma_string (context.gap_str_p);
  }

  ecma_deref_ecma_string (context.indent_str_p);

  ecma_free_values_collection (context.property_list_p, 0);

  return ret_value;
} /* ecma_builtin_json_stringify */

/**
 * Abstract operation 'Quote' defined in 15.12.3
 *
 * See also:
 *          ECMA-262 v5, 15.12.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_json_quote (ecma_string_t *string_p) /**< string that should be quoted*/
{
  ECMA_STRING_TO_UTF8_STRING (string_p, string_buff, string_buff_size);
  const lit_utf8_byte_t *str_p = string_buff;
  const lit_utf8_byte_t *str_end_p = str_p + string_buff_size;
  size_t n_bytes = 2; /* Start with 2 for surrounding quotes. */

  while (str_p < str_end_p)
  {
    lit_utf8_byte_t c = *str_p++;

    if (c == LIT_CHAR_BACKSLASH || c == LIT_CHAR_DOUBLE_QUOTE)
    {
      n_bytes += 2;
    }
    else if (c >= LIT_CHAR_SP && c < LIT_UTF8_1_BYTE_CODE_POINT_MAX)
    {
      n_bytes++;
    }
    else if (c > LIT_UTF8_1_BYTE_CODE_POINT_MAX)
    {
      lit_utf8_size_t sz = lit_get_unicode_char_size_by_utf8_first_byte (c);
      n_bytes += sz;
      str_p += sz - 1;
    }
    else
    {
      switch (c)
      {
        case LIT_CHAR_BS:
        case LIT_CHAR_FF:
        case LIT_CHAR_LF:
        case LIT_CHAR_CR:
        case LIT_CHAR_TAB:
        {
          n_bytes += 2;
          break;
        }
        default: /* Hexadecimal. */
        {
          n_bytes += 2 + 4;
          break;
        }
      }
    }
  }

  lit_utf8_byte_t *buf_begin = jmem_heap_alloc_block (n_bytes);
  JERRY_ASSERT (buf_begin != NULL);
  lit_utf8_byte_t *buf = buf_begin;
  str_p = string_buff;

  *buf++ = LIT_CHAR_DOUBLE_QUOTE;

  while (str_p < str_end_p)
  {
    lit_utf8_byte_t c = *str_p++;

    if (c == LIT_CHAR_BACKSLASH || c == LIT_CHAR_DOUBLE_QUOTE)
    {
      *buf++ = LIT_CHAR_BACKSLASH;
      *buf++ = c;
    }
    else if (c >= LIT_CHAR_SP && c < LIT_UTF8_1_BYTE_CODE_POINT_MAX)
    {
      *buf++ = c;
    }
    else if (c > LIT_UTF8_1_BYTE_CODE_POINT_MAX)
    {
      str_p--;
      ecma_char_t current_char = lit_utf8_read_next (&str_p);
      buf += lit_code_unit_to_utf8 (current_char, (lit_utf8_byte_t *) buf);
    }
    else
    {
      switch (c)
      {
        case LIT_CHAR_BS:
        {
          *buf++ = LIT_CHAR_BACKSLASH;
          *buf++ = LIT_CHAR_LOWERCASE_B;
          break;
        }
        case LIT_CHAR_FF:
        {
          *buf++ = LIT_CHAR_BACKSLASH;
          *buf++ = LIT_CHAR_LOWERCASE_F;
          break;
        }
        case LIT_CHAR_LF:
        {
          *buf++ = LIT_CHAR_BACKSLASH;
          *buf++ = LIT_CHAR_LOWERCASE_N;
          break;
        }
        case LIT_CHAR_CR:
        {
          *buf++ = LIT_CHAR_BACKSLASH;
          *buf++ = LIT_CHAR_LOWERCASE_R;
          break;
        }
        case LIT_CHAR_TAB:
        {
          *buf++ = LIT_CHAR_BACKSLASH;
          *buf++ = LIT_CHAR_LOWERCASE_T;
          break;
        }
        default: /* Hexadecimal. */
        {
          JERRY_ASSERT (c < 0x9f);
          *buf++ = LIT_CHAR_BACKSLASH;
          *buf++ = LIT_CHAR_LOWERCASE_U;
          *buf++ = LIT_CHAR_0;
          *buf++ = LIT_CHAR_0;
          *buf++ = (lit_utf8_byte_t) (LIT_CHAR_0 + (c >> 4)); /* Max range 0-9, hex digits unnecessary. */
          lit_utf8_byte_t c2 = (c & 0xf);
          *buf++ = (lit_utf8_byte_t) (c2 + ((c2 <= 9) ? LIT_CHAR_0 : (LIT_CHAR_LOWERCASE_A - 10)));
          break;
        }
      }
    }
  }

  *buf++ = LIT_CHAR_DOUBLE_QUOTE;

  /* Make sure we didn't run off the end or allocated more than we actually wanted. */
  JERRY_ASSERT ((size_t) (buf - buf_begin) == n_bytes);

  ecma_string_t *product_str_p = ecma_new_ecma_string_from_utf8 ((const lit_utf8_byte_t *) buf_begin,
                                                                 (lit_utf8_size_t) (buf - buf_begin));
  jmem_heap_free_block (buf_begin, n_bytes);
  ECMA_FINALIZE_UTF8_STRING (string_buff, string_buff_size);

  return ecma_make_string_value (product_str_p);
} /* ecma_builtin_json_quote */

/**
 * Abstract operation 'Str' defined in 15.12.3
 *
 * See also:
 *          ECMA-262 v5, 15.12.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_json_str (ecma_string_t *key_p, /**< property key*/
                       ecma_object_t *holder_p, /**< the object*/
                       ecma_json_stringify_context_t *context_p) /**< context*/
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  /* 1. */
  ECMA_TRY_CATCH (value,
                  ecma_op_object_get (holder_p, key_p),
                  ret_value);

  ecma_value_t my_val = ecma_copy_value (value);

  /* 2. */
  if (ecma_is_value_object (my_val))
  {
    ecma_object_t *value_obj_p = ecma_get_object_from_value (my_val);

    /* 2.a */
    ECMA_TRY_CATCH (toJSON,
                    ecma_op_object_get_by_magic_id (value_obj_p, LIT_MAGIC_STRING_TO_JSON_UL),
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

      ecma_free_value (my_val);
      my_val = ecma_copy_value (func_ret_val);

      ECMA_FINALIZE (func_ret_val);
    }

    ECMA_FINALIZE (toJSON);
  }

  /* 3. */
  if (context_p->replacer_function_p && ecma_is_value_empty (ret_value))
  {
    ecma_value_t holder_value = ecma_make_object_value (holder_p);
    ecma_value_t key_value = ecma_make_string_value (key_p);
    ecma_value_t call_args[] = { key_value, my_val };

    ECMA_TRY_CATCH (func_ret_val,
                    ecma_op_function_call (context_p->replacer_function_p, holder_value, call_args, 2),
                    ret_value);

    ecma_free_value (my_val);
    my_val = ecma_copy_value (func_ret_val);

    ECMA_FINALIZE (func_ret_val);
  }

  /* 4. */
  if (ecma_is_value_object (my_val) && ecma_is_value_empty (ret_value))
  {
    ecma_object_t *obj_p = ecma_get_object_from_value (my_val);
    lit_magic_string_id_t class_name = ecma_object_get_class_name (obj_p);

    /* 4.a */
    if (class_name == LIT_MAGIC_STRING_NUMBER_UL)
    {
      ECMA_TRY_CATCH (val,
                      ecma_op_to_number (my_val),
                      ret_value);

      ecma_free_value (my_val);
      my_val = ecma_copy_value (val);

      ECMA_FINALIZE (val);
    }
    /* 4.b */
    else if (class_name == LIT_MAGIC_STRING_STRING_UL)
    {
      ECMA_TRY_CATCH (val,
                      ecma_op_to_string (my_val),
                      ret_value);

      ecma_free_value (my_val);
      my_val = ecma_copy_value (val);

      ECMA_FINALIZE (val);
    }
    /* 4.c */
    else if (class_name == LIT_MAGIC_STRING_BOOLEAN_UL)
    {
      ECMA_TRY_CATCH (val,
                      ecma_op_to_primitive (my_val, ECMA_PREFERRED_TYPE_NO),
                      ret_value);

      ecma_free_value (my_val);
      my_val = ecma_copy_value (val);

      ECMA_FINALIZE (val);
    }
  }

  if (ecma_is_value_empty (ret_value))
  {
    /* 5. - 7. */
    if (ecma_is_value_null (my_val) || ecma_is_value_boolean (my_val))
    {
      ret_value = ecma_op_to_string (my_val);
      JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (ret_value));
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
      ecma_number_t num_value_p = ecma_get_number_from_value (my_val);

      /* 9.a */
      if (!ecma_number_is_nan (num_value_p) && !ecma_number_is_infinity (num_value_p))
      {
        ret_value = ecma_op_to_string (my_val);
        JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (ret_value));
      }
      else
      {
        /* 9.b */
        ret_value = ecma_make_magic_string_value (LIT_MAGIC_STRING_NULL);
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

        ret_value = ecma_copy_value (val);

        ECMA_FINALIZE (val);
      }
      /* 10.b */
      else
      {
        ECMA_TRY_CATCH (val,
                        ecma_builtin_json_object (obj_p, context_p),
                        ret_value);

        ret_value = ecma_copy_value (val);

        ECMA_FINALIZE (val);
      }
    }
    else
    {
      /* 11. */
      ret_value = ECMA_VALUE_UNDEFINED;
    }
  }

  ecma_free_value (my_val);
  ECMA_FINALIZE (value);

  return ret_value;
} /* ecma_builtin_json_str */

/**
 * Abstract operation 'JO' defined in 15.12.3
 *
 * See also:
 *          ECMA-262 v5, 15.12.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_json_object (ecma_object_t *obj_p, /**< the object*/
                          ecma_json_stringify_context_t *context_p) /**< context*/
{
  /* 1. */
  if (ecma_json_has_object_in_stack (context_p->occurence_stack_last_p, obj_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("The structure is cyclical."));
  }

  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  /* 2. */
  ecma_json_occurence_stack_item_t stack_item;
  stack_item.next_p = context_p->occurence_stack_last_p;
  stack_item.object_p = obj_p;
  context_p->occurence_stack_last_p = &stack_item;

  /* 3. */
  ecma_string_t *stepback_p = context_p->indent_str_p;

  /* 4. */
  ecma_ref_ecma_string (stepback_p);
  context_p->indent_str_p = ecma_concat_ecma_strings (stepback_p, context_p->gap_str_p);

  ecma_collection_header_t *property_keys_p;

  /* 5. */
  if (context_p->property_list_p->item_count > 0)
  {
    property_keys_p = context_p->property_list_p;
  }
  /* 6. */
  else
  {
    property_keys_p = ecma_new_values_collection ();

    ecma_collection_header_t *props_p = ecma_op_object_get_property_names (obj_p, ECMA_LIST_ENUMERABLE);

    ecma_value_t *ecma_value_p = ecma_collection_iterator_init (props_p);

    while (ecma_value_p != NULL)
    {
      ecma_string_t *property_name_p = ecma_get_string_from_value (*ecma_value_p);

      ecma_property_t property = ecma_op_object_get_own_property (obj_p,
                                                                  property_name_p,
                                                                  NULL,
                                                                  ECMA_PROPERTY_GET_NO_OPTIONS);

      JERRY_ASSERT (ecma_is_property_enumerable (property));

      if (ECMA_PROPERTY_GET_TYPE (property) != ECMA_PROPERTY_TYPE_SPECIAL)
      {
        ecma_append_to_values_collection (property_keys_p, *ecma_value_p, 0);
      }

      ecma_value_p = ecma_collection_iterator_next (ecma_value_p);
    }

    ecma_free_values_collection (props_p, 0);
  }

  /* 7. */
  ecma_collection_header_t *partial_p = ecma_new_values_collection ();

  /* 8. */
  ecma_value_t *ecma_value_p = ecma_collection_iterator_init (property_keys_p);

  while (ecma_value_p != NULL && ecma_is_value_empty (ret_value))
  {
    ecma_string_t *key_p = ecma_get_string_from_value (*ecma_value_p);
    ecma_value_p = ecma_collection_iterator_next (ecma_value_p);

    /* 8.a */
    ECMA_TRY_CATCH (str_val,
                    ecma_builtin_json_str (key_p, obj_p, context_p),
                    ret_value);

    /* 8.b */
    if (!ecma_is_value_undefined (str_val))
    {
      ecma_string_t *value_str_p = ecma_get_string_from_value (str_val);

      /* 8.b.i */
      ecma_value_t str_comp_val = ecma_builtin_json_quote (key_p);
      JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (str_comp_val));

      ecma_string_t *member_str_p = ecma_get_string_from_value (str_comp_val);

      /* 8.b.ii */
      member_str_p = ecma_append_magic_string_to_string (member_str_p, LIT_MAGIC_STRING_COLON_CHAR);

      /* 8.b.iii */
      if (!ecma_string_is_empty (context_p->gap_str_p))
      {
        member_str_p = ecma_append_magic_string_to_string (member_str_p, LIT_MAGIC_STRING_SPACE_CHAR);
      }

      /* 8.b.iv */
      member_str_p = ecma_concat_ecma_strings (member_str_p, value_str_p);

      /* 8.b.v */
      ecma_value_t member_value = ecma_make_string_value (member_str_p);
      ecma_append_to_values_collection (partial_p, member_value, 0);
      ecma_deref_ecma_string (member_str_p);
    }

    ECMA_FINALIZE (str_val);
  }

  if (context_p->property_list_p->item_count == 0)
  {
    ecma_free_values_collection (property_keys_p, 0);
  }

  if (ecma_is_value_empty (ret_value))
  {
    /* 9. */
    if (partial_p->item_count == 0)
    {
      lit_utf8_byte_t chars[2] = { LIT_CHAR_LEFT_BRACE, LIT_CHAR_RIGHT_BRACE };

      ecma_string_t *final_str_p = ecma_new_ecma_string_from_utf8 (chars, 2);
      ret_value = ecma_make_string_value (final_str_p);
    }
    /* 10. */
    else
    {
      /* 10.a */
      if (ecma_string_is_empty (context_p->gap_str_p))
      {
        ret_value = ecma_builtin_helper_json_create_non_formatted_json (LIT_CHAR_LEFT_BRACE,
                                                                        LIT_CHAR_RIGHT_BRACE,
                                                                        partial_p);
      }
      /* 10.b */
      else
      {
        ret_value = ecma_builtin_helper_json_create_formatted_json (LIT_CHAR_LEFT_BRACE,
                                                                    LIT_CHAR_RIGHT_BRACE,
                                                                    stepback_p,
                                                                    partial_p,
                                                                    context_p);
      }
    }
  }

  ecma_free_values_collection (partial_p, 0);

  /* 11. */
  context_p->occurence_stack_last_p = stack_item.next_p;

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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_json_array (ecma_object_t *obj_p, /**< the array object*/
                         ecma_json_stringify_context_t *context_p) /**< context*/
{
  JERRY_ASSERT (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_ARRAY);

  /* 1. */
  if (ecma_json_has_object_in_stack (context_p->occurence_stack_last_p, obj_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("The structure is cyclical."));
  }

  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  /* 2. */
  ecma_json_occurence_stack_item_t stack_item;
  stack_item.next_p = context_p->occurence_stack_last_p;
  stack_item.object_p = obj_p;
  context_p->occurence_stack_last_p = &stack_item;

  /* 3. */
  ecma_string_t *stepback_p = context_p->indent_str_p;

  /* 4. */
  ecma_ref_ecma_string (stepback_p);
  context_p->indent_str_p = ecma_concat_ecma_strings (stepback_p, context_p->gap_str_p);

  /* 5. */
  ecma_collection_header_t *partial_p = ecma_new_values_collection ();

  /* 6. */
  uint32_t array_length = ((ecma_extended_object_t *) obj_p)->u.array.length;

  /* 7. - 8. */
  for (uint32_t index = 0; index < array_length && ecma_is_value_empty (ret_value); index++)
  {

    /* 8.a */
    ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (index);

    ECMA_TRY_CATCH (str_val,
                    ecma_builtin_json_str (index_str_p, obj_p, context_p),
                    ret_value);

    /* 8.b */
    if (ecma_is_value_undefined (str_val))
    {
      ecma_append_to_values_collection (partial_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_NULL), 0);
    }
    /* 8.c */
    else
    {
      ecma_append_to_values_collection (partial_p, str_val, 0);
    }

    ECMA_FINALIZE (str_val);
    ecma_deref_ecma_string (index_str_p);
  }

  if (ecma_is_value_empty (ret_value))
  {
    /* 9. */
    if (partial_p->item_count == 0)
    {
      lit_utf8_byte_t chars[2] = { LIT_CHAR_LEFT_SQUARE, LIT_CHAR_RIGHT_SQUARE };

      ecma_string_t *final_str_p = ecma_new_ecma_string_from_utf8 (chars, 2);
      ret_value = ecma_make_string_value (final_str_p);
    }
    /* 10. */
    else
    {
      /* 10.a */
      if (ecma_string_is_empty (context_p->gap_str_p))
      {
        ret_value = ecma_builtin_helper_json_create_non_formatted_json (LIT_CHAR_LEFT_SQUARE,
                                                                        LIT_CHAR_RIGHT_SQUARE,
                                                                        partial_p);
      }
      /* 10.b */
      else
      {
        ret_value = ecma_builtin_helper_json_create_formatted_json (LIT_CHAR_LEFT_SQUARE,
                                                                    LIT_CHAR_RIGHT_SQUARE,
                                                                    stepback_p,
                                                                    partial_p,
                                                                    context_p);
      }
    }
  }

  ecma_free_values_collection (partial_p, 0);

  /* 11. */
  context_p->occurence_stack_last_p = stack_item.next_p;

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

#endif /* ENABLED (JERRY_BUILTIN_JSON) */
