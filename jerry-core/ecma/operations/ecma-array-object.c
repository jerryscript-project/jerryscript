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
  ecma_object_t *array_prototype_object_p = ecma_builtin_get (ECMA_BUILTIN_ID_ARRAY_PROTOTYPE);
#else /* CONFIG_DISABLE_ARRAY_BUILTIN */
  ecma_object_t *array_prototype_object_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);
#endif /* !CONFIG_DISABLE_ARRAY_BUILTIN */

  ecma_object_t *object_p = ecma_create_object (array_prototype_object_p,
                                                sizeof (ecma_extended_object_t),
                                                ECMA_OBJECT_TYPE_ARRAY);

  ecma_deref_object (array_prototype_object_p);

  /*
   * [[Class]] property is not stored explicitly for objects of ECMA_OBJECT_TYPE_ARRAY type.
   *
   * See also: ecma_object_get_class_name
   */

  ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) object_p;
  ext_obj_p->u.array.length = length;
  ext_obj_p->u.array.length_prop = ECMA_PROPERTY_FLAG_WRITABLE | ECMA_PROPERTY_TYPE_VIRTUAL;

  for (uint32_t index = 0;
       index < array_items_count;
       index++)
  {
    if (ecma_is_value_array_hole (array_items_p[index]))
    {
      continue;
    }

    ecma_string_t *item_name_string_p = ecma_new_ecma_string_from_uint32 (index);

    ecma_builtin_helper_def_prop (object_p,
                                  item_name_string_p,
                                  array_items_p[index],
                                  true, /* Writable */
                                  true, /* Enumerable */
                                  true, /* Configurable */
                                  false); /* Failure handling */

    ecma_deref_ecma_string (item_name_string_p);
  }

  return ecma_make_object_value (object_p);
} /* ecma_op_create_array_object */

/**
 * Update the length of an array to a new length
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
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

    if (ECMA_IS_VALUE_ERROR (compared_num_val))
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

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

  uint32_t old_len_uint32 = ext_object_p->u.array.length;

  if (new_len_num == old_len_uint32)
  {
    /* Only the writable flag must be updated. */
    if (flags & ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_WRITABLE_DEFINED)
    {
      if (!(flags & ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_WRITABLE))
      {
        uint8_t new_prop_value = (uint8_t) (ext_object_p->u.array.length_prop & ~ECMA_PROPERTY_FLAG_WRITABLE);
        ext_object_p->u.array.length_prop = new_prop_value;
      }
      else if (!ecma_is_property_writable (ext_object_p->u.array.length_prop))
      {
        return ecma_reject (is_throw);
      }
    }
    return ECMA_VALUE_TRUE;
  }
  else if (!ecma_is_property_writable (ext_object_p->u.array.length_prop))
  {
    return ecma_reject (is_throw);
  }

  uint32_t current_len_uint32 = new_len_uint32;

  if (new_len_uint32 < old_len_uint32)
  {
    current_len_uint32 = ecma_delete_array_properties (object_p, new_len_uint32, old_len_uint32);
  }

  ext_object_p->u.array.length = current_len_uint32;

  if ((flags & ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_WRITABLE_DEFINED)
      && !(flags & ECMA_ARRAY_OBJECT_SET_LENGTH_FLAG_WRITABLE))
  {
    uint8_t new_prop_value = (uint8_t) (ext_object_p->u.array.length_prop & ~ECMA_PROPERTY_FLAG_WRITABLE);
    ext_object_p->u.array.length_prop = new_prop_value;
  }

  if (current_len_uint32 == new_len_uint32)
  {
    return ECMA_VALUE_TRUE;
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

    if (property_desc_p->is_value_defined)
    {
      return ecma_op_array_object_set_length (object_p, property_desc_p->value, flags);
    }

    ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
    ecma_value_t length_value = ecma_make_uint32_value (ext_object_p->u.array.length);

    ecma_value_t result = ecma_op_array_object_set_length (object_p, length_value, flags);

    ecma_fast_free_value (length_value);
    return result;
  }

  uint32_t index = ecma_string_get_array_index (property_name_p);

  if (index == ECMA_STRING_NOT_ARRAY_INDEX)
  {
    return ecma_op_general_object_define_own_property (object_p, property_name_p, property_desc_p, is_throw);
  }

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

  bool update_length = (index >= ext_object_p->u.array.length);

  if (update_length && !ecma_is_property_writable (ext_object_p->u.array.length_prop))
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
    ext_object_p->u.array.length = index + 1;
  }

  return ECMA_VALUE_TRUE;
} /* ecma_op_array_object_define_own_property */

/**
 * List names of a String object's lazy instantiated properties
 *
 * @return string values collection
 */
void
ecma_op_array_list_lazy_property_names (ecma_object_t *obj_p, /**< a String object */
                                        bool separate_enumerable, /**< true -  list enumerable properties
                                                                   *           into main collection,
                                                                   *           and non-enumerable to collection of
                                                                   *           'skipped non-enumerable' properties,
                                                                   *   false - list all properties into main
                                                                   *           collection.
                                                                   */
                                        ecma_collection_header_t *main_collection_p, /**< 'main'
                                                                                      *   collection */
                                        ecma_collection_header_t *non_enum_collection_p) /**< skipped
                                                                                          *   'non-enumerable'
                                                                                          *   collection */
{
  JERRY_ASSERT (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_ARRAY);

  ecma_collection_header_t *for_non_enumerable_p = separate_enumerable ? non_enum_collection_p : main_collection_p;

  ecma_string_t *length_str_p = ecma_new_ecma_length_string ();
  ecma_append_to_values_collection (for_non_enumerable_p, ecma_make_string_value (length_str_p), true);
  ecma_deref_ecma_string (length_str_p);
} /* ecma_op_array_list_lazy_property_names */

/**
 * @}
 * @}
 */
