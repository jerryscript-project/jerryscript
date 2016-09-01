/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2015-2016 University of Szeged.
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

#include "ecma-alloc.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "vm-defines.h"
#include "vm-stack.h"

/** \addtogroup vm Virtual machine
 * @{
 *
 * \addtogroup stack VM stack
 * @{
 */

/**
 * Abort (finalize) the current stack context, and remove it.
 *
 * @return new stack top
 */
ecma_value_t *
vm_stack_context_abort (vm_frame_ctx_t *frame_ctx_p, /**< frame context */
                        ecma_value_t *vm_stack_top_p) /**< current stack top */
{
  switch (VM_GET_CONTEXT_TYPE (vm_stack_top_p[-1]))
  {
    case VM_CONTEXT_FINALLY_THROW:
    case VM_CONTEXT_FINALLY_RETURN:
    {
      ecma_free_value (vm_stack_top_p[-2]);

      VM_MINUS_EQUAL_U16 (frame_ctx_p->context_depth, PARSER_TRY_CONTEXT_STACK_ALLOCATION);
      vm_stack_top_p -= PARSER_TRY_CONTEXT_STACK_ALLOCATION;
      break;
    }
    case VM_CONTEXT_FINALLY_JUMP:
    case VM_CONTEXT_TRY:
    {
      VM_MINUS_EQUAL_U16 (frame_ctx_p->context_depth, PARSER_TRY_CONTEXT_STACK_ALLOCATION);
      vm_stack_top_p -= PARSER_TRY_CONTEXT_STACK_ALLOCATION;
      break;
    }
    case VM_CONTEXT_CATCH:
    case VM_CONTEXT_WITH:
    {
      ecma_deref_object (frame_ctx_p->lex_env_p);
      frame_ctx_p->lex_env_p = ecma_get_object_from_value (vm_stack_top_p[-2]);

      JERRY_ASSERT (PARSER_TRY_CONTEXT_STACK_ALLOCATION == PARSER_WITH_CONTEXT_STACK_ALLOCATION);

      VM_MINUS_EQUAL_U16 (frame_ctx_p->context_depth, PARSER_TRY_CONTEXT_STACK_ALLOCATION);
      vm_stack_top_p -= PARSER_TRY_CONTEXT_STACK_ALLOCATION;
      break;
    }
    case VM_CONTEXT_FOR_IN:
    {
      jmem_cpointer_t current = (jmem_cpointer_t) vm_stack_top_p[-2];

      while (current != JMEM_CP_NULL)
      {
        ecma_collection_chunk_t *chunk_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_collection_chunk_t,
                                                                         current);

        lit_utf8_byte_t *data_ptr = chunk_p->data;
        ecma_free_value (*(ecma_value_t *) data_ptr);

        current = chunk_p->next_chunk_cp;
        ecma_dealloc_collection_chunk (chunk_p);
      }

      ecma_free_value (vm_stack_top_p[-3]);

      VM_MINUS_EQUAL_U16 (frame_ctx_p->context_depth, PARSER_FOR_IN_CONTEXT_STACK_ALLOCATION);
      vm_stack_top_p -= PARSER_FOR_IN_CONTEXT_STACK_ALLOCATION;
      break;
    }
    default:
    {
      JERRY_UNREACHABLE ();
      break;
    }
  }

  return vm_stack_top_p;
} /* vm_stack_context_abort */

/**
 * Decode branch offset.
 *
 * @return branch offset
 */
static uint32_t
vm_decode_branch_offset (uint8_t *branch_offset_p, /**< start offset of byte code */
                         uint32_t length) /**< length of the branch */
{
  uint32_t branch_offset = *branch_offset_p;

  JERRY_ASSERT (length >= 1 && length <= 3);

  switch (length)
  {
    case 3:
    {
      branch_offset <<= 8;
      branch_offset |= *(branch_offset_p++);
      /* FALLTHRU */
    }
    case 2:
    {
      branch_offset <<= 8;
      branch_offset |= *(branch_offset_p++);
      break;
    }
  }

  return branch_offset;
} /* vm_decode_branch_offset */

/**
 * Find a finally up to the end position.
 *
 * @return true if 'finally' found,
 *         false otherwise
 */
