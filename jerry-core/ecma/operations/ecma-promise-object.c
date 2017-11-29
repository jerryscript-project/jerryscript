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

#include "ecma-boolean-object.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-jobqueue.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"
#include "ecma-promise-object.h"
#include "jcontext.h"

#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmapromiseobject ECMA Promise object related routines
 * @{
 */

/**
 * Check if an object is promise.
 *
 * @return true - if the object is a promise.
 *         false - otherwise.
 */
inline bool __attr_always_inline___
ecma_is_promise (ecma_object_t *obj_p) /**< points to object */
{
  return ecma_object_class_is (obj_p, LIT_MAGIC_STRING_PROMISE_UL);
} /* ecma_is_promise */

/**
 * Get the result of the promise.
 *
 * @return ecma value of the promise result.
 *         Returned value must be freed with ecma_free_value
 */
inline ecma_value_t
ecma_promise_get_result (ecma_object_t *obj_p) /**< points to promise object */
{
  JERRY_ASSERT (ecma_is_promise (obj_p));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

  return ecma_copy_value (ext_object_p->u.class_prop.u.value);
} /* ecma_promise_get_result */

/**
 * Set the PromiseResult of promise.
 */
inline void __attr_always_inline___
ecma_promise_set_result (ecma_object_t *obj_p, /**< points to promise object */
                         ecma_value_t result) /**< the result value */
{
  JERRY_ASSERT (ecma_is_promise (obj_p));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

  JERRY_ASSERT (ext_object_p->u.class_prop.u.value == ECMA_VALUE_UNDEFINED);

  ext_object_p->u.class_prop.u.value = result;
} /* ecma_promise_set_result */

/**
 * Get the PromiseState of promise.
 *
 * @return the state's enum value
 */
inline uint8_t __attr_always_inline___
ecma_promise_get_state (ecma_object_t *obj_p) /**< points to promise object */
{
  JERRY_ASSERT (ecma_is_promise (obj_p));

  return ((ecma_promise_object_t *) obj_p)->state;
} /* ecma_promise_get_state */

/**
 * Set the PromiseState of promise.
 */
inline void __attr_always_inline___
ecma_promise_set_state (ecma_object_t *obj_p, /**< points to promise object */
                        uint8_t state) /**< the state */
{
  JERRY_ASSERT (ecma_is_promise (obj_p));

  ((ecma_promise_object_t *) obj_p)->state = state;
} /* ecma_promise_set_state */

/**
 * Get the bool value of alreadyResolved.
 *
 * @return bool value of alreadyResolved.
 */
static bool
ecma_get_already_resolved_bool_value (ecma_value_t already_resolved) /**< the alreadyResolved */
{
  JERRY_ASSERT (ecma_is_value_object (already_resolved));

  ecma_object_t *already_resolved_p = ecma_get_object_from_value (already_resolved);
  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) already_resolved_p;

  JERRY_ASSERT (ext_object_p->u.class_prop.class_id == LIT_MAGIC_STRING_BOOLEAN_UL);

  return ext_object_p->u.class_prop.u.value == ecma_make_boolean_value (true);
} /* ecma_get_already_resolved_bool_value */

/**
 * Set the value of alreadyResolved.
 */
static void
ecma_set_already_resolved_value (ecma_value_t already_resolved, /**< the alreadyResolved */
                                 bool value) /**< the value to set */
{
  JERRY_ASSERT (ecma_is_value_object (already_resolved));

  ecma_object_t *already_resolved_p = ecma_get_object_from_value (already_resolved);
  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) already_resolved_p;

  JERRY_ASSERT (ext_object_p->u.class_prop.class_id == LIT_MAGIC_STRING_BOOLEAN_UL);

  ext_object_p->u.class_prop.u.value = ecma_make_boolean_value (value);
} /* ecma_set_already_resolved_value */

/**
 * Take a collection of Reactions and enqueue a new PromiseReactionJob for each Reaction.
 *
 * See also: ES2015 25.4.1.8
 */
