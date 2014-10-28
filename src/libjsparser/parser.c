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
#include "opcodes-native-call.h"
#include "parse-error.h"
#include "scopes-tree.h"
#include "ecma-helpers.h"

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
  union
  {
    void (*fun1) (idx_t);
  }
  funs;
  uint8_t args_count;
}
intrinsic_dumper;

typedef enum
{
  PROP_DATA,
  PROP_SET,
  PROP_GET,
  VARG
}
prop_type;

typedef struct
{
  lp_string str;
  bool was_num;
}
allocatable_string;

typedef struct
{
  prop_type type;
  allocatable_string str;
}
prop_as_str_or_varg;

#define NESTING_ITERATIONAL 1
#define NESTING_SWITCH      2
#define NESTING_FUNCTION    3

#define GLOBAL(NAME, VAR) STACK_ELEMENT (NAME, VAR)

enum
{
  no_in,
  U8_global_size
};
STATIC_STACK (U8, uint8_t, uint8_t)

enum
{
  native_calls = OPCODE_NATIVE_CALL__COUNT,
  this_arg,
  prop,
  IDX_global_size
};
STATIC_STACK (IDX, uint8_t, uint8_t)

#define THIS_ARG() STACK_ELEMENT (IDX, this_arg)
#define SET_THIS_ARG(S) STACK_SET_ELEMENT (IDX, this_arg, S)
#define PROP() STACK_ELEMENT (IDX, prop)
#define SET_PROP(S) STACK_SET_ELEMENT (IDX, prop, S)

#define ID(I) STACK_HEAD(IDX, I)

enum
{
  nestings_global_size
};
STATIC_STACK (nestings, uint8_t, uint8_t)

enum
{
  scopes_global_size
};
STATIC_STACK (scopes, uint8_t, scopes_tree)

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

#define SET_MAX_TEMP_NAME(S) \
do { STACK_SET_ELEMENT (temp_names, max_temp_name, S); } while (0)

#define MIN_TEMP_NAME() \
GLOBAL(temp_names, min_temp_name)

#define SET_MIN_TEMP_NAME(S) \
do { STACK_SET_ELEMENT (temp_names, min_temp_name, S); } while (0)

#define TEMP_NAME() \
GLOBAL(temp_names, temp_name)

#define SET_TEMP_NAME(S) \
do { STACK_SET_ELEMENT (temp_names, temp_name, S); } while (0)

enum
{
  tok = 0,
  toks_global_size
};
STATIC_STACK (toks, uint8_t, token)

#define TOK() \
GLOBAL(toks, tok)

#define SET_TOK(S) \
do { STACK_SET_ELEMENT (toks, tok, S); } while (0)

enum
{
  ops_global_size
};
STATIC_STACK (ops, uint8_t, opcode_t)

enum
{
  locs_global_size
};
STATIC_STACK (locs, uint8_t, locus)

enum
{
  opcode_counter = 0,
  U16_global_size
};
STATIC_STACK (U16, uint8_t, uint16_t)

#define OPCODE_COUNTER() \
GLOBAL(U16, opcode_counter)

#define SET_OPCODE_COUNTER(S) \
do { STACK_SET_ELEMENT (U16, opcode_counter, S); } while (0)

#define INCR_OPCODE_COUNTER() \
do { STACK_INCR_ELEMENT(U16, opcode_counter); } while (0)

#define DECR_OPCODE_COUNTER() \
do { STACK_DECR_ELEMENT(U16, opcode_counter); } while (0)

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

enum
{
  strings_global_size
};
STATIC_STACK (strings, uint8_t, allocatable_string)

enum
{
  props_global_size
};
STATIC_STACK (props, uint8_t, prop_as_str_or_varg)

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
  serializer_dump_opcode (getop_##GETOP ()); \
  INCR_OPCODE_COUNTER(); \
} while (0)

