/* Copyright 2014 Samsung Electronics Co., Ltd.
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

#ifndef PARSER_H
#define PARSER_H

#include "globals.h"

struct source_element_list;
struct statement_list;
struct statement;
struct assignment_expression;
struct member_expression;

#define MAX_PARAMS 5
#define MAX_EXPRS 2
#define MAX_PROPERTIES 5
#define MAX_DECLS 5
#define MAX_SUFFIXES 2

/** Represents list of parameters.  */
typedef struct formal_parameter_list
{
  /** Identifiers of a parameter. Next after last parameter is NULL.  */
  const char *names[MAX_PARAMS];
}
formal_parameter_list;

static const formal_parameter_list
empty_formal_parameter_list = 
{
  .names = { [0] = NULL }
};

bool is_formal_parameter_list_empty (formal_parameter_list);

/** @function_declaration represents both declaration and expression of a function. 
    After this parser must return a block of statements.  */
typedef struct
{
  /** Identifier: name of a function. Can be NULL for anonimous functions.  */
  const char *name;
  /** List of parameter of a function. Can be NULL.  */
  formal_parameter_list params;
}
function_declaration;

typedef function_declaration function_expression;

/** Types of literals: null, bool, decimal and string. 
    Decimal type is represented by LIT_INT and supports only double-word sized integers.  */
typedef enum
{
  LIT_NULL,
  LIT_BOOL,
  LIT_INT,
  LIT_STR
}
literal_type;

/** Represents different literals, contains a data of them.  */ 
typedef struct
{
  /** Type of a literal.  */
  literal_type type;

  /** Value of a literal.  */
  union
    {
      /** Used by null literal, always NULL.  */
      void *none;
      /** String literal value.  */
      const char *str;
      /** Number value.  */
      int num;
      /** Boolean value.  */
      bool is_true;
    }
  data;
}
literal;

typedef struct
{
  bool is_literal;

  union
    {
      void *none;
      literal lit;
      const char *name;
    }
  data;
}
operand;

typedef operand property_name;

static const operand 
empty_operand = 
{ 
  .is_literal = false, 
  .data.none = NULL 
};

bool is_operand_empty (operand);

typedef struct
{
  operand op1, op2;
}
operand_pair;

typedef struct
{
  operand ops[MAX_PARAMS];
}
operand_list;

static const operand_list 
empty_operand_list = 
{
  .ops = { [0] = { .is_literal = false, .data.none = NULL } }
};

bool is_operand_list_empty (operand_list);

typedef operand_list array_literal;
typedef operand_list argument_list;

typedef struct
{
  const char *name;
  argument_list args;
}
call_expression;

/** Represents a single property.  */
typedef struct
{
  /** Name of property.  */
  property_name name;
  /** Value of property.  */
  operand value;
}
property;

static const property empty_property = 
{
  .name = { .is_literal = false, .data.none = NULL },
  .value = { .is_literal = false, .data.none = NULL }
};

bool is_property_empty (property);

/** List of properties. Represents ObjectLiteral.  */
typedef struct
{
  /** Properties.  */
  property props[MAX_PROPERTIES];
}
property_list;

static const property_list
empty_property_list =
{
  .props = 
    { [0] = 
      { .name = 
          { .is_literal = false, .data.none = NULL }, 
        .value = 
          { .is_literal = false, .data.none = NULL }}}
};

bool is_property_list_empty (property_list);

typedef property_list object_literal;

typedef enum
{
  AO_NONE,
  AO_EQ,
  AO_MULT_EQ,
  AO_DIV_EQ,
  AO_MOD_EQ,
  AO_PLUS_EQ,
  AO_MINUS_EQ,
  AO_LSHIFT_EQ,
  AO_RSHIFT_EQ,
  AO_RSHIFT_EX_EQ,
  AO_AND_EQ,
  AO_XOR_EQ,
  AO_OR_EQ
}
assignment_operator;

