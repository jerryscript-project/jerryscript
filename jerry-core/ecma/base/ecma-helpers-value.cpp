/* Copyright 2015 Samsung Electronics Co., Ltd.
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

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

#include "ecma-alloc.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "jrt.h"
#include "jrt-bit-fields.h"

JERRY_STATIC_ASSERT (sizeof (ecma_value_t) * JERRY_BITSINBYTE >= ECMA_VALUE_SIZE);
JERRY_STATIC_ASSERT (sizeof (ecma_completion_value_t) * JERRY_BITSINBYTE >= ECMA_COMPLETION_VALUE_SIZE);

/**
 * Get type field of ecma-value
 *
 * @return type field
 */
static ecma_type_t __attr_pure___
ecma_get_value_type_field (ecma_value_t value) /**< ecma-value */
{
  return (ecma_type_t) jrt_extract_bit_field (value,
                                              ECMA_VALUE_TYPE_POS,
                                              ECMA_VALUE_TYPE_WIDTH);
} /* ecma_get_value_type_field */

/**
 * Get value field of ecma-value
 *
 * @return value field
 */
static uintptr_t __attr_pure___
ecma_get_value_value_field (ecma_value_t value) /**< ecma-value */
{
  return (uintptr_t) jrt_extract_bit_field (value,
                                            ECMA_VALUE_VALUE_POS,
                                            ECMA_VALUE_VALUE_WIDTH);
} /* ecma_get_value_value_field */

/**
 * Set type field of ecma-value
 *
 * @return ecma-value with updated field
 */
static ecma_value_t __attr_pure___
ecma_set_value_type_field (ecma_value_t value, /**< ecma-value to set field in */
                           ecma_type_t type_field) /**< new field value */
{
  return (ecma_value_t) jrt_set_bit_field_value (value,
                                                 type_field,
                                                 ECMA_VALUE_TYPE_POS,
                                                 ECMA_VALUE_TYPE_WIDTH);
} /* ecma_set_value_type_field */

/**
 * Set value field of ecma-value
 *
 * @return ecma-value with updated field
 */
static ecma_value_t __attr_pure___
ecma_set_value_value_field (ecma_value_t value, /**< ecma-value to set field in */
                            uintptr_t value_field) /**< new field value */
{
  return (ecma_value_t) jrt_set_bit_field_value (value,
                                                 value_field,
                                                 ECMA_VALUE_VALUE_POS,
                                                 ECMA_VALUE_VALUE_WIDTH);
} /* ecma_set_value_value_field */

/**
 * Check if the value is empty.
 *
 * @return true - if the value contains implementation-defined empty simple value,
 *         false - otherwise.
 */
bool __attr_pure___ __attr_always_inline___
ecma_is_value_empty (ecma_value_t value) /**< ecma-value */
{
  return (ecma_get_value_type_field (value) == ECMA_TYPE_SIMPLE
          && ecma_get_value_value_field (value) == ECMA_SIMPLE_VALUE_EMPTY);
} /* ecma_is_value_empty */

/**
 * Check if the value is undefined.
 *
 * @return true - if the value contains ecma-undefined simple value,
 *         false - otherwise.
 */
bool __attr_pure___ __attr_always_inline___
ecma_is_value_undefined (ecma_value_t value) /**< ecma-value */
{
  return (ecma_get_value_type_field (value) == ECMA_TYPE_SIMPLE
          && ecma_get_value_value_field (value) == ECMA_SIMPLE_VALUE_UNDEFINED);
} /* ecma_is_value_undefined */

/**
 * Check if the value is null.
 *
 * @return true - if the value contains ecma-null simple value,
 *         false - otherwise.
 */
bool __attr_pure___ __attr_always_inline___
ecma_is_value_null (ecma_value_t value) /**< ecma-value */
{
  return (ecma_get_value_type_field (value) == ECMA_TYPE_SIMPLE
          && ecma_get_value_value_field (value) == ECMA_SIMPLE_VALUE_NULL);
} /* ecma_is_value_null */

/**
 * Check if the value is boolean.
 *
 * @return true - if the value contains ecma-true or ecma-false simple values,
 *         false - otherwise.
 */