static void
ecma_promise_trigger_reactions (ecma_collection_header_t *reactions, /**< lists of reactions */
                                ecma_value_t value) /**< value for resolve or reject */
{
  ecma_collection_iterator_t iter;
  ecma_collection_iterator_init (&iter, reactions);

  while (ecma_collection_iterator_next (&iter))
  {
    ecma_enqueue_promise_reaction_job (*iter.current_value_p, value);
  }

  ecma_free_values_collection (reactions, false);
} /* ecma_promise_trigger_reactions */

/**
 * Reject a Promise with a reason.
 *
 * See also: ES2015 25.4.1.7
 */
static void
ecma_reject_promise (ecma_value_t promise, /**< promise */
                     ecma_value_t reason) /**< reason for reject */
{
  ecma_object_t *obj_p = ecma_get_object_from_value (promise);

  JERRY_ASSERT (ecma_promise_get_state (obj_p) == ECMA_PROMISE_STATE_PENDING);

  ecma_promise_set_state (obj_p, ECMA_PROMISE_STATE_REJECTED);
  ecma_promise_set_result (obj_p, ecma_copy_value_if_not_object (reason));
  ecma_promise_object_t *promise_p = (ecma_promise_object_t *) obj_p;
  ecma_promise_trigger_reactions (promise_p->reject_reactions, reason);
  promise_p->reject_reactions = ecma_new_values_collection (NULL, 0, false);
  /* Free all fullfill_reactions. */
  ecma_free_values_collection (promise_p->fulfill_reactions, false);
  promise_p->fulfill_reactions = ecma_new_values_collection (NULL, 0, false);
} /* ecma_reject_promise */

/**
 * Fulfill a Promise with a value.
 *
 * See also: ES2015 25.4.1.4
 */
static void
ecma_fulfill_promise (ecma_value_t promise, /**< promise */
                      ecma_value_t value) /**< fulfilled value */
{
  ecma_object_t *obj_p = ecma_get_object_from_value (promise);

  JERRY_ASSERT (ecma_promise_get_state (obj_p) == ECMA_PROMISE_STATE_PENDING);

  ecma_promise_set_state (obj_p, ECMA_PROMISE_STATE_FULFILLED);
  ecma_promise_set_result (obj_p, ecma_copy_value_if_not_object (value));
  ecma_promise_object_t *promise_p = (ecma_promise_object_t *) obj_p;
  ecma_promise_trigger_reactions (promise_p->fulfill_reactions, value);
  promise_p->fulfill_reactions = ecma_new_values_collection (NULL, 0, false);
  /* Free all reject_reactions. */
  ecma_free_values_collection (promise_p->reject_reactions, false);
  promise_p->reject_reactions = ecma_new_values_collection (NULL, 0, false);
} /* ecma_fulfill_promise */

/**
 * Native handler for Promise Reject Function.
 *
 * See also: ES2015 25.4.1.3.1
 *
 * @return ecma value of undefined.
 */
static ecma_value_t
ecma_promise_reject_handler (const ecma_value_t function, /**< the function itself */
                             const ecma_value_t this, /**< this_arg of the function */
                             const ecma_value_t argv[], /**< argument list */
                             const ecma_length_t argc) /**< argument number */
{
  JERRY_UNUSED (this);

  ecma_string_t str_promise;
  ecma_string_t str_already_resolved;
  ecma_init_ecma_magic_string (&str_promise, LIT_INTERNAL_MAGIC_STRING_PROMISE);
  ecma_init_ecma_magic_string (&str_already_resolved, LIT_INTERNAL_MAGIC_STRING_ALREADY_RESOLVED);
  ecma_object_t *function_p = ecma_get_object_from_value (function);
  /* 2. */
  ecma_value_t promise = ecma_op_object_get (function_p, &str_promise);
  /* 1. */
  JERRY_ASSERT (ecma_is_promise (ecma_get_object_from_value (promise)));
  /* 3. */
  ecma_value_t already_resolved = ecma_op_object_get (function_p, &str_already_resolved);

  /* 4. */
  if (ecma_get_already_resolved_bool_value (already_resolved))
  {
    ecma_free_value (promise);
    ecma_free_value (already_resolved);
    return ECMA_VALUE_UNDEFINED;
  }

  /* 5. */
  ecma_set_already_resolved_value (already_resolved, true);

  /* 6. */
  ecma_value_t reject_value = (argc == 0) ? ECMA_VALUE_UNDEFINED : argv[0];
  ecma_reject_promise (promise, reject_value);
  ecma_free_value (promise);
  ecma_free_value (already_resolved);
  return ECMA_VALUE_UNDEFINED;
} /* ecma_promise_reject_handler */

