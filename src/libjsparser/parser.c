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

#include "optimizer-passes.h"
#include "jerry-libc.h"
#include "lexer.h"
#include "parser.h"
#include "opcodes.h"
#include "serializer.h"
#include "interpreter.h"
#include "stack.h"
#include "hash-table.h"
#include "deserializer.h"

#define INVALID_VALUE 255
#define INTRINSICS_COUNT 1

typedef enum
{
  REWRITABLE_CONTINUE = 0,
  REWRITABLE_BREAK,
  REWRITABLE_OPCODES_COUNT
}
rewritable_opcode_type;

typedef struct
{
  uint8_t args_count;
  union
  {
    void (*fun1) (idx_t);
  }
  funs;
}
intrinsic_dumper;

#define NESTING_ITERATIONAL 1
#define NESTING_SWITCH      2
#define NESTING_FUNCTION    3

#define GLOBAL(NAME, VAR) STACK_ELEMENT (NAME, VAR)

enum
{
  U8_global_size
};
STATIC_STACK (U8, uint8_t, uint8_t)

enum
{
  IDX_global_size
};
STATIC_STACK (IDX, uint8_t, uint8_t)

enum
{
  nestings_global_size
};
STATIC_STACK (nestings, uint8_t, uint8_t)

enum
{
  temp_name,
  min_temp_name,
  max_temp_name,
  temp_names_global_size
};
STATIC_STACK (temp_names, uint8_t, uint8_t)

#define MAX_TEMP_NAME() \
GLOBAL(temp_names, max_temp_name)

#define MIN_TEMP_NAME() \
GLOBAL(temp_names, min_temp_name)

#define TEMP_NAME() \
GLOBAL(temp_names, temp_name)

enum
{
  tok = 0,
  toks_global_size
};
STATIC_STACK (toks, uint8_t, token)

#define TOK() \
GLOBAL(toks, tok)

enum
{
  opcode = 0,
  ops_global_size
};
STATIC_STACK (ops, uint8_t, opcode_t)

#define OPCODE() \
GLOBAL(ops, opcode)

enum
{
  opcode_counter = 0,
  U16_global_size
};
STATIC_STACK (U16, uint8_t, uint16_t)

#define OPCODE_COUNTER() \
GLOBAL(U16, opcode_counter)

enum
{
  rewritable_continue_global_size
};
STATIC_STACK (rewritable_continue, uint8_t, uint16_t)

enum
{
  rewritable_break_global_size
};
STATIC_STACK (rewritable_break, uint8_t, uint16_t)

#ifndef JERRY_NDEBUG
#define STACK_CHECK_USAGE_LHS() \
JERRY_ASSERT (IDX.current == IDX_current + 1);
#else
#define STACK_CHECK_USAGE_LHS() ;
#endif

static uint8_t lp_string_hash (lp_string);
STATIC_HASH_TABLE (intrinsics, lp_string, intrinsic_dumper)

#define LAST_OPCODE_IS(OP) (deserialize_opcode((opcode_counter_t)(OPCODE_COUNTER()-1)).op_idx == __op__idx_##OP)

JERRY_STATIC_ASSERT (sizeof (idx_t) == sizeof (uint8_t));

JERRY_STATIC_ASSERT (sizeof (opcode_counter_t) == sizeof (uint16_t));

static void skip_newlines (void);
#define NEXT(TYPE) \
do { skip_newlines (); parse_##TYPE (); } while (0)

#define DUMP_VOID_OPCODE(GETOP) \
do { \
  OPCODE()=getop_##GETOP (); \
  serializer_dump_opcode (OPCODE()); \
  OPCODE_COUNTER()++; \
} while (0)

#define DUMP_OPCODE_1(GETOP, OP1) \
do { \
  JERRY_ASSERT (0+OP1 <= 255); \
  OPCODE()=getop_##GETOP ((idx_t) (OP1)); \
  serializer_dump_opcode (OPCODE()); \
  OPCODE_COUNTER()++; \
} while (0)

#define DUMP_OPCODE_2(GETOP, OP1, OP2) \
do { \
  JERRY_ASSERT (0+OP1 <= 255); \
  JERRY_ASSERT (0+OP2 <= 255); \
  OPCODE()=getop_##GETOP ((idx_t) (OP1), (idx_t) (OP2)); \
  serializer_dump_opcode (OPCODE()); \
  OPCODE_COUNTER()++; \
} while (0)

#define DUMP_OPCODE_3(GETOP, OP1, OP2, OP3) \
do { \
  JERRY_ASSERT (0+OP1 <= 255); \
  JERRY_ASSERT (0+OP2 <= 255); \
  JERRY_ASSERT (0+OP3 <= 255); \
  OPCODE()=getop_##GETOP ((idx_t) (OP1), (idx_t) (OP2), (idx_t) (OP3)); \
  serializer_dump_opcode (OPCODE()); \
  OPCODE_COUNTER()++; \
} while (0)

#define REWRITE_VOID_OPCODE(OC, GETOP) \
do { \
  OPCODE()=getop_##GETOP (); \
  serializer_rewrite_opcode (OC, OPCODE()); \
} while (0)

#define REWRITE_OPCODE_1(OC, GETOP, OP1) \
do { \
  JERRY_ASSERT (0+OP1 <= 255); \
  OPCODE()=getop_##GETOP ((idx_t) (OP1)); \
  serializer_rewrite_opcode (OC, OPCODE()); \
} while (0)

#define REWRITE_OPCODE_2(OC, GETOP, OP1, OP2) \
do { \
  JERRY_ASSERT (0+OP1 <= 255); \
  JERRY_ASSERT (0+OP2 <= 255); \
  OPCODE()=getop_##GETOP ((idx_t) (OP1), (idx_t) (OP2)); \
  serializer_rewrite_opcode (OC, OPCODE()); \
} while (0)

#define REWRITE_OPCODE_3(OC, GETOP, OP1, OP2, OP3) \
do { \
  JERRY_ASSERT (0+OP1 <= 255); \
  JERRY_ASSERT (0+OP2 <= 255); \
  JERRY_ASSERT (0+OP3 <= 255); \
  OPCODE()=getop_##GETOP ((idx_t) (OP1), (idx_t) (OP2), (idx_t) (OP3)); \
  serializer_rewrite_opcode (OC, OPCODE()); \
} while (0)

#define REWRITE_COND_JMP(OC, GETOP, DIFF) \
do { \
  JERRY_ASSERT (0+DIFF <= 256*256 - 1); \
  STACK_DECLARE_USAGE (IDX); \
  JERRY_STATIC_ASSERT (sizeof (idx_t) == 1); \
  STACK_PUSH (IDX, (idx_t) ((DIFF) >> JERRY_BITSINBYTE)); \
  STACK_PUSH (IDX, (idx_t) ((DIFF) & ((1 << JERRY_BITSINBYTE) - 1))); \
  JERRY_ASSERT ((DIFF) == calc_opcode_counter_from_idx_idx (STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 1))); \
  OPCODE()=getop_##GETOP (STACK_HEAD (IDX, 3), STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 1)); \
  serializer_rewrite_opcode (OC, OPCODE()); \
  STACK_DROP (IDX, 2); \
  STACK_CHECK_USAGE (IDX); \
} while (0)

#define REWRITE_JMP(OC, GETOP, DIFF) \
do { \
  JERRY_ASSERT (0+DIFF <= 256*256 - 1); \
  STACK_DECLARE_USAGE (IDX) \
  JERRY_STATIC_ASSERT (sizeof (idx_t) == 1); \
  STACK_PUSH (IDX, (idx_t) ((DIFF) >> JERRY_BITSINBYTE)); \
  STACK_PUSH (IDX, (idx_t) ((DIFF) & ((1 << JERRY_BITSINBYTE) - 1))); \
  JERRY_ASSERT ((DIFF) == calc_opcode_counter_from_idx_idx (STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 1))); \
  OPCODE()=getop_##GETOP (STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 1)); \
  serializer_rewrite_opcode (OC, OPCODE()); \
  STACK_DROP (IDX, 2); \
  STACK_CHECK_USAGE (IDX); \
} while (0)

#define REWRITE_TRY(OC) \
do { \
  STACK_DECLARE_USAGE (IDX) \
  JERRY_STATIC_ASSERT (sizeof (idx_t) == 1); \
  STACK_PUSH (IDX, (idx_t) ((OPCODE_COUNTER ()) >> JERRY_BITSINBYTE)); \
  STACK_PUSH (IDX, (idx_t) ((OPCODE_COUNTER ()) & ((1 << JERRY_BITSINBYTE) - 1))); \
  JERRY_ASSERT ((OPCODE_COUNTER ()) \
                == calc_opcode_counter_from_idx_idx (STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 1))); \
  OPCODE()=getop_try (STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 1)); \
  serializer_rewrite_opcode ((OC), OPCODE()); \
  STACK_DROP (IDX, 2); \
  STACK_CHECK_USAGE (IDX); \
} while (0)

typedef enum
{
  AL_FUNC_DECL,
  AL_FUNC_EXPR,
  AL_ARRAY_DECL,
  AL_OBJ_DECL,
  AL_CONSTRUCT_EXPR,
  AL_CALL_EXPR
}
argument_list_type;

static void parse_expression (void);
static void parse_statement (void);
static void parse_assignment_expression (void);
static void parse_source_element_list (void);
static void parse_argument_list (argument_list_type, idx_t);

static uint8_t
lp_string_hash (lp_string str)
{
  return str.length % INTRINSICS_COUNT;
}

static idx_t
next_temp_name (void)
{
  TEMP_NAME ()++;
  if (MAX_TEMP_NAME () < TEMP_NAME ())
  {
    MAX_TEMP_NAME () = TEMP_NAME ();
  }
  return TEMP_NAME ();
}

static void
start_new_scope (void)
{
  STACK_PUSH (temp_names, MAX_TEMP_NAME());
  MAX_TEMP_NAME() = MIN_TEMP_NAME();
}

static void
finish_scope (void)
{
  STACK_POP (temp_names, TEMP_NAME());
  MAX_TEMP_NAME() = TEMP_NAME();
}

static void
reset_temp_name (void)
{
  TEMP_NAME() = MIN_TEMP_NAME();
}

static void
push_nesting (uint8_t nesting_type)
{
  STACK_PUSH (nestings, nesting_type);
}

static void
pop_nesting (uint8_t nesting_type)
{
  JERRY_ASSERT (STACK_HEAD (nestings, 1) == nesting_type);
  STACK_DROP (nestings, 1);
}

