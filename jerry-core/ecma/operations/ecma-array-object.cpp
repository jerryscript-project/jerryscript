/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
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
    ecma_number_t *num_p = ecma_get_number_from_value (arguments_list_p[0]);
    uint32_t num_uint32 = ecma_number_to_uint32 (*num_p);
    if (*num_p != ecma_uint32_to_number (num_uint32))
    {
      return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_RANGE));
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

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ARRAY_BUILTIN
  ecma_object_t *array_prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_ARRAY_PROTOTYPE);
#else /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ARRAY_BUILTIN */
  ecma_object_t *array_prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);
#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ARRAY_BUILTIN */

  ecma_object_t *obj_p = ecma_create_object (array_prototype_obj_p, true, ECMA_OBJECT_TYPE_ARRAY);
  ecma_deref_object (array_prototype_obj_p);

  /*
   * [[Class]] property is not stored explicitly for objects of ECMA_OBJECT_TYPE_ARRAY type.
   *
   * See also: ecma_object_get_class_name
   */

  ecma_string_t *length_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);
  ecma_number_t *length_num_p = ecma_alloc_number ();
  *length_num_p = ecma_uint32_to_number (length);

  ecma_property_t *length_prop_p = ecma_create_named_data_property (obj_p,
                                                                    length_magic_string_p,
                                                                    true, false, false);
  ecma_set_named_data_property_value (length_prop_p, ecma_make_number_value (length_num_p));

  ecma_deref_ecma_string (length_magic_string_p);

  for (uint32_t index = 0;
       index < array_items_count;
       index++)
  {
    if (ecma_is_value_array_hole (array_items_p[index]))
    {
      continue;
    }

    ecma_string_t* item_name_string_p = ecma_new_ecma_string_from_uint32 (index);

    ecma_builtin_helper_def_prop (obj_p,
                                  item_name_string_p,
                                  array_items_p[index],
                                  true, /* Writable */
                                  true, /* Enumerable */
                                  true, /* Configurable */
                                  false); /* Failure handling */

    ecma_deref_ecma_string (item_name_string_p);
  }

  return ecma_make_normal_completion_value (ecma_make_object_value (obj_p));
} /* ecma_op_create_array_object */

