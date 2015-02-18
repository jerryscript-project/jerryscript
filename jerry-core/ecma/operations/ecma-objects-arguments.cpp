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

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmafunctionobject ECMA Function object related routines
 * @{
 */

#include "ecma-alloc.h"
#include "ecma-builtins.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-objects.h"
#include "ecma-objects-arguments.h"
#include "ecma-objects-general.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"

/**
 * Arguments object creation operation.
 *
 * See also: ECMA-262 v5, 10.6
 *
 * @return pointer to newly created Arguments object
 */
ecma_object_t*
ecma_create_arguments_object (ecma_object_t *func_obj_p, /**< callee function */
                              ecma_object_t *lex_env_p, /**< lexical environment the Arguments
                                                             object is created for */
                              ecma_collection_iterator_t *formal_params_iter_p, /**< formal parameters
                                                                                     collection iterator */
                              const ecma_value_t *arguments_list_p, /**< list of arguments */
                              ecma_length_t arguments_list_length, /**< length of arguments' list */
                              bool is_strict) /**< flag indicating whether strict mode is enabled */
{
  // 1.
  ecma_number_t *len_p = ecma_alloc_number ();
  *len_p = ecma_uint32_to_number (arguments_list_length);

  // 2., 3., 6.
  ecma_object_t *prototype_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);

  ecma_object_t *obj_p = ecma_create_object (prototype_p, true, ECMA_OBJECT_TYPE_GENERAL);

  ecma_deref_object (prototype_p);

  // 4.
  ecma_property_t *class_prop_p = ecma_create_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_CLASS);
  class_prop_p->u.internal_property.value = ECMA_MAGIC_STRING_ARGUMENTS_UL;

  // 7.
  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();
  {
    prop_desc.is_value_defined = true;
    prop_desc.value = ecma_make_number_value (len_p);

    prop_desc.is_writable_defined = true;
    prop_desc.is_writable = true;

    prop_desc.is_enumerable_defined = true;
    prop_desc.is_enumerable = false;

    prop_desc.is_configurable_defined = true;
    prop_desc.is_configurable = true;
  }
  ecma_string_t *length_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);
  ecma_completion_value_t completion = ecma_op_object_define_own_property (obj_p,
                                                                           length_magic_string_p,
                                                                           &prop_desc,
                                                                           false);
  JERRY_ASSERT (ecma_is_completion_value_normal_true (completion));
  ecma_deref_ecma_string (length_magic_string_p);

  ecma_dealloc_number (len_p);

  // 11.a, 11.b
  for (ecma_length_t indx = 0;
       indx < arguments_list_length;
       indx++)
  {
    prop_desc = ecma_make_empty_property_descriptor ();
    {
      prop_desc.is_value_defined = true;
      prop_desc.value = arguments_list_p[indx];

      prop_desc.is_writable_defined = true;
      prop_desc.is_writable = true;

      prop_desc.is_enumerable_defined = true;
      prop_desc.is_enumerable = true;

      prop_desc.is_configurable_defined = true;
      prop_desc.is_configurable = true;
    }

    ecma_string_t *indx_string_p = ecma_new_ecma_string_from_number (ecma_uint32_to_number (indx));

    completion = ecma_op_object_define_own_property (obj_p,
                                                     indx_string_p,
                                                     &prop_desc,
                                                     false);
    JERRY_ASSERT (ecma_is_completion_value_normal_true (completion));

    ecma_deref_ecma_string (indx_string_p);
  }

  const ecma_length_t formal_params_number = formal_params_iter_p->header_p->unit_number;
  if (!is_strict
      && arguments_list_length > 0
      && formal_params_number > 0)
  {
    // 8.
    ecma_object_t *map_p = ecma_op_create_object_object_noarg ();

    // 11.c
    MEM_DEFINE_LOCAL_ARRAY (formal_params, formal_params_number, ecma_string_t *);

    JERRY_ASSERT (formal_params_iter_p->current_value_p == NULL);
    uint32_t param_index;
    for (param_index = 0;
         ecma_collection_iterator_next (formal_params_iter_p);
         param_index++)
    {
      JERRY_ASSERT (formal_params_iter_p->current_value_p != NULL);
      JERRY_ASSERT (param_index < formal_params_number);

      JERRY_ASSERT (ecma_is_value_string (*formal_params_iter_p->current_value_p));
      formal_params[param_index] = ecma_get_string_from_value (*formal_params_iter_p->current_value_p);
    }
    JERRY_ASSERT (param_index == formal_params_number);

    for (int32_t indx = formal_params_number - 1;
         indx >= 0;
         indx--)
    {
      // i.
      ecma_string_t *name_p = formal_params[indx];
      bool is_first_occurence = true;

      // ii.
      for (int32_t indx2 = indx + 1;
           indx2 < formal_params_number;
           indx2++)
      {
        if (ecma_compare_ecma_strings (name_p, formal_params[indx2]))
        {
          is_first_occurence = false;
        }
      }

      if (is_first_occurence)
      {
        ecma_string_t *indx_string_p = ecma_new_ecma_string_from_number (ecma_uint32_to_number ((uint32_t) indx));

        prop_desc = ecma_make_empty_property_descriptor ();
        {
          prop_desc.is_value_defined = true;
          prop_desc.value = ecma_make_string_value (name_p);
        }

        completion = ecma_op_object_define_own_property (map_p,
                                                         indx_string_p,
                                                         &prop_desc,
                                                         false);
        JERRY_ASSERT (ecma_is_completion_value_normal_true (completion));

        ecma_deref_ecma_string (indx_string_p);
      }
    }

    MEM_FINALIZE_LOCAL_ARRAY (formal_params);

    // 12.
    ecma_set_object_type (obj_p, ECMA_OBJECT_TYPE_ARGUMENTS);

    ecma_property_t *parameters_map_prop_p = ecma_create_internal_property (obj_p,
                                                                            ECMA_INTERNAL_PROPERTY_PARAMETERS_MAP);
    ECMA_SET_POINTER(parameters_map_prop_p->u.internal_property.value, map_p);

    ecma_property_t *scope_prop_p = ecma_create_internal_property (map_p,
                                                                   ECMA_INTERNAL_PROPERTY_SCOPE);
    ECMA_SET_POINTER(scope_prop_p->u.internal_property.value, lex_env_p);

    ecma_deref_object (map_p);
  }

  // 13.
  if (!is_strict)
  {
    prop_desc = ecma_make_empty_property_descriptor ();
    {
      prop_desc.is_value_defined = true;
      prop_desc.value = ecma_make_object_value (func_obj_p);

      prop_desc.is_writable_defined = true;
      prop_desc.is_writable = true;

      prop_desc.is_enumerable_defined = true;
      prop_desc.is_enumerable = false;

      prop_desc.is_configurable_defined = true;
      prop_desc.is_configurable = true;
    }

    ecma_string_t *callee_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_CALLEE);

    completion = ecma_op_object_define_own_property (obj_p,
                                                     callee_magic_string_p,
                                                     &prop_desc,
                                                     false);
    JERRY_ASSERT (ecma_is_completion_value_normal_true (completion));

    ecma_deref_ecma_string (callee_magic_string_p);
  }
  else
  {
    ecma_object_t *thrower_p = ecma_builtin_get (ECMA_BUILTIN_ID_TYPE_ERROR_THROWER);

    // 14.
    prop_desc = ecma_make_empty_property_descriptor ();
    {
      prop_desc.is_get_defined = true;
      prop_desc.get_p = thrower_p;

      prop_desc.is_set_defined = true;
      prop_desc.set_p = thrower_p;

      prop_desc.is_enumerable_defined = true;
      prop_desc.is_enumerable = false;

      prop_desc.is_configurable_defined = true;
      prop_desc.is_configurable = false;
    }

    ecma_string_t *callee_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_CALLEE);
    ecma_string_t *caller_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_CALLER);

    completion = ecma_op_object_define_own_property (obj_p,
                                                     callee_magic_string_p,
                                                     &prop_desc,
                                                     false);
    JERRY_ASSERT (ecma_is_completion_value_normal_true (completion));

    ecma_deref_ecma_string (callee_magic_string_p);
    ecma_deref_ecma_string (caller_magic_string_p);
    ecma_deref_object (thrower_p);
  }

  return obj_p;
} /* ecma_create_arguments_object */

