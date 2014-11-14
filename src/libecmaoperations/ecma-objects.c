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

#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-globals.h"
#include "ecma-array-object.h"
#include "ecma-function-object.h"
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
  JERRY_ASSERT(obj_p != NULL
               && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT(property_name_p != NULL);

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  JERRY_ASSERT (type < ECMA_OBJECT_TYPE__COUNT);

  typedef ecma_completion_value_t (*get_ptr_t) (ecma_object_t *, ecma_string_t *);
  static const get_ptr_t get [ECMA_OBJECT_TYPE__COUNT] =
  {
    [ECMA_OBJECT_TYPE_GENERAL]           = &ecma_op_general_object_get,
    [ECMA_OBJECT_TYPE_ARRAY]             = &ecma_op_general_object_get,
    [ECMA_OBJECT_TYPE_FUNCTION]          = &ecma_op_general_object_get,
    [ECMA_OBJECT_TYPE_BOUND_FUNCTION]    = &ecma_op_general_object_get,
    [ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION] = &ecma_op_general_object_get,
    [ECMA_OBJECT_TYPE_STRING]            = &ecma_op_general_object_get,
    [ECMA_OBJECT_TYPE_ARGUMENTS]         = &ecma_op_arguments_object_get
  };

  return get[type] (obj_p, property_name_p);
} /* ecma_op_object_get */

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
  JERRY_ASSERT(obj_p != NULL
               && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT(property_name_p != NULL);

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  JERRY_ASSERT (type < ECMA_OBJECT_TYPE__COUNT);

  const bool is_builtin = ecma_get_object_is_builtin (obj_p);

  ecma_property_t *prop_p = NULL;
  typedef ecma_property_t* (*get_own_property_ptr_t) (ecma_object_t *, ecma_string_t *);
  static const get_own_property_ptr_t get_own_property [ECMA_OBJECT_TYPE__COUNT] =
  {
    [ECMA_OBJECT_TYPE_GENERAL]           = &ecma_op_general_object_get_own_property,
    [ECMA_OBJECT_TYPE_ARRAY]             = &ecma_op_general_object_get_own_property,
    [ECMA_OBJECT_TYPE_FUNCTION]          = &ecma_op_general_object_get_own_property,
    [ECMA_OBJECT_TYPE_BOUND_FUNCTION]    = &ecma_op_general_object_get_own_property,
    [ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION] = &ecma_op_general_object_get_own_property,
    [ECMA_OBJECT_TYPE_ARGUMENTS]         = &ecma_op_arguments_object_get_own_property,
    [ECMA_OBJECT_TYPE_STRING]            = &ecma_op_string_object_get_own_property
  };

  prop_p = get_own_property[type] (obj_p, property_name_p);

  if (unlikely (prop_p == NULL))
  {
    ecma_magic_string_id_t magic_string_id __unused;

    if (is_builtin
        && type != ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION
        && ecma_is_string_magic (property_name_p, &magic_string_id))
    {
      prop_p = ecma_builtin_try_to_instantiate_property (obj_p, property_name_p);
    }
  }

  return prop_p;
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
  JERRY_ASSERT(obj_p != NULL
               && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT(property_name_p != NULL);

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  JERRY_ASSERT (type < ECMA_OBJECT_TYPE__COUNT);

  /*
   * typedef ecma_property_t* (*get_property_ptr_t) (ecma_object_t *, ecma_string_t *);
   * static const get_property_ptr_t get_property [ECMA_OBJECT_TYPE__COUNT] =
   * {
   *   [ECMA_OBJECT_TYPE_GENERAL]           = &ecma_op_general_object_get_property,
   *   [ECMA_OBJECT_TYPE_ARRAY]             = &ecma_op_general_object_get_property,
   *   [ECMA_OBJECT_TYPE_FUNCTION]          = &ecma_op_general_object_get_property,
   *   [ECMA_OBJECT_TYPE_BOUND_FUNCTION]    = &ecma_op_general_object_get_property,
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
  JERRY_ASSERT(obj_p != NULL
               && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT(property_name_p != NULL);

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  JERRY_ASSERT (type < ECMA_OBJECT_TYPE__COUNT);

  /*
   * typedef ecma_property_t* (*put_ptr_t) (ecma_object_t *, ecma_string_t *);
   * static const put_ptr_t put [ECMA_OBJECT_TYPE__COUNT] =
   * {
   *   [ECMA_OBJECT_TYPE_GENERAL]           = &ecma_op_general_object_put,
   *   [ECMA_OBJECT_TYPE_ARRAY]             = &ecma_op_general_object_put,
   *   [ECMA_OBJECT_TYPE_FUNCTION]          = &ecma_op_general_object_put,
   *   [ECMA_OBJECT_TYPE_BOUND_FUNCTION]    = &ecma_op_general_object_put,
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
  JERRY_ASSERT(obj_p != NULL
               && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT(property_name_p != NULL);

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  JERRY_ASSERT (type < ECMA_OBJECT_TYPE__COUNT);

  /*
   * typedef ecma_property_t* (*can_put_ptr_t) (ecma_object_t *, ecma_string_t *);
   * static const can_put_ptr_t can_put [ECMA_OBJECT_TYPE__COUNT] =
   * {
   *   [ECMA_OBJECT_TYPE_GENERAL]           = &ecma_op_general_object_can_put,
   *   [ECMA_OBJECT_TYPE_ARRAY]             = &ecma_op_general_object_can_put,
   *   [ECMA_OBJECT_TYPE_FUNCTION]          = &ecma_op_general_object_can_put,
   *   [ECMA_OBJECT_TYPE_BOUND_FUNCTION]    = &ecma_op_general_object_can_put,
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
 * [[HasProperty]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * @return true - if the object already has a property with the given property name;
 *         false - otherwise.
 */
bool
ecma_op_object_has_property (ecma_object_t *obj_p, /**< the object */
                             ecma_string_t *property_name_p) /**< property name */
{
  JERRY_ASSERT(obj_p != NULL
               && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT(property_name_p != NULL);

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  JERRY_ASSERT (type < ECMA_OBJECT_TYPE__COUNT);

  /*
   * typedef ecma_property_t* (*has_property_ptr_t) (ecma_object_t *, ecma_string_t *);
   * static const has_property_ptr_t has_property [ECMA_OBJECT_TYPE__COUNT] =
   * {
   *   [ECMA_OBJECT_TYPE_GENERAL]           = &ecma_op_general_object_has_property,
   *   [ECMA_OBJECT_TYPE_ARRAY]             = &ecma_op_general_object_has_property,
   *   [ECMA_OBJECT_TYPE_FUNCTION]          = &ecma_op_general_object_has_property,
   *   [ECMA_OBJECT_TYPE_BOUND_FUNCTION]    = &ecma_op_general_object_has_property,
   *   [ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION] = &ecma_op_general_object_has_property,
   *   [ECMA_OBJECT_TYPE_ARGUMENTS]         = &ecma_op_general_object_has_property,
   *   [ECMA_OBJECT_TYPE_STRING]            = &ecma_op_general_object_has_property
   * };
   *
   * return has_property[type] (obj_p, property_name_p);
   */

  return ecma_op_general_object_has_property (obj_p, property_name_p);
} /* ecma_op_object_has_property */

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
  JERRY_ASSERT(obj_p != NULL
               && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT(property_name_p != NULL);

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  JERRY_ASSERT (type < ECMA_OBJECT_TYPE__COUNT);

  typedef ecma_completion_value_t (*delete_ptr_t) (ecma_object_t *, ecma_string_t *, bool);
  static const delete_ptr_t delete [ECMA_OBJECT_TYPE__COUNT] =
  {
    [ECMA_OBJECT_TYPE_GENERAL]           = &ecma_op_general_object_delete,
    [ECMA_OBJECT_TYPE_ARRAY]             = &ecma_op_general_object_delete,
    [ECMA_OBJECT_TYPE_FUNCTION]          = &ecma_op_general_object_delete,
    [ECMA_OBJECT_TYPE_BOUND_FUNCTION]    = &ecma_op_general_object_delete,
    [ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION] = &ecma_op_general_object_delete,
    [ECMA_OBJECT_TYPE_STRING]            = &ecma_op_general_object_delete,
    [ECMA_OBJECT_TYPE_ARGUMENTS]         = &ecma_op_arguments_object_delete
  };

  return delete[type] (obj_p, property_name_p, is_throw);
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
  JERRY_ASSERT(obj_p != NULL
               && !ecma_is_lexical_environment (obj_p));

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  JERRY_ASSERT (type < ECMA_OBJECT_TYPE__COUNT);

  /*
   * typedef ecma_property_t* (*default_value_ptr_t) (ecma_object_t *, ecma_string_t *);
   * static const default_value_ptr_t default_value [ECMA_OBJECT_TYPE__COUNT] =
   * {
   *   [ECMA_OBJECT_TYPE_GENERAL]           = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_ARRAY]             = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_FUNCTION]          = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_BOUND_FUNCTION]    = &ecma_op_general_object_default_value,
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
                                    ecma_property_descriptor_t property_desc, /**< property descriptor */
                                    bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT(obj_p != NULL
               && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT(property_name_p != NULL);

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  JERRY_ASSERT (type < ECMA_OBJECT_TYPE__COUNT);

  typedef ecma_completion_value_t (*define_own_property_ptr_t) (ecma_object_t *,
                                                                ecma_string_t *,
                                                                ecma_property_descriptor_t,
                                                                bool);
  static const define_own_property_ptr_t define_own_property [ECMA_OBJECT_TYPE__COUNT] =
  {
    [ECMA_OBJECT_TYPE_GENERAL]           = &ecma_op_general_object_define_own_property,
    [ECMA_OBJECT_TYPE_FUNCTION]          = &ecma_op_general_object_define_own_property,
    [ECMA_OBJECT_TYPE_BOUND_FUNCTION]    = &ecma_op_general_object_define_own_property,
    [ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION] = &ecma_op_general_object_define_own_property,
    [ECMA_OBJECT_TYPE_STRING]            = &ecma_op_general_object_define_own_property,
    [ECMA_OBJECT_TYPE_ARRAY]             = &ecma_op_array_object_define_own_property,
    [ECMA_OBJECT_TYPE_ARGUMENTS]         = &ecma_op_arguments_object_define_own_property
  };

  return define_own_property[type] (obj_p,
                                    property_name_p,
                                    property_desc,
                                    is_throw);
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
  JERRY_ASSERT(obj_p != NULL
               && !ecma_is_lexical_environment (obj_p));

  const ecma_object_type_t type = ecma_get_object_type (obj_p);

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
    case ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION:
    {
      return ecma_op_function_has_instance (obj_p, value);
    }

    case ECMA_OBJECT_TYPE__COUNT:
    {
      break;
    }
  }

  JERRY_UNREACHABLE();
} /* ecma_op_object_has_instance */

/**
 * @}
 * @}
 */