static void
must_be_inside_but_not_in (uint8_t *inside, uint8_t insides_count, uint8_t not_in)
{
  STACK_DECLARE_USAGE (U8) // i, j
  STACK_PUSH (U8, 0);
  STACK_PUSH (U8, 0);
#define I() STACK_HEAD (U8, 2)
#define J() STACK_HEAD (U8, 1)

  if (STACK_SIZE(nestings) == 0)
  {
    parser_fatal (ERR_PARSER);
  }

  for (I() = (uint8_t) (STACK_SIZE(nestings)); I() != 0; I()--)
  {
    if (nestings.data[I() - 1] == not_in)
    {
      parser_fatal (ERR_PARSER);
    }

    for (J() = 0; J() < insides_count; J()++)
    {
      if (nestings.data[I() - 1] == inside[J()])
      {
        goto cleanup;
      }
    }
  }

  parser_fatal (ERR_PARSER);

cleanup:
  STACK_DROP (U8, 2);
#undef I
#undef J
  STACK_CHECK_USAGE (U8);
}

static bool
token_is (token_type tt)
{
  return TOK ().type == tt;
}

static uint8_t
token_data (void)
{
  return TOK ().uid;
}

static void
skip_token (void)
{
  TOK() = lexer_next_token ();
}

static void
assert_keyword (keyword kw)
{
  if (!token_is (TOK_KEYWORD) || token_data () != kw)
  {
#ifdef __TARGET_HOST_x64
    __printf ("assert_keyword: %d\n", kw);
#endif
    JERRY_UNREACHABLE ();
  }
}

static bool
is_keyword (keyword kw)
{
  return token_is (TOK_KEYWORD) && token_data () == kw;
}

static void
current_token_must_be (token_type tt)
{
  if (!token_is (tt))
  {
#ifdef __TARGET_HOST_x64
    __printf ("current_token_must_be: %d\n", tt);
#endif
    parser_fatal (ERR_PARSER);
  }
}

static void
skip_newlines (void)
{
  do
  {
    skip_token ();
  }
  while (token_is (TOK_NEWLINE));
}

static void
next_token_must_be (token_type tt)
{
  skip_token ();
  if (!token_is (tt))
  {
#ifdef __TARGET_HOST_x64
    __printf ("next_token_must_be: %d\n", tt);
#endif
    parser_fatal (ERR_PARSER);
  }
}

static void
token_after_newlines_must_be (token_type tt)
{
  skip_newlines ();
  if (!token_is (tt))
  {
    parser_fatal (ERR_PARSER);
  }
}

static inline void
token_after_newlines_must_be_keyword (keyword kw)
{
  skip_newlines ();
  if (!is_keyword (kw))
  {
    parser_fatal (ERR_PARSER);
  }
}

static void
integer_zero (void)
{
  STACK_DECLARE_USAGE (IDX)

  STACK_PUSH (IDX, next_temp_name ());
  DUMP_OPCODE_3 (assignment, STACK_HEAD (IDX, 1), OPCODE_ARG_TYPE_SMALLINT, 0);

  STACK_CHECK_USAGE_LHS ();
}

static void
boolean_true (void)
{
  STACK_DECLARE_USAGE (IDX)

  STACK_PUSH (IDX, next_temp_name ());
  DUMP_OPCODE_3 (assignment, STACK_HEAD (IDX, 1), OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_TRUE);

  STACK_CHECK_USAGE_LHS ();
}

static void
add_to_rewritable_opcodes (rewritable_opcode_type type, opcode_counter_t oc)
{
  switch (type)
  {
    case REWRITABLE_BREAK: STACK_PUSH (rewritable_break, oc); break;
    case REWRITABLE_CONTINUE: STACK_PUSH (rewritable_continue, oc); break;
    default: JERRY_UNREACHABLE ();
  }
}

static void
rewrite_rewritable_opcodes (rewritable_opcode_type type, opcode_counter_t oc)
{
  STACK_DECLARE_USAGE (U8)

  STACK_PUSH (U8, 0);

  switch (type)
  {
    case REWRITABLE_BREAK:
    {
      for (STACK_HEAD (U8, 1) = 0; STACK_HEAD (U8, 1) < STACK_SIZE (rewritable_break); STACK_HEAD (U8, 1)++)
      {
        REWRITE_JMP (STACK_ELEMENT (rewritable_break, STACK_HEAD (U8, 1)), jmp_down,
                     oc - STACK_ELEMENT (rewritable_break, STACK_HEAD (U8, 1)));
      }
      STACK_CLEAN (rewritable_break);
      break;
    }
    case REWRITABLE_CONTINUE:
    {
      for (STACK_HEAD (U8, 1) = 0; STACK_HEAD (U8, 1) < STACK_SIZE (rewritable_continue); STACK_HEAD (U8, 1)++)
      {
        REWRITE_JMP (STACK_ELEMENT (rewritable_continue, STACK_HEAD (U8, 1)), jmp_up,
                     STACK_ELEMENT (rewritable_continue, STACK_HEAD (U8, 1)) - oc);
      }
      STACK_CLEAN (rewritable_continue);
      break;
    }
    default: JERRY_UNREACHABLE ();
  }

  STACK_DROP (U8, 1);

  STACK_CHECK_USAGE (U8);
}

static void
dump_assert (idx_t arg)
{
  DUMP_OPCODE_3 (is_true_jmp_down, arg, 0, 2);
  DUMP_OPCODE_1 (exitval, 1);
}

static void
fill_intrinsics (void)
{
  lp_string str = (lp_string)
  {
    .length = 6,
    .str = (const ecma_char_t *) "assert"
  };
  intrinsic_dumper dumper = (intrinsic_dumper)
  {
    .args_count = 1,
    .funs.fun1 = dump_assert
  };
  HASH_INSERT (intrinsics, str, dumper);
}

static bool
is_intrinsic (idx_t obj)
{
  /* Every literal is represented by assignment to tmp.
     so if result of parse_primary_expression less then strings count,
     it is identifier, check for intrinsics.  */
  // U8 strs
  bool result = false;

  STACK_DECLARE_USAGE (U8)

  STACK_PUSH (U8, lexer_get_strings_count ());
  if (obj < STACK_HEAD (U8, 1))
  {
    if (HASH_LOOKUP (intrinsics, lexer_get_string_by_id (obj)) != NULL)
    {
      result = true;
      goto cleanup;
    }
  }

cleanup:
  STACK_DROP (U8, 1);

  STACK_CHECK_USAGE (U8);
  return result;
}

static void
dump_intrinsic (idx_t obj, idx_t arg)
{
  if (obj < lexer_get_strings_count ())
  {
    intrinsic_dumper *dumper = HASH_LOOKUP (intrinsics, lexer_get_string_by_id (obj));

    JERRY_ASSERT (dumper);

    switch (dumper->args_count)
    {
      case 1: dumper->funs.fun1 (arg); return;
      default: JERRY_UNREACHABLE ();
    }
  }

  JERRY_UNREACHABLE ();
}

/* property_name
  : Identifier
  | StringLiteral
  | NumericLiteral
  ; */
static void
parse_property_name (void)
{
  STACK_DECLARE_USAGE (IDX)

  switch (TOK ().type)
  {
    case TOK_NAME:
    case TOK_STRING:
    case TOK_NUMBER:
    {
      STACK_PUSH (IDX, token_data ());
      break;
    }
    case TOK_SMALL_INT:
    {
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (assignment, STACK_HEAD (IDX, 1), OPCODE_ARG_TYPE_SMALLINT, token_data ());
      break;
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }

  STACK_CHECK_USAGE_LHS ();
}

/* property_name_and_value
  : property_name LT!* ':' LT!* assignment_expression
  ; */
static void
parse_property_name_and_value (void)
{
  // IDX lhs, name, expr
  STACK_DECLARE_USAGE (IDX)

  parse_property_name (); // push name

  token_after_newlines_must_be (TOK_COLON);
  NEXT (assignment_expression); // push expr

  DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_VARG_PROP_DATA, STACK_HEAD(IDX, 2), STACK_HEAD(IDX, 1));

  STACK_DROP (IDX, 2);
  STACK_CHECK_USAGE (IDX);
}

static void
rewrite_meta_opcode_counter (opcode_counter_t meta_oc, opcode_meta_type type)
{
  // IDX oc_idx_1, oc_idx_2
  STACK_DECLARE_USAGE (IDX)

  JERRY_STATIC_ASSERT (sizeof (idx_t) == 1);

  STACK_PUSH (IDX, (idx_t) (OPCODE_COUNTER () >> JERRY_BITSINBYTE));
  STACK_PUSH (IDX, (idx_t) (OPCODE_COUNTER () & ((1 << JERRY_BITSINBYTE) - 1)));

  JERRY_ASSERT (OPCODE_COUNTER () == calc_opcode_counter_from_idx_idx (STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 1)));

  REWRITE_OPCODE_3 (meta_oc, meta, type, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 1));

  STACK_DROP (IDX, 2);

  STACK_CHECK_USAGE (IDX);
}

/* property_assignment
  : property_name_and_value
  | get LT!* property_name LT!* '(' LT!* ')' LT!* '{' LT!* function_body LT!* '}'
  | set LT!* property_name LT!* '(' identifier ')' LT!* '{' LT!* function_body LT!* '}'
  ; */
static void
parse_property_assignment (void)
{
  STACK_DECLARE_USAGE (IDX)
  STACK_DECLARE_USAGE (U16)

  current_token_must_be (TOK_NAME);

  if (lp_string_equal_s (lexer_get_string_by_id (token_data ()), "get"))
  {
    // name, lhs
    NEXT (property_name); // push name

    skip_newlines ();
    parse_argument_list (AL_FUNC_EXPR, next_temp_name ()); // push lhs

    STACK_PUSH (U16, opcode_counter);
    DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_FUNCTION_END, INVALID_VALUE, INVALID_VALUE);

    token_after_newlines_must_be (TOK_OPEN_BRACE);
    skip_newlines ();
    parse_source_element_list ();
    token_after_newlines_must_be (TOK_CLOSE_BRACE);

    DUMP_VOID_OPCODE (ret);
    rewrite_meta_opcode_counter (STACK_HEAD (U16, 1), OPCODE_META_TYPE_FUNCTION_END);
    DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_VARG_PROP_GETTER, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 1));

    STACK_DROP (IDX, 2);
    STACK_DROP (U16, 1);
  }
  else if (lp_string_equal_s (lexer_get_string_by_id (token_data ()), "set"))
  {
    // name, lhs
    NEXT (property_name); // push name

    skip_newlines ();
    parse_argument_list (AL_FUNC_EXPR, next_temp_name ()); // push lhs

    STACK_PUSH (U16, opcode_counter);
    DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_FUNCTION_END, INVALID_VALUE, INVALID_VALUE);

    token_after_newlines_must_be (TOK_OPEN_BRACE);
    skip_newlines ();
    parse_source_element_list ();
    token_after_newlines_must_be (TOK_CLOSE_BRACE);

    DUMP_VOID_OPCODE (ret);
    rewrite_meta_opcode_counter (STACK_HEAD (U16, 1), OPCODE_META_TYPE_FUNCTION_END);
    DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_VARG_PROP_SETTER, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 1));

    STACK_DROP (IDX, 2);
    STACK_DROP (U16, 1);
  }
  else
  {
    parse_property_name_and_value ();
  }

  STACK_CHECK_USAGE (U16);
  STACK_CHECK_USAGE (IDX);
}