/**
 * [[DefineOwnProperty]] ecma array object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 15.4.5.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_array_object_define_own_property (ecma_object_t *obj_p, /**< the array object */
                                          ecma_string_t *property_name_p, /**< property name */
                                          const ecma_property_descriptor_t* property_desc_p, /**< property descriptor */
                                          bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_ARRAY);


  // 1.
  ecma_string_t* magic_string_length_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);
  ecma_property_t *len_prop_p = ecma_op_object_get_own_property (obj_p, magic_string_length_p);
  JERRY_ASSERT (len_prop_p != NULL && len_prop_p->type == ECMA_PROPERTY_NAMEDDATA);

  // 2.
  ecma_value_t old_len_value = ecma_get_named_data_property_value (len_prop_p);

  ecma_number_t *num_p = ecma_get_number_from_value (old_len_value);
  uint32_t old_len_uint32 = ecma_number_to_uint32 (*num_p);

  // 3.
  bool is_property_name_equal_length = ecma_compare_ecma_strings (property_name_p,
                                                                  magic_string_length_p);

  ecma_deref_ecma_string (magic_string_length_p);

  if (is_property_name_equal_length)
  {
    // a.
    if (!property_desc_p->is_value_defined)
    {
      // i.
      return ecma_op_general_object_define_own_property (obj_p, property_name_p, property_desc_p, is_throw);
    }

    ecma_number_t new_len_num;

    // c.
    ecma_completion_value_t completion = ecma_op_to_number (property_desc_p->value);
    if (ecma_is_completion_value_throw (completion))
    {
      return completion;
    }

    JERRY_ASSERT (ecma_is_completion_value_normal (completion)
                  && ecma_is_value_number (ecma_get_completion_value_value (completion)));

    new_len_num = *ecma_get_number_from_completion_value (completion);

    ecma_free_completion_value (completion);

    uint32_t new_len_uint32 = ecma_number_to_uint32 (new_len_num);

    // d.
    if (ecma_uint32_to_number (new_len_uint32) != new_len_num)
    {
      return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_RANGE));
    }
    else
    {
      // b., e.
      ecma_number_t *new_len_num_p = ecma_alloc_number ();
      *new_len_num_p = new_len_num;

      ecma_property_descriptor_t new_len_property_desc = *property_desc_p;
      new_len_property_desc.value = ecma_make_number_value (new_len_num_p);

      ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

      // f.
      if (new_len_uint32 >= old_len_uint32)
      {
        // i.
        magic_string_length_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);
        ret_value = ecma_op_general_object_define_own_property (obj_p,
                                                                magic_string_length_p,
                                                                &new_len_property_desc,
                                                                is_throw);
        ecma_deref_ecma_string (magic_string_length_p);
      }
      else
      {
        // g.
        if (!ecma_is_property_writable (len_prop_p))
        {
          ret_value = ecma_reject (is_throw);
        }
        else
        {
          // h.
          bool new_writable;
          if (!new_len_property_desc.is_writable_defined
              || new_len_property_desc.is_writable)
          {
            new_writable = true;
          }
          else
          {
            // ii.
            new_writable = false;

            // iii.
            new_len_property_desc.is_writable_defined = true;
            new_len_property_desc.is_writable = true;
          }

          // j.
          magic_string_length_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);
          ecma_completion_value_t succeeded = ecma_op_general_object_define_own_property (obj_p,
                                                                                          magic_string_length_p,
                                                                                          &new_len_property_desc,
                                                                                          is_throw);
          ecma_deref_ecma_string (magic_string_length_p);

          /* Handling normal false and throw values */
          if (!ecma_is_completion_value_normal_true (succeeded))
          {
            JERRY_ASSERT (ecma_is_completion_value_normal_false (succeeded)
                          || ecma_is_completion_value_throw (succeeded));

            // k.
            ret_value = succeeded;
          }
          else
          {
            // l
            JERRY_ASSERT (new_len_uint32 < old_len_uint32);

            /*
             * Item i. is replaced with faster iteration: only indices that actually exist in the array, are iterated
             */
            bool is_reduce_succeeded = true;

            ecma_collection_header_t *array_index_props_p = ecma_op_object_get_property_names (obj_p,
                                                                                               true,
                                                                                               false,
                                                                                               false);

            ecma_length_t array_index_props_num = array_index_props_p->unit_number;

            MEM_DEFINE_LOCAL_ARRAY (array_index_values_p, array_index_props_num, uint32_t);

            ecma_collection_iterator_t iter;
            ecma_collection_iterator_init (&iter, array_index_props_p);

            uint32_t array_index_values_pos = 0;

            while (ecma_collection_iterator_next (&iter))
            {
              ecma_string_t *property_name_p = ecma_get_string_from_value (*iter.current_value_p);

              uint32_t index;
              bool is_index = ecma_string_get_array_index (property_name_p, &index);
              JERRY_ASSERT (is_index);
              JERRY_ASSERT (index < old_len_uint32);

              array_index_values_p[array_index_values_pos++] = index;
            }

            JERRY_ASSERT (array_index_values_pos == array_index_props_num);

            while (array_index_values_pos != 0
                   && array_index_values_p[--array_index_values_pos] >= new_len_uint32)
            {
              uint32_t index = array_index_values_p[array_index_values_pos];

              // ii.
              ecma_string_t *index_string_p = ecma_new_ecma_string_from_uint32 (index);
              ecma_completion_value_t delete_succeeded = ecma_op_object_delete (obj_p,
                                                                                index_string_p,
                                                                                false);
              ecma_deref_ecma_string (index_string_p);

              if (ecma_is_completion_value_normal_false (delete_succeeded))
              {
                // iii.
                new_len_uint32 = (index + 1u);

                ecma_number_t *new_len_num_p = ecma_get_number_from_value (new_len_property_desc.value);

                // 1.
                *new_len_num_p = ecma_uint32_to_number (index + 1u);

                // 2.
                if (!new_writable)
                {
                  new_len_property_desc.is_writable_defined = true;
                  new_len_property_desc.is_writable = false;
                }

                // 3.
                ecma_string_t *magic_string_length_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);
                ecma_completion_value_t completion = ecma_op_general_object_define_own_property (obj_p,
                                                                                                 magic_string_length_p,
                                                                                                 &new_len_property_desc,
                                                                                                 false);
                ecma_deref_ecma_string (magic_string_length_p);

                JERRY_ASSERT (ecma_is_completion_value_normal_true (completion)
                              || ecma_is_completion_value_normal_false (completion));

                is_reduce_succeeded = false;

                break;
              }
            }

            MEM_FINALIZE_LOCAL_ARRAY (array_index_values_p);

            ecma_free_values_collection (array_index_props_p, true);

            if (!is_reduce_succeeded)
            {
              ret_value = ecma_reject (is_throw);
            }
            else
            {
              // m.
              if (!new_writable)
              {
                ecma_property_descriptor_t prop_desc_not_writable = ecma_make_empty_property_descriptor ();

                prop_desc_not_writable.is_writable_defined = true;
                prop_desc_not_writable.is_writable = false;

                ecma_completion_value_t completion_set_not_writable;
                magic_string_length_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);
                completion_set_not_writable = ecma_op_general_object_define_own_property (obj_p,
                                                                                          magic_string_length_p,
                                                                                          &prop_desc_not_writable,
                                                                                          false);
                ecma_deref_ecma_string (magic_string_length_p);
                JERRY_ASSERT (ecma_is_completion_value_normal_true (completion_set_not_writable));
              }

              ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
            }
          }
        }
      }

      ecma_dealloc_number (new_len_num_p);

      return ret_value;
    }

    JERRY_UNREACHABLE ();
  }
  else
  {
    // 4.a.
    uint32_t index;

    if (!ecma_string_get_array_index (property_name_p, &index))
    {
      // 5.
      return ecma_op_general_object_define_own_property (obj_p,
                                                         property_name_p,
                                                         property_desc_p,
                                                         is_throw);
    }

    // 4.

    // b.
    if (index >= old_len_uint32
        && !ecma_is_property_writable (len_prop_p))
    {
      return ecma_reject (is_throw);
    }

    // c.
    ecma_completion_value_t succeeded = ecma_op_general_object_define_own_property (obj_p,
                                                                                    property_name_p,
                                                                                    property_desc_p,
                                                                                    false);
    // d.
    JERRY_ASSERT (ecma_is_completion_value_normal_true (succeeded)
                  || ecma_is_completion_value_normal_false (succeeded));

    if (ecma_is_completion_value_normal_false (succeeded))
    {
      return ecma_reject (is_throw);
    }

    // e.
    if (index >= old_len_uint32)
    {
      // i., ii.
      ecma_number_t *num_p = ecma_alloc_number ();
      *num_p = ecma_number_add (ecma_uint32_to_number (index), ECMA_NUMBER_ONE);

      ecma_named_data_property_assign_value (obj_p, len_prop_p, ecma_make_number_value (num_p));

      ecma_dealloc_number (num_p);
    }

    // f.
    return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
  }

  JERRY_UNREACHABLE ();
} /* ecma_op_array_object_define_own_property */

/**
 * @}
 * @}
 */
