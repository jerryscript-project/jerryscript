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
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"
#include "globals.h"

/** \addtogroup ecma ---TODO---
 * @{
 *
 * \addtogroup ecmafunctionobject ECMA Function object related routines
 * @{
 */

/**
 * Pack 'is_strict' flag and opcode index to value
 * that can be stored in an [[Code]] internal property.
 *
 * @return packed value
 */
static uint32_t
ecma_pack_code_internal_property_value (bool is_strict, /**< is code strict? */
                                        opcode_counter_t opcode_idx) /**< index of first opcode */
{
  uint32_t value = opcode_idx;
  const uint32_t is_strict_bit_offset = sizeof (value) * JERRY_BITSINBYTE - 1;

  JERRY_ASSERT(((value) & (1u << is_strict_bit_offset)) == 0);

  if (is_strict)
  {
    value |= (1u << is_strict_bit_offset);
  }

  return value;
} /* ecma_pack_code_internal_property_value */

/**
 * Unpack 'is_strict' flag and opcode index from value
 * that can be stored in an [[Code]] internal property.
 *
 * @return opcode index
 */
static opcode_counter_t
ecma_unpack_code_internal_property_value (uint32_t value, /**< packed value */
                                          bool* out_is_strict_p) /**< out: is code strict? */
{
  JERRY_ASSERT(out_is_strict_p != NULL);

  const uint32_t is_strict_bit_offset = sizeof (value) * JERRY_BITSINBYTE - 1;

  bool is_strict = ((value & (1u << is_strict_bit_offset)) != 0);
  *out_is_strict_p = is_strict;

  opcode_counter_t opcode_idx = (opcode_counter_t) (value & ~(1u << is_strict_bit_offset));

  return opcode_idx;
} /* ecma_unpack_code_internal_property_value */

/**
 * IsCallable operation.
 *
 * See also: ECMA-262 v5, 9.11
 *
 * @return true, if value is callable object;
 *         false - otherwise.
 */
bool
ecma_op_is_callable (ecma_value_t value) /**< ecma-value */
{
  if (value.value_type != ECMA_TYPE_OBJECT)
  {
    return false;
  }

  ecma_object_t *obj_p = ECMA_GET_POINTER(value.value);

  JERRY_ASSERT(obj_p != NULL);
  JERRY_ASSERT(!obj_p->is_lexical_environment);

  return (obj_p->u.object.type == ECMA_OBJECT_TYPE_FUNCTION
          || obj_p->u.object.type == ECMA_OBJECT_TYPE_BOUND_FUNCTION);
} /* ecma_op_is_callable */

/**
 * Function object creation operation.
 *
 * See also: ECMA-262 v5, 13.2
 *
 * @return pointer to newly created Function object
 */
ecma_object_t*
ecma_op_create_function_object (const ecma_char_t* formal_parameter_list_p[], /**< formal parameters list */
                                size_t formal_parameters_number, /**< formal parameters list's length */
                                ecma_object_t *scope_p, /**< function's scope */
                                bool is_strict, /**< 'strict' flag */
                                opcode_counter_t first_opcode_idx) /**< index of first opcode of function's body */
{
  // 1., 4., 13.
  FIXME(Setup prototype of Function object to built-in Function prototype object (15.3.3.1));

  ecma_object_t *f = ecma_create_object (NULL, true, ECMA_OBJECT_TYPE_FUNCTION);

  // 2., 6., 7., 8.
  /*
   * We don't setup [[Get]], [[Call]], [[Construct]], [[HasInstance]] for each function object.
   * Instead we set the object's type to ECMA_OBJECT_TYPE_FUNCTION
   * that defines which version of the routine should be used on demand.
   */

  // 3.
  ecma_property_t *class_prop_p = ecma_create_internal_property (f, ECMA_INTERNAL_PROPERTY_CLASS);
  class_prop_p->u.internal_property.value = ECMA_OBJECT_CLASS_FUNCTION;

  // 9.
  ecma_property_t *scope_prop_p = ecma_create_internal_property (f, ECMA_INTERNAL_PROPERTY_SCOPE);
  ECMA_SET_POINTER(scope_prop_p->u.internal_property.value, scope_p);

  ecma_gc_update_may_ref_younger_object_flag_by_object (f, scope_p);

  // 10., 11., 14., 15.
  if (formal_parameters_number != 0)
  {
    JERRY_UNIMPLEMENTED_REF_UNUSED_VARS(formal_parameter_list_p);
  }

  // 12.
  ecma_property_t *code_prop_p = ecma_create_internal_property (f, ECMA_INTERNAL_PROPERTY_CODE);
  code_prop_p->u.internal_property.value = ecma_pack_code_internal_property_value (is_strict,
                                                                                   first_opcode_idx);

  // 16.
  FIXME(Use 'new Object ()' instead);
  ecma_object_t *proto_p = ecma_create_object (NULL, true, ECMA_OBJECT_TYPE_GENERAL);

  // 17.
  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();
  {
    prop_desc.is_value_defined = true;
    prop_desc.value = ecma_make_object_value (f);

    prop_desc.is_writable_defined = true;
    prop_desc.writable = ECMA_PROPERTY_WRITABLE;

    prop_desc.is_enumerable_defined = true;
    prop_desc.enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;

    prop_desc.is_configurable_defined = true;
    prop_desc.configurable = ECMA_PROPERTY_CONFIGURABLE;
  }

  ecma_op_object_define_own_property (proto_p,
                                      ecma_get_magic_string (ECMA_MAGIC_STRING_CONSTRUCTOR),
                                      prop_desc,
                                      false);

  // 18.
  prop_desc.value = ecma_make_object_value (proto_p);
  prop_desc.configurable = ECMA_PROPERTY_NOT_CONFIGURABLE;
  ecma_op_object_define_own_property (f,
                                      ecma_get_magic_string (ECMA_MAGIC_STRING_PROTOTYPE),
                                      prop_desc,
                                      false);

  ecma_deref_object (proto_p);

  // 19.
  if (is_strict)
  {
    ecma_object_t *thrower_p = ecma_op_get_throw_type_error ();

    prop_desc = ecma_make_empty_property_descriptor ();
    {
      prop_desc.is_enumerable_defined = true;
      prop_desc.enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;

      prop_desc.is_configurable_defined = true;
      prop_desc.configurable = ECMA_PROPERTY_NOT_CONFIGURABLE;

      prop_desc.is_get_defined = true;
      prop_desc.get_p = thrower_p;

      prop_desc.is_set_defined = true;
      prop_desc.set_p = thrower_p;
    }

    ecma_op_object_define_own_property (f,
                                        ecma_get_magic_string (ECMA_MAGIC_STRING_CALLER),
                                        prop_desc,
                                        false);

    ecma_op_object_define_own_property (f,
                                        ecma_get_magic_string (ECMA_MAGIC_STRING_ARGUMENTS),
                                        prop_desc,
                                        false);

    ecma_deref_object (thrower_p);
  }

  return f;
} /* ecma_op_create_function_object */

