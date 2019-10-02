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

#ifndef ECMA_PROMISE_OBJECT_H
#define ECMA_PROMISE_OBJECT_H

#if ENABLED (JERRY_ES2015_BUILTIN_PROMISE)
#include "ecma-globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaarraybufferobject ECMA ArrayBuffer object related routines
 * @{
 */

/**
 * The PromiseState of promise object.
 */
typedef enum
{
  ECMA_PROMISE_STATE_PENDING, /**< pending state */
  ECMA_PROMISE_STATE_FULFILLED, /**< fulfilled state */
  ECMA_PROMISE_STATE_REJECTED, /** rejected state */
  ECMA_PROMISE_STATE__COUNT /**< number of states */
} ecma_promise_state_t;

/**
 * Indicates the type of the executor in promise construct.
 */
typedef enum
{
  ECMA_PROMISE_EXECUTOR_FUNCTION, /**< the executor is a function, it is for the usual constructor */
  ECMA_PROMISE_EXECUTOR_OBJECT, /**< the executor is an object, it is for the `then` routine */
  ECMA_PROMISE_EXECUTOR_EMPTY /**< the executor is empty, it is for external C API */
} ecma_promise_executor_type_t;

/**
 * Description of the promise resolving functions.
 */
typedef struct
{
  ecma_value_t resolve; /**< the resolve function */
  ecma_value_t reject; /**< the reject function */
} ecma_promise_resolving_functions_t;

/**
 * Description of the promise object.
 * It need more space than normal object to store builtin properties.
 */
typedef struct
{
  ecma_extended_object_t ecma_extended_object_t; /**< extended object part */
  uint8_t state; /**< promise state, see ecma_promise_state_t */
  ecma_collection_t *fulfill_reactions; /**< list of PromiseFullfillReactions */
  ecma_collection_t *reject_reactions; /**< list of PromiseRejectReactions */
} ecma_promise_object_t;

bool ecma_is_promise (ecma_object_t *obj_p);
ecma_value_t
ecma_op_create_promise_object (ecma_value_t executor, ecma_promise_executor_type_t type);
uint8_t ecma_promise_get_state (ecma_object_t *promise_p);
ecma_value_t ecma_promise_get_result (ecma_object_t *promise_p);
ecma_value_t ecma_promise_new_capability (void);
ecma_value_t
ecma_promise_then (ecma_value_t promise,
                   ecma_value_t on_fulfilled,
                   ecma_value_t on_rejected);
ecma_promise_resolving_functions_t *
ecma_promise_create_resolving_functions (ecma_object_t *object_p);
void ecma_promise_free_resolving_functions (ecma_promise_resolving_functions_t *funcs);

/**
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROMISE) */
#endif /* !ECMA_PROMISE_OBJECT_H */
