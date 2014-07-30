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

#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-magic-strings.h"
#include "ecma-objects-properties.h"

/** \addtogroup ecma ---TODO---
 * @{
 * 
 * \addtogroup ecmafunctionobject ECMA Function object related routines
 * @{
 */

/**
 * IsCallable operation.
 *
 * See also: ECMA-262 v5, 9.11
 *
 * @return true, if value is callable object;
 *         false - otherwise.
 */
bool
ecma_op_is_callable( ecma_value_t value) /**< ecma-value */
{
  if ( value.value_type != ECMA_TYPE_OBJECT )
    {
      return false;
    }

  ecma_object_t *obj_p = ecma_get_pointer( value.value);

  JERRY_ASSERT( obj_p != NULL );
  JERRY_ASSERT( !obj_p->is_lexical_environment );

  return true;
} /* ecma_op_is_callable */

/**
 * Function object creation operation.
 *
 * See also: ECMA-262 v5, 13.2
 *
 * @return pointer to newly created Function object
 */
ecma_object_t*
ecma_op_create_function_object( const ecma_char_t* formal_parameter_list_p[], /**< formal parameters list */
                                size_t formal_parameters_number, /**< formal parameters list's length */
                                ecma_object_t *scope_p, /**< function's scope */
                                bool is_strict, /**< 'strict' flag */
                                interp_bytecode_idx first_opcode_idx) /**< index of first opcode of function's body */
{
  // 1., 4., 13.
  FIXME( Setup prototype of Function object to built-in Function prototype object (15.3.3.1) );

  ecma_object_t *f = ecma_create_object( NULL, true, ECMA_OBJECT_TYPE_FUNCTION);

  // 2., 6., 7., 8.
  /*
   * We don't setup [[Get]], [[Call]], [[Construct]], [[HasInstance]] for each function object.
   * Instead we set the object's type to ECMA_OBJECT_TYPE_FUNCTION
   * that defines which version of the routine should be used on demand.
   */

  // 3.
  ecma_property_t *class_prop_p = ecma_create_internal_property( f, ECMA_INTERNAL_PROPERTY_CLASS);
  class_prop_p->u.internal_property.value = ECMA_OBJECT_CLASS_FUNCTION;

  // 9.
  ecma_ref_object( scope_p);

  ecma_property_t *scope_prop_p = ecma_create_internal_property( f, ECMA_INTERNAL_PROPERTY_SCOPE);
  ecma_set_pointer( scope_prop_p->u.internal_property.value, scope_p);

  // 10., 11., 14., 15.
  if ( formal_parameters_number != 0 )
    {
      JERRY_UNIMPLEMENTED_REF_UNUSED_VARS( formal_parameter_list_p);
    }

  // 12.
  ecma_property_t *code_prop_p = ecma_create_internal_property( f, ECMA_INTERNAL_PROPERTY_CODE);
  code_prop_p->u.internal_property.value = first_opcode_idx;

  // 16.
  FIXME( Use 'new Object()' instead );
  ecma_object_t *proto_p = ecma_create_object( NULL, true, ECMA_OBJECT_TYPE_GENERAL);

  // 17.
  ecma_ref_object( f);

  ecma_property_descriptor_t prop_desc = (ecma_property_descriptor_t) {
      .is_value_defined = true,
      .value = ecma_make_object_value( f),

      .is_writable_defined = true,
      .writable = ECMA_PROPERTY_WRITABLE,

      .is_enumerable_defined = true,
      .enumerable = ECMA_PROPERTY_NOT_ENUMERABLE,

      .is_configurable_defined = true,
      .configurable = ECMA_PROPERTY_CONFIGURABLE,

      .is_get_defined = false,
      .get_p = NULL,

      .is_set_defined = false,
      .set_p = NULL
  };
  ecma_op_object_define_own_property( proto_p,
                                      ecma_get_magic_string( ECMA_MAGIC_STRING_CONSTRUCTOR),
                                      prop_desc,
                                      false);

  // 18.
  ecma_ref_object( proto_p);

  prop_desc.value = ecma_make_object_value( proto_p);
  prop_desc.configurable = ECMA_PROPERTY_NOT_CONFIGURABLE;
  ecma_op_object_define_own_property( f,
                                      ecma_get_magic_string( ECMA_MAGIC_STRING_PROTOTYPE),
                                      prop_desc,
                                      false);

  ecma_deref_object( proto_p);

  // 19.
  if ( is_strict )
    {
      ecma_object_t *thrower_p = ecma_op_get_throw_type_error();

      prop_desc = (ecma_property_descriptor_t) {
          .is_value_defined = false,
          .value = ecma_make_simple_value( ECMA_SIMPLE_VALUE_UNDEFINED),

          .is_writable_defined = false,
          .writable = ECMA_PROPERTY_NOT_WRITABLE,

          .is_enumerable_defined = true,
          .enumerable = ECMA_PROPERTY_NOT_ENUMERABLE,

          .is_configurable_defined = true,
          .configurable = ECMA_PROPERTY_NOT_CONFIGURABLE,

          .is_get_defined = true,
          .get_p = thrower_p,

          .is_set_defined = true,
          .set_p = thrower_p
      };

      ecma_op_object_define_own_property( f,
                                          ecma_get_magic_string( ECMA_MAGIC_STRING_CALLER),
                                          prop_desc,
                                          false);

      ecma_op_object_define_own_property( f,
                                          ecma_get_magic_string( ECMA_MAGIC_STRING_ARGUMENTS),
                                          prop_desc,
                                          false);

      ecma_deref_object( thrower_p);
    }

  return f;
} /* ecma_op_create_function_object */

/**
 * Get [[ThrowTypeError]] Function Object
 *
 * @return pointer to unique [[ThrowTypeError]] Function Object
 */
ecma_object_t*
ecma_op_get_throw_type_error( void)
{
  TODO( Create [[ThrowTypeError]] during engine initialization and return it from here );

  JERRY_UNIMPLEMENTED();
} /* ecma_op_get_throw_type_error */

/**
 * @}
 * @}
 */
