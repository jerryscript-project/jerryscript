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

#ifndef RE_BYTECODE_H
#define RE_BYTECODE_H

#ifndef CONFIG_DISABLE_REGEXP_BUILTIN

#include "ecma-globals.h"

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
  * Size of the RegExp bytecode cache
  */
#define RE_CACHE_SIZE 8u

/**
  * RegExp flags mask (first 10 bits are for reference count and the rest for the actual RegExp flags)
  */
#define RE_FLAGS_MASK 0x3F

/**
 * RegExp opcodes
 */
typedef enum
{
  RE_OP_EOF,
  /* Group opcode order is important, because RE_IS_CAPTURE_GROUP is based on it.
   * Change it carefully. Capture opcodes should be at first.
   */
  RE_OP_CAPTURE_GROUP_START,                      /**< group start */
  RE_OP_CAPTURE_GREEDY_ZERO_GROUP_START,          /**< greedy zero group start */
  RE_OP_CAPTURE_NON_GREEDY_ZERO_GROUP_START,      /**< non-greedy zero group start */
  RE_OP_CAPTURE_GREEDY_GROUP_END,                 /**< greedy group end */
  RE_OP_CAPTURE_NON_GREEDY_GROUP_END,             /**< non-greedy group end */
  RE_OP_NON_CAPTURE_GROUP_START,                  /**< non-capture group start */
  RE_OP_NON_CAPTURE_GREEDY_ZERO_GROUP_START,      /**< non-capture greedy zero group start */
  RE_OP_NON_CAPTURE_NON_GREEDY_ZERO_GROUP_START,  /**< non-capture non-greedy zero group start */
  RE_OP_NON_CAPTURE_GREEDY_GROUP_END,             /**< non-capture greedy group end */
  RE_OP_NON_CAPTURE_NON_GREEDY_GROUP_END,         /**< non-capture non-greedy group end */

  RE_OP_MATCH,                                    /**< match */
  RE_OP_CHAR,                                     /**< any character */
  RE_OP_SAVE_AT_START,                            /**< save at start */
  RE_OP_SAVE_AND_MATCH,                           /**< save and match */
  RE_OP_PERIOD,                                   /**< "." */
  RE_OP_ALTERNATIVE,                              /**< "|" */
  RE_OP_GREEDY_ITERATOR,                          /**< greedy iterator */
  RE_OP_NON_GREEDY_ITERATOR,                      /**< non-greedy iterator */
  RE_OP_ASSERT_START,                             /**< "^" */
  RE_OP_ASSERT_END,                               /**< "$" */
  RE_OP_ASSERT_WORD_BOUNDARY,                     /**< "\b" */
  RE_OP_ASSERT_NOT_WORD_BOUNDARY,                 /**< "\B" */
  RE_OP_LOOKAHEAD_POS,                            /**< lookahead pos */
  RE_OP_LOOKAHEAD_NEG,                            /**< lookahead neg */
  RE_OP_BACKREFERENCE,                            /**< "\[0..9]" */
  RE_OP_CHAR_CLASS,                               /**< "[ ]" */
  RE_OP_INV_CHAR_CLASS                            /**< "[^ ]" */
} re_opcode_t;

/**
 * Compiled byte code data.
 */
typedef struct
{
  ecma_compiled_code_t header;       /**< compiled code header */
  jmem_cpointer_t pattern_cp;        /**< original RegExp pattern */
  uint32_t num_of_captures;          /**< number of capturing brackets */
  uint32_t num_of_non_captures;      /**< number of non capturing brackets */
} re_compiled_code_t;

/**
 * Context of RegExp bytecode container
 */
typedef struct
{
  uint8_t *block_start_p;      /**< start of bytecode block */
  uint8_t *block_end_p;        /**< end of bytecode block */
  uint8_t *current_p;          /**< current position in bytecode */
} re_bytecode_ctx_t;

re_opcode_t re_get_opcode (uint8_t **bc_p);
ecma_char_t re_get_char (uint8_t **bc_p);
uint32_t re_get_value (uint8_t **bc_p);
uint32_t re_get_bytecode_length (re_bytecode_ctx_t *bc_ctx_p) __attr_pure___;

void re_append_opcode (re_bytecode_ctx_t *bc_ctx_p, re_opcode_t opcode);
void re_append_u32 (re_bytecode_ctx_t *bc_ctx_p, uint32_t value);
void re_append_char (re_bytecode_ctx_t *bc_ctx_p, ecma_char_t input_char);
void re_append_jump_offset (re_bytecode_ctx_t *bc_ctx_p, uint32_t value);

void re_insert_opcode (re_bytecode_ctx_t *bc_ctx_p, uint32_t offset, re_opcode_t opcode);
void re_insert_u32 (re_bytecode_ctx_t *bc_ctx_p, uint32_t offset, uint32_t value);
void re_bytecode_list_insert (re_bytecode_ctx_t *bc_ctx_p, size_t offset, uint8_t *bytecode_p, size_t length);

#ifdef REGEXP_DUMP_BYTE_CODE
void re_dump_bytecode (re_bytecode_ctx_t *bc_ctx);
#endif /* REGEXP_DUMP_BYTE_CODE */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */
#endif /* !RE_BYTECODE_H */
