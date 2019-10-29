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

#include <math.h>

#include "ecma-arraybuffer-object.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtin-typedarray-helpers.h"
#include "ecma-builtins.h"
#include "ecma-comparison.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-iterator-object.h"
#include "ecma-number-object.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"
#include "ecma-typedarray-object.h"
#include "jcontext.h"
#include "jmem.h"
#include "jrt-libc-includes.h"
#include "jrt.h"
#include "lit-char-helpers.h"

#if ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-typedarray-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID typedarray_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup typedarrayprototype ECMA %TypedArray%.prototype object built-in
 * @{
 */

/**
 * The %TypedArray%.prototype.buffer accessor
 *
 * See also:
 *          ES2015, 22.2.3.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_buffer_getter (ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_typedarray (this_arg))
  {
    ecma_object_t *typedarray_p = ecma_get_object_from_value (this_arg);
    ecma_object_t *obj_p = ecma_typedarray_get_arraybuffer (typedarray_p);
    ecma_ref_object (obj_p);

    return ecma_make_object_value (obj_p);
  }

  return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
} /* ecma_builtin_typedarray_prototype_buffer_getter */

/**
 * The %TypedArray%.prototype.byteLength accessor
 *
 * See also:
 *          ES2015, 22.2.3.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_bytelength_getter (ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_typedarray (this_arg))
  {
    ecma_object_t *typedarray_p = ecma_get_object_from_value (this_arg);
    uint8_t shift = ecma_typedarray_get_element_size_shift (typedarray_p);

    return ecma_make_uint32_value (ecma_typedarray_get_length (typedarray_p) << shift);
  }

  return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
} /* ecma_builtin_typedarray_prototype_bytelength_getter */

/**
 * The %TypedArray%.prototype.byteOffset accessor
 *
 * See also:
 *          ES2015, 22.2.3.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_byteoffset_getter (ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_typedarray (this_arg))
  {
    ecma_object_t *typedarray_p = ecma_get_object_from_value (this_arg);

    return ecma_make_uint32_value (ecma_typedarray_get_offset (typedarray_p));
  }

  return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
} /* ecma_builtin_typedarray_prototype_byteoffset_getter */

/**
 * The %TypedArray%.prototype.length accessor
 *
 * See also:
 *          ES2015, 22.2.3.17
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_length_getter (ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_typedarray (this_arg))
  {
    ecma_object_t *typedarray_p = ecma_get_object_from_value (this_arg);

    return ecma_make_uint32_value (ecma_typedarray_get_length (typedarray_p));
  }

  return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
} /* ecma_builtin_typedarray_prototype_length_getter */

#if ENABLED (JERRY_ES2015)
/**
 * The %TypedArray%.prototype[Symbol.toStringTag] accessor
 *
 * See also:
 *          ES2015, 22.2.3.31
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_to_string_tag_getter (ecma_value_t this_arg) /**< this argument */
{
  if (!ecma_is_typedarray (this_arg))
  {
    return ECMA_VALUE_UNDEFINED;
  }

  return ecma_make_magic_string_value (ecma_object_get_class_name (ecma_get_object_from_value (this_arg)));
} /* ecma_builtin_typedarray_prototype_to_string_tag_getter */
#endif /* ENABLED (JERRY_ES2015) */

/**
 * Type of routine.
 */
typedef enum
{
  TYPEDARRAY_ROUTINE_EVERY, /**< routine: every ES2015, 22.2.3.7 */
  TYPEDARRAY_ROUTINE_SOME, /**< routine: some ES2015, 22.2.3.9 */
  TYPEDARRAY_ROUTINE_FOREACH, /**< routine: forEach ES2015, 15.4.4.18 */
  TYPEDARRAY_ROUTINE__COUNT /**< count of the modes */
} typedarray_routine_mode;

/**
 * The common function for 'every', 'some' and 'forEach'
 * because they have a similar structure.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_exec_routine (ecma_value_t this_arg, /**< this argument */
                                                ecma_value_t cb_func_val, /**< callback function */
                                                ecma_value_t cb_this_arg, /**< 'this' of the callback function */
                                                typedarray_routine_mode mode) /**< mode: which routine */
{
  JERRY_ASSERT (mode < TYPEDARRAY_ROUTINE__COUNT);

  if (!ecma_is_typedarray (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
  }

  if (!ecma_op_is_callable (cb_func_val))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (this_arg);
  ecma_typedarray_info_t info = ecma_typedarray_get_info (obj_p);
  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (obj_p);
  if (ecma_arraybuffer_is_detached (arraybuffer_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("ArrayBuffer has been detached."));
  }

  ecma_typedarray_getter_fn_t typedarray_getter_cb = ecma_get_typedarray_getter_fn (info.id);

  ecma_object_t *func_object_p = ecma_get_object_from_value (cb_func_val);
  ecma_length_t byte_pos = 0;
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  for (uint32_t index = 0; index < info.length && ecma_is_value_empty (ret_value); index++)
  {
    ecma_value_t current_index = ecma_make_uint32_value (index);
    ecma_number_t element_num = typedarray_getter_cb (info.buffer_p + byte_pos);
    ecma_value_t get_value = ecma_make_number_value (element_num);

    ecma_value_t call_args[] = { get_value, current_index, this_arg };

    ecma_value_t call_value = ecma_op_function_call (func_object_p, cb_this_arg, call_args, 3);

    ecma_fast_free_value (current_index);
    ecma_fast_free_value (get_value);

    if (ECMA_IS_VALUE_ERROR (call_value))
    {
      return call_value;
    }

    bool to_bool_result = ecma_op_to_boolean (call_value);
    ecma_free_value (call_value);

    if (mode == TYPEDARRAY_ROUTINE_EVERY)
    {
      if (!to_bool_result)
      {
        return ECMA_VALUE_FALSE;
      }
    }
    else if (mode == TYPEDARRAY_ROUTINE_SOME
             && to_bool_result)
    {
      return ECMA_VALUE_TRUE;
    }

    byte_pos += info.element_size;
  }

  if (mode == TYPEDARRAY_ROUTINE_EVERY)
  {
    ret_value = ECMA_VALUE_TRUE;
  }
  else if (mode == TYPEDARRAY_ROUTINE_SOME)
  {
    ret_value = ECMA_VALUE_FALSE;
  }
  else
  {
    ret_value = ECMA_VALUE_UNDEFINED;
  }

  return ret_value;
} /* ecma_builtin_typedarray_prototype_exec_routine */

/**
 * The %TypedArray%.prototype object's 'every' routine
 *
 * See also:
 *          ES2015, 22.2.3.7
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_every (ecma_value_t this_arg, /**< this argument */
                                         ecma_value_t cb_func_val, /**< callback function */
                                         ecma_value_t cb_this_arg) /**< this' of the callback function */
{
  return ecma_builtin_typedarray_prototype_exec_routine (this_arg,
                                                         cb_func_val,
                                                         cb_this_arg,
                                                         TYPEDARRAY_ROUTINE_EVERY);
} /* ecma_builtin_typedarray_prototype_every */

/**
 * The %TypedArray%.prototype object's 'some' routine
 *
 * See also:
 *          ES2015, 22.2.3.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_some (ecma_value_t this_arg, /**< this argument */
                                        ecma_value_t cb_func_val, /**< callback function */
                                        ecma_value_t cb_this_arg) /**< this' of the callback function */
{
  return ecma_builtin_typedarray_prototype_exec_routine (this_arg,
                                                         cb_func_val,
                                                         cb_this_arg,
                                                         TYPEDARRAY_ROUTINE_SOME);
} /* ecma_builtin_typedarray_prototype_some */

