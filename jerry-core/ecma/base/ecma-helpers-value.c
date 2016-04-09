/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "jrt.h"
#include "jrt-bit-fields.h"
#include "vm-defines.h"

JERRY_STATIC_ASSERT (sizeof (ecma_value_t) * JERRY_BITSINBYTE >= ECMA_VALUE_SIZE,
                     bits_in_ecma_value_t_must_be_greater_than_or_equal_to_ECMA_VALUE_SIZE);

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

/**
 * Get type field of ecma value
 *
 * @return type field
 */
ecma_type_t __attr_pure___
ecma_get_value_type_field (ecma_value_t value) /**< ecma value */
{
  return (ecma_type_t) JRT_EXTRACT_BIT_FIELD (ecma_value_t, value,
                                              ECMA_VALUE_TYPE_POS,
                                              ECMA_VALUE_TYPE_WIDTH);
} /* ecma_get_value_type_field */

/**
 * Get value field of ecma value
 *
 * @return value field
 */
static uintptr_t __attr_pure___
ecma_get_value_value_field (ecma_value_t value) /**< ecma value */
{
  return (uintptr_t) JRT_EXTRACT_BIT_FIELD (ecma_value_t, value,
                                            ECMA_VALUE_VALUE_POS,
                                            ECMA_VALUE_VALUE_WIDTH);
} /* ecma_get_value_value_field */

/**
 * Set type field of ecma value
 *
 * @return ecma value with updated field
 */
static ecma_value_t __attr_pure___
ecma_set_value_type_field (ecma_value_t value, /**< ecma value to set field in */
                           ecma_type_t type_field) /**< new field value */
{
  return JRT_SET_BIT_FIELD_VALUE (ecma_value_t, value,
                                  type_field,
                                  ECMA_VALUE_TYPE_POS,
                                  ECMA_VALUE_TYPE_WIDTH);
} /* ecma_set_value_type_field */

/**
 * Set value field of ecma value
 *
 * @return ecma value with updated field
 */