/**
 * Native handler for Promise Resolve Function.
 *
 * See also: ES2015 25.4.1.3.2
 *
 * @return ecma value of undefined.
 */
static ecma_value_t
ecma_promise_resolve_handler (const ecma_value_t function, /**< the function itself */
                              const ecma_value_t this, /**< this_arg of the function */
                              const ecma_value_t argv[], /**< argument list */
                              const ecma_length_t argc) /**< argument number */
{
  JERRY_UNUSED (this);

  ecma_string_t str_promise;
  ecma_string_t str_already_resolved;
  ecma_init_ecma_magic_string (&str_promise, LIT_INTERNAL_MAGIC_STRING_PROMISE);
  ecma_init_ecma_magic_string (&str_already_resolved, LIT_INTERNAL_MAGIC_STRING_ALREADY_RESOLVED);
  ecma_object_t *function_p = ecma_get_object_from_value (function);
  /* 2. */
  ecma_value_t promise = ecma_op_object_get (function_p, &str_promise);
  /* 1. */
  JERRY_ASSERT (ecma_is_promise (ecma_get_object_from_value (promise)));
  /* 3. */
  ecma_value_t already_resolved = ecma_op_object_get (function_p, &str_already_resolved);

  /* 4. */
  if (ecma_get_already_resolved_bool_value (already_resolved))
  {
    goto end_of_resolve_function;
  }

  /* 5. */
  ecma_set_already_resolved_value (already_resolved, true);

  /* If the argc is 0, then fulfill the `undefined`. */
  if (argc == 0)
  {
    ecma_fulfill_promise (promise, ECMA_VALUE_UNDEFINED);
    goto end_of_resolve_function;
  }

  /* 6. */
  if (argv[0] == promise)
  {
    ecma_object_t *error_p = ecma_new_standard_error (ECMA_ERROR_TYPE);
    ecma_reject_promise (promise, ecma_make_object_value (error_p));
    ecma_deref_object (error_p);
    goto end_of_resolve_function;
  }

  /* 7. */
  if (!ecma_is_value_object (argv[0]))
  {
    ecma_fulfill_promise (promise, argv[0]);
    goto end_of_resolve_function;
  }

  /* 8. */
  ecma_value_t then = ecma_op_object_get_by_magic_id (ecma_get_object_from_value (argv[0]),
                                                                                  LIT_MAGIC_STRING_THEN);

  if (ECMA_IS_VALUE_ERROR (then))
  {
    /* 9. */
    then = JERRY_CONTEXT (error_value);
    ecma_reject_promise (promise, then);
  }
  else if (!ecma_op_is_callable (then))
  {
    /* 11 .*/
    ecma_fulfill_promise (promise, argv[0]);
  }
  else
  {
    /* 12 */
    ecma_enqueue_promise_resolve_thenable_job (promise, argv[0], then);
  }

  ecma_free_value (then);

end_of_resolve_function:
  ecma_free_value (promise);
  ecma_free_value (already_resolved);
  return ECMA_VALUE_UNDEFINED;
} /* ecma_promise_resolve_handler */

/**
 * CapabilityiesExecutor Function.
 *
 * See also: ES2015 25.4.1.5.1
 *
 * @return ecma value of undefined or typerror.
 *         Returned value must be freed with ecma_free_value
 */
