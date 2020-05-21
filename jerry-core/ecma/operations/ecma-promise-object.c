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

#if ENABLED (JERRY_ES2015_BUILTIN_PROMISE)

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
inline bool JERRY_ATTR_ALWAYS_INLINE
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
ecma_value_t
ecma_promise_get_result (ecma_object_t *obj_p) /**< points to promise object */
{
  JERRY_ASSERT (ecma_is_promise (obj_p));

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

  return ecma_copy_value (ext_object_p->u.class_prop.u.value);
} /* ecma_promise_get_result */

/**
 * Set the PromiseResult of promise.
 */
static inline void JERRY_ATTR_ALWAYS_INLINE
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
uint16_t
ecma_promise_get_flags (ecma_object_t *obj_p) /**< points to promise object */
{
  JERRY_ASSERT (ecma_is_promise (obj_p));

  return ((ecma_extended_object_t *) obj_p)->u.class_prop.extra_info;
} /* ecma_promise_get_flags */

/**
 * Set the PromiseState of promise.
 */
static inline void JERRY_ATTR_ALWAYS_INLINE
ecma_promise_set_state (ecma_object_t *obj_p, /**< points to promise object */
                        bool is_fulfilled) /**< new flags */
{
  JERRY_ASSERT (ecma_is_promise (obj_p));
  JERRY_ASSERT (ecma_promise_get_flags (obj_p) & ECMA_PROMISE_IS_PENDING);

  uint16_t flags_to_invert = (is_fulfilled ? (ECMA_PROMISE_IS_PENDING | ECMA_PROMISE_IS_FULFILLED)
                                           : ECMA_PROMISE_IS_PENDING);

  ((ecma_extended_object_t *) obj_p)->u.class_prop.extra_info ^= flags_to_invert;
} /* ecma_promise_set_state */

/**
 * Take a collection of Reactions and enqueue a new PromiseReactionJob for each Reaction.
 *
 * See also: ES2015 25.4.1.8
 */
static void
ecma_promise_trigger_reactions (ecma_collection_t *reactions, /**< lists of reactions */
                                ecma_value_t value, /**< value for resolve or reject */
                                bool is_reject) /**< true if promise is rejected, false otherwise */
{
  ecma_value_t *buffer_p = reactions->buffer_p;
  ecma_value_t *buffer_end_p = buffer_p + reactions->item_count;

  while (buffer_p < buffer_end_p)
  {
    ecma_value_t capability_with_tag = *buffer_p++;
    ecma_object_t *capability_obj_p = ECMA_GET_NON_NULL_POINTER_FROM_POINTER_TAG (ecma_object_t, capability_with_tag);
    ecma_value_t capability = ecma_make_object_value (capability_obj_p);

    if (!is_reject)
    {
      ecma_value_t handler = ECMA_VALUE_TRUE;

      if (JMEM_CP_GET_FIRST_BIT_FROM_POINTER_TAG (capability_with_tag))
      {
        handler = *buffer_p++;
      }

      ecma_enqueue_promise_reaction_job (capability, handler, value);
    }
    else if (JMEM_CP_GET_FIRST_BIT_FROM_POINTER_TAG (capability_with_tag))
    {
      buffer_p++;
    }

    if (is_reject)
    {
      ecma_value_t handler = ECMA_VALUE_FALSE;

      if (JMEM_CP_GET_SECOND_BIT_FROM_POINTER_TAG (capability_with_tag))
      {
        handler = *buffer_p++;
      }

      ecma_enqueue_promise_reaction_job (capability, handler, value);
    }
    else if (JMEM_CP_GET_SECOND_BIT_FROM_POINTER_TAG (capability_with_tag))
    {
      buffer_p++;
    }
  }
} /* ecma_promise_trigger_reactions */

/**
 * Checks whether a resolver is called before.
 *
 * @return true if it was called before, false otherwise
 */