/**
 * Get value of function's argument mapped to index of Arguments object.
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
ecma_arguments_get_mapped_arg_value (ecma_object_t *map_p, /**< [[ParametersMap]] object */
                                     ecma_property_t *arg_name_prop_p) /**< property of [[ParametersMap]]
                                                                            corresponding to index and value
                                                                            equal to mapped argument's name */
{
  ecma_property_t *scope_prop_p = ecma_get_internal_property (map_p, ECMA_INTERNAL_PROPERTY_SCOPE);
  ecma_object_t *lex_env_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t,
                                                        scope_prop_p->u.internal_property.value);
  JERRY_ASSERT(lex_env_p != NULL
               && ecma_is_lexical_environment (lex_env_p));

  ecma_value_t arg_name_prop_value = ecma_get_named_data_property_value (arg_name_prop_p);

  ecma_string_t *arg_name_p = ecma_get_string_from_value (arg_name_prop_value);

  ecma_completion_value_t completion = ecma_op_get_binding_value (lex_env_p,
                                                                  arg_name_p,
                                                                  true);
  JERRY_ASSERT (ecma_is_completion_value_normal (completion));

  return completion;
} /* ecma_arguments_get_mapped_arg_value */

/**
 * [[Get]] ecma Arguments object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 10.6
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_arguments_object_get (ecma_object_t *obj_p, /**< the object */
                              ecma_string_t *property_name_p) /**< property name */
{
  // 1.
  ecma_property_t *map_prop_p = ecma_get_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_PARAMETERS_MAP);
  ecma_object_t *map_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t,
                                                    map_prop_p->u.internal_property.value);

  // 2.
  ecma_property_t *mapped_prop_p = ecma_op_object_get_own_property (map_p, property_name_p);

  // 3.
  if (mapped_prop_p == NULL)
  {
    /* We don't check for 'caller' (item 3.b) here, because the 'caller' property is defined
       as non-configurable and it's get/set are set to [[ThrowTypeError]] object */

    return ecma_op_general_object_get (obj_p, property_name_p);
  }
  else
  {
    // 4.
    return ecma_arguments_get_mapped_arg_value (map_p, mapped_prop_p);
  }
} /* ecma_op_arguments_object_get */