#define DUMP_OPCODE_1(GETOP, OP1) \
do { \
  JERRY_ASSERT (0+OP1 <= 255); \
  serializer_dump_opcode (getop_##GETOP ((idx_t) (OP1))); \
  INCR_OPCODE_COUNTER(); \
} while (0)

#define DUMP_OPCODE_2(GETOP, OP1, OP2) \
do { \
  JERRY_ASSERT (0+OP1 <= 255); \
  JERRY_ASSERT (0+OP2 <= 255); \
  serializer_dump_opcode (getop_##GETOP ((idx_t) (OP1), (idx_t) (OP2))); \
  INCR_OPCODE_COUNTER(); \
} while (0)

#define DUMP_OPCODE_3(GETOP, OP1, OP2, OP3) \
do { \
  JERRY_ASSERT (0+OP1 <= 255); \
  JERRY_ASSERT (0+OP2 <= 255); \
  JERRY_ASSERT (0+OP3 <= 255); \
  serializer_dump_opcode (getop_##GETOP ((idx_t) (OP1), (idx_t) (OP2), (idx_t) (OP3))); \
  INCR_OPCODE_COUNTER(); \
} while (0)

#define REWRITE_VOID_OPCODE(OC, GETOP) \
do { \
  serializer_rewrite_opcode (OC, getop_##GETOP ()); \
} while (0)

#define REWRITE_OPCODE_1(OC, GETOP, OP1) \
do { \
  JERRY_ASSERT (0+OP1 <= 255); \
  serializer_rewrite_opcode ((opcode_counter_t) (OC), getop_##GETOP ((idx_t) (OP1))); \
} while (0)

#define REWRITE_OPCODE_2(OC, GETOP, OP1, OP2) \
do { \
  JERRY_ASSERT (0+OP1 <= 255); \
  JERRY_ASSERT (0+OP2 <= 255); \
  serializer_rewrite_opcode ((opcode_counter_t) (OC), getop_##GETOP ((idx_t) (OP1), (idx_t) (OP2))); \
} while (0)

#define REWRITE_OPCODE_3(OC, GETOP, OP1, OP2, OP3) \
do { \
  JERRY_ASSERT (0+OP1 <= 255); \
  JERRY_ASSERT (0+OP2 <= 255); \
  JERRY_ASSERT (0+OP3 <= 255); \
  serializer_rewrite_opcode ((opcode_counter_t) (OC), getop_##GETOP ((idx_t) (OP1), (idx_t) (OP2), (idx_t) (OP3))); \
} while (0)

#define REWRITE_COND_JMP(OC, GETOP, DIFF) \
do { \
  JERRY_ASSERT (0+DIFF <= 256*256 - 1); \
  STACK_DECLARE_USAGE (IDX); \
  JERRY_STATIC_ASSERT (sizeof (idx_t) == 1); \
  STACK_PUSH (IDX, (idx_t) ((DIFF) >> JERRY_BITSINBYTE)); \
  STACK_PUSH (IDX, (idx_t) ((DIFF) & ((1 << JERRY_BITSINBYTE) - 1))); \
  JERRY_ASSERT ((DIFF) == calc_opcode_counter_from_idx_idx (ID(2), ID(1))); \
  serializer_rewrite_opcode (OC, getop_##GETOP (ID(3), ID(2), ID(1))); \
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
  JERRY_ASSERT ((DIFF) == calc_opcode_counter_from_idx_idx (ID(2), ID(1))); \
  serializer_rewrite_opcode (OC, getop_##GETOP (ID(2), ID(1))); \
  STACK_DROP (IDX, 2); \
  STACK_CHECK_USAGE (IDX); \
} while (0)

#define REWRITE_TRY(OC) \
do { \
  STACK_DECLARE_USAGE (IDX) \
  JERRY_STATIC_ASSERT (sizeof (idx_t) == 1); \
  STACK_PUSH (IDX, (idx_t) ((OPCODE_COUNTER () - OC) >> JERRY_BITSINBYTE)); \
  STACK_PUSH (IDX, (idx_t) ((OPCODE_COUNTER () - OC) & ((1 << JERRY_BITSINBYTE) - 1))); \
  JERRY_ASSERT ((OPCODE_COUNTER () - OC) \
                == calc_opcode_counter_from_idx_idx (ID(2), ID(1))); \
  serializer_rewrite_opcode ((OC), getop_try (ID(2), ID(1))); \
  STACK_DROP (IDX, 2); \
  STACK_CHECK_USAGE (IDX); \
} while (0)

#define EMIT_ERROR(MESSAGE) PARSE_ERROR(MESSAGE, TOK().loc)
#define EMIT_SORRY(MESSAGE) PARSE_SORRY(MESSAGE, TOK().loc)
#define EMIT_ERROR_VARG(MESSAGE, ...) PARSE_ERROR_VARG(MESSAGE, TOK().loc, __VA_ARGS__)

#define NESTING_TO_STRING(I) (I == NESTING_FUNCTION \
                                ? "function" \
                                : I == NESTING_ITERATIONAL \
                                  ? "iterational" \
                                  : I == NESTING_SWITCH \
                                    ? "switch" : "unknown")

#define OPCODE_IS(OP, ID) (OP.op_idx == __op__idx_##ID)

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
static void parse_source_element_list (bool);
static uint8_t parse_argument_list (argument_list_type, idx_t);
static void skip_braces (void);
static void parse_logical_expression (bool);

static uint8_t
lp_string_hash (lp_string str)
{
  return str.length % INTRINSICS_COUNT;
}

static idx_t
next_temp_name (void)
{
  idx_t res = TEMP_NAME ();
  STACK_INCR_ELEMENT (temp_names, temp_name);
  if (MAX_TEMP_NAME () < TEMP_NAME ())
  {
    SET_MAX_TEMP_NAME (TEMP_NAME ());
  }
  return res;
}

static void
start_new_scope (void)
{
  STACK_PUSH (temp_names, MAX_TEMP_NAME());
  SET_MAX_TEMP_NAME (MIN_TEMP_NAME ());
}

static void
finish_scope (void)
{
  SET_TEMP_NAME (STACK_HEAD (temp_names, 1));
  STACK_DROP (temp_names, 1);
  SET_MAX_TEMP_NAME (TEMP_NAME ());
}

static void
reset_temp_name (void)
{
  SET_TEMP_NAME (MIN_TEMP_NAME ());
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
#define J() STACK_TOP (U8)
#define SET_I(S) STACK_SET_HEAD (U8, 2, (uint8_t) (S))
#define SET_J(S) STACK_SET_HEAD (U8, 1, (uint8_t) (S))

  if (STACK_SIZE(nestings) == 0)
  {
    EMIT_ERROR ("Shall be inside a nesting");
  }

  SET_I(STACK_SIZE(nestings));
  while (I() != 0)
  {
    if (STACK_ELEMENT (nestings, I() - 1) == not_in)
    {
      EMIT_ERROR_VARG ("Shall not be inside a '%s' nesting", NESTING_TO_STRING(not_in));
    }

    SET_J(0);
    while (J() < insides_count)
    {
      if (STACK_ELEMENT (nestings, I() - 1) == inside[J()])
      {
        goto cleanup;
      }
      SET_J(J()+1);
    }
    SET_I(I()-1);
  }

  switch (insides_count)
  {
    case 1: EMIT_ERROR_VARG ("Shall be inside a '%s' nesting", NESTING_TO_STRING(inside[0])); break;
    case 2: EMIT_ERROR_VARG ("Shall be inside '%s' or '%s' nestings",
                             NESTING_TO_STRING(inside[0]), NESTING_TO_STRING(inside[1])); break;
    default: JERRY_UNREACHABLE ();
  }

cleanup:
  STACK_DROP (U8, 2);
#undef I
#undef J
#undef SET_I
#undef SET_J
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
  SET_TOK(lexer_next_token ());
}

static void
assert_keyword (keyword kw)
{
  if (!token_is (TOK_KEYWORD) || token_data () != kw)
  {
    EMIT_ERROR_VARG ("Expected keyword '%s'", lexer_keyword_to_string (kw));
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
    EMIT_ERROR_VARG ("Expected '%s' token", lexer_token_type_to_string (tt));
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
    EMIT_ERROR_VARG ("Expected '%s' token", lexer_token_type_to_string (tt));
  }
}

static void
token_after_newlines_must_be (token_type tt)
{
  skip_newlines ();
  if (!token_is (tt))
  {
    EMIT_ERROR_VARG ("Expected '%s' token", lexer_token_type_to_string (tt));
  }
}

static void
token_after_newlines_must_be_keyword (keyword kw)
{
  skip_newlines ();
  if (!is_keyword (kw))
  {
    EMIT_ERROR_VARG ("Expected keyword '%s'", lexer_keyword_to_string (kw));
  }
}

static void
boolean_true (void)
{
  STACK_DECLARE_USAGE (IDX)

  STACK_PUSH (IDX, next_temp_name ());
  DUMP_OPCODE_3 (assignment, ID(1), OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_TRUE);

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
rewrite_breaks (opcode_counter_t break_oc, opcode_counter_t dest_oc)
{
  REWRITE_JMP (break_oc, jmp_down, dest_oc - break_oc);
}

static void
rewrite_continues (opcode_counter_t cont_oc, opcode_counter_t dest_oc)
{
  if (cont_oc > dest_oc)
  {
    REWRITE_JMP (cont_oc, jmp_up, cont_oc - dest_oc);
  }
  else
  {
    // in case of do-while loop we must jump to condition
    REWRITE_JMP (cont_oc, jmp_down, dest_oc - cont_oc);
  }
}

static void
rewrite_rewritable_opcodes (rewritable_opcode_type type, uint8_t from, opcode_counter_t oc)
{
  STACK_DECLARE_USAGE (U8)

  STACK_PUSH (U8, 0);

  switch (type)
  {
    case REWRITABLE_BREAK:
    {
      STACK_ITERATE_VARG (rewritable_break, rewrite_breaks, from, oc);
      STACK_DROP (rewritable_break, STACK_SIZE (rewritable_break) - from);
      break;
    }
    case REWRITABLE_CONTINUE:
    {
      STACK_ITERATE_VARG (rewritable_continue, rewrite_continues, from, oc);
      STACK_DROP (rewritable_continue, STACK_SIZE (rewritable_continue) - from);
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
    .str = (ecma_char_t *) "assert"
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
  if (obj < STACK_TOP (U8))
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

static bool
is_native_call (idx_t obj)
{
  if (obj >= lexer_get_strings_count ())
  {
    return false;
  }

  for (uint8_t i = 0; i < OPCODE_NATIVE_CALL__COUNT; i++)
  {
    if (STACK_ELEMENT (IDX, i) == obj)
    {
      return true;
    }
  }
  return false;
}

static idx_t
name_to_native_call_id (idx_t obj)
{
  JERRY_ASSERT (obj < lexer_get_strings_count ());

  for (uint8_t i = 0; i < OPCODE_NATIVE_CALL__COUNT; i++)
  {
    if (STACK_ELEMENT (IDX, i) == obj)
    {
      return i;
    }
  }

  JERRY_UNREACHABLE ();
}

static void
free_allocatable_string (prop_as_str_or_varg p)
{
  if (p.str.was_num)
  {
    mem_heap_free_block ((uint8_t *) p.str.str.str);
  }
}

static allocatable_string
create_allocatable_string_from_num_uid (idx_t uid)
{
  ecma_char_t *str = mem_heap_alloc_block (ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER * sizeof (ecma_char_t),
                                           MEM_HEAP_ALLOC_SHORT_TERM);
  ecma_number_to_zt_string (lexer_get_num_by_id (uid), str,
                            ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER * sizeof (ecma_char_t));

  return (allocatable_string)
  {
    .was_num = true,
    .str = (lp_string)
    {
      .length = ecma_zt_string_length (str),
      .str = str
    }
  };
}

static allocatable_string
create_allocatable_string_from_literal (idx_t uid)
{
  return (allocatable_string)
  {
    .was_num = false,
    .str = lexer_get_string_by_id (uid)
  };
}

static allocatable_string
create_allocatable_string_from_small_int (idx_t uid)
{
  const uint8_t chars_need = 4; /* Max is 255.  */
  uint8_t index = 0;
  ecma_char_t *str = mem_heap_alloc_block (chars_need * sizeof (ecma_char_t),
                                           MEM_HEAP_ALLOC_SHORT_TERM);
  if (uid >= 100)
  {
    str[index++] = (ecma_char_t) (uid/100 + '0');
    uid %= 100;
  }
  if (uid >= 10)
  {
    str[index++] = (ecma_char_t) (uid/10 + '0');
  }
  str[index++] = (ecma_char_t) (uid%10 + '0');
  str[index] = ECMA_CHAR_NULL;

  return (allocatable_string)
  {
    .was_num = true,
    .str = (lp_string)
    {
      .length = index,
      .str = str
    }
  };
}

static prop_as_str_or_varg
create_prop_as_str_or_varg (allocatable_string str, prop_type type)
{
  return (prop_as_str_or_varg)
  {
    .type = type,
    .str = str
  };
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
    {
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (assignment, ID(1), OPCODE_ARG_TYPE_STRING, token_data ());
      STACK_PUSH (strings, create_allocatable_string_from_literal (token_data ()));
      break;
    }
    case TOK_NUMBER:
    {
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (assignment, ID(1), OPCODE_ARG_TYPE_NUMBER, token_data ());
      STACK_PUSH (strings, create_allocatable_string_from_num_uid (token_data ()));
      break;
    }
    case TOK_SMALL_INT:
    {
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (assignment, ID(1), OPCODE_ARG_TYPE_SMALLINT, token_data ());
      STACK_PUSH (strings, create_allocatable_string_from_small_int (token_data ()));
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

  STACK_PUSH (props, create_prop_as_str_or_varg (STACK_TOP (strings), PROP_DATA));
  STACK_DROP (strings, 1);

  STACK_DROP (IDX, 2);
  STACK_CHECK_USAGE (IDX);
}

static void
rewrite_meta_opcode_counter_set_oc (opcode_counter_t meta_oc, opcode_meta_type type, opcode_counter_t oc)
{
  // IDX oc_idx_1, oc_idx_2
  STACK_DECLARE_USAGE (IDX)

  JERRY_STATIC_ASSERT (sizeof (idx_t) == 1);

  STACK_PUSH (IDX, (idx_t) (oc >> JERRY_BITSINBYTE));
  STACK_PUSH (IDX, (idx_t) (oc & ((1 << JERRY_BITSINBYTE) - 1)));

  JERRY_ASSERT (oc == calc_opcode_counter_from_idx_idx (ID(2), ID(1)));

  REWRITE_OPCODE_3 (meta_oc, meta, type, ID(2), ID(1));

  STACK_DROP (IDX, 2);

  STACK_CHECK_USAGE (IDX);
}

static void
rewrite_meta_opcode_counter (opcode_counter_t meta_oc, opcode_meta_type type)
{
  rewrite_meta_opcode_counter_set_oc (meta_oc, type, (opcode_counter_t) (OPCODE_COUNTER () - meta_oc));
}

static void
generate_tmp_for_left_arg (void)
{
  STACK_DECLARE_USAGE (IDX);
  STACK_PUSH (IDX, next_temp_name ());
  DUMP_OPCODE_3 (assignment, ID(1), OPCODE_ARG_TYPE_VARIABLE, ID(2));
  STACK_SWAP (IDX);
  STACK_DROP (IDX, 1);
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
  STACK_DECLARE_USAGE (toks)
  STACK_DECLARE_USAGE (U8)
  STACK_DECLARE_USAGE (strings)

  if (!token_is (TOK_NAME))
  {
    parse_property_name_and_value ();
    goto cleanup;
  }

  if (lp_string_equal_s (lexer_get_string_by_id (token_data ()), "get"))
  {
    STACK_PUSH (toks, TOK ());
    skip_newlines ();
    if (token_is (TOK_COLON))
    {
      lexer_save_token (TOK ());
      SET_TOK (STACK_TOP (toks));
      STACK_DROP (toks, 1);
      goto simple_prop;
    }
    STACK_DROP (toks, 1);
    // name, lhs
    parse_property_name (); // push name
    STACK_PUSH (props, create_prop_as_str_or_varg (STACK_TOP (strings), PROP_GET));
    STACK_DROP (strings, 1);

    skip_newlines ();
    parse_argument_list (AL_FUNC_EXPR, next_temp_name ()); // push lhs

    STACK_PUSH (U16, OPCODE_COUNTER ());
    DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_FUNCTION_END, INVALID_VALUE, INVALID_VALUE);

    token_after_newlines_must_be (TOK_OPEN_BRACE);

    STACK_PUSH (U8, scopes_tree_strict_mode (STACK_TOP (scopes)) ? 1 : 0);
    scopes_tree_set_strict_mode (STACK_TOP (scopes), false);

    skip_newlines ();
    push_nesting (NESTING_FUNCTION);
    parse_source_element_list (false);
    pop_nesting (NESTING_FUNCTION);

    scopes_tree_set_strict_mode (STACK_TOP (scopes), STACK_TOP (U8) != 0);
    STACK_DROP (U8, 1);

    token_after_newlines_must_be (TOK_CLOSE_BRACE);

    DUMP_VOID_OPCODE (ret);
    rewrite_meta_opcode_counter (STACK_TOP (U16), OPCODE_META_TYPE_FUNCTION_END);
    DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_VARG_PROP_GETTER, ID(2), ID(1));

    STACK_DROP (IDX, 2);
    STACK_DROP (U16, 1);

    goto cleanup;
  }
  else if (lp_string_equal_s (lexer_get_string_by_id (token_data ()), "set"))
  {
    STACK_PUSH (toks, TOK ());
    skip_newlines ();
    if (token_is (TOK_COLON))
    {
      lexer_save_token (TOK ());
      SET_TOK (STACK_TOP (toks));
      STACK_DROP (toks, 1);
      goto simple_prop;
    }
    STACK_DROP (toks, 1);
    // name, lhs
    parse_property_name (); // push name
    STACK_PUSH (props, create_prop_as_str_or_varg (STACK_TOP (strings), PROP_SET));
    STACK_DROP (strings, 1);

    skip_newlines ();
    parse_argument_list (AL_FUNC_EXPR, next_temp_name ()); // push lhs

    STACK_PUSH (U16, OPCODE_COUNTER ());
    DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_FUNCTION_END, INVALID_VALUE, INVALID_VALUE);

    token_after_newlines_must_be (TOK_OPEN_BRACE);

    STACK_PUSH (U8, scopes_tree_strict_mode (STACK_TOP (scopes)) ? 1 : 0);
    scopes_tree_set_strict_mode (STACK_TOP (scopes), false);

    skip_newlines ();
    push_nesting (NESTING_FUNCTION);
    parse_source_element_list (false);
    pop_nesting (NESTING_FUNCTION);

    scopes_tree_set_strict_mode (STACK_TOP (scopes), STACK_TOP (U8) != 0);
    STACK_DROP (U8, 1);

    token_after_newlines_must_be (TOK_CLOSE_BRACE);

    DUMP_VOID_OPCODE (ret);
    rewrite_meta_opcode_counter (STACK_TOP (U16), OPCODE_META_TYPE_FUNCTION_END);
    DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_VARG_PROP_SETTER, ID(2), ID(1));

    STACK_DROP (IDX, 2);
    STACK_DROP (U16, 1);
    
    goto cleanup;
  }

simple_prop:
  parse_property_name_and_value ();

cleanup:
  STACK_CHECK_USAGE (strings);
  STACK_CHECK_USAGE (U8);
  STACK_CHECK_USAGE (toks);
  STACK_CHECK_USAGE (U16);
  STACK_CHECK_USAGE (IDX);
}

static void
emit_error_on_eval_and_arguments (idx_t id)
{
  if (id < lexer_get_strings_count ())
  {
    if (lp_string_equal_zt (lexer_get_string_by_id (id),
                            ecma_get_magic_string_zt (ECMA_MAGIC_STRING_ARGUMENTS))
        || lp_string_equal_zt (lexer_get_string_by_id (id),
                               ecma_get_magic_string_zt (ECMA_MAGIC_STRING_EVAL)))
    {
      EMIT_ERROR ("'eval' and 'arguments' are not allowed here in strict mode");
    }
  }
}

static void
check_for_eval_and_arguments_in_strict_mode (idx_t id)
{
  if (parser_strict_mode ())
  {
    emit_error_on_eval_and_arguments (id);
  }
}

/* 13.1, 15.3.2 */
static void
check_for_syntax_errors_in_formal_param_list (uint8_t from)
{
  if (STACK_SIZE (props) - from < 2 || !parser_strict_mode ())
  {
    return;
  }
  for (uint8_t i = (uint8_t) (from + 1); i < STACK_SIZE (props); i = (uint8_t) (i + 1))
  {
    JERRY_ASSERT (STACK_ELEMENT (props, i).type == VARG);
    for (uint8_t j = from; j < i; j = (uint8_t) (j + 1))
    {
      if (lp_string_equal (STACK_ELEMENT (props, i).str.str, STACK_ELEMENT (props, j).str.str))
      {
        EMIT_ERROR_VARG ("Duplication of literal '%s' in FormalParameterList is not allowed in strict mode",
                         STACK_ELEMENT (props, j).str.str.str);
      }
    }
  }
}

/* 11.1.5 */
static void
check_for_syntax_errors_in_obj_decl (uint8_t from)
{
  if (STACK_SIZE (props) - from < 2)
  {
    return;
  }

  for (uint8_t i = (uint8_t) (from + 1); i < STACK_SIZE (props); i = (uint8_t) (i + 1))
  {
    JERRY_ASSERT (STACK_ELEMENT (props, i).type == PROP_DATA
                  || STACK_ELEMENT (props, i).type == PROP_GET
                  || STACK_ELEMENT (props, i).type == PROP_SET);
    for (uint8_t j = from; j < i; j = (uint8_t) (j + 1))
    {
      /*4*/
      const lp_string previous = STACK_ELEMENT (props, j).str.str;
      const prop_type prop_type_previous = STACK_ELEMENT (props, j).type;
      const lp_string prop_id_desc = STACK_ELEMENT (props, i).str.str;
      const prop_type prop_type_prop_id_desc = STACK_ELEMENT (props, i).type;
      if (lp_string_equal (previous, prop_id_desc))
      {
        /*a*/
        if (parser_strict_mode () && prop_type_previous == PROP_DATA && prop_type_prop_id_desc == PROP_DATA)
        {
          EMIT_ERROR_VARG ("Duplication of parameter name '%s' in ObjectDeclaration is not allowed in strict mode",
                           previous.str);
        }
        /*b*/
        if (prop_type_previous == PROP_DATA
            && (prop_type_prop_id_desc == PROP_SET || prop_type_prop_id_desc == PROP_GET))
        {
          EMIT_ERROR_VARG ("Parameter name '%s' in ObjectDeclaration may not be both data and accessor",
                           previous.str);
        }
        /*c*/
        if (prop_type_prop_id_desc == PROP_DATA
            && (prop_type_previous == PROP_SET || prop_type_previous == PROP_GET))
        {
          EMIT_ERROR_VARG ("Parameter name '%s' in ObjectDeclaration may not be both data and accessor",
                           previous.str);
        }
        /*d*/
        if ((prop_type_previous == PROP_SET && prop_type_prop_id_desc == PROP_SET)
            || (prop_type_previous == PROP_GET && prop_type_prop_id_desc == PROP_GET))
        {
          EMIT_ERROR_VARG ("Parameter name '%s' in ObjectDeclaration may not be accessor of same type",
                           previous.str);
        }
      }
    }
  }
}

/** Parse list of identifiers, assigment expressions or properties, splitted by comma.
    For each ALT dumps appropriate bytecode. Uses OBJ during dump if neccesary.
    Returns number of arguments.  */
static uint8_t
parse_argument_list (argument_list_type alt, idx_t obj)
{
  // U8 open_tt, close_tt, args_count
  // IDX lhs, current_arg
  // U16 oc
  STACK_DECLARE_USAGE (U8)
  STACK_DECLARE_USAGE (U16)
  STACK_DECLARE_USAGE (IDX)
  STACK_DECLARE_USAGE (temp_names)
  STACK_DECLARE_USAGE (props)

  STACK_PUSH (U8, STACK_SIZE (props));
  STACK_PUSH (U8, TOK_OPEN_PAREN);
  STACK_PUSH (U8, TOK_CLOSE_PAREN);
  STACK_PUSH (U8, 0);

  STACK_PUSH (U16, OPCODE_COUNTER ());
  switch (alt)
  {
    case AL_FUNC_DECL:
    {
      DUMP_OPCODE_2 (func_decl_n, obj, INVALID_VALUE);
      break;
    }
    case AL_FUNC_EXPR:
    {
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (func_expr_n, ID(1), obj, INVALID_VALUE);
      break;
    }
    case AL_CONSTRUCT_EXPR:
    {
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (construct_n, ID(1), obj, INVALID_VALUE);
      break;
    }
    case AL_CALL_EXPR:
    {
      if (is_intrinsic (obj))
      {
        break;
      }
      else if (is_native_call (obj))
      {
        STACK_PUSH (IDX, next_temp_name ());
        DUMP_OPCODE_3 (native_call, ID(1),
                       name_to_native_call_id (obj), INVALID_VALUE);
      }
      else
      {
        STACK_PUSH (IDX, next_temp_name ());
        DUMP_OPCODE_3 (call_n, ID(1), obj, INVALID_VALUE);
      }
      break;
    }
    case AL_ARRAY_DECL:
    {
      STACK_SET_HEAD (U8, 3, TOK_OPEN_SQUARE);
      STACK_SET_HEAD (U8, 2, TOK_CLOSE_SQUARE);
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_2 (array_decl, ID(1), INVALID_VALUE);
      break;
    }
    case AL_OBJ_DECL:
    {
      STACK_SET_HEAD (U8, 3, TOK_OPEN_BRACE);
      STACK_SET_HEAD (U8, 2, TOK_CLOSE_BRACE);
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_2 (obj_decl, ID(1), INVALID_VALUE);
      break;
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }

  current_token_must_be (STACK_HEAD (U8, 3));

  switch (alt)
  {
    case AL_CALL_EXPR:
    {
      if (THIS_ARG () != INVALID_VALUE)
      {
        DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_THIS_ARG, THIS_ARG (), INVALID_VALUE);
        STACK_INCR_HEAD (U8, 1);
      }
      break;
    }
    default: break;
  }

  STACK_PUSH (temp_names, TEMP_NAME ());

  skip_newlines ();
  while (!token_is (STACK_HEAD (U8, 2)))
  {
    STACK_SET_ELEMENT (temp_names, temp_name, STACK_TOP (temp_names));
    switch (alt)
    {
      case AL_FUNC_DECL:
      case AL_FUNC_EXPR:
      {
        current_token_must_be (TOK_NAME);
        STACK_PUSH (IDX, token_data ());
        STACK_PUSH (props,
                    create_prop_as_str_or_varg (create_allocatable_string_from_literal (token_data ()),
                                                VARG));
        check_for_eval_and_arguments_in_strict_mode (STACK_TOP (IDX));
        break;
      }
      case AL_ARRAY_DECL:
      {
        if (token_is (TOK_COMMA))
        {
          STACK_PUSH (IDX, next_temp_name ());
          DUMP_OPCODE_3 (assignment, ID(1), OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_UNDEFINED);
          DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_VARG, ID(1), INVALID_VALUE);
          STACK_INCR_HEAD(U8, 1);
          STACK_DROP (IDX, 1);
          skip_newlines ();
          continue;
        }
        /* FALLTHRU  */
      }
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
          dump_intrinsic (obj, ID(1));
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
      DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_VARG, ID(1), INVALID_VALUE);
      STACK_DROP (IDX, 1);
    }
    STACK_INCR_HEAD(U8, 1);

next:
    skip_newlines ();
    if (!token_is (TOK_COMMA))
    {
      current_token_must_be (STACK_HEAD (U8, 2));
      break;
    }

    skip_newlines ();
  }
  STACK_DROP (temp_names, 1);

  switch (alt)
  {
    case AL_FUNC_DECL:
    {
      check_for_syntax_errors_in_formal_param_list (STACK_HEAD (U8, 4));
      REWRITE_OPCODE_2 (STACK_TOP (U16), func_decl_n, obj, STACK_TOP (U8));
      break;
    }
    case AL_FUNC_EXPR:
    {
      check_for_syntax_errors_in_formal_param_list (STACK_HEAD (U8, 4));
      REWRITE_OPCODE_3 (STACK_TOP (U16), func_expr_n, ID(1), obj, STACK_TOP (U8));
      break;
    }
    case AL_CONSTRUCT_EXPR:
    {
      REWRITE_OPCODE_3 (STACK_TOP (U16), construct_n, ID(1), obj, STACK_TOP (U8));
      break;
    }
    case AL_CALL_EXPR:
    {
      if (is_intrinsic (obj))
      {
        break;
      }
      else if (is_native_call (obj))
      {
        REWRITE_OPCODE_3 (STACK_TOP (U16), native_call, ID(1),
                          name_to_native_call_id (obj), STACK_TOP (U8));
      }
      else
      {
        REWRITE_OPCODE_3 (STACK_TOP (U16), call_n, ID(1), obj,
                          STACK_TOP (U8));
      }
      break;
    }
    case AL_ARRAY_DECL:
    {
      REWRITE_OPCODE_2 (STACK_TOP (U16), array_decl, ID(1), STACK_TOP (U8));
      break;
    }
    case AL_OBJ_DECL:
    {
      check_for_syntax_errors_in_obj_decl (STACK_HEAD (U8, 4));
      REWRITE_OPCODE_2 (STACK_TOP (U16), obj_decl, ID(1), STACK_TOP (U8));
      break;
    }
    default:
    {
      JERRY_UNREACHABLE ();
    }
  }

  const uint8_t args_num = STACK_TOP (U8);

  STACK_ITERATE (props, free_allocatable_string, STACK_HEAD (U8, 4));

  STACK_DROP (props, (uint8_t) (STACK_SIZE (props) - STACK_HEAD (U8, 4)));
  STACK_DROP (U8, 4);
  STACK_DROP (U16, 1);

  STACK_CHECK_USAGE (props);
  STACK_CHECK_USAGE (U8);
  STACK_CHECK_USAGE (U16);
  STACK_CHECK_USAGE (temp_names);

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

  return args_num;
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
  STACK_DECLARE_USAGE (scopes)
  STACK_DECLARE_USAGE (U8)

  assert_keyword (KW_FUNCTION);

  token_after_newlines_must_be (TOK_NAME);

  STACK_PUSH (IDX, token_data ());

  skip_newlines ();
  SET_OPCODE_COUNTER (0);
  STACK_PUSH (scopes, scopes_tree_init (STACK_TOP (scopes)));
  serializer_set_scope (STACK_TOP (scopes));
  scopes_tree_set_strict_mode (STACK_TOP (scopes), scopes_tree_strict_mode (STACK_HEAD (scopes, 2)));
  STACK_PUSH (U8, parse_argument_list (AL_FUNC_DECL, ID(1)));
  scopes_tree_set_strict_mode (STACK_TOP (scopes), false);

  STACK_PUSH (U16, OPCODE_COUNTER ());
  DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_UNDEFINED, INVALID_VALUE, INVALID_VALUE);

  token_after_newlines_must_be (TOK_OPEN_BRACE);

  skip_newlines ();
  push_nesting (NESTING_FUNCTION);
  parse_source_element_list (false);
  pop_nesting (NESTING_FUNCTION);

  next_token_must_be (TOK_CLOSE_BRACE);

  DUMP_VOID_OPCODE (ret);

  rewrite_meta_opcode_counter_set_oc (STACK_TOP (U16), OPCODE_META_TYPE_FUNCTION_END,
                                      (opcode_counter_t) (
                                        scopes_tree_count_opcodes (STACK_TOP (scopes)) - (STACK_TOP (U8) + 1)));

  STACK_DROP (U16, 1);
  STACK_DROP (U8, 1);
  STACK_DROP (IDX, 1);
  STACK_DROP (scopes, 1);
  SET_OPCODE_COUNTER (scopes_tree_opcodes_num (STACK_TOP (scopes)));
  serializer_set_scope (STACK_TOP (scopes));

  STACK_CHECK_USAGE (U8);
  STACK_CHECK_USAGE (IDX);
  STACK_CHECK_USAGE (U16);
  STACK_CHECK_USAGE (nestings);
  STACK_CHECK_USAGE (scopes);
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
  STACK_DECLARE_USAGE (U8)

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
  parse_argument_list (AL_FUNC_EXPR, ID(1)); // push lhs

  STACK_PUSH (U16, OPCODE_COUNTER ());
  DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_UNDEFINED, INVALID_VALUE, INVALID_VALUE);

  token_after_newlines_must_be (TOK_OPEN_BRACE);

  STACK_PUSH (U8, scopes_tree_strict_mode (STACK_TOP (scopes)) ? 1 : 0);
  scopes_tree_set_strict_mode (STACK_TOP (scopes), false);

  skip_newlines ();
  push_nesting (NESTING_FUNCTION);
  parse_source_element_list (false);
  pop_nesting (NESTING_FUNCTION);

  scopes_tree_set_strict_mode (STACK_TOP (scopes), STACK_TOP (U8) != 0);
  STACK_DROP (U8, 1);

  next_token_must_be (TOK_CLOSE_BRACE);

  DUMP_VOID_OPCODE (ret);
  rewrite_meta_opcode_counter (STACK_TOP (U16), OPCODE_META_TYPE_FUNCTION_END);

  STACK_SWAP (IDX);
  STACK_DROP (IDX, 1);
  STACK_DROP (U16, 1);

  STACK_CHECK_USAGE (U8);
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
      DUMP_OPCODE_3 (assignment, ID(1), OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_NULL);
      break;
    }
    case TOK_BOOL:
    {
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (assignment, ID(1), OPCODE_ARG_TYPE_SIMPLE,
                     token_data () ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE);
      break;
    }
    case TOK_NUMBER:
    {
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (assignment, ID(1), OPCODE_ARG_TYPE_NUMBER, token_data ());
      break;
    }
    case TOK_SMALL_INT:
    {
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (assignment, ID(1), OPCODE_ARG_TYPE_SMALLINT, token_data ());
      break;
    }
    case TOK_STRING:
    {
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (assignment, ID(1), OPCODE_ARG_TYPE_STRING, token_data ());
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
  | 'undefined'
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
    DUMP_OPCODE_1 (this, ID(1));
    goto cleanup;
  }

  switch (TOK ().type)
  {
    case TOK_NAME:
    {
      STACK_PUSH (IDX, token_data ());
      break;
    }
    case TOK_UNDEFINED:
    {
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (assignment, ID(1), OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_UNDEFINED);
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
      EMIT_ERROR_VARG ("Unknown token %s", lexer_token_type_to_string (TOK ().type));
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
    skip_newlines ();
    parse_member_expression ();

    skip_newlines ();
    if (token_is (TOK_OPEN_PAREN))
    {
      parse_argument_list (AL_CONSTRUCT_EXPR, ID(1)); // push obj
    }
    else
    {
      lexer_save_token (TOK ());
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (construct_n, ID (1), ID (2), 0);
    }

    STACK_SWAP (IDX);
    STACK_DROP (IDX, 1);
  }
  else
  {
    parse_primary_expression ();
  }

  skip_newlines ();
  while (token_is (TOK_OPEN_SQUARE) || token_is (TOK_DOT))
  {
    SET_THIS_ARG(ID (1));

    STACK_PUSH (IDX, next_temp_name ());

    if (token_is (TOK_OPEN_SQUARE))
    {
      NEXT (expression); // push prop
      SET_PROP (ID (1));
      next_token_must_be (TOK_CLOSE_SQUARE);
    }
    else if (token_is (TOK_DOT))
    {
      skip_newlines ();
      if (!token_is (TOK_NAME))
      {
        EMIT_ERROR ("Expected identifier");
      }
      STACK_PUSH (IDX, next_temp_name ());
      SET_PROP (ID (1));
      DUMP_OPCODE_3 (assignment, ID(1), OPCODE_ARG_TYPE_STRING, token_data ());
    }
    else
    {
      JERRY_UNREACHABLE ();
    }

    DUMP_OPCODE_3 (prop_getter, ID(2), ID(3), ID(1));
    STACK_DROP (IDX, 1);
    STACK_SWAP (IDX);

    skip_newlines ();

    STACK_DROP (IDX, 1);
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
  SET_THIS_ARG (INVALID_VALUE);

  parse_member_expression ();

  skip_newlines ();
  if (!token_is (TOK_OPEN_PAREN))
  {
    lexer_save_token (TOK ());
    goto cleanup;
  }

  if (THIS_ARG () < lexer_get_reserved_ids_count ())
  {
    STACK_PUSH (IDX, next_temp_name ());
    DUMP_OPCODE_3 (assignment, ID(1), OPCODE_ARG_TYPE_VARIABLE, THIS_ARG ());
    SET_THIS_ARG (ID(1));
    STACK_DROP (IDX, 1);
  }

  parse_argument_list (AL_CALL_EXPR, ID(1)); // push lhs
  SET_THIS_ARG (INVALID_VALUE);
  STACK_SWAP (IDX);

  skip_newlines ();
  while (token_is (TOK_OPEN_PAREN) || token_is (TOK_OPEN_SQUARE)
         || token_is (TOK_DOT))
  {
    STACK_DROP (IDX, 1);
    if (TOK ().type == TOK_OPEN_PAREN)
    {
      parse_argument_list (AL_CALL_EXPR, ID(1)); // push lhs
      skip_newlines ();
    }
    else
    {
      SET_THIS_ARG (ID (1));
      if (TOK ().type == TOK_OPEN_SQUARE)
      {
        NEXT (expression); // push prop
        next_token_must_be (TOK_CLOSE_SQUARE);
      }
      else if (TOK ().type == TOK_DOT)
      {
        token_after_newlines_must_be (TOK_NAME);
        STACK_PUSH (IDX, next_temp_name ());
        DUMP_OPCODE_3 (assignment, ID(1), OPCODE_ARG_TYPE_STRING, token_data ());
      }
      else
      {
        JERRY_UNREACHABLE ();
      }
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (prop_getter, ID(1), ID(3), ID(2));
      STACK_SWAP (IDX);
      STACK_DROP (IDX, 1);
      skip_newlines ();
    }
    STACK_SWAP (IDX);
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
  SET_PROP (INVALID_VALUE);

  parse_left_hand_side_expression (); // push expr

  if (lexer_prev_token ().type == TOK_NEWLINE)
  {
    goto cleanup;
  }

  check_for_eval_and_arguments_in_strict_mode (STACK_TOP (IDX));

  skip_token ();
  if (token_is (TOK_DOUBLE_PLUS))
  {
    STACK_PUSH (IDX, next_temp_name ());
    DUMP_OPCODE_2 (post_incr, ID(1), ID(2));
    if (THIS_ARG () != INVALID_VALUE && PROP () != INVALID_VALUE)
    {
      DUMP_OPCODE_3 (prop_setter, THIS_ARG (), PROP (), ID (2));
    }
    STACK_SWAP (IDX);
    STACK_DROP (IDX, 1);
  }
  else if (token_is (TOK_DOUBLE_MINUS))
  {
    STACK_PUSH (IDX, next_temp_name ());
    DUMP_OPCODE_2 (post_decr, ID(1), ID(2));
    if (THIS_ARG () != INVALID_VALUE && PROP () != INVALID_VALUE)
    {
      DUMP_OPCODE_3 (prop_setter, THIS_ARG (), PROP (), ID (2));
    }
    STACK_SWAP (IDX);
    STACK_DROP (IDX, 1);
  }
  else
  {
    lexer_save_token (TOK ());
  }

cleanup:
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
  STACK_DECLARE_USAGE (ops)

  switch (TOK ().type)
  {
    case TOK_DOUBLE_PLUS:
    {
      NEXT (unary_expression);
      check_for_eval_and_arguments_in_strict_mode (STACK_TOP (IDX));
      DUMP_OPCODE_2 (pre_incr, next_temp_name (), ID(1));
      if (THIS_ARG () != INVALID_VALUE && PROP () != INVALID_VALUE)
      {
        DUMP_OPCODE_3 (prop_setter, THIS_ARG (), PROP (), ID (1));
      }
      break;
    }
    case TOK_DOUBLE_MINUS:
    {
      NEXT (unary_expression);
      check_for_eval_and_arguments_in_strict_mode (STACK_TOP (IDX));
      DUMP_OPCODE_2 (pre_decr, next_temp_name (), ID(1));
      if (THIS_ARG () != INVALID_VALUE && PROP () != INVALID_VALUE)
      {
        DUMP_OPCODE_3 (prop_setter, THIS_ARG (), PROP (), ID (1));
      }
      break;
    }
    case TOK_PLUS:
    {
      STACK_PUSH (IDX, next_temp_name ());
      NEXT (unary_expression);
      DUMP_OPCODE_2 (unary_plus, ID(2), ID(1));
      STACK_DROP (IDX, 1);
      break;
    }
    case TOK_MINUS:
    {
      STACK_PUSH (IDX, next_temp_name ());
      NEXT (unary_expression);
      DUMP_OPCODE_2 (unary_minus, ID(2), ID(1));
      STACK_DROP (IDX, 1);
      break;
    }
    case TOK_COMPL:
    {
      STACK_PUSH (IDX, next_temp_name ());
      NEXT (unary_expression);
      DUMP_OPCODE_2 (b_not, ID(2), ID(1));
      STACK_DROP (IDX, 1);
      break;
    }
    case TOK_NOT:
    {
      STACK_PUSH (IDX, next_temp_name ());
      NEXT (unary_expression);
      DUMP_OPCODE_2 (logical_not, ID(2), ID(1));
      STACK_DROP (IDX, 1);
      break;
    }
    case TOK_KEYWORD:
    {
      if (is_keyword (KW_DELETE))
      {
        NEXT (unary_expression);
        if (ID (1) < lexer_get_strings_count ())
        {
          if (parser_strict_mode ())
          {
            EMIT_ERROR ("'delete' operator shall not apply on identifier in strict mode.");
          }
          STACK_PUSH (IDX, next_temp_name ());
          DUMP_OPCODE_2 (delete_var, ID (1), ID (2));
          STACK_SWAP (IDX);
          STACK_DROP (IDX, 1);
          break;
        }
        STACK_PUSH (ops, deserialize_opcode ((opcode_counter_t) (OPCODE_COUNTER () - 1)));
        if (LAST_OPCODE_IS (assignment)
            && STACK_TOP (ops).data.assignment.type_value_right == OPCODE_ARG_TYPE_VARIABLE)
        {
          STACK_PUSH (IDX, next_temp_name ());
          REWRITE_OPCODE_2 (OPCODE_COUNTER () - 1, delete_var, ID (1), STACK_TOP (ops).data.assignment.value_right);
          STACK_SWAP (IDX);
          STACK_DROP (IDX, 1);
        }
        else if (LAST_OPCODE_IS (prop_getter))
        {
          STACK_PUSH (IDX, next_temp_name ());
          REWRITE_OPCODE_3 (OPCODE_COUNTER () - 1, delete_prop, ID (1),
                            STACK_TOP (ops).data.prop_getter.obj, STACK_TOP (ops).data.prop_getter.prop);
          STACK_SWAP (IDX);
          STACK_DROP (IDX, 1);
        }
        else
        {
          STACK_PUSH (IDX, next_temp_name ());
          DUMP_OPCODE_3 (assignment, ID (1), OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_TRUE);
          STACK_SWAP (IDX);
          STACK_DROP (IDX, 1);
        }
        STACK_DROP (ops, 1);
        break;
      }
      else if (is_keyword (KW_VOID))
      {
        STACK_PUSH (IDX, next_temp_name ());
        NEXT (unary_expression);
        DUMP_OPCODE_3 (assignment, ID(2), OPCODE_ARG_TYPE_VARIABLE, ID (1));
        DUMP_OPCODE_3 (assignment, ID(2), OPCODE_ARG_TYPE_SIMPLE, ECMA_SIMPLE_VALUE_UNDEFINED);
        STACK_DROP (IDX, 1);
        break;
      }
      else if (is_keyword (KW_TYPEOF))
      {
        STACK_PUSH (IDX, next_temp_name ());
        NEXT (unary_expression);
        DUMP_OPCODE_2 (typeof, ID(2), ID(1));
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
  STACK_CHECK_USAGE (ops);
}

#define DUMP_OF(GETOP, EXPR) \
do { \
  generate_tmp_for_left_arg (); \
  STACK_PUSH (IDX, next_temp_name ()); \
  NEXT (EXPR);\
  DUMP_OPCODE_3 (GETOP, ID(2), ID(3), ID(1)); \
  STACK_DROP (IDX, 1); \
  STACK_SWAP (IDX); \
  STACK_DROP (IDX, 1); \
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
  STACK_DECLARE_USAGE (U8)

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
          if (STACK_ELEMENT (U8, no_in) == 0)
          {
            DUMP_OF (in, shift_expression);
            break;
          }
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
  STACK_CHECK_USAGE (U8);
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

static void
dump_logical_check_and_op (bool logical_or)
{
  STACK_DECLARE_USAGE (IDX);

  STACK_PUSH (U16, OPCODE_COUNTER ());
  if (logical_or)
  {
    DUMP_OPCODE_3 (is_true_jmp_down, ID (1), INVALID_VALUE, INVALID_VALUE);
    skip_newlines ();
    parse_logical_expression (false);
  }
  else
  {
    DUMP_OPCODE_3 (is_false_jmp_down, ID (1), INVALID_VALUE, INVALID_VALUE);
    NEXT (bitwise_or_expression);
  }
  DUMP_OPCODE_3 (assignment, ID(2), OPCODE_ARG_TYPE_VARIABLE, ID (1));
  STACK_DROP (IDX, 1);

  STACK_CHECK_USAGE (IDX);
}

static void
rewrite_logical_check (bool logical_or, uint8_t elem)
{
  if (logical_or)
  {
    REWRITE_COND_JMP (STACK_ELEMENT (U16, elem), is_true_jmp_down,
                      OPCODE_COUNTER () - STACK_ELEMENT (U16, elem));
  }
  else
  {
    REWRITE_COND_JMP (STACK_ELEMENT (U16, elem), is_false_jmp_down,
                      OPCODE_COUNTER () - STACK_ELEMENT (U16, elem));
  }
}

/* logical_and_expression
  : bitwise_or_expression (LT!* '&&' LT!* bitwise_or_expression)*
  ;
   logical_or_expression
  : logical_and_expression (LT!* '||' LT!* logical_and_expression)*
  ; */
static void
parse_logical_expression (bool logical_or)
{
  STACK_DECLARE_USAGE (IDX)
  STACK_DECLARE_USAGE (U16)
  STACK_DECLARE_USAGE (U8)

  STACK_PUSH (U8, logical_or ? TOK_DOUBLE_OR : TOK_DOUBLE_AND);
  STACK_PUSH (U8, STACK_SIZE (U16));

  if (logical_or)
  {
    parse_logical_expression (false);
  }
  else
  {
    parse_bitwise_or_expression ();
  }

  skip_newlines ();
  if (token_is (STACK_HEAD (U8, 2)))
  {
    STACK_PUSH (IDX, next_temp_name ());
    DUMP_OPCODE_3 (assignment, ID(1), OPCODE_ARG_TYPE_VARIABLE, ID (2));
    dump_logical_check_and_op (logical_or);
    skip_newlines ();
  }
  while (token_is (STACK_HEAD (U8, 2)))
  {
    dump_logical_check_and_op (logical_or);
    skip_newlines ();
  }
  lexer_save_token (TOK ());

  if (STACK_TOP (U8) != STACK_SIZE (U16))
  {
    for (uint8_t i = STACK_TOP (U8); i < STACK_SIZE (U16); i++)
    {
      rewrite_logical_check (logical_or, i);
    }
    STACK_SWAP (IDX);
    STACK_DROP (IDX, 1);
  }

  STACK_DROP (U16, STACK_SIZE (U16) - STACK_TOP (U8));
  STACK_DROP (U8, 2);

  STACK_CHECK_USAGE (U16);
  STACK_CHECK_USAGE (U8);
  STACK_CHECK_USAGE_LHS ();
}

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

  parse_logical_expression (true);

  skip_newlines ();
  if (token_is (TOK_QUERY))
  {
    generate_tmp_for_left_arg ();
    STACK_PUSH (U16, OPCODE_COUNTER ());
    DUMP_OPCODE_3 (is_false_jmp_down, ID(1), INVALID_VALUE, INVALID_VALUE);

    STACK_PUSH (IDX, next_temp_name ());
    NEXT (assignment_expression);
    DUMP_OPCODE_3 (assignment, ID(2), OPCODE_ARG_TYPE_VARIABLE, ID(1));
    STACK_DROP (IDX, 1);
    STACK_SWAP (IDX);
    token_after_newlines_must_be (TOK_COLON);

    STACK_PUSH (U16, OPCODE_COUNTER ());
    DUMP_OPCODE_2 (jmp_down, INVALID_VALUE, INVALID_VALUE);

    REWRITE_COND_JMP (STACK_HEAD (U16, 2), is_false_jmp_down, OPCODE_COUNTER () - STACK_HEAD (U16, 2));

    STACK_DROP (IDX, 1);
    NEXT (assignment_expression);
    DUMP_OPCODE_3 (assignment, ID(2), OPCODE_ARG_TYPE_VARIABLE, ID(1));
    REWRITE_JMP (STACK_TOP (U16), jmp_down, OPCODE_COUNTER () - STACK_TOP (U16));

    STACK_DROP (U8, 1);
    STACK_DROP (U16, 2);
    STACK_PUSH (U8, 1);
    STACK_DROP (IDX, 1);
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
  if (STACK_TOP (U8))
  {
    goto cleanup;
  }

  /* Rewrite prop_getter with nop and generate prop_setter in constructions like:
    a.b = c; */
  if (LAST_OPCODE_IS (prop_getter))
  {
    STACK_PUSH (U16, (opcode_counter_t) (OPCODE_COUNTER () - 1));
    STACK_DROP (U8, 1);
    STACK_PUSH (U8, 1);
  }

  check_for_eval_and_arguments_in_strict_mode (STACK_TOP (IDX));

  skip_newlines ();
  switch (TOK ().type)
  {
    case TOK_EQ:
    {
      if (STACK_TOP (U8))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_TOP (U16)));
        JERRY_ASSERT (OPCODE_IS (STACK_TOP (ops), prop_getter));
        DECR_OPCODE_COUNTER ();
        serializer_set_writing_position (OPCODE_COUNTER ());
        NEXT (assignment_expression);
        DUMP_OPCODE_3 (prop_setter, STACK_TOP (ops).data.prop_getter.obj,
                       STACK_TOP (ops).data.prop_getter.prop, ID(1));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        NEXT (assignment_expression);
        DUMP_OPCODE_3 (assignment, ID(2), OPCODE_ARG_TYPE_VARIABLE,
                       ID(1));
      }
      break;
    }
    case TOK_MULT_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_TOP (U8))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (multiplication, ID(2), ID(2),
                       ID(1));
        DUMP_OPCODE_3 (prop_setter, STACK_TOP (ops).data.prop_getter.obj,
                       STACK_TOP (ops).data.prop_getter.prop, ID(2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (multiplication, ID(2), ID(2),
                       ID(1));
      }
      break;
    }
    case TOK_DIV_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_TOP (U8))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (division, ID(2), ID(2),
                       ID(1));
        DUMP_OPCODE_3 (prop_setter, STACK_TOP (ops).data.prop_getter.obj,
                       STACK_TOP (ops).data.prop_getter.prop, ID(2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (division, ID(2), ID(2),
                       ID(1));
      }
      break;
    }
    case TOK_MOD_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_TOP (U8))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (remainder, ID(2), ID(2),
                       ID(1));
        DUMP_OPCODE_3 (prop_setter, STACK_TOP (ops).data.prop_getter.obj,
                       STACK_TOP (ops).data.prop_getter.prop, ID(2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (remainder, ID(2), ID(2),
                       ID(1));
      }
      break;
    }
    case TOK_PLUS_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_TOP (U8))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (addition, ID(2), ID(2),
                       ID(1));
        DUMP_OPCODE_3 (prop_setter, STACK_TOP (ops).data.prop_getter.obj,
                       STACK_TOP (ops).data.prop_getter.prop, ID(2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (addition, ID(2), ID(2),
                       ID(1));
      }
      break;
    }
    case TOK_MINUS_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_TOP (U8))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (substraction, ID(2), ID(2),
                       ID(1));
        DUMP_OPCODE_3 (prop_setter, STACK_TOP (ops).data.prop_getter.obj,
                       STACK_TOP (ops).data.prop_getter.prop, ID(2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (substraction, ID(2), ID(2),
                       ID(1));
      }
      break;
    }
    case TOK_LSHIFT_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_TOP (U8))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (b_shift_left, ID(2), ID(2),
                       ID(1));
        DUMP_OPCODE_3 (prop_setter, STACK_TOP (ops).data.prop_getter.obj,
                       STACK_TOP (ops).data.prop_getter.prop, ID(2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (b_shift_left, ID(2), ID(2),
                       ID(1));
      }
      break;
    }
    case TOK_RSHIFT_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_TOP (U8))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (b_shift_right, ID(2), ID(2),
                       ID(1));
        DUMP_OPCODE_3 (prop_setter, STACK_TOP (ops).data.prop_getter.obj,
                       STACK_TOP (ops).data.prop_getter.prop, ID(2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (b_shift_right, ID(2), ID(2),
                       ID(1));
      }
      break;
    }
    case TOK_RSHIFT_EX_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_TOP (U8))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (b_shift_uright, ID(2), ID(2),
                       ID(1));
        DUMP_OPCODE_3 (prop_setter, STACK_TOP (ops).data.prop_getter.obj,
                       STACK_TOP (ops).data.prop_getter.prop, ID(2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (b_shift_uright, ID(2), ID(2),
                       ID(1));
      }
      break;
    }
    case TOK_AND_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_TOP (U8))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (b_and, ID(2), ID(2),
                       ID(1));
        DUMP_OPCODE_3 (prop_setter, STACK_TOP (ops).data.prop_getter.obj,
                       STACK_TOP (ops).data.prop_getter.prop, ID(2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (b_and, ID(2), ID(2),
                       ID(1));
      }
      break;
    }
    case TOK_XOR_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_TOP (U8))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (b_xor, ID(2), ID(2),
                       ID(1));
        DUMP_OPCODE_3 (prop_setter, STACK_TOP (ops).data.prop_getter.obj,
                       STACK_TOP (ops).data.prop_getter.prop, ID(2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (b_xor, ID(2), ID(2),
                       ID(1));
      }
      break;
    }
    case TOK_OR_EQ:
    {
      NEXT (assignment_expression);
      if (STACK_TOP (U8))
      {
        STACK_PUSH (ops, deserialize_opcode (STACK_HEAD (U16, 0)));
        DUMP_OPCODE_3 (b_or, ID(2), ID(2),
                       ID(1));
        DUMP_OPCODE_3 (prop_setter, STACK_TOP (ops).data.prop_getter.obj,
                       STACK_TOP (ops).data.prop_getter.prop, ID(2));
        STACK_DROP (ops, 1);
        STACK_DROP (U16, 1);
      }
      else
      {
        DUMP_OPCODE_3 (b_or, ID(2), ID(2),
                       ID(1));
      }
      break;
    }
    default:
    {
      if (STACK_TOP (U8))
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
      STACK_SWAP (IDX);
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

  skip_newlines ();
  if (token_is (TOK_EQ))
  {
    NEXT (assignment_expression);
    DUMP_OPCODE_3 (assignment, ID(2), OPCODE_ARG_TYPE_VARIABLE, ID(1));
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
  STACK_DECLARE_USAGE (U8)

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
        EMIT_ERROR ("Expected either ';' or 'in' token");
      }
    }
  }

  /* expression contains left_hand_side_expression.  */
  STACK_SET_ELEMENT (U8, no_in, 1);
  parse_expression ();
  STACK_SET_ELEMENT (U8, no_in, 0);
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
    EMIT_ERROR ("Expected either ';' or 'in' token");
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
  STACK_PUSH (U8, STACK_SIZE (rewritable_continue));
  STACK_PUSH (U8, STACK_SIZE (rewritable_break));

  STACK_PUSH (U16, OPCODE_COUNTER ()); // cond_oc;
  skip_newlines ();
  if (!token_is (TOK_SEMICOLON))
  {
    parse_expression ();
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
    parse_expression ();
    STACK_DROP (IDX, 1);
    next_token_must_be (TOK_CLOSE_PAREN);
  }
  DUMP_OPCODE_2 (jmp_up, 0, OPCODE_COUNTER () - STACK_HEAD (U16, 4));
  REWRITE_JMP (STACK_HEAD (U16, 2), jmp_down, OPCODE_COUNTER () - STACK_HEAD (U16, 2));

  skip_newlines ();
  push_nesting (NESTING_ITERATIONAL);
  parse_statement ();
  pop_nesting (NESTING_ITERATIONAL);

  DUMP_OPCODE_2 (jmp_up, 0, OPCODE_COUNTER () - STACK_TOP (U16));
  REWRITE_COND_JMP (STACK_HEAD (U16, 3), is_false_jmp_down, OPCODE_COUNTER () - STACK_HEAD (U16, 3));

  rewrite_rewritable_opcodes (REWRITABLE_CONTINUE, STACK_HEAD (U8, 2), STACK_TOP (U16));
  rewrite_rewritable_opcodes (REWRITABLE_BREAK, STACK_TOP (U8), OPCODE_COUNTER ());

  STACK_DROP (U8, 2);
  STACK_DROP (IDX, 1);
  STACK_DROP (U16, 4);

  goto cleanup;

