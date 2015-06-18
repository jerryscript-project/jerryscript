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

#include "syntax-errors.h"
#include "stack.h"
#include "jrt.h"
#include "parser.h"
#include "jrt-libc-includes.h"
#include "ecma-helpers.h"

/**
 * SyntaxError longjmp label, used to finish parse upon a SyntaxError is raised
 *
 * See also:
 *          syntax_get_syntax_error_longjmp_label
 *          syntax_raise_error
 */
static jmp_buf jsp_syntax_error_label;

typedef struct
{
  prop_type type;
  literal_t lit;
} prop_literal;

enum
{
  props_global_size
};
STATIC_STACK (props, prop_literal)

enum
{
  U8_global_size
};
STATIC_STACK (U8, uint8_t)

/**
 * Get buffer for SyntaxError longjmp label
 *
 * @return pointer to jmp_buf
 */
jmp_buf *
syntax_get_syntax_error_longjmp_label (void)
{
  return &jsp_syntax_error_label;
} /* syntax_get_syntax_error_longjmp_label */

/**
 * Raise SyntaxError, i.e. perform longjmp to SyntaxError longjmp label
 */
void __attribute__((noreturn))
syntax_raise_error (void)
{
  longjmp (jsp_syntax_error_label, 1);
} /* syntax_raise_error */

static prop_literal
create_prop_literal (literal_t lit, prop_type type)
{
  prop_literal ret;

  ret.type = type;
  ret.lit = lit;

  return ret;
}

void
syntax_start_checking_of_prop_names (void)
{
  STACK_PUSH (U8, (uint8_t) STACK_SIZE (props));
}

void
syntax_add_prop_name (operand op, prop_type pt)
{
  JERRY_ASSERT (op.type == OPERAND_LITERAL);
  STACK_PUSH (props, create_prop_literal (lit_get_literal_by_cp (op.data.lit_id), pt));
}

void
syntax_check_for_duplication_of_prop_names (bool is_strict, locus loc __attr_unused___)
{
  if (STACK_SIZE (props) - STACK_TOP (U8) < 2)
  {
    STACK_DROP (U8, 1);
    return;
  }

  for (uint8_t i = (uint8_t) (STACK_TOP (U8) + 1);
       i < STACK_SIZE (props);
       i++)
  {
    const prop_literal previous = STACK_ELEMENT (props, i);
    if (previous.type == VARG)
    {
      continue;
    }
    JERRY_ASSERT (previous.type == PROP_DATA
                  || previous.type == PROP_GET
                  || previous.type == PROP_SET);
    for (uint8_t j = STACK_TOP (U8); j < i; j = (uint8_t) (j + 1))
    {
      /*4*/
      const prop_literal current = STACK_ELEMENT (props, j);
      if (current.type == VARG)
      {
        continue;
      }
      JERRY_ASSERT (current.type == PROP_DATA
                    || current.type == PROP_GET
                    || current.type == PROP_SET);
      if (lit_literal_equal (previous.lit, current.lit))
      {
        /*a*/
        if (is_strict && previous.type == PROP_DATA && current.type == PROP_DATA)
        {
          PARSE_ERROR_VARG ("Duplication of parameter name '%s' in ObjectDeclaration is not allowed in strict mode",
                            loc, lit_literal_to_str_internal_buf (current.lit));
        }
        /*b*/
        if (previous.type == PROP_DATA
            && (current.type == PROP_SET || current.type == PROP_GET))
        {
          PARSE_ERROR_VARG ("Parameter name '%s' in ObjectDeclaration may not be both data and accessor",
                            loc, lit_literal_to_str_internal_buf (current.lit));
        }
        /*c*/
        if (current.type == PROP_DATA
            && (previous.type == PROP_SET || previous.type == PROP_GET))
        {
          PARSE_ERROR_VARG ("Parameter name '%s' in ObjectDeclaration may not be both data and accessor",
                            loc, lit_literal_to_str_internal_buf (current.lit));
        }
        /*d*/
        if ((previous.type == PROP_SET && current.type == PROP_SET)
            || (previous.type == PROP_GET && current.type == PROP_GET))
        {
          PARSE_ERROR_VARG ("Parameter name '%s' in ObjectDeclaration may not be accessor of same type",
                            loc, lit_literal_to_str_internal_buf (current.lit));
        }
      }
    }
  }

  STACK_DROP (props, (uint8_t) (STACK_SIZE (props) - STACK_TOP (U8)));
  STACK_DROP (U8, 1);
}