/**
 * [[GetOwnProperty]] ecma Arguments object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 10.6
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_property_t*
ecma_op_arguments_object_get_own_property (ecma_object_t *obj_p, /**< the object */
                                           ecma_string_t *property_name_p) /**< property name */
{
  // 1.
  ecma_property_t *desc_p = ecma_op_general_object_get_own_property (obj_p, property_name_p);

  // 2.
  if (desc_p == NULL)
  {
    return desc_p;
  }

  // 3.
  ecma_property_t *map_prop_p = ecma_get_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_PARAMETERS_MAP);
  ecma_object_t *map_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t,
                                                    map_prop_p->u.internal_property.value);

  // 4.
  ecma_property_t *mapped_prop_p = ecma_op_object_get_own_property (map_p, property_name_p);

  // 5.
  if (mapped_prop_p != NULL)
  {
    // a.
    ecma_completion_value_t completion = ecma_arguments_get_mapped_arg_value (map_p, mapped_prop_p);

    ecma_named_data_property_assign_value (obj_p, desc_p, ecma_get_completion_value_value (completion));

    ecma_free_completion_value (completion);
  }

  // 6.
  return desc_p;
} /* ecma_op_arguments_object_get_own_property */

/**
 * [[DefineOwnProperty]] ecma Arguments object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 10.6
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_arguments_object_define_own_property (ecma_object_t *obj_p, /**< the object */
                                              ecma_string_t *property_name_p, /**< property name */
                                              const ecma_property_descriptor_t* property_desc_p, /**< property
                                                                                                  *   descriptor */
                                              bool is_throw) /**< flag that controls failure handling */
{
  // 1.
  ecma_property_t *map_prop_p = ecma_get_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_PARAMETERS_MAP);
  ecma_object_t *map_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t,
                                                    map_prop_p->u.internal_property.value);

  // 2.
  ecma_property_t *mapped_prop_p = ecma_op_object_get_own_property (map_p, property_name_p);

  // 3.
  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (defined,
                  ecma_op_general_object_define_own_property (obj_p,
                                                              property_name_p,
                                                              property_desc_p,
                                                              is_throw),
                  ret_value);

  // 5.
  if (mapped_prop_p != NULL)
  {
    // a.
    if (property_desc_p->is_get_defined
        || property_desc_p->is_set_defined)
    {
      ecma_completion_value_t completion = ecma_op_object_delete (map_p, property_name_p, false);

      JERRY_ASSERT (ecma_is_completion_value_normal_true (completion));

      // 6.
      ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
    }
    else
    {
      // b.

      ecma_completion_value_t completion = ecma_make_empty_completion_value ();

      // i.
      if (property_desc_p->is_value_defined)
      {
        completion = ecma_op_object_put (map_p,
                                         property_name_p,
                                         property_desc_p->value,
                                         is_throw);
      }

      if (unlikely (ecma_is_completion_value_throw (completion)))
      {
        ret_value = completion;
      }
      else
      {
        // ii.
        if (property_desc_p->is_writable_defined
            && !property_desc_p->is_writable)
        {
          completion = ecma_op_object_delete (map_p,
                                              property_name_p,
                                              false);

          JERRY_ASSERT (ecma_is_completion_value_normal_true (completion));
        }

        // 6.
        ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
      }
    }
  }
  else
  {
    ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
  }

  ECMA_FINALIZE (defined);

  return ret_value;
} /* ecma_op_arguments_object_define_own_property */