bool __attr_pure___ __attr_always_inline___
ecma_is_value_boolean (ecma_value_t value) /**< ecma-value */
{
  return (ecma_get_value_type_field (value) == ECMA_TYPE_SIMPLE
          && (ecma_get_value_value_field (value) == ECMA_SIMPLE_VALUE_TRUE
              || ecma_get_value_value_field (value) == ECMA_SIMPLE_VALUE_FALSE));
} /* ecma_is_value_boolean */

/**
 * Check if the value is true.
 *
 * Warning:
 *         value must be boolean
 *
 * @return true - if the value contains ecma-true simple value,
 *         false - otherwise.
 */
bool __attr_pure___ __attr_always_inline___
ecma_is_value_true (ecma_value_t value) /**< ecma-value */
{
  return (ecma_get_value_type_field (value) == ECMA_TYPE_SIMPLE
          && ecma_get_value_value_field (value) == ECMA_SIMPLE_VALUE_TRUE);
} /* ecma_is_value_true */

/**
 * Check if the value is array hole.
 *
 * @return true - if the value contains ecma-array-hole simple value,
 *         false - otherwise.
 */
bool __attr_pure___ __attr_always_inline___
ecma_is_value_array_hole (ecma_value_t value) /**< ecma-value */
{
  return (ecma_get_value_type_field (value) == ECMA_TYPE_SIMPLE
          && ecma_get_value_value_field (value) == ECMA_SIMPLE_VALUE_ARRAY_HOLE);
} /* ecma_is_value_array_hole */

/**
 * Check if the value is ecma-number.
 *
 * @return true - if the value contains ecma-number value,
 *         false - otherwise.
 */
bool __attr_pure___ __attr_always_inline___
ecma_is_value_number (ecma_value_t value) /**< ecma-value */
{
  return (ecma_get_value_type_field (value) == ECMA_TYPE_NUMBER);
} /* ecma_is_value_number */

/**
 * Check if the value is ecma-string.
 *
 * @return true - if the value contains ecma-string value,
 *         false - otherwise.
 */
bool __attr_pure___ __attr_always_inline___
ecma_is_value_string (ecma_value_t value) /**< ecma-value */
{
  return (ecma_get_value_type_field (value) == ECMA_TYPE_STRING);
} /* ecma_is_value_string */

/**
 * Check if the value is object.
 *
 * @return true - if the value contains object value,
 *         false - otherwise.
 */
bool __attr_pure___ __attr_always_inline___
ecma_is_value_object (ecma_value_t value) /**< ecma-value */
{
  return (ecma_get_value_type_field (value) == ECMA_TYPE_OBJECT);
} /* ecma_is_value_object */

/**
 * Debug assertion that specified value's type is one of ECMA-defined
 * script-visible types, i.e.: undefined, null, boolean, number, string, object.
 */
void
ecma_check_value_type_is_spec_defined (ecma_value_t value) /**< ecma-value */
{
  JERRY_ASSERT (ecma_is_value_undefined (value)
                || ecma_is_value_null (value)
                || ecma_is_value_boolean (value)
                || ecma_is_value_number (value)
                || ecma_is_value_string (value)
                || ecma_is_value_object (value));
} /* ecma_check_value_type_is_spec_defined */

/**
 * Simple value constructor
 */
ecma_value_t __attr_const___ __attr_always_inline___
ecma_make_simple_value (const ecma_simple_value_t value) /**< simple value */
{
  ecma_value_t ret_value = 0;

  ret_value = ecma_set_value_type_field (ret_value, ECMA_TYPE_SIMPLE);
  ret_value = ecma_set_value_value_field (ret_value, value);

  return ret_value;
} /* ecma_make_simple_value */

/**
 * Number value constructor
 */
ecma_value_t __attr_const___
ecma_make_number_value (const ecma_number_t* num_p) /**< number to reference in value */
{
  JERRY_ASSERT (num_p != NULL);

  mem_cpointer_t num_cp;
  ECMA_SET_NON_NULL_POINTER (num_cp, num_p);

  ecma_value_t ret_value = 0;

  ret_value = ecma_set_value_type_field (ret_value, ECMA_TYPE_NUMBER);
  ret_value = ecma_set_value_value_field (ret_value, num_cp);

  return ret_value;
} /* ecma_make_number_value */

/**
 * String value constructor
 */
