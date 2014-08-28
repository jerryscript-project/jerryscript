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

#include "ecma-alloc.h"
#include "ecma-array-object.h"
#include "ecma-exceptions.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-magic-strings.h"
#include "ecma-number-arithmetic.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"

/** \addtogroup ecma ---TODO---
 * @{
 *
 * \addtogroup ecmaarrayobject ECMA Array object related routines
 * @{
 */

/**
 * Reject sequence
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
ecma_reject (bool is_throw) /**< Throw flag */
{
  if (is_throw)
  {
    return ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE);
  }
} /* ecma_reject */

/**
 * Array object creation operation.
 *
 * See also: ECMA-262 v5, 15.4.2.1
 *           ECMA-262 v5, 15.4.2.2
 *
 * @return pointer to newly created Array object
 */
ecma_object_t*
ecma_op_create_array_object (ecma_value_t *arguments_list_p, /**< list of arguments that
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

  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (arguments_list_p, arguments_list_len, is_treat_single_arg_as_length);
} /* ecma_op_create_array_object */

/**
 * Reduce length of Array to specified value.
 *
 * Warning:
 *         the routine may change value of number, stored in passed property descriptor template.
 *
 * See also:
 *          ECMA-262 v5, 15.4.5.1, Block 3.l
 *
 * @return true - length was reduced successfully,
 *         false - reduce routine stopped at some length > new_length
 *                 because element with index (length - 1) cannot be deleted.
 */
static bool
ecma_array_object_reduce_length (ecma_object_t *obj_p, /**< the array object */
                                 uint32_t new_length, /**< length to reduce to */
                                 uint32_t old_length, /**< current length of array */
                                 ecma_property_descriptor_t new_len_desc) /**< property descriptor template
                                                                               for length property */
{
  JERRY_ASSERT (new_length < old_length);

  while (new_length < old_length)
  {
    // i.
    old_length--;

    // ii.
    ecma_string_t *old_length_string_p = ecma_new_ecma_string_from_number (ecma_uint32_to_number (old_length));
    ecma_completion_value_t delete_succeeded = ecma_op_object_delete (obj_p,
                                                                      old_length_string_p,
                                                                      false);
    ecma_deref_ecma_string (old_length_string_p);

    // iii.
    if (ecma_is_completion_value_normal_false (delete_succeeded))
    {
      JERRY_ASSERT (new_len_desc.value.value_type == ECMA_TYPE_NUMBER);

      ecma_number_t *new_len_num_p = ECMA_GET_POINTER (new_len_desc.value.value);

      // 1.
      *new_len_num_p = ecma_uint32_to_number (old_length + 1);

      // 2.
      JERRY_ASSERT (new_len_desc.is_writable_defined
                    && new_len_desc.writable == ECMA_PROPERTY_NOT_WRITABLE);

      // 3.
      ecma_string_t *magic_string_length_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);
      ecma_completion_value_t completion = ecma_op_general_object_define_own_property (obj_p,
                                                                                       magic_string_length_p,
                                                                                       new_len_desc,
                                                                                       false);
      ecma_deref_ecma_string (magic_string_length_p);

      JERRY_ASSERT (ecma_is_completion_value_normal_true (completion)
                    || ecma_is_completion_value_normal_false (completion));

      return false;
    }
    else
    {
      JERRY_ASSERT (ecma_is_completion_value_normal_true (delete_succeeded));
    }
  }

  return true;
} /* ecma_array_object_reduce_length */

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
                                          ecma_property_descriptor_t property_desc, /**< property descriptor */
                                          bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT (obj_p->u.object.type == ECMA_OBJECT_TYPE_ARRAY);


  // 1.
  ecma_string_t* magic_string_length_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);
  ecma_property_t *len_prop_p = ecma_op_object_get_own_property (obj_p, magic_string_length_p);
  JERRY_ASSERT (len_prop_p != NULL && len_prop_p->type == ECMA_PROPERTY_NAMEDDATA);

  // 2.
  ecma_value_t old_len_value = len_prop_p->u.named_data_property.value;

  JERRY_ASSERT (old_len_value.value_type == ECMA_TYPE_NUMBER);

  ecma_number_t *num_p = ECMA_GET_POINTER (old_len_value.value);
  uint32_t old_len_uint32 = ecma_number_to_uint32 (*num_p);

  // 3.
  bool is_property_name_equal_length = (ecma_compare_ecma_string_to_ecma_string (property_name_p,
                                                                                 magic_string_length_p) == 0);

  ecma_deref_ecma_string (magic_string_length_p);

  if (is_property_name_equal_length)
  {
    // a.
    if (!property_desc.is_value_defined)
    {
      // i.
      return ecma_op_general_object_define_own_property (obj_p, property_name_p, property_desc, is_throw);
    }

    ecma_number_t new_len_num;

    // c.
    ecma_completion_value_t completion = ecma_op_to_number (property_desc.value);
    if (ecma_is_completion_value_throw (completion))
    {
      return completion;
    }

    JERRY_ASSERT (ecma_is_completion_value_normal (completion)
                  && completion.u.value.value_type == ECMA_TYPE_NUMBER);

    new_len_num = *(ecma_number_t*) ECMA_GET_POINTER (completion.u.value.value);

    ecma_free_completion_value (completion);

    uint32_t new_len_uint32 = ecma_number_to_uint32 (new_len_num);

    // d.
    if (ecma_uint32_to_number (new_len_uint32) != new_len_num)
    {
      return ecma_make_throw_value (ecma_new_standard_error (ECMA_ERROR_RANGE));
    }
    else
    {
      // b., e.
      ecma_number_t *new_len_num_p = ecma_alloc_number ();
      *new_len_num_p = new_len_num;

      ecma_property_descriptor_t new_len_property_desc = property_desc;
      new_len_property_desc.value = ecma_make_number_value (new_len_num_p);

      ecma_completion_value_t ret_value;

      // f.
      if (new_len_uint32 >= old_len_uint32)
      {
        // i.
        magic_string_length_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);
        ret_value = ecma_op_general_object_define_own_property (obj_p,
                                                                magic_string_length_p,
                                                                new_len_property_desc,
                                                                is_throw);
        ecma_deref_ecma_string (magic_string_length_p);
      }
      else
      {
        // g.
        if (len_prop_p->u.named_data_property.writable == ECMA_PROPERTY_NOT_WRITABLE)
        {
          ret_value = ecma_reject (is_throw);
        }
        else
        {
          // h.
          bool new_writable;
          if (!new_len_property_desc.is_writable_defined
              || new_len_property_desc.writable == ECMA_PROPERTY_WRITABLE)
          {
            new_writable = true;
          }
          else
          {
            // ii.
            new_writable = false;

            // iii.
            new_len_property_desc.is_writable_defined = true;
            new_len_property_desc.writable = ECMA_PROPERTY_WRITABLE;
          }

          // j.
          magic_string_length_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);
          ecma_completion_value_t succeeded = ecma_op_general_object_define_own_property (obj_p,
                                                                                          magic_string_length_p,
                                                                                          new_len_property_desc,
                                                                                          is_throw);
          ecma_deref_ecma_string (magic_string_length_p);

          /* Handling normal false and throw values */
          if (!ecma_is_completion_value_normal_true (succeeded))
          {
            // k.
            ret_value = succeeded;
          }
          else
          {
            // l
            
            // iii.2
            if (!new_writable)
            {
              new_len_property_desc.is_writable_defined = true;
              new_len_property_desc.writable = ECMA_PROPERTY_NOT_WRITABLE;
            }

            bool reduce_succeeded = ecma_array_object_reduce_length (obj_p,
                                                                     new_len_uint32,
                                                                     old_len_uint32,
                                                                     new_len_property_desc);

            if (!reduce_succeeded)
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
                prop_desc_not_writable.writable = ECMA_PROPERTY_NOT_WRITABLE;

                ecma_completion_value_t completion_set_not_writable;
                magic_string_length_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);
                completion_set_not_writable = ecma_op_general_object_define_own_property (obj_p,
                                                                                          magic_string_length_p,
                                                                                          prop_desc_not_writable,
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

    JERRY_UNREACHABLE();
  }
  else
  {
    // 4.a.
    ecma_number_t number = ecma_string_to_number (property_name_p);
    uint32_t index = ecma_number_to_uint32 (number);

    TODO (Check if array index recognition is done according to ECMA);

    if (index >= ECMA_MAX_VALUE_OF_VALID_ARRAY_INDEX
        || ecma_uint32_to_number (index) != number)
    {
      // 5.
      return ecma_op_general_object_define_own_property (obj_p,
                                                         property_name_p,
                                                         property_desc,
                                                         false);
    }

    // 4.

    // b.
    if (index >= old_len_uint32
        && len_prop_p->u.named_data_property.writable == ECMA_PROPERTY_NOT_WRITABLE)
    {
      return ecma_reject (is_throw);
    }

    // c.
    ecma_completion_value_t succeeded = ecma_op_general_object_define_own_property (obj_p,
                                                                                    property_name_p,
                                                                                    property_desc,
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
      *num_p = ecma_op_number_add (ecma_uint32_to_number (index), ECMA_NUMBER_ONE);

      ecma_free_value (len_prop_p->u.named_data_property.value, false);
      len_prop_p->u.named_data_property.value = ecma_make_number_value (num_p);
    }

    // f.
    return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
  }

  JERRY_UNREACHABLE();
} /* ecma_op_array_object_define_own_property */

/**
 * @}
 * @}
 */
