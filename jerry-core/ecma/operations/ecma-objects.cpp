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

#include "ecma-array-object.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-globals.h"
#include "ecma-function-object.h"
#include "ecma-lcache.h"
#include "ecma-string-object.h"
#include "ecma-objects-arguments.h"
#include "ecma-objects-general.h"
#include "ecma-objects.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaobjectsinternalops ECMA objects' operations
 * @{
 */

/**
 * Assert that specified object type value is valid
 */
static void
ecma_assert_object_type_is_valid (ecma_object_type_t type) /**< object's implementation-defined type */
{
  JERRY_ASSERT (type == ECMA_OBJECT_TYPE_GENERAL
                || type == ECMA_OBJECT_TYPE_ARRAY
                || type == ECMA_OBJECT_TYPE_FUNCTION
                || type == ECMA_OBJECT_TYPE_BOUND_FUNCTION
                || type == ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION
                || type == ECMA_OBJECT_TYPE_STRING
                || type == ECMA_OBJECT_TYPE_ARGUMENTS
                || type == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);
} /* ecma_assert_object_type_is_valid */

/**
 * [[Get]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_object_get (ecma_object_t *obj_p, /**< the object */
                    ecma_string_t *property_name_p) /**< property name */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  ecma_assert_object_type_is_valid (type);

  switch (type)
  {
    case ECMA_OBJECT_TYPE_GENERAL:
    case ECMA_OBJECT_TYPE_ARRAY:
    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    case ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION:
    case ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION:
    case ECMA_OBJECT_TYPE_STRING:
    {
      return ecma_op_general_object_get (obj_p, property_name_p);
    }

    case ECMA_OBJECT_TYPE_ARGUMENTS:
    {
      return ecma_op_arguments_object_get (obj_p, property_name_p);
    }
  }

  JERRY_ASSERT (false);

  return ecma_make_empty_completion_value ();
} /* ecma_op_object_get */

/**
 * Long path for ecma_op_object_get_own_property
 *
 * @return pointer to a property - if it exists,
 *         NULL (i.e. ecma-undefined) - otherwise.
 */
static __attr_noinline___ ecma_property_t*
ecma_op_object_get_own_property_longpath (ecma_object_t *obj_p, /**< the object */
                                          ecma_string_t *property_name_p) /**< property name */
{
  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  ecma_assert_object_type_is_valid (type);

  const bool is_builtin = ecma_get_object_is_builtin (obj_p);

  ecma_property_t *prop_p = NULL;

  switch (type)
  {
    case ECMA_OBJECT_TYPE_GENERAL:
    case ECMA_OBJECT_TYPE_ARRAY:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    case ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION:
    case ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION:
    {
      prop_p = ecma_op_general_object_get_own_property (obj_p, property_name_p);

      break;
    }

    case ECMA_OBJECT_TYPE_FUNCTION:
    {
      prop_p = ecma_op_function_object_get_own_property (obj_p, property_name_p);

      break;
    }

    case ECMA_OBJECT_TYPE_STRING:
    {
      prop_p = ecma_op_string_object_get_own_property (obj_p, property_name_p);

      break;
    }

    case ECMA_OBJECT_TYPE_ARGUMENTS:
    {
      prop_p = ecma_op_arguments_object_get_own_property (obj_p, property_name_p);

      break;
    }
  }

  if (unlikely (prop_p == NULL))
  {
    if (is_builtin)
    {
      prop_p = ecma_builtin_try_to_instantiate_property (obj_p, property_name_p);
    }
  }

  return prop_p;
} /* ecma_op_object_get_own_property_longpath */

/**
 * [[GetOwnProperty]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * @return pointer to a property - if it exists,
 *         NULL (i.e. ecma-undefined) - otherwise.
 */
ecma_property_t*
ecma_op_object_get_own_property (ecma_object_t *obj_p, /**< the object */
                                 ecma_string_t *property_name_p) /**< property name */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  ecma_property_t *prop_p = NULL;

  if (likely (ecma_lcache_lookup (obj_p, property_name_p, &prop_p)))
  {
    return prop_p;
  }
  else
  {
    return ecma_op_object_get_own_property_longpath (obj_p, property_name_p);
  }
} /* ecma_op_object_get_own_property */