/**
 * [[Delete]] ecma Arguments object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 10.6
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_arguments_object_delete (ecma_object_t *obj_p, /**< the object */
                                 ecma_string_t *property_name_p, /**< property name */
                                 bool is_throw) /**< flag that controls failure handling */
{
  // 1.
  ecma_property_t *map_prop_p = ecma_get_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_PARAMETERS_MAP);
  ecma_object_t *map_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t,
                                                    map_prop_p->u.internal_property.value);

  // 2.
  ecma_property_t *mapped_prop_p = ecma_op_object_get_own_property (map_p, property_name_p);

  // 3.
  ecma_completion_value_t ret_value;

  ECMA_TRY_CATCH (delete_in_args_ret,
                  ecma_op_general_object_delete (obj_p,
                                                 property_name_p,
                                                 is_throw),
                  ret_value);

  if (ecma_is_value_true (delete_in_args_ret))
  {
    if (mapped_prop_p != NULL)
    {
      ecma_completion_value_t delete_in_map_completion = ecma_op_object_delete (map_p,
                                                                                property_name_p,
                                                                                false);
      JERRY_ASSERT (ecma_is_completion_value_normal_true (delete_in_map_completion));
    }

    ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (delete_in_args_ret));

    ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE);
  }

  ECMA_FINALIZE (delete_in_args_ret);

  return ret_value;
} /* ecma_op_arguments_object_delete */

/**
 * @}
 * @}
 */
