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

#include "jsp-label.h"
#include "lexer.h"
#include "opcodes-dumper.h"

/** \addtogroup jsparser ECMAScript parser
 * @{
 *
 * \addtogroup labels Jump labels
 * @{
 */

/**
 * Stack, containing current label set
 */
jsp_label_t *label_set_p = NULL;

/**
 * Initialize jumps labels mechanism
 */
void
jsp_label_init (void)
{
  JERRY_ASSERT (label_set_p == NULL);
} /* jsp_label_init */

/**
 * Finalize jumps labels mechanism
 */
void
jsp_label_finalize (void)
{
  JERRY_ASSERT (label_set_p == NULL);
} /* jsp_label_finalize */

/**
 * Remove all labels
 *
 * Note:
 *      should be used only upon a SyntaxError is raised
 */
void
jsp_label_remove_all_labels (void)
{
  label_set_p = NULL;
} /* jsp_label_remove_all_labels */

/**
 * Add label to the current label set
 */
void
jsp_label_push (jsp_label_t *out_label_p, /**< out: place where label structure
                                           *        should be initialized and
                                           *        linked into label set stack */
                jsp_label_type_flag_t type_mask, /**< label's type mask */
                token id) /**< identifier of the label (TOK_NAME)
                           *   if mask includes JSP_LABEL_TYPE_NAMED,
                           *   or empty token - otherwise */
{
  JERRY_ASSERT (out_label_p != NULL);

  if (type_mask & JSP_LABEL_TYPE_NAMED)
  {
    JERRY_ASSERT (id.type == TOK_NAME);

    JERRY_ASSERT (jsp_label_find (JSP_LABEL_TYPE_NAMED, id, NULL) == NULL);
  }
  else
  {
    JERRY_ASSERT (id.type == TOK_EMPTY);
  }

  out_label_p->type_mask = type_mask;
  out_label_p->id = id;
  out_label_p->continue_tgt_oc = MAX_OPCODES;
  out_label_p->breaks_list_oc = MAX_OPCODES;
  out_label_p->breaks_number = 0;
  out_label_p->continues_list_oc = MAX_OPCODES;
  out_label_p->continues_number = 0;
  out_label_p->next_label_p = label_set_p;
  out_label_p->is_nested_jumpable_border = false;

  label_set_p = out_label_p;
} /* jsp_label_push */

/**
 * Rewrite jumps to the label, if there any,
 * and remove it from the current label set
 *
 * @return the label should be on top of label set stack
 */
void
jsp_label_rewrite_jumps_and_pop (jsp_label_t *label_p, /**< label to remove (should be on top of stack) */
                                 vm_instr_counter_t break_tgt_oc) /**< target instruction counter
                                                                   *   for breaks on the label */
{
  JERRY_ASSERT (label_p != NULL);
  JERRY_ASSERT (break_tgt_oc != MAX_OPCODES);
  JERRY_ASSERT (label_set_p == label_p);

  /* Iterating jumps list, rewriting them */
  while (label_p->breaks_number--)
  {
    JERRY_ASSERT (label_p->breaks_list_oc != MAX_OPCODES);

    label_p->breaks_list_oc = rewrite_simple_or_nested_jump_and_get_next (label_p->breaks_list_oc,
                                                                          break_tgt_oc);
  }
  while (label_p->continues_number--)
  {
    JERRY_ASSERT (label_p->continue_tgt_oc != MAX_OPCODES);
    JERRY_ASSERT (label_p->continues_list_oc != MAX_OPCODES);

    label_p->continues_list_oc = rewrite_simple_or_nested_jump_and_get_next (label_p->continues_list_oc,
                                                                             label_p->continue_tgt_oc);
  }

  label_set_p = label_set_p->next_label_p;
} /* jsp_label_rewrite_jumps_and_pop */

/**
 * Find label with specified identifier
 *
 * @return if found, pointer to label descriptor,
 *         otherwise - NULL.
 */
