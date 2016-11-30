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

#include "ecma-globals.h"
#include "re-bytecode.h"

#ifndef CONFIG_DISABLE_REGEXP_BUILTIN

/** \addtogroup parser Parser
 * @{
 *
 * \addtogroup regexparser Regular expression
 * @{
 *
 * \addtogroup regexparser_bytecode Bytecode
 * @{
 */

/**
 * Size of block of RegExp bytecode. Used for allocation
 */
#define REGEXP_BYTECODE_BLOCK_SIZE 256UL

/**
 * Realloc the bytecode container
 *
 * @return current position in RegExp bytecode
 */
static uint8_t *
re_realloc_regexp_bytecode_block (re_bytecode_ctx_t *bc_ctx_p) /**< RegExp bytecode context */
{
  JERRY_ASSERT (bc_ctx_p->block_end_p >= bc_ctx_p->block_start_p);
  size_t old_size = (size_t) (bc_ctx_p->block_end_p - bc_ctx_p->block_start_p);

  /* If one of the members of RegExp bytecode context is NULL, then all member should be NULL
   * (it means first allocation), otherwise all of the members should be a non NULL pointer. */
  JERRY_ASSERT ((!bc_ctx_p->current_p && !bc_ctx_p->block_end_p && !bc_ctx_p->block_start_p)
                || (bc_ctx_p->current_p && bc_ctx_p->block_end_p && bc_ctx_p->block_start_p));

  size_t new_block_size = old_size + REGEXP_BYTECODE_BLOCK_SIZE;
  JERRY_ASSERT (bc_ctx_p->current_p >= bc_ctx_p->block_start_p);
  size_t current_ptr_offset = (size_t) (bc_ctx_p->current_p - bc_ctx_p->block_start_p);

  uint8_t *new_block_start_p = (uint8_t *) jmem_heap_alloc_block (new_block_size);
  if (bc_ctx_p->current_p)
  {
    memcpy (new_block_start_p, bc_ctx_p->block_start_p, (size_t) (current_ptr_offset));
    jmem_heap_free_block (bc_ctx_p->block_start_p, old_size);
  }
  bc_ctx_p->block_start_p = new_block_start_p;
  bc_ctx_p->block_end_p = new_block_start_p + new_block_size;
  bc_ctx_p->current_p = new_block_start_p + current_ptr_offset;

  return bc_ctx_p->current_p;
} /* re_realloc_regexp_bytecode_block */

/**
 * Append a new bytecode to the and of the bytecode container
 */
static void
re_bytecode_list_append (re_bytecode_ctx_t *bc_ctx_p, /**< RegExp bytecode context */
                         uint8_t *bytecode_p, /**< input bytecode */
                         size_t length) /**< length of input */
{
  JERRY_ASSERT (length <= REGEXP_BYTECODE_BLOCK_SIZE);

  uint8_t *current_p = bc_ctx_p->current_p;
  if (current_p + length > bc_ctx_p->block_end_p)
  {
    current_p = re_realloc_regexp_bytecode_block (bc_ctx_p);
  }

  memcpy (current_p, bytecode_p, length);
  bc_ctx_p->current_p += length;
} /* re_bytecode_list_append */

/**
 * Insert a new bytecode to the bytecode container
 */
void
re_bytecode_list_insert (re_bytecode_ctx_t *bc_ctx_p, /**< RegExp bytecode context */
                         size_t offset, /**< distance from the start of the container */
                         uint8_t *bytecode_p, /**< input bytecode */
                         size_t length) /**< length of input */
{
  JERRY_ASSERT (length <= REGEXP_BYTECODE_BLOCK_SIZE);

  uint8_t *current_p = bc_ctx_p->current_p;
  if (current_p + length > bc_ctx_p->block_end_p)
  {
    re_realloc_regexp_bytecode_block (bc_ctx_p);
  }

  uint8_t *src_p = bc_ctx_p->block_start_p + offset;
  if ((re_get_bytecode_length (bc_ctx_p) - offset) > 0)
  {
    uint8_t *dest_p = src_p + length;
    uint8_t *tmp_block_start_p;
    tmp_block_start_p = (uint8_t *) jmem_heap_alloc_block (re_get_bytecode_length (bc_ctx_p) - offset);
    memcpy (tmp_block_start_p, src_p, (size_t) (re_get_bytecode_length (bc_ctx_p) - offset));
    memcpy (dest_p, tmp_block_start_p, (size_t) (re_get_bytecode_length (bc_ctx_p) - offset));
    jmem_heap_free_block (tmp_block_start_p, re_get_bytecode_length (bc_ctx_p) - offset);
  }
  memcpy (src_p, bytecode_p, length);

  bc_ctx_p->current_p += length;
} /* re_bytecode_list_insert */