/**
 * [[GetProperty]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * @return pointer to a property - if it exists,
 *         NULL (i.e. ecma-undefined) - otherwise.
 */
ecma_property_t*
ecma_op_object_get_property (ecma_object_t *obj_p, /**< the object */
                             ecma_string_t *property_name_p) /**< property name */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  ecma_assert_object_type_is_valid (type);

  /*
   * typedef ecma_property_t* (*get_property_ptr_t) (ecma_object_t *, ecma_string_t *);
   * static const get_property_ptr_t get_property [ECMA_OBJECT_TYPE__COUNT] =
   * {
   *   [ECMA_OBJECT_TYPE_GENERAL]           = &ecma_op_general_object_get_property,
   *   [ECMA_OBJECT_TYPE_ARRAY]             = &ecma_op_general_object_get_property,
   *   [ECMA_OBJECT_TYPE_FUNCTION]          = &ecma_op_general_object_get_property,
   *   [ECMA_OBJECT_TYPE_BOUND_FUNCTION]    = &ecma_op_general_object_get_property,
   *   [ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION] = &ecma_op_general_object_get_property,
   *   [ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION] = &ecma_op_general_object_get_property,
   *   [ECMA_OBJECT_TYPE_ARGUMENTS]         = &ecma_op_general_object_get_property,
   *   [ECMA_OBJECT_TYPE_STRING]            = &ecma_op_general_object_get_property
   * };
   *
   * return get_property[type] (obj_p, property_name_p);
   */

  return ecma_op_general_object_get_property (obj_p, property_name_p);
} /* ecma_op_object_get_property */

/**
 * [[Put]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_object_put (ecma_object_t *obj_p, /**< the object */
                    ecma_string_t *property_name_p, /**< property name */
                    ecma_value_t value, /**< ecma-value */
                    bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  ecma_assert_object_type_is_valid (type);

  /*
   * typedef ecma_property_t* (*put_ptr_t) (ecma_object_t *, ecma_string_t *);
   * static const put_ptr_t put [ECMA_OBJECT_TYPE__COUNT] =
   * {
   *   [ECMA_OBJECT_TYPE_GENERAL]           = &ecma_op_general_object_put,
   *   [ECMA_OBJECT_TYPE_ARRAY]             = &ecma_op_general_object_put,
   *   [ECMA_OBJECT_TYPE_FUNCTION]          = &ecma_op_general_object_put,
   *   [ECMA_OBJECT_TYPE_BOUND_FUNCTION]    = &ecma_op_general_object_put,
   *   [ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION] = &ecma_op_general_object_put,
   *   [ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION] = &ecma_op_general_object_put,
   *   [ECMA_OBJECT_TYPE_ARGUMENTS]         = &ecma_op_general_object_put,
   *   [ECMA_OBJECT_TYPE_STRING]            = &ecma_op_general_object_put
   * };
   *
   * return put[type] (obj_p, property_name_p);
   */

  return ecma_op_general_object_put (obj_p, property_name_p, value, is_throw);
} /* ecma_op_object_put */

/**
 * [[CanPut]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * @return true - if [[Put]] with the given property name can be performed;
 *         false - otherwise.
 */
bool
ecma_op_object_can_put (ecma_object_t *obj_p, /**< the object */
                        ecma_string_t *property_name_p) /**< property name */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  ecma_assert_object_type_is_valid (type);

  /*
   * typedef ecma_property_t* (*can_put_ptr_t) (ecma_object_t *, ecma_string_t *);
   * static const can_put_ptr_t can_put [ECMA_OBJECT_TYPE__COUNT] =
   * {
   *   [ECMA_OBJECT_TYPE_GENERAL]           = &ecma_op_general_object_can_put,
   *   [ECMA_OBJECT_TYPE_ARRAY]             = &ecma_op_general_object_can_put,
   *   [ECMA_OBJECT_TYPE_FUNCTION]          = &ecma_op_general_object_can_put,
   *   [ECMA_OBJECT_TYPE_BOUND_FUNCTION]    = &ecma_op_general_object_can_put,
   *   [ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION] = &ecma_op_general_object_can_put,
   *   [ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION] = &ecma_op_general_object_can_put,
   *   [ECMA_OBJECT_TYPE_ARGUMENTS]         = &ecma_op_general_object_can_put,
   *   [ECMA_OBJECT_TYPE_STRING]            = &ecma_op_general_object_can_put
   * };
   *
   * return can_put[type] (obj_p, property_name_p);
   */

  return ecma_op_general_object_can_put (obj_p, property_name_p);
} /* ecma_op_object_can_put */