static ecma_value_t
ecma_call_builtin_executor (ecma_object_t *executor_p, /**< the executor object */
                            ecma_value_t resolve_func, /**< the resolve function */
                            ecma_value_t reject_func) /**< the reject function */
{
  ecma_string_t *str_capability = ecma_new_ecma_string_from_uint32 (ECMA_PROMISE_PROPERTY_CAPABILITY);
  ecma_string_t *str_resolve = ecma_new_ecma_string_from_uint32 (ECMA_PROMISE_PROPERTY_RESOLVE);
  ecma_string_t *str_reject = ecma_new_ecma_string_from_uint32 (ECMA_PROMISE_PROPERTY_REJECT);

  /* 2. */
  ecma_value_t capability = ecma_op_object_get (executor_p, str_capability);
  /* 3. */
  ecma_value_t resolve = ecma_op_object_get (ecma_get_object_from_value (capability), str_resolve);

  if (resolve != ECMA_VALUE_UNDEFINED)
  {
    ecma_free_value (resolve);
    ecma_free_value (capability);
    ecma_deref_ecma_string (str_capability);
    ecma_deref_ecma_string (str_resolve);
    ecma_deref_ecma_string (str_reject);

    return ecma_raise_type_error (ECMA_ERR_MSG ("'resolve' function should be undefined."));
  }

  /* 4. */
  ecma_value_t reject = ecma_op_object_get (ecma_get_object_from_value (capability), str_reject);

  if (reject != ECMA_VALUE_UNDEFINED)
  {
    ecma_free_value (reject);
    ecma_free_value (capability);
    ecma_deref_ecma_string (str_capability);
    ecma_deref_ecma_string (str_resolve);
    ecma_deref_ecma_string (str_reject);

    return ecma_raise_type_error (ECMA_ERR_MSG ("'reject' function should be undefined."));
  }

  /* 5. */
  ecma_op_object_put (ecma_get_object_from_value (capability),
                      str_resolve,
                      resolve_func,
                      false);
  /* 6. */
  ecma_op_object_put (ecma_get_object_from_value (capability),
                      str_reject,
                      reject_func,
                      false);

  ecma_free_value (capability);
  ecma_deref_ecma_string (str_capability);
  ecma_deref_ecma_string (str_resolve);
  ecma_deref_ecma_string (str_reject);

  return ECMA_VALUE_UNDEFINED;
} /* ecma_call_builtin_executor */

/**
 * Create a PromiseCreateResovingFucntions.
 *
 * See also: ES2015 25.4.1.3
 *
 * @return pointer to the resolving functions
 */
ecma_promise_resolving_functions_t *
ecma_promise_create_resolving_functions (ecma_object_t *object_p) /**< the promise object */
{
  /* 1. */
  ecma_value_t already_resolved = ecma_op_create_boolean_object (ecma_make_boolean_value (false));

  ecma_string_t str_promise;
  ecma_string_t str_already_resolved;
  ecma_init_ecma_magic_string (&str_promise, LIT_INTERNAL_MAGIC_STRING_PROMISE);
  ecma_init_ecma_magic_string (&str_already_resolved, LIT_INTERNAL_MAGIC_STRING_ALREADY_RESOLVED);
  /* 2. */
  ecma_object_t *resolve_p;
  resolve_p = ecma_op_create_external_function_object (ecma_promise_resolve_handler);

  /* 3. */
  ecma_op_object_put (resolve_p,
                      &str_promise,
                      ecma_make_object_value (object_p),
                      false);
  /* 4. */
  ecma_op_object_put (resolve_p,
                      &str_already_resolved,
                      already_resolved,
                      false);
  /* 5. */
  ecma_object_t *reject_p;
  reject_p = ecma_op_create_external_function_object (ecma_promise_reject_handler);
  /* 6. */
  ecma_op_object_put (reject_p,
                      &str_promise,
                      ecma_make_object_value (object_p),
                      false);
  /* 7. */
  ecma_op_object_put (reject_p,
                      &str_already_resolved,
                      already_resolved,
                      false);

  /* 8. */
  ecma_promise_resolving_functions_t *funcs = jmem_heap_alloc_block (sizeof (ecma_promise_resolving_functions_t));
  funcs->resolve = ecma_make_object_value (resolve_p);
  funcs->reject = ecma_make_object_value (reject_p);

  ecma_free_value (already_resolved);

  return funcs;
} /* ecma_promise_create_resolving_functions */

