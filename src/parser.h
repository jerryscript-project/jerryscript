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

#include <stdio.h>
#include <stdbool.h>

struct source_element_list;
struct statement_list;
struct statement;
struct assignment_expression;
struct member_expression;

/** Represents list of parameters.  */
typedef struct formal_parameter_list
{
  /** Identifier of a parameter. Cannot be NULL.  */
  const char *name;
  /** Next parameter: can be NULL.  */
  struct formal_parameter_list *next;
}
formal_parameter_list;

/** @function_declaration represents both declaration and expression of a function. 
    After this parser must return a block of statements.  */
typedef struct
{
  /** Identifier: name of a function. Can be NULL for anonimous functions.  */
  const char *name;
  /** List of parameter of a function. Can be NULL.  */
  formal_parameter_list *params;
}
function_declaration;

typedef function_declaration function_expression;

/** Represents expression, array literal and list of argument.  */
typedef struct expression_list
{
  /** Single assignment expression. Cannot be NULL for expression and list of arguments.
      But can be NULL for array literal.  */
  struct assignment_expression *assign_expr;
  /** Next expression. Can be NULL.  */
  struct expression_list *next;
}
expression_list;

typedef expression_list expression;
typedef expression_list array_literal;
typedef expression_list argument_list;

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
      void *data;
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

/** type of PropertyName. Can be integer, identifier of string literal.  */
typedef enum
{
  PN_NAME,
  PN_STRING,
  PN_NUM
}
property_name_type;

/** Represents name of property.  */
typedef struct
{
  /** Type of property name.  */
  property_name_type type;

  /** Value of property name.  */
  union
    {
      /** Identifier.  */
      const char *name;
      /** Value of string literal.  */
      const char *str;
      /** Numeric value.  */
      int num;
    }
  data;
}
property_name;

/** Represents a single property.  */
typedef struct
{
  /** Name of property.  */
  property_name *name;
  /** Value of property.  */
  struct assignment_expression *assign_expr;
}
property_name_and_value;

/** List of properties. Represents ObjectLiteral.  */
typedef struct property_name_and_value_list
{
  /** Current property.  */
  property_name_and_value *nav;

  /** Next property.  */
  struct property_name_and_value_list *next;
}
property_name_and_value_list;

typedef property_name_and_value_list object_literal;

/** Type of PrimaryExpression. Can be ThisLiteral, Identifier, Literal, ArrayLiteral,
    ObjectLiteral or expression.  */
typedef enum
{
  PE_THIS,
  PE_NAME,
  PE_LITERAL,
  PE_ARRAY,
  PE_OBJECT,
  PE_EXPR
}
primary_expression_type;

/** PrimaryExpression.  */
typedef struct
{
  /** Type of PrimaryExpression.  */
  primary_expression_type type;

  /** Value of PrimaryExpression.  */
  union
    {
      /** Used for ThisLiteral. Always NULL.  */
      void *none;
      /** Identifier.  */
      const char *name;
      /** Literal.  */
      literal *lit;
      /** ArrayLiteral.  */
      array_literal *array_lit;
      /** ObjectLiteral.  */
      object_literal *object_lit;
      /** Expression.  */
      expression *expr;
    }
  data;
}
primary_expression;

/** Type of suffix of MemberExpression. Can be either index-like ([]) or property-like (.).  */
typedef enum
{
  MES_INDEX,
  MES_PROPERTY
}
member_expression_suffix_type;

/** Suffix of MemberExpression.  */
typedef struct
{
  /** Type of suffix.  */
  member_expression_suffix_type type;

  /** Value of suffix.  */
  union
    {
      /** Used by index-like suffix.  */
      expression *index_expr;
      /** Used by property-like suffix.  */
      const char *name;
    }
  data;
}
member_expression_suffix;

/** List of MemberExpression's suffixes.  */
typedef struct member_expression_suffix_list
{
  /** Current suffix.  */
  member_expression_suffix *suffix;

  /** Next suffix.  */
  struct member_expression_suffix_list *next;
}
member_expression_suffix_list;

/** Represents MemberExpression Arguments grammar production.  */
typedef struct
{
  /** MemberExpression.  */
  struct member_expression *member_expr;
  /** Arguments.  */
  argument_list *args;
}
member_expression_with_arguments;

/** Types of MemberExpression. Can be PrimaryExpression, 
    FunctionExpression or MemberExpression Arguments.  */
typedef enum
{
  ME_PRIMARY,
  ME_FUNCTION,
  ME_ARGS
}
member_expression_type;