for_in:
  EMIT_SORRY ("'for in' loops are not supported yet");

cleanup:
  STACK_CHECK_USAGE (IDX);
  STACK_CHECK_USAGE (U16);
  STACK_CHECK_USAGE (U8);
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
    if (is_keyword (KW_CASE) || is_keyword (KW_DEFAULT))
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

    REWRITE_JMP (STACK_TOP (U16), jmp_down, OPCODE_COUNTER () - STACK_TOP (U16));

    STACK_DROP (U16, 1);
  }
  else
  {
    REWRITE_COND_JMP (STACK_TOP (U16), is_false_jmp_down, OPCODE_COUNTER () - STACK_TOP (U16));
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
  STACK_DECLARE_USAGE (U8)
  STACK_DECLARE_USAGE (nestings)

  assert_keyword (KW_DO);

  STACK_PUSH (U8, STACK_SIZE (rewritable_continue));
  STACK_PUSH (U8, STACK_SIZE (rewritable_break));

  STACK_PUSH (U16, OPCODE_COUNTER ());
  skip_newlines ();
  push_nesting (NESTING_ITERATIONAL);
  parse_statement ();
  pop_nesting (NESTING_ITERATIONAL);

  rewrite_rewritable_opcodes (REWRITABLE_CONTINUE, STACK_HEAD (U8, 2), OPCODE_COUNTER ());
  token_after_newlines_must_be_keyword (KW_WHILE);
  parse_expression_inside_parens ();
  STACK_PUSH (U16, OPCODE_COUNTER ());
  DUMP_OPCODE_3 (is_true_jmp_up, INVALID_VALUE, INVALID_VALUE, INVALID_VALUE);
  REWRITE_COND_JMP (STACK_HEAD(U16, 1), is_true_jmp_up, STACK_HEAD(U16, 1) - STACK_HEAD (U16, 2));

  rewrite_rewritable_opcodes (REWRITABLE_BREAK, STACK_TOP (U8), OPCODE_COUNTER ());

  STACK_DROP (IDX, 1);
  STACK_DROP (U16, 2);
  STACK_DROP (U8, 2);

  STACK_CHECK_USAGE (U8);
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
  STACK_DECLARE_USAGE (U8)
  STACK_DECLARE_USAGE (nestings)

  assert_keyword (KW_WHILE);

  STACK_PUSH (U8, STACK_SIZE (rewritable_continue));
  STACK_PUSH (U8, STACK_SIZE (rewritable_break));

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
  REWRITE_JMP (STACK_TOP (U16), jmp_up, STACK_TOP (U16) - STACK_HEAD (U16, 3));
  REWRITE_COND_JMP (STACK_HEAD (U16, 2), is_false_jmp_down, OPCODE_COUNTER () - STACK_HEAD (U16, 2));

  rewrite_rewritable_opcodes (REWRITABLE_CONTINUE, STACK_HEAD (U8, 2), STACK_HEAD (U16, 3));
  rewrite_rewritable_opcodes (REWRITABLE_BREAK, STACK_TOP (U8), OPCODE_COUNTER ());

  STACK_DROP (IDX, 1);
  STACK_DROP (U16, 3);
  STACK_DROP (U8, 2);

  STACK_CHECK_USAGE (U8);
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

  if (parser_strict_mode ())
  {
    EMIT_ERROR ("'with' expression is not allowed in strict mode.");
  }

  parse_expression_inside_parens ();

  DUMP_OPCODE_1 (with, ID(1));

  skip_newlines ();
  parse_statement ();

  DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_END_WITH, INVALID_VALUE, INVALID_VALUE);

  STACK_DROP (IDX, 1);

  STACK_CHECK_USAGE (IDX);
}