/**
 * The %TypedArray%.prototype object's 'forEach' routine
 *
 * See also:
 *          ES2015, 15.4.4.18
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_for_each (ecma_value_t this_arg, /**< this argument */
                                            ecma_value_t cb_func_val, /**< callback function */
                                            ecma_value_t cb_this_arg) /**< this' of the callback function */
{
  return ecma_builtin_typedarray_prototype_exec_routine (this_arg,
                                                         cb_func_val,
                                                         cb_this_arg,
                                                         TYPEDARRAY_ROUTINE_FOREACH);
} /* ecma_builtin_typedarray_prototype_for_each */


#if ENABLED (JERRY_ES2015_BUILTIN_ITERATOR)
/**
 * Helper function for typedArray.prototype object's {'keys', 'values', 'entries', '@@iterator'}
 * routines common parts.
 *
 * See also:
 *          ECMA-262 v6, 22.2.3.15
 *          ECMA-262 v6, 22.2.3.29
 *          ECMA-262 v6, 22.2.3.6
 *          ECMA-262 v6, 22.1.3.30
 *
 * Note:
 *      Returned value must be freed with ecma_free_value.
 *
 * @return iterator result object, if success
 *         error - otherwise
 */
static ecma_value_t
ecma_builtin_typedarray_iterators_helper (ecma_value_t this_arg, /**< this argument */
                                          uint8_t type) /**< any combination of ecma_iterator_type_t bits */
{
  if (!ecma_is_typedarray (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
  }

  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_ARRAY_ITERATOR_PROTOTYPE);

  return ecma_op_create_iterator_object (this_arg,
                                         prototype_obj_p,
                                         ECMA_PSEUDO_ARRAY_ITERATOR,
                                         type);
} /* ecma_builtin_typedarray_iterators_helper */

/**
 * The %TypedArray%.prototype object's 'keys' routine
 *
 * See also:
 *          ES2015, 22.2.3.15
 *          ES2015, 22.1.3.30
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_keys (ecma_value_t this_arg) /**< this argument */
{
  return ecma_builtin_typedarray_iterators_helper (this_arg, ECMA_ITERATOR_KEYS);
} /* ecma_builtin_typedarray_prototype_keys */

/**
 * The %TypedArray%.prototype object's 'values' and @@iterator routines
 *
 * See also:
 *          ES2015, 22.2.3.29
 *          ES2015, 22.1.3.30
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_values (ecma_value_t this_arg) /**< this argument */
{
  return ecma_builtin_typedarray_iterators_helper (this_arg, ECMA_ITERATOR_VALUES);
} /* ecma_builtin_typedarray_prototype_values */

/**
 * The %TypedArray%.prototype object's 'entries' routine
 *
 * See also:
 *          ES2015, 22.2.3.6
 *          ES2015, 22.1.3.30
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_entries (ecma_value_t this_arg) /**< this argument */
{
  return ecma_builtin_typedarray_iterators_helper (this_arg, ECMA_ITERATOR_KEYS_VALUES);
} /* ecma_builtin_typedarray_prototype_entries */

#endif /* ENABLED (JERRY_ES2015_BUILTIN_ITERATOR) */

/**
 * The %TypedArray%.prototype object's 'map' routine
 *
 * See also:
 *          ES2015, 22.2.3.8
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_map (ecma_value_t this_arg, /**< this argument */
                                       ecma_value_t cb_func_val, /**< callback function */
                                       ecma_value_t cb_this_arg) /**< this' of the callback function */
{
  if (!ecma_is_typedarray (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
  }

  if (!ecma_op_is_callable (cb_func_val))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
  }

  ecma_object_t *src_obj_p = ecma_get_object_from_value (this_arg);
  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (src_obj_p);
  if (ecma_arraybuffer_is_detached (arraybuffer_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("ArrayBuffer has been detached."));
  }

  ecma_typedarray_info_t src_info = ecma_typedarray_get_info (src_obj_p);

  ecma_object_t *func_object_p = ecma_get_object_from_value (cb_func_val);

  ecma_value_t new_typedarray = ecma_op_create_typedarray_with_type_and_length (src_obj_p, src_info.length);

  if (ECMA_IS_VALUE_ERROR (new_typedarray))
  {
    return new_typedarray;
  }

  ecma_object_t *target_obj_p = ecma_get_object_from_value (new_typedarray);
  ecma_typedarray_info_t target_info = ecma_typedarray_get_info (target_obj_p);

  ecma_typedarray_getter_fn_t src_typedarray_getter_cb = ecma_get_typedarray_getter_fn (src_info.id);
  ecma_typedarray_setter_fn_t target_typedarray_setter_cb = ecma_get_typedarray_setter_fn (target_info.id);

  ecma_length_t src_byte_pos = 0;

  for (uint32_t index = 0; index < src_info.length; index++)
  {
    ecma_value_t current_index = ecma_make_uint32_value (index);
    ecma_number_t element_num = src_typedarray_getter_cb (src_info.buffer_p + src_byte_pos);
    ecma_value_t get_value = ecma_make_number_value (element_num);
    ecma_value_t call_args[] = { get_value, current_index, this_arg };

    ecma_value_t mapped_value = ecma_op_function_call (func_object_p, cb_this_arg, call_args, 3);
    if (ECMA_IS_VALUE_ERROR (mapped_value))
    {
      ecma_free_value (current_index);
      ecma_free_value (get_value);
      return mapped_value;
    }

    ecma_number_t mapped_num;
    if (ECMA_IS_VALUE_ERROR (ecma_get_number (mapped_value, &mapped_num)))
    {
      ecma_free_value (mapped_value);
      ecma_free_value (current_index);
      ecma_free_value (get_value);
      return ECMA_VALUE_ERROR;
    }
    else
    {
      ecma_length_t target_byte_pos = index << target_info.shift;
      target_typedarray_setter_cb (target_info.buffer_p + target_byte_pos, mapped_num);
    }

    src_byte_pos += src_info.element_size;

    ecma_fast_free_value (mapped_value);
    ecma_fast_free_value (current_index);
    ecma_fast_free_value (get_value);
  }

  return new_typedarray;
} /* ecma_builtin_typedarray_prototype_map */

/**
 * Reduce and reduceRight routines share a similar structure.
 * And we use 'is_right' to distinguish between them.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_reduce_with_direction (ecma_value_t this_arg, /**< this argument */
                                                         ecma_value_t cb_func_val, /**< callback function */
                                                         ecma_value_t initial_val, /**< initial value */
                                                         bool is_right) /**< choose order, true is reduceRight */
{
  if (!ecma_is_typedarray (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
  }

  if (!ecma_op_is_callable (cb_func_val))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (this_arg);
  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (obj_p);
  if (ecma_arraybuffer_is_detached (arraybuffer_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("ArrayBuffer has been detached."));
  }

  ecma_typedarray_info_t info = ecma_typedarray_get_info (obj_p);

  ecma_typedarray_getter_fn_t getter_cb = ecma_get_typedarray_getter_fn (info.id);
  ecma_length_t byte_pos;

  if (info.length == 0)
  {
    if (ecma_is_value_undefined (initial_val))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("Initial value cannot be undefined."));
    }

    return ecma_copy_value (initial_val);
  }

  JERRY_ASSERT (info.length > 0);

  ecma_value_t accumulator = ECMA_VALUE_UNDEFINED;
  uint32_t index = is_right ? (info.length - 1) : 0;

  if (ecma_is_value_undefined (initial_val))
  {
    byte_pos = index << info.shift;
    ecma_number_t acc_num = getter_cb (info.buffer_p + byte_pos);
    accumulator = ecma_make_number_value (acc_num);

    JERRY_ASSERT (ecma_is_value_number (accumulator));

    if (is_right)
    {
      if (index == 0)
      {
        return accumulator;
      }

      index--;
    }
    else
    {
      index++;

      if (index == info.length)
      {
        return accumulator;
      }
    }
  }
  else
  {
    accumulator = ecma_copy_value (initial_val);
  }

  ecma_object_t *func_object_p = ecma_get_object_from_value (cb_func_val);

  while (true)
  {
    ecma_value_t current_index = ecma_make_uint32_value (index);
    byte_pos = index << info.shift;
    ecma_number_t get_num = getter_cb (info.buffer_p + byte_pos);
    ecma_value_t get_value = ecma_make_number_value (get_num);

    ecma_value_t call_args[] = { accumulator, get_value, current_index, this_arg };

    JERRY_ASSERT (ecma_is_value_number (get_value));

    ecma_value_t call_value = ecma_op_function_call (func_object_p,
                                                     ECMA_VALUE_UNDEFINED,
                                                     call_args,
                                                     4);

    ecma_fast_free_value (accumulator);
    ecma_fast_free_value (get_value);
    ecma_fast_free_value (current_index);

    if (ECMA_IS_VALUE_ERROR (call_value))
    {
      return call_value;
    }

    accumulator = call_value;

    if (is_right)
    {
      if (index == 0)
      {
        break;
      }

      index--;
    }
    else
    {
      index++;

      if (index == info.length)
      {
        break;
      }
    }
  }

  return accumulator;
} /* ecma_builtin_typedarray_prototype_reduce_with_direction */