/** Parse list of identifiers, assigment expressions or properties, splitted by comma.
    For each ALT dumps appropriate bytecode. Uses OBJ during dump if neccesary.
    Returns temp var if expression has lhs, or 0 otherwise.  */
static void
parse_argument_list (argument_list_type alt, idx_t obj)
{
  // U8 open_tt, close_tt, args_count
  // IDX lhs, current_arg
  // U16 oc
  STACK_DECLARE_USAGE (U8)
  STACK_DECLARE_USAGE (U16)
  STACK_DECLARE_USAGE (IDX)

  STACK_PUSH (U16, OPCODE_COUNTER ());
  switch (alt)
  {
    case AL_FUNC_DECL:
    {
      STACK_PUSH (U8, TOK_OPEN_PAREN); // Openning token;
      STACK_PUSH (U8, TOK_CLOSE_PAREN); // Ending token;
      DUMP_OPCODE_2 (func_decl_n, obj, INVALID_VALUE);
      break;
    }
    case AL_FUNC_EXPR:
    {
      STACK_PUSH (U8, TOK_OPEN_PAREN);
      STACK_PUSH (U8, TOK_CLOSE_PAREN);
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (func_expr_n, STACK_HEAD (IDX, 1), obj, INVALID_VALUE);
      break;
    }
    case AL_CONSTRUCT_EXPR:
    {
      STACK_PUSH (U8, TOK_OPEN_PAREN);
      STACK_PUSH (U8, TOK_CLOSE_PAREN);
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (construct_n, STACK_HEAD (IDX, 1), obj, INVALID_VALUE);
      break;
    }
    case AL_CALL_EXPR:
    {
      STACK_PUSH (U8, TOK_OPEN_PAREN);
      STACK_PUSH (U8, TOK_CLOSE_PAREN);
      if (is_intrinsic (obj))
      {
        break;
      }
      else
      {
        STACK_PUSH (IDX, next_temp_name ());
        DUMP_OPCODE_3 (call_n, STACK_HEAD (IDX, 1), obj, INVALID_VALUE);
      }
      break;
    }
    case AL_ARRAY_DECL:
    {
      STACK_PUSH (U8, TOK_OPEN_SQUARE);
      STACK_PUSH (U8, TOK_CLOSE_SQUARE);
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_2 (array_decl, STACK_HEAD (IDX, 1), INVALID_VALUE);
      break;
    }
    case AL_OBJ_DECL:
    {
      STACK_PUSH (U8, TOK_OPEN_BRACE);
      STACK_PUSH (U8, TOK_CLOSE_BRACE);
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_2 (obj_decl, STACK_HEAD (IDX, 1), INVALID_VALUE);
      break;
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }

  STACK_PUSH (U8, 0);

  current_token_must_be (STACK_HEAD (U8, 3));

  skip_newlines ();
  while (!token_is (STACK_HEAD (U8, 2)))
  {
    switch (alt)
    {
      case AL_FUNC_DECL:
      {
        current_token_must_be (TOK_NAME);
        STACK_PUSH (IDX, token_data ());
        break;
      }
      case AL_FUNC_EXPR:
      case AL_ARRAY_DECL:
      case AL_CONSTRUCT_EXPR:
      {
        parse_assignment_expression ();
        break;
      }
      case AL_CALL_EXPR:
      {
        parse_assignment_expression ();
        if (is_intrinsic (obj))
        {
          dump_intrinsic (obj, STACK_HEAD (IDX, 1));
          goto next;
        }
        break;
      }
      case AL_OBJ_DECL:
      {
        parse_property_assignment ();
        break;
      }
      default:
      {
        JERRY_UNREACHABLE ();
      }
    }

    if (alt != AL_OBJ_DECL)
    {
      DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_VARG, STACK_HEAD (IDX, 1), INVALID_VALUE);
      STACK_DROP (IDX, 1);
    }
    STACK_HEAD(U8, 1)++;

next:
    skip_newlines ();
    if (!token_is (TOK_COMMA))
    {
      current_token_must_be (STACK_HEAD (U8, 2));
      break;
    }

    skip_newlines ();
  }

  switch (alt)
  {
    case AL_FUNC_DECL:
    {
      REWRITE_OPCODE_2 (STACK_HEAD (U16, 1), func_decl_n, obj, STACK_HEAD (U8, 1));
      break;
    }
    case AL_FUNC_EXPR:
    {
      REWRITE_OPCODE_3 (STACK_HEAD (U16, 1), func_expr_n, STACK_HEAD (IDX, 1), obj, STACK_HEAD (U8, 1));
      break;
    }
    case AL_CONSTRUCT_EXPR:
    {
      REWRITE_OPCODE_3 (STACK_HEAD (U16, 1), construct_n, STACK_HEAD (IDX, 1), obj, STACK_HEAD (U8, 1));
      break;
    }
    case AL_CALL_EXPR:
    {
      if (is_intrinsic (obj))
      {
        break;
      }
      else
      {
        REWRITE_OPCODE_3 (STACK_HEAD (U16, 1), call_n, STACK_HEAD (IDX, 1), obj, STACK_HEAD (U8, 1));
      }
      break;
    }
    case AL_ARRAY_DECL:
    {
      REWRITE_OPCODE_2 (STACK_HEAD (U16, 1), array_decl, STACK_HEAD (IDX, 1), STACK_HEAD (U8, 1));
      break;
    }
    case AL_OBJ_DECL:
    {
      REWRITE_OPCODE_2 (STACK_HEAD (U16, 1), obj_decl, STACK_HEAD (IDX, 1), STACK_HEAD (U8, 1));
      break;
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }

  STACK_DROP (U8, 3);
  STACK_DROP (U16, 1);

  STACK_CHECK_USAGE (U8);
  STACK_CHECK_USAGE (U16);

#ifndef JERRY_NDEBUG
  if (alt == AL_FUNC_DECL)
  {
    STACK_CHECK_USAGE (IDX);
  }
  else
  {
    STACK_CHECK_USAGE_LHS ();
  }
#endif
}

/* function_declaration
  : 'function' LT!* Identifier LT!*
    '(' (LT!* Identifier (LT!* ',' LT!* Identifier)*) ? LT!* ')' LT!* function_body
  ;

   function_body
  : '{' LT!* sourceElements LT!* '}' */
static void
parse_function_declaration (void)
{
  // IDX name
  // U16 meta_oc
  STACK_DECLARE_USAGE (IDX)
  STACK_DECLARE_USAGE (U16)
  STACK_DECLARE_USAGE (nestings)

  assert_keyword (KW_FUNCTION);

  token_after_newlines_must_be (TOK_NAME);

  STACK_PUSH (IDX, token_data ());

  skip_newlines ();
  parse_argument_list (AL_FUNC_DECL, STACK_HEAD (IDX, 1));

  STACK_PUSH (U16, OPCODE_COUNTER ());
  DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_UNDEFINED, INVALID_VALUE, INVALID_VALUE);

  token_after_newlines_must_be (TOK_OPEN_BRACE);

  skip_newlines ();
  push_nesting (NESTING_FUNCTION);
  parse_source_element_list ();
  pop_nesting (NESTING_FUNCTION);

  next_token_must_be (TOK_CLOSE_BRACE);

  DUMP_VOID_OPCODE (ret);

  rewrite_meta_opcode_counter (STACK_HEAD (U16, 1), OPCODE_META_TYPE_FUNCTION_END);

  STACK_DROP (U16, 1);
  STACK_DROP (IDX, 1);

  STACK_CHECK_USAGE (IDX);
  STACK_CHECK_USAGE (U16);
  STACK_CHECK_USAGE (nestings);
}

/* function_expression
  : 'function' LT!* Identifier? LT!* '(' formal_parameter_list? LT!* ')' LT!* function_body
  ; */
static void
parse_function_expression (void)
{
  // IDX lhs, name
  // U16 meta_oc
  STACK_DECLARE_USAGE (IDX)
  STACK_DECLARE_USAGE (U16)
  STACK_DECLARE_USAGE (nestings)

  assert_keyword (KW_FUNCTION);

  skip_newlines ();
  if (token_is (TOK_NAME))
  {
    STACK_PUSH (IDX, token_data ());
  }
  else
  {
    lexer_save_token (TOK());
    STACK_PUSH (IDX, next_temp_name ());
  }

  skip_newlines ();
  parse_argument_list (AL_FUNC_EXPR, STACK_HEAD (IDX, 1)); // push lhs

  STACK_PUSH (U16, OPCODE_COUNTER ());
  DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_UNDEFINED, INVALID_VALUE, INVALID_VALUE);

  token_after_newlines_must_be (TOK_OPEN_BRACE);

  skip_newlines ();
  push_nesting (NESTING_FUNCTION);
  parse_source_element_list ();
  pop_nesting (NESTING_FUNCTION);

  token_after_newlines_must_be (TOK_CLOSE_BRACE);

  DUMP_VOID_OPCODE (ret);
  rewrite_meta_opcode_counter (STACK_HEAD (U16, 1), OPCODE_META_TYPE_FUNCTION_END);

  STACK_HEAD (IDX, 2) = STACK_HEAD (IDX, 1);
  STACK_DROP (IDX, 1);
  STACK_DROP (U16, 1);

  STACK_CHECK_USAGE (U16);
  STACK_CHECK_USAGE_LHS ();
  STACK_CHECK_USAGE (nestings);
}

/* array_literal
  : '[' LT!* assignment_expression? (LT!* ',' (LT!* assignment_expression)?)* LT!* ']' LT!*
  ; */
static void
parse_array_literal (void)
{
  STACK_DECLARE_USAGE (IDX)

  parse_argument_list (AL_ARRAY_DECL, 0);

  STACK_CHECK_USAGE_LHS ();
}

/* object_literal
  : '{' LT!* property_assignment (LT!* ',' LT!* property_assignment)* LT!* '}'
  ; */
static void
parse_object_literal (void)
{
  STACK_DECLARE_USAGE (IDX)

  parse_argument_list (AL_OBJ_DECL, 0);

  STACK_CHECK_USAGE_LHS ();;
}