/** Represents MemberExpression.  */
typedef struct member_expression
{
  /** Type of MemberExpression.  */
  member_expression_type type;

  /** Value of MemberExpression.  */
  union
    {
      /** PrimaryExpression.  */
      primary_expression *primary_expr;
      /** FunctionExpression.  */
      function_expression *function_expr;
      /** MemberExpression Arguments.  */
      member_expression_with_arguments *args;
    }
  data;

  member_expression_suffix_list *suffix_list;
}
member_expression;

/** Types of NewExpression. Can be either MemberExpression or NewExpression.  */
typedef enum
{
  NE_MEMBER,
  NE_NEW
}
new_expression_type;

/** Represents NewExpression.  */
typedef struct new_expression
{
  /** Type of NewExpression.  */
  new_expression_type type;

  /** Value of NewExpression.  */
  union
    {
      /** MemberExpression.  */
      member_expression *member_expr;
      /** NewExpression.  */
      struct new_expression *new_expr;
    }
  data;
}
new_expression;

/** Types of CallExpression' suffix. Can be Arguments, index-like access ([]) or 
    property-like access (.).  */
typedef enum
{
  CAS_ARGS,
  CAS_INDEX,
  CAS_PROPERTY
}
call_expression_suffix_type;

/** Suffix of CallExpression.  */
typedef struct
{
  /** Type of suffix.  */
  call_expression_suffix_type type;

  /** Value of suffix.  */
  union
    {
      /** Arguments.  */
      argument_list *args;
      /** index-like access expression.  */
      expression *index_expr;
      /** Identifier of property.  */
      const char *name;
    }
  data;
}
call_expression_suffix;

/** List of CallExpression's suffixes.  */
typedef struct call_expression_suffix_list
{
  /** Current suffix.  */
  call_expression_suffix *suffix;

  /** Next suffix.  */
  struct call_expression_suffix_list *next;
}
call_expression_suffix_list;

/** CallExpression.  */
typedef struct
{
  /** Callee. Cannot be NULL.  */
  member_expression *member_expr;
  /** List of arguments. Can be NULL.  */
  argument_list *args;
  /** Suffixes of CallExpression. Can be NULL.  */
  call_expression_suffix_list *suffix_list;
}
call_expression;

/** Types of LeftHandSideExpression. Can be either CallExpression or NewExpression.  */
typedef enum
{
  LHSE_CALL,
  LHSE_NEW
}
left_hand_side_expression_type;

/** LeftHandSideExpression.  */
typedef struct
{
  /** Type of LeftHandSideExpression.  */
  left_hand_side_expression_type type;

  /** Value of LeftHandSideExpression.  */
  union
    {
      /** Value of CallExpression.  */
      call_expression *call_expr;
      /** Value of NewExpression.  */
      new_expression *new_expr;
    }
  data;
}
left_hand_side_expression;

/** Type of PostfixExpression. Unlike ECMA, it can contain no postfix operator in addition to
    increment and decrement.  */
typedef enum
{
  PE_NONE,
  PE_INCREMENT,
  PE_DECREMENT
}
postfix_expression_type;

/** PostfixExpression.  */
typedef struct
{
  /** Type of PostfixExpression.  */
  postfix_expression_type type;
  /** LeftHandSideExpression.  */
  left_hand_side_expression *expr;
}
postfix_expression;

/** Types of UnaryExpression. Can be PostfixExpression, delete UnaryExpression, 
    void UnaryExpression, typeof UnaryExpression, ++ UnaryExpression, -- UnaryExpression,
    + UnaryExpression, - UnaryExpression, ~ UnaryExpression, ! UnaryExpression.  */
typedef enum
{
  UE_POSTFIX,
  UE_DELETE,
  UE_VOID,
  UE_TYPEOF,
  UE_INCREMENT,
  UE_DECREMENT,
  UE_PLUS,
  UE_MINUS,
  UE_COMPL,
  UE_NOT
}
unary_expression_type;

/** UnaryExpression.  */
typedef struct unary_expression
{
  /** Type of UnaryExpression.  */
  unary_expression_type type;

  /** Data of UnaryExpression.  */
  union
    {
      /** PostfixExpression. Exists only when type of UE_POSTFIX.  */
      postfix_expression *postfix_expr;
      /** UnaryExpression after an operator. Exists otherwise.  */ 
      struct unary_expression *unary_expr;
    }
  data;
}
unary_expression;

/** Type of MultiplicativeExpression. In addition to ECMA if there is only one operand,
    we use ME_NONE.  */
typedef enum
{
  ME_NONE,
  ME_MULT,
  ME_DIV,
  ME_MOD
}
multiplicative_expression_type;

/** List of MultiplicativeExpressions. It can contain 1..n operands.  */
typedef struct multiplicative_expression_list
{
  /** Type of current MultiplicativeExpression.  */
  multiplicative_expression_type type;
  /** Current operand.  */
  unary_expression *unary_expr;

  /** Next operand.  */
  struct multiplicative_expression_list *next;
}
multiplicative_expression_list;

