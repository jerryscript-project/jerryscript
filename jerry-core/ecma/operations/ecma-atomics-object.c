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

#include "ecma-atomics-object.h"

#include "ecma-arraybuffer-object.h"
#include "ecma-bigint.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-shared-arraybuffer-object.h"
#include "ecma-typedarray-object.h"

#include "jcontext.h"
#include "jmem.h"

#if JERRY_BUILTIN_ATOMICS

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaatomicsobject ECMA builtin Atomics helper functions
 * @{
 */

/**
 * Atomics validate integer typedArray
 *
 * See also: ES12 25.4.1.1
 *
 * @return ecma value
 */
ecma_value_t
ecma_validate_integer_typedarray (ecma_value_t typedarray, /**< typedArray argument */
                                  bool waitable) /**< waitable argument */
{
  /* 2. */
  if (!ecma_is_typedarray (typedarray))
  {
    return ecma_raise_type_error (ECMA_ERR_ARGUMENT_THIS_NOT_TYPED_ARRAY);
  }

  /* 3-4. */
  ecma_object_t *typedarray_p = ecma_get_object_from_value (typedarray);
  ecma_typedarray_info_t target_info = ecma_typedarray_get_info (typedarray_p);

  if (ECMA_ARRAYBUFFER_LAZY_ALLOC (target_info.array_buffer_p))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 5-6. */
  if (waitable)
  {
    if (!(target_info.id == ECMA_BIGINT64_ARRAY || target_info.id == ECMA_INT32_ARRAY))
    {
      return ecma_raise_type_error (ECMA_ERR_ARGUMENT_NOT_SUPPORTED);
    }
  }
  else
  {
    if (target_info.id == ECMA_UINT8_CLAMPED_ARRAY || target_info.id == ECMA_FLOAT32_ARRAY
        || target_info.id == ECMA_FLOAT64_ARRAY)
    {
      return ecma_raise_type_error (ECMA_ERR_ARGUMENT_NOT_SUPPORTED);
    }
  }

  /* 7. */
  JERRY_ASSERT (target_info.array_buffer_p != NULL);

  /* 8-10. */
  ecma_object_t *buffer = ecma_typedarray_get_arraybuffer (typedarray_p);

  if (!ecma_object_class_is (buffer, ECMA_OBJECT_CLASS_SHARED_ARRAY_BUFFER)
      && !ecma_object_class_is (buffer, ECMA_OBJECT_CLASS_ARRAY_BUFFER))
  {
    return ecma_raise_type_error (ECMA_ERR_ARGUMENT_NOT_ARRAY_BUFFER);
  }

  return ecma_make_object_value (buffer);
} /* ecma_validate_integer_typedarray */

/**
 * Atomics validate Atomic Access
 *
 * See also: ES11 24.4.1.2
 *
 * @return ecma value
 */
uint32_t
ecma_validate_atomic_access (ecma_value_t typedarray, /**< typedArray argument */
                             ecma_value_t request_index) /**< request_index argument */
{
  /* 1. */
  JERRY_ASSERT (ecma_is_value_object (typedarray)
                && ecma_typedarray_get_arraybuffer (ecma_get_object_from_value (typedarray)) != NULL);

  ecma_object_t *typedarray_p = ecma_get_object_from_value (typedarray);

  /* 2. */
  ecma_number_t access_index;
  if (ECMA_IS_VALUE_ERROR (ecma_op_to_index (request_index, &access_index)))
  {
    return ECMA_STRING_NOT_ARRAY_INDEX;
  }

  /* 3. */
  ecma_typedarray_info_t target_info = ecma_typedarray_get_info (typedarray_p);

  if (ECMA_ARRAYBUFFER_LAZY_ALLOC (target_info.array_buffer_p))
  {
    return ECMA_STRING_NOT_ARRAY_INDEX;
  }

  /* 4. */
  JERRY_ASSERT (access_index >= 0);

  /* 5-6. */
  if (JERRY_UNLIKELY (access_index >= target_info.length))
  {
    ecma_raise_range_error (ECMA_ERR_INVALID_LENGTH);
    return ECMA_STRING_NOT_ARRAY_INDEX;
  }

  return (uint32_t) access_index;
} /* ecma_validate_atomic_access */

/**
 * Atomics read, modify, write
 *
 * See also: ES11 24.4.1.11
 *
 * @return ecma value
 */
ecma_value_t
ecma_atomic_read_modify_write (ecma_value_t typedarray, /**< typedArray argument */
                               ecma_value_t index, /**< index argument */
                               ecma_value_t value, /**< value argument */
                               ecma_atomics_op_t op) /**< operation argument */
{
  /* 1. */
  ecma_value_t buffer = ecma_validate_integer_typedarray (typedarray, false);

  if (ECMA_IS_VALUE_ERROR (buffer))
  {
    return buffer;
  }

  /* 2. */
  uint32_t idx = ecma_validate_atomic_access (typedarray, index);

  if (idx == ECMA_STRING_NOT_ARRAY_INDEX)
  {
    return ECMA_VALUE_ERROR;
  }

  /* 3. */
  ecma_object_t *typedarray_p = ecma_get_object_from_value (typedarray);
  ecma_typedarray_info_t target_info = ecma_typedarray_get_info (typedarray_p);

  if (ECMA_ARRAYBUFFER_LAZY_ALLOC (target_info.array_buffer_p))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_typedarray_type_t element_type = target_info.id;

  /* 4-5. */
  ecma_value_t val = ECMA_VALUE_ERROR;
  ecma_number_t tmp;
#if JERRY_BUILTIN_BIGINT
  if (element_type == ECMA_BIGINT64_ARRAY || element_type == ECMA_BIGUINT64_ARRAY)
  {
    val = ecma_bigint_to_bigint (value, false);
  }
  else
#endif /* JERRY_BUILTIN_BIGINT */
    if (!ECMA_IS_VALUE_ERROR (ecma_op_to_integer (value, &tmp)))
    {
      val = ecma_make_number_value (tmp);
    }

  if (ECMA_IS_VALUE_ERROR (val))
  {
    return val;
  }

  /* 6. */
  uint8_t element_size = target_info.element_size;

  /* 8. */
  uint32_t offset = target_info.offset;

  /* 9. */
  uint32_t indexed_position = idx * element_size + offset;

  /* 10. */
  return ecma_arraybuffer_get_modify_set_value_in_buffer (buffer,
                                                          indexed_position,
                                                          val,
                                                          op,
                                                          element_type,
                                                          ecma_get_typedarray_getter_fn (element_type),
                                                          ecma_get_typedarray_setter_fn (element_type));
} /* ecma_atomic_read_modify_write */

/**
 * Atomics load
 *
 * See also: ES12 25.4.7
 *
 * @return ecma value
 */
ecma_value_t
ecma_atomic_load (ecma_value_t typedarray, /**< typedArray argument */
                  ecma_value_t index) /**< index argument */
{
  ecma_value_t buffer = ecma_validate_integer_typedarray (typedarray, false);

  if (ECMA_IS_VALUE_ERROR (buffer))
  {
    return buffer;
  }

  if (ecma_arraybuffer_is_detached (ecma_get_object_from_value (buffer)))
  {
    ecma_raise_type_error (ECMA_ERR_ARRAYBUFFER_IS_DETACHED);
  }

  /* 2. */
  uint32_t idx = ecma_validate_atomic_access (typedarray, index);

  if (idx == ECMA_STRING_NOT_ARRAY_INDEX)
  {
    return ECMA_VALUE_ERROR;
  }

  /* 3. */
  ecma_object_t *typedarray_p = ecma_get_object_from_value (typedarray);
  ecma_typedarray_info_t target_info = ecma_typedarray_get_info (typedarray_p);

  if (ECMA_ARRAYBUFFER_LAZY_ALLOC (target_info.array_buffer_p))
  {
    return ECMA_VALUE_ERROR;
  }

  /* 4. */
  uint8_t element_size = target_info.element_size;

  /* 5. */
  ecma_typedarray_type_t element_type = target_info.id;

  /* 6. */
  uint32_t offset = target_info.offset;

  /* 7. */
  uint32_t indexed_position = idx * element_size + offset;

  ecma_typedarray_getter_fn_t typedarray_getter_cb = ecma_get_typedarray_getter_fn (element_type);
  uint8_t *buffer_p = ecma_arraybuffer_get_buffer (ecma_get_object_from_value (buffer));

  return typedarray_getter_cb (buffer_p + indexed_position);
} /* ecma_atomic_load */

/**
 * @}
 * @}
 */
#endif /* JERRY_BUILTIN_ATOMICS */