/**
 * Free the heap and the member of the resolving functions.
 */
void
ecma_promise_free_resolving_functions (ecma_promise_resolving_functions_t *funcs) /**< points to the functions */
{
  ecma_free_value (funcs->resolve);
  ecma_free_value (funcs->reject);
  jmem_heap_free_block (funcs, sizeof (ecma_promise_resolving_functions_t));
} /* ecma_promise_free_resolving_functions */

/**
 * Create a promise object.
 *
 * See also: ES2015 25.4.3.1
 *
 * @return ecma value of the new promise object
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_create_promise_object (ecma_value_t executor, /**< the executor function or object */
                               ecma_promise_executor_type_t type) /**< indicates the type of executor */
{
  /* 3. */
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_PROMISE_PROTOTYPE);
  ecma_object_t *object_p = ecma_create_object (prototype_obj_p,
                                                sizeof (ecma_promise_object_t),
                                                ECMA_OBJECT_TYPE_CLASS);
  ecma_deref_object (prototype_obj_p);
  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  ext_object_p->u.class_prop.class_id = LIT_MAGIC_STRING_PROMISE_UL;
  ext_object_p->u.class_prop.u.value = ECMA_VALUE_UNDEFINED;
  ecma_promise_object_t *promise_object_p = (ecma_promise_object_t *) object_p;
  promise_object_p->fulfill_reactions = NULL;
  promise_object_p->reject_reactions = NULL;

  /* 5 */
  ecma_promise_set_state (object_p, ECMA_PROMISE_STATE_PENDING);
  /* 6-7. */
  promise_object_p->fulfill_reactions = ecma_new_values_collection (NULL, 0, false);
  promise_object_p->reject_reactions = ecma_new_values_collection (NULL, 0, false);
  /* 8. */
  ecma_promise_resolving_functions_t *funcs = ecma_promise_create_resolving_functions (object_p);

  ecma_string_t str_resolve, str_reject;
  ecma_init_ecma_magic_string (&str_resolve, LIT_INTERNAL_MAGIC_STRING_RESOLVE_FUNCTION);
  ecma_init_ecma_magic_string (&str_reject, LIT_INTERNAL_MAGIC_STRING_REJECT_FUNCTION);
  ecma_op_object_put (object_p,
                      &str_resolve,
                      funcs->resolve,
                      false);
  ecma_op_object_put (object_p,
                      &str_reject,
                      funcs->reject,
                      false);

  /* 9. */
  ecma_value_t completion = ECMA_VALUE_UNDEFINED;

  if (type == ECMA_PROMISE_EXECUTOR_FUNCTION)
  {
    JERRY_ASSERT (ecma_op_is_callable (executor));

    ecma_value_t argv[] = { funcs->resolve, funcs->reject };
    completion = ecma_op_function_call (ecma_get_object_from_value (executor),
                                        ECMA_VALUE_UNDEFINED,
                                        argv,
                                        2);
  }
  else if (type == ECMA_PROMISE_EXECUTOR_OBJECT)
  {
    JERRY_ASSERT (ecma_is_value_object (executor));

    completion = ecma_call_builtin_executor (ecma_get_object_from_value (executor),
                                             funcs->resolve,
                                             funcs->reject);
  }
  else
  {
    JERRY_ASSERT (type == ECMA_PROMISE_EXECUTOR_EMPTY);
    JERRY_UNUSED (executor);
  }

  ecma_value_t status = ECMA_VALUE_EMPTY;

  if (ECMA_IS_VALUE_ERROR (completion))
  {
    /* 10.a. */
    completion = JERRY_CONTEXT (error_value);
    status = ecma_op_function_call (ecma_get_object_from_value (funcs->reject),
                                    ECMA_VALUE_UNDEFINED,
                                    &completion,
                                    1);
  }

  ecma_promise_free_resolving_functions (funcs);
  ecma_free_value (completion);

  /* 10.b. */
  if (ECMA_IS_VALUE_ERROR (status))
  {
    ecma_deref_object (object_p);
    return status;
  }

  /* 11. */
  ecma_free_value (status);

  return ecma_make_object_value (object_p);
} /* ecma_op_create_promise_object */

