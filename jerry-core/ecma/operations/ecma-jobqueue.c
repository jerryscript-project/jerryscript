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

#include "ecma-function-object.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-jobqueue.h"
#include "ecma-objects.h"
#include "ecma-promise-object.h"
#include "jcontext.h"

#if ENABLED (JERRY_ES2015_BUILTIN_PROMISE)

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmajobqueue ECMA Job Queue related routines
 * @{
 */

/**
 * Description of the PromiseReactionJob
 */
typedef struct
{
  ecma_value_t reaction; /**< the PromiseReaction */
  ecma_value_t argument; /**< argument for the reaction */
} ecma_job_promise_reaction_t;

/**
 * Description of the PromiseResolveThenableJob
 */
typedef struct
{
  ecma_value_t promise; /**< promise to be resolved */
  ecma_value_t thenable; /**< thenbale object */
  ecma_value_t then; /**< 'then' function */
} ecma_job_promise_resolve_thenable_t;

/**
 * Initialize the jobqueue.
 */
void ecma_job_queue_init (void)
{
  JERRY_CONTEXT (job_queue_head_p) = NULL;
  JERRY_CONTEXT (job_queue_tail_p) = NULL;
} /* ecma_job_queue_init */

/**
 * Create a PromiseReactionJob.
 *
 * @return pointer to the PromiseReactionJob
 */
static ecma_job_promise_reaction_t *
ecma_create_promise_reaction_job (ecma_value_t reaction, /**< PromiseReaction */
                                  ecma_value_t argument) /**< argument for the reaction */
{
  JERRY_ASSERT (ecma_is_value_object (reaction));

  ecma_job_promise_reaction_t *job_p;
  job_p = (ecma_job_promise_reaction_t *) jmem_heap_alloc_block (sizeof (ecma_job_promise_reaction_t));
  job_p->reaction = ecma_copy_value (reaction);
  job_p->argument = ecma_copy_value (argument);

  return job_p;
} /* ecma_create_promise_reaction_job */

/**
 * Free the heap and the member of the PromiseReactionJob.
 */
static void
ecma_free_promise_reaction_job (ecma_job_promise_reaction_t *job_p) /**< points to the PromiseReactionJob */
{
  JERRY_ASSERT (job_p != NULL);

  ecma_free_value (job_p->reaction);
  ecma_free_value (job_p->argument);

  jmem_heap_free_block (job_p, sizeof (ecma_job_promise_reaction_t));
} /* ecma_free_promise_reaction_job */

/**
 * Create a PromiseResolveThenableJob
 *
 * @return pointer to the PromiseResolveThenableJob
 */
static ecma_job_promise_resolve_thenable_t *
ecma_create_promise_resolve_thenable_job (ecma_value_t promise, /**< promise to be resolved */
                                          ecma_value_t thenable, /**< thenable object */
                                          ecma_value_t then) /**< 'then' function */
{
  JERRY_ASSERT (ecma_is_promise (ecma_get_object_from_value (promise)));
  JERRY_ASSERT (ecma_is_value_object (thenable));
  JERRY_ASSERT (ecma_op_is_callable (then));

  ecma_job_promise_resolve_thenable_t *job_p;
  job_p = (ecma_job_promise_resolve_thenable_t *) jmem_heap_alloc_block (sizeof (ecma_job_promise_resolve_thenable_t));

  job_p->promise = ecma_copy_value (promise);
  job_p->thenable = ecma_copy_value (thenable);
  job_p->then = ecma_copy_value (then);

  return job_p;
} /* ecma_create_promise_resolve_thenable_job */

/**
 * Free the heap and the member of the PromiseResolveThenableJob.
 */
static void
ecma_free_promise_resolve_thenable_job (ecma_job_promise_resolve_thenable_t *job_p) /**< points to the
                                                                                     *   PromiseResolveThenableJob */
{
  JERRY_ASSERT (job_p != NULL);

  ecma_free_value (job_p->promise);
  ecma_free_value (job_p->thenable);
  ecma_free_value (job_p->then);

  jmem_heap_free_block (job_p, sizeof (ecma_job_promise_resolve_thenable_t));
} /* ecma_free_promise_resolve_thenable_job */

/**
 * The processor for PromiseReactionJob.
 *
 * See also: ES2015 25.4.2.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
static ecma_value_t
ecma_process_promise_reaction_job (void *obj_p) /**< the job to be operated */
{
  ecma_job_promise_reaction_t *job_p = (ecma_job_promise_reaction_t *) obj_p;
  ecma_object_t *reaction_p = ecma_get_object_from_value (job_p->reaction);

  ecma_string_t *capability_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_CAPABILITY);
  ecma_string_t *handler_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_HANDLER);
  ecma_string_t *resolve_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_RESOLVE);
  ecma_string_t *reject_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_REJECT);

  /* 2. */
  ecma_value_t capability = ecma_op_object_get (reaction_p, capability_str_p);
  /* 3. */
  ecma_value_t handler = ecma_op_object_get (reaction_p, handler_str_p);

  JERRY_ASSERT (ecma_is_value_boolean (handler) || ecma_op_is_callable (handler));

  ecma_value_t handler_result;

  if (ecma_is_value_boolean (handler))
  {
    /* 4-5. True indicates "identity" and false indicates "thrower" */
    handler_result = ecma_copy_value (job_p->argument);
  }
  else
  {
    /* 6. */
    handler_result = ecma_op_function_call (ecma_get_object_from_value (handler),
                                            ECMA_VALUE_UNDEFINED,
                                            &(job_p->argument),
                                            1);
  }

  ecma_value_t status;

  if (ecma_is_value_false (handler) || ECMA_IS_VALUE_ERROR (handler_result))
  {
    if (ECMA_IS_VALUE_ERROR (handler_result))
    {
      handler_result = JERRY_CONTEXT (error_value);
    }

    /* 7. */
    ecma_value_t reject = ecma_op_object_get (ecma_get_object_from_value (capability), reject_str_p);

    JERRY_ASSERT (ecma_op_is_callable (reject));

    status = ecma_op_function_call (ecma_get_object_from_value (reject),
                                    ECMA_VALUE_UNDEFINED,
                                    &handler_result,
                                    1);
    ecma_free_value (reject);
  }
  else
  {
    /* 8. */
    ecma_value_t resolve = ecma_op_object_get (ecma_get_object_from_value (capability), resolve_str_p);

    JERRY_ASSERT (ecma_op_is_callable (resolve));

    status = ecma_op_function_call (ecma_get_object_from_value (resolve),
                                    ECMA_VALUE_UNDEFINED,
                                    &handler_result,
                                    1);
    ecma_free_value (resolve);
  }

  ecma_free_value (handler_result);
  ecma_free_value (handler);
  ecma_free_value (capability);
  ecma_free_promise_reaction_job (job_p);

  return status;
} /* ecma_process_promise_reaction_job */

