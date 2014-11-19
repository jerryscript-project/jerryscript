/* Copyright 2014 Samsung Electronics Co., Ltd.
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
#include "globals.h"

/**
 * Check if the value is empty.
 *
 * @return true - if the value contains implementation-defined empty simple value,
 *         false - otherwise.
 */
bool
ecma_is_value_empty (ecma_value_t value) /**< ecma-value */
{
  return (value.value_type == ECMA_TYPE_SIMPLE && value.value == ECMA_SIMPLE_VALUE_EMPTY);
} /* ecma_is_value_empty */

/**
 * Check if the value is undefined.
 *
 * @return true - if the value contains ecma-undefined simple value,
 *         false - otherwise.
 */
bool
ecma_is_value_undefined (ecma_value_t value) /**< ecma-value */
{
  return (value.value_type == ECMA_TYPE_SIMPLE && value.value == ECMA_SIMPLE_VALUE_UNDEFINED);
} /* ecma_is_value_undefined */

/**
 * Check if the value is null.
 *
 * @return true - if the value contains ecma-null simple value,
 *         false - otherwise.
 */
bool
ecma_is_value_null (ecma_value_t value) /**< ecma-value */
{
  return (value.value_type == ECMA_TYPE_SIMPLE && value.value == ECMA_SIMPLE_VALUE_NULL);
} /* ecma_is_value_null */

/**
 * Check if the value is boolean.
 *
 * @return true - if the value contains ecma-true or ecma-false simple values,
 *         false - otherwise.
 */
