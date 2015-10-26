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

#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "vm-stack.h"

/** \addtogroup vm Virtual machine
 * @{
 *
 * \addtogroup stack VM stack
 * @{
 */

/**
 * Size of a stack frame's dynamic chunk
 */
#define VM_STACK_DYNAMIC_CHUNK_SIZE (mem_heap_recommend_allocation_size (sizeof (vm_stack_chunk_header_t) + \
                                                                         sizeof (ecma_value_t)))

/**
 * Number of value slots in a stack frame's dynamic chunk
 */
#define VM_STACK_SLOTS_IN_DYNAMIC_CHUNK ((VM_STACK_DYNAMIC_CHUNK_SIZE - sizeof (vm_stack_chunk_header_t)) / \
                                         sizeof (ecma_value_t))

/**
 * The top-most stack frame
 */
vm_stack_frame_t* vm_stack_top_frame_p;

/**
 * Initialize stack
 */
void
vm_stack_init (void)
{
  vm_stack_top_frame_p = NULL;
} /* vm_stack_init */

/**
 * Finalize stack
 */
void
vm_stack_finalize ()
{
  JERRY_ASSERT (vm_stack_top_frame_p == NULL);
} /* vm_stack_finalize */

/**
 * Get stack's top frame
 *
 * @return pointer to the top frame descriptor
 */
vm_stack_frame_t*
vm_stack_get_top_frame (void)
{
  return vm_stack_top_frame_p;
} /* vm_stack_get_top_frame */

/**
 * Add the frame to stack
 */
void
vm_stack_add_frame (vm_stack_frame_t *frame_p, /**< frame to initialize */
                    ecma_value_t *regs_p, /**< array of register variables' values */
                    uint32_t regs_num, /**< total number of register variables */
                    uint32_t local_vars_regs_num, /**< number of register variables,
                                                   *   used for local variables */
                    uint32_t arg_regs_num, /**< number of register variables,
                                            *   used for arguments */
                    ecma_collection_header_t *arguments_p) /**< collection of arguments
                                                            *   (for case, their values
                                                            *    are moved to registers) */
{
  frame_p->prev_frame_p = vm_stack_top_frame_p;
  vm_stack_top_frame_p = frame_p;

  frame_p->top_chunk_p = NULL;
  frame_p->dynamically_allocated_value_slots_p = frame_p->inlined_values;
  frame_p->current_slot_index = 0;
  frame_p->regs_p = regs_p;
  frame_p->regs_number = regs_num;

  JERRY_ASSERT (regs_num >= VM_SPECIAL_REGS_NUMBER);

  for (uint32_t i = 0; i < regs_num - local_vars_regs_num - arg_regs_num; i++)
  {
    regs_p[i] = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  }

  for (uint32_t i = regs_num - local_vars_regs_num - arg_regs_num;
       i < regs_num;
       i++)
  {
    regs_p[i] = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  }

  if (arg_regs_num != 0)
  {
    ecma_collection_iterator_t args_iterator;
    ecma_collection_iterator_init (&args_iterator, arguments_p);

    for (uint32_t i = regs_num - arg_regs_num;
         i < regs_num && ecma_collection_iterator_next (&args_iterator);
         i++)
    {
      regs_p[i] = ecma_copy_value (*args_iterator.current_value_p, false);
    }
  }
} /* vm_stack_add_frame */

/**
 * Free the stack frame
 *
 * Note:
 *      the frame should be the top-most frame
 */
void
vm_stack_free_frame (vm_stack_frame_t *frame_p) /**< frame to initialize */
{
  /* the frame should be the top-most frame */
  JERRY_ASSERT (vm_stack_top_frame_p == frame_p);

  vm_stack_top_frame_p = frame_p->prev_frame_p;

  while (frame_p->top_chunk_p != NULL)
  {
    vm_stack_pop (frame_p);
  }

  for (uint32_t reg_index = 0;
       reg_index < frame_p->regs_number;
       reg_index++)
  {
    ecma_free_value (frame_p->regs_p[reg_index], false);
  }
} /* vm_stack_free_frame */

/**
 * Get value of specified register variable
 *
 * @return ecma-value
 */
ecma_value_t
vm_stack_frame_get_reg_value (vm_stack_frame_t *frame_p, /**< frame */
                              uint32_t reg_index) /**< index of register variable */
{
  JERRY_ASSERT (reg_index >= VM_REG_FIRST && reg_index < VM_REG_FIRST + frame_p->regs_number);

  return frame_p->regs_p[reg_index - VM_REG_FIRST];
} /* vm_stack_frame_get_reg_value */

/**
 * Set value of specified register variable
 */
void
vm_stack_frame_set_reg_value (vm_stack_frame_t *frame_p, /**< frame */
                              uint32_t reg_index, /**< index of register variable */
                              ecma_value_t value) /**< ecma-value */
{
  JERRY_ASSERT (reg_index >= VM_REG_FIRST && reg_index < VM_REG_FIRST + frame_p->regs_number);

  frame_p->regs_p[reg_index - VM_REG_FIRST] = value;
} /* vm_stack_frame_set_reg_value */

/**
 * Calculate number of value slots in the top-most chunk of the frame
 *
 * @return number of value slots
 */