/**
 * The %TypedArray%.prototype object's 'reduce' routine
 *
 * See also:
 *          ES2015, 22.2.3.19
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_reduce (ecma_value_t this_arg, /**< this argument */
                                          ecma_value_t cb_func_val, /**< callback function */
                                          ecma_value_t initial_val) /**< initial value */
{
  return ecma_builtin_typedarray_prototype_reduce_with_direction (this_arg,
                                                                  cb_func_val,
                                                                  initial_val,
                                                                  false);
} /* ecma_builtin_typedarray_prototype_reduce */

/**
 * The %TypedArray%.prototype object's 'reduceRight' routine
 *
 * See also:
 *          ES2015, 22.2.3.20
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_reduce_right (ecma_value_t this_arg, /**< this argument */
                                                ecma_value_t cb_func_val, /**< callback function */
                                                ecma_value_t initial_val) /**< initial value */
{
  return ecma_builtin_typedarray_prototype_reduce_with_direction (this_arg,
                                                                  cb_func_val,
                                                                  initial_val,
                                                                  true);
} /* ecma_builtin_typedarray_prototype_reduce_right */

/**
 * The %TypedArray%.prototype object's 'filter' routine
 *
 * See also:
 *          ES2015, 22.2.3.9
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_filter (ecma_value_t this_arg, /**< this argument */
                                          ecma_value_t cb_func_val, /**< callback function */
                                          ecma_value_t cb_this_arg) /**< 'this' of the callback function */
{
  if (!ecma_is_typedarray (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
  }

  if (!ecma_op_is_callable (cb_func_val))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (this_arg);
  ecma_typedarray_info_t info = ecma_typedarray_get_info (obj_p);

  ecma_typedarray_getter_fn_t getter_cb = ecma_get_typedarray_getter_fn (info.id);

  ecma_object_t *func_object_p = ecma_get_object_from_value (cb_func_val);
  ecma_value_t ret_value = ECMA_VALUE_ERROR;

  if (info.length == 0)
  {
    return ecma_op_create_typedarray_with_type_and_length (obj_p, 0);
  }

  JMEM_DEFINE_LOCAL_ARRAY (pass_value_list_p, info.length * info.element_size, lit_utf8_byte_t);

  lit_utf8_byte_t *pass_value_p = pass_value_list_p;
  ecma_length_t byte_pos = 0;

  for (uint32_t index = 0; index < info.length; index++)
  {
    ecma_value_t current_index = ecma_make_uint32_value (index);
    ecma_number_t get_num = getter_cb (info.buffer_p + byte_pos);
    ecma_value_t get_value = ecma_make_number_value (get_num);

    JERRY_ASSERT (ecma_is_value_number (get_value));

    ecma_value_t call_args[] = { get_value, current_index, this_arg };

    ecma_fast_free_value (current_index);
    ecma_fast_free_value (get_value);

    ecma_value_t call_value = ecma_op_function_call (func_object_p, cb_this_arg, call_args, 3);

    if (ECMA_IS_VALUE_ERROR (call_value))
    {
      goto cleanup;
    }

    if (ecma_op_to_boolean (call_value))
    {
      memcpy (pass_value_p, info.buffer_p + byte_pos, info.element_size);
      pass_value_p += info.element_size;
    }

    byte_pos += info.element_size;

    ecma_free_value (call_value);
  }

  uint32_t pass_num = (uint32_t) ((pass_value_p - pass_value_list_p) >> info.shift);

  ret_value = ecma_op_create_typedarray_with_type_and_length (obj_p, pass_num);

  if (!ECMA_IS_VALUE_ERROR (ret_value))
  {
    obj_p = ecma_get_object_from_value (ret_value);

    JERRY_ASSERT (ecma_typedarray_get_offset (obj_p) == 0);

    memcpy (ecma_typedarray_get_buffer (obj_p),
            pass_value_list_p,
            (size_t) (pass_value_p - pass_value_list_p));
  }

cleanup:
  JMEM_FINALIZE_LOCAL_ARRAY (pass_value_list_p);

  return ret_value;
} /* ecma_builtin_typedarray_prototype_filter */

/**
 * The %TypedArray%.prototype object's 'reverse' routine
 *
 * See also:
 *          ES2015, 22.2.3.21
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_typedarray_prototype_reverse (ecma_value_t this_arg) /**< this argument */
{
  if (!ecma_is_typedarray (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (this_arg);
  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (obj_p);
  if (ecma_arraybuffer_is_detached (arraybuffer_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("ArrayBuffer has been detached."));
  }

  ecma_typedarray_info_t info = ecma_typedarray_get_info (obj_p);

  uint32_t middle = (info.length / 2) << info.shift;
  uint32_t buffer_last = (info.length << info.shift) - info.element_size;

  for (uint32_t lower = 0; lower < middle; lower += info.element_size)
  {
    uint32_t upper = buffer_last - lower;
    lit_utf8_byte_t *lower_p = info.buffer_p + lower;
    lit_utf8_byte_t *upper_p = info.buffer_p + upper;

    lit_utf8_byte_t tmp[8];
    memcpy (&tmp[0], lower_p, info.element_size);
    memcpy (lower_p, upper_p, info.element_size);
    memcpy (upper_p, &tmp[0], info.element_size);
  }

  return ecma_copy_value (this_arg);
} /* ecma_builtin_typedarray_prototype_reverse */

/**
 * The %TypedArray%.prototype object's 'set' routine for a typedArray source
 *
 * See also:
 *          ES2015, 22.2.3.22, 22.2.3.22.2
 *
 * @return ecma value of undefined if success, error otherwise.
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_op_typedarray_set_with_typedarray (ecma_value_t this_arg, /**< this argument */
                                        ecma_value_t arr_val, /**< typedarray object */
                                        ecma_value_t offset_val) /**< offset value */
{
  /* 6.~ 8. targetOffset */
  ecma_number_t target_offset_num;
  if (ECMA_IS_VALUE_ERROR (ecma_get_number (offset_val, &target_offset_num)))
  {
    return ECMA_VALUE_ERROR;
  }

  if (ecma_number_is_nan (target_offset_num))
  {
    target_offset_num = 0;
  }

  if (target_offset_num <= -1.0 || target_offset_num >= (ecma_number_t) UINT32_MAX + 0.5)
  {
    return ecma_raise_range_error (ECMA_ERR_MSG ("Invalid offset"));
  }

  ecma_object_t *target_typedarray_p = ecma_get_object_from_value (this_arg);
  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (target_typedarray_p);
  if (ecma_arraybuffer_is_detached (arraybuffer_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("ArrayBuffer has been detached."));
  }

  ecma_typedarray_info_t target_info = ecma_typedarray_get_info (target_typedarray_p);

  ecma_object_t *src_typedarray_p = ecma_get_object_from_value (arr_val);
  ecma_object_t *src_arraybuffer_p = ecma_typedarray_get_arraybuffer (src_typedarray_p);
  if (ecma_arraybuffer_is_detached (src_arraybuffer_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("ArrayBuffer has been detached."));
  }

  ecma_typedarray_info_t src_info = ecma_typedarray_get_info (src_typedarray_p);

  uint32_t target_offset_uint32 = ecma_number_to_uint32 (target_offset_num);

  if ((int64_t) src_info.length + target_offset_uint32 > target_info.length)
  {
    return ecma_raise_range_error (ECMA_ERR_MSG ("Invalid range of index"));
  }

  /* Fast path first. If the source and target arrays are the same we do not need to copy anything. */
  if (this_arg == arr_val)
  {
    return ECMA_VALUE_UNDEFINED;
  }

  /* 26. targetByteIndex */
  uint32_t target_byte_index = target_offset_uint32 * target_info.element_size;

  /* 27. limit */
  uint32_t limit = target_byte_index + target_info.element_size * src_info.length;

  if (src_info.id == target_info.id)
  {
    memmove (target_info.buffer_p + target_byte_index, src_info.buffer_p,
             target_info.element_size * src_info.length);
  }
  else
  {
    ecma_typedarray_getter_fn_t src_typedarray_getter_cb = ecma_get_typedarray_getter_fn (src_info.id);
    ecma_typedarray_setter_fn_t target_typedarray_setter_cb = ecma_get_typedarray_setter_fn (target_info.id);

    uint32_t src_byte_index = 0;
    while (target_byte_index < limit)
    {
      ecma_number_t elem_num = src_typedarray_getter_cb (src_info.buffer_p + src_byte_index);
      target_typedarray_setter_cb (target_info.buffer_p + target_byte_index, elem_num);
      src_byte_index += src_info.element_size;
      target_byte_index += target_info.element_size;
    }
  }

  return ECMA_VALUE_UNDEFINED;
} /* ecma_op_typedarray_set_with_typedarray */

/**
 * The %TypedArray%.prototype object's 'set' routine
 *
 * See also:
 *          ES2015, 22.2.3.22, 22.2.3.22.1
 *
 * @return ecma value of undefined if success, error otherwise.
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_typedarray_prototype_set (ecma_value_t this_arg, /**< this argument */
                                       ecma_value_t arr_val, /**< array object */
                                       ecma_value_t offset_val) /**< offset value */
{
  /* 2.~ 4. */
  if (!ecma_is_typedarray (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
  }

  /* 1. */
  if (ecma_is_typedarray (arr_val))
  {
    /* 22.2.3.22.2 */
    return ecma_op_typedarray_set_with_typedarray (this_arg, arr_val, offset_val);
  }

  /* 6.~ 8. targetOffset */
  ecma_value_t ret_val = ECMA_VALUE_EMPTY;
  ECMA_OP_TO_NUMBER_TRY_CATCH (target_offset_num, offset_val, ret_val);
  if (ecma_number_is_nan (target_offset_num))
  {
    target_offset_num = 0;
  }
  if (target_offset_num <= -1.0 || target_offset_num >= (ecma_number_t) UINT32_MAX + 0.5)
  {
    return ecma_raise_range_error (ECMA_ERR_MSG ("Invalid offset"));
  }
  uint32_t target_offset_uint32 = ecma_number_to_uint32 (target_offset_num);

  /* 11. ~ 15. */
  ecma_object_t *typedarray_p = ecma_get_object_from_value (this_arg);
  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (typedarray_p);
  if (ecma_arraybuffer_is_detached (arraybuffer_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("ArrayBuffer has been detached."));
  }

  ecma_typedarray_info_t target_info = ecma_typedarray_get_info (typedarray_p);

  /* 16.~ 17. */
  ECMA_TRY_CATCH (source_obj, ecma_op_to_object (arr_val), ret_val);

  /* 18.~ 19. */
  ecma_object_t *source_obj_p = ecma_get_object_from_value (source_obj);

  ECMA_TRY_CATCH (source_length,
                  ecma_op_object_get_by_magic_id (source_obj_p, LIT_MAGIC_STRING_LENGTH),
                  ret_val);

  uint32_t source_length_uint32;
  if (ECMA_IS_VALUE_ERROR (ecma_op_to_length (source_length, &source_length_uint32)))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 20. if srcLength + targetOffset > targetLength, throw a RangeError */
  if ((int64_t) source_length_uint32 + target_offset_uint32 > target_info.length)
  {
    ret_val = ecma_raise_range_error (ECMA_ERR_MSG ("Invalid range of index"));
  }

  /* 21.~ 25. */
  uint32_t target_byte_index = target_offset_uint32 * target_info.element_size;
  uint32_t k = 0;

  ecma_typedarray_setter_fn_t target_typedarray_setter_cb = ecma_get_typedarray_setter_fn (target_info.id);

  while (k < source_length_uint32 && ecma_is_value_empty (ret_val))
  {
    ECMA_TRY_CATCH (elem,
                    ecma_op_object_get_by_uint32_index (source_obj_p, k),
                    ret_val);

    ECMA_OP_TO_NUMBER_TRY_CATCH (elem_num, elem, ret_val);

    target_typedarray_setter_cb (target_info.buffer_p + target_byte_index, elem_num);

    ECMA_OP_TO_NUMBER_FINALIZE (elem_num);
    ECMA_FINALIZE (elem);

    k++;
    target_byte_index += target_info.element_size;
  }

  ECMA_FINALIZE (source_length);
  ECMA_FINALIZE (source_obj);
  ECMA_OP_TO_NUMBER_FINALIZE (target_offset_num);

  if (ecma_is_value_empty (ret_val))
  {
    ret_val = ECMA_VALUE_UNDEFINED;
  }
  return ret_val;
} /* ecma_builtin_typedarray_prototype_set */

/**
 * TypedArray.prototype's 'toString' single element operation routine based
 * on the Array.prototype's 'toString' single element operation routine
 *
 * See also:
 *          ECMA-262 v5.1, 15.4.4.2
 *
 * @return NULL - if the converison fails
 *         ecma_string_t * - otherwise
 */
static ecma_string_t *
ecma_op_typedarray_get_to_string_at_index (ecma_object_t *obj_p, /**< this object */
                                           uint32_t index) /**< array index */
{
  ecma_value_t index_value = ecma_op_object_get_by_uint32_index (obj_p, index);

  if (ECMA_IS_VALUE_ERROR (index_value))
  {
    return NULL;
  }

  if (ecma_is_value_undefined (index_value)
      || ecma_is_value_null (index_value))
  {
    ecma_free_value (index_value);
    return ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
  }

  ecma_string_t *ret_str_p = ecma_op_to_string (index_value);

  ecma_free_value (index_value);

  return ret_str_p;
} /* ecma_op_typedarray_get_to_string_at_index */

/**
 * The TypedArray.prototype.toString's separator creation routine based on
 * the Array.prototype.toString's separator routine
 *
 * See also:
 *          ECMA-262 v5.1, 15.4.4.2 4th step
 *
 * @return NULL - if the conversion fails
 *         ecma_string_t * - otherwise
 */
static ecma_string_t *
ecma_op_typedarray_get_separator_string (ecma_value_t separator) /**< possible separator */
{
  if (ecma_is_value_undefined (separator))
  {
    return ecma_get_magic_string (LIT_MAGIC_STRING_COMMA_CHAR);
  }

  return ecma_op_to_string (separator);
} /* ecma_op_typedarray_get_separator_string */

/**
 * The TypedArray.prototype object's 'join' routine basen on
 * the Array.porottype object's 'join'
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_join (ecma_value_t this_arg, /**< this argument */
                                        ecma_value_t separator_arg) /**< separator argument */
{
  /* 1. */
  ecma_value_t obj_value = ecma_op_to_object (this_arg);

  if (ECMA_IS_VALUE_ERROR (obj_value))
  {
    return obj_value;
  }
  ecma_object_t *obj_p = ecma_get_object_from_value (obj_value);

  /* 2. */
  ecma_value_t length_value = ecma_op_object_get_by_magic_id (obj_p, LIT_MAGIC_STRING_LENGTH);

  if (ECMA_IS_VALUE_ERROR (length_value))
  {
    ecma_free_value (obj_value);
    return length_value;
  }

  ecma_number_t length_number;

  if (ECMA_IS_VALUE_ERROR (ecma_get_number (length_value, &length_number)))
  {
    ecma_free_value (length_value);
    ecma_free_value (obj_value);
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t ret_value = ECMA_VALUE_ERROR;

  /* 3. */
  uint32_t length = ecma_number_to_uint32 (length_number);

  if (length == 0)
  {
    /* 6. */
    ecma_free_value (length_value);
    ecma_free_value (obj_value);
    return ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
  }
  else
  {
    ecma_string_t *separator_string_p = ecma_op_typedarray_get_separator_string (separator_arg);

    if (JERRY_UNLIKELY (separator_string_p == NULL))
    {
      goto cleanup;
    }

    /* 7-8. */
    ecma_string_t *return_string_p = ecma_op_typedarray_get_to_string_at_index (obj_p, 0);

    if (JERRY_UNLIKELY (return_string_p == NULL))
    {
      ecma_deref_ecma_string (separator_string_p);
      goto cleanup;
    }

    /* 9-10. */
    for (uint32_t k = 1; k < length; k++)
    {
      /* 10.a */
      return_string_p = ecma_concat_ecma_strings (return_string_p, separator_string_p);

      /* 10.d */
      ecma_string_t *next_string_p = ecma_op_typedarray_get_to_string_at_index (obj_p, k);

      if (JERRY_UNLIKELY (next_string_p == NULL))
      {
        ecma_deref_ecma_string (separator_string_p);
        ecma_deref_ecma_string (return_string_p);
        goto cleanup;
      }

      return_string_p = ecma_concat_ecma_strings (return_string_p, next_string_p);
      ecma_deref_ecma_string (next_string_p);
    }

    ecma_deref_ecma_string (separator_string_p);
    ret_value = ecma_make_string_value (return_string_p);
  }

cleanup:
  ecma_free_value (length_value);
  ecma_free_value (obj_value);

  return ret_value;
} /* ecma_builtin_typedarray_prototype_join */

/**
 * The TypedArray.prototype object's 'toString' routine basen on
 * the Array.porottype object's 'toString'
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_object_to_string (ecma_value_t this_arg) /**< this argument */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  /* 1. */
  ecma_value_t obj_this_value = ecma_op_to_object (this_arg);
  if (ECMA_IS_VALUE_ERROR (obj_this_value))
  {
    return obj_this_value;
  }
  ecma_object_t *obj_p = ecma_get_object_from_value (obj_this_value);

  /* 2. */
  ecma_value_t join_value = ecma_op_object_get_by_magic_id (obj_p, LIT_MAGIC_STRING_JOIN);
  if (ECMA_IS_VALUE_ERROR (join_value))
  {
    ecma_free_value (obj_this_value);
    return join_value;
  }

  if (!ecma_op_is_callable (join_value))
  {
    /* 3. */
    ret_value = ecma_builtin_helper_object_to_string (this_arg);
  }
  else
  {
    /* 4. */
    ecma_object_t *join_func_obj_p = ecma_get_object_from_value (join_value);

    ret_value = ecma_op_function_call (join_func_obj_p, this_arg, NULL, 0);
  }

  ecma_free_value (join_value);
  ecma_free_value (obj_this_value);

  return ret_value;
} /* ecma_builtin_typedarray_prototype_object_to_string */

/**
 * The %TypedArray%.prototype object's 'subarray' routine.
 *
 * See also:
 *          ES2015, 22.2.3.26
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_subarray (ecma_value_t this_arg, /**< this argument */
                                            ecma_value_t begin, /**< begin */
                                            ecma_value_t end) /**< end */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  /* 2.~ 4. */
  if (!ecma_is_typedarray (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
  }

  ecma_object_t *src_typedarray_p = ecma_get_object_from_value (this_arg);
  ecma_typedarray_info_t info = ecma_typedarray_get_info (src_typedarray_p);

  /* 9. beginIndex, 12. endIndex */
  uint32_t begin_index_uint32 = 0, end_index_uint32 = 0;

  /* 7. relativeBegin */
  ECMA_OP_TO_NUMBER_TRY_CATCH (relative_begin, begin, ret_value);

  begin_index_uint32 = ecma_builtin_helper_array_index_normalize (relative_begin, info.length, false);

  if (ecma_is_value_undefined (end))
  {
    end_index_uint32 = (uint32_t) info.length;
  }
  else
  {
    /* 10. relativeEnd */
    ECMA_OP_TO_NUMBER_TRY_CATCH (relative_end, end, ret_value);

    end_index_uint32 = ecma_builtin_helper_array_index_normalize (relative_end, info.length, false);

    ECMA_OP_TO_NUMBER_FINALIZE (relative_end);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (relative_begin);

  if (!ecma_is_value_empty (ret_value))
  {
    return ret_value;
  }

  /* 13. newLength */
  ecma_length_t subarray_length = 0;

  if (end_index_uint32 > begin_index_uint32)
  {
    subarray_length = end_index_uint32 - begin_index_uint32;
  }

  /* 17. beginByteOffset */
  ecma_length_t begin_byte_offset = info.offset + begin_index_uint32 * info.element_size;

  ecma_value_t arguments_p[3] =
  {
    ecma_make_object_value (info.array_buffer_p),
    ecma_make_uint32_value (begin_byte_offset),
    ecma_make_uint32_value (subarray_length)
  };

  ret_value = ecma_typedarray_helper_dispatch_construct (arguments_p, 3, info.id);

  ecma_free_value (arguments_p[1]);
  ecma_free_value (arguments_p[2]);
  return ret_value;
} /* ecma_builtin_typedarray_prototype_subarray */

/**
 * The %TypedArray%.prototype object's 'fill' routine.
 *
 * See also:
 *          ES2015, 22.2.3.8, 22.1.3.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_fill (ecma_value_t this_arg, /**< this argument */
                                        ecma_value_t value, /**< value */
                                        ecma_value_t begin, /**< begin */
                                        ecma_value_t end) /**< end */
{
  if (!ecma_is_typedarray (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
  }

  ecma_number_t value_num;
  ecma_value_t ret_value = ecma_get_number (value, &value_num);

  if (!ecma_is_value_empty (ret_value))
  {
    return ret_value;
  }

  ecma_object_t *typedarray_p = ecma_get_object_from_value (this_arg);
  ecma_typedarray_info_t info = ecma_typedarray_get_info (typedarray_p);

  uint32_t begin_index_uint32 = 0, end_index_uint32 = 0;

  ECMA_OP_TO_NUMBER_TRY_CATCH (relative_begin, begin, ret_value);

  begin_index_uint32 = ecma_builtin_helper_array_index_normalize (relative_begin, info.length, false);

  if (ecma_is_value_undefined (end))
  {
    end_index_uint32 = (uint32_t) info.length;
  }
  else
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (relative_end, end, ret_value);

    end_index_uint32 = ecma_builtin_helper_array_index_normalize (relative_end, info.length, false);

    ECMA_OP_TO_NUMBER_FINALIZE (relative_end);
  }

  ECMA_OP_TO_NUMBER_FINALIZE (relative_begin);

  if (!ecma_is_value_empty (ret_value))
  {
    return ret_value;
  }

  ecma_length_t subarray_length = 0;

  if (end_index_uint32 > begin_index_uint32)
  {
    subarray_length = end_index_uint32 - begin_index_uint32;
  }

  ecma_typedarray_setter_fn_t typedarray_setter_cb = ecma_get_typedarray_setter_fn (info.id);
  uint32_t byte_index = begin_index_uint32 * info.element_size;
  uint32_t limit = byte_index + subarray_length * info.element_size;

  while (byte_index < limit)
  {
    typedarray_setter_cb (info.buffer_p + byte_index, value_num);
    byte_index += info.element_size;
  }

  return ecma_copy_value (this_arg);
} /* ecma_builtin_typedarray_prototype_fill */