/**
 * [[Delete]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_object_delete (ecma_object_t *obj_p, /**< the object */
                       ecma_string_t *property_name_p, /**< property name */
                       bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  ecma_assert_object_type_is_valid (type);

  switch (type)
  {
    case ECMA_OBJECT_TYPE_GENERAL:
    case ECMA_OBJECT_TYPE_ARRAY:
    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    case ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION:
    case ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION:
    case ECMA_OBJECT_TYPE_STRING:
    {
      return ecma_op_general_object_delete (obj_p,
                                            property_name_p,
                                            is_throw);
    }

    case ECMA_OBJECT_TYPE_ARGUMENTS:
    {
      return ecma_op_arguments_object_delete (obj_p,
                                              property_name_p,
                                              is_throw);
    }
  }

  JERRY_ASSERT (false);

  return ecma_make_empty_completion_value ();
} /* ecma_op_object_delete */

/**
 * [[DefaultValue]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_object_default_value (ecma_object_t *obj_p, /**< the object */
                              ecma_preferred_type_hint_t hint) /**< hint on preferred result type */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  ecma_assert_object_type_is_valid (type);

  /*
   * typedef ecma_property_t* (*default_value_ptr_t) (ecma_object_t *, ecma_string_t *);
   * static const default_value_ptr_t default_value [ECMA_OBJECT_TYPE__COUNT] =
   * {
   *   [ECMA_OBJECT_TYPE_GENERAL]           = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_ARRAY]             = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_FUNCTION]          = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_BOUND_FUNCTION]    = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION] = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION] = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_ARGUMENTS]         = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_STRING]            = &ecma_op_general_object_default_value
   * };
   *
   * return default_value[type] (obj_p, property_name_p);
   */

  return ecma_op_general_object_default_value (obj_p, hint);
} /* ecma_op_object_default_value */

/**
 * [[DefineOwnProperty]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_object_define_own_property (ecma_object_t *obj_p, /**< the object */
                                    ecma_string_t *property_name_p, /**< property name */
                                    const ecma_property_descriptor_t* property_desc_p, /**< property
                                                                                        *   descriptor */
                                    bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  ecma_assert_object_type_is_valid (type);

  switch (type)
  {
    case ECMA_OBJECT_TYPE_GENERAL:
    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    case ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION:
    case ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION:
    case ECMA_OBJECT_TYPE_STRING:
    {
      return ecma_op_general_object_define_own_property (obj_p,
                                                         property_name_p,
                                                         property_desc_p,
                                                         is_throw);
    }

    case ECMA_OBJECT_TYPE_ARRAY:
    {
      return ecma_op_array_object_define_own_property (obj_p,
                                                       property_name_p,
                                                       property_desc_p,
                                                       is_throw);
    }

    case ECMA_OBJECT_TYPE_ARGUMENTS:
    {
      return ecma_op_arguments_object_define_own_property (obj_p,
                                                           property_name_p,
                                                           property_desc_p,
                                                           is_throw);
    }
  }

  JERRY_ASSERT (false);

  return ecma_make_empty_completion_value ();
} /* ecma_op_object_define_own_property */

/**
 * [[HasInstance]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 9
 */
ecma_completion_value_t
ecma_op_object_has_instance (ecma_object_t *obj_p, /**< the object */
                             ecma_value_t value) /**< argument 'V' */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  ecma_assert_object_type_is_valid (type);

  switch (type)
  {
    case ECMA_OBJECT_TYPE_ARRAY:
    case ECMA_OBJECT_TYPE_GENERAL:
    case ECMA_OBJECT_TYPE_STRING:
    case ECMA_OBJECT_TYPE_ARGUMENTS:
    {
      return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }

    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    case ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION:
    case ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION:
    {
      return ecma_op_function_has_instance (obj_p, value);
    }
  }

  JERRY_UNREACHABLE ();
} /* ecma_op_object_has_instance */

