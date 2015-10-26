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

#ifndef VM_STACK_H
#define VM_STACK_H

#include "config.h"
#include "ecma-globals.h"

/** \addtogroup vm Virtual machine
 * @{
 *
 * \addtogroup stack VM stack
 * @{
 */

/**
 * Number of ecma-values inlined into stack frame
 */
#define VM_STACK_FRAME_INLINED_VALUES_NUMBER CONFIG_VM_STACK_FRAME_INLINED_VALUES_NUMBER

/**
 * Header of a ECMA stack frame's chunk
 */
typedef struct
{
  uint16_t prev_chunk_p; /**< previous chunk of same frame */
} vm_stack_chunk_header_t;

/**
 * ECMA stack frame
 */
typedef struct vm_stack_frame_t
{
  struct vm_stack_frame_t *prev_frame_p; /**< previous frame */
  vm_stack_chunk_header_t *top_chunk_p; /**< the top-most chunk of the frame */
  ecma_value_t *dynamically_allocated_value_slots_p; /**< pointer to dynamically allocated value slots
                                                      *   in the top-most chunk */
  uint32_t current_slot_index; /**< index of first free slot in the top chunk */
  ecma_value_t inlined_values[VM_STACK_FRAME_INLINED_VALUES_NUMBER]; /**< place for values inlined into stack frame
                                                                      *   (instead of being placed on heap) */
  ecma_value_t *regs_p; /**< register variables */
  uint32_t regs_number; /**< number of register variables */
} vm_stack_frame_t;

extern void vm_stack_init (void);
extern void vm_stack_finalize (void);
extern vm_stack_frame_t *
vm_stack_get_top_frame (void);
extern void
vm_stack_add_frame (vm_stack_frame_t *, ecma_value_t *, uint32_t, uint32_t, uint32_t, ecma_collection_header_t *);
extern void vm_stack_free_frame (vm_stack_frame_t *);
extern ecma_value_t vm_stack_frame_get_reg_value (vm_stack_frame_t *, uint32_t);
extern void vm_stack_frame_set_reg_value (vm_stack_frame_t *, uint32_t, ecma_value_t);
extern void vm_stack_push_value (vm_stack_frame_t *, ecma_value_t);
extern ecma_value_t vm_stack_top_value (vm_stack_frame_t *);
extern void vm_stack_pop (vm_stack_frame_t *);
extern void vm_stack_pop_multiple (vm_stack_frame_t *, uint32_t);

/**
 * @}
 * @}
 */

#endif /* !VM_STACK_H */
