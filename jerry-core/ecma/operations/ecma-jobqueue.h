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

#ifndef ECMA_JOB_QUEUE_H
#define ECMA_JOB_QUEUE_H

#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmajobqueue ECMA Job Queue related routines
 * @{
 */

/**
 * Jerry job handler function type
 */
typedef ecma_value_t (*ecma_job_handler_t) (void *job_p);

/**
 * Description of the job queue item.
 */
typedef struct ecma_job_queueitem_t
{
  struct ecma_job_queueitem_t *next_p; /**< points to next item */
  ecma_job_handler_t handler; /**< the handler for the job*/
  void *job_p; /**< points to the job */
} ecma_job_queueitem_t;

void ecma_job_queue_init (void);

void ecma_enqueue_promise_reaction_job (ecma_value_t reaction, ecma_value_t argument);
void ecma_enqueue_promise_resolve_thenable_job (ecma_value_t promise, ecma_value_t thenable, ecma_value_t then);

ecma_value_t ecma_process_all_enqueued_jobs (void);

/**
 * @}
 * @}
 */
#endif /* !CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
#endif /* !ECMA_JOB_QUEUE_H */
