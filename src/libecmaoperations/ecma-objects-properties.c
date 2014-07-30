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

#include "ecma-exceptions.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects-properties.h"

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
ecma_op_object_get( ecma_object_t *obj_p, /**< the object */
                    const ecma_char_t *property_name_p) /**< property name */
{
  JERRY_ASSERT( obj_p != NULL && !obj_p->is_lexical_environment );
  JERRY_ASSERT( property_name_p != NULL );

  /*
   * ECMA-262 v5, 8.12.3
   *
   * Common implementation of operation for objects other than Host, Function or Arguments objects.
   */

  // 1.
  const ecma_property_t* prop_p = ecma_op_object_get_property( obj_p, property_name_p);

  // 2.
  if ( prop_p == NULL )
    {
      return ecma_make_simple_completion_value( ECMA_SIMPLE_VALUE_UNDEFINED);
    }

  // 3.
  if ( prop_p->type == ECMA_PROPERTY_NAMEDDATA )
    {
      return ecma_make_completion_value( ECMA_COMPLETION_TYPE_NORMAL,
                                         ecma_copy_value( prop_p->u.named_data_property.value),
                                         ECMA_TARGET_ID_RESERVED);
    }
  else
    {
      // 4.
      ecma_object_t *getter = ecma_get_pointer( prop_p->u.named_accessor_property.get_p);

      // 5.
      if ( getter == NULL )
        {
          return ecma_make_simple_completion_value( ECMA_SIMPLE_VALUE_UNDEFINED);
        }
      else
        {
          /*
           * Return result of O.[[Call]];
           */
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
ecma_op_object_get_own_property( ecma_object_t *obj_p, /**< the object */
                                 const ecma_char_t *property_name_p) /**< property name */
{
  JERRY_ASSERT( obj_p != NULL && !obj_p->is_lexical_environment );
  JERRY_ASSERT( property_name_p != NULL );

  /*
   * ECMA-262 v5, 8.12.2
   *
   * Common implementation of operation for objects other than Host, Function or Arguments objects.
   */

  return ecma_find_named_property( obj_p, property_name_p);
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
ecma_op_object_get_property( ecma_object_t *obj_p, /**< the object */
                             const ecma_char_t *property_name_p) /**< property name */
{
  JERRY_ASSERT( obj_p != NULL && !obj_p->is_lexical_environment );
  JERRY_ASSERT( property_name_p != NULL );

  /*
   * ECMA-262 v5, 8.12.2
   *
   * Common implementation of operation for objects other than Host, Function or Arguments objects.
   */

  // 1.
  ecma_property_t *prop_p = ecma_op_object_get_own_property( obj_p, property_name_p);

  // 2.
  if ( prop_p != NULL )
    {
      return prop_p;
    }

  // 3.
  ecma_object_t *prototype_p = ecma_get_pointer( obj_p->u.object.prototype_object_p);

  // 4., 5.
  if ( prototype_p != NULL )
    {
      return ecma_op_object_get_property( prototype_p, property_name_p);
    }
  else
    {
      return NULL;
    }
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
ecma_op_object_put( ecma_object_t *obj_p, /**< the object */
                    const ecma_char_t *property_name_p, /**< property name */
                    ecma_value_t value, /**< ecma-value */
                    bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT( obj_p != NULL && !obj_p->is_lexical_environment );
  JERRY_ASSERT( property_name_p != NULL );

  // 1.
  if ( !ecma_op_object_can_put( obj_p, property_name_p) )
    {
      if ( is_throw )
        {
          // a.
          return ecma_make_throw_value( ecma_new_standard_error( ECMA_ERROR_TYPE));
        }
      else
        {
          // b.
          return ecma_make_simple_completion_value( ECMA_SIMPLE_VALUE_FALSE);
        }
    }

  // 2.
  ecma_property_t *own_desc_p = ecma_op_object_get_own_property( obj_p, property_name_p);

  // 3.
  if ( own_desc_p->type == ECMA_PROPERTY_NAMEDDATA )
    {
      // a.
      ecma_property_descriptor_t value_desc = (ecma_property_descriptor_t) {
          .is_value_defined = true,
          .value = value,

          .is_get_defined = false,
          .get_p = NULL,

          .is_set_defined = false,
          .set_p = NULL,

          .is_writable_defined = false,
          .writable = ECMA_PROPERTY_NOT_WRITABLE,

          .is_enumerable_defined = false,
          .enumerable = ECMA_PROPERTY_NOT_ENUMERABLE,

          .is_configurable_defined = false,
          .configurable = ECMA_PROPERTY_NOT_CONFIGURABLE,
      };

      // b., c.
      return ecma_op_object_define_own_property( obj_p,
                                                 property_name_p,
                                                 value_desc,
                                                 is_throw);
    }

  // 4.
  ecma_property_t *desc_p = ecma_op_object_get_property( obj_p, property_name_p);

  // 5.
  if ( desc_p->type == ECMA_PROPERTY_NAMEDACCESSOR )
    {
      // a.
      ecma_object_t *setter_p = ecma_get_pointer( desc_p->u.named_accessor_property.set_p);

      JERRY_ASSERT( setter_p != NULL );

      // b.
      /*
       * setter_p->[[Call]]( obj_p, value);
       */
      JERRY_UNIMPLEMENTED();
    }
  else
    {
      // 6.

      // a.
      ecma_property_descriptor_t new_desc = (ecma_property_descriptor_t) {
          .is_value_defined = true,
          .value = value,

          .is_get_defined = false,
          .get_p = NULL,

          .is_set_defined = false,
          .set_p = NULL,

          .is_writable_defined = true,
          .writable = ECMA_PROPERTY_WRITABLE,

          .is_enumerable_defined = true,
          .enumerable = ECMA_PROPERTY_ENUMERABLE,

          .is_configurable_defined = true,
          .configurable = ECMA_PROPERTY_CONFIGURABLE
      };
      
      // b.
      return ecma_op_object_define_own_property( obj_p,
                                                 property_name_p,
                                                 new_desc,
                                                 is_throw);
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
ecma_op_object_can_put( ecma_object_t *obj_p, /**< the object */
                        const ecma_char_t *property_name_p) /**< property name */
{
  JERRY_ASSERT( obj_p != NULL && !obj_p->is_lexical_environment );
  JERRY_ASSERT( property_name_p != NULL );

  /*
   * ECMA-262 v5, 8.12.2
   *
   * Common implementation of operation for objects other than Host, Function or Arguments objects.
   */

  // 1.
  ecma_property_t *prop_p = ecma_op_object_get_own_property( obj_p, property_name_p);

  // 2.
  if ( prop_p != NULL )
    {
      // a.
      if ( prop_p->type == ECMA_PROPERTY_NAMEDACCESSOR )
        {
          ecma_object_t *setter_p = ecma_get_pointer( prop_p->u.named_accessor_property.set_p);

          // i.
          if ( setter_p == NULL )
            {
              return false;
            }

          // ii.
          return true;
        }
      else
        {
          // b.

          JERRY_ASSERT( prop_p->type == ECMA_PROPERTY_NAMEDDATA );

          return ( prop_p->u.named_data_property.writable == ECMA_PROPERTY_WRITABLE );
        }
    }

  // 3.
  ecma_object_t *proto_p = ecma_get_pointer( obj_p->u.object.prototype_object_p);

  // 4.
  if ( proto_p == NULL )
    {
      return obj_p->u.object.extensible;
    }

  // 5.
  ecma_property_t *inherited_p = ecma_op_object_get_property( proto_p, property_name_p);

  // 6.
  if ( inherited_p == NULL )
    {
      return obj_p->u.object.extensible;
    }

  // 7.
  if ( inherited_p->type == ECMA_PROPERTY_NAMEDACCESSOR )
    {
      ecma_object_t *setter_p = ecma_get_pointer( inherited_p->u.named_accessor_property.set_p);

      // a.
      if ( setter_p == NULL )
        {
          return false;
        }

      // b.
      return true;
    }
  else
    {
      // 8.
      JERRY_ASSERT( inherited_p->type == ECMA_PROPERTY_NAMEDDATA );

      // a.
      if ( !obj_p->u.object.extensible )
        {
          return false;
        }
      else
        {
          // b.
          return ( inherited_p->u.named_data_property.writable == ECMA_PROPERTY_WRITABLE );
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
ecma_op_object_has_property( ecma_object_t *obj_p, /**< the object */
                             const ecma_char_t *property_name_p) /**< property name */
{
  JERRY_ASSERT( obj_p != NULL && !obj_p->is_lexical_environment );
  JERRY_ASSERT( property_name_p != NULL );

  /*
   * ECMA-262 v5, 8.12.2
   *
   * Common implementation of operation for objects other than Host, Function or Arguments objects.
   */

  ecma_property_t *desc_p = ecma_op_object_get_property( obj_p, property_name_p);

  return ( desc_p != NULL );
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
ecma_op_object_delete( ecma_object_t *obj_p, /**< the object */
                       const ecma_char_t *property_name_p, /**< property name */
                       bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT( obj_p != NULL && !obj_p->is_lexical_environment );
  JERRY_ASSERT( property_name_p != NULL );

  /*
   * ECMA-262 v5, 8.12.2
   *
   * Common implementation of operation for objects other than Host, Function or Arguments objects.
   */

  // 1.
  ecma_property_t *desc_p = ecma_op_object_get_own_property( obj_p, property_name_p);

  // 2.
  if ( desc_p == NULL )
    {
      return ecma_make_simple_completion_value( ECMA_SIMPLE_VALUE_TRUE);
    }

  // 3.
  bool is_configurable;
  if ( desc_p->type == ECMA_PROPERTY_NAMEDACCESSOR )
    {
      is_configurable = desc_p->u.named_accessor_property.configurable;
    }
  else
    {
      JERRY_ASSERT( desc_p->type == ECMA_PROPERTY_NAMEDDATA );

      is_configurable = desc_p->u.named_data_property.configurable;
    }

  if ( is_configurable )
    {
      // a.
      ecma_delete_property( obj_p, desc_p);

      // b.
      return ecma_make_simple_completion_value( ECMA_SIMPLE_VALUE_TRUE);
    }
  else if ( is_throw )
    {
      // 4.
      return ecma_make_throw_value( ecma_new_standard_error( ECMA_ERROR_TYPE));
    }
  else
    {
      // 5.
      return ecma_make_simple_completion_value( ECMA_SIMPLE_VALUE_FALSE);
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
ecma_op_object_default_value( ecma_object_t *obj_p, /**< the object */
                              ecma_preferred_type_hint_t hint) /**< hint on preferred result type */
{
  JERRY_ASSERT( obj_p != NULL && !obj_p->is_lexical_environment );

  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS( obj_p, hint);
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
ecma_op_object_define_own_property( ecma_object_t *obj_p, /**< the object */
                                    const ecma_char_t *property_name_p, /**< property name */
                                    ecma_property_descriptor_t property_desc, /**< property descriptor */
                                    bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT( obj_p != NULL && !obj_p->is_lexical_environment );
  JERRY_ASSERT( property_name_p != NULL );

  const bool is_property_desc_generic_descriptor = ( !property_desc.is_value_defined
                                                     && !property_desc.is_writable_defined
                                                     && !property_desc.is_get_defined
                                                     && !property_desc.is_set_defined );
  const bool is_property_desc_data_descriptor = ( property_desc.is_value_defined
                                                  || property_desc.is_writable_defined );
  const bool is_property_desc_accessor_descriptor = ( property_desc.is_get_defined
                                                      || property_desc.is_set_defined );

  // 1.
  ecma_property_t *current_p = ecma_op_object_get_own_property( obj_p, property_name_p);

  // 2.
  bool extensible = obj_p->u.object.extensible;

  if ( current_p == NULL )
    {
      // 3.
      if ( !extensible )
        {
          // Reject
          if ( is_throw )
            {
              return ecma_make_throw_value( ecma_new_standard_error( ECMA_ERROR_TYPE));
            }
          else
            {
              return ecma_make_simple_completion_value( ECMA_SIMPLE_VALUE_FALSE);
            }
        }
      else
        {
          // 4.

          // a.
          if ( is_property_desc_generic_descriptor
               || is_property_desc_data_descriptor )
            {
              ecma_create_named_data_property( obj_p,
                                               property_name_p,
                                               property_desc.writable,
                                               property_desc.enumerable,
                                               property_desc.configurable);
            }
          else
            {
              // b.
              JERRY_ASSERT( is_property_desc_accessor_descriptor );

              ecma_create_named_accessor_property( obj_p,
                                                   property_name_p,
                                                   property_desc.get_p,
                                                   property_desc.set_p,
                                                   property_desc.enumerable,
                                                   property_desc.configurable);

            }

          return ecma_make_simple_completion_value( ECMA_SIMPLE_VALUE_TRUE);
        }
    }

  // 5.
  if ( is_property_desc_generic_descriptor
       && !property_desc.is_enumerable_defined
       && !property_desc.is_configurable_defined )
    {
      return ecma_make_simple_completion_value( ECMA_SIMPLE_VALUE_TRUE);
    }

  // 6.
  const bool is_current_data_descriptor = ( current_p->type == ECMA_PROPERTY_NAMEDDATA );
  const bool is_current_accessor_descriptor = ( current_p->type == ECMA_PROPERTY_NAMEDACCESSOR );

  JERRY_ASSERT( is_current_data_descriptor || is_current_accessor_descriptor );

  bool is_every_field_in_desc_also_occurs_in_current_desc_with_same_value = true;
  if ( property_desc.is_value_defined )
    {
      if ( !is_current_data_descriptor
           || !ecma_op_same_value( property_desc.value,
                                   current_p->u.named_data_property.value) )
        {
          is_every_field_in_desc_also_occurs_in_current_desc_with_same_value = false;
        }
    }

  if ( property_desc.is_writable_defined )
    {
      if ( !is_current_data_descriptor
           || property_desc.writable != current_p->u.named_data_property.writable )
        {
          is_every_field_in_desc_also_occurs_in_current_desc_with_same_value = false;
        }
    }

  if ( property_desc.is_get_defined )
    {
      if ( !is_current_accessor_descriptor
           || property_desc.get_p != ecma_get_pointer( current_p->u.named_accessor_property.get_p) )
        {
          is_every_field_in_desc_also_occurs_in_current_desc_with_same_value = false;
        }
    }

  if ( property_desc.is_set_defined )
    {
      if ( !is_current_accessor_descriptor
           || property_desc.set_p != ecma_get_pointer( current_p->u.named_accessor_property.set_p) )
        {
          is_every_field_in_desc_also_occurs_in_current_desc_with_same_value = false;
        }
    }

  if ( property_desc.is_enumerable_defined )
    {
      if ( ( is_current_data_descriptor
             && property_desc.enumerable != current_p->u.named_data_property.enumerable )
           || ( is_current_accessor_descriptor
                && property_desc.configurable != current_p->u.named_accessor_property.enumerable ) )
        {
          is_every_field_in_desc_also_occurs_in_current_desc_with_same_value = false;
        }
    }

  if ( property_desc.is_configurable_defined )
    {
      if ( ( is_current_data_descriptor
            && property_desc.configurable != current_p->u.named_data_property.configurable )
          || ( is_current_accessor_descriptor
              && property_desc.configurable != current_p->u.named_accessor_property.configurable ) )
        {
          is_every_field_in_desc_also_occurs_in_current_desc_with_same_value = false;
        }
    }

  if ( is_every_field_in_desc_also_occurs_in_current_desc_with_same_value )
    {
      return ecma_make_simple_completion_value( ECMA_SIMPLE_VALUE_TRUE);
    }

  // 7., ...
  JERRY_UNIMPLEMENTED();
} /* ecma_op_object_define_own_property */

/**
 * @}
 * @}
 */