ecma_value_t __attr_const___
ecma_make_string_value (const ecma_string_t* ecma_string_p) /**< string to reference in value */
{
  JERRY_ASSERT (ecma_string_p != NULL);

  mem_cpointer_t string_cp;
  ECMA_SET_NON_NULL_POINTER (string_cp, ecma_string_p);

  ecma_value_t ret_value = 0;

  ret_value = ecma_set_value_type_field (ret_value, ECMA_TYPE_STRING);
  ret_value = ecma_set_value_value_field (ret_value, string_cp);

  return ret_value;
} /* ecma_make_string_value */

/**
 * object value constructor
 */
ecma_value_t __attr_const___
ecma_make_object_value (const ecma_object_t* object_p) /**< object to reference in value */
{
  JERRY_ASSERT (object_p != NULL);

  mem_cpointer_t object_cp;
  ECMA_SET_NON_NULL_POINTER (object_cp, object_p);

  ecma_value_t ret_value = 0;

  ret_value = ecma_set_value_type_field (ret_value, ECMA_TYPE_OBJECT);
  ret_value = ecma_set_value_value_field (ret_value, object_cp);

  return ret_value;
} /* ecma_make_object_value */

/**
 * Get pointer to ecma-number from ecma-value
 *
 * @return the pointer
 */
ecma_number_t* __attr_pure___
ecma_get_number_from_value (ecma_value_t value) /**< ecma-value */
{
  JERRY_ASSERT (ecma_get_value_type_field (value) == ECMA_TYPE_NUMBER);

  return ECMA_GET_NON_NULL_POINTER (ecma_number_t,
                                    ecma_get_value_value_field (value));
} /* ecma_get_number_from_value */

/**
 * Get pointer to ecma-string from ecma-value
 *
 * @return the pointer
 */
ecma_string_t* __attr_pure___
ecma_get_string_from_value (ecma_value_t value) /**< ecma-value */
{
  JERRY_ASSERT (ecma_get_value_type_field (value) == ECMA_TYPE_STRING);

  return ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                    ecma_get_value_value_field (value));
} /* ecma_get_string_from_value */

/**
 * Get pointer to ecma-object from ecma-value
 *
 * @return the pointer
 */
ecma_object_t* __attr_pure___
ecma_get_object_from_value (ecma_value_t value) /**< ecma-value */
{
  JERRY_ASSERT (ecma_get_value_type_field (value) == ECMA_TYPE_OBJECT);

  return ECMA_GET_NON_NULL_POINTER (ecma_object_t,
                                    ecma_get_value_value_field (value));
} /* ecma_get_object_from_value */

/**
 * Copy ecma-value.
 *
 * Note:
 *  Operation algorithm.
 *   switch (valuetype)
 *    case simple:
 *      simply return the value as it was passed;
 *    case number:
 *      copy the number
 *      and return new ecma-value
 *      pointing to copy of the number;
 *    case string:
 *      increase reference counter of the string
 *      and return the value as it was passed.
 *    case object;
 *      increase reference counter of the object if do_ref_if_object is true
 *      and return the value as it was passed.
 *
 * @return See note.
 */
ecma_value_t
ecma_copy_value (ecma_value_t value, /**< ecma-value */
                 bool do_ref_if_object) /**< if the value is object value,
                                             increment reference counter of the object */
{
  ecma_value_t value_copy = 0;

  switch (ecma_get_value_type_field (value))
  {
    case ECMA_TYPE_SIMPLE:
    {
      value_copy = value;

      break;
    }
    case ECMA_TYPE_NUMBER:
    {
      ecma_number_t *num_p = ecma_get_number_from_value (value);

      ecma_number_t *number_copy_p = ecma_alloc_number ();
      *number_copy_p = *num_p;

      value_copy = ecma_make_number_value (number_copy_p);

      break;
    }
    case ECMA_TYPE_STRING:
    {
      ecma_string_t *string_p = ecma_get_string_from_value (value);

      string_p = ecma_copy_or_ref_ecma_string (string_p);

      value_copy = ecma_make_string_value (string_p);

      break;
    }
    case ECMA_TYPE_OBJECT:
    {
      ecma_object_t *obj_p = ecma_get_object_from_value (value);

      if (do_ref_if_object)
      {
        ecma_ref_object (obj_p);
      }

      value_copy = value;

      break;
    }
  }

  return value_copy;
} /* ecma_copy_value */

