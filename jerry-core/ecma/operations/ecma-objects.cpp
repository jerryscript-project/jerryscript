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
    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    case ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION:
    case ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION:
    {
      prop_p = ecma_op_general_object_get_own_property (obj_p, property_name_p);

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
    if (is_builtin
        && type != ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION)
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
 * Get [[Class]] string of specified object
 *
 * @return class name magic string
 */
ecma_magic_string_id_t
ecma_object_get_class_name (ecma_object_t *obj_p) /**< object */
{
  ecma_object_type_t type = ecma_get_object_type (obj_p);

  switch (type)
  {
    case ECMA_OBJECT_TYPE_ARRAY:
    {
      return ECMA_MAGIC_STRING_ARRAY_UL;
    }
    case ECMA_OBJECT_TYPE_STRING:
    {
      return ECMA_MAGIC_STRING_STRING_UL;
    }
    case ECMA_OBJECT_TYPE_ARGUMENTS:
    {
      return ECMA_MAGIC_STRING_ARGUMENTS_UL;
    }
    case ECMA_OBJECT_TYPE_FUNCTION:
    {
      ecma_magic_string_id_t class_name;

      if (ecma_get_object_is_builtin (obj_p))
      {
        ecma_property_t *built_in_id_prop_p = ecma_get_internal_property (obj_p,
                                                                          ECMA_INTERNAL_PROPERTY_BUILT_IN_ID);
        ecma_builtin_id_t builtin_id = (ecma_builtin_id_t) built_in_id_prop_p->u.internal_property.value;

        switch (builtin_id)
        {
          case ECMA_BUILTIN_ID_OBJECT:
          {
            class_name = ECMA_MAGIC_STRING_OBJECT_UL;
            break;
          }
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ARRAY_BUILTIN
          case ECMA_BUILTIN_ID_ARRAY:
          {
            class_name = ECMA_MAGIC_STRING_ARRAY_UL;
            break;
          }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ARRAY_BUILTIN */
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_STRING_BUILTIN
          case ECMA_BUILTIN_ID_STRING:
          {
            class_name = ECMA_MAGIC_STRING_STRING_UL;
            break;
          }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_STRING_BUILTIN */
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_BOOLEAN_BUILTIN
          case ECMA_BUILTIN_ID_BOOLEAN:
          {
            class_name = ECMA_MAGIC_STRING_BOOLEAN_UL;
            break;
          }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_BOOLEAN_BUILTIN */
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_NUMBER_BUILTIN
          case ECMA_BUILTIN_ID_NUMBER:
          {
            class_name = ECMA_MAGIC_STRING_NUMBER_UL;
            break;
          }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_NUMBER_BUILTIN */
          case ECMA_BUILTIN_ID_FUNCTION:
          {
            class_name = ECMA_MAGIC_STRING_FUNCTION_UL;
            break;
          }
#ifdef CONFIG_ECMA_COMPACT_PROFILE
          case ECMA_BUILTIN_ID_COMPACT_PROFILE_ERROR:
          {
            class_name = ECMA_MAGIC_STRING_COMPACT_PROFILE_ERROR_UL;
            break;
          }
#endif /* CONFIG_ECMA_COMPACT_PROFILE */
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ERROR_BUILTINS
          case ECMA_BUILTIN_ID_ERROR:
          case ECMA_BUILTIN_ID_EVAL_ERROR:
          case ECMA_BUILTIN_ID_RANGE_ERROR:
          case ECMA_BUILTIN_ID_REFERENCE_ERROR:
          case ECMA_BUILTIN_ID_SYNTAX_ERROR:
          case ECMA_BUILTIN_ID_TYPE_ERROR:
          case ECMA_BUILTIN_ID_URI_ERROR:
          {
            class_name = ECMA_MAGIC_STRING_ERROR_UL;
            break;
          }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ERROR_BUILTINS */
          default:
          {
            JERRY_ASSERT (builtin_id == ECMA_BUILTIN_ID_TYPE_ERROR_THROWER);

            class_name = ECMA_MAGIC_STRING_FUNCTION_UL;
            break;
          }
        }
      }
      else
      {
        class_name = ECMA_MAGIC_STRING_FUNCTION_UL;
      }

      return class_name;
    }
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    case ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION:
    case ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION:
    {
      return ECMA_MAGIC_STRING_FUNCTION_UL;
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
            return ECMA_MAGIC_STRING_OBJECT_UL;
          }
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_STRING_BUILTIN
          case ECMA_BUILTIN_ID_STRING_PROTOTYPE:
          {
            return ECMA_MAGIC_STRING_STRING_UL;
          }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_STRING_BUILTIN */
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_BOOLEAN_BUILTIN
          case ECMA_BUILTIN_ID_BOOLEAN_PROTOTYPE:
          {
            return ECMA_MAGIC_STRING_BOOLEAN_UL;
          }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_BOOLEAN_BUILTIN */
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_NUMBER_BUILTIN
          case ECMA_BUILTIN_ID_NUMBER_PROTOTYPE:
          {
            return ECMA_MAGIC_STRING_NUMBER_UL;
          }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_NUMBER_BUILTIN */
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_MATH_BUILTIN
          case ECMA_BUILTIN_ID_MATH:
          {
            return ECMA_MAGIC_STRING_MATH_UL;
          }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_MATH_BUILTIN */
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ERROR_BUILTINS
          case ECMA_BUILTIN_ID_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_EVAL_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_RANGE_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_REFERENCE_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_SYNTAX_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_TYPE_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_URI_ERROR_PROTOTYPE:
          {
            return ECMA_MAGIC_STRING_ERROR_UL;
          }
#endif /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_ERROR_BUILTINS */
#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN
          case ECMA_BUILTIN_ID_REGEXP_PROTOTYPE:
          {
            return ECMA_MAGIC_STRING_REGEXP_UL;
          }
#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */
          default:
          {
            JERRY_ASSERT (ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_GLOBAL));

            return ECMA_MAGIC_STRING_OBJECT_UL;
          }
        }
      }
      else
      {
        ecma_property_t *class_name_prop_p = ecma_find_internal_property (obj_p,
                                                                          ECMA_INTERNAL_PROPERTY_CLASS);

        if (class_name_prop_p == NULL)
        {
          return ECMA_MAGIC_STRING_OBJECT_UL;
        }
        else
        {
          return (ecma_magic_string_id_t) class_name_prop_p->u.internal_property.value;
        }
      }
    }
  }
} /* ecma_object_get_class_name */

/**
 * @}
 * @}
 */
