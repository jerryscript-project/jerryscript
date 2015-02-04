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

#include "ecma-alloc.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-value.h"
#include "globals.h"
#include "jrt-bit-fields.h"

JERRY_STATIC_ASSERT (sizeof (ecma_value_packed_t) * JERRY_BITSINBYTE == ECMA_VALUE_SIZE);

/**
 * Get pointer to ecma-number from ecma-value
 *
 * @return the pointer
 */
ecma_number_t* __attribute_pure__
ecma_get_number_from_value (const ecma_value_t& value) /**< ecma-value */
{
  return value.get_number ();
} /* ecma_get_number_from_value */

/**
 * Get pointer to ecma-string from ecma-value
 *
 * @return the pointer
 */
ecma_string_t* __attribute_pure__
ecma_get_string_from_value (const ecma_value_t& value) /**< ecma-value */
{
  return value.get_string ();
} /* ecma_get_string_from_value */

/**
 * Get pointer to ecma-object from ecma-value
 *
 * @return the pointer
 */
void
ecma_get_object_from_value (ecma_object_ptr_t &ret_val, /**< out: object pointer */
                            const ecma_value_t& value) /**< ecma-value */
{
  ret_val = value.get_object ();
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
void
ecma_copy_value (ecma_value_t &ret, /**< out: ecma-value */
                 const ecma_value_t& value, /**< ecma-value */
                 bool do_ref_if_object) /**< if the value is object value,
                                             increment reference counter of the object */
{
  switch (ecma_get_value_type_field (value))
  {
    case ECMA_TYPE_SIMPLE:
    {
      ret = value;

      break;
    }
    case ECMA_TYPE_NUMBER:
    {
      ecma_number_t *num_p = ecma_get_number_from_value (value);

      ecma_number_t *number_copy_p = ecma_alloc_number ();
      *number_copy_p = *num_p;

      ret = number_copy_p;

      break;
    }
    case ECMA_TYPE_STRING:
    {
      ecma_string_t *string_p = ecma_get_string_from_value (value);

      string_p = ecma_copy_or_ref_ecma_string (string_p);

      ret = string_p;

      break;
    }
    case ECMA_TYPE_OBJECT:
    {
      ecma_object_ptr_t obj_p;
      ecma_get_object_from_value (obj_p, value);

      if (do_ref_if_object)
      {
        ecma_ref_object (obj_p);
      }

      ret = value;

      break;
    }
  }
} /* ecma_copy_value */

/**
 * Free the ecma-value
 */
void
ecma_free_value (ecma_value_t& value, /**< value description */
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
        ecma_object_ptr_t obj_p;
        ecma_get_object_from_value (obj_p, value);
        ecma_deref_object (obj_p);
      }
      break;
    }
  }
} /* ecma_free_value */

/**
 * Throw completion value constructor.
 *
 * @return 'throw' completion value
 */
void
ecma_make_throw_obj_completion_value (ecma_completion_value_t &ret_value, /**< out: completion value */
                                      const ecma_object_ptr_t& exception_p) /**< an object */
{
  JERRY_ASSERT(exception_p.is_not_null ()
               && !ecma_is_lexical_environment (exception_p));

  ecma_value_t exception (exception_p);

  ecma_make_throw_completion_value (ret_value, exception);
} /* ecma_make_throw_obj_completion_value */

/**
 * Copy ecma-completion value.
 *
 * @return (source.type, ecma_copy_value (source.value), source.target).
 */
void
ecma_copy_completion_value (ecma_completion_value_t &ret_value, /**< out: completion value */
                            const ecma_completion_value_t& value) /**< completion value */
{
  const ecma_completion_type_t type = (ecma_completion_type_t) (value);
  const bool is_type_ok = (type == ECMA_COMPLETION_TYPE_NORMAL
#ifdef CONFIG_ECMA_EXCEPTION_SUPPORT
                           || type == ECMA_COMPLETION_TYPE_THROW
#endif /* CONFIG_ECMA_EXCEPTION_SUPPORT */
                           || type == ECMA_COMPLETION_TYPE_RETURN
                           || type == ECMA_COMPLETION_TYPE_EXIT);

  JERRY_ASSERT (is_type_ok);

  ecma_value_t v ((ecma_value_packed_t) value);

  ecma_value_t value_copy;
  ecma_copy_value (value_copy, v, true);

  ecma_make_completion_value (ret_value, type, value_copy);
} /* ecma_copy_completion_value */

/**
 * Free the completion value.
 */
void
ecma_free_completion_value (ecma_completion_value_t& completion_value) /**< completion value */
{
  switch ((ecma_completion_type_t) (completion_value))
  {
    case ECMA_COMPLETION_TYPE_NORMAL:
#ifdef CONFIG_ECMA_EXCEPTION_SUPPORT
    case ECMA_COMPLETION_TYPE_THROW:
#endif /* CONFIG_ECMA_EXCEPTION_SUPPORT */
    case ECMA_COMPLETION_TYPE_RETURN:
    {
      ecma_value_t v ((ecma_value_packed_t) completion_value);

      ecma_free_value (v, true);
      break;
    }
    case ECMA_COMPLETION_TYPE_EXIT:
    {
      const ecma_value_t& v = completion_value;

      JERRY_ASSERT (v.is_simple ());

      break;
    }
    case ECMA_COMPLETION_TYPE_META:
    {
      JERRY_UNREACHABLE ();
    }
  }

  ecma_make_empty_completion_value (completion_value);
} /* ecma_free_completion_value */

/**
 * Debug assertion that specified value's type is one of ECMA-defined
 * script-visible types, i.e.: undefined, null, boolean, number, string, object.
 */
void
ecma_check_value_type_is_spec_defined (const ecma_value_t& value) /**< ecma-value */
{
  JERRY_ASSERT (ecma_is_value_undefined (value)
                || ecma_is_value_null (value)
                || ecma_is_value_boolean (value)
                || ecma_is_value_number (value)
                || ecma_is_value_string (value)
                || ecma_is_value_object (value));
} /* ecma_check_value_type_is_spec_defined */
