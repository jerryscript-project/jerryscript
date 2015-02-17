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

#ifndef ECMA_STACK_H
#define ECMA_STACK_H

#include "config.h"
#include "ecma-globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmastack ECMA stack
 * @{
 */

/**
 * Number of ecma-values inlined into stack frame
 */
#define ECMA_STACK_FRAME_INLINED_VALUES_NUMBER CONFIG_ECMA_STACK_FRAME_INLINED_VALUES_NUMBER

/**
 * Header of a ECMA stack frame's chunk
 */
typedef struct
{
  uint16_t prev_chunk_p; /**< previous chunk of same frame */
} ecma_stack_chunk_header_t;

/**
 * ECMA stack frame
 */
typedef struct ecma_stack_frame_t
{
  struct ecma_stack_frame_t *prev_frame_p; /**< previous frame */
  ecma_stack_chunk_header_t *top_chunk_p; /**< the top-most chunk of the frame */
  ecma_value_t *dynamically_allocated_value_slots_p; /**< pointer to dynamically allocated value slots
                                                      *   in the top-most chunk */
  uint32_t current_slot_index; /**< index of first free slot in the top chunk */
  ecma_value_t inlined_values [ECMA_STACK_FRAME_INLINED_VALUES_NUMBER]; /**< place for values inlined in stack frame
                                                                         *   (instead of being dynamically allocated
                                                                         *   on the heap) */
  ecma_value_t *regs_p; /**< register variables */
  int32_t regs_number; /**< number of register variables */
} ecma_stack_frame_t;

extern void ecma_stack_init (void);
extern void ecma_stack_finalize (void);
extern ecma_stack_frame_t*
ecma_stack_get_top_frame (void);
extern void
ecma_stack_add_frame (ecma_stack_frame_t *frame_p,
                      ecma_value_t *regs_p,
                      int32_t regs_num);
extern void ecma_stack_free_frame (ecma_stack_frame_t *frame_p);
extern ecma_value_t ecma_stack_frame_get_reg_value (ecma_stack_frame_t *frame_p, int32_t reg_index);
extern void ecma_stack_frame_set_reg_value (ecma_stack_frame_t *frame_p, int32_t reg_index, ecma_value_t value);
extern void ecma_stack_push_value (ecma_stack_frame_t *frame_p, ecma_value_t value);
extern ecma_value_t ecma_stack_top_value (ecma_stack_frame_t *frame_p);
extern void ecma_stack_pop (ecma_stack_frame_t *frame_p);
extern void ecma_stack_pop_multiple (ecma_stack_frame_t *frame_p, uint32_t number);

/**
 * @}
 * @}
 */

#endif /* !ECMA_STACK_H */