bool
vm_stack_find_finally (vm_frame_ctx_t *frame_ctx_p, /**< frame context */
                       ecma_value_t **vm_stack_top_ref_p, /**< current stack top */
                       vm_stack_context_type_t finally_type, /**< searching this finally */
                       uint32_t search_limit) /**< search up-to this byte code */
{
  ecma_value_t *vm_stack_top_p = *vm_stack_top_ref_p;

  JERRY_ASSERT (finally_type <= VM_CONTEXT_FINALLY_RETURN);

  if (finally_type != VM_CONTEXT_FINALLY_JUMP)
  {
    search_limit = 0xffffffffu;
  }

  while (frame_ctx_p->context_depth > 0)
  {
    vm_stack_context_type_t context_type;
    uint32_t context_end = VM_GET_CONTEXT_END (vm_stack_top_p[-1]);

    if (search_limit < context_end)
    {
      *vm_stack_top_ref_p = vm_stack_top_p;
      return false;
    }

    context_type = VM_GET_CONTEXT_TYPE (vm_stack_top_p[-1]);
    if (context_type == VM_CONTEXT_TRY || context_type == VM_CONTEXT_CATCH)
    {
      uint8_t *byte_code_p;
      uint32_t branch_offset_length;
      uint32_t branch_offset;

      if (search_limit == context_end)
      {
        *vm_stack_top_ref_p = vm_stack_top_p;
        return false;
      }

      byte_code_p = frame_ctx_p->byte_code_start_p + VM_GET_CONTEXT_END (vm_stack_top_p[-1]);

      if (context_type == VM_CONTEXT_TRY)
      {
        JERRY_ASSERT (byte_code_p[0] == CBC_EXT_OPCODE);

        if (byte_code_p[1] >= CBC_EXT_CATCH
            && byte_code_p[1] <= CBC_EXT_CATCH_3)
        {
          branch_offset_length = CBC_BRANCH_OFFSET_LENGTH (byte_code_p[1]);
          branch_offset = vm_decode_branch_offset (byte_code_p + 2,
                                                   branch_offset_length);

          if (finally_type == VM_CONTEXT_FINALLY_THROW)
          {
            branch_offset += (uint32_t) (byte_code_p - frame_ctx_p->byte_code_start_p);

            vm_stack_top_p[-1] = VM_CREATE_CONTEXT (VM_CONTEXT_CATCH, branch_offset);

            byte_code_p += 2 + branch_offset_length;
            frame_ctx_p->byte_code_p = byte_code_p;

            *vm_stack_top_ref_p = vm_stack_top_p;
            return true;
          }

          byte_code_p += branch_offset;

          if (*byte_code_p == CBC_CONTEXT_END)
          {
            VM_MINUS_EQUAL_U16 (frame_ctx_p->context_depth, PARSER_TRY_CONTEXT_STACK_ALLOCATION);
            vm_stack_top_p -= PARSER_TRY_CONTEXT_STACK_ALLOCATION;
            continue;
          }
        }
      }
      else
      {
        ecma_deref_object (frame_ctx_p->lex_env_p);
        frame_ctx_p->lex_env_p = ecma_get_object_from_value (vm_stack_top_p[-2]);

        if (byte_code_p[0] == CBC_CONTEXT_END)
        {
          VM_MINUS_EQUAL_U16 (frame_ctx_p->context_depth, PARSER_TRY_CONTEXT_STACK_ALLOCATION);
          vm_stack_top_p -= PARSER_TRY_CONTEXT_STACK_ALLOCATION;
          continue;
        }
      }

      JERRY_ASSERT (byte_code_p[0] == CBC_EXT_OPCODE);
      JERRY_ASSERT (byte_code_p[1] >= CBC_EXT_FINALLY
                    && byte_code_p[1] <= CBC_EXT_FINALLY_3);

      branch_offset_length = CBC_BRANCH_OFFSET_LENGTH (byte_code_p[1]);
      branch_offset = vm_decode_branch_offset (byte_code_p + 2,
                                               branch_offset_length);

      branch_offset += (uint32_t) (byte_code_p - frame_ctx_p->byte_code_start_p);

      vm_stack_top_p[-1] = VM_CREATE_CONTEXT ((uint32_t) finally_type, branch_offset);

      byte_code_p += 2 + branch_offset_length;
      frame_ctx_p->byte_code_p = byte_code_p;

      *vm_stack_top_ref_p = vm_stack_top_p;
      return true;
    }

    vm_stack_top_p = vm_stack_context_abort (frame_ctx_p, vm_stack_top_p);
  }

  *vm_stack_top_ref_p = vm_stack_top_p;
  return false;
} /* vm_stack_find_finally */

/**
 * @}
 * @}
 */
