/* Copyright 2015 Samsung Electronics Co., Ltd.
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

#ifndef JSP_INTERNAL_H
#define JSP_INTERNAL_H

#include "scopes-tree.h"

/**
 * Parse stage
 */
typedef enum
{
  PREPARSE, /**< preparse stage */
  DUMP /**< dump stage */
} jsp_parse_mode_t;

/**
 * Size of the temporary literal set
 */
#define SCOPE_TMP_LIT_SET_SIZE 32

/**
 * Parser context
 */
typedef struct
{
  struct jsp_state_t *state_stack_p; /**< parser stack */

  jsp_parse_mode_t mode; /**< parse stage */
  scope_type_t scope_type; /**< type of currently parsed scope */

  uint16_t processed_child_scopes_counter; /**< number of processed
                                            *   child scopes of the
                                            *   current scope */

  union
  {
    /**
     * Preparse stage information
     */
    struct
    {
      /**
       * Currently parsed scope
       */
      scopes_tree current_scope_p;

      /**
       * Container of the temporary literal set
       *
       * Temporary literal set is used for estimation of number of unique literals
       * in a byte-code instructions block (BLOCK_SIZE). The calculated number
       * is always equal or larger than actual number of unique literals.
       *
       * The set is emptied upon:
       *  - reaching a bytecode block border;
       *  - changing scope, to which instructions are dumped.
       *
       * Emptying the set in second case is necessary, as the set should contain
       * unique literals of a bytecode block, and, upon switching to another scope,
       * current bytecode block is also switched, correspondingly. However, this
       * could only lead to overestimation of unique literals number, relatively
       * to the actual number.
       */
      lit_cpointer_t tmp_lit_set[SCOPE_TMP_LIT_SET_SIZE];

      /**
       * Number of items in the temporary literal set
       */
      uint8_t tmp_lit_set_num;
    } preparse_stage;

    /**
     * Dump stage information
     */
    struct
    {
      /**
       * Current scope's byte-code header
       */
      bytecode_data_header_t *current_bc_header_p;

      /**
       * Pointer to the scope that would be parsed next
       */
      scopes_tree next_scope_to_dump_p;
    } dump_stage;
  } u;
} jsp_ctx_t;

void jsp_init_ctx (jsp_ctx_t *, scope_type_t);
bool jsp_is_dump_mode (jsp_ctx_t *);
void jsp_switch_to_dump_mode (jsp_ctx_t *, scopes_tree);
bool jsp_is_strict_mode (jsp_ctx_t *);
void jsp_set_strict_mode (jsp_ctx_t *);
scope_type_t jsp_get_scope_type (jsp_ctx_t *);
void jsp_set_scope_type (jsp_ctx_t *, scope_type_t);
uint16_t jsp_get_processed_child_scopes_counter (jsp_ctx_t *);
void jsp_set_processed_child_scopes_counter (jsp_ctx_t *, uint16_t);
uint16_t jsp_get_and_inc_processed_child_scopes_counter (jsp_ctx_t *);
scopes_tree jsp_get_next_scopes_tree_node_to_dump (jsp_ctx_t *);
scopes_tree jsp_get_current_scopes_tree_node (jsp_ctx_t *);
void jsp_set_current_scopes_tree_node (jsp_ctx_t *, scopes_tree);
bytecode_data_header_t *jsp_get_current_bytecode_header (jsp_ctx_t *);
void jsp_set_current_bytecode_header (jsp_ctx_t *, bytecode_data_header_t *);
void jsp_empty_tmp_literal_set (jsp_ctx_t *);
void jsp_account_next_bytecode_to_literal_reference (jsp_ctx_t *, lit_cpointer_t);

#endif /* !JSP_INTERNAL_H */
