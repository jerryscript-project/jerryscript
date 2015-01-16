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

#include "ecma-helpers.h"
#include "ecma-stack.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmastack ecma-stack
 * @{
 */

/**
 * Size of a ecma-stack frame's dynamic chunk
 */
#define ECMA_STACK_DYNAMIC_CHUNK_SIZE (mem_heap_recommend_allocation_size (sizeof (ecma_stack_chunk_header_t) + \
                                                                           sizeof (ecma_value_t)))

/**
 * Number of value slots in a ecma-stack frame's dynamic chunk
 */
#define ECMA_STACK_SLOTS_IN_DYNAMIC_CHUNK ((ECMA_STACK_DYNAMIC_CHUNK_SIZE - sizeof (ecma_stack_chunk_header_t)) / \
                                           sizeof (ecma_value_t))

/**
 * The top-most ecma-stack frame
 */
ecma_stack_frame_t* ecma_stack_top_frame_p;

/**
 * Initialize ecma-stack
 */
void
ecma_stack_init (void)
{
  ecma_stack_top_frame_p = NULL;
} /* ecma_stack_init */

/**
 * Finalize ecma-stack
 */
void
ecma_stack_finalize ()
{
  JERRY_ASSERT (ecma_stack_top_frame_p == NULL);
} /* ecma_stack_finalize */

/**
 * Get ecma-stack's top frame
 *
 * @return pointer to the top frame descriptor
 */
ecma_stack_frame_t*
ecma_stack_get_top_frame (void)
{
  return ecma_stack_top_frame_p;
} /* ecma_stack_get_top_frame */

/**
 * Add the frame to ecma-stack
 */
void
ecma_stack_add_frame (ecma_stack_frame_t *frame_p, /**< frame to initialize */
                      ecma_value_t *regs_p, /**< array of register variables' values */
                      int32_t regs_num) /**< number of register variables */
{
  frame_p->prev_frame_p = ecma_stack_top_frame_p;
  ecma_stack_top_frame_p = frame_p;

  frame_p->top_chunk_p = NULL;
  frame_p->dynamically_allocated_value_slots_p = frame_p->inlined_values;
  frame_p->current_slot_index = 0;
  frame_p->regs_p = regs_p;
  frame_p->regs_number = regs_num;

  for (int32_t i = 0; i < regs_num; i++)
  {
    regs_p [i] = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  }
} /* ecma_stack_add_frame */

/**
 * Free the ecma-stack frame
 *
 * Note:
 *      the frame should be the top-most frame
 */
void
ecma_stack_free_frame (ecma_stack_frame_t *frame_p) /**< frame to initialize */
{
  /* the frame should be the top-most frame */
  JERRY_ASSERT (ecma_stack_top_frame_p == frame_p);

  ecma_stack_top_frame_p = frame_p->prev_frame_p;

  while (frame_p->top_chunk_p != NULL)
  {
    ecma_stack_pop (frame_p);
  }

  for (int32_t reg_index = 0;
       reg_index < frame_p->regs_number;
       reg_index++)
  {
    ecma_free_value (frame_p->regs_p [reg_index], false);
  }
} /* ecma_stack_free_frame */

/**
 * Get value of specified register variable
 *
 * @return ecma-value
 */
ecma_value_t
ecma_stack_frame_get_reg_value (ecma_stack_frame_t *frame_p, /**< frame */
                                int32_t reg_index) /**< index of register variable */
{
  JERRY_ASSERT (reg_index >= 0 && reg_index < frame_p->regs_number);

  return frame_p->regs_p [reg_index];
} /* ecma_stack_frame_get_reg_value */

/**
 * Set value of specified register variable
 */
void
ecma_stack_frame_set_reg_value (ecma_stack_frame_t *frame_p, /**< frame */
                                int32_t reg_index, /**< index of register variable */
                                ecma_value_t value) /**< ecma-value */
{
  JERRY_ASSERT (reg_index >= 0 && reg_index < frame_p->regs_number);

  frame_p->regs_p [reg_index] = value;
} /* ecma_stack_frame_set_reg_value */

/**
 * Calculate number of value slots in the top-most chunk of the frame
 *
 * @return number of value slots
 */
static size_t
ecma_stack_slots_in_top_chunk (ecma_stack_frame_t *frame_p) /**< ecma-stack frame */
{
  return ((frame_p->top_chunk_p == NULL) ? ECMA_STACK_FRAME_INLINED_VALUES_NUMBER : ECMA_STACK_SLOTS_IN_DYNAMIC_CHUNK);
} /* ecma_stack_slots_in_top_chunk */

/**
 * Longpath for ecma_stack_push_value (for case current chunk may be doesn't have free slots)
 */