/**
 * Process the PromiseResolveThenableJob.
 *
 * See also: ES2015 25.4.2.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
static ecma_value_t
ecma_process_promise_resolve_thenable_job (void *obj_p) /**< the job to be operated */
{
  ecma_job_promise_resolve_thenable_t *job_p = (ecma_job_promise_resolve_thenable_t *) obj_p;
  ecma_object_t *promise_p = ecma_get_object_from_value (job_p->promise);
  ecma_promise_resolving_functions_t *funcs = ecma_promise_create_resolving_functions (promise_p);

  ecma_string_t *str_resolve_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_RESOLVE_FUNCTION);
  ecma_string_t *str_reject_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_REJECT_FUNCTION);

  ecma_op_object_put (promise_p,
                      str_resolve_p,
                      funcs->resolve,
                      false);
  ecma_op_object_put (promise_p,
                      str_reject_p,
                      funcs->reject,
                      false);

  ecma_value_t argv[] = { funcs->resolve, funcs->reject };
  ecma_value_t ret;
  ecma_value_t then_call_result = ecma_op_function_call (ecma_get_object_from_value (job_p->then),
                                                         job_p->thenable,
                                                         argv,
                                                         2);

  ret = then_call_result;

  if (ECMA_IS_VALUE_ERROR (then_call_result))
  {
    then_call_result = JERRY_CONTEXT (error_value);

    ret = ecma_op_function_call (ecma_get_object_from_value (funcs->reject),
                                 ECMA_VALUE_UNDEFINED,
                                 &then_call_result,
                                 1);

    ecma_free_value (then_call_result);
  }

  ecma_promise_free_resolving_functions (funcs);
  ecma_free_promise_resolve_thenable_job (job_p);

  return ret;
} /* ecma_process_promise_resolve_thenable_job */

