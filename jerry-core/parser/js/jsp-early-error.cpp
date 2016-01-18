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

#include "jsp-early-error.h"
#include "stack.h"
#include "jrt.h"
#include "parser.h"
#include "jrt-libc-includes.h"
#include "ecma-helpers.h"
#include "lit-magic-strings.h"

/**
 * Early error longjmp label, used to finish parse upon an early error occurence
 *
 * See also:
 *          jsp_early_error_get_early_error_longjmp_label
 *          jsp_early_error_raise_error
 */
static jmp_buf jsp_early_error_label;

/**
 * Type of early error occured, or JSP_EARLY_ERROR__NO_ERROR
 */
jsp_early_error_t jsp_early_error_type;

typedef struct
{
  prop_type type;
  lit_literal_t lit;
} prop_literal;

enum
{
  props_global_size
};
STATIC_STACK (props, prop_literal)

enum
{
  size_t_stack_global_size
};
STATIC_STACK (size_t_stack, size_t)

/**
 * Get buffer for SyntaxError longjmp label
 *
 * @return pointer to jmp_buf
 */
jmp_buf *
jsp_early_error_get_early_error_longjmp_label (void)
{
  return &jsp_early_error_label;
} /* jsp_early_error_get_early_error_longjmp_label */

/**
 * Raise an early error of specified type
 *
 * Note:
 *      Performs longjmp to early error longjmp label
 *
 * See also:
 *          parser_parse_program
 */
void __attribute__((noreturn))
jsp_early_error_raise_error (jsp_early_error_t type) /**< type of error to raise */
{
  JERRY_ASSERT (jsp_early_error_type == JSP_EARLY_ERROR__NO_ERROR);

  jsp_early_error_type = type;

  longjmp (jsp_early_error_label, 1);
} /* jsp_early_error_raise_error */

/**
 * Get type of occured early error
 *
 * @return type
 */
jsp_early_error_t
jsp_early_error_get_type (void)
{
  JERRY_ASSERT (jsp_early_error_type != JSP_EARLY_ERROR__NO_ERROR);

  return jsp_early_error_type;
} /* jsp_early_error_get_type */

static prop_literal
create_prop_literal (lit_literal_t lit, prop_type type)
{
  prop_literal ret;

  ret.type = type;
  ret.lit = lit;

  return ret;
}

void
jsp_early_error_start_checking_of_prop_names (void)
{
  STACK_PUSH (size_t_stack, STACK_SIZE (props));
}

void
jsp_early_error_add_prop_name (jsp_operand_t op, prop_type pt)
{
  JERRY_ASSERT (jsp_is_string_lit_operand (op));
  STACK_PUSH (props, create_prop_literal (lit_get_literal_by_cp (jsp_operand_get_literal (op)), pt));
}

void
jsp_early_error_check_for_duplication_of_prop_names (bool is_strict, locus loc __attr_unused___)
{
  if (STACK_SIZE (props) - STACK_TOP (size_t_stack) < 2)
  {
    STACK_DROP (props, (size_t) (STACK_SIZE (props) - STACK_TOP (size_t_stack)));
    STACK_DROP (size_t_stack, 1);
    return;
  }

  for (size_t i = (STACK_TOP (size_t_stack) + 1);
       i < STACK_SIZE (props);
       i++)
  {
    const prop_literal previous = STACK_ELEMENT (props, i);
    JERRY_ASSERT (previous.type == PROP_DATA
                  || previous.type == PROP_GET
                  || previous.type == PROP_SET);

    for (size_t j = STACK_TOP (size_t_stack); j < i; j = j + 1)
    {
      /*4*/
      const prop_literal current = STACK_ELEMENT (props, j);
      JERRY_ASSERT (current.type == PROP_DATA
                    || current.type == PROP_GET
                    || current.type == PROP_SET);

      if (lit_literal_equal (previous.lit, current.lit))
      {
        /*a*/
        if (is_strict && previous.type == PROP_DATA && current.type == PROP_DATA)
        {
          PARSE_ERROR_VARG (JSP_EARLY_ERROR_SYNTAX,
                            "Duplication of parameter name '%s' in ObjectDeclaration is not allowed in strict mode",
                            loc, lit_literal_to_str_internal_buf (current.lit));
        }
        /*b*/
        if (previous.type == PROP_DATA
            && (current.type == PROP_SET || current.type == PROP_GET))
        {
          PARSE_ERROR_VARG (JSP_EARLY_ERROR_SYNTAX,
                            "Parameter name '%s' in ObjectDeclaration may not be both data and accessor",
                            loc, lit_literal_to_str_internal_buf (current.lit));
        }
        /*c*/
        if (current.type == PROP_DATA
            && (previous.type == PROP_SET || previous.type == PROP_GET))
        {
          PARSE_ERROR_VARG (JSP_EARLY_ERROR_SYNTAX,
                            "Parameter name '%s' in ObjectDeclaration may not be both data and accessor",
                            loc, lit_literal_to_str_internal_buf (current.lit));
        }
        /*d*/
        if ((previous.type == PROP_SET && current.type == PROP_SET)
            || (previous.type == PROP_GET && current.type == PROP_GET))
        {
          PARSE_ERROR_VARG (JSP_EARLY_ERROR_SYNTAX,
                            "Parameter name '%s' in ObjectDeclaration may not be accessor of same type",
                            loc, lit_literal_to_str_internal_buf (current.lit));
        }
      }
    }
  }

  STACK_DROP (props, (size_t) (STACK_SIZE (props) - STACK_TOP (size_t_stack)));
  STACK_DROP (size_t_stack, 1);
}

void
jsp_early_error_emit_error_on_eval_and_arguments (lit_literal_t lit, /**< literal to check */
                                                  locus loc) /**< location of the literal in source code */
{
  if (lit_literal_equal_type_utf8 (lit,
                                   lit_get_magic_string_utf8 (LIT_MAGIC_STRING_ARGUMENTS),
                                   lit_get_magic_string_size (LIT_MAGIC_STRING_ARGUMENTS))
      || lit_literal_equal_type_utf8 (lit,
                                      lit_get_magic_string_utf8 (LIT_MAGIC_STRING_EVAL),
                                      lit_get_magic_string_size (LIT_MAGIC_STRING_EVAL)))
  {
    PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX, "'eval' and 'arguments' are not allowed here in strict mode", loc);
  }
}

void
jsp_early_error_check_for_eval_and_arguments_in_strict_mode (jsp_operand_t op, bool is_strict, locus loc)
{
  if (is_strict)
  {
    lit_cpointer_t lit_cp;

    if (jsp_is_string_lit_operand (op)
        || jsp_is_number_lit_operand (op))
    {
      lit_cp = jsp_operand_get_literal (op);
    }
    else if (jsp_is_identifier_operand (op))
    {
      lit_cp = jsp_operand_get_identifier_name (op);
    }
    else
    {
      return;
    }

    jsp_early_error_emit_error_on_eval_and_arguments (lit_get_literal_by_cp (lit_cp), loc);
  }
}

void
jsp_early_error_check_delete (bool is_strict, locus loc __attr_unused___)
{
  if (is_strict)
  {
    PARSE_ERROR (JSP_EARLY_ERROR_SYNTAX, "'delete' operator shall not apply on identifier in strict mode.", loc);
  }
}

void
jsp_early_error_init (void)
{
  jsp_early_error_type = JSP_EARLY_ERROR__NO_ERROR;

  STACK_INIT (props);
  STACK_INIT (size_t_stack);
}

void
jsp_early_error_free (void)
{
  STACK_FREE (size_t_stack);
  STACK_FREE (props);
}