/**
 * Create a new PromiseCapability.
 *
 * See also: ES2015 25.4.1.5
 *
 * @return ecma value of the new PromiseCapability
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_promise_new_capability (void)
{
  /* 3. */
  ecma_object_t *capability_p = ecma_op_create_object_object_noarg ();
  ecma_string_t *str_capability = ecma_new_ecma_string_from_uint32 (ECMA_PROMISE_PROPERTY_CAPABILITY);
  ecma_string_t *str_promise = ecma_new_ecma_string_from_uint32 (ECMA_PROMISE_PROPERTY_PROMISE);
  /* 4. */
  ecma_object_t *executor_p;
  executor_p = ecma_op_create_object_object_noarg ();
  ecma_value_t executor = ecma_make_object_value (executor_p);
  /* 5. */
  ecma_op_object_put (executor_p,
                      str_capability,
                      ecma_make_object_value (capability_p),
                      false);

  /* 6. */
  ecma_value_t promise = ecma_op_create_promise_object (executor, ECMA_PROMISE_EXECUTOR_OBJECT);

  /* 10. */
  ecma_op_object_put (capability_p,
                      str_promise,
                      promise,
                      false);

  ecma_deref_object (executor_p);
  ecma_deref_ecma_string (str_promise);
  ecma_deref_ecma_string (str_capability);
  /* 7. */
  if (ECMA_IS_VALUE_ERROR (promise))
  {
    ecma_free_value (promise);
    ecma_deref_object (capability_p);
    return promise;
  }

  ecma_free_value (promise);
  /* 8. */
  ecma_string_t *str_resolve = ecma_new_ecma_string_from_uint32 (ECMA_PROMISE_PROPERTY_RESOLVE);
  ecma_value_t resolve = ecma_op_object_get (capability_p, str_resolve);
  ecma_deref_ecma_string (str_resolve);

  if (!ecma_op_is_callable (resolve))
  {
    ecma_free_value (resolve);
    ecma_deref_object (capability_p);
    return ecma_raise_type_error (ECMA_ERR_MSG ("'resolve' parameter must be callable."));
  }

  ecma_free_value (resolve);
  /* 9. */
  ecma_string_t *str_reject = ecma_new_ecma_string_from_uint32 (ECMA_PROMISE_PROPERTY_REJECT);
  ecma_value_t reject = ecma_op_object_get (capability_p, str_reject);
  ecma_deref_ecma_string (str_reject);

  if (!ecma_op_is_callable (reject))
  {
    ecma_free_value (reject);
    ecma_deref_object (capability_p);
    return ecma_raise_type_error (ECMA_ERR_MSG ("'reject' parameter must be callable."));
  }

  ecma_free_value (reject);
  /* 11. */
  return ecma_make_object_value (capability_p);
} /* ecma_promise_new_capability */

/**
 * It performs the "then" operation on promiFulfilled
 * and onRejected as its settlement actions.
 *
 * See also: 25.4.5.3.1
 *
 * @return ecma value of the new promise object
 *         Returned value must be freed with ecma_free_value
 */