static void __noinline
ecma_stack_push_value_longpath (ecma_stack_frame_t *frame_p) /**< ecma-stack frame */
{
  JERRY_ASSERT (frame_p->current_slot_index >= JERRY_MIN (ECMA_STACK_FRAME_INLINED_VALUES_NUMBER,
                                                          ECMA_STACK_SLOTS_IN_DYNAMIC_CHUNK));

  const size_t slots_in_top_chunk = ecma_stack_slots_in_top_chunk (frame_p);

  if (frame_p->current_slot_index == slots_in_top_chunk)
  {
    ecma_stack_chunk_header_t *chunk_p;
    chunk_p = (ecma_stack_chunk_header_t *) mem_heap_alloc_block (ECMA_STACK_DYNAMIC_CHUNK_SIZE,
                                                                  MEM_HEAP_ALLOC_SHORT_TERM);

    ECMA_SET_POINTER (chunk_p->prev_chunk_p, frame_p->top_chunk_p);

    frame_p->top_chunk_p = chunk_p;
    frame_p->dynamically_allocated_value_slots_p = (ecma_value_t*) (frame_p->top_chunk_p + 1);
    frame_p->current_slot_index = 0;
  }
} /* ecma_stack_push_value_longpath */

/**
 * Push ecma-value to ecma-stack
 */
void
ecma_stack_push_value (ecma_stack_frame_t *frame_p, /**< ecma-stack frame */
                       ecma_value_t value) /**< ecma-value */
{
  frame_p->current_slot_index++;

  if (frame_p->current_slot_index >= JERRY_MIN (ECMA_STACK_FRAME_INLINED_VALUES_NUMBER,
                                                ECMA_STACK_SLOTS_IN_DYNAMIC_CHUNK))
  {
    ecma_stack_push_value_longpath (frame_p);
  }

  JERRY_ASSERT (frame_p->current_slot_index < ecma_stack_slots_in_top_chunk (frame_p));

  frame_p->dynamically_allocated_value_slots_p [frame_p->current_slot_index] = value;
} /* ecma_stack_push_value */

/**
 * Get top value from ecma-stack
 */
inline ecma_value_t __attribute_always_inline__
ecma_stack_top_value (ecma_stack_frame_t *frame_p) /**< ecma-stack frame */
{
  const size_t slots_in_top_chunk = ecma_stack_slots_in_top_chunk (frame_p);

  JERRY_ASSERT (frame_p->current_slot_index < slots_in_top_chunk);

  return frame_p->dynamically_allocated_value_slots_p [frame_p->current_slot_index];
} /* ecma_stack_top_value */

/**
 * Longpath for ecma_stack_pop (for case a dynamically allocated chunk needs to be deallocated)
 */
static void __noinline
ecma_stack_pop_longpath (ecma_stack_frame_t *frame_p) /**< ecma-stack frame */
{
  JERRY_ASSERT (frame_p->current_slot_index == 0 && frame_p->top_chunk_p != NULL);

  ecma_stack_chunk_header_t *chunk_to_free_p = frame_p->top_chunk_p;
  frame_p->top_chunk_p = ECMA_GET_POINTER (ecma_stack_chunk_header_t,
                                           frame_p->top_chunk_p->prev_chunk_p);

  if (frame_p->top_chunk_p != NULL)
  {
    frame_p->dynamically_allocated_value_slots_p = (ecma_value_t*) (frame_p->top_chunk_p + 1);
    frame_p->current_slot_index = (uint32_t) (ECMA_STACK_SLOTS_IN_DYNAMIC_CHUNK - 1u);
  }
  else
  {
    frame_p->dynamically_allocated_value_slots_p = frame_p->inlined_values;
    frame_p->current_slot_index = (uint32_t) (ECMA_STACK_FRAME_INLINED_VALUES_NUMBER - 1u);
  }

  mem_heap_free_block (chunk_to_free_p);
} /* ecma_stack_pop_longpath */

/**
 * Pop top value from ecma-stack and free it
 */
void
ecma_stack_pop (ecma_stack_frame_t *frame_p) /**< ecma-stack frame */
{
  JERRY_ASSERT (frame_p->current_slot_index < ecma_stack_slots_in_top_chunk (frame_p));

  ecma_value_t value = ecma_stack_top_value (frame_p);

  if (unlikely (frame_p->current_slot_index == 0
                && frame_p->top_chunk_p != NULL))
  {
    ecma_stack_pop_longpath (frame_p);
  }
  else
  {
    frame_p->current_slot_index--;
  }

  ecma_free_value (value, true);
} /* ecma_stack_pop */

/**
 * Pop multiple top values from ecma-stack and free them
 */
void
ecma_stack_pop_multiple (ecma_stack_frame_t *frame_p, /**< ecma-stack frame */
                         uint32_t number) /**< number of elements to pop */
{
  for (uint32_t i = 0; i < number; i++)
  {
    ecma_stack_pop (frame_p);
  }
} /* ecma_stack_pop_multiple */

/**
 * @}
 * @}
 */