static void
skip_case_clause_body (void)
{
  while (!is_keyword (KW_CASE) && !is_keyword (KW_DEFAULT) && !token_is (TOK_CLOSE_BRACE))
  {
    skip_newlines ();
    if (token_is (TOK_OPEN_BRACE))
    {
      skip_braces ();
    }
  }
}

/* switch_statement
  : 'switch' LT!* '(' LT!* expression LT!* ')' LT!* '{' LT!* case_block LT!* '}'
  ;
   case_block
  : '{' LT!* case_clause* LT!* '}'
  | '{' LT!* case_clause* LT!* default_clause LT!* case_clause* LT!* '}'
  ;
   case_clause
  : 'case' LT!* expression LT!* ':' LT!* statement*
  ; */
static void
parse_switch_statement (void)
{
  STACK_DECLARE_USAGE (rewritable_break)
  STACK_DECLARE_USAGE (IDX)
  STACK_DECLARE_USAGE (U16)
  STACK_DECLARE_USAGE (U8)
  STACK_DECLARE_USAGE (locs)

  assert_keyword (KW_SWITCH);

  STACK_PUSH (U8, 0);
  STACK_PUSH (U8, 0);
  STACK_PUSH (U8, STACK_SIZE (IDX));
  STACK_PUSH (U8, STACK_SIZE (U16));
  STACK_PUSH (U8, STACK_SIZE (rewritable_break));

  parse_expression_inside_parens ();
  token_after_newlines_must_be (TOK_OPEN_BRACE);

#define SWITCH_EXPR() STACK_ELEMENT (IDX, STACK_HEAD (U8, 3))
#define CURRENT_JUMP() STACK_HEAD (U8, 4)
#define INCR_CURRENT_JUMP() STACK_INCR_HEAD (U8, 4)
#define WAS_DEFAULT() STACK_HEAD (U8, 5)
#define SET_WAS_DEFAULT(S) STACK_SET_HEAD (U8, 5, S);

  STACK_PUSH (locs, TOK ().loc);
  // Fisrt, generate table of jumps
  skip_newlines ();
  while (is_keyword (KW_CASE) || is_keyword (KW_DEFAULT))
  {
    if (is_keyword (KW_CASE))
    {
      NEXT (expression);
      next_token_must_be (TOK_COLON);
      STACK_PUSH (IDX, next_temp_name ());
      DUMP_OPCODE_3 (equal_value_type, ID (1), ID (2), SWITCH_EXPR ());
      STACK_SWAP (IDX);
      STACK_DROP (IDX, 1);
      STACK_PUSH (U16, OPCODE_COUNTER ());
      DUMP_OPCODE_3 (is_true_jmp_down, STACK_TOP (IDX), INVALID_VALUE, INVALID_VALUE);
      skip_newlines ();
      skip_case_clause_body ();
    }
    else if (is_keyword (KW_DEFAULT))
    {
      SET_WAS_DEFAULT (1);
      token_after_newlines_must_be (TOK_COLON);
      skip_newlines ();
      skip_case_clause_body ();
    }
    else
    {
      JERRY_UNREACHABLE ();
    }
  }
  current_token_must_be (TOK_CLOSE_BRACE);

  if (WAS_DEFAULT ())
  {
    STACK_PUSH (U16, OPCODE_COUNTER ());
    DUMP_OPCODE_2 (jmp_down, INVALID_VALUE, INVALID_VALUE);
  }

  lexer_seek (STACK_TOP (locs));
  next_token_must_be (TOK_OPEN_BRACE);

  push_nesting (NESTING_SWITCH);
  // Second, parse case clauses' bodies and rewrite jumps
  skip_newlines ();
  while (is_keyword (KW_CASE) || is_keyword (KW_DEFAULT))
  {
    if (is_keyword (KW_CASE))
    {
      while (!token_is (TOK_COLON))
      {
        skip_newlines ();
      }
      REWRITE_OPCODE_3 (STACK_ELEMENT (U16, STACK_HEAD (U8, 2) + CURRENT_JUMP ()),
                        is_true_jmp_down,
                        STACK_ELEMENT (IDX, STACK_HEAD (U8, 3) + CURRENT_JUMP () + 1),
                        (idx_t) ((OPCODE_COUNTER () - STACK_ELEMENT (
                          U16, STACK_HEAD (U8, 2) + CURRENT_JUMP ())) >> JERRY_BITSINBYTE),
                        (idx_t) ((OPCODE_COUNTER () - STACK_ELEMENT (
                          U16, STACK_HEAD (U8, 2) + CURRENT_JUMP ())) & ((1 << JERRY_BITSINBYTE) - 1)));
      skip_newlines ();
      if (is_keyword (KW_CASE) || is_keyword (KW_DEFAULT))
      {
        goto next;
      }
      parse_statement_list ();
    }
    else if (is_keyword (KW_DEFAULT))
    {
      token_after_newlines_must_be (TOK_COLON);
      skip_newlines ();
      if (is_keyword (KW_CASE) || is_keyword (KW_DEFAULT))
      {
        continue;
      }
      parse_statement_list ();
      continue;
    }
    else
    {
      JERRY_UNREACHABLE ();
    }
    skip_newlines ();

next:
    INCR_CURRENT_JUMP ();
  }
  current_token_must_be (TOK_CLOSE_BRACE);
  skip_token ();
  pop_nesting (NESTING_SWITCH);

  // Finally, dump 'finally' jump
  if (WAS_DEFAULT ())
  {
    REWRITE_JMP (STACK_TOP (U16), jmp_down, OPCODE_COUNTER () - STACK_TOP (U16));
  }

  rewrite_rewritable_opcodes (REWRITABLE_BREAK, STACK_TOP (U8), OPCODE_COUNTER ());
  STACK_DROP (U16, STACK_SIZE (U16) - STACK_HEAD (U8, 2));
  STACK_DROP (IDX, STACK_SIZE (IDX) - STACK_HEAD (U8, 3));
  STACK_DROP (U8, 5);
  STACK_DROP (locs, 1);

#undef WAS_DEFAULT
#undef SET_WAS_DEFAULT
#undef CURRENT_JUMP
#undef INCR_CURRENT_JUMP
#undef SWITCH_EXPR

  STACK_CHECK_USAGE (locs);
  STACK_CHECK_USAGE (IDX);
  STACK_CHECK_USAGE (U16);
  STACK_CHECK_USAGE (U8);
  STACK_CHECK_USAGE (rewritable_break);
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
  check_for_eval_and_arguments_in_strict_mode (STACK_TOP (IDX));
  token_after_newlines_must_be (TOK_CLOSE_PAREN);

  STACK_PUSH (U16, OPCODE_COUNTER ());
  DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_CATCH, INVALID_VALUE, INVALID_VALUE);
  DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_CATCH_EXCEPTION_IDENTIFIER, ID(1), INVALID_VALUE);

  token_after_newlines_must_be (TOK_OPEN_BRACE);
  skip_newlines ();
  parse_statement_list ();
  next_token_must_be (TOK_CLOSE_BRACE);

  rewrite_meta_opcode_counter (STACK_TOP (U16), OPCODE_META_TYPE_CATCH);

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

  rewrite_meta_opcode_counter (STACK_TOP (U16), OPCODE_META_TYPE_FINALLY);

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

  REWRITE_TRY (STACK_TOP (U16));

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
    EMIT_ERROR ("Expected either 'catch' or 'finally' token");
  }

  DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_END_TRY_CATCH_FINALLY, INVALID_VALUE, INVALID_VALUE);

  STACK_DROP (U16, 1);

  STACK_CHECK_USAGE (U16);
}