static void
parse_literal (void)
{
  // IDX lhs;
  STACK_DECLARE_USAGE (IDX)

  switch (TOK ().type)
  {
    case TOK_NULL:
    {
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (assignment, STACK_HEAD (IDX, 1), OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_NULL);
      break;
    }
    case TOK_BOOL:
    {
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (assignment, STACK_HEAD (IDX, 1), OPCODE_ARG_TYPE_SIMPLE,
                     token_data () ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE);
      break;
    }
    case TOK_NUMBER:
    {
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (assignment, STACK_HEAD (IDX, 1), OPCODE_ARG_TYPE_NUMBER, token_data ());
      break;
    }
    case TOK_SMALL_INT:
    {
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (assignment, STACK_HEAD (IDX, 1), OPCODE_ARG_TYPE_SMALLINT, token_data ());
      break;
    }
    case TOK_STRING:
    {
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (assignment, STACK_HEAD (IDX, 1), OPCODE_ARG_TYPE_STRING, token_data ());
      break;
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }

  STACK_CHECK_USAGE_LHS ();
}

/* primary_expression
  : 'this'
  | Identifier
  | literal
  | '[' LT!* array_literal LT!* ']'
  | '{' LT!* object_literal LT!* '}'
  | '(' LT!* expression LT!* ')'
  ; */
static void
parse_primary_expression (void)
{
  // IDX lhs;
  STACK_DECLARE_USAGE (IDX)

  if (is_keyword (KW_THIS))
  {
    STACK_PUSH (IDX, next_temp_name ());
    DUMP_OPCODE_1 (this, STACK_HEAD (IDX, 1));
    goto cleanup;
  }

  switch (TOK ().type)
  {
    case TOK_NAME:
    {
      STACK_PUSH (IDX, token_data ());
      break;
    }
    case TOK_NULL:
    case TOK_BOOL:
    case TOK_SMALL_INT:
    case TOK_NUMBER:
    case TOK_STRING:
    {
      parse_literal ();
      break;
    }
    case TOK_OPEN_SQUARE:
    {
      parse_array_literal ();
      break;
    }
    case TOK_OPEN_BRACE:
    {
      parse_object_literal ();
      break;
    }
    case TOK_OPEN_PAREN:
    {
      skip_newlines ();
      if (!token_is (TOK_CLOSE_PAREN))
      {
        parse_expression ();
        token_after_newlines_must_be (TOK_CLOSE_PAREN);
        break;
      }
      /* FALLTHRU */
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }

cleanup:
  STACK_CHECK_USAGE_LHS ();
}

/* member_expression
  : (primary_expression | function_expression | 'new' LT!* member_expression (LT!* '(' LT!* arguments? LT!* ')')
      (LT!* member_expression_suffix)*
  ;

   arguments
  : assignment_expression (LT!* ',' LT!* assignment_expression)*)?
  ;

   member_expression_suffix
  : index_suffix
  | property_reference_suffix
  ;

   index_suffix
  : '[' LT!* expression LT!* ']'
  ;

   property_reference_suffix
  : '.' LT!* Identifier
  ; */
static void
parse_member_expression (void)
{
  // IDX obj, lhs, prop;
  STACK_DECLARE_USAGE (IDX)

  if (is_keyword (KW_FUNCTION))
  {
    parse_function_expression ();
  }
  else if (is_keyword (KW_NEW))
  {
    NEXT (member_expression); // push member

    skip_newlines ();
    parse_argument_list (AL_CONSTRUCT_EXPR, STACK_HEAD (IDX, 1)); // push obj

    STACK_HEAD (IDX, 2) = STACK_HEAD (IDX, 1);
    STACK_DROP (IDX, 1);
  }
  else
  {
    parse_primary_expression ();
  }

  skip_newlines ();
  while (token_is (TOK_OPEN_SQUARE) || token_is (TOK_DOT))
  {
    STACK_PUSH (IDX, next_temp_name ());

    if (token_is (TOK_OPEN_SQUARE))
    {
      NEXT (expression); // push prop
      next_token_must_be (TOK_CLOSE_SQUARE);
    }
    else if (token_is (TOK_DOT))
    {
      skip_newlines ();
      if (!token_is (TOK_NAME))
      {
        parser_fatal (ERR_PARSER);
      }
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (assignment, STACK_HEAD (IDX, 1), OPCODE_ARG_TYPE_STRING, token_data ());
    }
    else
    {
      JERRY_UNREACHABLE ();
    }

    DUMP_OPCODE_3 (prop_getter, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 3), STACK_HEAD (IDX, 1));
    STACK_HEAD (IDX, 3) = STACK_HEAD (IDX, 2);
    skip_newlines ();

    STACK_DROP (IDX, 2);
  }

  lexer_save_token (TOK ());

  STACK_CHECK_USAGE_LHS ();
}

/* call_expression
  : member_expression LT!* arguments (LT!* call_expression_suffix)*
  ;

   call_expression_suffix
  : arguments
  | index_suffix
  | property_reference_suffix
  ;

   arguments
  : '(' LT!* assignment_expression LT!* (',' LT!* assignment_expression * LT!*)* ')'
  ; */
static void
parse_call_expression (void)
{
  // IDX obj, lhs, prop;
  STACK_DECLARE_USAGE (IDX)

  parse_member_expression ();

  skip_newlines ();
  if (!token_is (TOK_OPEN_PAREN))
  {
    lexer_save_token (TOK ());
    goto cleanup;
  }

  parse_argument_list (AL_CALL_EXPR, STACK_HEAD (IDX, 1)); // push lhs
  STACK_HEAD (IDX, 2) = STACK_HEAD (IDX, 1);

  skip_newlines ();
  while (token_is (TOK_OPEN_PAREN) || token_is (TOK_OPEN_SQUARE)
         || token_is (TOK_DOT))
  {
    switch (TOK ().type)
    {
      case TOK_OPEN_PAREN:
      {
        STACK_DROP (IDX, 1);
        parse_argument_list (AL_CALL_EXPR, STACK_HEAD (IDX, 1)); // push lhs
        skip_newlines ();
        break;
      }
      case TOK_OPEN_SQUARE:
      {
        NEXT (expression); // push prop
        next_token_must_be (TOK_CLOSE_SQUARE);

        DUMP_OPCODE_3 (prop_getter, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 3), STACK_HEAD (IDX, 1));
        STACK_DROP (IDX, 1);
        STACK_HEAD (IDX, 2) = STACK_HEAD (IDX, 1);
        skip_newlines ();
        break;
      }
      case TOK_DOT:
      {
        token_after_newlines_must_be (TOK_NAME);
        STACK_PUSH (IDX, token_data ());

        DUMP_OPCODE_3 (prop_getter, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 3), STACK_HEAD (IDX, 1));
        STACK_DROP (IDX, 1);
        STACK_HEAD (IDX, 2) = STACK_HEAD (IDX, 1);
        skip_newlines ();
        break;
      }
      default:
      {
        JERRY_UNREACHABLE ();
      }
    }
  }
  lexer_save_token (TOK ());

  STACK_DROP (IDX, 1);

cleanup:
  STACK_CHECK_USAGE_LHS ();
}

/* left_hand_side_expression
  : call_expression
  | new_expression
  ; */
static void
parse_left_hand_side_expression (void)
{
  STACK_DECLARE_USAGE (IDX)

  parse_call_expression ();

  STACK_CHECK_USAGE_LHS ();
}

/* postfix_expression
  : left_hand_side_expression ('++' | '--')?
  ; */
static void
parse_postfix_expression (void)
{
  // IDX expr, lhs
  STACK_DECLARE_USAGE (IDX)

  parse_left_hand_side_expression (); // push expr

  skip_token ();
  if (token_is (TOK_DOUBLE_PLUS))
  {
    STACK_PUSH (IDX, next_temp_name ());
    DUMP_OPCODE_2 (post_incr, STACK_HEAD (IDX, 1), STACK_HEAD (IDX, 2));
    STACK_HEAD (IDX, 2) = STACK_HEAD (IDX, 1);
    STACK_DROP (IDX, 1);
  }
  else if (token_is (TOK_DOUBLE_MINUS))
  {
    STACK_PUSH (IDX, next_temp_name ());
    DUMP_OPCODE_2 (post_decr, STACK_HEAD (IDX, 1), STACK_HEAD (IDX, 2));
    STACK_HEAD (IDX, 2) = STACK_HEAD (IDX, 1);
    STACK_DROP (IDX, 1);
  }
  else
  {
    lexer_save_token (TOK ());
  }

  STACK_CHECK_USAGE_LHS ();
}

/* unary_expression
  : postfix_expression
  | ('delete' | 'void' | 'typeof' | '++' | '--' | '+' | '-' | '~' | '!') unary_expression
  ; */
static void
parse_unary_expression (void)
{
  // IDX expr, lhs;
  STACK_DECLARE_USAGE (IDX)

  switch (TOK ().type)
  {
    case TOK_DOUBLE_PLUS:
    {
      NEXT (unary_expression);
      DUMP_OPCODE_2 (pre_incr, next_temp_name (), STACK_HEAD (IDX, 1));
      break;
    }
    case TOK_DOUBLE_MINUS:
    {
      NEXT (unary_expression);
      DUMP_OPCODE_2 (pre_decr, next_temp_name (), STACK_HEAD (IDX, 1));
      break;
    }
    case TOK_PLUS:
    {
      STACK_PUSH (IDX, next_temp_name ());
      NEXT (unary_expression);
      integer_zero ();
      DUMP_OPCODE_3 (addition, STACK_HEAD (IDX, 3), STACK_HEAD (IDX, 1), STACK_HEAD (IDX, 2));
      STACK_DROP (IDX, 2);
      break;
    }
    case TOK_MINUS:
    {
      STACK_PUSH (IDX, next_temp_name ());
      NEXT (unary_expression);
      integer_zero ();
      DUMP_OPCODE_3 (substraction, STACK_HEAD (IDX, 3), STACK_HEAD (IDX, 1), STACK_HEAD (IDX, 2));
      STACK_DROP (IDX, 2);
      break;
    }
    case TOK_COMPL:
    {
      STACK_PUSH (IDX, next_temp_name ());
      NEXT (unary_expression);
      DUMP_OPCODE_2 (b_not, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 1));
      STACK_DROP (IDX, 1);
      break;
    }
    case TOK_NOT:
    {
      STACK_PUSH (IDX, next_temp_name ());
      NEXT (unary_expression);
      DUMP_OPCODE_2 (logical_not, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 1));
      STACK_DROP (IDX, 1);
      break;
    }
    case TOK_KEYWORD:
    {
      if (is_keyword (KW_DELETE))
      {
        TODO (/* lhs = delete_var for delete, applied to expression, that is evaluating to Identifier;
                 lhs = delete_prop for 'delete expr[expr]';
                 lhs = true - otherwise; */);
        // DUMP_OPCODE_2 (delete, lhs, expr);
        JERRY_UNIMPLEMENTED ();
      }
      if (is_keyword (KW_VOID))
      {
        JERRY_UNIMPLEMENTED ();
      }
      if (is_keyword (KW_TYPEOF))
      {
        STACK_PUSH (IDX, next_temp_name ());
        NEXT (unary_expression);
        DUMP_OPCODE_2 (typeof, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 1));
        STACK_DROP (IDX, 1);
        break;
      }
      /* FALLTHRU.  */
    }
    default:
    {
      parse_postfix_expression ();
    }
  }

  STACK_CHECK_USAGE_LHS ();
}