typedef enum
{
  AE_NONE,
  AE_PLUS,
  AE_MINUS
}
additive_expression_type;

typedef struct additive_expression_list
{
  additive_expression_type type;
  multiplicative_expression_list *mult_expr;

  struct additive_expression_list *next;
}
additive_expression_list;

typedef enum
{
  SE_NONE,
  SE_LSHIFT,
  SE_RSHIFT,
  SE_RSHIFT_EX
}
shift_expression_type;

typedef struct shift_expression_list
{
  shift_expression_type type;
  additive_expression_list *add_expr;

  struct shift_expression_list *next;
}
shift_expression_list;

typedef enum
{
  RE_NONE,
  RE_LESS,
  RE_GREATER,
  RE_LESS_EQ,
  RE_GREATER_EQ,
  RE_INSTANCEOF,
  RE_IN
}
relational_expression_type;

typedef struct relational_expression_list
{
  relational_expression_type type;
  shift_expression_list *shift_expr;

  struct relational_expression_list *next;
}
relational_expression_list;

typedef enum
{
  EE_NONE,
  EE_DOUBLE_EQ,
  EE_NOT_EQ,
  EE_TRIPLE_EQ,
  EE_NOT_DOUBLE_EQ
}
equality_expression_type;

typedef struct equality_expression_list
{
  equality_expression_type type;
  relational_expression_list *rel_expr;

  struct equality_expression_list *next;
}
equality_expression_list;

typedef struct bitwise_and_expression_list
{
  equality_expression_list *eq_expr;
  
  struct bitwise_and_expression_list *next;
}
bitwise_and_expression_list;

typedef struct bitwise_xor_expression_list
{
  bitwise_and_expression_list *and_expr;
  
  struct bitwise_xor_expression_list *next;
}
bitwise_xor_expression_list;

typedef struct bitwise_or_expression_list
{
  bitwise_xor_expression_list *xor_expr;
  
  struct bitwise_or_expression_list *next;
}
bitwise_or_expression_list;

typedef struct logical_and_expression_list
{
  bitwise_or_expression_list *or_expr;
  
  struct logical_and_expression_list *next;
}
logical_and_expression_list;

typedef struct logical_or_expression_list
{
  logical_and_expression_list *and_expr;
  
  struct logical_or_expression_list *next;
}
logical_or_expression_list;

typedef struct
{
  logical_or_expression_list *or_expr;
  struct assignment_expression *then_expr, *else_expr;
}
conditional_expression;

typedef enum
{
  AE_COND,
  AE_EQ,
  AE_MULT_EQ,
  AE_DIV_EQ,
  AE_MOD_EQ,
  AE_PLUS_EQ,
  AE_MINUS_EQ,
  AE_LSHIFT_EQ,
  AE_RSHIFT_EQ,
  AE_RSHIFT_EX_EQ,
  AE_AND_EQ,
  AE_OR_EQ,
  AE_XOR_EQ
}
assignment_expression_type;

typedef struct
{
  left_hand_side_expression *left_hand_expr;
  struct assignment_expression *assign_expr;
}
left_hand_and_assignment_expression;

typedef struct assignment_expression
{
  assignment_expression_type type;

  union
    {
      conditional_expression *cond_expr;
      left_hand_and_assignment_expression s;
    }
  data;
}
assignment_expression;

/* Statements.  */

typedef struct
{
  const char *name;

  assignment_expression *ass_expr;
}
variable_declaration;

typedef struct variable_declaration_list
{
  variable_declaration *var_decl;

  struct variable_declaration_list *next;
}
variable_declaration_list;

typedef struct
{
  bool is_decl;

  union
    {
      expression *expr;
      variable_declaration_list *decl_list;
    }
  data;
}
for_statement_initialiser_part;

typedef struct
{
  for_statement_initialiser_part *init;
  expression *limit, *incr;
}
for_statement;

typedef struct
{
  bool is_decl;

  union
    {
      left_hand_side_expression *left_hand_expr;
      variable_declaration *decl;
    }
  data;
}
for_in_statement_initializer_part;

typedef struct
{
  for_in_statement_initializer_part *init;
  expression *list_expr;
}
for_in_statement;

typedef struct
{
  bool is_for_in;

  union
    {
      for_statement *for_stmt;
      for_in_statement *for_in_stmt;
    }
  data;
}
for_or_for_in_statement;

typedef enum
{
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
      variable_declaration_list *var_stmt;
      expression *expr;
      for_or_for_in_statement *for_stmt;
      const char *name;
      function_declaration *fun_decl;
    }
  data;
}
statement;

void parser_init ();
statement *parser_parse_statement ();

#endif
