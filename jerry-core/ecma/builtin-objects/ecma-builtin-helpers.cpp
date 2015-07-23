/* Copyright 2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged.
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

#include "ecma-array-object.h"
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-function-object.h"
#include "ecma-exceptions.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"
#include "lit-magic-strings.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltinhelpers ECMA builtin helper operations
 * @{
 */

/**
 * Common implementation of the Object.prototype.toString routine
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.2
 *
 * Used by:
 *         - The Object.prototype.toString routine.
 *         - The Array.prototype.toString routine as fallback.
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */

ecma_completion_value_t
ecma_builtin_helper_object_to_string (const ecma_value_t this_arg) /**< this argument */
{
  lit_magic_string_id_t type_string;

  if (ecma_is_value_undefined (this_arg))
  {
    type_string = LIT_MAGIC_STRING_UNDEFINED_UL;
  }
  else if (ecma_is_value_null (this_arg))
  {
    type_string = LIT_MAGIC_STRING_NULL_UL;
  }
  else
  {
    ecma_completion_value_t obj_this = ecma_op_to_object (this_arg);

    if (!ecma_is_completion_value_normal (obj_this))
    {
      return obj_this;
    }

    JERRY_ASSERT (ecma_is_value_object (ecma_get_completion_value_value (obj_this)));

    ecma_object_t *obj_p = ecma_get_object_from_completion_value (obj_this);

    type_string = ecma_object_get_class_name (obj_p);

    ecma_free_completion_value (obj_this);
  }

  ecma_string_t *ret_string_p;

  /* Building string "[object #type#]" where type is 'Undefined',
     'Null' or one of possible object's classes.
     The string with null character is maximum 19 characters long. */
  const ssize_t buffer_size = 19;
  MEM_DEFINE_LOCAL_ARRAY (str_buffer, buffer_size, lit_utf8_byte_t);

  lit_utf8_byte_t *buffer_ptr = str_buffer;
  ssize_t buffer_size_left = buffer_size;

  const lit_magic_string_id_t magic_string_ids[] =
  {
    LIT_MAGIC_STRING_LEFT_SQUARE_CHAR,
    LIT_MAGIC_STRING_OBJECT,
    LIT_MAGIC_STRING_SPACE_CHAR,
    type_string,
    LIT_MAGIC_STRING_RIGHT_SQUARE_CHAR
  };

  for (uint32_t i = 0; i < sizeof (magic_string_ids) / sizeof (lit_magic_string_id_t); ++i)
  {
    buffer_ptr = lit_copy_magic_string_to_buffer (magic_string_ids[i], buffer_ptr, buffer_size_left);
    buffer_size_left = buffer_size - (buffer_ptr - str_buffer);
  }

  JERRY_ASSERT (buffer_size_left >= 0);

  ret_string_p = ecma_new_ecma_string_from_utf8 (str_buffer, (lit_utf8_size_t) (buffer_size - buffer_size_left));

  MEM_FINALIZE_LOCAL_ARRAY (str_buffer);

  return ecma_make_normal_completion_value (ecma_make_string_value (ret_string_p));
} /* ecma_builtin_helper_object_to_string */

/**
 * The Array.prototype's 'toLocaleString' single element operation routine
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.3 steps 6-8 and 10.b-d
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_builtin_helper_get_to_locale_string_at_index (ecma_object_t *obj_p, /** < this object */
                                                   uint32_t index) /** < array index */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  ecma_string_t *index_string_p = ecma_new_ecma_string_from_uint32 (index);

  ECMA_TRY_CATCH (index_value,
                  ecma_op_object_get (obj_p, index_string_p),
                  ret_value);

  if (ecma_is_value_undefined (index_value) || ecma_is_value_null (index_value))
  {
    ecma_string_t *return_string_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
    ret_value = ecma_make_normal_completion_value (ecma_make_string_value (return_string_p));
  }
  else
  {
    ECMA_TRY_CATCH (index_obj_value,
                    ecma_op_to_object (index_value),
                    ret_value);

    ecma_object_t *index_obj_p = ecma_get_object_from_value (index_obj_value);
    ecma_string_t *locale_string_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_TO_LOCALE_STRING_UL);

    ECMA_TRY_CATCH (to_locale_value,
                    ecma_op_object_get (index_obj_p, locale_string_magic_string_p),
                    ret_value);

    if (ecma_op_is_callable (to_locale_value))
    {
      ecma_object_t *locale_func_obj_p = ecma_get_object_from_value (to_locale_value);
      ret_value = ecma_op_function_call (locale_func_obj_p,
                                         ecma_make_object_value (index_obj_p),
                                         NULL,
                                         0);
    }
    else
    {
      ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }

    ECMA_FINALIZE (to_locale_value);

    ecma_deref_ecma_string (locale_string_magic_string_p);

    ECMA_FINALIZE (index_obj_value);
  }

  ECMA_FINALIZE (index_value);

  ecma_deref_ecma_string (index_string_p);

  return ret_value;
} /* ecma_builtin_helper_get_to_locale_string_at_index */


/**
 * The Object.keys and Object.getOwnPropertyName routine's common part.
 *
 * See also:
 *          ECMA-262 v5, 15.2.3.4 steps 2-5
 *          ECMA-262 v5, 15.2.3.14 steps 3-6
 *
 * @return completion value - Array of property names.
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_builtin_helper_object_get_properties (ecma_object_t *obj_p, /** < object */
                                           bool only_enumerable_properties) /** < list enumerable properties? */
{
  JERRY_ASSERT (obj_p != NULL);

  ecma_completion_value_t new_array = ecma_op_create_array_object (NULL, 0, false);
  JERRY_ASSERT (ecma_is_completion_value_normal (new_array));
  ecma_object_t *new_array_p = ecma_get_object_from_completion_value (new_array);

  uint32_t index = 0;

  for (ecma_property_t *property_p = ecma_get_property_list (obj_p);
       property_p != NULL;
       property_p = ECMA_GET_POINTER (ecma_property_t, property_p->next_property_p))
  {
    ecma_string_t *property_name_p;

    if (property_p->type == ECMA_PROPERTY_NAMEDDATA)
    {
      property_name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                   property_p->u.named_data_property.name_p);
    }
    else if (property_p->type == ECMA_PROPERTY_NAMEDACCESSOR)
    {
      property_name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                   property_p->u.named_accessor_property.name_p);
    }
    else
    {
      continue;
    }

    if (only_enumerable_properties && !ecma_is_property_enumerable (property_p))
    {
      continue;
    }

    JERRY_ASSERT (property_name_p != NULL);

    ecma_string_t *index_string_p = ecma_new_ecma_string_from_uint32 (index);

    ecma_property_descriptor_t item_prop_desc = ecma_make_empty_property_descriptor ();
    {
      item_prop_desc.is_value_defined = true;
      item_prop_desc.value = ecma_make_string_value (property_name_p);

      item_prop_desc.is_writable_defined = true;
      item_prop_desc.is_writable = true;

      item_prop_desc.is_enumerable_defined = true;
      item_prop_desc.is_enumerable = true;

      item_prop_desc.is_configurable_defined = true;
      item_prop_desc.is_configurable = true;
    }

    ecma_completion_value_t completion = ecma_op_object_define_own_property (new_array_p,
                                                                             index_string_p,
                                                                             &item_prop_desc,
                                                                             false);

    JERRY_ASSERT (ecma_is_completion_value_normal_true (completion));

    ecma_free_completion_value (completion);
    ecma_deref_ecma_string (index_string_p);

    index++;
  }

  return new_array;
} /* ecma_builtin_helper_object_get_properties */

/**
 * Helper function to normalizing an array index
 *
 * This function clamps the given index to the [0, length] range.
 * If the index is negative, it is used as the offset from the end of the array,
 * to compute normalized index.
 * If the index is greater than the length of the array, the normalized index will be the length of the array.
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.10 steps 5-6, 7 (part 2) and 8
 *          ECMA-262 v5, 15.4.4.12 steps 5-6
 *          ECMA-262 v5, 15.4.4.14 steps 5
 *          ECMA-262 v5, 15.5.4.13 steps 4, 5 (part 2) and 6-7
 *
 * Used by:
 *         - The Array.prototype.slice routine.
 *         - The Array.prototype.splice routine.
 *         - The Array.prototype.indexOf routine.
 *         - The String.prototype.slice routine.
 *
 * @return uint32_t - the normalized value of the index
 */
uint32_t
ecma_builtin_helper_array_index_normalize (ecma_number_t index, /**< index */
                                           uint32_t length) /**< array's length */
{
  uint32_t norm_index;

  if (!ecma_number_is_nan (index))
  {

    if (ecma_number_is_infinity (index))
    {
      norm_index = ecma_number_is_negative (index) ? 0 : length;
    }
    else
    {
      if (ecma_number_is_negative (index))
      {
        ecma_number_t index_neg = ecma_number_negate (index);

        if (index_neg > length)
        {
          norm_index = 0;
        }
        else
        {
          norm_index = length - ecma_number_to_uint32 (index_neg);
        }
      }
      else
      {
        if (index > length)
        {
          norm_index = length;
        }
        else
        {
          norm_index = ecma_number_to_uint32 (index);
        }
      }
    }
  }
  else
  {
    norm_index = 0;
  }

  return norm_index;
} /* ecma_builtin_helper_array_index_normalize */

/**
 * Helper function for concatenating an ecma_value_t to an Array.
 *
 * See also:
 *          ECMA-262 v5, 15.4.4.4 steps 5.b - 5.c
 *
 * Used by:
 *         - The Array.prototype.concat routine.
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_builtin_helper_array_concat_value (ecma_object_t *obj_p, /**< array */
                                        uint32_t *length_p, /**< in-out: array's length */
                                        ecma_value_t value) /**< value to concat */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  /* 5.b */
  if (ecma_is_value_object (value)
      && (ecma_object_get_class_name (ecma_get_object_from_value (value)) == LIT_MAGIC_STRING_ARRAY_UL))
  {
    ecma_string_t *magic_string_length_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);
    /* 5.b.ii */
    ECMA_TRY_CATCH (arg_len_value,
                    ecma_op_object_get (ecma_get_object_from_value (value),
                                        magic_string_length_p),
                    ret_value);
    ECMA_OP_TO_NUMBER_TRY_CATCH (arg_len_number, arg_len_value, ret_value);

    uint32_t arg_len = ecma_number_to_uint32 (arg_len_number);

    /* 5.b.iii */
    for (uint32_t array_index = 0;
         array_index < arg_len && ecma_is_completion_value_empty (ret_value);
         array_index++)
    {
      ecma_string_t *array_index_string_p = ecma_new_ecma_string_from_uint32 (array_index);

      /* 5.b.iii.2 */
      if (ecma_op_object_get_property (ecma_get_object_from_value (value),
                                       array_index_string_p) != NULL)
      {
        ecma_string_t *new_array_index_string_p = ecma_new_ecma_string_from_uint32 (*length_p + array_index);

        /* 5.b.iii.3.a */
        ECMA_TRY_CATCH (get_value,
                        ecma_op_object_get (ecma_get_object_from_value (value),
                                            array_index_string_p),
                        ret_value);

        ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();
        {
          prop_desc.is_value_defined = true;
          prop_desc.value = get_value;

          prop_desc.is_writable_defined = true;
          prop_desc.is_writable = true;

          prop_desc.is_enumerable_defined = true;
          prop_desc.is_enumerable = true;

          prop_desc.is_configurable_defined = true;
          prop_desc.is_configurable = true;
        }

        /* 5.b.iii.3.b */
        /* This will always be a simple value since 'is_throw' is false, so no need to free. */
        ecma_completion_value_t put_comp = ecma_op_object_define_own_property (obj_p,
                                                                               new_array_index_string_p,
                                                                               &prop_desc,
                                                                               false);
        JERRY_ASSERT (ecma_is_completion_value_normal_true (put_comp));

        ECMA_FINALIZE (get_value);
        ecma_deref_ecma_string (new_array_index_string_p);
      }

      ecma_deref_ecma_string (array_index_string_p);
    }

    *length_p += arg_len;

    ECMA_OP_TO_NUMBER_FINALIZE (arg_len_number);
    ECMA_FINALIZE (arg_len_value);
    ecma_deref_ecma_string (magic_string_length_p);
  }
  else
  {
    ecma_string_t *new_array_index_string_p = ecma_new_ecma_string_from_uint32 ((*length_p)++);

    ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();
    {
      prop_desc.is_value_defined = true;
      prop_desc.value = value;

      prop_desc.is_writable_defined = true;
      prop_desc.is_writable = true;

      prop_desc.is_enumerable_defined = true;
      prop_desc.is_enumerable = true;

      prop_desc.is_configurable_defined = true;
      prop_desc.is_configurable = true;
    }

    /* 5.c.i */
    /* This will always be a simple value since 'is_throw' is false, so no need to free. */
    ecma_completion_value_t put_comp = ecma_op_object_define_own_property (obj_p,
                                                                           new_array_index_string_p,
                                                                           &prop_desc,
                                                                           false);
    JERRY_ASSERT (ecma_is_completion_value_normal_true (put_comp));

    ecma_deref_ecma_string (new_array_index_string_p);
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
  }

  return ret_value;
} /* ecma_builtin_helper_array_concat_value */

/**
 * Helper function to normalizing a string index
 *
 * This function clamps the given index to the [0, length] range.
 * If the index is negative, 0 value is used.
 * If the index is greater than the length of the string, the normalized index will be the length of the string.
 *
 * See also:
 *          ECMA-262 v5, 15.5.4.15
 *
 * Used by:
 *         - The String.prototype.substring routine.
 *
 * @return uint32_t - the normalized value of the index
 */
uint32_t
ecma_builtin_helper_string_index_normalize (ecma_number_t index, /**< index */
                                            uint32_t length) /**< string's length */
{
  uint32_t norm_index = 0;

  if (!ecma_number_is_nan (index) && !ecma_number_is_negative (index))
  {
    if (ecma_number_is_infinity (index))
    {
      norm_index = length;
    }
    else
    {
      norm_index = ecma_number_to_uint32 (index);

      if (norm_index > length)
      {
        norm_index = length;
      }
    }
  }

  return norm_index;
} /* ecma_builtin_helper_string_index_normalize */

/**
 * @}
 * @}
 * @}
 */