#define DUMP_OF(GETOP, EXPR) \
do { \
  STACK_PUSH (IDX, next_temp_name ()); \
  NEXT (EXPR);\
  DUMP_OPCODE_3 (GETOP, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 3), STACK_HEAD (IDX, 1)); \
  STACK_HEAD (IDX, 3) = STACK_HEAD (IDX, 2); \
  STACK_DROP (IDX, 2); \
} while (0)

/* multiplicative_expression
  : unary_expression (LT!* ('*' | '/' | '%') LT!* unary_expression)*
  ; */
static void
parse_multiplicative_expression (void)
{
  // IDX expr1, lhs, expr2;
  STACK_DECLARE_USAGE (IDX)

  parse_unary_expression ();

  skip_newlines ();
  while (true)
  {
    switch (TOK ().type)
    {
      case TOK_MULT: DUMP_OF (multiplication, unary_expression); break;
      case TOK_DIV: DUMP_OF (division, unary_expression); break;
      case TOK_MOD: DUMP_OF (remainder, unary_expression); break;
      default: lexer_save_token (TOK ()); goto cleanup;
    }

    skip_newlines ();
  }

cleanup:
  STACK_CHECK_USAGE_LHS ();
}

/* additive_expression
  : multiplicative_expression (LT!* ('+' | '-') LT!* multiplicative_expression)*
  ; */
static void
parse_additive_expression (void)
{
  // IDX expr1, lhs, expr2;
  STACK_DECLARE_USAGE (IDX)

  parse_multiplicative_expression ();

  skip_newlines ();
  while (true)
  {
    switch (TOK ().type)
    {
      case TOK_PLUS: DUMP_OF (addition, multiplicative_expression); break;
      case TOK_MINUS: DUMP_OF (substraction, multiplicative_expression); break;
      default: lexer_save_token (TOK ()); goto cleanup;
    }

    skip_newlines ();
  }

cleanup:
  STACK_CHECK_USAGE_LHS ();
}

/* shift_expression
  : additive_expression (LT!* ('<<' | '>>' | '>>>') LT!* additive_expression)*
  ; */
static void
parse_shift_expression (void)
{
  // IDX expr1, lhs, expr2;
  STACK_DECLARE_USAGE (IDX)

  parse_additive_expression ();

  skip_newlines ();
  while (true)
  {
    switch (TOK ().type)
    {
      case TOK_LSHIFT: DUMP_OF (b_shift_left, additive_expression); break;
      case TOK_RSHIFT: DUMP_OF (b_shift_right, additive_expression); break;
      case TOK_RSHIFT_EX: DUMP_OF (b_shift_uright, additive_expression); break;
      default: lexer_save_token (TOK ()); goto cleanup;
    }

    skip_newlines ();
  }

cleanup:
  STACK_CHECK_USAGE_LHS ();
}

/* relational_expression
  : shift_expression (LT!* ('<' | '>' | '<=' | '>=' | 'instanceof' | 'in') LT!* shift_expression)*
  ; */
static void
parse_relational_expression (void)
{
  // IDX expr1, lhs, expr2;
  STACK_DECLARE_USAGE (IDX)

  parse_shift_expression ();

  skip_newlines ();
  while (true)
  {
    switch (TOK ().type)
    {
      case TOK_LESS: DUMP_OF (less_than, shift_expression); break;
      case TOK_GREATER: DUMP_OF (greater_than, shift_expression); break;
      case TOK_LESS_EQ: DUMP_OF (less_or_equal_than, shift_expression); break;
      case TOK_GREATER_EQ: DUMP_OF (greater_or_equal_than, shift_expression); break;
      case TOK_KEYWORD:
      {
        if (is_keyword (KW_INSTANCEOF))
        {
          DUMP_OF (instanceof, shift_expression);
          break;
        }
        else if (is_keyword (KW_IN))
        {
          DUMP_OF (in, shift_expression);
          break;
        }
        /* FALLTHROUGH */
      }
      default:
      {
        lexer_save_token (TOK ());
        goto cleanup;
      }
    }

    skip_newlines ();
  }

cleanup:
  STACK_CHECK_USAGE_LHS ();
}

/* equality_expression
  : relational_expression (LT!* ('==' | '!=' | '===' | '!==') LT!* relational_expression)*
  ; */
static void
parse_equality_expression (void)
{
  // IDX expr1, lhs, expr2;
  STACK_DECLARE_USAGE (IDX)

  parse_relational_expression ();

  skip_newlines ();
  while (true)
  {
    switch (TOK ().type)
    {
      case TOK_DOUBLE_EQ: DUMP_OF (equal_value, relational_expression); break;
      case TOK_NOT_EQ: DUMP_OF (not_equal_value, relational_expression); break;
      case TOK_TRIPLE_EQ: DUMP_OF (equal_value_type, relational_expression); break;
      case TOK_NOT_DOUBLE_EQ: DUMP_OF (not_equal_value_type, relational_expression); break;
      default:
      {
        lexer_save_token (TOK ());
        goto cleanup;
      }
    }

    skip_newlines ();
  }

cleanup:
  STACK_CHECK_USAGE_LHS ();
}

#define PARSE_OF(FUNC, EXPR, TOK_TYPE, GETOP) \
static void parse_##FUNC (void) { \
  STACK_DECLARE_USAGE (IDX) \
  parse_##EXPR (); \
  skip_newlines (); \
  while (true) \
  { \
    switch (TOK ().type) \
    { \
      case TOK_##TOK_TYPE: DUMP_OF (GETOP, EXPR); break; \
      default: lexer_save_token (TOK ()); goto cleanup; \
    } \
    skip_newlines (); \
  } \
cleanup: \
  STACK_CHECK_USAGE_LHS () \
}

/* bitwise_and_expression
  : equality_expression (LT!* '&' LT!* equality_expression)*
  ; */
PARSE_OF (bitwise_and_expression, equality_expression, AND, b_and)

/* bitwise_xor_expression
  : bitwise_and_expression (LT!* '^' LT!* bitwise_and_expression)*
  ; */
PARSE_OF (bitwise_xor_expression, bitwise_and_expression, XOR, b_xor)

/* bitwise_or_expression
  : bitwise_xor_expression (LT!* '|' LT!* bitwise_xor_expression)*
  ; */
PARSE_OF (bitwise_or_expression, bitwise_xor_expression, OR, b_or)

/* logical_and_expression
  : bitwise_or_expression (LT!* '&&' LT!* bitwise_or_expression)*
  ; */
PARSE_OF (logical_and_expression, bitwise_or_expression, DOUBLE_AND, logical_and)

/* logical_or_expression
  : logical_and_expression (LT!* '||' LT!* logical_and_expression)*
  ; */
PARSE_OF (logical_or_expression, logical_and_expression, DOUBLE_OR, logical_or)

/* conditional_expression
  : logical_or_expression (LT!* '?' LT!* assignment_expression LT!* ':' LT!* assignment_expression)?
  ; */
static void
parse_conditional_expression (void)
{
  // IDX expr, res, lhs
  STACK_DECLARE_USAGE (IDX)
  STACK_DECLARE_USAGE (U16)
  STACK_DECLARE_USAGE (U8)

  parse_logical_or_expression ();

  skip_newlines ();
  if (token_is (TOK_QUERY))
  {
    DUMP_OPCODE_3 (is_true_jmp_down, STACK_HEAD (IDX, 1), 0, 2);
    STACK_PUSH (IDX, next_temp_name ());
    STACK_PUSH (U16, OPCODE_COUNTER ());
    DUMP_OPCODE_2 (jmp_down, INVALID_VALUE, INVALID_VALUE);

    NEXT (assignment_expression);
    DUMP_OPCODE_3 (assignment, STACK_HEAD (IDX, 2), OPCODE_ARG_TYPE_VARIABLE, STACK_HEAD (IDX, 1));
    token_after_newlines_must_be (TOK_COLON);

    REWRITE_JMP (STACK_HEAD (U16, 1), jmp_down, OPCODE_COUNTER () - STACK_HEAD (U16, 1));
    STACK_HEAD (U16, 1) = OPCODE_COUNTER ();
    DUMP_OPCODE_2 (jmp_down, INVALID_VALUE, INVALID_VALUE);

    STACK_DROP (IDX, 1);
    NEXT (assignment_expression);
    DUMP_OPCODE_3 (assignment, STACK_HEAD (IDX, 2), OPCODE_ARG_TYPE_VARIABLE, STACK_HEAD (IDX, 1));
    REWRITE_JMP (STACK_HEAD (U16, 1), jmp_down, OPCODE_COUNTER () - STACK_HEAD (U16, 1));

    STACK_HEAD (U8, 1) = 1;
    STACK_HEAD (IDX, 3) = STACK_HEAD (IDX, 2);
    STACK_DROP (IDX, 2);
  }
  else
  {
    lexer_save_token (TOK ());
  }

  STACK_CHECK_USAGE (U16);
  STACK_CHECK_USAGE (U8);
  STACK_CHECK_USAGE_LHS ();
}

/* assignment_expression
  : conditional_expression
  | left_hand_side_expression LT!* assignment_operator LT!* assignment_expression
  ; */