/**
 * Enqueue a Promise job into the jobqueue.
 */
static void
ecma_enqueue_job (ecma_job_handler_t handler, /**< the handler for the job */
                  void *job_p) /**< the job */
{
  ecma_job_queueitem_t *item_p = jmem_heap_alloc_block (sizeof (ecma_job_queueitem_t));
  item_p->job_p = job_p;
  item_p->handler = handler;
  item_p->next_p = NULL;

  if (JERRY_CONTEXT (job_queue_head_p) == NULL)
  {
    JERRY_CONTEXT (job_queue_head_p) = item_p;
    JERRY_CONTEXT (job_queue_tail_p) = item_p;
  }
  else
  {
    JERRY_CONTEXT (job_queue_tail_p)->next_p = item_p;
    JERRY_CONTEXT (job_queue_tail_p) = item_p;
  }
} /* ecma_enqueue_job */

/**
 * Enqueue a PromiseReactionJob into the jobqueue.
 */
void
ecma_enqueue_promise_reaction_job (ecma_value_t reaction, /**< PromiseReaction */
                                   ecma_value_t argument) /**< argument for the reaction */
{
  ecma_job_promise_reaction_t *job_p = ecma_create_promise_reaction_job (reaction, argument);
  ecma_enqueue_job (ecma_process_promise_reaction_job, job_p);
} /* ecma_enqueue_promise_reaction_job */

/**
 * Enqueue a PromiseResolveThenableJob into the jobqueue.
 */
void
ecma_enqueue_promise_resolve_thenable_job (ecma_value_t promise, /**< promise to be resolved */
                                           ecma_value_t thenable, /**< thenable object */
                                           ecma_value_t then) /**< 'then' function */
{
  ecma_job_promise_resolve_thenable_t *job_p = ecma_create_promise_resolve_thenable_job (promise,
                                                                                         thenable,
                                                                                         then);
  ecma_enqueue_job (ecma_process_promise_resolve_thenable_job, job_p);
} /* ecma_enqueue_promise_resolve_thenable_job */

/**
 * Process enqueued Promise jobs until the first thrown error or until the
 * jobqueue becomes empty.
 *
 * @return result of the last processed job - if the jobqueue was non-empty,
 *         undefined - otherwise.
 */
ecma_value_t
ecma_process_all_enqueued_jobs (void)
{
  ecma_value_t ret = ECMA_VALUE_UNDEFINED;

  while (JERRY_CONTEXT (job_queue_head_p) != NULL && !ECMA_IS_VALUE_ERROR (ret))
  {
    ecma_job_queueitem_t *item_p = JERRY_CONTEXT (job_queue_head_p);
    JERRY_CONTEXT (job_queue_head_p) = JERRY_CONTEXT (job_queue_head_p)->next_p;

    void *job_p = item_p->job_p;
    ecma_job_handler_t handler = item_p->handler;
    jmem_heap_free_block (item_p, sizeof (ecma_job_queueitem_t));

    ecma_free_value (ret);
    ret = handler (job_p);
  }

  return ret;
} /* ecma_process_all_enqueued_jobs */

/**
 * Release enqueued Promise jobs.
 */
void
ecma_free_all_enqueued_jobs (void)
{
  while (JERRY_CONTEXT (job_queue_head_p) != NULL)
  {
    ecma_job_queueitem_t *item_p = JERRY_CONTEXT (job_queue_head_p);
    JERRY_CONTEXT (job_queue_head_p) = item_p->next_p;
    void *job_p = item_p->job_p;
    jmem_heap_free_block (item_p, sizeof (ecma_job_queueitem_t));

    ecma_free_promise_reaction_job (job_p);
  }
} /* ecma_free_all_enqueued_jobs */

/**
 * @}
 * @}
 */
#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROMISE) */
