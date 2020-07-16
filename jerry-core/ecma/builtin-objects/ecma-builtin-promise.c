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
#include "ecma-array-object.h"
#include "ecma-builtin-helpers.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-iterator-object.h"
#include "ecma-number-object.h"
#include "ecma-promise-object.h"
#include "jcontext.h"

#if ENABLED (JERRY_BUILTIN_PROMISE)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-promise.inc.h"
#define BUILTIN_UNDERSCORED_ID promise
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup promise ECMA Promise object built-in
 * @{
 */

/**
 * Reject the promise if the value is error.
 *
 * See also:
 *         ES2015 25.4.1.1.1
 *
 * @return ecma value of the new promise.
 *         Returned value must be freed with ecma_free_value.
 */
inline static ecma_value_t
ecma_builtin_promise_reject_abrupt (ecma_value_t value, /**< value */
                                    ecma_object_t *capability_obj_p) /**< capability */
{
  JERRY_ASSERT (ecma_object_class_is (capability_obj_p, LIT_INTERNAL_MAGIC_PROMISE_CAPABILITY));

  if (!ECMA_IS_VALUE_ERROR (value))
  {
    return value;
  }

  ecma_value_t reason = jcontext_take_exception ();

  ecma_promise_capabality_t *capability_p = (ecma_promise_capabality_t *) capability_obj_p;
  ecma_value_t call_ret = ecma_op_function_call (ecma_get_object_from_value (capability_p->reject),
                                                 ECMA_VALUE_UNDEFINED,
                                                 &reason,
                                                 1);
  ecma_free_value (reason);

  if (ECMA_IS_VALUE_ERROR (call_ret))
  {
    return call_ret;
  }

  ecma_free_value (call_ret);

  return ecma_copy_value (capability_p->header.u.class_prop.u.promise);
} /* ecma_builtin_promise_reject_abrupt */

/**
 * The Promise.reject routine.
 *
 * See also:
 *         ES2015 25.4.4.4
 *
 * @return ecma value of the new promise.
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_promise_reject (ecma_value_t this_arg, /**< 'this' argument */
                             ecma_value_t reason) /**< the reason for reject */
{
  return ecma_promise_reject_or_resolve (this_arg, reason, false);
} /* ecma_builtin_promise_reject */

/**
 * The Promise.resolve routine.
 *
 * See also:
 *         ES2015 25.4.4.5
 *
 * @return ecma value of the new promise.
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_promise_resolve (ecma_value_t this_arg, /**< 'this' argument */
                              ecma_value_t argument) /**< the argument for resolve */
{
  return ecma_promise_reject_or_resolve (this_arg, argument, true);
} /* ecma_builtin_promise_resolve */

/**
 * Runtime Semantics: PerformPromiseRace.
 *
 * See also:
 *         ES2015 25.4.4.3.1
 *
 * @return ecma value of the new promise.
 *         Returned value must be freed with ecma_free_value.
 */
inline static ecma_value_t
ecma_builtin_promise_perform_race (ecma_value_t iterator, /**< the iterator for race */
                                   ecma_value_t next_method, /**< next method */
                                   ecma_object_t *capability_obj_p, /**< PromiseCapability record */
                                   ecma_value_t ctor, /**< Constructor value */
                                   bool *done_p) /**< [out] iteratorRecord[[done]] */
{
  JERRY_ASSERT (ecma_is_value_object (iterator));
  JERRY_ASSERT (ecma_object_class_is (capability_obj_p, LIT_INTERNAL_MAGIC_PROMISE_CAPABILITY));

  ecma_promise_capabality_t *capability_p = (ecma_promise_capabality_t *) capability_obj_p;

  /* 1. */
  while (true)
  {
    /* a. */
    ecma_value_t next = ecma_op_iterator_step (iterator, next_method);
    /* b, c. */
    if (ECMA_IS_VALUE_ERROR (next))
    {
      *done_p = true;
      return next;
    }

    /* d. */
    if (ecma_is_value_false (next))
    {
      /* i. */
      *done_p = true;
      /* ii. */
      return ecma_copy_value (capability_p->header.u.class_prop.u.promise);
    }

    /* e. */
    ecma_value_t next_val = ecma_op_iterator_value (next);
    ecma_free_value (next);

    /* f, g. */
    if (ECMA_IS_VALUE_ERROR (next_val))
    {
      *done_p = true;
      return next_val;
    }

    /* h. */
    ecma_value_t next_promise = ecma_op_invoke_by_magic_id (ctor, LIT_MAGIC_STRING_RESOLVE, &next_val, 1);
    ecma_free_value (next_val);

    /* i. */
    if (ECMA_IS_VALUE_ERROR (next_promise))
    {
      return next_promise;
    }

    /* j. */
    ecma_value_t args[2] = {capability_p->resolve, capability_p->reject};
    ecma_value_t result = ecma_op_invoke_by_magic_id (next_promise, LIT_MAGIC_STRING_THEN, args, 2);
    ecma_free_value (next_promise);

    /* k. */
    if (ECMA_IS_VALUE_ERROR (result))
    {
      return result;
    }

    ecma_free_value (result);
  }

  JERRY_UNREACHABLE ();
} /* ecma_builtin_promise_perform_race */