/**
 * Object's isPrototypeOf operation
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.6; 3
 *
 * @return true if the target object is prototype of the base object
 *         false if the target object is not prototype of the base object
 */
bool
ecma_op_object_is_prototype_of (ecma_object_t *base_p, /** < base object */
                                ecma_object_t *target_p) /** < target object */
{
  do
  {
    target_p = ecma_get_object_prototype (target_p);
    if (target_p == NULL)
    {
      return false;
    }
    else if (target_p == base_p)
    {
      return true;
    }
  } while (true);
} /* ecma_op_object_is_prototype_of */

/**
 * Get collection of property names
 *
 * Order of names in the collection:
 *  - integer indices in ascending order
 *  - other indices in creation order (for built-ins - in the order the properties are listed in specification).
 *
 * Note:
 *      Implementation of the routine assumes that new properties are appended to beginning of corresponding object's
 *      property list, and the list is not reordered (in other words, properties are stored in order that is reversed
 *      to the properties' addition order).
 *
 * @return collection of strings - property names
 */
ecma_collection_header_t *
ecma_op_object_get_property_names (ecma_object_t *obj_p, /**< object */
                                   bool is_array_indices_only, /**< true - exclude properties with names
                                                                *          that are not indices */
                                   bool is_enumerable_only, /**< true - exclude non-enumerable properties */
                                   bool is_with_prototype_chain) /**< true - list properties from prototype chain,
                                                                  *   false - list only own properties */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));

  ecma_collection_header_t *ret_p = ecma_new_strings_collection (NULL, 0);
  ecma_collection_header_t *skipped_non_enumerable_p = ecma_new_strings_collection (NULL, 0);

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  ecma_assert_object_type_is_valid (type);

  const size_t bitmap_row_size = sizeof (uint32_t) * JERRY_BITSINBYTE;
  uint32_t names_hashes_bitmap[(1u << LIT_STRING_HASH_BITS) / bitmap_row_size];

  memset (names_hashes_bitmap, 0, sizeof (names_hashes_bitmap));

  for (ecma_object_t *prototype_chain_iter_p = obj_p;
       prototype_chain_iter_p != NULL;
       prototype_chain_iter_p = is_with_prototype_chain ? ecma_get_object_prototype (prototype_chain_iter_p)
                                                        : NULL)
  {
    ecma_length_t string_named_properties_count = 0;
    ecma_length_t array_index_named_properties_count = 0;

    ecma_collection_header_t *prop_names_p = ecma_new_strings_collection (NULL, 0);

    if (ecma_get_object_is_builtin (obj_p))
    {
      ecma_builtin_list_lazy_property_names (obj_p,
                                             is_enumerable_only,
                                             prop_names_p,
                                             skipped_non_enumerable_p);
    }
    else
    {
      switch (type)
      {
        case ECMA_OBJECT_TYPE_FUNCTION:
        {
          ecma_op_function_list_lazy_property_names (is_enumerable_only,
                                                     prop_names_p,
                                                     skipped_non_enumerable_p);
          break;
        }

        case ECMA_OBJECT_TYPE_STRING:
        {
          ecma_op_string_list_lazy_property_names (obj_p,
                                                   is_enumerable_only,
                                                   prop_names_p,
                                                   skipped_non_enumerable_p);
          break;
        }

        case ECMA_OBJECT_TYPE_ARRAY:
        case ECMA_OBJECT_TYPE_GENERAL:
        case ECMA_OBJECT_TYPE_ARGUMENTS:
        case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
        case ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION:
        case ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION:
        {
          break;
        }
      }
    }

    ecma_collection_iterator_t iter;
    ecma_collection_iterator_init (&iter, prop_names_p);

    uint32_t own_names_hashes_bitmap[(1u << LIT_STRING_HASH_BITS) / bitmap_row_size];
    memset (own_names_hashes_bitmap, 0, sizeof (own_names_hashes_bitmap));

    while (ecma_collection_iterator_next (&iter))
    {
      ecma_string_t *name_p = ecma_get_string_from_value (*iter.current_value_p);

      lit_string_hash_t hash = name_p->hash;
      uint32_t bitmap_row = hash / bitmap_row_size;
      uint32_t bitmap_column = hash % bitmap_row_size;

      if ((own_names_hashes_bitmap[bitmap_row] & (1u << bitmap_column)) == 0)
      {
        own_names_hashes_bitmap[bitmap_row] |= (1u << bitmap_column);
      }
    }

    for (ecma_property_t *prop_iter_p = ecma_get_property_list (prototype_chain_iter_p);
         prop_iter_p != NULL;
         prop_iter_p = ECMA_GET_POINTER (ecma_property_t, prop_iter_p->next_property_p))
    {
      if (prop_iter_p->type == ECMA_PROPERTY_NAMEDDATA
          || prop_iter_p->type == ECMA_PROPERTY_NAMEDACCESSOR)
      {
        ecma_string_t *name_p;

        if (prop_iter_p->type == ECMA_PROPERTY_NAMEDDATA)
        {
          name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t, prop_iter_p->u.named_data_property.name_p);
        }
        else
        {
          JERRY_ASSERT (prop_iter_p->type == ECMA_PROPERTY_NAMEDACCESSOR);

          name_p = ECMA_GET_NON_NULL_POINTER (ecma_string_t, prop_iter_p->u.named_accessor_property.name_p);
        }

        if (!(is_enumerable_only && !ecma_is_property_enumerable (prop_iter_p)))
        {
          lit_string_hash_t hash = name_p->hash;
          uint32_t bitmap_row = hash / bitmap_row_size;
          uint32_t bitmap_column = hash % bitmap_row_size;

          bool is_add = true;

          if ((own_names_hashes_bitmap[bitmap_row] & (1u << bitmap_column)) != 0)
          {
            ecma_collection_iterator_init (&iter, prop_names_p);

            while (ecma_collection_iterator_next (&iter))
            {
              ecma_string_t *name2_p = ecma_get_string_from_value (*iter.current_value_p);

              if (ecma_compare_ecma_strings (name_p, name2_p))
              {
                is_add = false;
                break;
              }
            }
          }

          if (is_add)
          {
            own_names_hashes_bitmap[bitmap_row] |= (1u << bitmap_column);

            ecma_append_to_values_collection (prop_names_p,
                                              ecma_make_string_value (name_p),
                                              true);
          }
        }
        else
        {
          JERRY_ASSERT (is_enumerable_only && !ecma_is_property_enumerable (prop_iter_p));

          ecma_append_to_values_collection (skipped_non_enumerable_p,
                                            ecma_make_string_value (name_p),
                                            true);

          lit_string_hash_t hash = name_p->hash;
          uint32_t bitmap_row = hash / bitmap_row_size;
          uint32_t bitmap_column = hash % bitmap_row_size;

          if ((names_hashes_bitmap[bitmap_row] & (1u << bitmap_column)) == 0)
          {
            names_hashes_bitmap[bitmap_row] |= (1u << bitmap_column);
          }
        }
      }
      else
      {
        JERRY_ASSERT (prop_iter_p->type == ECMA_PROPERTY_INTERNAL);
      }
    }

    ecma_collection_iterator_init (&iter, prop_names_p);
    while (ecma_collection_iterator_next (&iter))
    {
      ecma_string_t *name_p = ecma_get_string_from_value (*iter.current_value_p);

      uint32_t index;

      if (ecma_string_get_array_index (name_p, &index))
      {
        /* name_p is a valid array index */
        array_index_named_properties_count++;
      }
      else if (!is_array_indices_only)
      {
        string_named_properties_count++;
      }
    }

    /* Second pass: collecting properties names into arrays */
    MEM_DEFINE_LOCAL_ARRAY (names_p,
                            array_index_named_properties_count + string_named_properties_count,
                            ecma_string_t *);
    MEM_DEFINE_LOCAL_ARRAY (array_index_names_p, array_index_named_properties_count, uint32_t);

    uint32_t name_pos = array_index_named_properties_count + string_named_properties_count;
    uint32_t array_index_name_pos = 0;

    ecma_collection_iterator_init (&iter, prop_names_p);
    while (ecma_collection_iterator_next (&iter))
    {
      ecma_string_t *name_p = ecma_get_string_from_value (*iter.current_value_p);

      uint32_t index;

      if (ecma_string_get_array_index (name_p, &index))
      {
        JERRY_ASSERT (array_index_name_pos < array_index_named_properties_count);

        uint32_t insertion_pos = 0;
        while (insertion_pos < array_index_name_pos
               && index < array_index_names_p[insertion_pos])
        {
          insertion_pos++;
        }

        if (insertion_pos == array_index_name_pos)
        {
          array_index_names_p[array_index_name_pos++] = index;
        }
        else
        {
          JERRY_ASSERT (insertion_pos < array_index_name_pos);
          JERRY_ASSERT (index >= array_index_names_p[insertion_pos]);

          uint32_t move_pos = ++array_index_name_pos;

          while (move_pos != insertion_pos)
          {
            array_index_names_p[move_pos] = array_index_names_p[move_pos - 1u];

            move_pos--;
          }

          array_index_names_p[insertion_pos] = index;
        }
      }
      else if (!is_array_indices_only)
      {
        /*
         * Filling from end to begin, as list of object's properties is sorted
         * in order that is reverse to properties creation order
         */

        JERRY_ASSERT (name_pos > 0
                      && name_pos <= array_index_named_properties_count + string_named_properties_count);
        names_p[--name_pos] = ecma_copy_or_ref_ecma_string (name_p);
      }
    }

    for (uint32_t i = 0; i < array_index_named_properties_count; i++)
    {
      JERRY_ASSERT (name_pos > 0
                    && name_pos <= array_index_named_properties_count + string_named_properties_count);
      names_p[--name_pos] = ecma_new_ecma_string_from_uint32 (array_index_names_p[i]);
    }

    JERRY_ASSERT (name_pos == 0);

    MEM_FINALIZE_LOCAL_ARRAY (array_index_names_p);

    ecma_free_values_collection (prop_names_p, true);

    /* Third pass:
     *   embedding own property names of current object of prototype chain to aggregate property names collection */
    for (uint32_t i = 0;
         i < array_index_named_properties_count + string_named_properties_count;
         i++)
    {
      bool is_append;

      ecma_string_t *name_p = names_p[i];

      lit_string_hash_t hash = name_p->hash;
      uint32_t bitmap_row = hash / bitmap_row_size;
      uint32_t bitmap_column = hash % bitmap_row_size;

      if ((names_hashes_bitmap[bitmap_row] & (1u << bitmap_column)) == 0)
      {
        /* no name with the hash is in constructed collection */
        is_append = true;

        names_hashes_bitmap[bitmap_row] |= (1u << bitmap_column);
      }
      else
      {
        /* name with same hash already occured */
        bool is_equal_found = false;

        ecma_collection_iterator_t iter;
        ecma_collection_iterator_init (&iter, ret_p);

        while (ecma_collection_iterator_next (&iter))
        {
          ecma_string_t *iter_name_p = ecma_get_string_from_value (*iter.current_value_p);

          if (ecma_compare_ecma_strings (name_p, iter_name_p))
          {
            is_equal_found = true;
          }
        }

        ecma_collection_iterator_init (&iter, skipped_non_enumerable_p);
        while (ecma_collection_iterator_next (&iter))
        {
          ecma_string_t *iter_name_p = ecma_get_string_from_value (*iter.current_value_p);

          if (ecma_compare_ecma_strings (name_p, iter_name_p))
          {
            is_equal_found = true;
          }
        }

        is_append = !is_equal_found;
      }

      if (is_append)
      {
        JERRY_ASSERT ((names_hashes_bitmap[bitmap_row] & (1u << bitmap_column)) != 0);

        ecma_append_to_values_collection (ret_p, ecma_make_string_value (names_p[i]), true);
      }

      ecma_deref_ecma_string (name_p);
    }

    MEM_FINALIZE_LOCAL_ARRAY (names_p);
  }

  ecma_free_values_collection (skipped_non_enumerable_p, true);

  return ret_p;
} /* ecma_op_object_get_property_names */