/**
 * Free the ecma-value
 */
void
ecma_free_value (ecma_value_t value, /**< value description */
                 bool do_deref_if_object) /**< if the value is object value,
                                               decrement reference counter of the object */
{
  switch (ecma_get_value_type_field (value))
  {
    case ECMA_TYPE_SIMPLE:
    {
      /* doesn't hold additional memory */
      break;
    }

    case ECMA_TYPE_NUMBER:
    {
      ecma_number_t *number_p = ecma_get_number_from_value (value);
      ecma_dealloc_number (number_p);
      break;
    }

    case ECMA_TYPE_STRING:
    {
      ecma_string_t *string_p = ecma_get_string_from_value (value);
      ecma_deref_ecma_string (string_p);
      break;
    }

    case ECMA_TYPE_OBJECT:
    {
      if (do_deref_if_object)
      {
        ecma_deref_object (ecma_get_object_from_value (value));
      }
      break;
    }
  }
} /* ecma_free_value */

/**
 * Get type field of completion value
 *
 * @return type field
 */
static ecma_completion_type_t __attr_const___
ecma_get_completion_value_type_field (ecma_completion_value_t completion_value) /**< completion value */
{
  return (ecma_completion_type_t) jrt_extract_bit_field (completion_value,
                                                         ECMA_COMPLETION_VALUE_TYPE_POS,
                                                         ECMA_COMPLETION_VALUE_TYPE_WIDTH);
} /* ecma_get_completion_value_type_field */

/**
 * Get value field of completion value
 *
 * @return value field
 */
static ecma_value_t __attr_const___
ecma_get_completion_value_value_field (ecma_completion_value_t completion_value) /**< completion value */
{
  return (ecma_value_t) jrt_extract_bit_field (completion_value,
                                               ECMA_COMPLETION_VALUE_VALUE_POS,
                                               ECMA_COMPLETION_VALUE_VALUE_WIDTH);
} /* ecma_get_completion_value_value_field */

/**
 * Get target of break / continue completion value
 *
 * @return instruction counter
 */
static vm_instr_counter_t
ecma_get_completion_value_target (ecma_completion_value_t completion_value) /**< completion value */
{
  return (vm_instr_counter_t) jrt_extract_bit_field (completion_value,
                                                     ECMA_COMPLETION_VALUE_TARGET_POS,
                                                     ECMA_COMPLETION_VALUE_TARGET_WIDTH);
} /* ecma_get_completion_value_target */

/**
 * Set type field of completion value
 *
 * @return completion value with updated field
 */
static ecma_completion_value_t __attr_const___
ecma_set_completion_value_type_field (ecma_completion_value_t completion_value, /**< completion value
                                                                                 * to set field in */
                                      ecma_completion_type_t type_field) /**< new field value */
{
  return (ecma_completion_value_t) jrt_set_bit_field_value (completion_value,
                                                            type_field,
                                                            ECMA_COMPLETION_VALUE_TYPE_POS,
                                                            ECMA_COMPLETION_VALUE_TYPE_WIDTH);
} /* ecma_set_completion_value_type_field */

/**
 * Set value field of completion value
 *
 * @return completion value with updated field
 */
static ecma_completion_value_t __attr_pure___
ecma_set_completion_value_value_field (ecma_completion_value_t completion_value, /**< completion value
                                                                                  * to set field in */
                                       ecma_value_t value_field) /**< new field value */
{
  return (ecma_completion_value_t) jrt_set_bit_field_value (completion_value,
                                                            value_field,
                                                            ECMA_COMPLETION_VALUE_VALUE_POS,
                                                            ECMA_COMPLETION_VALUE_VALUE_WIDTH);
} /* ecma_set_completion_value_value_field */

/**
 * Set target of break / continue completion value
 *
 * @return completion value with updated field
 */
static ecma_completion_value_t __attr_const___
ecma_set_completion_value_target (ecma_completion_value_t completion_value, /**< completion value
                                                                             * to set field in */
                                  vm_instr_counter_t target) /**< break / continue target */
{
  return (ecma_completion_value_t) jrt_set_bit_field_value (completion_value,
                                                            target,
                                                            ECMA_COMPLETION_VALUE_TARGET_POS,
                                                            ECMA_COMPLETION_VALUE_TARGET_WIDTH);
} /* ecma_set_completion_value_target */

