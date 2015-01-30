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
  JERRY_ASSERT (ecma_get_value_type_field (value) == ECMA_TYPE_NUMBER);

  return ECMA_GET_NON_NULL_POINTER (ecma_number_t,
                                    ecma_get_value_value_field (value));
} /* ecma_get_number_from_value */

/**
 * Get pointer to ecma-string from ecma-value
 *
 * @return the pointer
 */
ecma_string_t* __attribute_pure__
ecma_get_string_from_value (const ecma_value_t& value) /**< ecma-value */
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
ecma_object_t* __attribute_pure__
ecma_get_object_from_value (const ecma_value_t& value) /**< ecma-value */
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
      ecma_object_t *obj_p = ecma_get_object_from_value (value);

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
        ecma_deref_object (ecma_get_object_from_value (value));
      }
      break;
    }
  }
} /* ecma_free_value */

/**
 * Get pointer to label descriptor from completion value
 *
 * @return pointer to label descriptor
 */
static ecma_label_descriptor_t* __attribute_const__
ecma_get_completion_value_label_descriptor (ecma_completion_value_t completion_value) /**< completion value */
{
  return ECMA_GET_NON_NULL_POINTER (ecma_label_descriptor_t,
                                    (uintptr_t) jrt_extract_bit_field (completion_value,
                                                                       ECMA_COMPLETION_VALUE_LABEL_DESC_CP_POS,
                                                                       ECMA_COMPLETION_VALUE_LABEL_DESC_CP_WIDTH));
} /* ecma_get_completion_value_label_descriptor */

/**
 * Set label descriptor of completion value
 *
 * @return completion value with updated field
 */
static ecma_completion_value_t __attribute_const__
ecma_set_completion_value_label_descriptor (ecma_completion_value_t completion_value, /**< completion value
                                                                                       * to set field in */
                                            ecma_label_descriptor_t* label_desc_p) /**< pointer to the
                                                                                    *   label descriptor */
{
  uintptr_t label_desc_cp;
  ECMA_SET_NON_NULL_POINTER (label_desc_cp, label_desc_p);

  return (ecma_completion_value_t) jrt_set_bit_field_value (completion_value,
                                                            label_desc_cp,
                                                            ECMA_COMPLETION_VALUE_LABEL_DESC_CP_POS,
                                                            ECMA_COMPLETION_VALUE_LABEL_DESC_CP_WIDTH);
} /* ecma_set_completion_value_label_descriptor */

/**
 * Break and continue completion values constructor
 *
 * @return completion value
 */
ecma_completion_value_t __attribute_const__
ecma_make_label_completion_value (ecma_completion_type_t type, /**< type */
                                  uint8_t depth_level, /**< depth level (in try constructions,
                                                            with blocks, etc.) */
                                  uint16_t offset) /**< offset to label from end of last block */
{
  JERRY_ASSERT (type == ECMA_COMPLETION_TYPE_BREAK
                || type == ECMA_COMPLETION_TYPE_CONTINUE);

  ecma_label_descriptor_t *label_desc_p = ecma_alloc_label_descriptor ();
  label_desc_p->offset = offset;
  label_desc_p->depth = depth_level;

  ecma_completion_value_t completion_value = 0;

  completion_value = ecma_set_completion_value_type_field (completion_value,
                                                           type);
  completion_value = ecma_set_completion_value_label_descriptor (completion_value,
                                                                 label_desc_p);

  return completion_value;
} /* ecma_make_label_completion_value */

/**
 * Throw completion value constructor.
 *
 * @return 'throw' completion value
 */
ecma_completion_value_t __attribute_const__
ecma_make_throw_obj_completion_value (ecma_object_t *exception_p) /**< an object */
{
  JERRY_ASSERT(exception_p != NULL
               && !ecma_is_lexical_environment (exception_p));

  ecma_value_t exception (exception_p);

  return ecma_make_throw_completion_value (exception);
} /* ecma_make_throw_obj_completion_value */

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
#ifdef CONFIG_ECMA_EXCEPTION_SUPPORT
                           || type == ECMA_COMPLETION_TYPE_THROW
#endif /* CONFIG_ECMA_EXCEPTION_SUPPORT */
                           || type == ECMA_COMPLETION_TYPE_RETURN
                           || type == ECMA_COMPLETION_TYPE_EXIT);

  JERRY_ASSERT (is_type_ok);

  ecma_value_t v;
  ecma_get_completion_value_value_field (v, value);

  ecma_value_t value_copy;
  ecma_copy_value (value_copy, v, true);

  return ecma_make_completion_value (type, value_copy);
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
#ifdef CONFIG_ECMA_EXCEPTION_SUPPORT
    case ECMA_COMPLETION_TYPE_THROW:
#endif /* CONFIG_ECMA_EXCEPTION_SUPPORT */
    case ECMA_COMPLETION_TYPE_RETURN:
    {
      ecma_value_t v;
      ecma_get_completion_value_value_field (v, completion_value);

      ecma_free_value (v, true);
      break;
    }
    case ECMA_COMPLETION_TYPE_EXIT:
    {
      ecma_value_t v;

      ecma_get_completion_value_value_field (v, completion_value);
      JERRY_ASSERT(ecma_get_value_type_field (v) == ECMA_TYPE_SIMPLE);

      break;
    }
    case ECMA_COMPLETION_TYPE_CONTINUE:
    case ECMA_COMPLETION_TYPE_BREAK:
    {
      ecma_dealloc_label_descriptor (ecma_get_completion_value_label_descriptor (completion_value));
      break;
    }
    case ECMA_COMPLETION_TYPE_META:
    {
      JERRY_UNREACHABLE ();
    }
  }
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