static size_t
vm_stack_slots_in_top_chunk (vm_stack_frame_t *frame_p) /**< stack frame */
{
  return ((frame_p->top_chunk_p == NULL) ? VM_STACK_FRAME_INLINED_VALUES_NUMBER : VM_STACK_SLOTS_IN_DYNAMIC_CHUNK);
} /* vm_stack_slots_in_top_chunk */

/**
 * Longpath for vm_stack_push_value (for case current chunk may be doesn't have free slots)
 */
static void __attr_noinline___
vm_stack_push_value_longpath (vm_stack_frame_t *frame_p) /**< stack frame */
{
  JERRY_ASSERT (frame_p->current_slot_index >= JERRY_MIN (VM_STACK_FRAME_INLINED_VALUES_NUMBER,
                                                          VM_STACK_SLOTS_IN_DYNAMIC_CHUNK));

  const size_t slots_in_top_chunk = vm_stack_slots_in_top_chunk (frame_p);

  if (frame_p->current_slot_index == slots_in_top_chunk)
  {
    vm_stack_chunk_header_t *chunk_p;
    chunk_p = (vm_stack_chunk_header_t *) mem_heap_alloc_block (VM_STACK_DYNAMIC_CHUNK_SIZE,
                                                                MEM_HEAP_ALLOC_SHORT_TERM);

    ECMA_SET_POINTER (chunk_p->prev_chunk_p, frame_p->top_chunk_p);

    frame_p->top_chunk_p = chunk_p;
    frame_p->dynamically_allocated_value_slots_p = (ecma_value_t*) (frame_p->top_chunk_p + 1);
    frame_p->current_slot_index = 0;
  }
} /* vm_stack_push_value_longpath */

/**
 * Push ecma-value to stack
 */
void
vm_stack_push_value (vm_stack_frame_t *frame_p, /**< stack frame */
                     ecma_value_t value) /**< ecma-value */
{
  frame_p->current_slot_index++;

  if (frame_p->current_slot_index >= JERRY_MIN (VM_STACK_FRAME_INLINED_VALUES_NUMBER,
                                                VM_STACK_SLOTS_IN_DYNAMIC_CHUNK))
  {
    vm_stack_push_value_longpath (frame_p);
  }

  JERRY_ASSERT (frame_p->current_slot_index < vm_stack_slots_in_top_chunk (frame_p));

  frame_p->dynamically_allocated_value_slots_p[frame_p->current_slot_index] = value;
} /* vm_stack_push_value */

/**
 * Get top value from stack
 */
ecma_value_t __attr_always_inline___
vm_stack_top_value (vm_stack_frame_t *frame_p) /**< stack frame */
{
  const size_t slots_in_top_chunk = vm_stack_slots_in_top_chunk (frame_p);

  JERRY_ASSERT (frame_p->current_slot_index < slots_in_top_chunk);

  return frame_p->dynamically_allocated_value_slots_p[frame_p->current_slot_index];
} /* vm_stack_top_value */

/**
 * Longpath for vm_stack_pop (for case a dynamically allocated chunk needs to be deallocated)
 */
static void __attr_noinline___
vm_stack_pop_longpath (vm_stack_frame_t *frame_p) /**< stack frame */
{
  JERRY_ASSERT (frame_p->current_slot_index == 0 && frame_p->top_chunk_p != NULL);

  vm_stack_chunk_header_t *chunk_to_free_p = frame_p->top_chunk_p;
  frame_p->top_chunk_p = ECMA_GET_POINTER (vm_stack_chunk_header_t,
                                           frame_p->top_chunk_p->prev_chunk_p);

  if (frame_p->top_chunk_p != NULL)
  {
    frame_p->dynamically_allocated_value_slots_p = (ecma_value_t*) (frame_p->top_chunk_p + 1);
    frame_p->current_slot_index = (uint32_t) (VM_STACK_SLOTS_IN_DYNAMIC_CHUNK - 1u);
  }
  else
  {
    frame_p->dynamically_allocated_value_slots_p = frame_p->inlined_values;
    frame_p->current_slot_index = (uint32_t) (VM_STACK_FRAME_INLINED_VALUES_NUMBER - 1u);
  }

  mem_heap_free_block (chunk_to_free_p);
} /* vm_stack_pop_longpath */

/**
 * Pop top value from stack and free it
 */
void
vm_stack_pop (vm_stack_frame_t *frame_p) /**< stack frame */
{
  JERRY_ASSERT (frame_p->current_slot_index < vm_stack_slots_in_top_chunk (frame_p));

  ecma_value_t value = vm_stack_top_value (frame_p);

  if (unlikely (frame_p->current_slot_index == 0
                && frame_p->top_chunk_p != NULL))
  {
    vm_stack_pop_longpath (frame_p);
  }
  else
  {
    frame_p->current_slot_index--;
  }

  ecma_free_value (value, true);
} /* vm_stack_pop */

/**
 * Pop multiple top values from stack and free them
 */
void
vm_stack_pop_multiple (vm_stack_frame_t *frame_p, /**< stack frame */
                       uint32_t number) /**< number of elements to pop */
{
  for (uint32_t i = 0; i < number; i++)
  {
    vm_stack_pop (frame_p);
  }
} /* vm_stack_pop_multiple */

/**
 * @}
 * @}
 */