/**
 * Runtime Semantics: PerformPromiseAll.
 *
 * See also:
 *         ES2015 25.4.4.1.1
 *
 * @return ecma value of the new promise.
 *         Returned value must be freed with ecma_free_value.
 */
inline static ecma_value_t
ecma_builtin_promise_perform_all (ecma_value_t iterator, /**< iteratorRecord */
                                  ecma_value_t next_method, /**< next method */
                                  ecma_object_t *capability_obj_p,  /**< PromiseCapability record */
                                  ecma_value_t ctor, /**< the caller of Promise.race */
                                  bool *done_p) /**< [out] iteratorRecord[[done]] */
{
  /* 1. - 2. */
  JERRY_ASSERT (ecma_object_class_is (capability_obj_p, LIT_INTERNAL_MAGIC_PROMISE_CAPABILITY));
  JERRY_ASSERT (ecma_is_constructor (ctor));

  ecma_promise_capabality_t *capability_p = (ecma_promise_capabality_t *) capability_obj_p;

  /* 3. */
  ecma_object_t *values_array_obj_p = ecma_op_new_fast_array_object (0);
  ecma_value_t values_array = ecma_make_object_value (values_array_obj_p);
  /* 4. */
  ecma_value_t remaining = ecma_op_create_number_object (ecma_make_integer_value (1));
  /* 5. */
  uint32_t idx = 0;

  ecma_value_t ret_value = ECMA_VALUE_ERROR;

  /* 6. */
  while (true)
  {
    /* a. */
    ecma_value_t next = ecma_op_iterator_step (iterator, next_method);
    /* b. - c. */
    if (ECMA_IS_VALUE_ERROR (next))
    {
      *done_p = true;
      break;
    }

    /* d. */
    if (ecma_is_value_false (next))
    {
      /* i. */
      *done_p = true;

      /* ii. - iii. */
      if (ecma_promise_remaining_inc_or_dec (remaining, false) == 0)
      {
        /* 2. */
        ecma_value_t resolve_result = ecma_op_function_call (ecma_get_object_from_value (capability_p->resolve),
                                                             ECMA_VALUE_UNDEFINED,
                                                             &values_array,
                                                             1);
        /* 3. */
        if (ECMA_IS_VALUE_ERROR (resolve_result))
        {
          break;
        }

        ecma_free_value (resolve_result);
      }

      /* iv. */
      ret_value = ecma_copy_value (capability_p->header.u.class_prop.u.promise);
      break;
    }

    /* e. */
    ecma_value_t next_value = ecma_op_iterator_value (next);
    ecma_free_value (next);

    /* f. - g. */
    if (ECMA_IS_VALUE_ERROR (next_value))
    {
      *done_p = true;
      break;
    }

    /* h. */
    ecma_builtin_helper_def_prop_by_index (values_array_obj_p,
                                           idx,
                                           ECMA_VALUE_UNDEFINED,
                                           ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);

    /* i. */
    ecma_value_t next_promise = ecma_op_invoke_by_magic_id (ctor, LIT_MAGIC_STRING_RESOLVE, &next_value, 1);
    ecma_free_value (next_value);

    /* j. */
    if (ECMA_IS_VALUE_ERROR (next_promise))
    {
      break;
    }

    /* k. */
    ecma_object_t *executor_func_p = ecma_create_object (ecma_builtin_get (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE),
                                                         sizeof (ecma_promise_all_executor_t),
                                                         ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);

    ecma_promise_all_executor_t *executor_p = (ecma_promise_all_executor_t *) executor_func_p;
    executor_p->header.u.external_handler_cb = ecma_promise_all_handler_cb;

    /* l. */
    if (JERRY_UNLIKELY (idx == UINT32_MAX - 1))
    {
      ecma_deref_object (executor_func_p);
      ecma_raise_range_error (ECMA_ERR_MSG ("Promise.all remaining elements limit reached."));
      break;
    }

    /* m. + t. */
    executor_p->index = ++idx;

    /* n. */
    executor_p->values = values_array;

    /* o. */
    executor_p->capability = ecma_make_object_value (capability_obj_p);

    /* p. */
    executor_p->remaining_elements = remaining;

    /* q. */
    ecma_promise_remaining_inc_or_dec (remaining, true);

    /* r. */
    ecma_value_t args[2];
    args[0] = ecma_make_object_value (executor_func_p);
    args[1] = capability_p->reject;
    ecma_value_t result = ecma_op_invoke_by_magic_id (next_promise, LIT_MAGIC_STRING_THEN, args, 2);
    ecma_free_value (next_promise);
    ecma_deref_object (executor_func_p);

    /* s. */
    if (ECMA_IS_VALUE_ERROR (result))
    {
      break;
    }

    ecma_free_value (result);
  }

  ecma_free_value (remaining);
  ecma_deref_object (values_array_obj_p);

  return ret_value;
} /* ecma_builtin_promise_perform_all */