static ecma_value_t __attr_pure___
ecma_set_value_value_field (ecma_value_t value, /**< ecma value to set field in */
                            uintptr_t value_field) /**< new field value */
{
  return JRT_SET_BIT_FIELD_VALUE (ecma_value_t, value,
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
inline bool __attr_pure___ __attr_always_inline___
ecma_is_value_empty (ecma_value_t value) /**< ecma value */
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
inline bool __attr_pure___ __attr_always_inline___
ecma_is_value_undefined (ecma_value_t value) /**< ecma value */
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
inline bool __attr_pure___ __attr_always_inline___
ecma_is_value_null (ecma_value_t value) /**< ecma value */
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
inline bool __attr_pure___ __attr_always_inline___
ecma_is_value_boolean (ecma_value_t value) /**< ecma value */
{
  return (ecma_get_value_type_field (value) == ECMA_TYPE_SIMPLE
          && (ecma_get_value_value_field (value) == ECMA_SIMPLE_VALUE_TRUE
              || ecma_get_value_value_field (value) == ECMA_SIMPLE_VALUE_FALSE));
} /* ecma_is_value_boolean */

/**
 * Check if the value is true.
 *
 * @return true - if the value contains ecma-true simple value,
 *         false - otherwise.
 */
inline bool __attr_pure___ __attr_always_inline___
ecma_is_value_true (ecma_value_t value) /**< ecma value */
{
  return (ecma_get_value_type_field (value) == ECMA_TYPE_SIMPLE
          && ecma_get_value_value_field (value) == ECMA_SIMPLE_VALUE_TRUE);
} /* ecma_is_value_true */

/**
 * Check if the value is false.
 *
 * @return true - if the value contains ecma-false simple value,
 *         false - otherwise.
 */
inline bool __attr_pure___ __attr_always_inline___
ecma_is_value_false (ecma_value_t value) /**< ecma value */
{
  return (ecma_get_value_type_field (value) == ECMA_TYPE_SIMPLE
          && ecma_get_value_value_field (value) == ECMA_SIMPLE_VALUE_FALSE);
} /* ecma_is_value_false */

/**
 * Check if the value is array hole.
 *
 * @return true - if the value contains ecma-array-hole simple value,
 *         false - otherwise.
 */
inline bool __attr_pure___ __attr_always_inline___
ecma_is_value_array_hole (ecma_value_t value) /**< ecma value */
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
inline bool __attr_pure___ __attr_always_inline___
ecma_is_value_number (ecma_value_t value) /**< ecma value */
{
  return (ecma_get_value_type_field (value) == ECMA_TYPE_NUMBER);
} /* ecma_is_value_number */

/**
 * Check if the value is ecma-string.
 *
 * @return true - if the value contains ecma-string value,
 *         false - otherwise.
 */
inline bool __attr_pure___ __attr_always_inline___
ecma_is_value_string (ecma_value_t value) /**< ecma value */
{
  return (ecma_get_value_type_field (value) == ECMA_TYPE_STRING);
} /* ecma_is_value_string */

/**
 * Check if the value is object.
 *
 * @return true - if the value contains object value,
 *         false - otherwise.
 */
inline bool __attr_pure___ __attr_always_inline___
ecma_is_value_object (ecma_value_t value) /**< ecma value */
{
  return (ecma_get_value_type_field (value) == ECMA_TYPE_OBJECT);
} /* ecma_is_value_object */

/**
 * Check if the value is an error value.
 *
 * @return true - if the value contains an error value,
 *         false - otherwise.
 */
inline bool __attr_pure___ __attr_always_inline___
ecma_is_value_error (ecma_value_t value) /**< ecma value */
{
  return (value & (1u << ECMA_VALUE_ERROR_POS)) != 0;
} /* ecma_is_value_error */

/**
 * Debug assertion that specified value's type is one of ECMA-defined
 * script-visible types, i.e.: undefined, null, boolean, number, string, object.
 */
void
ecma_check_value_type_is_spec_defined (ecma_value_t value) /**< ecma value */
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
inline ecma_value_t __attr_const___ __attr_always_inline___
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
ecma_make_number_value (const ecma_number_t *num_p) /**< number to reference in value */
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
ecma_make_string_value (const ecma_string_t *ecma_string_p) /**< string to reference in value */
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
 * Object value constructor
 */
ecma_value_t __attr_const___
ecma_make_object_value (const ecma_object_t *object_p) /**< object to reference in value */
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
 * Error value constructor
 */
ecma_value_t __attr_const___
ecma_make_error_value (ecma_value_t value) /**< original ecma value */
{
  /* Error values cannot be converted. */
  JERRY_ASSERT (!ecma_is_value_error (value));

  return value | (1u << ECMA_VALUE_ERROR_POS);
} /* ecma_make_error_value */


/**
  * Error value constructor
  */
ecma_value_t __attr_const___
ecma_make_error_obj_value (const ecma_object_t *object_p) /**< object to reference in value */
{
  return ecma_make_error_value (ecma_make_object_value (object_p));
} /* ecma_make_error_obj_value */

/**
 * Get pointer to ecma-number from ecma value
 *
 * @return the pointer
 */
ecma_number_t *__attr_pure___
ecma_get_number_from_value (ecma_value_t value) /**< ecma value */
{
  JERRY_ASSERT (ecma_get_value_type_field (value) == ECMA_TYPE_NUMBER);

  return ECMA_GET_NON_NULL_POINTER (ecma_number_t,
                                    ecma_get_value_value_field (value));
} /* ecma_get_number_from_value */

/**
 * Get pointer to ecma-string from ecma value
 *
 * @return the pointer
 */
ecma_string_t *__attr_pure___
ecma_get_string_from_value (ecma_value_t value) /**< ecma value */
{
  JERRY_ASSERT (ecma_get_value_type_field (value) == ECMA_TYPE_STRING);

  return ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                    ecma_get_value_value_field (value));
} /* ecma_get_string_from_value */

/**
 * Get pointer to ecma-object from ecma value
 *
 * @return the pointer
 */
ecma_object_t *__attr_pure___
ecma_get_object_from_value (ecma_value_t value) /**< ecma value */
{
  JERRY_ASSERT (ecma_get_value_type_field (value) == ECMA_TYPE_OBJECT);

  return ECMA_GET_NON_NULL_POINTER (ecma_object_t,
                                    ecma_get_value_value_field (value));
} /* ecma_get_object_from_value */

/**
 * Get the value from an error ecma value
 *
 * @return ecma value
 */
ecma_value_t __attr_pure___
ecma_get_value_from_error_value (ecma_value_t value) /**< ecma value */
{
  JERRY_ASSERT (ecma_is_value_error (value));

  value = (ecma_value_t) (value & ~(1u << ECMA_VALUE_ERROR_POS));

  JERRY_ASSERT (!ecma_is_value_error (value));

  return value;
} /* ecma_get_value_from_error_value */

/**
 * Copy ecma value.
 *
 * @return copy of the given value
 */
ecma_value_t
ecma_copy_value (ecma_value_t value)  /**< value description */
{
  switch (ecma_get_value_type_field (value))
  {
    case ECMA_TYPE_SIMPLE:
    {
      return value;
    }
    case ECMA_TYPE_NUMBER:
    {
      ecma_number_t *num_p = ecma_get_number_from_value (value);

      ecma_number_t *number_copy_p = ecma_alloc_number ();
      *number_copy_p = *num_p;

      return ecma_make_number_value (number_copy_p);
    }
    case ECMA_TYPE_STRING:
    {
      return ecma_make_string_value (ecma_copy_or_ref_ecma_string (ecma_get_string_from_value (value)));
    }
    case ECMA_TYPE_OBJECT:
    {
      ecma_ref_object (ecma_get_object_from_value (value));
      return value;
    }
  }

  JERRY_UNREACHABLE ();
} /* ecma_copy_value */

/**
 * Copy the ecma value if not an object
 *
 * @return copy of the given value
 */
ecma_value_t
ecma_copy_value_if_not_object (ecma_value_t value) /**< value description */
{
  if (ecma_get_value_type_field (value) != ECMA_TYPE_OBJECT)
  {
    return ecma_copy_value (value);
  }

  return value;
} /* ecma_copy_value_if_not_object */

/**
 * Free the ecma value
 */
void
ecma_free_value (ecma_value_t value) /**< value description */
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
      ecma_deref_object (ecma_get_object_from_value (value));
      break;
    }
  }
} /* ecma_free_value */

/**
 * Free the ecma value if not an object
 */
void
ecma_free_value_if_not_object (ecma_value_t value) /**< value description */
{
  if (ecma_get_value_type_field (value) != ECMA_TYPE_OBJECT)
  {
    ecma_free_value (value);
  }
} /* ecma_free_value_if_not_object */

/**
 * @}
 * @}
 */