/**
 * Get [[Class]] string of specified object
 *
 * @return class name magic string
 */
lit_magic_string_id_t
ecma_object_get_class_name (ecma_object_t *obj_p) /**< object */
{
  ecma_object_type_t type = ecma_get_object_type (obj_p);

  switch (type)
  {
    case ECMA_OBJECT_TYPE_ARRAY:
    {
      return LIT_MAGIC_STRING_ARRAY_UL;
    }
    case ECMA_OBJECT_TYPE_STRING:
    {
      return LIT_MAGIC_STRING_STRING_UL;
    }
    case ECMA_OBJECT_TYPE_ARGUMENTS:
    {
      return LIT_MAGIC_STRING_ARGUMENTS_UL;
    }
    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    case ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION:
    case ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION:
    {
      return LIT_MAGIC_STRING_FUNCTION_UL;
    }
    default:
    {
      JERRY_ASSERT (type == ECMA_OBJECT_TYPE_GENERAL);

      if (ecma_get_object_is_builtin (obj_p))
      {
        ecma_property_t *built_in_id_prop_p = ecma_get_internal_property (obj_p,
                                                                          ECMA_INTERNAL_PROPERTY_BUILT_IN_ID);
        ecma_builtin_id_t builtin_id = (ecma_builtin_id_t) built_in_id_prop_p->u.internal_property.value;

        switch (builtin_id)
        {
          case ECMA_BUILTIN_ID_OBJECT_PROTOTYPE:
          {
            return LIT_MAGIC_STRING_OBJECT_UL;
          }
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_STRING_BUILTIN
          case ECMA_BUILTIN_ID_STRING_PROTOTYPE:
          {
            return LIT_MAGIC_STRING_STRING_UL;
          }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_STRING_BUILTIN */
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_BOOLEAN_BUILTIN
          case ECMA_BUILTIN_ID_BOOLEAN_PROTOTYPE:
          {
            return LIT_MAGIC_STRING_BOOLEAN_UL;
          }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_BOOLEAN_BUILTIN */
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_NUMBER_BUILTIN
          case ECMA_BUILTIN_ID_NUMBER_PROTOTYPE:
          {
            return LIT_MAGIC_STRING_NUMBER_UL;
          }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_NUMBER_BUILTIN */
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_MATH_BUILTIN
          case ECMA_BUILTIN_ID_MATH:
          {
            return LIT_MAGIC_STRING_MATH_UL;
          }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_MATH_BUILTIN */
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_JSON_BUILTIN
          case ECMA_BUILTIN_ID_JSON:
          {
            return LIT_MAGIC_STRING_JSON_U;
          }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_JSON_BUILTIN */
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ERROR_BUILTINS
          case ECMA_BUILTIN_ID_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_EVAL_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_RANGE_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_REFERENCE_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_SYNTAX_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_TYPE_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_URI_ERROR_PROTOTYPE:
          {
            return LIT_MAGIC_STRING_ERROR_UL;
          }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ERROR_BUILTINS */
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN
          case ECMA_BUILTIN_ID_DATE_PROTOTYPE:
          {
            return LIT_MAGIC_STRING_DATE_UL;
          }
#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_DATE_BUILTIN */
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN
          case ECMA_BUILTIN_ID_REGEXP_PROTOTYPE:
          {
            return LIT_MAGIC_STRING_REGEXP_UL;
          }
#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */
          default:
          {
            JERRY_ASSERT (ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_GLOBAL));

            return LIT_MAGIC_STRING_OBJECT_UL;
          }
        }
      }
      else
      {
        ecma_property_t *class_name_prop_p = ecma_find_internal_property (obj_p,
                                                                          ECMA_INTERNAL_PROPERTY_CLASS);

        if (class_name_prop_p == NULL)
        {
          return LIT_MAGIC_STRING_OBJECT_UL;
        }
        else
        {
          return (lit_magic_string_id_t) class_name_prop_p->u.internal_property.value;
        }
      }
    }
  }
} /* ecma_object_get_class_name */

/**
 * @}
 * @}
 */
