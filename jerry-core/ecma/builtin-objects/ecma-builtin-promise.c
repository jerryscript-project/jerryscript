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

#if ENABLED (JERRY_ES2015_BUILTIN_PROMISE)

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
                                    ecma_value_t capability) /**< capability */
{
  if (!ECMA_IS_VALUE_ERROR (value))
  {
    return value;
  }

  ecma_value_t reason = jcontext_take_exception ();
  ecma_value_t reject = ecma_op_object_get_by_magic_id (ecma_get_object_from_value (capability),
                                                        LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_REJECT);

  ecma_value_t call_ret = ecma_op_function_call (ecma_get_object_from_value (reject),
                                                 ECMA_VALUE_UNDEFINED,
                                                 &reason,
                                                 1);
  ecma_free_value (reject);
  ecma_free_value (reason);

  if (ECMA_IS_VALUE_ERROR (call_ret))
  {
    return call_ret;
  }

  ecma_free_value (call_ret);

  return ecma_op_object_get_by_magic_id (ecma_get_object_from_value (capability),
                                         LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_PROMISE);
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
                                   ecma_value_t capability, /**< PromiseCapability record */
                                   ecma_value_t ctor, /**< Constructor value */
                                   bool *done_p) /**< [out] iteratorRecord[[done]] */
{
  JERRY_ASSERT (ecma_is_value_object (iterator)
                && ecma_is_value_object (capability));

  ecma_object_t *capability_obj_p = ecma_get_object_from_value (capability);
  /* 1. */
  while (true)
  {
    /* a. */
    ecma_value_t next = ecma_op_iterator_step (iterator);
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
      return ecma_op_object_get_by_magic_id (capability_obj_p, LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_PROMISE);
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
    ecma_value_t args[2];
    args[0] = ecma_op_object_get_by_magic_id (capability_obj_p, LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_RESOLVE);
    args[1] = ecma_op_object_get_by_magic_id (capability_obj_p, LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_REJECT);
    ecma_value_t result = ecma_op_invoke_by_magic_id (next_promise, LIT_MAGIC_STRING_THEN, args, 2);
    ecma_free_value (next_promise);

    for (uint8_t i = 0; i < 2; i++)
    {
      ecma_free_value (args[i]);
    }

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
 * Helper function for increase or decrease the remaining count.
 *
 * @return the current remaining count after increase or decrease.
 */
static ecma_length_t
ecma_builtin_promise_remaining_inc_or_dec (ecma_value_t remaining, /**< the remaining count */
                                           bool is_inc) /**< whether to increase the count */
{
  JERRY_ASSERT (ecma_is_value_object (remaining));

  ecma_object_t *remaining_p = ecma_get_object_from_value (remaining);
  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) remaining_p;

  JERRY_ASSERT (ext_object_p->u.class_prop.class_id == LIT_MAGIC_STRING_NUMBER_UL);

  JERRY_ASSERT (ecma_is_value_integer_number (ext_object_p->u.class_prop.u.value));

  ecma_length_t current = (ecma_length_t) ecma_get_integer_from_value (ext_object_p->u.class_prop.u.value);

  if (is_inc)
  {
    current++;
  }
  else
  {
    current--;
  }
  ext_object_p->u.class_prop.u.value = ecma_make_uint32_value (current);

  return current;
} /* ecma_builtin_promise_remaining_inc_or_dec */

/**
 * Native handler for Promise.all Resolve Element Function.
 *
 * See also:
 *         ES2015 25.4.4.1.2
 *
 * @return ecma value of undefined.
 */
static ecma_value_t
ecma_builtin_promise_all_handler (const ecma_value_t function, /**< the function itself */
                                  const ecma_value_t this, /**< this_arg of the function */
                                  const ecma_value_t argv[], /**< argument list */
                                  const ecma_length_t argc) /**< argument number */
{
  JERRY_UNUSED (this);
  JERRY_UNUSED (argc);

  /* 1. */
  ecma_object_t *function_p = ecma_get_object_from_value (function);
  ecma_string_t *already_called_str_p;
  already_called_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_ALREADY_CALLED);
  ecma_value_t already_called = ecma_op_object_get (function_p, already_called_str_p);

  JERRY_ASSERT (ecma_is_value_boolean (already_called));

  /* 2. */
  if (ecma_is_value_true (already_called))
  {
    return ECMA_VALUE_UNDEFINED;
  }

  /* 3. */
  ecma_op_object_put (function_p,
                      already_called_str_p,
                      ECMA_VALUE_TRUE,
                      false);

  ecma_string_t *str_index_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_INDEX);
  ecma_string_t *str_value_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_VALUE);
  ecma_string_t *str_capability_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_CAPABILITY);
  ecma_string_t *str_remaining_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_REMAINING_ELEMENT);

  /* 4-7. */
  ecma_value_t index_val = ecma_op_object_get (function_p, str_index_p);
  ecma_value_t values_array = ecma_op_object_get (function_p, str_value_p);
  ecma_value_t capability = ecma_op_object_get (function_p, str_capability_p);
  ecma_value_t remaining = ecma_op_object_get (function_p, str_remaining_p);

  JERRY_ASSERT (ecma_is_value_integer_number (index_val));

  /* 8. */
  ecma_op_object_put_by_uint32_index (ecma_get_object_from_value (values_array),
                                      (uint32_t) ecma_get_integer_from_value (index_val),
                                      argv[0],
                                      false);

  /* 9-10. */
  ecma_value_t ret = ECMA_VALUE_UNDEFINED;
  if (ecma_builtin_promise_remaining_inc_or_dec (remaining, false) == 0)
  {
    ecma_value_t resolve = ecma_op_object_get_by_magic_id (ecma_get_object_from_value (capability),
                                                           LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_RESOLVE);
    ret = ecma_op_function_call (ecma_get_object_from_value (resolve),
                                 ECMA_VALUE_UNDEFINED,
                                 &values_array,
                                 1);
    ecma_free_value (resolve);
  }

  ecma_free_value (remaining);
  ecma_free_value (capability);
  ecma_free_value (values_array);
  ecma_free_value (index_val);

  return ret;
} /* ecma_builtin_promise_all_handler */

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
                                  ecma_value_t ctor, /**< the caller of Promise.race */
                                  ecma_value_t capability,  /**< PromiseCapability record */
                                  bool *done_p) /**< [out] iteratorRecord[[done]] */
{
  /* 1. - 2. */
  JERRY_ASSERT (ecma_is_value_object (capability) && ecma_is_constructor (ctor));

  /* 3. */
  ecma_object_t *values_array_obj_p = ecma_op_new_fast_array_object (0);
  ecma_value_t values_array = ecma_make_object_value (values_array_obj_p);
  /* 4. */
  ecma_value_t remaining = ecma_op_create_number_object (ecma_make_integer_value (1));
  /* 5. */
  uint32_t idx = 0;

  ecma_value_t ret_value = ECMA_VALUE_ERROR;
  ecma_object_t *capability_obj_p = ecma_get_object_from_value (capability);

  ecma_string_t *already_called_str_p;
  already_called_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_ALREADY_CALLED);
  ecma_string_t *index_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_INDEX);
  ecma_string_t *value_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_VALUE);
  ecma_string_t *capability_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_CAPABILITY);
  ecma_string_t *remaining_str_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_REMAINING_ELEMENT);

  /* 6. */
  while (true)
  {
    /* a. */
    ecma_value_t next = ecma_op_iterator_step (iterator);
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
      if (ecma_builtin_promise_remaining_inc_or_dec (remaining, false) == 0)
      {
        /* 2. */
        ecma_value_t resolve = ecma_op_object_get_by_magic_id (capability_obj_p,
                                                               LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_RESOLVE);
        ecma_value_t resolve_result = ecma_op_function_call (ecma_get_object_from_value (resolve),
                                                             ECMA_VALUE_UNDEFINED,
                                                             &values_array,
                                                             1);
        ecma_free_value (resolve);

        /* 3. */
        if (ECMA_IS_VALUE_ERROR (resolve_result))
        {
          break;
        }

        ecma_free_value (resolve_result);
      }

      /* iv. */
      ret_value = ecma_op_object_get_by_magic_id (capability_obj_p,
                                                  LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_PROMISE);
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
    ecma_object_t *res_ele_p;
    res_ele_p = ecma_op_create_external_function_object (ecma_builtin_promise_all_handler);
    /* l. */
    ecma_op_object_put (res_ele_p,
                        already_called_str_p,
                        ECMA_VALUE_FALSE,
                        false);
    /* m. */
    ecma_value_t idx_value = ecma_make_uint32_value (idx);
    ecma_op_object_put (res_ele_p,
                        index_str_p,
                        idx_value,
                        false);
    ecma_free_value (idx_value);

    /* n. */
    ecma_op_object_put (res_ele_p,
                        value_str_p,
                        values_array,
                        false);

    /* o. */
    ecma_op_object_put (res_ele_p,
                        capability_str_p,
                        capability,
                        false);
    /* p. */
    ecma_op_object_put (res_ele_p,
                        remaining_str_p,
                        remaining,
                        false);

    /* q. */
    ecma_builtin_promise_remaining_inc_or_dec (remaining, true);

    /* r. */
    ecma_value_t args[2];
    args[0] = ecma_make_object_value (res_ele_p);
    args[1] = ecma_op_object_get_by_magic_id (capability_obj_p, LIT_INTERNAL_MAGIC_STRING_PROMISE_PROPERTY_REJECT);
    ecma_value_t result = ecma_op_invoke_by_magic_id (next_promise, LIT_MAGIC_STRING_THEN, args, 2);
    ecma_free_value (next_promise);

    for (uint8_t i = 0; i < 2; i++)
    {
      ecma_free_value (args[i]);
    }

    /* s. */
    if (ECMA_IS_VALUE_ERROR (result))
    {
      break;
    }

    ecma_free_value (result);

    /* t. */
    idx++;
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

  ecma_object_t *this_obj_p = ecma_get_object_from_value (this_arg);
  ecma_value_t species_symbol = ecma_op_object_get_by_magic_id (this_obj_p,
                                                                LIT_MAGIC_STRING_SYMBOL);

  if (ECMA_IS_VALUE_ERROR (species_symbol))
  {
    return species_symbol;
  }

  ecma_value_t constructor_value = this_arg;

  if (!ecma_is_value_null (species_symbol) && !ecma_is_value_undefined (species_symbol))
  {
    constructor_value = species_symbol;
  }
  else
  {
    ecma_ref_object (this_obj_p);
  }

  ecma_value_t capability = ecma_promise_new_capability (constructor_value);

  if (ECMA_IS_VALUE_ERROR (capability))
  {
    ecma_free_value (constructor_value);
    return capability;
  }

  ecma_value_t iterator = ecma_builtin_promise_reject_abrupt (ecma_op_get_iterator (iterable, ECMA_VALUE_EMPTY),
                                                              capability);

  if (ECMA_IS_VALUE_ERROR (iterator))
  {
    ecma_free_value (constructor_value);
    ecma_free_value (capability);
    return iterator;
  }

  ecma_value_t ret = ECMA_VALUE_EMPTY;
  bool is_done = false;

  if (is_race)
  {
    ret = ecma_builtin_promise_perform_race (iterator, capability, constructor_value, &is_done);
  }
  else
  {
    ret = ecma_builtin_promise_perform_all (iterator, constructor_value, capability, &is_done);
  }

  ecma_free_value (constructor_value);

  if (ECMA_IS_VALUE_ERROR (ret))
  {
    if (!is_done)
    {
      ret = ecma_op_iterator_close (iterator);
    }

    ret = ecma_builtin_promise_reject_abrupt (ret, capability);
  }

  ecma_free_value (iterator);
  ecma_free_value (capability);

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
                                    ecma_length_t arguments_list_len) /**< number of arguments */
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
                                         ecma_length_t arguments_list_len) /**< number of arguments */
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

#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROMISE) */