static bool
ecma_is_resolver_already_called (ecma_object_t *resolver_p, /**< resolver */
                                 ecma_object_t *promise_obj_p) /**< promise */
{
  ecma_string_t *str_already_resolved_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_ALREADY_RESOLVED);
  ecma_property_t *property_p = ecma_find_named_property (resolver_p, str_already_resolved_p);

  if (property_p == NULL)
  {
    return (ecma_promise_get_flags (promise_obj_p) & ECMA_PROMISE_ALREADY_RESOLVED) != 0;
  }

  JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA);

  ecma_value_t already_resolved = ECMA_PROPERTY_VALUE_PTR (property_p)->value;
  ecma_object_t *object_p = ecma_get_object_from_value (already_resolved);
  JERRY_ASSERT (ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_CLASS);

  ecma_extended_object_t *already_resolved_p = (ecma_extended_object_t *) object_p;
  JERRY_ASSERT (already_resolved_p->u.class_prop.class_id == LIT_MAGIC_STRING_BOOLEAN_UL);

  ecma_value_t current_value = already_resolved_p->u.class_prop.u.value;
  already_resolved_p->u.class_prop.u.value = ECMA_VALUE_TRUE;

  return current_value == ECMA_VALUE_TRUE;
} /* ecma_is_resolver_already_called */

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

  JERRY_ASSERT (ecma_promise_get_flags (obj_p) & ECMA_PROMISE_IS_PENDING);

  ecma_promise_set_state (obj_p, false);
  ecma_promise_set_result (obj_p, ecma_copy_value_if_not_object (reason));
  ecma_promise_object_t *promise_p = (ecma_promise_object_t *) obj_p;

  /* GC can be triggered by ecma_new_collection so freeing the collection
     first and creating a new one might cause a heap after use event. */
  ecma_collection_t *reactions = promise_p->reactions;

  /* Fulfill reactions will never be triggered. */
  ecma_promise_trigger_reactions (reactions, reason, true);

  promise_p->reactions = ecma_new_collection ();

  ecma_collection_destroy (reactions);
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

  JERRY_ASSERT (ecma_promise_get_flags (obj_p) & ECMA_PROMISE_IS_PENDING);

  ecma_promise_set_state (obj_p, true);
  ecma_promise_set_result (obj_p, ecma_copy_value_if_not_object (value));
  ecma_promise_object_t *promise_p = (ecma_promise_object_t *) obj_p;

  /* GC can be triggered by ecma_new_collection so freeing the collection
     first and creating a new one might cause a heap after use event. */
  ecma_collection_t *reactions = promise_p->reactions;

  /* Reject reactions will never be triggered. */
  ecma_promise_trigger_reactions (reactions, value, false);

  promise_p->reactions = ecma_new_collection ();

  ecma_collection_destroy (reactions);
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

  ecma_object_t *function_p = ecma_get_object_from_value (function);
  /* 2. */
  ecma_value_t promise = ecma_op_object_get_by_magic_id (function_p, LIT_INTERNAL_MAGIC_STRING_PROMISE);
  /* 1. */
  ecma_object_t *promise_obj_p = ecma_get_object_from_value (promise);
  JERRY_ASSERT (ecma_is_promise (promise_obj_p));

  /* 3., 4. */
  if (!ecma_is_resolver_already_called (function_p, promise_obj_p))
  {
    /* 5. */
    ((ecma_extended_object_t *) promise_obj_p)->u.class_prop.extra_info |= ECMA_PROMISE_ALREADY_RESOLVED;

    /* 6. */
    ecma_value_t reject_value = (argc == 0) ? ECMA_VALUE_UNDEFINED : argv[0];
    ecma_reject_promise (promise, reject_value);
  }

  ecma_free_value (promise);
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

  ecma_object_t *function_p = ecma_get_object_from_value (function);
  /* 2. */
  ecma_value_t promise = ecma_op_object_get_by_magic_id (function_p, LIT_INTERNAL_MAGIC_STRING_PROMISE);
  /* 1. */
  ecma_object_t *promise_obj_p = ecma_get_object_from_value (promise);
  JERRY_ASSERT (ecma_is_promise (promise_obj_p));

  /* 3., 4. */
  if (ecma_is_resolver_already_called (function_p, promise_obj_p))
  {
    goto end_of_resolve_function;
  }

  /* 5. */
  ((ecma_extended_object_t *) promise_obj_p)->u.class_prop.extra_info |= ECMA_PROMISE_ALREADY_RESOLVED;

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
    then = jcontext_take_exception ();
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
  return ECMA_VALUE_UNDEFINED;
} /* ecma_promise_resolve_handler */