bool
ecma_is_value_boolean (ecma_value_t value) /**< ecma-value */
{
  return ((value.value_type == ECMA_TYPE_SIMPLE && value.value == ECMA_SIMPLE_VALUE_FALSE)
          || (value.value_type == ECMA_TYPE_SIMPLE && value.value == ECMA_SIMPLE_VALUE_TRUE));
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
bool
ecma_is_value_true (ecma_value_t value) /**< ecma-value */
{
  JERRY_ASSERT(ecma_is_value_boolean (value));

  return (value.value_type == ECMA_TYPE_SIMPLE && value.value == ECMA_SIMPLE_VALUE_TRUE);
} /* ecma_is_value_true */

/**
 * Check if the value is ecma-number.
 *
 * @return true - if the value contains ecma-number value,
 *         false - otherwise.
 */
bool
ecma_is_value_number (ecma_value_t value) /**< ecma-value */
{
  return (value.value_type == ECMA_TYPE_NUMBER);
} /* ecma_is_value_number */

/**
 * Check if the value is ecma-string.
 *
 * @return true - if the value contains ecma-string value,
 *         false - otherwise.
 */
bool
ecma_is_value_string (ecma_value_t value) /**< ecma-value */
{
  return (value.value_type == ECMA_TYPE_STRING);
} /* ecma_is_value_string */

/**
 * Check if the value is object.
 *
 * @return true - if the value contains object value,
 *         false - otherwise.
 */
bool
ecma_is_value_object (ecma_value_t value) /**< ecma-value */
{
  return (value.value_type == ECMA_TYPE_OBJECT);
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
ecma_value_t
ecma_make_simple_value (ecma_simple_value_t value) /**< simple value */
{
  ecma_value_t ret_value = (ecma_value_t)
  {
    .value_type = ECMA_TYPE_SIMPLE,
    .value = value
  };

  return ret_value;
} /* ecma_make_simple_value */

/**
 * Number value constructor
 */
ecma_value_t
ecma_make_number_value (ecma_number_t* num_p) /**< number to reference in value */
{
  JERRY_ASSERT(num_p != NULL);

  ecma_value_t number_value;

  number_value.value_type = ECMA_TYPE_NUMBER;
  ECMA_SET_POINTER(number_value.value, num_p);

  return number_value;
} /* ecma_make_number_value */

/**
 * String value constructor
 */
ecma_value_t
ecma_make_string_value (ecma_string_t* ecma_string_p) /**< string to reference in value */
{
  JERRY_ASSERT(ecma_string_p != NULL);

  ecma_value_t string_value;

  string_value.value_type = ECMA_TYPE_STRING;
  ECMA_SET_POINTER(string_value.value, ecma_string_p);

  return string_value;
} /* ecma_make_string_value */

/**
 * object value constructor
 */
ecma_value_t
ecma_make_object_value (ecma_object_t* object_p) /**< object to reference in value */
{
  JERRY_ASSERT(object_p != NULL);

  ecma_value_t object_value;

  object_value.value_type = ECMA_TYPE_OBJECT;
  ECMA_SET_POINTER(object_value.value, object_p);

  return object_value;
} /* ecma_make_object_value */

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
ecma_copy_value (const ecma_value_t value, /**< ecma-value */
                 bool do_ref_if_object) /**< if the value is object value,
                                             increment reference counter of the object */
{
  ecma_value_t value_copy;

  switch ((ecma_type_t)value.value_type)
  {
    case ECMA_TYPE_SIMPLE:
    {
      value_copy = value;

      break;
    }
    case ECMA_TYPE_NUMBER:
    {
      ecma_number_t *num_p = ECMA_GET_NON_NULL_POINTER(value.value);
      JERRY_ASSERT(num_p != NULL);

      ecma_number_t *number_copy_p = ecma_alloc_number ();
      *number_copy_p = *num_p;

      value_copy = (ecma_value_t)
      {
        .value_type = ECMA_TYPE_NUMBER
      };
      ECMA_SET_NON_NULL_POINTER(value_copy.value, number_copy_p);

      break;
    }
    case ECMA_TYPE_STRING:
    {
      ecma_string_t *string_p = ECMA_GET_NON_NULL_POINTER(value.value);
      JERRY_ASSERT(string_p != NULL);

      string_p = ecma_copy_or_ref_ecma_string (string_p);

      value_copy = (ecma_value_t)
      {
        .value_type = ECMA_TYPE_STRING
      };
      ECMA_SET_NON_NULL_POINTER(value_copy.value, string_p);

      break;
    }
    case ECMA_TYPE_OBJECT:
    {
      ecma_object_t *obj_p = ECMA_GET_NON_NULL_POINTER(value.value);
      JERRY_ASSERT(obj_p != NULL);

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
  switch ((ecma_type_t) value.value_type)
  {
    case ECMA_TYPE_SIMPLE:
    {
      /* doesn't hold additional memory */
      break;
    }

    case ECMA_TYPE_NUMBER:
    {
      ecma_number_t *number_p = ECMA_GET_NON_NULL_POINTER(value.value);
      ecma_dealloc_number (number_p);
      break;
    }

    case ECMA_TYPE_STRING:
    {
      ecma_string_t *string_p = ECMA_GET_NON_NULL_POINTER(value.value);
      ecma_deref_ecma_string (string_p);
      break;
    }

    case ECMA_TYPE_OBJECT:
    {
      if (do_deref_if_object)
      {
        ecma_deref_object (ECMA_GET_NON_NULL_POINTER(value.value));
      }
      break;
    }
  }
} /* ecma_free_value */

/**
 * Normal, throw, return, exit and meta completion values constructor
 *
 * @return completion value
 */
ecma_completion_value_t
ecma_make_completion_value (ecma_completion_type_t type, /**< type */
                            ecma_value_t value) /**< value */
{
  const bool is_type_ok = (type == ECMA_COMPLETION_TYPE_NORMAL
#ifdef CONFIG_ECMA_EXCEPTION_SUPPORT
                     || type == ECMA_COMPLETION_TYPE_THROW
#endif /* CONFIG_ECMA_EXCEPTION_SUPPORT */
                     || type == ECMA_COMPLETION_TYPE_RETURN
                     || type == ECMA_COMPLETION_TYPE_EXIT
                     || (type == ECMA_COMPLETION_TYPE_META
                         && ecma_is_value_empty (value)));

  JERRY_ASSERT (is_type_ok);

  ecma_completion_value_t ret_value = (ecma_completion_value_t)
  {
    .type = type,
    .u.value = value,
  };

  return ret_value;
} /* ecma_make_completion_value */

/**
 * Break and continue completion values constructor
 *
 * @return completion value
 */
ecma_completion_value_t
ecma_make_label_completion_value (ecma_completion_type_t type, /**< type */
                                  uint8_t depth_level, /**< depth level (in try constructions,
                                                            with blocks, etc.) */
                                  uint16_t offset) /**< offset to label from end of last block */
{
  JERRY_ASSERT (type == ECMA_COMPLETION_TYPE_BREAK
                || type == ECMA_COMPLETION_TYPE_CONTINUE);

  ecma_completion_value_t ret_value;

  ecma_label_descriptor_t *label_desc_p = ecma_alloc_label_descriptor ();
  *label_desc_p = (ecma_label_descriptor_t)
  {
    .offset = offset,
    .depth = depth_level
  };

  ret_value.type = type;
  ECMA_SET_POINTER (ret_value.u.label_desc_cp, label_desc_p);

  return ret_value;
} /* ecma_make_label_completion_value */

/**
 * Simple normal completion value constructor
 *
 * @return completion value
 */
ecma_completion_value_t
ecma_make_simple_completion_value (ecma_simple_value_t simple_value) /**< simple ecma-value */
{
  JERRY_ASSERT(simple_value == ECMA_SIMPLE_VALUE_UNDEFINED
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
ecma_completion_value_t
ecma_make_normal_completion_value (ecma_value_t value) /**< value */
{
  return ecma_make_completion_value (ECMA_COMPLETION_TYPE_NORMAL, value);
} /* ecma_make_normal_completion_value */

/**
 * Throw completion value constructor
 *
 * @return completion value
 */
ecma_completion_value_t
ecma_make_throw_completion_value (ecma_value_t value) /**< value */
{
#ifdef CONFIG_ECMA_EXCEPTION_SUPPORT
  return ecma_make_completion_value (ECMA_COMPLETION_TYPE_THROW, value);
#else /* CONFIG_ECMA_EXCEPTION_SUPPORT */
  (void) value;

  jerry_exit (ERR_UNHANDLED_EXCEPTION);
#endif /* !CONFIG_ECMA_EXCEPTION_SUPPORT */
} /* ecma_make_throw_completion_value */

/**
 * Throw completion value constructor.
 *
 * @return 'throw' completion value
 */
ecma_completion_value_t
ecma_make_throw_obj_completion_value (ecma_object_t *exception_p) /**< an object */
{
  JERRY_ASSERT(exception_p != NULL
               && !ecma_is_lexical_environment (exception_p));

  ecma_value_t exception = ecma_make_object_value (exception_p);

  return ecma_make_throw_completion_value (exception);
} /* ecma_make_throw_obj_completion_value */

/**
 * Empty completion value constructor.
 *
 * @return (normal, empty, reserved) completion value.
 */
ecma_completion_value_t
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
ecma_completion_value_t
ecma_make_return_completion_value (ecma_value_t value) /**< value */
{
  return ecma_make_completion_value (ECMA_COMPLETION_TYPE_RETURN, value);
} /* ecma_make_return_completion_value */

/**
 * Exit completion value constructor
 *
 * @return completion value
 */
ecma_completion_value_t
ecma_make_exit_completion_value (bool is_successful) /**< does completion value indicate
                                                          successfulness completion
                                                          of script execution (true) or not (false) */
{
  return ecma_make_completion_value (ECMA_COMPLETION_TYPE_EXIT,
                                     ecma_make_simple_value (is_successful ? ECMA_SIMPLE_VALUE_TRUE
                                                                           : ECMA_SIMPLE_VALUE_FALSE));
} /* ecma_make_exit_completion_value */

/**
 * Meta completion value constructor
 *
 * @return completion value
 */
ecma_completion_value_t
ecma_make_meta_completion_value (void)
{
  return ecma_make_completion_value (ECMA_COMPLETION_TYPE_META,
                                     ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY));
} /* ecma_make_meta_completion_value */

/**
 * Copy ecma-completion value.
 *
 * @return (source.type, ecma_copy_value (source.value), source.target).
 */
ecma_completion_value_t
ecma_copy_completion_value (ecma_completion_value_t value) /**< completion value */
{
  const bool is_type_ok = (value.type == ECMA_COMPLETION_TYPE_NORMAL
#ifdef CONFIG_ECMA_EXCEPTION_SUPPORT
                           || value.type == ECMA_COMPLETION_TYPE_THROW
#endif /* CONFIG_ECMA_EXCEPTION_SUPPORT */
                           || value.type == ECMA_COMPLETION_TYPE_RETURN
                           || value.type == ECMA_COMPLETION_TYPE_EXIT);

  JERRY_ASSERT (is_type_ok);

  return ecma_make_completion_value (value.type,
                                     ecma_copy_value (value.u.value, true));
} /* ecma_copy_completion_value */

/**
 * Free the completion value.
 */
void
ecma_free_completion_value (ecma_completion_value_t completion_value) /**< completion value */
{
  switch ((ecma_completion_type_t)completion_value.type)
  {
    case ECMA_COMPLETION_TYPE_NORMAL:
#ifdef CONFIG_ECMA_EXCEPTION_SUPPORT
    case ECMA_COMPLETION_TYPE_THROW:
#endif /* CONFIG_ECMA_EXCEPTION_SUPPORT */
    case ECMA_COMPLETION_TYPE_RETURN:
    {
      ecma_free_value (completion_value.u.value, true);
      break;
    }
    case ECMA_COMPLETION_TYPE_EXIT:
    {
      JERRY_ASSERT(completion_value.u.value.value_type == ECMA_TYPE_SIMPLE);
      break;
    }
    case ECMA_COMPLETION_TYPE_CONTINUE:
    case ECMA_COMPLETION_TYPE_BREAK:
    {
      ecma_dealloc_label_descriptor (ECMA_GET_NON_NULL_POINTER (completion_value.u.label_desc_cp));
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
bool
ecma_is_completion_value_normal (ecma_completion_value_t value) /**< completion value */
{
  return (value.type == ECMA_COMPLETION_TYPE_NORMAL);
} /* ecma_is_completion_value_normal */

/**
 * Check if the completion value is throw value.
 *
 * @return true - if the completion type is throw,
 *         false - otherwise.
 */
bool
ecma_is_completion_value_throw (ecma_completion_value_t value) /**< completion value */
{
#ifdef CONFIG_ECMA_EXCEPTION_SUPPORT
  return (value.type == ECMA_COMPLETION_TYPE_THROW);
#else /* CONFIG_ECMA_EXCEPTION_SUPPORT */
  (void) value;

  return false;
#endif /* !CONFIG_ECMA_EXCEPTION_SUPPORT */
} /* ecma_is_completion_value_throw */

/**
 * Check if the completion value is return value.
 *
 * @return true - if the completion type is return,
 *         false - otherwise.
 */
bool
ecma_is_completion_value_return (ecma_completion_value_t value) /**< completion value */
{
  return (value.type == ECMA_COMPLETION_TYPE_RETURN);
} /* ecma_is_completion_value_return */

/**
 * Check if the completion value is exit value.
 *
 * @return true - if the completion type is exit,
 *         false - otherwise.
 */
bool
ecma_is_completion_value_exit (ecma_completion_value_t value) /**< completion value */
{
  if (value.type == ECMA_COMPLETION_TYPE_EXIT)
  {
    JERRY_ASSERT (ecma_is_value_boolean (value.u.value));

    return true;
  }
  else
  {
    return false;
  }
} /* ecma_is_completion_value_exit */

/**
 * Check if the completion value is meta value.
 *
 * @return true - if the completion type is meta,
 *         false - otherwise.
 */
bool
ecma_is_completion_value_meta (ecma_completion_value_t value) /**< completion value */
{
  if (value.type == ECMA_COMPLETION_TYPE_META)
  {
    JERRY_ASSERT (ecma_is_value_empty (value.u.value));

    return true;
  }
  else
  {
    return false;
  }
} /* ecma_is_completion_value_meta */

/**
 * Check if the completion value is specified normal simple value.
 *
 * @return true - if the completion type is normal and
 *                value contains specified simple ecma-value,
 *         false - otherwise.
 */
bool
ecma_is_completion_value_normal_simple_value (ecma_completion_value_t value, /**< completion value */
                                              ecma_simple_value_t simple_value) /**< simple value to check
                                                                                     for equality with */
{
  return (ecma_is_completion_value_normal (value)
          && value.u.value.value_type == ECMA_TYPE_SIMPLE
          && value.u.value.value == simple_value);
} /* ecma_is_completion_value_normal_simple_value */

/**
 * Check if the completion value is normal true.
 *
 * @return true - if the completion type is normal and
 *                value contains ecma-true simple value,
 *         false - otherwise.
 */
bool
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
bool
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
bool
ecma_is_completion_value_empty (ecma_completion_value_t value) /**< completion value */
{
  return (ecma_is_completion_value_normal (value)
          && ecma_is_value_empty (value.u.value));
} /* ecma_is_completion_value_empty */

/**
 * @}
 * @}
 */