/**
 * SortCompare abstract method
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.11
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_sort_compare_helper (ecma_value_t lhs, /**< left value */
                                                       ecma_value_t rhs, /**< right value */
                                                       ecma_value_t compare_func) /**< compare function */
{
  ecma_value_t ret_value = ECMA_VALUE_EMPTY;
  ecma_number_t result = ECMA_NUMBER_ZERO;

  if (ecma_is_value_undefined (compare_func))
  {
    /* Default comparison when no comparefn is passed. */
    double lhs_value = (double) ecma_get_number_from_value (lhs);
    double rhs_value = (double) ecma_get_number_from_value (rhs);

    if (ecma_number_is_nan (lhs_value))
    {
      // Keep NaNs at the end of the array.
      result = ECMA_NUMBER_ONE;
    }
    else if (ecma_number_is_nan (rhs_value))
    {
      // Keep NaNs at the end of the array.
      result = ECMA_NUMBER_MINUS_ONE;
    }
    else if (lhs_value < rhs_value)
    {
      result = ECMA_NUMBER_MINUS_ONE;
    }
    else if (lhs_value > rhs_value)
    {
      result = ECMA_NUMBER_ONE;
    }
    else
    {
      result = ECMA_NUMBER_ZERO;
    }

    return ecma_make_number_value (result);
  }

  /*
   * compare_func, if not undefined, will always contain a callable function object.
   * We checked this previously, before this function was called.
   */
  JERRY_ASSERT (ecma_op_is_callable (compare_func));
  ecma_object_t *comparefn_obj_p = ecma_get_object_from_value (compare_func);

  ecma_value_t compare_args[] = { lhs, rhs };

  ECMA_TRY_CATCH (call_value,
                  ecma_op_function_call (comparefn_obj_p,
                                         ECMA_VALUE_UNDEFINED,
                                         compare_args,
                                         2),
                  ret_value);

  if (!ecma_is_value_number (call_value))
  {
    ECMA_OP_TO_NUMBER_TRY_CATCH (ret_num, call_value, ret_value);
    result = ret_num;
    ECMA_OP_TO_NUMBER_FINALIZE (ret_num);

    // If the coerced value can't be represented as a Number, compare them as equals.
    if (ecma_number_is_nan (result))
    {
      result = ECMA_NUMBER_ZERO;
    }
  }
  else
  {
    result = ecma_get_number_from_value (call_value);
  }

  ECMA_FINALIZE (call_value);

  if (ecma_is_value_empty (ret_value))
  {
    ret_value = ecma_make_number_value (result);
  }

  return ret_value;
} /* ecma_builtin_typedarray_prototype_sort_compare_helper */