/**
 * CapabilitiesExecutor Function.
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
  ecma_string_t *capability_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_CAPABILITY);
  ecma_string_t *resolve_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_RESOLVE);
  ecma_string_t *reject_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_REJECT);

  /* 2. */
  ecma_value_t capability = ecma_op_object_get (executor_p, capability_str_p);
  /* 3. */
  ecma_value_t resolve = ecma_op_object_get (ecma_get_object_from_value (capability), resolve_str_p);

  if (resolve != ECMA_VALUE_UNDEFINED)
  {
    ecma_free_value (resolve);
    ecma_free_value (capability);

    return ecma_raise_type_error (ECMA_ERR_MSG ("'resolve' function should be undefined."));
  }

  /* 4. */
  ecma_value_t reject = ecma_op_object_get (ecma_get_object_from_value (capability), reject_str_p);

  if (reject != ECMA_VALUE_UNDEFINED)
  {
    ecma_free_value (reject);
    ecma_free_value (capability);

    return ecma_raise_type_error (ECMA_ERR_MSG ("'reject' function should be undefined."));
  }

  /* 5. */
  ecma_op_object_put (ecma_get_object_from_value (capability),
                      resolve_str_p,
                      resolve_func,
                      false);
  /* 6. */
  ecma_op_object_put (ecma_get_object_from_value (capability),
                      reject_str_p,
                      reject_func,
                      false);

  ecma_free_value (capability);

  return ECMA_VALUE_UNDEFINED;
} /* ecma_call_builtin_executor */

/**
 * Helper function for PromiseCreateResovingFucntions.
 *
 * See also: ES2015 25.4.1.3 2. - 7.
 *
 * @return pointer to the resolving function
 */
static ecma_value_t
ecma_promise_create_resolving_functions_helper (ecma_object_t *obj_p, /**< Promise Object */
                                                ecma_external_handler_t handler_cb) /**< Callback handler */
{
  ecma_string_t *str_promise_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE);
  ecma_object_t *func_obj_p = ecma_op_create_external_function_object (handler_cb);

  ecma_op_object_put (func_obj_p,
                      str_promise_p,
                      ecma_make_object_value (obj_p),
                      false);

  return ecma_make_object_value (func_obj_p);
} /* ecma_promise_create_resolving_functions_helper */

/**
 * Create a PromiseCreateResovingFucntions.
 *
 * See also: ES2015 25.4.1.3
 *
 * @return pointer to the resolving functions
 */
void
ecma_promise_create_resolving_functions (ecma_object_t *object_p, /**< the promise object */
                                         ecma_promise_resolving_functions_t *funcs, /**< [out] resolving functions */
                                         bool create_already_resolved) /**< create already resolved flag */
{
  /* 2. - 4. */
  funcs->resolve = ecma_promise_create_resolving_functions_helper (object_p,
                                                                   ecma_promise_resolve_handler);

  /* 5. - 7. */
  funcs->reject = ecma_promise_create_resolving_functions_helper (object_p,
                                                                  ecma_promise_reject_handler);
  if (!create_already_resolved)
  {
    return;
  }

  ecma_value_t already_resolved = ecma_op_create_boolean_object (ECMA_VALUE_FALSE);
  ecma_string_t *str_already_resolved_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_ALREADY_RESOLVED);
  ecma_property_value_t *value_p;

  value_p = ecma_create_named_data_property (ecma_get_object_from_value (funcs->resolve),
                                             str_already_resolved_p,
                                             ECMA_PROPERTY_FIXED,
                                             NULL);
  value_p->value = already_resolved;

  value_p = ecma_create_named_data_property (ecma_get_object_from_value (funcs->reject),
                                             str_already_resolved_p,
                                             ECMA_PROPERTY_FIXED,
                                             NULL);
  value_p->value = already_resolved;

  ecma_free_value (already_resolved);
} /* ecma_promise_create_resolving_functions */