static void
insert_semicolon (void)
{
  // We cannot use TOK (), since we may use lexer_save_token
  skip_token ();
  if (token_is (TOK_NEWLINE) || lexer_prev_token ().type == TOK_NEWLINE)
  {
    lexer_save_token (TOK ());
    return;
  }
  if (token_is (TOK_CLOSE_BRACE))
  {
    lexer_save_token (TOK ());
    return;
  }
  else if (!token_is (TOK_SEMICOLON))
  {
    EMIT_ERROR ("Expected either ';' or newline token");
  }
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
  if (is_keyword (KW_FUNCTION))
  {
    parse_function_declaration ();
    goto cleanup;
  }
  if (token_is (TOK_SEMICOLON))
  {
    goto cleanup;
  }
  if (is_keyword (KW_CASE) || is_keyword (KW_DEFAULT))
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
    add_to_rewritable_opcodes (REWRITABLE_BREAK, OPCODE_COUNTER ());
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
      DUMP_OPCODE_1 (retval, ID(1));
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

    DUMP_OPCODE_1 (throw, ID(1));
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
      EMIT_SORRY ("Labelled statements are not supported yet");
    }
    else
    {
      lexer_save_token (TOK ());
      SET_TOK (STACK_TOP (toks));
      STACK_DROP (toks, 1);
      parse_expression ();
      STACK_DROP (IDX, 1);
      skip_newlines ();
      if (!token_is (TOK_SEMICOLON))
      {
        lexer_save_token (TOK ());
      }
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

static void
skip_optional_name_and_parens (void)
{
  if (token_is (TOK_NAME))
  {
    token_after_newlines_must_be (TOK_OPEN_PAREN);
  }
  else
  {
    current_token_must_be (TOK_OPEN_PAREN);
  }

  while (!token_is (TOK_CLOSE_PAREN))
  {
    skip_newlines ();
  }
}

static void
skip_braces (void)
{
  STACK_DECLARE_USAGE (U8)

  current_token_must_be (TOK_OPEN_BRACE);

  STACK_PUSH (U8, 1);

  while (STACK_TOP (U8) > 0)
  {
    skip_newlines ();
    if (token_is (TOK_OPEN_BRACE))
    {
      STACK_INCR_HEAD (U8, 1);
    }
    else if (token_is (TOK_CLOSE_BRACE))
    {
      STACK_DECR_HEAD (U8, 1);
    }
  }

  STACK_DROP (U8,1);
  STACK_CHECK_USAGE (U8);
}

static void
skip_function (void)
{
  skip_newlines ();
  skip_optional_name_and_parens ();
  skip_newlines ();
  skip_braces ();
}

static void
skip_squares (void)
{
  STACK_DECLARE_USAGE (U8);

  current_token_must_be (TOK_OPEN_SQUARE);

  STACK_PUSH (U8, 1);

  while (STACK_TOP (U8) > 0)
  {
    skip_newlines ();
    if (token_is (TOK_OPEN_SQUARE))
    {
      STACK_INCR_HEAD (U8, 1);
    }
    else if (token_is (TOK_CLOSE_SQUARE))
    {
      STACK_DECR_HEAD (U8, 1);
    }
  }

  STACK_DROP (U8,1);
  STACK_CHECK_USAGE (U8);
}

static void
skip_parens (void)
{
  STACK_DECLARE_USAGE (U8);

  current_token_must_be (TOK_OPEN_PAREN);

  STACK_PUSH (U8, 1);

  while (STACK_TOP (U8) > 0)
  {
    skip_newlines ();
    if (token_is (TOK_OPEN_PAREN))
    {
      STACK_INCR_HEAD (U8, 1);
    }
    else if (token_is (TOK_CLOSE_PAREN))
    {
      STACK_DECR_HEAD (U8, 1);
    }
  }
  
  STACK_DROP (U8,1);
  STACK_CHECK_USAGE (U8);
}

static bool
var_declared (idx_t var_id)
{
  STACK_DECLARE_USAGE (U16)
  STACK_DECLARE_USAGE (ops)
  bool result = false;

  STACK_PUSH (U16, (opcode_counter_t) (OPCODE_COUNTER () - 1));
  STACK_PUSH (ops, deserialize_opcode (STACK_TOP (U16)));

  while (OPCODE_IS (STACK_TOP (ops), var_decl))
  {
    if (STACK_TOP (ops).data.var_decl.variable_name == var_id)
    {
      result = true;
      goto cleanup;
    }
    STACK_DECR_HEAD (U16, 1);
    STACK_SET_HEAD (ops, 1, deserialize_opcode (STACK_TOP (U16)));
  }

cleanup:
  STACK_DROP (U16, 1);
  STACK_DROP (ops, 1);

  STACK_CHECK_USAGE (U16);
  STACK_CHECK_USAGE (ops);

  return result;
}

static void
preparse_var_decls (void)
{
  assert_keyword (KW_VAR);

  skip_newlines ();
  while (!token_is (TOK_NEWLINE) && !token_is (TOK_SEMICOLON) && !is_keyword (KW_IN))
  {
    if (token_is (TOK_NAME))
    {
      if (!var_declared (token_data ()))
      {
        DUMP_OPCODE_1 (var_decl, token_data ());
      }
      skip_token ();
      continue;
    }
    else if (token_is (TOK_EQ))
    {
      while (!token_is (TOK_COMMA) && !token_is (TOK_NEWLINE) && !token_is (TOK_SEMICOLON))
      {
        if (is_keyword (KW_FUNCTION))
        {
          skip_function ();
        }
        else if (token_is (TOK_OPEN_BRACE))
        {
          skip_braces ();
        }
        else if (token_is (TOK_OPEN_SQUARE))
        {
          skip_squares ();
        }
        else if (token_is (TOK_OPEN_PAREN))
        {
          skip_parens ();
        }
        skip_token ();
      }
    }
    else if (!token_is (TOK_COMMA))
    {
      EMIT_ERROR ("Expected ','");
    }
    else
    {
      skip_token ();
      continue;
    }
  }
}

static void
check_for_eval_and_arguments_in_var_decls (opcode_counter_t first_var_decl)
{
  if (!parser_strict_mode ())
  {
    return;
  }

  STACK_DECLARE_USAGE (ops);

  for (opcode_counter_t oc = first_var_decl; oc < OPCODE_COUNTER (); oc = (opcode_counter_t) (oc + 1))
  {
    STACK_PUSH (ops, deserialize_opcode (oc));
    if (!OPCODE_IS (STACK_TOP (ops), var_decl))
    {
      STACK_DROP (ops, 1);
      break;
    }
    check_for_eval_and_arguments_in_strict_mode (STACK_TOP (ops).data.var_decl.variable_name);
    STACK_DROP (ops, 1);
  }

  STACK_CHECK_USAGE (ops);
}

static void
preparse_scope (bool is_global)
{
  STACK_DECLARE_USAGE (locs);
  STACK_DECLARE_USAGE (U8)
  STACK_DECLARE_USAGE (U16)

  STACK_PUSH (locs, TOK ().loc);
  STACK_PUSH (U8, is_global ? TOK_EOF : TOK_CLOSE_BRACE);

  STACK_PUSH (U16, OPCODE_COUNTER ());

  if (token_is (TOK_STRING) && lp_string_equal_s (lexer_get_string_by_id (token_data ()), "use strict"))
  {
    scopes_tree_set_strict_mode (STACK_TOP (scopes), true);
    DUMP_OPCODE_3 (meta, OPCODE_META_TYPE_STRICT_CODE, INVALID_VALUE, INVALID_VALUE);
  }

  while (!token_is (STACK_TOP (U8)))
  {
    if (is_keyword (KW_VAR))
    {
      preparse_var_decls ();
    }
    else if (is_keyword (KW_FUNCTION))
    {
      STACK_PUSH (U8, scopes_tree_strict_mode (STACK_TOP (scopes)) ? 1 : 0);
      scopes_tree_set_strict_mode (STACK_TOP (scopes), false);
      skip_function ();
      scopes_tree_set_strict_mode (STACK_TOP (scopes), STACK_TOP (U8) != 0);
      STACK_DROP (U8, 1);
    }
    else if (token_is (TOK_OPEN_BRACE))
    {
      STACK_PUSH (U8, scopes_tree_strict_mode (STACK_TOP (scopes)) ? 1 : 0);
      scopes_tree_set_strict_mode (STACK_TOP (scopes), false);
      skip_braces ();
      scopes_tree_set_strict_mode (STACK_TOP (scopes), STACK_TOP (U8) != 0);
      STACK_DROP (U8, 1);
    }
    skip_newlines ();
  }

  if (parser_strict_mode ())
  {
    check_for_eval_and_arguments_in_var_decls ((opcode_counter_t) (STACK_TOP (U16) + 1));
  }

  lexer_seek (STACK_TOP (locs));
  STACK_DROP (locs, 1);
  STACK_DROP (U8, 1);
  STACK_DROP (U16, 1);

  STACK_CHECK_USAGE (U8);
  STACK_CHECK_USAGE (U16);
  STACK_CHECK_USAGE (locs);
}

/* source_element_list
  : source_element (LT!* source_element)*
  ; */
static void
parse_source_element_list (bool is_global)
{
  // U16 reg_var_decl_loc;
  STACK_DECLARE_USAGE (U16)
  STACK_DECLARE_USAGE (IDX)
  STACK_DECLARE_USAGE (temp_names)

  start_new_scope ();
  STACK_PUSH (U16, OPCODE_COUNTER ());
  DUMP_OPCODE_2 (reg_var_decl, MIN_TEMP_NAME (), INVALID_VALUE);

  preparse_scope (is_global);

  skip_newlines ();
  while (!token_is (TOK_EOF) && !token_is (TOK_CLOSE_BRACE))
  {
    parse_source_element ();
    skip_newlines ();
  }
  lexer_save_token (TOK ());
  REWRITE_OPCODE_2 (STACK_TOP (U16), reg_var_decl, MIN_TEMP_NAME (), MAX_TEMP_NAME ());
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
  STACK_DECLARE_USAGE (scopes);

  STACK_PUSH (scopes, scopes_tree_init (NULL));
  serializer_set_scope (STACK_TOP (scopes));

  skip_newlines ();
  parse_source_element_list (true);

  skip_newlines ();
  JERRY_ASSERT (token_is (TOK_EOF));
  DUMP_OPCODE_1 (exitval, 0);

  serializer_merge_scopes_into_bytecode ();

  scopes_tree_free (STACK_TOP (scopes));
  serializer_set_scope (NULL);
  STACK_DROP (scopes, 1);

  STACK_CHECK_USAGE (IDX);
  STACK_CHECK_USAGE (scopes);
}

bool
parser_strict_mode (void)
{
  if (STACK_SIZE (scopes) > 0)
  {
    return scopes_tree_strict_mode (STACK_TOP (scopes));
  }
  else
  {
    return false;
  }
}

void
parser_init (const char *source, size_t source_size, bool show_opcodes)
{
  lexer_init (source, source_size, show_opcodes);
  serializer_init (show_opcodes);

  lexer_run_first_pass ();

  lexer_adjust_num_ids ();

  const lp_string *identifiers = lexer_get_strings ();

  serializer_dump_strings_and_nums (identifiers, lexer_get_strings_count (),
                                    lexer_get_nums (), lexer_get_nums_count ());

  STACK_INIT (U8);
  STACK_INIT (IDX);
  STACK_INIT (nestings);
  STACK_INIT (temp_names);
  STACK_INIT (toks);
  STACK_INIT (ops);
  STACK_INIT (U16);
  STACK_INIT (rewritable_continue);
  STACK_INIT (rewritable_break);
  STACK_INIT (locs);
  STACK_INIT (scopes);
  STACK_INIT (props);
  STACK_INIT (strings);

  HASH_INIT (intrinsics, 1);

  fill_intrinsics ();

  SET_MAX_TEMP_NAME (lexer_get_reserved_ids_count ());
  SET_TEMP_NAME (lexer_get_reserved_ids_count ());
  SET_MIN_TEMP_NAME (lexer_get_reserved_ids_count ());
  SET_OPCODE_COUNTER (0);
  STACK_SET_ELEMENT (U8, no_in, 0);

  TODO (/* Rewrite using hash when number of natives reaches 20 */)
  for (uint8_t i = 0; i < OPCODE_NATIVE_CALL__COUNT; i++)
  {
    STACK_SET_ELEMENT (IDX, i, INVALID_VALUE);
  }

  for (uint8_t i = 0, strs_count = lexer_get_strings_count (); i < strs_count; i++)
  {
    if (lp_string_equal_s (identifiers[i], "LEDToggle"))
    {
      STACK_SET_ELEMENT (IDX, OPCODE_NATIVE_CALL_LED_TOGGLE, i);
    }
    else if (lp_string_equal_s (identifiers[i], "LEDOn"))
    {
      STACK_SET_ELEMENT (IDX, OPCODE_NATIVE_CALL_LED_ON, i);
    }
    else if (lp_string_equal_s (identifiers[i], "LEDOff"))
    {
      STACK_SET_ELEMENT (IDX, OPCODE_NATIVE_CALL_LED_OFF, i);
    }
    else if (lp_string_equal_s (identifiers[i], "LEDOnce"))
    {
      STACK_SET_ELEMENT (IDX, OPCODE_NATIVE_CALL_LED_ONCE, i);
    }
    else if (lp_string_equal_s (identifiers[i], "wait"))
    {
      STACK_SET_ELEMENT (IDX, OPCODE_NATIVE_CALL_WAIT, i);
    }
    else if (lp_string_equal_s (identifiers[i], "print"))
    {
      STACK_SET_ELEMENT (IDX, OPCODE_NATIVE_CALL_PRINT, i);
    }
  }
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
  STACK_FREE (locs);
  STACK_FREE (scopes);
  STACK_FREE (props);
  STACK_FREE (strings);

  HASH_FREE (intrinsics);

  serializer_free ();
  lexer_free ();
}