/**
 * The %TypedArray%.prototype object's 'sort' routine.
 *
 * See also:
 *          ES2015, 22.2.3.25, 22.1.3.24
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_sort (ecma_value_t this_arg, /**< this argument */
                                        ecma_value_t compare_func) /**< comparator fn */
{
  if (!ecma_is_typedarray (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
  }

  if (!ecma_is_value_undefined (compare_func) && !ecma_op_is_callable (compare_func))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Compare function is not callable."));
  }

  ecma_object_t *typedarray_p = ecma_get_object_from_value (this_arg);
  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (typedarray_p);
  if (ecma_arraybuffer_is_detached (arraybuffer_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("ArrayBuffer has been detached."));
  }
  ecma_typedarray_info_t info = ecma_typedarray_get_info (typedarray_p);

  if (!info.length)
  {
    return ecma_copy_value (this_arg);
  }

  ecma_value_t ret_value = ECMA_VALUE_EMPTY;

  JMEM_DEFINE_LOCAL_ARRAY (values_buffer, info.length, ecma_value_t);

  uint32_t byte_index = 0, buffer_index = 0;
  uint32_t limit = info.length * info.element_size;

  ecma_typedarray_getter_fn_t typedarray_getter_cb = ecma_get_typedarray_getter_fn (info.id);
  /* Copy unsorted array into a native c array. */
  while (byte_index < limit)
  {
    JERRY_ASSERT (buffer_index < info.length);
    ecma_number_t element_num = typedarray_getter_cb (info.buffer_p + byte_index);
    ecma_value_t element_value = ecma_make_number_value (element_num);
    values_buffer[buffer_index++] = element_value;
    byte_index += info.element_size;
  }

  JERRY_ASSERT (buffer_index == info.length);

  const ecma_builtin_helper_sort_compare_fn_t sort_cb = &ecma_builtin_typedarray_prototype_sort_compare_helper;
  ECMA_TRY_CATCH (sort_value,
                  ecma_builtin_helper_array_heap_sort_helper (values_buffer,
                                                              (uint32_t) (info.length - 1),
                                                              compare_func,
                                                              sort_cb),
                  ret_value);
  ECMA_FINALIZE (sort_value);

  ecma_typedarray_setter_fn_t typedarray_setter_cb = ecma_get_typedarray_setter_fn (info.id);

  if (ecma_is_value_empty (ret_value))
  {
    byte_index = 0;
    buffer_index = 0;
    limit = info.length * info.element_size;
    /* Put sorted values from the native array back into the typedarray buffer. */
    while (byte_index < limit)
    {
      JERRY_ASSERT (buffer_index < info.length);
      ecma_value_t element_value = values_buffer[buffer_index++];
      ecma_number_t element_num = ecma_get_number_from_value (element_value);
      typedarray_setter_cb (info.buffer_p + byte_index, element_num);
      byte_index += info.element_size;
    }

    JERRY_ASSERT (buffer_index == info.length);
  }

  /* Free values that were copied to the local array. */
  for (uint32_t index = 0; index < info.length; index++)
  {
    ecma_free_value (values_buffer[index]);
  }

  JMEM_FINALIZE_LOCAL_ARRAY (values_buffer);

  if (ecma_is_value_empty (ret_value))
  {
    ret_value = ecma_copy_value (this_arg);
  }

  return ret_value;
} /* ecma_builtin_typedarray_prototype_sort */