/**
 * Free the heap and the member of the resolving functions.
 */
void
ecma_promise_free_resolving_functions (ecma_promise_resolving_functions_t *funcs) /**< points to the functions */
{
  ecma_free_value (funcs->resolve);
  ecma_free_value (funcs->reject);
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
  JERRY_ASSERT (JERRY_CONTEXT (current_new_target) != NULL);
  /* 3. */
  ecma_object_t *proto_p = ecma_op_get_prototype_from_constructor (JERRY_CONTEXT (current_new_target),
                                                                   ECMA_BUILTIN_ID_PROMISE_PROTOTYPE);

  if (JERRY_UNLIKELY (proto_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  /* Calling ecma_new_collection might trigger a GC call, so this
   * allocation is performed before the object is constructed. */
  ecma_collection_t *reactions = ecma_new_collection ();

  ecma_object_t *object_p = ecma_create_object (proto_p,
                                                sizeof (ecma_promise_object_t),
                                                ECMA_OBJECT_TYPE_CLASS);
  ecma_deref_object (proto_p);
  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  ext_object_p->u.class_prop.class_id = LIT_MAGIC_STRING_PROMISE_UL;
  /* 5 */
  ext_object_p->u.class_prop.extra_info = ECMA_PROMISE_IS_PENDING;
  ext_object_p->u.class_prop.u.value = ECMA_VALUE_UNDEFINED;
  ecma_promise_object_t *promise_object_p = (ecma_promise_object_t *) object_p;

  /* 6-7. */
  promise_object_p->reactions = reactions;
  /* 8. */
  ecma_promise_resolving_functions_t funcs;
  ecma_promise_create_resolving_functions (object_p, &funcs, false);

  ecma_string_t *str_resolve_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_RESOLVE_FUNCTION);
  ecma_string_t *str_reject_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_REJECT_FUNCTION);

  ecma_op_object_put (object_p,
                      str_resolve_p,
                      funcs.resolve,
                      false);
  ecma_op_object_put (object_p,
                      str_reject_p,
                      funcs.reject,
                      false);

  /* 9. */
  ecma_value_t completion = ECMA_VALUE_UNDEFINED;

  if (type == ECMA_PROMISE_EXECUTOR_FUNCTION)
  {
    JERRY_ASSERT (ecma_op_is_callable (executor));

    ecma_value_t argv[] = { funcs.resolve, funcs.reject };
    completion = ecma_op_function_call (ecma_get_object_from_value (executor),
                                        ECMA_VALUE_UNDEFINED,
                                        argv,
                                        2);
  }
  else if (type == ECMA_PROMISE_EXECUTOR_OBJECT)
  {
    JERRY_ASSERT (ecma_is_value_object (executor));

    completion = ecma_call_builtin_executor (ecma_get_object_from_value (executor),
                                             funcs.resolve,
                                             funcs.reject);
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
    completion = jcontext_take_exception ();
    status = ecma_op_function_call (ecma_get_object_from_value (funcs.reject),
                                    ECMA_VALUE_UNDEFINED,
                                    &completion,
                                    1);
  }

  ecma_promise_free_resolving_functions (&funcs);
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
 * 25.4.1.5.1 GetCapabilitiesExecutor Functions
 *
 * Checks and sets a promiseCapability's resolve and reject properties.
 *
 * @return ECMA_VALUE_UNDEFINED or TypeError
 *         returned value must be freed with ecma_free_value
 */
static ecma_value_t
ecma_op_get_capabilities_executor_cb (const ecma_value_t function_obj, /**< the function itself */
                                      const ecma_value_t this_val, /**< this_arg of the function */
                                      const ecma_value_t args_p[], /**< argument list */
                                      const ecma_length_t args_count) /**< argument number */
{
  JERRY_UNUSED (this_val);
  /* 1. */
  ecma_value_t capability = ecma_op_object_get_by_magic_id (ecma_get_object_from_value (function_obj),
                                                            LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_CAPABILITY);
  JERRY_ASSERT (ecma_is_value_object (capability));

  /* 2. */
  ecma_object_t *capability_obj_p = ecma_get_object_from_value (capability);

  /* 3. */
  ecma_value_t resolve = ecma_op_object_get_by_magic_id (capability_obj_p,
                                                         LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_RESOLVE);

  if (!ecma_is_value_undefined (resolve))
  {
    ecma_free_value (resolve);
    ecma_deref_object (capability_obj_p);

    return ecma_raise_type_error (ECMA_ERR_MSG ("Resolve must be undefined"));
  }

  /* 4. */
  ecma_value_t reject = ecma_op_object_get_by_magic_id (capability_obj_p,
                                                        LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_REJECT);

  if (!ecma_is_value_undefined (reject))
  {
    ecma_free_value (reject);
    ecma_deref_object (capability_obj_p);

    return ecma_raise_type_error (ECMA_ERR_MSG ("Reject must be undefined"));
  }

  /* 5. */
  ecma_op_object_put (capability_obj_p,
                      ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_RESOLVE),
                      args_count > 0 ? args_p[0] : ECMA_VALUE_UNDEFINED,
                      false);

  /* 6. */
  ecma_op_object_put (capability_obj_p,
                      ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_REJECT),
                      args_count > 1 ? args_p[1] : ECMA_VALUE_UNDEFINED,
                      false);

  ecma_deref_object (capability_obj_p);

  /* 7. */
  return ECMA_VALUE_UNDEFINED;
} /* ecma_op_get_capabilities_executor_cb */