typedef enum
{
  ET_NONE,
  ET_LOGICAL_OR,
  ET_LOGICAL_AND,
  ET_BITWISE_OR,
  ET_BITWISE_XOR,
  ET_BITWISE_AND,
  ET_DOUBLE_EQ,
  ET_NOT_EQ,
  ET_TRIPLE_EQ,
  ET_NOT_DOUBLE_EQ,
  ET_LESS,
  ET_GREATER,
  ET_LESS_EQ,
  ET_GREATER_EQ,
  ET_INSTANCEOF,
  ET_IN,
  ET_LSHIFT,
  ET_RSHIFT,
  ET_RSHIFT_EX,
  ET_PLUS,
  ET_MINUS,
  ET_MULT,
  ET_DIV,
  ET_MOD,
  ET_UNARY_DELETE,
  ET_UNARY_VOID,
  ET_UNARY_TYPEOF,
  ET_UNARY_INCREMENT,
  ET_UNARY_DECREMENT,
  ET_UNARY_PLUS,
  ET_UNARY_MINUS,
  ET_UNARY_COMPL,
  ET_UNARY_NOT,
  ET_POSTFIX_INCREMENT,
  ET_POSTFIX_DECREMENT,
  ET_CALL,
  ET_NEW,
  ET_INDEX,
  ET_PROP_REF,
  ET_OBJECT,
  ET_FUNCTION,
  ET_ARRAY,
  ET_SUBEXPRESSION,
  ET_LITERAL,
  ET_IDENTIFIER
}
expression_type;

typedef struct
{
  assignment_operator oper;
  expression_type type;

  /** NUllable.  */
  const char *var;

  union
    {
      void *none;
      operand_pair ops;
      call_expression call_expr;
      array_literal arr_lit;
      object_literal obj_lit;
      function_expression func_expr;
    }
  data;
}
assignment_expression;

static const assignment_expression 
empty_expression = 
{
  .oper = AO_NONE,
  .type = ET_NONE,
  .data.none = NULL
};

bool is_expression_empty (assignment_expression);

/** Represents expression, array literal and list of argument.  */
typedef struct
{
  /** Single assignment expression. Cannot be NULL for expression and list of arguments.
      But can be NULL for array literal.  */
  assignment_expression exprs[MAX_EXPRS];
}
expression_list;

typedef expression_list expression;

/* Statements.  */

typedef struct
{
  const char *name;
  assignment_expression assign_expr;
}
variable_declaration;

static const variable_declaration
empty_variable_declaration = 
{
  .name = NULL,
  .assign_expr = { .oper = AO_NONE, .type = ET_NONE, .data.none = NULL }
};

bool is_variable_declaration_empty (variable_declaration);

typedef struct
{
  variable_declaration decls[MAX_DECLS];
}
variable_declaration_list;

typedef struct
{
  bool is_decl;

  union
    {
      expression expr;
      variable_declaration_list decl_list;
    }
  data;
}
for_statement_initialiser_part;

typedef struct
{
  for_statement_initialiser_part init;
  assignment_expression limit, incr;
}
for_statement;

typedef struct
{
  bool is_decl;

  union
    {
      assignment_expression left_hand_expr;
      variable_declaration decl;
    }
  data;
}
for_in_statement_initializer_part;

typedef struct
{
  for_in_statement_initializer_part init;
  expression list_expr;
}
for_in_statement;

typedef struct
{
  bool is_for_in;

  union
    {
      for_statement for_stmt;
      for_in_statement for_in_stmt;
    }
  data;
}
for_or_for_in_statement;

typedef enum
{
  STMT_NULL,
  STMT_BLOCK_START,
  STMT_BLOCK_END,
  STMT_VARIABLE,
  STMT_EMPTY,
  STMT_IF,
  STMT_ELSE,
  STMT_ELSE_IF,
  STMT_DO,

  STMT_WHILE,
  STMT_FOR_OR_FOR_IN,
  STMT_CONTINUE,
  STMT_BREAK,

  STMT_RETURN,
  STMT_WITH,
  STMT_LABELLED,
  STMT_SWITCH,
  STMT_CASE,
  STMT_THROW,

  STMT_TRY,
  STMT_CATCH,
  STMT_FINALLY,
  STMT_EXPRESSION,
  STMT_SUBEXPRESSION_END,
  STMT_FUNCTION,
  STMT_EOF
}
statement_type;

typedef struct statement
{
  statement_type type;

  union
    {
      void *none;
      variable_declaration_list var_stmt;
      expression expr;
      for_or_for_in_statement for_stmt;
      const char *name;
      function_declaration fun_decl;
    }
  data;
}
statement;

static const statement
null_statement = 
{
  .type = STMT_NULL,
  .data.none = NULL
};

bool is_statement_null (statement);

void parser_init (void);
statement parser_parse_statement (void);

void parser_fatal (jerry_Status_t code);

#endif