jsp_label_t*
jsp_label_find (jsp_label_type_flag_t type_mask, /**< types to search for */
                token id, /**< identifier of the label (TOK_NAME)
                           *   if mask equals to JSP_LABEL_TYPE_NAMED,
                           *   or empty token - otherwise
                           *   (if so, mask should not include JSP_LABEL_TYPE_NAMED) */
                bool *out_is_simply_jumpable_p) /**< out: is the label currently
                                                 *        accessible with a simple jump */
{
  bool is_search_named = (type_mask == JSP_LABEL_TYPE_NAMED);

  if (is_search_named)
  {
    JERRY_ASSERT (id.type == TOK_NAME);
  }
  else
  {
    JERRY_ASSERT (!(type_mask & JSP_LABEL_TYPE_NAMED));
    JERRY_ASSERT (id.type == TOK_EMPTY);
  }

  bool is_simply_jumpable = true;
  jsp_label_t *ret_label_p = NULL;

  for (jsp_label_t *label_iter_p = label_set_p;
       label_iter_p != NULL;
       label_iter_p = label_iter_p->next_label_p)
  {
    if (label_iter_p->is_nested_jumpable_border)
    {
      is_simply_jumpable = false;
    }

    bool is_named_label = (label_iter_p->type_mask & JSP_LABEL_TYPE_NAMED);
    if ((is_search_named
         && is_named_label
         && lexer_are_tokens_with_same_identifier (label_iter_p->id, id))
        || (!is_search_named
            && (type_mask & label_iter_p->type_mask)))
    {
      ret_label_p = label_iter_p;

      break;
    }
  }

  if (out_is_simply_jumpable_p != NULL)
  {
    *out_is_simply_jumpable_p = is_simply_jumpable;
  }

  return ret_label_p;
} /* jsp_label_find */

/**
 * Dump jump and register it in the specified label to be rewritten later (see also: jsp_label_rewrite_jumps_and_pop)
 *
 * Warning:
 *         The dumped instruction should not be modified before it is rewritten, as its idx fields are used
 *         to link jump instructions related to the label into singly linked list.
 */
void
jsp_label_add_jump (jsp_label_t *label_p, /**< label to register jump for */
                    bool is_simply_jumpable, /**< is the label currently
                                              *    accessible with a simple jump */
                    bool is_break) /**< type of jump - 'break' (true) or 'continue' (false) */
{
  JERRY_ASSERT (label_p != NULL);

  if (is_break)
  {
    label_p->breaks_list_oc = dump_simple_or_nested_jump_for_rewrite (is_simply_jumpable,
                                                                      label_p->breaks_list_oc);
    label_p->breaks_number++;
  }
  else
  {
    label_p->continues_list_oc = dump_simple_or_nested_jump_for_rewrite (is_simply_jumpable,
                                                                         label_p->continues_list_oc);
    label_p->continues_number++;
  }
} /* jsp_label_add_jump */

/**
 * Setup target for 'continue' jumps,
 * associated with the labels, from innermost
 * to the specified label.
 */
void
jsp_label_setup_continue_target (jsp_label_t *outermost_label_p, /**< the outermost label to setup target for */
                                 vm_instr_counter_t tgt_oc) /**< target */
{
  /* There are no labels that could not be targeted with 'break' jumps */
  JERRY_ASSERT (tgt_oc != MAX_OPCODES);
  JERRY_ASSERT (outermost_label_p != NULL);

  for (jsp_label_t *label_iter_p = label_set_p;
       label_iter_p != outermost_label_p->next_label_p;
       label_iter_p = label_iter_p->next_label_p)
  {
    JERRY_ASSERT (label_iter_p != NULL);
    JERRY_ASSERT (label_iter_p->continue_tgt_oc == MAX_OPCODES);

    label_iter_p->continue_tgt_oc = tgt_oc;
  }
} /* jsp_label_setup_continue_target */

/**
 * Add nested jumpable border at current label, if there is any.
 *
 * @return true - if the border is added (in the case, it should be removed
 *                using jsp_label_remove_nested_jumpable_border, when parse of
 *                the corresponding statement would be finished),
 *         false - otherwise, new border is not raised, because there are no labels,
 *                 or current label already contains a border.
 */
bool
jsp_label_raise_nested_jumpable_border (void)
{
  bool is_any_label = (label_set_p != NULL);

  if (is_any_label)
  {
    if (!label_set_p->is_nested_jumpable_border)
    {
      label_set_p->is_nested_jumpable_border = true;

      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    return false;
  }
} /* jsp_label_raise_nested_jumpable_border */

/**
 * Remove nested jumpable border from current label
 */
void
jsp_label_remove_nested_jumpable_border (void)
{
  JERRY_ASSERT (label_set_p != NULL && label_set_p->is_nested_jumpable_border);

  label_set_p->is_nested_jumpable_border = false;
} /* jsp_label_remove_nested_jumpable_border */

/**
 * Mask current label set to restore it later, and start new label set
 *
 * @return pointer to masked label set's list of labels
 */
jsp_label_t*
jsp_label_mask_set (void)
{
  jsp_label_t *ret_p = label_set_p;

  label_set_p = NULL;

  return ret_p;
} /* jsp_label_mask_set */

/**
 * Restore previously masked label set
 *
 * Note:
 *      current label set should be empty
 */
void
jsp_label_restore_set (jsp_label_t *masked_label_set_list_p) /**< list of labels of
                                                              *   a masked label set */
{
  JERRY_ASSERT (label_set_p == NULL);

  label_set_p = masked_label_set_list_p;
} /* jsp_label_restore_set */

/**
 * @}
 * @}
 */