static void
parse_assignment_expression (void)
{
  // IDX lhs, rhs;
  STACK_DECLARE_USAGE (IDX)
  STACK_DECLARE_USAGE (U16);
  STACK_DECLARE_USAGE (U8);
  STACK_DECLARE_USAGE (ops);

  STACK_PUSH (U8, 0);

  parse_conditional_expression ();
  if (STACK_HEAD (U8, 1))
  {
    STACK_DROP (U8, 1);
    goto cleanup;
  }

  /* Rewrite prop_getter with nop and generate prop_setter in constructions like:
    a.b = c; */
  if (LAST_OPCODE_IS (prop_getter))
  {
    STACK_PUSH (U16, (opcode_counter_t) (OPCODE_COUNTER () - 1));
    STACK_HEAD (U8, 1) = 1;
  }

  skip_newlines ();
  switch (TOK ().type)
  {
    case TOK_EQ:
    {
      if (STACK_HEAD (U8, 1))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 1)));
        JERRY_ASSERT (STACK_HEAD (ops, 1).op_idx == __op__idx_prop_getter);
        serializer_set_writing_position (--OPCODE_COUNTER ());
        NEXT (assignment_expression);
        DUMP_OPCODE_3 (prop_setter, STACK_HEAD (ops, 1).data.prop_getter.obj,
                       STACK_HEAD (ops, 1).data.prop_getter.prop, STACK_HEAD (IDX, 1));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        NEXT (assignment_expression);
        DUMP_OPCODE_3 (assignment, STACK_HEAD (IDX, 2), OPCODE_ARG_TYPE_VARIABLE,
                       STACK_HEAD (IDX, 1));
      }
      break;
    }
    case TOK_MULT_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_HEAD (U8, 1))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (multiplication, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
        DUMP_OPCODE_3 (prop_setter, STACK_HEAD (ops, 1).data.prop_getter.obj,
                       STACK_HEAD (ops, 1).data.prop_getter.prop, STACK_HEAD (IDX, 2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (multiplication, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
      }
      break;
    }
    case TOK_DIV_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_HEAD (U8, 1))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (division, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
        DUMP_OPCODE_3 (prop_setter, STACK_HEAD (ops, 1).data.prop_getter.obj,
                       STACK_HEAD (ops, 1).data.prop_getter.prop, STACK_HEAD (IDX, 2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (division, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
      }
      break;
    }
    case TOK_MOD_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_HEAD (U8, 1))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (remainder, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
        DUMP_OPCODE_3 (prop_setter, STACK_HEAD (ops, 1).data.prop_getter.obj,
                       STACK_HEAD (ops, 1).data.prop_getter.prop, STACK_HEAD (IDX, 2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (remainder, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
      }
      break;
    }
    case TOK_PLUS_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_HEAD (U8, 1))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (addition, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
        DUMP_OPCODE_3 (prop_setter, STACK_HEAD (ops, 1).data.prop_getter.obj,
                       STACK_HEAD (ops, 1).data.prop_getter.prop, STACK_HEAD (IDX, 2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (addition, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
      }
      break;
    }
    case TOK_MINUS_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_HEAD (U8, 1))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (substraction, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
        DUMP_OPCODE_3 (prop_setter, STACK_HEAD (ops, 1).data.prop_getter.obj,
                       STACK_HEAD (ops, 1).data.prop_getter.prop, STACK_HEAD (IDX, 2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (substraction, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
      }
      break;
    }
    case TOK_LSHIFT_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_HEAD (U8, 1))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (b_shift_left, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
        DUMP_OPCODE_3 (prop_setter, STACK_HEAD (ops, 1).data.prop_getter.obj,
                       STACK_HEAD (ops, 1).data.prop_getter.prop, STACK_HEAD (IDX, 2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (b_shift_left, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
      }
      break;
    }
    case TOK_RSHIFT_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_HEAD (U8, 1))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (b_shift_right, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
        DUMP_OPCODE_3 (prop_setter, STACK_HEAD (ops, 1).data.prop_getter.obj,
                       STACK_HEAD (ops, 1).data.prop_getter.prop, STACK_HEAD (IDX, 2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (b_shift_right, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
      }
      break;
    }
    case TOK_RSHIFT_EX_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_HEAD (U8, 1))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (b_shift_uright, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
        DUMP_OPCODE_3 (prop_setter, STACK_HEAD (ops, 1).data.prop_getter.obj,
                       STACK_HEAD (ops, 1).data.prop_getter.prop, STACK_HEAD (IDX, 2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (b_shift_uright, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
      }
      break;
    }
    case TOK_AND_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_HEAD (U8, 1))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (b_and, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
        DUMP_OPCODE_3 (prop_setter, STACK_HEAD (ops, 1).data.prop_getter.obj,
                       STACK_HEAD (ops, 1).data.prop_getter.prop, STACK_HEAD (IDX, 2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (b_and, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
      }
      break;
    }
    case TOK_XOR_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_HEAD (U8, 1))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (b_xor, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
        DUMP_OPCODE_3 (prop_setter, STACK_HEAD (ops, 1).data.prop_getter.obj,
                       STACK_HEAD (ops, 1).data.prop_getter.prop, STACK_HEAD (IDX, 2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (b_xor, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
      }
      break;
    }
    case TOK_OR_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_HEAD (U8, 1))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (b_or, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
        DUMP_OPCODE_3 (prop_setter, STACK_HEAD (ops, 1).data.prop_getter.obj,
                       STACK_HEAD (ops, 1).data.prop_getter.prop, STACK_HEAD (IDX, 2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (b_or, STACK_HEAD (IDX, 2), STACK_HEAD (IDX, 2),
                       STACK_HEAD (IDX, 1));
      }
      break;
    }
    default:
    {
      if (STACK_HEAD (U8, 1))
      {
        STACK_DROP (U16, 1);
      }
      lexer_save_token (TOK ());
      goto cleanup;
    }
  }

  STACK_DROP (IDX, 1);

cleanup:
  STACK_DROP (U8, 1);

  STACK_CHECK_USAGE (U16);
  STACK_CHECK_USAGE (U8);
  STACK_CHECK_USAGE (ops);
  STACK_CHECK_USAGE_LHS ();
}

/* expression
  : assignment_expression (LT!* ',' LT!* assignment_expression)*
  ;
 */
static void
parse_expression (void)
{
  // IDX expr
  STACK_DECLARE_USAGE (IDX)

  parse_assignment_expression ();

  while (true)
  {
    skip_newlines ();
    if (token_is (TOK_COMMA))
    {
      NEXT (assignment_expression);
      STACK_HEAD (IDX, 2) = STACK_HEAD (IDX, 1);
      STACK_DROP (IDX, 1);
    }
    else
    {
      lexer_save_token (TOK ());
      break;
    }
  }

  STACK_CHECK_USAGE_LHS ();
}

/* variable_declaration
  : Identifier LT!* initialiser?
  ;
   initialiser
  : '=' LT!* assignment_expression
  ; */
static void
parse_variable_declaration (void)
{
  //IDX name, expr;

  STACK_DECLARE_USAGE (IDX)

  current_token_must_be (TOK_NAME);
  STACK_PUSH (IDX, token_data ());
  DUMP_OPCODE_1 (var_decl, STACK_HEAD (IDX, 1));

  skip_newlines ();
  if (token_is (TOK_EQ))
  {
    NEXT (assignment_expression);
    DUMP_OPCODE_3 (assignment, STACK_HEAD (IDX, 2), OPCODE_ARG_TYPE_VARIABLE, STACK_HEAD (IDX, 1));
    STACK_DROP (IDX, 1);
  }
  else
  {
    lexer_save_token (TOK ());
  }

  STACK_DROP (IDX, 1);

  STACK_CHECK_USAGE (IDX);
}

/* variable_declaration_list
  : variable_declaration
    (LT!* ',' LT!* variable_declaration)*
  ; */
static void
parse_variable_declaration_list (bool *several_decls)
{
  STACK_DECLARE_USAGE (IDX)

  while (true)
  {
    parse_variable_declaration ();

    skip_newlines ();
    if (!token_is (TOK_COMMA))
    {
      lexer_save_token (TOK ());
      return;
    }

    skip_newlines ();
    if (several_decls)
    {
      *several_decls = true;
    }
  }

  STACK_CHECK_USAGE (IDX);
}

/* for_statement
  : 'for' LT!* '(' (LT!* for_statement_initialiser_part)? LT!* ';'
    (LT!* expression)? LT!* ';' (LT!* expression)? LT!* ')' LT!* statement
  ;

   for_statement_initialiser_part
  : expression
  | 'var' LT!* variable_declaration_list
  ;

   for_in_statement
  : 'for' LT!* '(' LT!* for_in_statement_initialiser_part LT!* 'in'
    LT!* expression LT!* ')' LT!* statement
  ;

   for_in_statement_initialiser_part
  : left_hand_side_expression
  | 'var' LT!* variable_declaration
  ;*/

static void
parse_for_or_for_in_statement (void)
{
  // IDX stop;
  // U16 cond_oc, body_oc, step_oc, end_oc;
  STACK_DECLARE_USAGE (IDX)
  STACK_DECLARE_USAGE (U16)
  STACK_DECLARE_USAGE (nestings)

  assert_keyword (KW_FOR);
  token_after_newlines_must_be (TOK_OPEN_PAREN);

  skip_newlines ();
  if (token_is (TOK_SEMICOLON))
  {
    goto plain_for;
  }
  /* Both for_statement_initialiser_part and for_in_statement_initialiser_part
     contains 'var'. Check it first.  */
  if (is_keyword (KW_VAR))
  {
    bool several_decls = false;
    skip_newlines ();
    parse_variable_declaration_list (&several_decls);
    if (several_decls)
    {
      token_after_newlines_must_be (TOK_SEMICOLON);
      goto plain_for;
    }
    else
    {
      skip_newlines ();
      if (token_is (TOK_SEMICOLON))
      {
        goto plain_for;
      }
      else if (is_keyword (KW_IN))
      {
        goto for_in;
      }
      else
      {
        parser_fatal (ERR_PARSER);
      }
    }
  }

  /* expression contains left_hand_side_expression.  */
  parse_expression ();
  STACK_DROP (IDX, 1);

  skip_newlines ();
  if (token_is (TOK_SEMICOLON))
  {
    goto plain_for;
  }
  else if (is_keyword (KW_IN))
  {
    goto for_in;
  }
  else
  {
    parser_fatal (ERR_PARSER);
  }

  JERRY_UNREACHABLE ();

plain_for:
  /* Represent loop like

     for (i = 0; i < 10; i++) {
       body;
     }

     as

  11   i = #0;
   cond_oc:
  12   tmp1 = #10;
  13   tmp2 = i < tmp1;
   end_oc:
  14   is_false_jmp_down tmp2, +8 // end of loop
   body_oc:
  15   jmp_down 5 // body
   step_oc:
  16   tmp3 = #1;
  17   tmp4 = i + 1;
  18   i = tmp4;
  19   jmp_up 7; // cond_oc

  20   body

  21   jmp_up 5; // step_oc;
  22   ...
   */
  STACK_PUSH (U16, OPCODE_COUNTER ()); // cond_oc;
  skip_newlines ();
  if (!token_is (TOK_SEMICOLON))
  {
    parse_assignment_expression ();
    next_token_must_be (TOK_SEMICOLON);
  }
  else
  {
    boolean_true ();
  }

  STACK_PUSH (U16, OPCODE_COUNTER ()); // end_oc;
  DUMP_OPCODE_3 (is_false_jmp_down, INVALID_VALUE, INVALID_VALUE, INVALID_VALUE);

  STACK_PUSH (U16, OPCODE_COUNTER ()); // body_oc;
  DUMP_OPCODE_2 (jmp_down, INVALID_VALUE, INVALID_VALUE);

  STACK_PUSH (U16, OPCODE_COUNTER ()); // step_oc;
  skip_newlines ();
  if (!token_is (TOK_CLOSE_PAREN))
  {
    parse_assignment_expression ();
    STACK_DROP (IDX, 1);
    next_token_must_be (TOK_CLOSE_PAREN);
  }
  DUMP_OPCODE_2 (jmp_up, 0, OPCODE_COUNTER () - STACK_HEAD (U16, 4));
  REWRITE_JMP (STACK_HEAD (U16, 2), jmp_down, OPCODE_COUNTER () - STACK_HEAD (U16, 2));

  skip_newlines ();
  push_nesting (NESTING_ITERATIONAL);
  parse_statement ();
  pop_nesting (NESTING_ITERATIONAL);

  DUMP_OPCODE_2 (jmp_up, 0, OPCODE_COUNTER () - STACK_HEAD (U16, 1));
  REWRITE_COND_JMP (STACK_HEAD (U16, 3), is_false_jmp_down, OPCODE_COUNTER () - STACK_HEAD (U16, 3));

  rewrite_rewritable_opcodes (REWRITABLE_CONTINUE, STACK_HEAD (U16, 1));
  rewrite_rewritable_opcodes (REWRITABLE_BREAK, OPCODE_COUNTER ());

  STACK_DROP (IDX, 1);
  STACK_DROP (U16, 4);

  goto cleanup;

for_in:
  JERRY_UNIMPLEMENTED ();

cleanup:
  STACK_CHECK_USAGE (IDX);
  STACK_CHECK_USAGE (U16);
  STACK_CHECK_USAGE (nestings);
}

static void
parse_expression_inside_parens (void)
{
  // IDX expr;
  STACK_DECLARE_USAGE (IDX)

  token_after_newlines_must_be (TOK_OPEN_PAREN);
  NEXT (expression);
  token_after_newlines_must_be (TOK_CLOSE_PAREN);

  STACK_CHECK_USAGE_LHS ();
}

/* statement_list
  : statement (LT!* statement)*
  ; */
static void
parse_statement_list (void)
{
  STACK_DECLARE_USAGE (IDX)

  while (true)
  {
    parse_statement ();

    skip_newlines ();
    while (token_is (TOK_SEMICOLON))
    {
      skip_newlines ();
    }
    if (token_is (TOK_CLOSE_BRACE))
    {
      lexer_save_token (TOK ());
      break;
    }
  }

  STACK_CHECK_USAGE (IDX);
}

/* if_statement
  : 'if' LT!* '(' LT!* expression LT!* ')' LT!* statement (LT!* 'else' LT!* statement)?
  ; */
static void
parse_if_statement (void)
{
  // IDX cond;
  // U16 cond_oc;
  STACK_DECLARE_USAGE (IDX)
  STACK_DECLARE_USAGE (U16)

  assert_keyword (KW_IF);

  parse_expression_inside_parens ();
  STACK_PUSH (U16, OPCODE_COUNTER ());
  DUMP_OPCODE_3 (is_false_jmp_down, INVALID_VALUE, INVALID_VALUE, INVALID_VALUE);

  skip_newlines ();
  parse_statement ();

  skip_newlines ();
  if (is_keyword (KW_ELSE))
  {
    STACK_PUSH (U16, OPCODE_COUNTER ());
    DUMP_OPCODE_2 (jmp_down, INVALID_VALUE, INVALID_VALUE);

    REWRITE_COND_JMP (STACK_HEAD (U16, 2), is_false_jmp_down, OPCODE_COUNTER () - STACK_HEAD (U16, 2));

    skip_newlines ();
    parse_statement ();

    REWRITE_JMP (STACK_HEAD (U16, 1), jmp_down, OPCODE_COUNTER () - STACK_HEAD (U16, 1));

    STACK_DROP (U16, 1);
  }
  else
  {
    REWRITE_COND_JMP (STACK_HEAD (U16, 1), is_false_jmp_down, OPCODE_COUNTER () - STACK_HEAD (U16, 1));
    lexer_save_token (TOK ());
  }

  STACK_DROP (U16, 1);
  STACK_DROP (IDX, 1);

  STACK_CHECK_USAGE (U16);
  STACK_CHECK_USAGE (IDX);
}

/* do_while_statement
  : 'do' LT!* statement LT!* 'while' LT!* '(' expression ')' (LT | ';')!
  ; */
static void
parse_do_while_statement (void)
{
  // IDX cond;
  // U16 loop_oc;
  STACK_DECLARE_USAGE (IDX)
  STACK_DECLARE_USAGE (U16)
  STACK_DECLARE_USAGE (nestings)

  assert_keyword (KW_DO);

  STACK_PUSH (U16, OPCODE_COUNTER ());

  skip_newlines ();
  push_nesting (NESTING_ITERATIONAL);
  parse_statement ();
  pop_nesting (NESTING_ITERATIONAL);

  token_after_newlines_must_be_keyword (KW_WHILE);
  parse_expression_inside_parens ();
  STACK_PUSH (U16, OPCODE_COUNTER ());
  DUMP_OPCODE_3 (is_true_jmp_up, INVALID_VALUE, INVALID_VALUE, INVALID_VALUE);
  REWRITE_COND_JMP (STACK_HEAD(U16, 1), is_true_jmp_up, OPCODE_COUNTER () - STACK_HEAD (U16, 2));

  rewrite_rewritable_opcodes (REWRITABLE_CONTINUE, STACK_HEAD (U16, 2));
  rewrite_rewritable_opcodes (REWRITABLE_BREAK, OPCODE_COUNTER ());

  STACK_DROP (IDX, 1);
  STACK_DROP (U16, 2);

  STACK_CHECK_USAGE (U16);
  STACK_CHECK_USAGE (IDX);
  STACK_CHECK_USAGE (nestings);
}

/* while_statement
  : 'while' LT!* '(' LT!* expression LT!* ')' LT!* statement
  ; */
static void
parse_while_statement (void)
{
  // IDX cond;
  // U16 cond_oc, jmp_oc;
  STACK_DECLARE_USAGE (IDX)
  STACK_DECLARE_USAGE (U16)
  STACK_DECLARE_USAGE (nestings)

  assert_keyword (KW_WHILE);

  STACK_PUSH (U16, OPCODE_COUNTER ()); // cond_oc;
  parse_expression_inside_parens ();
  STACK_PUSH (U16, OPCODE_COUNTER ()); // jmp_oc;
  DUMP_OPCODE_3 (is_false_jmp_down, INVALID_VALUE, INVALID_VALUE, INVALID_VALUE);

  skip_newlines ();
  push_nesting (NESTING_ITERATIONAL);
  parse_statement ();
  pop_nesting (NESTING_ITERATIONAL);

  STACK_PUSH (U16, OPCODE_COUNTER ());
  DUMP_OPCODE_2 (jmp_up, INVALID_VALUE, INVALID_VALUE);
  REWRITE_JMP (STACK_HEAD (U16, 1), jmp_up, OPCODE_COUNTER () - STACK_HEAD (U16, 3));
  REWRITE_COND_JMP (STACK_HEAD (U16, 2), is_false_jmp_down, OPCODE_COUNTER () - STACK_HEAD (U16, 2));

  rewrite_rewritable_opcodes (REWRITABLE_CONTINUE, STACK_HEAD (U16, 3));
  rewrite_rewritable_opcodes (REWRITABLE_BREAK, OPCODE_COUNTER ());

  STACK_DROP (IDX, 1);
  STACK_DROP (U16, 3);

  STACK_CHECK_USAGE (U16);
  STACK_CHECK_USAGE (IDX);
  STACK_CHECK_USAGE (nestings);
}

/* with_statement
  : 'with' LT!* '(' LT!* expression LT!* ')' LT!* statement
  ; */
static void
parse_with_statement (void)
{
  // IDX expr;
  STACK_DECLARE_USAGE (IDX)

  assert_keyword (KW_WITH);
  parse_expression_inside_parens ();

  DUMP_OPCODE_1 (with, STACK_HEAD (IDX, 1));

  skip_newlines ();
  parse_statement ();

  DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_END_WITH, INVALID_VALUE, INVALID_VALUE);

  STACK_DROP (IDX, 1);

  STACK_CHECK_USAGE (IDX);
}

/* case_block
  : '{' LT!* case_clause* LT!* '}'
  | '{' LT!* case_clause* LT!* default_clause LT!* case_clause* LT!* '}'
  ;
   case_clause
  : 'case' LT!* expression LT!* ':' LT!* statement*
  ;
   default_clause
  : 'default' LT!* ':' LT!* statement*
  ; */
/* switch_statement
  : 'switch' LT!* '(' LT!* expression LT!* ')' LT!* '{' LT!* case_block LT!* '}'
  ; */
static void
parse_switch_statement (void)
{
  JERRY_UNIMPLEMENTED ();
}

/* catch_clause
  : 'catch' LT!* '(' LT!* Identifier LT!* ')' LT!* '{' LT!* statement_list LT!* '}'
  ; */
static void
parse_catch_clause (void)
{
  // IDX ex_name;
  // U16 catch_oc;
  STACK_DECLARE_USAGE (IDX)
  STACK_DECLARE_USAGE (U16)

  assert_keyword (KW_CATCH);

  token_after_newlines_must_be (TOK_OPEN_PAREN);
  token_after_newlines_must_be (TOK_NAME);
  STACK_PUSH (IDX, token_data ());
  token_after_newlines_must_be (TOK_CLOSE_PAREN);

  STACK_PUSH (U16, OPCODE_COUNTER ());
  DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_CATCH, INVALID_VALUE, INVALID_VALUE);
  DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_CATCH_EXCEPTION_IDENTIFIER, STACK_HEAD (IDX, 1), INVALID_VALUE);

  token_after_newlines_must_be (TOK_OPEN_BRACE);
  skip_newlines ();
  parse_statement_list ();
  next_token_must_be (TOK_CLOSE_BRACE);

  rewrite_meta_opcode_counter (STACK_HEAD (U16, 1), OPCODE_META_TYPE_CATCH);

  STACK_DROP (U16, 1);
  STACK_DROP (IDX, 1);

  STACK_CHECK_USAGE (IDX);
  STACK_CHECK_USAGE (U16);
}

/* finally_clause
  : 'finally' LT!* '{' LT!* statement_list LT!* '}'
  ; */
static void
parse_finally_clause (void)
{
  // U16 finally_oc;
  STACK_DECLARE_USAGE (U16)

  assert_keyword (KW_FINALLY);

  STACK_PUSH (U16, OPCODE_COUNTER ());
  DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_FINALLY, INVALID_VALUE, INVALID_VALUE);

  token_after_newlines_must_be (TOK_OPEN_BRACE);
  skip_newlines ();
  parse_statement_list ();
  next_token_must_be (TOK_CLOSE_BRACE);

  rewrite_meta_opcode_counter (STACK_HEAD (U16, 1), OPCODE_META_TYPE_FINALLY);

  STACK_DROP (U16, 1);

  STACK_CHECK_USAGE (U16);
}

/* try_statement
  : 'try' LT!* '{' LT!* statement_list LT!* '}' LT!* (finally_clause | catch_clause (LT!* finally_clause)?)
  ; */
static void
parse_try_statement (void)
{
  // U16 try_oc;
  STACK_DECLARE_USAGE (U16)

  assert_keyword (KW_TRY);

  STACK_PUSH (U16, OPCODE_COUNTER ());
  DUMP_OPCODE_2 (try, INVALID_VALUE, INVALID_VALUE);

  token_after_newlines_must_be (TOK_OPEN_BRACE);
  skip_newlines ();
  parse_statement_list ();
  next_token_must_be (TOK_CLOSE_BRACE);

  REWRITE_TRY (STACK_HEAD (U16, 1));

  token_after_newlines_must_be (TOK_KEYWORD);
  if (is_keyword (KW_CATCH))
  {
    parse_catch_clause ();

    skip_newlines ();
    if (is_keyword (KW_FINALLY))
    {
      parse_finally_clause ();
    }
    else
    {
      lexer_save_token (TOK ());
    }
  }
  else if (is_keyword (KW_FINALLY))
  {
    parse_finally_clause ();
  }
  else
  {
    parser_fatal (ERR_PARSER);
  }

  DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_END_TRY_CATCH_FINALLY, INVALID_VALUE, INVALID_VALUE);

  STACK_DROP (U16, 1);

  STACK_CHECK_USAGE (U16);
}

static void
insert_semicolon (void)
{
  skip_token ();
  JERRY_ASSERT (token_is (TOK_SEMICOLON) || token_is (TOK_NEWLINE));
}

/* statement
  : statement_block
  | variable_statement
  | empty_statement
  | if_statement
  | iteration_statement
  | continue_statement
  | break_statement
  | return_statement
  | with_statement
  | labelled_statement
  | switch_statement
  | throw_statement
  | try_statement
  | expression_statement
  ;

   statement_block
  : '{' LT!* statement_list? LT!* '}'
  ;

   variable_statement
  : 'var' LT!* variable_declaration_list (LT | ';')!
  ;

   empty_statement
  : ';'
  ;

   expression_statement
  : expression (LT | ';')!
  ;

   iteration_statement
  : do_while_statement
  | while_statement
  | for_statement
  | for_in_statement
  ;

   continue_statement
  : 'continue' Identifier? (LT | ';')!
  ;

   break_statement
  : 'break' Identifier? (LT | ';')!
  ;

   return_statement
  : 'return' expression? (LT | ';')!
  ;

   switchStatement
  : 'switch' LT!* '(' LT!* expression LT!* ')' LT!* caseBlock
  ;

   throw_statement
  : 'throw' expression (LT | ';')!
  ;

   try_statement
  : 'try' LT!* '{' LT!* statement_list LT!* '}' LT!* (finally_clause | catch_clause (LT!* finally_clause)?)
  ;*/
static void
parse_statement (void)
{
  reset_temp_name ();

  STACK_DECLARE_USAGE (IDX)
  STACK_DECLARE_USAGE (toks)

  if (token_is (TOK_CLOSE_BRACE))
  {
    lexer_save_token (TOK ());
    goto cleanup;
  }
  if (token_is (TOK_OPEN_BRACE))
  {
    skip_newlines ();
    if (!token_is (TOK_CLOSE_BRACE))
    {
      parse_statement_list ();
      next_token_must_be (TOK_CLOSE_BRACE);
    }
    goto cleanup;
  }
  if (is_keyword (KW_VAR))
  {
    skip_newlines ();
    parse_variable_declaration_list (NULL);
    goto cleanup;
  }
  if (token_is (TOK_SEMICOLON))
  {
    goto cleanup;
  }
  if (is_keyword (KW_IF))
  {
    parse_if_statement ();
    goto cleanup;
  }
  if (is_keyword (KW_DO))
  {
    parse_do_while_statement ();
    goto cleanup;
  }
  if (is_keyword (KW_WHILE))
  {
    parse_while_statement ();
    goto cleanup;
  }
  if (is_keyword (KW_FOR))
  {
    parse_for_or_for_in_statement ();
    goto cleanup;
  }
  if (is_keyword (KW_CONTINUE))
  {
    uint8_t *temp = mem_heap_alloc_block (1, MEM_HEAP_ALLOC_SHORT_TERM);
    temp[0] = NESTING_ITERATIONAL;
    must_be_inside_but_not_in (temp, 1, NESTING_FUNCTION);
    add_to_rewritable_opcodes (REWRITABLE_CONTINUE, OPCODE_COUNTER ());
    mem_heap_free_block (temp);
    DUMP_OPCODE_2 (jmp_up, INVALID_VALUE, INVALID_VALUE);
    goto cleanup;
  }
  if (is_keyword (KW_BREAK))
  {
    uint8_t *temp = mem_heap_alloc_block (2, MEM_HEAP_ALLOC_SHORT_TERM);
    temp[0] = NESTING_ITERATIONAL;
    temp[1] = NESTING_SWITCH;
    must_be_inside_but_not_in (temp, 2, NESTING_FUNCTION);
    add_to_rewritable_opcodes (REWRITABLE_BREAK, opcode_counter);
    mem_heap_free_block (temp);
    DUMP_OPCODE_2 (jmp_down, INVALID_VALUE, INVALID_VALUE);
    goto cleanup;
  }
  if (is_keyword (KW_RETURN))
  {
    skip_token ();
    if (!token_is (TOK_SEMICOLON) && !token_is (TOK_NEWLINE))
    {
      parse_expression ();
      DUMP_OPCODE_1 (retval, STACK_HEAD (IDX, 1));
      STACK_DROP (IDX, 1);
      insert_semicolon ();
      goto cleanup;
    }
    else
    {
      DUMP_VOID_OPCODE (ret);
      goto cleanup;
    }
  }
  if (is_keyword (KW_WITH))
  {
    parse_with_statement ();
    goto cleanup;
  }
  if (is_keyword (KW_SWITCH))
  {
    parse_switch_statement ();
    goto cleanup;
  }
  if (is_keyword (KW_THROW))
  {
    skip_token ();
    parse_expression ();
    insert_semicolon ();

    DUMP_OPCODE_1 (throw, STACK_HEAD (IDX, 1));
    STACK_DROP (IDX, 1);
    goto cleanup;
  }
  if (is_keyword (KW_TRY))
  {
    parse_try_statement ();
    goto cleanup;
  }
  if (token_is (TOK_NAME))
  {
    STACK_PUSH (toks, TOK ());
    skip_newlines ();
    if (token_is (TOK_COLON))
    {
      // STMT_LABELLED;
      JERRY_UNIMPLEMENTED ();
    }
    else
    {
      lexer_save_token (TOK ());
      STACK_POP (toks, TOK());
      parse_expression ();
      STACK_DROP (IDX, 1);
      goto cleanup;
    }
  }
  else
  {
    parse_expression ();
    STACK_DROP (IDX, 1);
    goto cleanup;
  }

cleanup:
  STACK_CHECK_USAGE (IDX);
  STACK_CHECK_USAGE (toks);
}

/* source_element
  : function_declaration
  | statement
  ; */
static void
parse_source_element (void)
{
  STACK_DECLARE_USAGE (IDX)

  if (is_keyword (KW_FUNCTION))
  {
    parse_function_declaration ();
  }
  else
  {
    parse_statement ();
  }

  STACK_CHECK_USAGE (IDX);
}

/* source_element_list
  : source_element (LT!* source_element)*
  ; */
static void
parse_source_element_list (void)
{
  // U16 reg_var_decl_loc;
  STACK_DECLARE_USAGE (U16)
  STACK_DECLARE_USAGE (IDX)
  STACK_DECLARE_USAGE (temp_names)

  start_new_scope ();
  STACK_PUSH (U16, OPCODE_COUNTER ());
  DUMP_OPCODE_2 (reg_var_decl, MIN_TEMP_NAME (), INVALID_VALUE);

  while (!token_is (TOK_EOF) && !token_is (TOK_CLOSE_BRACE))
  {
    parse_source_element ();
    skip_newlines ();
  }
  lexer_save_token (TOK ());
  REWRITE_OPCODE_2 (STACK_HEAD (U16, 1), reg_var_decl, MIN_TEMP_NAME (), MAX_TEMP_NAME ());
  finish_scope ();

  STACK_DROP (U16, 1);

  STACK_CHECK_USAGE (U16);
  STACK_CHECK_USAGE (IDX);
  STACK_CHECK_USAGE (temp_names);
}

/* program
  : LT!* source_element_list LT!* EOF!
  ; */
void
parser_parse_program (void)
{
  STACK_DECLARE_USAGE (IDX)

  skip_newlines ();
  parse_source_element_list ();

  skip_newlines ();
  JERRY_ASSERT (token_is (TOK_EOF));
  DUMP_OPCODE_1 (exitval, 0);

  serializer_adjust_strings ();

  STACK_CHECK_USAGE (IDX);
}

void
parser_init (const char *source, size_t source_size, bool show_opcodes)
{
  lexer_init (source, source_size, show_opcodes);
  serializer_init (show_opcodes);

  lexer_run_first_pass ();

  lexer_adjust_num_ids ();

  serializer_dump_strings_and_nums (lexer_get_strings (), lexer_get_strings_count (),
                                    lexer_get_nums (), lexer_get_nums_count ());

  STACK_INIT (uint8_t, U8);
  STACK_INIT (uint8_t, IDX);
  STACK_INIT (uint8_t, nestings);
  STACK_INIT (uint8_t, temp_names);
  STACK_INIT (token, toks);
  STACK_INIT (opcode_t, ops);
  STACK_INIT (uint16_t, U16);
  STACK_INIT (opcode_counter_t, rewritable_continue);
  STACK_INIT (opcode_counter_t, rewritable_break);

  HASH_INIT (intrinsics, 1);

  fill_intrinsics ();

  MAX_TEMP_NAME () = TEMP_NAME () = MIN_TEMP_NAME () = lexer_get_reserved_ids_count ();
  OPCODE_COUNTER () = 0;
}

void
parser_free (void)
{
  STACK_FREE (U8);
  STACK_FREE (IDX);
  STACK_FREE (nestings);
  STACK_FREE (temp_names);
  STACK_FREE (toks);
  STACK_FREE (ops);
  STACK_FREE (U16);
  STACK_FREE (rewritable_continue);
  STACK_FREE (rewritable_break);

  HASH_FREE (intrinsics);

  serializer_free ();
  lexer_free ();
}

void
parser_fatal (jerry_status_t code)
{
  __printf ("FATAL: %d\n", code);
  lexer_dump_buffer_state ();

  jerry_exit (code);
}