void
syntax_start_checking_of_vargs (void)
{
  STACK_PUSH (U8, (uint8_t) STACK_SIZE (props));
}

void syntax_add_varg (operand op)
{
  JERRY_ASSERT (op.type == OPERAND_LITERAL);
  STACK_PUSH (props, create_prop_literal (lit_get_literal_by_cp (op.data.lit_id), VARG));
}

static void
emit_error_on_eval_and_arguments (operand op, locus loc __attr_unused___)
{
  if (op.type == OPERAND_LITERAL)
  {
    if (lit_literal_equal_type_zt (lit_get_literal_by_cp (op.data.lit_id),
                                   ecma_get_magic_string_zt (ECMA_MAGIC_STRING_ARGUMENTS))
        || lit_literal_equal_type_zt (lit_get_literal_by_cp (op.data.lit_id),
                                      ecma_get_magic_string_zt (ECMA_MAGIC_STRING_EVAL)))
    {
      PARSE_ERROR ("'eval' and 'arguments' are not allowed here in strict mode", loc);
    }
  }
}

void
syntax_check_for_eval_and_arguments_in_strict_mode (operand op, bool is_strict, locus loc)
{
  if (is_strict)
  {
    emit_error_on_eval_and_arguments (op, loc);
  }
}

/* 13.1, 15.3.2 */
void
syntax_check_for_syntax_errors_in_formal_param_list (bool is_strict, locus loc __attr_unused___)
{
  if (STACK_SIZE (props) - STACK_TOP (U8) < 2 || !is_strict)
  {
    STACK_DROP (U8, 1);
    return;
  }
  for (uint8_t i = (uint8_t) (STACK_TOP (U8) + 1); i < STACK_SIZE (props); i = (uint8_t) (i + 1))
  {
    JERRY_ASSERT (STACK_ELEMENT (props, i).type == VARG);
    literal_t previous = STACK_ELEMENT (props, i).lit;
    JERRY_ASSERT (previous->get_type () == LIT_STR_T
                  || previous->get_type () == LIT_MAGIC_STR_T
                  || previous->get_type () == LIT_MAGIC_STR_EX_T);
    for (uint8_t j = STACK_TOP (U8); j < i; j = (uint8_t) (j + 1))
    {
      JERRY_ASSERT (STACK_ELEMENT (props, j).type == VARG);
      literal_t current = STACK_ELEMENT (props, j).lit;
      JERRY_ASSERT (current->get_type () == LIT_STR_T
                    || current->get_type () == LIT_MAGIC_STR_T
                    || current->get_type () == LIT_MAGIC_STR_EX_T);
      if (lit_literal_equal_type (previous, current))
      {
        PARSE_ERROR_VARG ("Duplication of literal '%s' in FormalParameterList is not allowed in strict mode",
                          loc, lit_literal_to_str_internal_buf (previous));
      }
    }
  }

  STACK_DROP (props, (uint8_t) (STACK_SIZE (props) - STACK_TOP (U8)));
  STACK_DROP (U8, 1);
}

void
syntax_check_delete (bool is_strict, locus loc __attr_unused___)
{
  if (is_strict)
  {
    PARSE_ERROR ("'delete' operator shall not apply on identifier in strict mode.", loc);
  }
}

void
syntax_init (void)
{
  STACK_INIT (props);
  STACK_INIT (U8);
}

void
syntax_free (void)
{
  STACK_FREE (U8);
  STACK_FREE (props);
}

