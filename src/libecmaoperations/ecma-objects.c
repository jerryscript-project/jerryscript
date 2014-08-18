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

#include "ecma-globals.h"
#include "ecma-objects-general.h"
#include "ecma-objects.h"

/** \addtogroup ecma ---TODO---
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
  JERRY_ASSERT(obj_p != NULL && !obj_p->is_lexical_environment);
  JERRY_ASSERT(property_name_p != NULL);

  const ecma_object_type_t type = obj_p->u.object.type;

  switch (type)
  {
    case ECMA_OBJECT_TYPE_GENERAL:
    case ECMA_OBJECT_TYPE_ARRAY:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    case ECMA_OBJECT_TYPE_STRING:
    {
      return ecma_op_general_object_get (obj_p, property_name_p);
    }

    case ECMA_OBJECT_TYPE_ARGUMENTS:
    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_HOST:
    {
      JERRY_UNIMPLEMENTED();
    }
  }

  JERRY_UNREACHABLE();
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
  JERRY_ASSERT(obj_p != NULL && !obj_p->is_lexical_environment);
  JERRY_ASSERT(property_name_p != NULL);

  const ecma_object_type_t type = obj_p->u.object.type;

  switch (type)
  {
    case ECMA_OBJECT_TYPE_GENERAL:
    case ECMA_OBJECT_TYPE_ARRAY:
    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    {
      return ecma_op_general_object_get_own_property (obj_p, property_name_p);
    }

    case ECMA_OBJECT_TYPE_ARGUMENTS:
    case ECMA_OBJECT_TYPE_STRING:
    case ECMA_OBJECT_TYPE_HOST:
    {
      JERRY_UNIMPLEMENTED();
    }
  }

  JERRY_UNREACHABLE();
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
  JERRY_ASSERT(obj_p != NULL && !obj_p->is_lexical_environment);
  JERRY_ASSERT(property_name_p != NULL);

  const ecma_object_type_t type = obj_p->u.object.type;

  switch (type)
  {
    case ECMA_OBJECT_TYPE_GENERAL:
    case ECMA_OBJECT_TYPE_ARGUMENTS:
    case ECMA_OBJECT_TYPE_ARRAY:
    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    case ECMA_OBJECT_TYPE_STRING:
    {
      return ecma_op_general_object_get_property (obj_p, property_name_p);
    }

    case ECMA_OBJECT_TYPE_HOST:
    {
      JERRY_UNIMPLEMENTED();
    }
  }

  JERRY_UNREACHABLE();
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
  JERRY_ASSERT(obj_p != NULL && !obj_p->is_lexical_environment);
  JERRY_ASSERT(property_name_p != NULL);

  const ecma_object_type_t type = obj_p->u.object.type;

  switch (type)
  {
    case ECMA_OBJECT_TYPE_GENERAL:
    case ECMA_OBJECT_TYPE_ARGUMENTS:
    case ECMA_OBJECT_TYPE_ARRAY:
    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    case ECMA_OBJECT_TYPE_STRING:
    {
      return ecma_op_general_object_put (obj_p, property_name_p, value, is_throw);
    }

    case ECMA_OBJECT_TYPE_HOST:
    {
      JERRY_UNIMPLEMENTED();
    }
  }

  JERRY_UNREACHABLE();
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
  JERRY_ASSERT(obj_p != NULL && !obj_p->is_lexical_environment);
  JERRY_ASSERT(property_name_p != NULL);

  const ecma_object_type_t type = obj_p->u.object.type;

  switch (type)
  {
    case ECMA_OBJECT_TYPE_GENERAL:
    case ECMA_OBJECT_TYPE_ARGUMENTS:
    case ECMA_OBJECT_TYPE_ARRAY:
    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    case ECMA_OBJECT_TYPE_STRING:
    {
      return ecma_op_general_object_can_put (obj_p, property_name_p);
    }

    case ECMA_OBJECT_TYPE_HOST:
    {
      JERRY_UNIMPLEMENTED();
    }
  }

  JERRY_UNREACHABLE();
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
  JERRY_ASSERT(obj_p != NULL && !obj_p->is_lexical_environment);
  JERRY_ASSERT(property_name_p != NULL);

  const ecma_object_type_t type = obj_p->u.object.type;

  switch (type)
  {
    case ECMA_OBJECT_TYPE_GENERAL:
    case ECMA_OBJECT_TYPE_ARGUMENTS:
    case ECMA_OBJECT_TYPE_ARRAY:
    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    case ECMA_OBJECT_TYPE_STRING:
    {
      return ecma_op_general_object_has_property (obj_p, property_name_p);
    }

    case ECMA_OBJECT_TYPE_HOST:
    {
      JERRY_UNIMPLEMENTED();
    }
  }

  JERRY_UNREACHABLE();
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
  JERRY_ASSERT(obj_p != NULL && !obj_p->is_lexical_environment);
  JERRY_ASSERT(property_name_p != NULL);

  const ecma_object_type_t type = obj_p->u.object.type;

  switch (type)
  {
    case ECMA_OBJECT_TYPE_GENERAL:
    case ECMA_OBJECT_TYPE_ARRAY:
    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    case ECMA_OBJECT_TYPE_STRING:
    {
      return ecma_op_general_object_delete (obj_p, property_name_p, is_throw);
    }

    case ECMA_OBJECT_TYPE_ARGUMENTS:
    case ECMA_OBJECT_TYPE_HOST:
    {
      JERRY_UNIMPLEMENTED();
    }
  }

  JERRY_UNREACHABLE();
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
  JERRY_ASSERT(obj_p != NULL && !obj_p->is_lexical_environment);

  const ecma_object_type_t type = obj_p->u.object.type;

  switch (type)
  {
    case ECMA_OBJECT_TYPE_GENERAL:
    case ECMA_OBJECT_TYPE_ARGUMENTS:
    case ECMA_OBJECT_TYPE_ARRAY:
    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    case ECMA_OBJECT_TYPE_STRING:
    {
      return ecma_op_general_object_default_value (obj_p, hint);
    }

    case ECMA_OBJECT_TYPE_HOST:
    {
      JERRY_UNIMPLEMENTED();
    }
  }

  JERRY_UNREACHABLE();
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
  JERRY_ASSERT(obj_p != NULL && !obj_p->is_lexical_environment);
  JERRY_ASSERT(property_name_p != NULL);

  const ecma_object_type_t type = obj_p->u.object.type;

  switch (type)
  {
    case ECMA_OBJECT_TYPE_GENERAL:
    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    case ECMA_OBJECT_TYPE_STRING:
    {
      return ecma_op_general_object_define_own_property (obj_p, property_name_p, property_desc, is_throw);
    }

    case ECMA_OBJECT_TYPE_ARGUMENTS:
    case ECMA_OBJECT_TYPE_ARRAY:
    case ECMA_OBJECT_TYPE_HOST:
    {
      JERRY_UNIMPLEMENTED();
    }
  }

  JERRY_UNREACHABLE();
} /* ecma_op_object_define_own_property */

/**
 * @}
 * @}
 */