/**
 * The %TypedArray%.prototype object's 'find' and 'findIndex' routine helper
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_find_helper (ecma_value_t this_arg, /**< this argument */
                                               ecma_value_t predicate, /**< callback function */
                                               ecma_value_t predicate_this_arg, /**< this argument for
                                                                                 *   invoke predicate */
                                               bool is_find) /**< true - find routine
                                                              *   false - findIndex routine */
{
  if (!ecma_is_typedarray (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
  }

  if (!ecma_op_is_callable (predicate))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Callback function is not callable."));
  }

  JERRY_ASSERT (ecma_is_value_object (predicate));
  ecma_object_t *func_object_p = ecma_get_object_from_value (predicate);

  ecma_object_t *typedarray_p = ecma_get_object_from_value (this_arg);
  ecma_typedarray_info_t info = ecma_typedarray_get_info (typedarray_p);
  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (typedarray_p);
  if (ecma_arraybuffer_is_detached (arraybuffer_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("ArrayBuffer has been detached."));
  }

  uint32_t buffer_index = 0;
  uint32_t limit = info.length * info.element_size;

  ecma_typedarray_getter_fn_t typedarray_getter_cb = ecma_get_typedarray_getter_fn (info.id);

  for (uint32_t byte_index = 0; byte_index < limit; byte_index += info.element_size)
  {
    JERRY_ASSERT (buffer_index < info.length);
    ecma_number_t element_num = typedarray_getter_cb (info.buffer_p + byte_index);
    ecma_value_t element_value = ecma_make_number_value (element_num);

    ecma_value_t call_args[] = { element_value, ecma_make_uint32_value (buffer_index), this_arg };

    ecma_value_t call_value = ecma_op_function_call (func_object_p, predicate_this_arg, call_args, 3);

    if (ECMA_IS_VALUE_ERROR (call_value))
    {
      ecma_free_value (element_value);
      return call_value;
    }

    bool call_result = ecma_op_to_boolean (call_value);
    ecma_free_value (call_value);

    if (call_result)
    {
      if (is_find)
      {
        return element_value;
      }

      ecma_free_value (element_value);
      return ecma_make_uint32_value (buffer_index);
    }
    buffer_index++;
    ecma_free_value (element_value);
  }

  return is_find ? ECMA_VALUE_UNDEFINED : ecma_make_integer_value (-1);
} /* ecma_builtin_typedarray_prototype_find_helper */

