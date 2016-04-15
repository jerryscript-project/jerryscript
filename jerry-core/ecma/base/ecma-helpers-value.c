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

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

/**
 * Masking the type and flags
 */
#define ECMA_VALUE_FULL_MASK (ECMA_VALUE_TYPE_MASK | ECMA_VALUE_ERROR_FLAG)

JERRY_STATIC_ASSERT (ECMA_TYPE___MAX <= ECMA_VALUE_TYPE_MASK,
                     ecma_types_must_be_less_than_mask);

JERRY_STATIC_ASSERT ((ECMA_VALUE_FULL_MASK + 1) == (1 << ECMA_VALUE_SHIFT),
                     ecma_value_part_must_start_after_flags);

JERRY_STATIC_ASSERT (ECMA_VALUE_SHIFT <= MEM_ALIGNMENT_LOG,
                     ecma_value_shift_must_be_less_than_or_equal_than_mem_alignment_log);

JERRY_STATIC_ASSERT ((sizeof (ecma_value_t) * JERRY_BITSINBYTE)
                     >= (sizeof (mem_cpointer_t) * JERRY_BITSINBYTE + ECMA_VALUE_SHIFT),
                     ecma_value_must_be_large_enough_to_store_compressed_pointers);

#ifdef ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY

JERRY_STATIC_ASSERT (sizeof (uintptr_t) <= sizeof (ecma_value_t),
                     uintptr_t_must_fit_in_ecma_value_t);

#else /* ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */

JERRY_STATIC_ASSERT (sizeof (uintptr_t) > sizeof (ecma_value_t),
                     uintptr_t_must_not_fit_in_ecma_value_t);

#endif /* ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */

/**
 * Get type field of ecma value
 *
 * @return type field
 */
static inline ecma_type_t __attr_pure___ __attr_always_inline___
ecma_get_value_type_field (ecma_value_t value) /**< ecma value */
{
  return value & ECMA_VALUE_TYPE_MASK;
} /* ecma_get_value_type_field */

/**
 * Convert a pointer into an ecma value.
 *
 * @return ecma value
 */
static inline ecma_value_t __attr_pure___ __attr_always_inline___
ecma_pointer_to_ecma_value (const void *ptr) /**< pointer */
{
#ifdef ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY

  uintptr_t uint_ptr = (uintptr_t) ptr;
  JERRY_ASSERT ((uint_ptr & ECMA_VALUE_FULL_MASK) == 0);
  return (ecma_value_t) uint_ptr;

#else /* ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */

  mem_cpointer_t ptr_cp;
  ECMA_SET_NON_NULL_POINTER (ptr_cp, ptr);
  return ((ecma_value_t) ptr_cp) << ECMA_VALUE_SHIFT;

#endif /* ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */
} /* ecma_pointer_to_ecma_value */

/**
 * Get a pointer from an ecma value
 *
 * @return pointer
 */
static inline void * __attr_pure___ __attr_always_inline___
ecma_get_pointer_from_ecma_value (ecma_value_t value) /**< value */
{
#ifdef ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY
  return (void *) (uintptr_t) ((value) & ~ECMA_VALUE_FULL_MASK);
#else /* ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */
  return ECMA_GET_NON_NULL_POINTER (ecma_number_t,
                                    value >> ECMA_VALUE_SHIFT);
#endif /* ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */
} /* ecma_get_pointer_from_ecma_value */

/**
 * Check whether the value is a given simple value.
 *
 * @return true - if the value is equal to the given simple value,
 *         false - otherwise.
 */
static inline bool __attr_pure___ __attr_always_inline___
ecma_is_value_equal_to_simple_value (ecma_value_t value, /**< ecma value */
                                     ecma_simple_value_t simple_value) /**< simple value */
{
  return (value | ECMA_VALUE_ERROR_FLAG) == (ecma_make_simple_value (simple_value) | ECMA_VALUE_ERROR_FLAG);
} /* ecma_is_value_equal_to_simple_value */

/**
 * Check if the value is empty.
 *
 * @return true - if the value contains implementation-defined empty simple value,
 *         false - otherwise.
 */
inline bool __attr_pure___ __attr_always_inline___
ecma_is_value_empty (ecma_value_t value) /**< ecma value */
{
  return ecma_is_value_equal_to_simple_value (value, ECMA_SIMPLE_VALUE_EMPTY);
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
  return ecma_is_value_equal_to_simple_value (value, ECMA_SIMPLE_VALUE_UNDEFINED);
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
  return ecma_is_value_equal_to_simple_value (value, ECMA_SIMPLE_VALUE_NULL);
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
  return ecma_is_value_true (value) || ecma_is_value_false (value);
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
  return ecma_is_value_equal_to_simple_value (value, ECMA_SIMPLE_VALUE_TRUE);
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
  return ecma_is_value_equal_to_simple_value (value, ECMA_SIMPLE_VALUE_FALSE);
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
  return ecma_is_value_equal_to_simple_value (value, ECMA_SIMPLE_VALUE_ARRAY_HOLE);
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
  return (value & ECMA_VALUE_ERROR_FLAG) != 0;
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
ecma_make_simple_value (const ecma_simple_value_t simple_value) /**< simple value */
{
  return (((ecma_value_t) (simple_value)) << ECMA_VALUE_SHIFT) | ECMA_TYPE_SIMPLE;
} /* ecma_make_simple_value */

/**
 * Number value constructor
 */
ecma_value_t __attr_const___
ecma_make_number_value (const ecma_number_t *ecma_num_p) /**< number to reference in value */
{
  JERRY_ASSERT (ecma_num_p != NULL);

  return ecma_pointer_to_ecma_value (ecma_num_p) | ECMA_TYPE_NUMBER;
} /* ecma_make_number_value */

/**
 * String value constructor
 */
ecma_value_t __attr_const___
ecma_make_string_value (const ecma_string_t *ecma_string_p) /**< string to reference in value */
{
  JERRY_ASSERT (ecma_string_p != NULL);

  return ecma_pointer_to_ecma_value (ecma_string_p) | ECMA_TYPE_STRING;
} /* ecma_make_string_value */

/**
 * Object value constructor
 */
ecma_value_t __attr_const___
ecma_make_object_value (const ecma_object_t *object_p) /**< object to reference in value */
{
  JERRY_ASSERT (object_p != NULL);

  return ecma_pointer_to_ecma_value (object_p) | ECMA_TYPE_OBJECT;
} /* ecma_make_object_value */

/**
 * Error value constructor
 */
ecma_value_t __attr_const___
ecma_make_error_value (ecma_value_t value) /**< original ecma value */
{
  /* Error values cannot be converted. */
  JERRY_ASSERT (!ecma_is_value_error (value));

  return value | ECMA_VALUE_ERROR_FLAG;
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

  return (ecma_number_t *) ecma_get_pointer_from_ecma_value (value);
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

  return (ecma_string_t *) ecma_get_pointer_from_ecma_value (value);
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

  return (ecma_object_t *) ecma_get_pointer_from_ecma_value (value);
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

  value = (ecma_value_t) (value & ~ECMA_VALUE_ERROR_FLAG);

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