/**
 * Get a character from the RegExp bytecode and increase the bytecode position
 *
 * @return ecma character
 */
ecma_char_t __attr_always_inline___
re_get_char (uint8_t **bc_p) /**< pointer to bytecode start */
{
  ecma_char_t chr = *((ecma_char_t *) *bc_p);
  (*bc_p) += sizeof (ecma_char_t);
  return chr;
} /* re_get_char */

/**
 * Get a RegExp opcode and increase the bytecode position
 *
 * @return current RegExp opcode
 */
re_opcode_t __attr_always_inline___
re_get_opcode (uint8_t **bc_p) /**< pointer to bytecode start */
{
  uint8_t bytecode = **bc_p;
  (*bc_p) += sizeof (uint8_t);
  return (re_opcode_t) bytecode;
} /* re_get_opcode */

/**
 * Get a parameter of a RegExp opcode and increase the bytecode position
 *
 * @return opcode parameter
 */
uint32_t __attr_always_inline___
re_get_value (uint8_t **bc_p) /**< pointer to bytecode start */
{
  uint32_t value = *((uint32_t *) *bc_p);
  (*bc_p) += sizeof (uint32_t);
  return value;
} /* re_get_value */

/**
 * Get length of bytecode
 *
 * @return bytecode length (unsigned integer)
 */
uint32_t __attr_pure___ __attr_always_inline___
re_get_bytecode_length (re_bytecode_ctx_t *bc_ctx_p) /**< RegExp bytecode context */
{
  return ((uint32_t) (bc_ctx_p->current_p - bc_ctx_p->block_start_p));
} /* re_get_bytecode_length */

/**
 * Append a RegExp opcode
 */
void
re_append_opcode (re_bytecode_ctx_t *bc_ctx_p, /**< RegExp bytecode context */
                  re_opcode_t opcode) /**< input opcode */
{
  re_bytecode_list_append (bc_ctx_p, (uint8_t *) &opcode, sizeof (uint8_t));
} /* re_append_opcode */

/**
 * Append a parameter of a RegExp opcode
 */
void
re_append_u32 (re_bytecode_ctx_t *bc_ctx_p, /**< RegExp bytecode context */
               uint32_t value) /**< input value */
{
  re_bytecode_list_append (bc_ctx_p, (uint8_t *) &value, sizeof (uint32_t));
} /* re_append_u32 */

/**
 * Append a character to the RegExp bytecode
 */
void
re_append_char (re_bytecode_ctx_t *bc_ctx_p, /**< RegExp bytecode context */
                ecma_char_t input_char) /**< input char */
{
  re_bytecode_list_append (bc_ctx_p, (uint8_t *) &input_char, sizeof (ecma_char_t));
} /* re_append_char */

/**
 * Append a jump offset parameter of a RegExp opcode
 */
void
re_append_jump_offset (re_bytecode_ctx_t *bc_ctx_p, /**< RegExp bytecode context */
                       uint32_t value) /**< input value */
{
  value += (uint32_t) (sizeof (uint32_t));
  re_append_u32 (bc_ctx_p, value);
} /* re_append_jump_offset */

/**
 * Insert a RegExp opcode
 */
void
re_insert_opcode (re_bytecode_ctx_t *bc_ctx_p, /**< RegExp bytecode context */
                  uint32_t offset, /**< distance from the start of the container */
                  re_opcode_t opcode) /**< input opcode */
{
  re_bytecode_list_insert (bc_ctx_p, offset, (uint8_t *) &opcode, sizeof (uint8_t));
} /* re_insert_opcode */