/**
 * Normal, throw, return, exit and meta completion values constructor
 *
 * @return completion value
 */
ecma_completion_value_t __attr_pure___ __attr_always_inline___
ecma_make_completion_value (ecma_completion_type_t type, /**< type */
                            ecma_value_t value) /**< value */
{
  const bool is_type_ok = (type == ECMA_COMPLETION_TYPE_NORMAL
                     || type == ECMA_COMPLETION_TYPE_THROW
                     || type == ECMA_COMPLETION_TYPE_RETURN
                     || (type == ECMA_COMPLETION_TYPE_META
                         && ecma_is_value_empty (value)));

  JERRY_ASSERT (is_type_ok);

  ecma_completion_value_t completion_value = 0;

  completion_value = ecma_set_completion_value_type_field (completion_value,
                                                           type);
  completion_value = ecma_set_completion_value_value_field (completion_value,
                                                            value);

  return completion_value;
} /* ecma_make_completion_value */

/**
 * Simple normal completion value constructor
 *
 * @return completion value
 */
ecma_completion_value_t __attr_const___ __attr_always_inline___
ecma_make_simple_completion_value (ecma_simple_value_t simple_value) /**< simple ecma-value */
{
  JERRY_ASSERT (simple_value == ECMA_SIMPLE_VALUE_UNDEFINED
                || simple_value == ECMA_SIMPLE_VALUE_NULL
                || simple_value == ECMA_SIMPLE_VALUE_FALSE
                || simple_value == ECMA_SIMPLE_VALUE_TRUE);

  return ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
                                     ecma_make_simple_value (simple_value));
} /* ecma_make_simple_completion_value */

/**
 * Normal completion value constructor
 *
 * @return completion value
 */
ecma_completion_value_t __attr_pure___ __attr_always_inline___
ecma_make_normal_completion_value (ecma_value_t value) /**< value */
{
  return ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL, value);
} /* ecma_make_normal_completion_value */

/**
 * Throw completion value constructor
 *
 * @return completion value
 */
ecma_completion_value_t __attr_pure___ __attr_always_inline___
ecma_make_throw_completion_value (ecma_value_t value) /**< value */
{
  return ecma_make_completion_value (ECMA_COMPLETION_TYPE_THROW, value);
} /* ecma_make_throw_completion_value */

/**
 * Throw completion value constructor.
 *
 * @return 'throw' completion value
 */
ecma_completion_value_t __attr_const___
ecma_make_throw_obj_completion_value (ecma_object_t *exception_p) /**< an object */
{
  JERRY_ASSERT (exception_p != NULL
                && !ecma_is_lexical_environment (exception_p));

  ecma_value_t exception = ecma_make_object_value (exception_p);

  return ecma_make_throw_completion_value (exception);
} /* ecma_make_throw_obj_completion_value */

/**
 * Empty completion value constructor.
 *
 * @return (normal, empty, reserved) completion value.
 */
ecma_completion_value_t __attr_const___ __attr_always_inline___
ecma_make_empty_completion_value (void)
{
  return ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL,
                                     ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY));
} /* ecma_make_empty_completion_value */

/**
 * Return completion value constructor
 *
 * @return completion value
 */
ecma_completion_value_t __attr_pure___ __attr_always_inline___
ecma_make_return_completion_value (ecma_value_t value) /**< value */
{
  return ecma_make_completion_value (ECMA_COMPLETION_TYPE_RETURN, value);
} /* ecma_make_return_completion_value */

/**
 * Meta completion value constructor
 *
 * @return completion value
 */
ecma_completion_value_t __attr_const___ __attr_always_inline___
ecma_make_meta_completion_value (void)
{
  return ecma_make_completion_value (ECMA_COMPLETION_TYPE_META,
                                     ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY));
} /* ecma_make_meta_completion_value */

/**
 * Break / continue completion values constructor
 *
 * @return completion value
 */