/**
 * The %TypedArray%.prototype object's 'find' routine
 *
 * See also:
 *          ECMA-262 v6, 22.2.3.10
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_find (ecma_value_t this_arg, /**< this argument */
                                        ecma_value_t predicate, /**< callback function */
                                        ecma_value_t predicate_this_arg) /**< this argument for
                                                                          *   invoke predicate */
{
  return ecma_builtin_typedarray_prototype_find_helper (this_arg, predicate, predicate_this_arg, true);
} /* ecma_builtin_typedarray_prototype_find */

/**
 * The %TypedArray%.prototype object's 'findIndex' routine
 *
 * See also:
 *          ECMA-262 v6, 22.2.3.11
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_find_index (ecma_value_t this_arg, /**< this argument */
                                              ecma_value_t predicate, /**< callback function */
                                              ecma_value_t predicate_this_arg) /**< this argument for
                                                                                *   invoke predicate */
{
  return ecma_builtin_typedarray_prototype_find_helper (this_arg, predicate, predicate_this_arg, false);
} /* ecma_builtin_typedarray_prototype_find_index */

/**
 * The %TypedArray%.prototype object's 'indexOf' and 'lastIndexOf' routine helper
 *
 * See also:
 *         ECMA-262 v6, 22.2.3.13, 22.2.3.16
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_index_helper (ecma_value_t this_arg, /**< this argument */
                                                const ecma_value_t args[], /**< arguments list */
                                                ecma_length_t args_number, /**< number of arguments */
                                                bool is_last_index_of) /**< true - lastIndexOf routine
                                                                         false - indexOf routine */
{
  if (!ecma_is_typedarray (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
  }

  ecma_object_t *typedarray_p = ecma_get_object_from_value (this_arg);
  ecma_typedarray_info_t info = ecma_typedarray_get_info (typedarray_p);
  if (ecma_arraybuffer_is_detached (info.array_buffer_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("ArrayBuffer has been detached."));
  }

  uint32_t limit = info.length * info.element_size;
  uint32_t from_index;

  if (args_number == 0
      || !ecma_is_value_number (args[0])
      || info.length == 0)
  {
    return ecma_make_integer_value (-1);
  }
  if (args_number == 1)
  {
    from_index = is_last_index_of ? info.length - 1 : 0;
  }
  else
  {
    ecma_number_t num_var;

    if (ECMA_IS_VALUE_ERROR (ecma_get_number (args[1], &num_var)))
    {
      return ECMA_VALUE_ERROR;
    }

    if (!ecma_number_is_nan (num_var)
        && is_last_index_of
        && num_var < 0
        && fabs (num_var) > info.length)
    {
      return ecma_make_integer_value (-1);
    }
    else
    {
      from_index = ecma_builtin_helper_array_index_normalize (num_var, info.length, is_last_index_of);
    }
  }

  ecma_number_t search_num = ecma_get_number_from_value (args[0]);

  int32_t increment = is_last_index_of ? -info.element_size : info.element_size;
  ecma_typedarray_getter_fn_t getter_cb = ecma_get_typedarray_getter_fn (info.id);

  for (int32_t position = (int32_t) from_index * info.element_size;
       (is_last_index_of ? position >= 0 : (uint32_t) position < limit);
       position += increment)
  {
    ecma_number_t element_num = getter_cb (info.buffer_p + position);

    if (search_num == element_num)
    {
      return ecma_make_number_value ((ecma_number_t) position / info.element_size);
    }
  }

  return ecma_make_integer_value (-1);
} /* ecma_builtin_typedarray_prototype_index_helper */

/**
 * The %TypedArray%.prototype object's 'indexOf' routine
 *
 * See also:
 *         ECMA-262 v6, 22.2.3.13
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_index_of (ecma_value_t this_arg, /**< this argument */
                                           const ecma_value_t args[], /**< arguments list */
                                           ecma_length_t args_number) /**< number of arguments */
{
  return ecma_builtin_typedarray_prototype_index_helper (this_arg, args, args_number, false);
} /* ecma_builtin_typedarray_prototype_index_of */

/**
 * The %TypedArray%.prototype object's 'lastIndexOf' routine
 *
 * See also:
 *          ECMA-262 v6, 22.2.3.16
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_last_index_of (ecma_value_t this_arg, /**< this argument */
                                                const ecma_value_t args[], /**< arguments list */
                                                ecma_length_t args_number) /**< number of arguments */
{
  return ecma_builtin_typedarray_prototype_index_helper (this_arg, args, args_number, true);
} /* ecma_builtin_typedarray_prototype_last_index_of */

/**
 * Helper function to get the uint32_t value from an argument.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_get_uint32_value_from_argument (ecma_value_t arg_value, /**< this argument */
                                             uint32_t length, /**< length of the TypedArray */
                                             uint32_t *index, /**< [out] pointer to the given index */
                                             bool is_end_index) /**< true - normalize the end index */
{
  ecma_number_t num_var;
  ecma_value_t ret_value = ecma_get_number (arg_value, &num_var);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    return ret_value;
  }

  if (is_end_index && ecma_number_is_nan (num_var))
  {
    *index = length;
  }
  else
  {
    *index = ecma_builtin_helper_array_index_normalize (num_var, length, false);
  }

  return ECMA_VALUE_EMPTY;
} /* ecma_builtin_get_uint32_value_from_argument */