/**
 * Create a new PromiseCapability.
 *
 * See also: ES2015 25.4.1.5
 *
 * @return ecma value of the new PromiseCapability
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_promise_new_capability (ecma_value_t constructor)
{
  /* 1. */
  if (!ecma_is_constructor (constructor))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Invalid capability"));
  }

  ecma_object_t *constructor_obj_p = ecma_get_object_from_value (constructor);
  /* 3. */
  ecma_object_t *capability_p = ecma_op_create_object_object_noarg ();

  ecma_string_t *capability_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_CAPABILITY);
  ecma_string_t *promise_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_PROMISE);
  /* 4. */
  ecma_object_t *executor_p = ecma_op_create_external_function_object (ecma_op_get_capabilities_executor_cb);
  ecma_value_t executor = ecma_make_object_value (executor_p);
  /* 5. */
  ecma_op_object_put (executor_p,
                      capability_str_p,
                      ecma_make_object_value (capability_p),
                      false);

  /* 6. */
  ecma_value_t promise = ecma_op_function_construct (constructor_obj_p,
                                                     constructor_obj_p,
                                                     &executor,
                                                     1);
  ecma_deref_object (executor_p);

  /* 7. */
  if (ECMA_IS_VALUE_ERROR (promise))
  {
    ecma_deref_object (capability_p);
    return promise;
  }

  /* 10. */
  ecma_op_object_put (capability_p,
                      promise_str_p,
                      promise,
                      false);

  ecma_free_value (promise);

  /* 8. */
  ecma_string_t *resolve_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_RESOLVE);
  ecma_value_t resolve = ecma_op_object_get (capability_p, resolve_str_p);

  if (!ecma_op_is_callable (resolve))
  {
    ecma_free_value (resolve);
    ecma_deref_object (capability_p);
    return ecma_raise_type_error (ECMA_ERR_MSG ("'resolve' parameter must be callable."));
  }

  ecma_free_value (resolve);
  /* 9. */
  ecma_string_t *reject_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_REJECT);
  ecma_value_t reject = ecma_op_object_get (capability_p, reject_str_p);

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
 * The common function for 'reject' and 'resolve'.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_promise_reject_or_resolve (ecma_value_t this_arg, /**< "this" argument */
                                ecma_value_t value, /**< rejected or resolved value */
                                bool is_resolve) /**< the operation is resolve */
{
  if (!ecma_is_value_object (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("'this' is not an object."));
  }

  if (is_resolve
      && ecma_is_value_object (value)
      && ecma_is_promise (ecma_get_object_from_value (value)))
  {
    ecma_object_t *object_p = ecma_get_object_from_value (value);
    ecma_value_t constructor = ecma_op_object_get_by_magic_id (object_p, LIT_MAGIC_STRING_CONSTRUCTOR);

    if (ECMA_IS_VALUE_ERROR (constructor))
    {
      return constructor;
    }

    /* The this_arg must be an object. */
    bool is_same_value = (constructor == this_arg);
    ecma_free_value (constructor);

    if (is_same_value)
    {
      return ecma_copy_value (value);
    }
  }

  ecma_value_t capability = ecma_promise_new_capability (this_arg);

  if (ECMA_IS_VALUE_ERROR (capability))
  {
    return capability;
  }

  ecma_string_t *property_str_p;

  if (is_resolve)
  {
    property_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_RESOLVE);
  }
  else
  {
    property_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_REJECT);
  }

  ecma_value_t func = ecma_op_object_get (ecma_get_object_from_value (capability), property_str_p);

  ecma_value_t call_ret = ecma_op_function_call (ecma_get_object_from_value (func),
                                                 ECMA_VALUE_UNDEFINED,
                                                 &value,
                                                 1);

  ecma_free_value (func);

  if (ECMA_IS_VALUE_ERROR (call_ret))
  {
    return call_ret;
  }

  ecma_free_value (call_ret);

  ecma_string_t *promise_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_PROMISE);
  ecma_value_t promise = ecma_op_object_get (ecma_get_object_from_value (capability), promise_str_p);
  ecma_free_value (capability);

  return promise;
} /* ecma_promise_reject_or_resolve */

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
  ecma_string_t *promise_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_PROMISE);

  /* 3. boolean true indicates "indentity" */
  if (!ecma_op_is_callable (on_fulfilled))
  {
    on_fulfilled = ECMA_VALUE_TRUE;
  }

  /* 4. boolean false indicates "thrower" */
  if (!ecma_op_is_callable (on_rejected))
  {
    on_rejected = ECMA_VALUE_FALSE;
  }

  ecma_object_t *promise_obj_p = ecma_get_object_from_value (promise);
  ecma_promise_object_t *promise_p = (ecma_promise_object_t *) promise_obj_p;

  uint16_t flags = ecma_promise_get_flags (promise_obj_p);

  if (flags & ECMA_PROMISE_IS_PENDING)
  {
    /* 7. */
    ecma_value_t capability_with_tag;
    ECMA_SET_NON_NULL_POINTER_TAG (capability_with_tag, ecma_get_object_from_value (result_capability), 0);

    if (on_fulfilled != ECMA_VALUE_TRUE)
    {
      ECMA_SET_FIRST_BIT_TO_POINTER_TAG (capability_with_tag);
    }

    if (on_rejected != ECMA_VALUE_FALSE)
    {
      ECMA_SET_SECOND_BIT_TO_POINTER_TAG (capability_with_tag);
    }

    ecma_collection_push_back (promise_p->reactions, capability_with_tag);

    if (on_fulfilled != ECMA_VALUE_TRUE)
    {
      ecma_collection_push_back (promise_p->reactions, on_fulfilled);
    }

    if (on_rejected != ECMA_VALUE_FALSE)
    {
      ecma_collection_push_back (promise_p->reactions, on_rejected);
    }
  }
  else if (flags & ECMA_PROMISE_IS_FULFILLED)
  {
    /* 8. */
    ecma_value_t value = ecma_promise_get_result (promise_obj_p);
    ecma_enqueue_promise_reaction_job (result_capability, on_fulfilled, value);
    ecma_free_value (value);
  }
  else
  {
    /* 9. */
    ecma_value_t reason = ecma_promise_get_result (promise_obj_p);
    ecma_enqueue_promise_reaction_job (result_capability, on_rejected, reason);
    ecma_free_value (reason);
  }

  /* 10. */
  return ecma_op_object_get (ecma_get_object_from_value (result_capability), promise_str_p);
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

  ecma_value_t species = ecma_op_species_constructor (obj, ECMA_BUILTIN_ID_PROMISE);
  if (ECMA_IS_VALUE_ERROR (species))
  {
    return species;
  }

  ecma_value_t result_capability = ecma_promise_new_capability (species);
  ecma_free_value (species);

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
#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROMISE) */
