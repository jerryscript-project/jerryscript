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

#include "ecma-alloc.h"
#include "ecma-async-generator-object.h"
#include "ecma-builtins.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-iterator-object.h"
#include "ecma-promise-object.h"
#include "jcontext.h"
#include "opcodes.h"
#include "vm.h"

#if ENABLED (JERRY_ESNEXT)

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaasyncgeneratorobject ECMA AsyncGenerator object related routines
 * @{
 */

/**
 * Enqueue a task into the command queue of an async generator
 *
 * @return ecma Promise value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_async_generator_enqueue (vm_executable_object_t *async_generator_object_p, /**< async generator */
                              ecma_async_generator_operation_type_t operation, /**< operation */
                              ecma_value_t value) /**< value argument of operation */
{
  ecma_async_generator_task_t *task_p = jmem_heap_alloc_block (sizeof (ecma_async_generator_task_t));

  ECMA_SET_INTERNAL_VALUE_ANY_POINTER (task_p->next, NULL);
  task_p->operation_value = ecma_copy_value_if_not_object (value);
  task_p->operation_type = (uint8_t) operation;

  ecma_object_t *old_new_target_p = JERRY_CONTEXT (current_new_target);
  JERRY_CONTEXT (current_new_target) = ecma_builtin_get (ECMA_BUILTIN_ID_PROMISE);
  ecma_value_t result = ecma_op_create_promise_object (ECMA_VALUE_EMPTY, ECMA_PROMISE_EXECUTOR_EMPTY);
  JERRY_CONTEXT (current_new_target) = old_new_target_p;
  task_p->promise = result;

  ecma_value_t head = async_generator_object_p->extended_object.u.class_prop.u.head;

  if (ECMA_IS_INTERNAL_VALUE_NULL (head))
  {
    ECMA_SET_INTERNAL_VALUE_POINTER (async_generator_object_p->extended_object.u.class_prop.u.head, task_p);

    if (async_generator_object_p->extended_object.u.class_prop.extra_info & ECMA_ASYNC_GENERATOR_CALLED)
    {
      ecma_value_t executable_object = ecma_make_object_value ((ecma_object_t *) async_generator_object_p);
      ecma_enqueue_promise_async_generator_job (executable_object);
      return result;
    }

    async_generator_object_p->extended_object.u.class_prop.extra_info |= ECMA_ASYNC_GENERATOR_CALLED;
    ecma_async_generator_run (async_generator_object_p);
    return result;
  }

  /* Append the new task at the end. */
  ecma_async_generator_task_t *prev_task_p;
  prev_task_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_async_generator_task_t, head);

  while (!ECMA_IS_INTERNAL_VALUE_NULL (prev_task_p->next))
  {
    prev_task_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_async_generator_task_t, prev_task_p->next);
  }

  ECMA_SET_INTERNAL_VALUE_POINTER (prev_task_p->next, task_p);
  return result;
} /* ecma_async_generator_enqueue */

/**
 * Execute the next task in the command queue of the async generator
 */
void
ecma_async_generator_run (vm_executable_object_t *async_generator_object_p) /**< async generator */
{
  JERRY_ASSERT (async_generator_object_p->extended_object.u.class_prop.class_id
                == LIT_MAGIC_STRING_ASYNC_GENERATOR_UL);
  JERRY_ASSERT (!ECMA_IS_INTERNAL_VALUE_NULL (async_generator_object_p->extended_object.u.class_prop.u.head));

  ecma_value_t head = async_generator_object_p->extended_object.u.class_prop.u.head;
  ecma_async_generator_task_t *task_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_async_generator_task_t, head);

  if (task_p->operation_type == ECMA_ASYNC_GENERATOR_DO_RETURN)
  {
    async_generator_object_p->frame_ctx.byte_code_p = opfunc_resume_executable_object_with_return;
  }
  else if (task_p->operation_type == ECMA_ASYNC_GENERATOR_DO_THROW)
  {
    async_generator_object_p->frame_ctx.byte_code_p = opfunc_resume_executable_object_with_throw;
  }

  ecma_value_t value = task_p->operation_value;
  ecma_ref_if_object (value);
  task_p->operation_value = ECMA_VALUE_UNDEFINED;

  value = opfunc_resume_executable_object (async_generator_object_p, value);

  if (async_generator_object_p->extended_object.u.class_prop.extra_info & ECMA_EXECUTABLE_OBJECT_COMPLETED)
  {
    JERRY_ASSERT (head == async_generator_object_p->extended_object.u.class_prop.u.head);
    ecma_async_generator_finalize (async_generator_object_p, value);
  }
} /* ecma_async_generator_run */

/**
 * Finalize the promises of an executable generator
 */
void
ecma_async_generator_finalize (vm_executable_object_t *async_generator_object_p, /**< async generator */
                               ecma_value_t value) /**< final value (takes reference) */
{
  ecma_value_t next = async_generator_object_p->extended_object.u.class_prop.u.head;
  ecma_async_generator_task_t *task_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_async_generator_task_t, next);

  ECMA_SET_INTERNAL_VALUE_ANY_POINTER (async_generator_object_p->extended_object.u.class_prop.u.head, NULL);

  if (ECMA_IS_VALUE_ERROR (value))
  {
    value = jcontext_take_exception ();
    ecma_reject_promise (task_p->promise, value);
  }
  else
  {
    ecma_value_t result = ecma_create_iter_result_object (value, ECMA_VALUE_TRUE);
    ecma_fulfill_promise (task_p->promise, result);
    ecma_free_value (result);
  }

  ecma_free_value (value);

  next = task_p->next;
  jmem_heap_free_block (task_p, sizeof (ecma_async_generator_task_t));

  while (!ECMA_IS_INTERNAL_VALUE_NULL (next))
  {
    task_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_async_generator_task_t, next);

    if (task_p->operation_type != ECMA_ASYNC_GENERATOR_DO_THROW)
    {
      value = ecma_create_iter_result_object (ECMA_VALUE_UNDEFINED, ECMA_VALUE_TRUE);
      ecma_fulfill_promise (task_p->promise, value);
      ecma_free_value (value);
    }
    else
    {
      ecma_reject_promise (task_p->promise, task_p->operation_value);
    }

    ecma_free_value_if_not_object (task_p->operation_value);

    next = task_p->next;
    jmem_heap_free_block (task_p, sizeof (ecma_async_generator_task_t));
  }
} /* ecma_async_generator_finalize */

#endif /* ENABLED (JERRY_ESNEXT) */

/**
 * @}
 * @}
 */