/**
 * The %TypedArray%.prototype object's 'copyWithin' routine
 *
 * See also:
 *          ECMA-262 v6, 22.2.3.5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_copy_within (ecma_value_t this_arg, /**< this argument */
                                               const ecma_value_t args[], /**< arguments list */
                                               ecma_length_t args_number) /**< number of arguments */
{
  if (!ecma_is_typedarray (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
  }

  ecma_object_t *typedarray_p = ecma_get_object_from_value (this_arg);
  ecma_typedarray_info_t info = ecma_typedarray_get_info (typedarray_p);
  uint32_t target = 0;
  uint32_t start = 0;
  uint32_t end = info.length;

  if (args_number > 0)
  {
    ecma_value_t target_value = ecma_builtin_get_uint32_value_from_argument (args[0],
                                                                             info.length,
                                                                             &target,
                                                                             false);

    if (ECMA_IS_VALUE_ERROR (target_value))
    {
      return target_value;
    }

    if (args_number > 1)
    {
      ecma_value_t start_value = ecma_builtin_get_uint32_value_from_argument (args[1],
                                                                              info.length,
                                                                              &start,
                                                                              false);

      if (ECMA_IS_VALUE_ERROR (start_value))
      {
        return start_value;
      }

      if (args_number > 2)
      {
        ecma_value_t end_value = ecma_builtin_get_uint32_value_from_argument (args[2],
                                                                              info.length,
                                                                              &end,
                                                                              true);

        if (ECMA_IS_VALUE_ERROR (end_value))
        {
          return end_value;
        }
      }
    }
  }

  if (target >= info.length || start >= end || end == 0)
  {
    return ecma_copy_value (this_arg);
  }
  else
  {
    uint32_t distance = end - start;
    uint32_t offset = info.length - target;
    uint32_t count = JERRY_MIN (distance, offset);

    memmove (info.buffer_p + (target * info.element_size),
             info.buffer_p + (start * info.element_size),
             (size_t) (count * info.element_size));
  }

  return ecma_copy_value (this_arg);
} /* ecma_builtin_typedarray_prototype_copy_within */

/**
 * The %TypedArray%.prototype object's 'slice' routine
 *
 * See also:
 *          ECMA-262 v6, 22.2.3.23
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_slice (ecma_value_t this_arg, /**< this argument */
                                         const ecma_value_t args[], /**< arguments list */
                                         ecma_length_t args_number) /**< number of arguments */
{
  if (!ecma_is_typedarray (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
  }

  ecma_object_t *typedarray_p = ecma_get_object_from_value (this_arg);
  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (typedarray_p);
  if (ecma_arraybuffer_is_detached (arraybuffer_p))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("ArrayBuffer has been detached."));
  }

  ecma_typedarray_info_t info = ecma_typedarray_get_info (typedarray_p);
  uint32_t start = 0;
  uint32_t end = info.length;

  if (args_number > 0)
  {
    ecma_value_t start_value = ecma_builtin_get_uint32_value_from_argument (args[0],
                                                                            info.length,
                                                                            &start,
                                                                            false);

    if (ECMA_IS_VALUE_ERROR (start_value))
    {
      return start_value;
    }

    if (args_number > 1)
    {
      ecma_value_t end_value = ecma_builtin_get_uint32_value_from_argument (args[1],
                                                                            info.length,
                                                                            &end,
                                                                            true);

      if (ECMA_IS_VALUE_ERROR (end_value))
      {
        return end_value;
      }
    }
  }

  int32_t distance = (int32_t) (end - start);
  uint32_t count = distance > 0 ? (uint32_t) distance : 0;

  ecma_value_t new_typedarray = ecma_op_create_typedarray_with_type_and_length (typedarray_p, count);

  if (ECMA_IS_VALUE_ERROR (new_typedarray))
  {
    return new_typedarray;
  }

  if (count > 0)
  {
    ecma_object_t *new_typedarray_p = ecma_get_object_from_value (new_typedarray);

    lit_utf8_byte_t *new_typedarray_buffer_p = ecma_typedarray_get_buffer (new_typedarray_p);
    uint32_t src_byte_index = (start * info.element_size);

    memcpy (new_typedarray_buffer_p,
            info.buffer_p + src_byte_index,
            count * info.element_size);
  }

  return new_typedarray;
} /* ecma_builtin_typedarray_prototype_slice */

/**
 * The TypedArray.prototype's 'toLocaleString' single element operation routine.
 *
 * See also:
 *          ECMA-262 v6, 22.1.3.26 steps 7-10 and 12.b-e
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
*/
static ecma_value_t
ecma_builtin_typedarray_prototype_to_locale_string_helper (ecma_object_t *this_obj, /**< TypedArray object */
                                                           uint32_t index) /** array index */
{
  ecma_typedarray_type_t class_id = ecma_get_typedarray_id (this_obj);
  lit_utf8_byte_t *typedarray_buffer_p = ecma_typedarray_get_buffer (this_obj);

  ecma_value_t ret_value = ECMA_VALUE_EMPTY;
  ecma_number_t element_num = ecma_get_typedarray_element (typedarray_buffer_p + index, class_id);
  ecma_value_t element_value = ecma_make_number_value (element_num);

  ecma_value_t element_obj = ecma_op_create_number_object (element_value);

  ecma_free_value (element_value);

  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (element_obj));

  ecma_object_t *element_obj_p = ecma_get_object_from_value (element_obj);

  ecma_value_t func_value = ecma_op_object_get_by_magic_id (element_obj_p,
                                                            LIT_MAGIC_STRING_TO_LOCALE_STRING_UL);

  if (ECMA_IS_VALUE_ERROR (func_value))
  {
    ecma_deref_object (element_obj_p);
    return func_value;
  }

  if (ecma_op_is_callable (func_value))
  {
    ecma_object_t *func_obj = ecma_get_object_from_value (func_value);
    ecma_value_t call_value = ecma_op_function_call (func_obj,
                                                     element_obj,
                                                     NULL,
                                                     0);

    ecma_deref_object (func_obj);

    if (ECMA_IS_VALUE_ERROR (call_value))
    {
      ecma_deref_object (element_obj_p);
      return call_value;
    }

    ecma_string_t *str_p = ecma_op_to_string (call_value);

    if (JERRY_UNLIKELY (str_p == NULL))
    {
      ecma_free_value (element_value);
      ecma_deref_object (element_obj_p);
      return ECMA_VALUE_ERROR;
    }

    ret_value = ecma_make_string_value (str_p);
    ecma_deref_ecma_string (str_p);
  }
  else
  {
    ecma_free_value (func_value);
    ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("'toLocaleString' is missing or not a function."));
  }

  ecma_deref_object (element_obj_p);

  return ret_value;
} /* ecma_builtin_typedarray_prototype_to_locale_string_helper */

/**
 * The %TypedArray%.prototype object's 'toLocaleString' routine
 *
 * See also:
 *          ECMA-262 v6, 22.2.3.27
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_to_locale_string (ecma_value_t this_arg) /**< this argument */
{
  if (!ecma_is_typedarray (this_arg))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
  }

  ecma_object_t *typedarray_p = ecma_get_object_from_value (this_arg);
  ecma_typedarray_info_t info = ecma_typedarray_get_info (typedarray_p);
  uint32_t limit = info.length * info.element_size;

  if (info.length == 0)
  {
    return ecma_make_magic_string_value (LIT_MAGIC_STRING__EMPTY);
  }

  ecma_value_t first_element = ecma_builtin_typedarray_prototype_to_locale_string_helper (typedarray_p, 0);

  if (ECMA_IS_VALUE_ERROR (first_element))
  {
    return first_element;
  }

  ecma_string_t *return_string_p = ecma_get_string_from_value (first_element);
  ecma_stringbuilder_t builder = ecma_stringbuilder_create_from (return_string_p);
  ecma_deref_ecma_string (return_string_p);

  for (uint32_t k = info.element_size; k < limit; k += info.element_size)
  {
    ecma_stringbuilder_append_byte (&builder, LIT_CHAR_COMMA);
    ecma_value_t next_element = ecma_builtin_typedarray_prototype_to_locale_string_helper (typedarray_p, k);

    if (ECMA_IS_VALUE_ERROR (next_element))
    {
      ecma_stringbuilder_destroy (&builder);
      return next_element;
    }

    ecma_string_t *next_element_p = ecma_get_string_from_value (next_element);
    ecma_stringbuilder_append (&builder, next_element_p);
    ecma_deref_ecma_string (next_element_p);
  }

  return ecma_make_string_value (ecma_stringbuilder_finalize (&builder));
} /* ecma_builtin_typedarray_prototype_to_locale_string */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */
