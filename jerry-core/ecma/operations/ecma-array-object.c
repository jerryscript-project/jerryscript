/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
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
#include "ecma-array-object.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-number-arithmetic.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaarrayobject ECMA Array object related routines
 * @{
 */

/**
 * Array object creation operation.
 *
 * See also: ECMA-262 v5, 15.4.2.1
 *           ECMA-262 v5, 15.4.2.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_create_array_object (const ecma_value_t *arguments_list_p, /**< list of arguments that
                                                                        are passed to Array constructor */
                             ecma_length_t arguments_list_len, /**< length of the arguments' list */
                             bool is_treat_single_arg_as_length) /**< if the value is true,
                                                                      arguments_list_len is 1
                                                                      and single argument is Number,
                                                                      then treat the single argument
                                                                      as new Array's length rather
                                                                      than as single item of the Array */
{
  JERRY_ASSERT (arguments_list_len == 0
                || arguments_list_p != NULL);

  uint32_t length;
  const ecma_value_t *array_items_p;
  ecma_length_t array_items_count;

  if (is_treat_single_arg_as_length
      && arguments_list_len == 1
      && ecma_is_value_number (arguments_list_p[0]))
  {
    ecma_number_t num = ecma_get_number_from_value (arguments_list_p[0]);
    uint32_t num_uint32 = ecma_number_to_uint32 (num);

    if (num != ((ecma_number_t) num_uint32))
    {
      return ecma_raise_range_error (ECMA_ERR_MSG ("Invalid array length."));
    }
    else
    {
      length = num_uint32;
      array_items_p = NULL;
      array_items_count = 0;
    }
  }
  else
  {
    length = arguments_list_len;
    array_items_p = arguments_list_p;
    array_items_count = arguments_list_len;
  }

#ifndef CONFIG_DISABLE_ARRAY_BUILTIN
  ecma_object_t *array_prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_ARRAY_PROTOTYPE);
#else /* CONFIG_DISABLE_ARRAY_BUILTIN */
  ecma_object_t *array_prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);
#endif /* !CONFIG_DISABLE_ARRAY_BUILTIN */

  ecma_object_t *obj_p = ecma_create_object (array_prototype_obj_p,
                                             0,
                                             ECMA_OBJECT_TYPE_ARRAY);

  ecma_deref_object (array_prototype_obj_p);

  /*
   * [[Class]] property is not stored explicitly for objects of ECMA_OBJECT_TYPE_ARRAY type.
   *
   * See also: ecma_object_get_class_name
   */

  ecma_string_t *length_magic_string_p = ecma_new_ecma_length_string ();

  ecma_property_value_t *length_prop_value_p;
  length_prop_value_p = ecma_create_named_data_property (obj_p,
                                                         length_magic_string_p,
                                                         ECMA_PROPERTY_FLAG_WRITABLE,
                                                         NULL);

  length_prop_value_p->value = ecma_make_number_value ((ecma_number_t) length);

  ecma_deref_ecma_string (length_magic_string_p);

  for (uint32_t index = 0;
       index < array_items_count;
       index++)
  {
    if (ecma_is_value_array_hole (array_items_p[index]))
    {
      continue;
    }

    ecma_string_t *item_name_string_p = ecma_new_ecma_string_from_uint32 (index);

    ecma_builtin_helper_def_prop (obj_p,
                                  item_name_string_p,
                                  array_items_p[index],
                                  true, /* Writable */
                                  true, /* Enumerable */
                                  true, /* Configurable */
                                  false); /* Failure handling */

    ecma_deref_ecma_string (item_name_string_p);
  }

  return ecma_make_object_value (obj_p);
} /* ecma_op_create_array_object */

/**
 * Update the length of an array to a new length
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
extern ecma_value_t
ecma_op_array_object_set_length (ecma_object_t *object_p, /**< the array object */
                                 ecma_value_t new_value, /**< new length value */
                                 uint32_t flags) /**< configuration options */
{
  bool is_throw = (flags & ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_IS_THROW);

  ecma_value_t completion = ecma_op_to_number (new_value);

  if (ECMA_IS_VALUE_ERROR (completion))
  {
    return completion;
  }

  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (completion)
                && ecma_is_value_number (completion));

  ecma_number_t new_len_num = ecma_get_number_from_value (completion);

  ecma_free_value (completion);

  if (ecma_is_value_object (new_value))
  {
    ecma_value_t compared_num_val = ecma_op_to_number (new_value);

    if (ECMA_IS_VALUE_ERROR (completion))
    {
      return compared_num_val;
    }

    new_len_num = ecma_get_number_from_value (compared_num_val);
    ecma_free_value (compared_num_val);
  }

  uint32_t new_len_uint32 = ecma_number_to_uint32 (new_len_num);

  if (((ecma_number_t) new_len_uint32) != new_len_num)
  {
    return ecma_raise_range_error (ECMA_ERR_MSG ("Invalid array length."));
  }

  if (flags & ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_REJECT)
  {
    return ecma_reject (is_throw);
  }

  ecma_string_t magic_string_length;
  ecma_init_ecma_length_string (&magic_string_length);

  ecma_property_t *len_prop_p = ecma_find_named_property (object_p, &magic_string_length);
  JERRY_ASSERT (len_prop_p != NULL
                && ECMA_PROPERTY_GET_TYPE (*len_prop_p) == ECMA_PROPERTY_TYPE_NAMEDDATA);

  ecma_property_value_t *len_prop_value_p = ECMA_PROPERTY_VALUE_PTR (len_prop_p);
  uint32_t old_len_uint32 = ecma_get_uint32_from_value (len_prop_value_p->value);

  if (new_len_num == old_len_uint32)
  {
    /* Only the writable flag must be updated. */
    if (flags & ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_WRITABLE_DEFINED)
    {
      if (!(flags & ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_WRITABLE))
      {
        ecma_set_property_writable_attr (len_prop_p, false);
      }
      else if (!ecma_is_property_writable (*len_prop_p))
      {
        return ecma_reject (is_throw);
      }
    }
    return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
  }
  else if (!ecma_is_property_writable (*len_prop_p))
  {
    return ecma_reject (is_throw);
  }

  uint32_t current_len_uint32 = new_len_uint32;

  if (new_len_uint32 < old_len_uint32)
  {
    current_len_uint32 = ecma_delete_array_properties (object_p, new_len_uint32, old_len_uint32);
  }

  ecma_value_assign_uint32 (&len_prop_value_p->value, current_len_uint32);

  if ((flags & ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_WRITABLE_DEFINED)
      && !(flags & ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_WRITABLE))
  {
    ecma_set_property_writable_attr (len_prop_p, false);
  }

  if (current_len_uint32 == new_len_uint32)
  {
    return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
  }
  return ecma_reject (is_throw);
} /* ecma_op_array_object_set_length */

/**
 * [[DefineOwnProperty]] ecma array object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 15.4.5.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_array_object_define_own_property (ecma_object_t *object_p, /**< the array object */
                                          ecma_string_t *property_name_p, /**< property name */
                                          const ecma_property_descriptor_t *property_desc_p, /**< property descriptor */
                                          bool is_throw) /**< flag that controls failure handling */
{
  if (ecma_string_is_length (property_name_p))
  {
    if (!property_desc_p->is_value_defined)
    {
      return ecma_op_general_object_define_own_property (object_p, property_name_p, property_desc_p, is_throw);
    }

    JERRY_ASSERT (property_desc_p->is_configurable_defined || !property_desc_p->is_configurable);
    JERRY_ASSERT (property_desc_p->is_enumerable_defined || !property_desc_p->is_enumerable);
    JERRY_ASSERT (property_desc_p->is_writable_defined || !property_desc_p->is_writable);

    uint32_t flags = 0;

    if (is_throw)
    {
      flags |= ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_IS_THROW;
    }

    /* Only the writable and data properties can be modified. */
    if (property_desc_p->is_configurable
        || property_desc_p->is_enumerable
        || property_desc_p->is_get_defined
        || property_desc_p->is_set_defined)
    {
      flags |= ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_REJECT;
    }

    if (property_desc_p->is_writable_defined)
    {
      flags |= ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_WRITABLE_DEFINED;
    }

    if (property_desc_p->is_writable)
    {
      flags |= ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_WRITABLE;
    }

    return ecma_op_array_object_set_length (object_p, property_desc_p->value, flags);
  }

  uint32_t index;

  if (!ecma_string_get_array_index (property_name_p, &index))
  {
    return ecma_op_general_object_define_own_property (object_p, property_name_p, property_desc_p, is_throw);
  }

  ecma_string_t magic_string_length;
  ecma_init_ecma_length_string (&magic_string_length);

  ecma_property_t *len_prop_p = ecma_find_named_property (object_p, &magic_string_length);
  JERRY_ASSERT (len_prop_p != NULL
                && ECMA_PROPERTY_GET_TYPE (*len_prop_p) == ECMA_PROPERTY_TYPE_NAMEDDATA);

  ecma_property_value_t *len_prop_value_p = ECMA_PROPERTY_VALUE_PTR (len_prop_p);

  bool update_length = (index >= ecma_get_uint32_from_value (len_prop_value_p->value));

  if (update_length && !ecma_is_property_writable (*len_prop_p))
  {
    return ecma_reject (is_throw);
  }

  ecma_value_t completition = ecma_op_general_object_define_own_property (object_p,
                                                                          property_name_p,
                                                                          property_desc_p,
                                                                          false);
  JERRY_ASSERT (ecma_is_value_boolean (completition));

  if (ecma_is_value_false (completition))
  {
    return ecma_reject (is_throw);
  }

  if (update_length)
  {
    ecma_value_assign_uint32 (&len_prop_value_p->value, index + 1);
  }
  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
} /* ecma_op_array_object_define_own_property */

/**
 * @}
 * @}
 */