ecma_completion_value_t __attr_const___
ecma_make_jump_completion_value (vm_instr_counter_t target) /**< target break / continue */
{
  ecma_completion_value_t completion_value = 0;

  completion_value = ecma_set_completion_value_type_field (completion_value,
                                                           ECMA_COMPLETION_TYPE_JUMP);
  completion_value = ecma_set_completion_value_target (completion_value,
                                                       target);

  return completion_value;
} /* ecma_make_jump_completion_value */

/**
 * Get ecma-value from specified completion value
 *
 * @return ecma-value
 */
ecma_value_t __attr_const___ __attr_always_inline___
ecma_get_completion_value_value (ecma_completion_value_t completion_value) /**< completion value */
{
  const ecma_completion_type_t type = ecma_get_completion_value_type_field (completion_value);

  const bool is_type_ok = (type == ECMA_COMPLETION_TYPE_NORMAL
                           || type == ECMA_COMPLETION_TYPE_THROW
                           || type == ECMA_COMPLETION_TYPE_RETURN);

  JERRY_ASSERT (is_type_ok);

  return ecma_get_completion_value_value_field (completion_value);
} /* ecma_get_completion_value_value */

/**
 * Get pointer to ecma-number from completion value
 *
 * @return pointer
 */
ecma_number_t* __attr_const___
ecma_get_number_from_completion_value (ecma_completion_value_t completion_value) /**< completion value */
{
  return ecma_get_number_from_value (ecma_get_completion_value_value (completion_value));
} /* ecma_get_number_from_completion_value */

/**
 * Get pointer to ecma-string from completion value
 *
 * @return pointer
 */
ecma_string_t* __attr_const___
ecma_get_string_from_completion_value (ecma_completion_value_t completion_value) /**< completion value */
{
  return ecma_get_string_from_value (ecma_get_completion_value_value (completion_value));
} /* ecma_get_string_from_completion_value */

/**
 * Get pointer to ecma-object from completion value
 *
 * @return pointer
 */
ecma_object_t* __attr_const___
ecma_get_object_from_completion_value (ecma_completion_value_t completion_value) /**< completion value */
{
  return ecma_get_object_from_value (ecma_get_completion_value_value (completion_value));
} /* ecma_get_object_from_completion_value */

/**
 * Get break / continue target from completion value
 *
 * @return instruction counter
 */
vm_instr_counter_t
ecma_get_jump_target_from_completion_value (ecma_completion_value_t completion_value) /**< completion
                                                                                       *   value */
{
  JERRY_ASSERT (ecma_get_completion_value_type_field (completion_value) == ECMA_COMPLETION_TYPE_JUMP);

  return ecma_get_completion_value_target (completion_value);
} /* ecma_get_jump_target_from_completion_value */

/**
 * Copy ecma-completion value.
 *
 * @return (source.type, ecma_copy_value (source.value), source.target).
 */
ecma_completion_value_t
ecma_copy_completion_value (ecma_completion_value_t value) /**< completion value */
{
  const ecma_completion_type_t type = ecma_get_completion_value_type_field (value);
  const bool is_type_ok = (type == ECMA_COMPLETION_TYPE_NORMAL
                           || type == ECMA_COMPLETION_TYPE_THROW
                           || type == ECMA_COMPLETION_TYPE_RETURN
                           || type == ECMA_COMPLETION_TYPE_JUMP);

  JERRY_ASSERT (is_type_ok);

  return ecma_make_completion_value (type,
                                     ecma_copy_value (ecma_get_completion_value_value_field (value),
                                                      true));
} /* ecma_copy_completion_value */

/**
 * Free the completion value.
 */
