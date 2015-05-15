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

#include "ecma-helpers.h"
#include "jrt-libc-includes.h"
#include "mem-heap.h"
#include "re-compiler.h"
#include "re-parser.h"
#include "stdio.h"

#define REGEXP_BYTECODE_BLOCK_SIZE 256UL

void
regexp_dump_bytecode (bytecode_ctx_t *bc_ctx);

static regexp_bytecode_t*
realloc_regexp_bytecode_block (bytecode_ctx_t *bc_ctx)
{
  JERRY_ASSERT (bc_ctx->block_end_p - bc_ctx->block_start_p >= 0);
  size_t old_size = static_cast<size_t> (bc_ctx->block_end_p - bc_ctx->block_start_p);
  JERRY_ASSERT (!bc_ctx->current_p && !bc_ctx->block_end_p && !bc_ctx->block_start_p);

  size_t new_block_size = old_size + REGEXP_BYTECODE_BLOCK_SIZE;
  JERRY_ASSERT (bc_ctx->current_p - bc_ctx->block_start_p >= 0);
  size_t current_ptr_offset = static_cast<size_t> (bc_ctx->current_p - bc_ctx->block_start_p);

  regexp_bytecode_t *new_block_start_p = (regexp_bytecode_t *) mem_heap_alloc_block (new_block_size,
                                                                                     MEM_HEAP_ALLOC_SHORT_TERM);
  if (bc_ctx->current_p)
  {
    memcpy (new_block_start_p, bc_ctx->block_start_p, static_cast<size_t> (current_ptr_offset));
    mem_heap_free_block (bc_ctx->block_start_p);
  }
  bc_ctx->block_start_p = new_block_start_p;
  bc_ctx->block_end_p = new_block_start_p + new_block_size;
  bc_ctx->current_p = new_block_start_p + current_ptr_offset;

  return bc_ctx->current_p;
} /* realloc_regexp_bytecode_block */

static void
bytecode_list_append (bytecode_ctx_t *bc_ctx, regexp_bytecode_t bytecode)
{
  regexp_bytecode_t *current_p = bc_ctx->current_p;
  if (current_p  + sizeof (regexp_bytecode_t) > bc_ctx->block_end_p)
  {
    current_p = realloc_regexp_bytecode_block (bc_ctx);
  }

  *current_p = bytecode;
  bc_ctx->current_p += sizeof (regexp_bytecode_t);
} /* bytecode_list_append */

#if 0 /* unused */
static void
bytecode_list_insert (bytecode_ctx_t *bc_ctx, regexp_bytecode_t bytecode, size_t offset)
{
  regexp_bytecode_t *current_p = bc_ctx->current_p;
  if (current_p  + sizeof (regexp_bytecode_t) > bc_ctx->block_end_p)
  {
    current_p = realloc_regexp_bytecode_block (bc_ctx);
  }
  regexp_bytecode_t *src = current_p - offset;
  regexp_bytecode_t *dest = src + sizeof (regexp_bytecode_t);
  memcpy (dest, src, offset);

  *(current_p - offset) = bytecode;
  bc_ctx->current_p += sizeof (regexp_bytecode_t);
} /* bytecode_list_insert  */
#endif

static void
append_opcode (bytecode_ctx_t *bc_ctx, regexp_opcode_t opcode)
{
  bytecode_list_append (bc_ctx, (regexp_bytecode_t) opcode);
}

static void
append_u32 (bytecode_ctx_t *bc_ctx, uint32_t value)
{
  bytecode_list_append (bc_ctx, (regexp_bytecode_t) value);
}

regexp_opcode_t
get_opcode (regexp_bytecode_t **bc_p)
{
  regexp_bytecode_t bytecode = **bc_p;
  (*bc_p)++;
  return (regexp_opcode_t) bytecode;
}

uint32_t
get_value (regexp_bytecode_t **bc_p)
{
  /* FIXME: Read 32bit! */
  regexp_bytecode_t bytecode = **bc_p;
  (*bc_p)++;
  return (uint32_t) bytecode;
}

/**
 * Compilation of RegExp bytecode
 */
void
regexp_compile_bytecode (ecma_property_t *bytecode, /**< bytecode */
                         ecma_string_t *pattern __attr_unused___) /**< pattern */
{
  re_token_t re_tok;
  bytecode_ctx_t bc_ctx;
  bc_ctx.block_start_p = NULL;
  bc_ctx.block_end_p = NULL;
  bc_ctx.current_p = NULL;

  int32_t chars = ecma_string_get_length (pattern);
  MEM_DEFINE_LOCAL_ARRAY (pattern_start_p, chars + 1, ecma_char_t);
  ssize_t zt_str_size = (ssize_t) sizeof (ecma_char_t) * (chars + 1);
  ecma_string_to_zt_string (pattern, pattern_start_p, zt_str_size);

  /* 1. Add extra informations for bytecode header */
  append_u32 (&bc_ctx, 0);
  append_u32 (&bc_ctx, 2); // FIXME: Count the number of capture groups
  append_u32 (&bc_ctx, 0); // FIXME: Count the number of non-capture groups

  append_opcode (&bc_ctx, RE_OP_SAVE_AT_START);

  ecma_char_t *pattern_char_p = pattern_start_p;
  while (pattern_char_p && *pattern_char_p != '\0')
  {
    re_tok = re_parse_next_token (&pattern_char_p);

    switch (re_tok.type)
    {
      case RE_TOK_CHAR:
      {
        append_opcode (&bc_ctx, RE_OP_CHAR);
        append_u32 (&bc_ctx, re_tok.value);
        break;
      }
      default:
      {
        break;
      }
    }
  }

  append_opcode (&bc_ctx, RE_OP_SAVE_AND_MATCH);
  append_opcode (&bc_ctx, RE_OP_EOF);

  MEM_FINALIZE_LOCAL_ARRAY (pattern_start_p);

  ECMA_SET_POINTER (bytecode->u.internal_property.value, bc_ctx.block_start_p);

  regexp_dump_bytecode (&bc_ctx);
} /* regexp_compile_bytecode */

/**
 * RegExp bytecode dumper
 */
void
regexp_dump_bytecode (bytecode_ctx_t *bc_ctx)
{
  regexp_bytecode_t *bytecode_p = bc_ctx->block_start_p;
  fprintf (stderr, "%d %d %d | ",
           get_value (&bytecode_p),
           get_value (&bytecode_p),
           get_value (&bytecode_p));

  regexp_opcode_t op;
  while ((op = get_opcode (&bytecode_p)))
  {
    switch (op)
    {
      case RE_OP_MATCH:
      {
        fprintf (stderr, "MATCH ");
        break;
      }
      case RE_OP_CHAR:
      {
        fprintf (stderr, "CHAR ");
        fprintf (stderr, "%d, ", get_value (&bytecode_p));
        break;
      }
      case RE_OP_SAVE_AT_START:
      {
        fprintf (stderr, "RE_START ");
        break;
      }
      case RE_OP_SAVE_AND_MATCH:
      {
        fprintf (stderr, "RE_END ");
        break;
      }
      default:
      {
        fprintf (stderr, "UNKNOWN ");
        break;
      }
    }
  }
  fprintf (stderr, "EOF\n");
} /* regexp_dump_bytecode */