static ecma_value_t
ecma_promise_do_then (ecma_value_t promise, /**< the promise which call 'then' */
                      ecma_value_t on_fulfilled, /**< on_fulfilled function */
                      ecma_value_t on_rejected, /**< on_rejected function */
                      ecma_value_t result_capability) /**< promise capability */
{
  ecma_string_t *str_capability = ecma_new_ecma_string_from_uint32 (ECMA_PROMISE_PROPERTY_CAPABILITY);
  ecma_string_t *str_handler = ecma_new_ecma_string_from_uint32 (ECMA_PROMISE_PROPERTY_HANDLER);
  ecma_string_t *str_promise = ecma_new_ecma_string_from_uint32 (ECMA_PROMISE_PROPERTY_PROMISE);

  /* 3. boolean true indicates "indentity" */
  if (!ecma_op_is_callable (on_fulfilled))
  {
    on_fulfilled = ecma_make_boolean_value (true);
  }

  /* 4. boolean false indicates "thrower" */
  if (!ecma_op_is_callable (on_rejected))
  {
    on_rejected = ecma_make_boolean_value (false);
  }

  /* 5-6. */
  ecma_object_t *fulfill_reaction_p = ecma_op_create_object_object_noarg ();
  ecma_object_t *reject_reaction_p = ecma_op_create_object_object_noarg ();
  ecma_op_object_put (fulfill_reaction_p,
                      str_capability,
                      result_capability,
                      false);
  ecma_op_object_put (fulfill_reaction_p,
                      str_handler,
                      on_fulfilled,
                      false);

  ecma_op_object_put (reject_reaction_p,
                      str_capability,
                      result_capability,
                      false);
  ecma_op_object_put (reject_reaction_p,
                      str_handler,
                      on_rejected,
                      false);

  ecma_object_t *obj_p = ecma_get_object_from_value (promise);
  ecma_promise_object_t *promise_p = (ecma_promise_object_t *) obj_p;

  if (ecma_promise_get_state (obj_p) == ECMA_PROMISE_STATE_PENDING)
  {
    /* 7. */
    ecma_append_to_values_collection (promise_p->fulfill_reactions,
                                      ecma_make_object_value (fulfill_reaction_p),
                                      false);

    ecma_append_to_values_collection (promise_p->reject_reactions,
                                      ecma_make_object_value (reject_reaction_p),
                                      false);
  }
  else if (ecma_promise_get_state (obj_p) == ECMA_PROMISE_STATE_FULFILLED)
  {
    /* 8. */
    ecma_value_t value = ecma_promise_get_result (obj_p);
    ecma_enqueue_promise_reaction_job (ecma_make_object_value (fulfill_reaction_p), value);
    ecma_free_value (value);
  }
  else
  {
    /* 9. */
    ecma_value_t reason = ecma_promise_get_result (obj_p);
    ecma_enqueue_promise_reaction_job (ecma_make_object_value (reject_reaction_p), reason);
    ecma_free_value (reason);
  }

  /* 10. */
  ecma_value_t ret = ecma_op_object_get (ecma_get_object_from_value (result_capability), str_promise);

  ecma_deref_object (fulfill_reaction_p);
  ecma_deref_object (reject_reaction_p);
  ecma_deref_ecma_string (str_capability);
  ecma_deref_ecma_string (str_handler);
  ecma_deref_ecma_string (str_promise);
  return ret;
} /* ecma_promise_do_then */

/**
 * The common function for ecma_builtin_promise_prototype_then
 * and ecma_builtin_promise_prototype_catch.
 *
 * @return ecma value of a new promise object.
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_promise_then (ecma_value_t promise, /**< the promise which call 'then' */
                   ecma_value_t on_fulfilled, /**< on_fulfilled function */
                   ecma_value_t on_rejected) /**< on_rejected function */
{
  if (!ecma_is_value_object (promise))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("'this' is not an object."));
  }

  ecma_object_t *obj = ecma_get_object_from_value (promise);

  if (!ecma_is_promise (obj))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("'this' is not a Promise."));
  }

  ecma_value_t result_capability = ecma_promise_new_capability ();

  if (ECMA_IS_VALUE_ERROR (result_capability))
  {
    return result_capability;
  }

  ecma_value_t ret = ecma_promise_do_then (promise, on_fulfilled, on_rejected, result_capability);
  ecma_free_value (result_capability);

  return ret;
} /* ecma_promise_then */

/**
 * @}
 * @}
 */
#endif /* !CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