void
ecma_free_completion_value (ecma_completion_value_t completion_value) /**< completion value */
{
  switch (ecma_get_completion_value_type_field (completion_value))
  {
    case ECMA_COMPLETION_TYPE_NORMAL:
    case ECMA_COMPLETION_TYPE_THROW:
    case ECMA_COMPLETION_TYPE_RETURN:
    {
      ecma_value_t v = ecma_get_completion_value_value_field (completion_value);
      ecma_free_value (v, true);
      break;
    }
    case ECMA_COMPLETION_TYPE_JUMP:
    {
      break;
    }
    case ECMA_COMPLETION_TYPE_META:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* ecma_free_completion_value */

/**
 * Check if the completion value is normal value.
 *
 * @return true - if the completion type is normal,
 *         false - otherwise.
 */
bool __attr_const___ __attr_always_inline___
ecma_is_completion_value_normal (ecma_completion_value_t value) /**< completion value */
{
  return (ecma_get_completion_value_type_field (value) == ECMA_COMPLETION_TYPE_NORMAL);
} /* ecma_is_completion_value_normal */

/**
 * Check if the completion value is throw value.
 *
 * @return true - if the completion type is throw,
 *         false - otherwise.
 */
bool __attr_const___ __attr_always_inline___
ecma_is_completion_value_throw (ecma_completion_value_t value) /**< completion value */
{
  return (ecma_get_completion_value_type_field (value) == ECMA_COMPLETION_TYPE_THROW);
} /* ecma_is_completion_value_throw */

/**
 * Check if the completion value is return value.
 *
 * @return true - if the completion type is return,
 *         false - otherwise.
 */
bool __attr_const___ __attr_always_inline___
ecma_is_completion_value_return (ecma_completion_value_t value) /**< completion value */
{
  return (ecma_get_completion_value_type_field (value) == ECMA_COMPLETION_TYPE_RETURN);
} /* ecma_is_completion_value_return */

/**
 * Check if the completion value is meta value.
 *
 * @return true - if the completion type is meta,
 *         false - otherwise.
 */
bool __attr_const___ __attr_always_inline___
ecma_is_completion_value_meta (ecma_completion_value_t value) /**< completion value */
{
  if (ecma_get_completion_value_type_field (value) == ECMA_COMPLETION_TYPE_META)
  {
    JERRY_ASSERT (ecma_is_value_empty (ecma_get_completion_value_value_field (value)));

    return true;
  }
  else
  {
    return false;
  }
} /* ecma_is_completion_value_meta */

/**
 * Check if the completion value is break / continue value.
 *
 * @return true - if the completion type is break / continue,
 *         false - otherwise.
 */
bool __attr_const___ __attr_always_inline___
ecma_is_completion_value_jump (ecma_completion_value_t value) /**< completion value */
{
  return (ecma_get_completion_value_type_field (value) == ECMA_COMPLETION_TYPE_JUMP);
} /* ecma_is_completion_value_jump */

/**
 * Check if the completion value is specified normal simple value.
 *
 * @return true - if the completion type is normal and
 *                value contains specified simple ecma-value,
 *         false - otherwise.
 */
bool __attr_const___ __attr_always_inline___
ecma_is_completion_value_normal_simple_value (ecma_completion_value_t value, /**< completion value */
                                              ecma_simple_value_t simple_value) /**< simple value to check
                                                                                     for equality with */
{
  return (value == ecma_make_simple_completion_value (simple_value));
} /* ecma_is_completion_value_normal_simple_value */

/**
 * Check if the completion value is normal true.
 *
 * @return true - if the completion type is normal and
 *                value contains ecma-true simple value,
 *         false - otherwise.
 */
bool __attr_const___ __attr_always_inline___
ecma_is_completion_value_normal_true (ecma_completion_value_t value) /**< completion value */
{
  return ecma_is_completion_value_normal_simple_value (value, ECMA_SIMPLE_VALUE_TRUE);
} /* ecma_is_completion_value_normal_true */

/**
 * Check if the completion value is normal false.
 *
 * @return true - if the completion type is normal and
 *                value contains ecma-false simple value,
 *         false - otherwise.
 */
bool __attr_const___ __attr_always_inline___
ecma_is_completion_value_normal_false (ecma_completion_value_t value) /**< completion value */
{
  return ecma_is_completion_value_normal_simple_value (value, ECMA_SIMPLE_VALUE_FALSE);
} /* ecma_is_completion_value_normal_false */

/**
 * Check if the completion value is normal empty value.
 *
 * @return true - if the completion type is normal and
 *                value contains empty simple value,
 *         false - otherwise.
 */
bool __attr_const___ __attr_always_inline___
ecma_is_completion_value_empty (ecma_completion_value_t value) /**< completion value */
{
  return (ecma_is_completion_value_normal (value)
          && ecma_is_value_empty (ecma_get_completion_value_value_field (value)));
} /* ecma_is_completion_value_empty */

/**
 * @}
 * @}
 */