/**
 * The common function for both Promise.race and Promise.all.
 *
 * @return ecma value of the new promise.
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_promise_race_or_all (ecma_value_t this_arg, /**< 'this' argument */
                                  ecma_value_t iterable, /**< the items to be resolved */
                                  bool is_race) /**< indicates whether it is race function */
{
  if (!ecma_is_value_object (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("'this' is not an object."));
  }

  ecma_object_t *capability_obj_p = ecma_promise_new_capability (this_arg);

  if (JERRY_UNLIKELY (capability_obj_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t next_method;
  ecma_value_t iterator = ecma_op_get_iterator (iterable, ECMA_VALUE_SYNC_ITERATOR, &next_method);
  iterator = ecma_builtin_promise_reject_abrupt (iterator, capability_obj_p);

  if (ECMA_IS_VALUE_ERROR (iterator))
  {
    ecma_deref_object (capability_obj_p);
    return iterator;
  }

  ecma_value_t ret = ECMA_VALUE_EMPTY;
  bool is_done = false;

  if (is_race)
  {
    ret = ecma_builtin_promise_perform_race (iterator, next_method, capability_obj_p, this_arg, &is_done);
  }
  else
  {
    ret = ecma_builtin_promise_perform_all (iterator, next_method, capability_obj_p, this_arg, &is_done);
  }

  if (ECMA_IS_VALUE_ERROR (ret))
  {
    if (!is_done)
    {
      ret = ecma_op_iterator_close (iterator);
    }

    ret = ecma_builtin_promise_reject_abrupt (ret, capability_obj_p);
  }

  ecma_free_value (iterator);
  ecma_free_value (next_method);
  ecma_deref_object (capability_obj_p);

  return ret;
} /* ecma_builtin_promise_race_or_all */

/**
 * The Promise.race routine.
 *
 * See also:
 *         ES2015 25.4.4.3
 *
 * @return ecma value of the new promise.
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_promise_race (ecma_value_t this_arg, /**< 'this' argument */
                           ecma_value_t array) /**< the items to be resolved */
{
  return ecma_builtin_promise_race_or_all (this_arg, array, true);
} /* ecma_builtin_promise_race */

/**
 * The Promise.all routine.
 *
 * See also:
 *         ES2015 25.4.4.1
 *
 * @return ecma value of the new promise.
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_promise_all (ecma_value_t this_arg, /**< 'this' argument */
                           ecma_value_t array) /**< the items to be resolved */
{
  return ecma_builtin_promise_race_or_all (this_arg, array, false);
} /* ecma_builtin_promise_all */

/**
 * Handle calling [[Call]] of built-in Promise object.
 *
 * ES2015 25.4.3 Promise is not intended to be called
 * as a function and will throw an exception when called
 * in that manner.
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_promise_dispatch_call (const ecma_value_t *arguments_list_p, /**< arguments list */
                                    uint32_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_raise_type_error (ECMA_ERR_MSG ("Constructor Promise requires 'new'."));
} /* ecma_builtin_promise_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in Promise object.
 *
 * @return ecma value
 */
ecma_value_t
ecma_builtin_promise_dispatch_construct (const ecma_value_t *arguments_list_p, /**< arguments list */
                                         uint32_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  if (arguments_list_len == 0 || !ecma_op_is_callable (arguments_list_p[0]))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("First parameter must be callable."));
  }

  return ecma_op_create_promise_object (arguments_list_p[0], ECMA_PROMISE_EXECUTOR_FUNCTION);
} /* ecma_builtin_promise_dispatch_construct */

/**
 * 25.4.4.6 get Promise [ @@species ] accessor
 *
 * @return ecma_value
 *         returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_builtin_promise_species_get (ecma_value_t this_value) /**< This Value */
{
  return ecma_copy_value (this_value);
} /* ecma_builtin_promise_species_get */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_BUILTIN_PROMISE) */
