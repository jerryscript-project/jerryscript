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

#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-jobqueue.h"
#include "jerry-port.h"

#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmajobqueue ECMA Job Queue related routines
 * @{
 */

/**
 * The processor for PromiseReactionJob
 */
static ecma_value_t  __attribute__ ((unused))
ecma_job_process_promise_reaction_job (void *job_p) /**< the job to be operated */
{
  JERRY_UNUSED (job_p);
   /** TODO: implement the function body */
  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
} /* ecma_job_process_promise_reaction_job */

/**
 * The processor for PromiseResolveThenableJob
 */
static ecma_value_t __attribute__ ((unused))
ecma_job_process_promise_thenable_job (void *job_p) /**< the job to be operated */
{
  JERRY_UNUSED (job_p);
   /** TODO: implement the function body */
  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
} /* ecma_job_process_promise_thenable_job */

/**
 * @}
 * @}
 */
#endif /* !CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
