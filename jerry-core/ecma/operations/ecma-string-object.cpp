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
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"
#include "ecma-string-object.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmastringobject ECMA String object related routines
 * @{
 */

/**
 * String object creation operation.
 *
 * See also: ECMA-262 v5, 15.5.2.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_create_string_object (const ecma_value_t *arguments_list_p, /**< list of arguments that
                                                                         are passed to String constructor */
                              ecma_length_t arguments_list_len) /**< length of the arguments' list */
{
  JERRY_ASSERT (arguments_list_len == 0
                || arguments_list_p != NULL);

  ecma_string_t* prim_prop_str_value_p;

  ecma_number_t length_value;

  if (arguments_list_len == 0)
  {
    prim_prop_str_value_p = ecma_new_ecma_string_from_magic_string_id (LIT_MAGIC_STRING__EMPTY);
    length_value = ECMA_NUMBER_ZERO;
  }
  else
  {
    ecma_completion_value_t to_str_arg_value = ecma_op_to_string (arguments_list_p[0]);

    if (ecma_is_completion_value_throw (to_str_arg_value))
    {
      return to_str_arg_value;
    }
    else
    {
      JERRY_ASSERT (ecma_is_completion_value_normal (to_str_arg_value));

      JERRY_ASSERT (ecma_is_value_string (ecma_get_completion_value_value (to_str_arg_value)));
      prim_prop_str_value_p = ecma_get_string_from_completion_value (to_str_arg_value);

      ecma_length_t string_len = ecma_string_get_length (prim_prop_str_value_p);
      length_value = ecma_uint32_to_number ((uint32_t) string_len);
    }
  }

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_STRING_BUILTIN
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_STRING_PROTOTYPE);
#else /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_STRING_BUILTIN */
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);
#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_STRING_BUILTIN */

  ecma_object_t *obj_p = ecma_create_object (prototype_obj_p,
                                             true,
                                             ECMA_OBJECT_TYPE_STRING);
  ecma_deref_object (prototype_obj_p);

  /*
   * [[Class]] property is not stored explicitly for objects of ECMA_OBJECT_TYPE_STRING type.
   *
   * See also: ecma_object_get_class_name
   */

  ecma_property_t *prim_value_prop_p = ecma_create_internal_property (obj_p,
                                                                      ECMA_INTERNAL_PROPERTY_PRIMITIVE_STRING_VALUE);
  ECMA_SET_POINTER (prim_value_prop_p->u.internal_property.value, prim_prop_str_value_p);

  // 15.5.5.1
  ecma_string_t *length_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);
  ecma_property_t *length_prop_p = ecma_create_named_data_property (obj_p,
                                                                    length_magic_string_p,
                                                                    false, false, false);
  ecma_number_t *length_prop_value_p = ecma_alloc_number ();
  *length_prop_value_p = length_value;
  ecma_set_named_data_property_value (length_prop_p, ecma_make_number_value (length_prop_value_p));
  ecma_deref_ecma_string (length_magic_string_p);

  return ecma_make_normal_completion_value (ecma_make_object_value (obj_p));
} /* ecma_op_create_string_object */

/**
 * [[GetOwnProperty]] ecma String object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 15.5.5.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_property_t*
ecma_op_string_object_get_own_property (ecma_object_t *obj_p, /**< a String object */
                                        ecma_string_t *property_name_p) /**< property name */
{
  JERRY_ASSERT (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_STRING);

  // 1.
  ecma_property_t *prop_p = ecma_op_general_object_get_own_property (obj_p, property_name_p);

  // 2.
  if (prop_p != NULL)
  {
    return prop_p;
  }

  // 3., 5.
  uint32_t uint32_index;
  ecma_string_t *new_prop_name_p;

  if (property_name_p->container == ECMA_STRING_CONTAINER_UINT32_IN_DESC)
  {
    uint32_index = property_name_p->u.uint32_number;

    new_prop_name_p = ecma_copy_or_ref_ecma_string (property_name_p);
  }
  else
  {
    ecma_number_t index = ecma_string_to_number (property_name_p);
    uint32_index = ecma_number_to_uint32 (index);

    ecma_string_t *to_str_p = ecma_new_ecma_string_from_uint32 (uint32_index);

    bool are_equal = ecma_compare_ecma_strings (to_str_p, property_name_p);

    if (!are_equal)
    {
      ecma_deref_ecma_string (to_str_p);

      return NULL;
    }
    else
    {
      new_prop_name_p = to_str_p;
    }
  }

  // 4.
  ecma_property_t* prim_value_prop_p = ecma_get_internal_property (obj_p,
                                                                   ECMA_INTERNAL_PROPERTY_PRIMITIVE_STRING_VALUE);
  ecma_string_t *prim_value_str_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                               prim_value_prop_p->u.internal_property.value);

  // 6.
  ecma_length_t length = ecma_string_get_length (prim_value_str_p);

  ecma_property_t *new_prop_p;

  if (uint32_index >= (uint32_t) length)
  {
    // 7.
    new_prop_p = NULL;
  }
  else
  {
    // 8.
    ecma_char_t c = ecma_string_get_char_at_pos (prim_value_str_p, uint32_index);

    // 9.
    ecma_string_t *new_prop_str_value_p = ecma_new_ecma_string_from_code_unit (c);

    new_prop_p = ecma_create_named_data_property (obj_p,
                                                  new_prop_name_p,
                                                  false, true, false);

    ecma_set_named_data_property_value (new_prop_p,
                                        ecma_make_string_value (new_prop_str_value_p));
  }

  ecma_deref_ecma_string (new_prop_name_p);

  return new_prop_p;
} /* ecma_op_string_object_get_own_property */

/**
 * List names of a String object's lazy instantiated properties
 *
 * See also:
 *          ecma_op_string_object_get_own_property
 *
 * @return string values collection
 */
void
ecma_op_string_list_lazy_property_names (ecma_object_t *obj_p, /**< a String object */
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
                                                                                           *   collection
                                                                                                        */
{
  JERRY_ASSERT (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_STRING);

  ecma_collection_header_t *for_enumerable_p = main_collection_p;

  ecma_collection_header_t *for_non_enumerable_p = separate_enumerable ? main_collection_p : non_enum_collection_p;
  (void) for_non_enumerable_p;

  ecma_property_t* prim_value_prop_p = ecma_get_internal_property (obj_p,
                                                                   ECMA_INTERNAL_PROPERTY_PRIMITIVE_STRING_VALUE);
  ecma_string_t *prim_value_str_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                               prim_value_prop_p->u.internal_property.value);

  ecma_length_t length = ecma_string_get_length (prim_value_str_p);

  for (ecma_length_t i = 0; i < length; i++)
  {
    ecma_string_t *name_p = ecma_new_ecma_string_from_uint32 (i);

    /* the properties are enumerable (ECMA-262 v5, 15.5.5.2.9) */
    ecma_append_to_values_collection (for_enumerable_p, ecma_make_string_value (name_p), true);

    ecma_deref_ecma_string (name_p);
  }
} /* ecma_op_string_list_lazy_property_names */

/**
 * @}
 * @}
 */
