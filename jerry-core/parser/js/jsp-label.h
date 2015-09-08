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

#ifndef JSP_LABEL_H
#define JSP_LABEL_H

#include "lexer.h"
#include "opcodes.h"

/** \addtogroup jsparser ECMAScript parser
 * @{
 *
 * \addtogroup labels Jump labels
 * @{
 */

/**
 * Label types
 */
typedef enum
{
  JSP_LABEL_TYPE_NAMED = (1 << 0), /**< label for breaks and continues with identifiers */
  JSP_LABEL_TYPE_UNNAMED_BREAKS = (1 << 1), /**< label for breaks without identifiers */
  JSP_LABEL_TYPE_UNNAMED_CONTINUES = (1 << 2) /**< label for continues without identifiers */
} jsp_label_type_flag_t;

/**
 * Descriptor of a jump label (See also: ECMA-262 v5, 12.12, Labelled statements)
 *
 * Note:
 *      Jump instructions with target identified by some specific label,
 *      are linked into singly-linked list.
 *
 *      Pointer to a next element of the list is represented with instruction counter.
 *      stored in instructions linked into the list.
 *
 */
typedef struct jsp_label_t
{
  jsp_label_type_flag_t type_mask; /**< label type mask */
  token id; /**< label name (TOK_NAME), if type is LABEL_NAMED */
  vm_instr_counter_t continue_tgt_oc; /**< target instruction counter for continues on the label */
  vm_instr_counter_t breaks_list_oc; /**< instruction counter of first 'break' instruction in the list
                                      *   of instructions with the target identified by the label */
  vm_instr_counter_t breaks_number; /**< number of 'break' instructions in the list */
  vm_instr_counter_t continues_list_oc; /**< instruction counter of first 'continue' instruction in the list
                                         *   of instructions with the target identified by the label */
  vm_instr_counter_t continues_number; /**< number of 'continue' instructions in the list */
  jsp_label_t *next_label_p; /**< next label in current label set stack */
  bool is_nested_jumpable_border : 1; /**< flag, indicating that this and outer labels
                                       *   are not currently accessible with simple jumps,
                                       *   and so should be targetted with nested jumps only */
} jsp_label_t;

extern void jsp_label_init (void);
extern void jsp_label_finalize (void);

extern void jsp_label_remove_all_labels (void);

extern void jsp_label_push (jsp_label_t *, jsp_label_type_flag_t, token);
extern void jsp_label_rewrite_jumps_and_pop (jsp_label_t *, vm_instr_counter_t);

extern jsp_label_t *jsp_label_find (jsp_label_type_flag_t, token, bool *);

extern void jsp_label_add_jump (jsp_label_t *, bool, bool);
extern void jsp_label_setup_continue_target (jsp_label_t *, vm_instr_counter_t);

extern bool jsp_label_raise_nested_jumpable_border (void);
extern void jsp_label_remove_nested_jumpable_border (void);

extern jsp_label_t *jsp_label_mask_set (void);
extern void jsp_label_restore_set (jsp_label_t *);

/**
 * @}
 * @}
 */

#endif /* !JSP_LABEL_H */