/**
 * [[Call]] implementation for Function objects,
 * created through 13.2 (ECMA_OBJECT_TYPE_FUNCTION)
 * or 15.3.4.5 (ECMA_OBJECT_TYPE_BOUND_FUNCTION).
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_function_call (ecma_object_t *func_obj_p, /**< Function object */
                       ecma_value_t this_arg_value, /**< 'this' argument's value */
                       ecma_value_t* arguments_list_p, /**< arguments list */
                       size_t arguments_list_len) /**< length of arguments list */
{
  JERRY_ASSERT(func_obj_p != NULL && !func_obj_p->is_lexical_environment);
  JERRY_ASSERT(ecma_op_is_callable (ecma_make_object_value (func_obj_p)));
  JERRY_ASSERT(arguments_list_len == 0 || arguments_list_p != NULL);

  if (func_obj_p->u.object.type == ECMA_OBJECT_TYPE_FUNCTION)
  {
    ecma_completion_value_t ret_value;

    /* Entering Function Code (ECMA-262 v5, 10.4.3) */

    ecma_property_t *scope_prop_p = ecma_get_internal_property (func_obj_p, ECMA_INTERNAL_PROPERTY_SCOPE);
    ecma_property_t *code_prop_p = ecma_get_internal_property (func_obj_p, ECMA_INTERNAL_PROPERTY_CODE);

    ecma_object_t *scope_p = ECMA_GET_POINTER(scope_prop_p->u.internal_property.value);
    uint32_t code_prop_value = code_prop_p->u.internal_property.value;

    bool is_strict;
    // 8.
    opcode_counter_t code_first_opcode_idx = ecma_unpack_code_internal_property_value (code_prop_value, &is_strict);

    ecma_value_t this_binding;
    // 1.
    if (is_strict)
    {
      this_binding = ecma_copy_value (this_arg_value, true);
    }
    else if (ecma_is_value_undefined (this_arg_value)
             || ecma_is_value_null (this_arg_value))
    {
      // 2.
      FIXME(Assign Global object when it will be implemented);

      this_binding = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
    else
    {
      // 3., 4.
      ecma_completion_value_t completion = ecma_op_to_object (this_arg_value);
      JERRY_ASSERT(ecma_is_completion_value_normal (completion));

      this_binding = completion.value;
    }

    // 5.
    ecma_object_t *local_env_p = ecma_create_decl_lex_env (scope_p);

    // 9.
    /* Declaration binding instantiation (ECMA-262 v5, 10.5), block 4 */
    TODO(Perform declaration binding instantion when [[FormalParameters]] list will be supported);
    if (arguments_list_len != 0)
    {
      JERRY_UNIMPLEMENTED_REF_UNUSED_VARS(arguments_list_p);
    }

    ecma_completion_value_t completion = run_int_from_pos (code_first_opcode_idx,
                                                           this_binding,
                                                           local_env_p,
                                                           is_strict);
    if (ecma_is_completion_value_normal (completion))
    {
      JERRY_ASSERT(ecma_is_empty_completion_value (completion));

      ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
    else
    {
      ret_value = completion;
    }

    ecma_deref_object (local_env_p);
    ecma_free_value (this_binding, true);

    return ret_value;
  }
  else
  {
    JERRY_ASSERT(func_obj_p->u.object.type == ECMA_OBJECT_TYPE_BOUND_FUNCTION);

    JERRY_UNIMPLEMENTED();
  }
} /* ecma_op_function_call */

/**
 * Get [[ThrowTypeError]] Function Object
 *
 * @return pointer to unique [[ThrowTypeError]] Function Object
 */
ecma_object_t*
ecma_op_get_throw_type_error (void)
{
  TODO(Create [[ThrowTypeError]] during engine initialization and return it from here);

  JERRY_UNIMPLEMENTED();
} /* ecma_op_get_throw_type_error */

/**
 * @}
 * @}
 */
