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

#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-conversion.h"
#include "ecma-function-object.h"
#include "ecma-typedarray-object.h"
#include "ecma-arraybuffer-object.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"
#include "jrt-libc-includes.h"
#include "ecma-gc.h"
#include "jmem.h"

#ifndef CONFIG_DISABLE_TYPEDARRAY_BUILTIN

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
  uint32_t len = ecma_typedarray_get_length (obj_p);
  ecma_object_t *func_object_p = ecma_get_object_from_value (cb_func_val);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  for (uint32_t index = 0; index < len && ecma_is_value_empty (ret_value); index++)
  {
    ecma_value_t current_index =  ecma_make_uint32_value (index);
    ecma_value_t get_value = ecma_op_typedarray_get_index_prop (obj_p, index);

    JERRY_ASSERT (ecma_is_value_number (get_value));

    ecma_value_t call_args[] = { get_value, current_index, this_arg };

    ECMA_TRY_CATCH (call_value, ecma_op_function_call (func_object_p, cb_this_arg, call_args, 3), ret_value);

    if (mode == TYPEDARRAY_ROUTINE_EVERY)
    {
      if (!ecma_op_to_boolean (call_value))
      {
        ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
      }
    }
    else if (mode == TYPEDARRAY_ROUTINE_SOME)
    {
      if (ecma_op_to_boolean (call_value))
      {
        ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
      }
    }

    ECMA_FINALIZE (call_value);

    ecma_fast_free_value (current_index);
    ecma_fast_free_value (get_value);
  }

  if (ecma_is_value_empty (ret_value))
  {
    if (mode == TYPEDARRAY_ROUTINE_EVERY)
    {
      ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
    }
    else if (mode == TYPEDARRAY_ROUTINE_SOME)
    {
      ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
    }
    else
    {
      ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
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

  ecma_object_t *obj_p = ecma_get_object_from_value (this_arg);
  uint32_t len = ecma_typedarray_get_length (obj_p);
  ecma_object_t *func_object_p = ecma_get_object_from_value (cb_func_val);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ecma_value_t new_typedarray = ecma_op_create_typedarray_with_type_and_length (obj_p, len);
  ecma_object_t *new_obj_p = ecma_get_object_from_value (new_typedarray);

  for (uint32_t index = 0; index < len && ecma_is_value_empty (ret_value); index++)
  {
    ecma_value_t current_index =  ecma_make_uint32_value (index);
    ecma_value_t get_value = ecma_op_typedarray_get_index_prop (obj_p, index);
    ecma_value_t call_args[] = { get_value, current_index, this_arg };

    ECMA_TRY_CATCH (mapped_value, ecma_op_function_call (func_object_p, cb_this_arg, call_args, 3), ret_value);

    ecma_value_t set_status = ecma_op_typedarray_set_index_prop (new_obj_p, index, mapped_value);

    if (!set_status)
    {
      ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("error in typedarray set"));
    }

    ECMA_FINALIZE (mapped_value);

    ecma_fast_free_value (current_index);
    ecma_fast_free_value (get_value);
  }

  if (ecma_is_value_empty (ret_value))
  {
    ret_value = new_typedarray;
  }
  else
  {
    ecma_free_value (new_typedarray);
  }

  return ret_value;
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
  uint32_t len = ecma_typedarray_get_length (obj_p);

  if (len == 0)
  {
    if (ecma_is_value_undefined (initial_val))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("Initial value cannot be undefined."));
    }

    return ecma_copy_value (initial_val);
  }

  JERRY_ASSERT (len > 0);

  ecma_value_t accumulator = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  uint32_t index = is_right ? (len - 1) : 0;

  if (ecma_is_value_undefined (initial_val))
  {
    accumulator = ecma_op_typedarray_get_index_prop (obj_p, index);

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

      if (index == len)
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
    ecma_value_t current_index =  ecma_make_uint32_value (index);
    ecma_value_t get_value = ecma_op_typedarray_get_index_prop (obj_p, index);
    ecma_value_t call_args[] = { accumulator, get_value, current_index, this_arg };

    JERRY_ASSERT (ecma_is_value_number (get_value));

    ecma_value_t call_value = ecma_op_function_call (func_object_p,
                                                     ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED),
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

      if (index == len)
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
  uint32_t len = ecma_typedarray_get_length (obj_p);
  ecma_object_t *func_object_p = ecma_get_object_from_value (cb_func_val);
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  uint32_t pass_num = 0;

  JMEM_DEFINE_LOCAL_ARRAY (pass_value_list, len, ecma_value_t);

  for (uint32_t index = 0; index < len && ecma_is_value_empty (ret_value); index++)
  {
    ecma_value_t current_index =  ecma_make_uint32_value (index);
    ecma_value_t get_value = ecma_op_typedarray_get_index_prop (obj_p, index);

    JERRY_ASSERT (ecma_is_value_number (get_value));

    ecma_value_t call_args[] = { get_value, current_index, this_arg };

    ECMA_TRY_CATCH (call_value, ecma_op_function_call (func_object_p, cb_this_arg, call_args, 3), ret_value);

    if (ecma_op_to_boolean (call_value))
    {
      *(pass_value_list + pass_num) = ecma_fast_copy_value (get_value);
      pass_num++;
    }

    ECMA_FINALIZE (call_value);

    ecma_fast_free_value (current_index);
    ecma_fast_free_value (get_value);
  }

  if (ecma_is_value_empty (ret_value))
  {
    ecma_value_t new_typedarray = ecma_op_create_typedarray_with_type_and_length (obj_p, pass_num);
    ecma_object_t *new_obj_p = ecma_get_object_from_value (new_typedarray);

    for (uint32_t index = 0; index < pass_num && ecma_is_value_empty (ret_value); index++)
    {
      ecma_value_t set_status = ecma_op_typedarray_set_index_prop (new_obj_p, index, *(pass_value_list + index));

      if (!set_status)
      {
        ret_value = ecma_raise_type_error (ECMA_ERR_MSG ("error in typedarray set"));
      }
    }

    if (ecma_is_value_empty (ret_value))
    {
      ret_value = new_typedarray;
    }
    else
    {
      ecma_free_value (new_typedarray);
    }
  }

  uint32_t index = 0;

  while (index < pass_num)
  {
    ecma_fast_free_value (*(pass_value_list + index));
    index++;
  }

  JMEM_FINALIZE_LOCAL_ARRAY (pass_value_list);

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
  uint32_t len = ecma_typedarray_get_length (obj_p);
  ecma_object_t *arraybuffer_p = ecma_typedarray_get_arraybuffer (obj_p);
  lit_utf8_byte_t *buffer = (ecma_arraybuffer_get_buffer (arraybuffer_p)
                             + ecma_typedarray_get_offset (obj_p));
  uint8_t shift = ecma_typedarray_get_element_size_shift (obj_p);
  uint8_t element_size = (uint8_t) (1 << shift);
  uint32_t middle = (len / 2) << shift;
  uint32_t buffer_last = (len << shift) - element_size;

  for (uint32_t lower = 0; lower < middle; lower += element_size)
  {
    uint32_t upper = buffer_last - lower;
    lit_utf8_byte_t *lower_p = buffer + lower;
    lit_utf8_byte_t *upper_p = buffer + upper;

    lit_utf8_byte_t tmp[8];
    memcpy (&tmp[0], lower_p, element_size);
    memcpy (lower_p, upper_p, element_size);
    memcpy (upper_p, &tmp[0], element_size);
  }

  return ecma_copy_value (this_arg);
} /* ecma_builtin_typedarray_prototype_reverse */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_TYPEDARRAY_BUILTIN */