/**
 * Insert a parameter of a RegExp opcode
 */
void
re_insert_u32 (re_bytecode_ctx_t *bc_ctx_p, /**< RegExp bytecode context */
               uint32_t offset, /**< distance from the start of the container */
               uint32_t value) /**< input value */
{
  re_bytecode_list_insert (bc_ctx_p, offset, (uint8_t *) &value, sizeof (uint32_t));
} /* re_insert_u32 */

#ifdef REGEXP_DUMP_BYTE_CODE
/**
 * RegExp bytecode dumper
 */
void
re_dump_bytecode (re_bytecode_ctx_t *bc_ctx_p) /**< RegExp bytecode context */
{
  re_compiled_code_t *compiled_code_p = (re_compiled_code_t *) bc_ctx_p->block_start_p;
  JERRY_DEBUG_MSG ("%d ", compiled_code_p->header.status_flags);
  JERRY_DEBUG_MSG ("%d ", compiled_code_p->num_of_captures);
  JERRY_DEBUG_MSG ("%d | ", compiled_code_p->num_of_non_captures);

  uint8_t *bytecode_p = (uint8_t *) (compiled_code_p + 1);

  re_opcode_t op;
  while ((op = re_get_opcode (&bytecode_p)))
  {
    switch (op)
    {
      case RE_OP_MATCH:
      {
        JERRY_DEBUG_MSG ("MATCH, ");
        break;
      }
      case RE_OP_CHAR:
      {
        JERRY_DEBUG_MSG ("CHAR ");
        JERRY_DEBUG_MSG ("%c, ", (char) re_get_char (&bytecode_p));
        break;
      }
      case RE_OP_CAPTURE_NON_GREEDY_ZERO_GROUP_START:
      {
        JERRY_DEBUG_MSG ("N");
        /* FALLTHRU */
      }
      case RE_OP_CAPTURE_GREEDY_ZERO_GROUP_START:
      {
        JERRY_DEBUG_MSG ("GZ_START ");
        JERRY_DEBUG_MSG ("%d ", re_get_value (&bytecode_p));
        JERRY_DEBUG_MSG ("%d ", re_get_value (&bytecode_p));
        JERRY_DEBUG_MSG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_CAPTURE_GROUP_START:
      {
        JERRY_DEBUG_MSG ("START ");
        JERRY_DEBUG_MSG ("%d ", re_get_value (&bytecode_p));
        JERRY_DEBUG_MSG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_CAPTURE_NON_GREEDY_GROUP_END:
      {
        JERRY_DEBUG_MSG ("N");
        /* FALLTHRU */
      }
      case RE_OP_CAPTURE_GREEDY_GROUP_END:
      {
        JERRY_DEBUG_MSG ("G_END ");
        JERRY_DEBUG_MSG ("%d ", re_get_value (&bytecode_p));
        JERRY_DEBUG_MSG ("%d ", re_get_value (&bytecode_p));
        JERRY_DEBUG_MSG ("%d ", re_get_value (&bytecode_p));
        JERRY_DEBUG_MSG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_NON_CAPTURE_NON_GREEDY_ZERO_GROUP_START:
      {
        JERRY_DEBUG_MSG ("N");
        /* FALLTHRU */
      }
      case RE_OP_NON_CAPTURE_GREEDY_ZERO_GROUP_START:
      {
        JERRY_DEBUG_MSG ("GZ_NC_START ");
        JERRY_DEBUG_MSG ("%d ", re_get_value (&bytecode_p));
        JERRY_DEBUG_MSG ("%d ", re_get_value (&bytecode_p));
        JERRY_DEBUG_MSG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_NON_CAPTURE_GROUP_START:
      {
        JERRY_DEBUG_MSG ("NC_START ");
        JERRY_DEBUG_MSG ("%d ", re_get_value (&bytecode_p));
        JERRY_DEBUG_MSG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_NON_CAPTURE_NON_GREEDY_GROUP_END:
      {
        JERRY_DEBUG_MSG ("N");
        /* FALLTHRU */
      }
      case RE_OP_NON_CAPTURE_GREEDY_GROUP_END:
      {
        JERRY_DEBUG_MSG ("G_NC_END ");
        JERRY_DEBUG_MSG ("%d ", re_get_value (&bytecode_p));
        JERRY_DEBUG_MSG ("%d ", re_get_value (&bytecode_p));
        JERRY_DEBUG_MSG ("%d ", re_get_value (&bytecode_p));
        JERRY_DEBUG_MSG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_SAVE_AT_START:
      {
        JERRY_DEBUG_MSG ("RE_START ");
        JERRY_DEBUG_MSG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_SAVE_AND_MATCH:
      {
        JERRY_DEBUG_MSG ("RE_END, ");
        break;
      }
      case RE_OP_GREEDY_ITERATOR:
      {
        JERRY_DEBUG_MSG ("GREEDY_ITERATOR ");
        JERRY_DEBUG_MSG ("%d ", re_get_value (&bytecode_p));
        JERRY_DEBUG_MSG ("%d ", re_get_value (&bytecode_p));
        JERRY_DEBUG_MSG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_NON_GREEDY_ITERATOR:
      {
        JERRY_DEBUG_MSG ("NON_GREEDY_ITERATOR ");
        JERRY_DEBUG_MSG ("%d, ", re_get_value (&bytecode_p));
        JERRY_DEBUG_MSG ("%d, ", re_get_value (&bytecode_p));
        JERRY_DEBUG_MSG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_PERIOD:
      {
        JERRY_DEBUG_MSG ("PERIOD ");
        break;
      }
      case RE_OP_ALTERNATIVE:
      {
        JERRY_DEBUG_MSG ("ALTERNATIVE ");
        JERRY_DEBUG_MSG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_ASSERT_START:
      {
        JERRY_DEBUG_MSG ("ASSERT_START ");
        break;
      }
      case RE_OP_ASSERT_END:
      {
        JERRY_DEBUG_MSG ("ASSERT_END ");
        break;
      }
      case RE_OP_ASSERT_WORD_BOUNDARY:
      {
        JERRY_DEBUG_MSG ("ASSERT_WORD_BOUNDARY ");
        break;
      }
      case RE_OP_ASSERT_NOT_WORD_BOUNDARY:
      {
        JERRY_DEBUG_MSG ("ASSERT_NOT_WORD_BOUNDARY ");
        break;
      }
      case RE_OP_LOOKAHEAD_POS:
      {
        JERRY_DEBUG_MSG ("LOOKAHEAD_POS ");
        JERRY_DEBUG_MSG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_LOOKAHEAD_NEG:
      {
        JERRY_DEBUG_MSG ("LOOKAHEAD_NEG ");
        JERRY_DEBUG_MSG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_BACKREFERENCE:
      {
        JERRY_DEBUG_MSG ("BACKREFERENCE ");
        JERRY_DEBUG_MSG ("%d, ", re_get_value (&bytecode_p));
        break;
      }
      case RE_OP_INV_CHAR_CLASS:
      {
        JERRY_DEBUG_MSG ("INV_");
        /* FALLTHRU */
      }
      case RE_OP_CHAR_CLASS:
      {
        JERRY_DEBUG_MSG ("CHAR_CLASS ");
        uint32_t num_of_class = re_get_value (&bytecode_p);
        JERRY_DEBUG_MSG ("%d", num_of_class);
        while (num_of_class)
        {
          JERRY_DEBUG_MSG (" %d", re_get_char (&bytecode_p));
          JERRY_DEBUG_MSG ("-%d", re_get_char (&bytecode_p));
          num_of_class--;
        }
        JERRY_DEBUG_MSG (", ");
        break;
      }
      default:
      {
        JERRY_DEBUG_MSG ("UNKNOWN(%d), ", (uint32_t) op);
        break;
      }
    }
  }
  JERRY_DEBUG_MSG ("EOF\n");
} /* re_dump_bytecode */
#endif /* REGEXP_DUMP_BYTE_CODE */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */
