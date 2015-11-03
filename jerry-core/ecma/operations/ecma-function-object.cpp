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

#include "bytecode-data.h"
#include "ecma-alloc.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"
#include "ecma-objects-arguments.h"
#include "ecma-try-catch-macro.h"

#define JERRY_INTERNAL
#include "jerry-internal.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmafunctionobject ECMA Function object related routines
 * @{
 */

/**
 * Pack 'is_strict', 'do_instantiate_arguments_object' flags and instruction position to value
 * that can be stored in an [[Code]] internal property.
 *
 * @return packed value
 */
static uint32_t
ecma_pack_code_internal_property_value (bool is_strict, /**< is code strict? */
                                        bool do_instantiate_args_obj, /**< should an Arguments object be
                                                                       *   instantiated for the code */
                                        bool is_arguments_moved_to_regs, /**< values of the function's arguments
                                                                          *   are placed on registers */
                                        bool is_no_lex_env, /**< the function needs no lexical environment */
                                        vm_instr_counter_t instr_oc) /**< position of first instruction */
{
  uint32_t value = instr_oc;
  const uint32_t is_strict_bit_offset = (uint32_t) (sizeof (value) * JERRY_BITSINBYTE - 1);
  const uint32_t do_instantiate_arguments_object_bit_offset = (uint32_t) (sizeof (value) * JERRY_BITSINBYTE - 2);
  const uint32_t arguments_moved_to_regs_bit_offset = (uint32_t) (sizeof (value) * JERRY_BITSINBYTE - 3);
  const uint32_t no_lex_env_bit_offset = (uint32_t) (sizeof (value) * JERRY_BITSINBYTE - 4);

  JERRY_ASSERT (((value) & (1u << is_strict_bit_offset)) == 0);
  JERRY_ASSERT (((value) & (1u << do_instantiate_arguments_object_bit_offset)) == 0);
  JERRY_ASSERT (((value) & (1u << arguments_moved_to_regs_bit_offset)) == 0);
  JERRY_ASSERT (((value) & (1u << no_lex_env_bit_offset)) == 0);

  if (is_strict)
  {
    value |= (1u << is_strict_bit_offset);
  }

  if (do_instantiate_args_obj)
  {
    value |= (1u << do_instantiate_arguments_object_bit_offset);
  }

  if (is_arguments_moved_to_regs)
  {
    value |= (1u << arguments_moved_to_regs_bit_offset);
  }

  if (is_no_lex_env)
  {
    value |= (1u << no_lex_env_bit_offset);
  }

  return value;
} /* ecma_pack_code_internal_property_value */

/**
 * Unpack 'is_strict', 'do_instantiate_arguments_object' flags and instruction position from value
 * that can be stored in an [[Code]] internal property.
 *
 * @return instruction position
 */
static vm_instr_counter_t
ecma_unpack_code_internal_property_value (uint32_t value, /**< packed value */
                                          bool* out_is_strict_p, /**< out: is code strict? */
                                          bool* out_do_instantiate_args_obj_p, /**< should an Arguments object be
                                                                                *   instantiated for the code */
                                          bool* out_is_arguments_moved_to_regs_p, /**< values of the function's
                                                                                   *   arguments are placed
                                                                                   *   on registers */
                                          bool* out_is_no_lex_env_p) /**< the function needs no lexical environment */
{
  JERRY_ASSERT (out_is_strict_p != NULL);
  JERRY_ASSERT (out_do_instantiate_args_obj_p != NULL);
  JERRY_ASSERT (out_is_arguments_moved_to_regs_p != NULL);
  JERRY_ASSERT (out_is_no_lex_env_p != NULL);

  const uint32_t is_strict_bit_offset = (uint32_t) (sizeof (value) * JERRY_BITSINBYTE - 1);
  const uint32_t do_instantiate_arguments_object_bit_offset = (uint32_t) (sizeof (value) * JERRY_BITSINBYTE - 2);
  const uint32_t is_arguments_moved_to_regs_bit_offset = (uint32_t) (sizeof (value) * JERRY_BITSINBYTE - 3);
  const uint32_t is_no_lex_env_bit_offset = (uint32_t) (sizeof (value) * JERRY_BITSINBYTE - 4);

  *out_is_strict_p = ((value & (1u << is_strict_bit_offset)) != 0);
  *out_do_instantiate_args_obj_p = ((value & (1u << do_instantiate_arguments_object_bit_offset)) != 0);
  *out_is_arguments_moved_to_regs_p = ((value & (1u << is_arguments_moved_to_regs_bit_offset)) != 0);
  *out_is_no_lex_env_p = ((value & (1u << is_no_lex_env_bit_offset)) != 0);
  value &= ~((1u << is_strict_bit_offset)
             | (1u << do_instantiate_arguments_object_bit_offset)
             | (1u << is_arguments_moved_to_regs_bit_offset)
             | (1u << is_no_lex_env_bit_offset));

  return (vm_instr_counter_t) value;
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
  if (!ecma_is_value_object (value))
  {
    return false;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (value);

  JERRY_ASSERT (obj_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (obj_p));

  return (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_FUNCTION
          || ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION
          || ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION
          || ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION);
} /* ecma_op_is_callable */

/**
 * Check whether the value is Object that implements [[Construct]].
 *
 * @return true, if value is constructor object;
 *         false - otherwise.
 */
bool
ecma_is_constructor (ecma_value_t value) /**< ecma-value */
{
  if (!ecma_is_value_object (value))
  {
    return false;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (value);

  JERRY_ASSERT (obj_p != NULL);
  JERRY_ASSERT (!ecma_is_lexical_environment (obj_p));

  return (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_FUNCTION
          || ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION
          || ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);
} /* ecma_is_constructor */

/**
 * Helper function to merge argument lists
 *
 * See also:
 *          ECMA-262 v5, 15.3.4.5.1 step 4
 *          ECMA-262 v5, 15.3.4.5.2 step 4
 *
 * Used by:
 *         - [[Call]] implementation for Function objects.
 *         - [[Construct]] implementation for Function objects.
 *
 * @return collection of arguments that should be freed
 *         using ecma_free_values_collection with do_ref_if_object flag set
 */
static ecma_collection_header_t *
ecma_function_bind_merge_arg_lists (ecma_object_t *func_obj_p, /**< Function object */
                                    ecma_collection_header_t* passed_arg_collection_p) /**< passed arguments list */
{
  ecma_length_t passed_args_number = passed_arg_collection_p != NULL ? passed_arg_collection_p->unit_number : 0;

  ecma_collection_header_t *merged_arg_collection_p = ecma_new_values_collection (NULL, 0, true);

  ecma_property_t *bound_args_prop_p;
  bound_args_prop_p = ecma_find_internal_property (func_obj_p, ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_BOUND_ARGS);

  if (bound_args_prop_p != NULL)
  {
    ecma_collection_header_t *bound_arg_list_p = ECMA_GET_POINTER (ecma_collection_header_t,
                                                                   bound_args_prop_p->u.internal_property.value);

    ecma_collection_iterator_t bound_args_iterator;
    ecma_collection_iterator_init (&bound_args_iterator, bound_arg_list_p);

    for (ecma_length_t i = 0; i < bound_arg_list_p->unit_number; i++)
    {
      bool is_moved = ecma_collection_iterator_next (&bound_args_iterator);
      JERRY_ASSERT (is_moved);

      ecma_append_to_values_collection (merged_arg_collection_p, *bound_args_iterator.current_value_p, true);
    }
  }

  ecma_collection_iterator_t passed_args_iterator;
  ecma_collection_iterator_init (&passed_args_iterator, passed_arg_collection_p);
  for (ecma_length_t i = 0; i < passed_args_number; i++)
  {
    bool is_moved = ecma_collection_iterator_next (&passed_args_iterator);
    JERRY_ASSERT (is_moved);

    ecma_append_to_values_collection (merged_arg_collection_p, *passed_args_iterator.current_value_p, true);
  }

  return merged_arg_collection_p;
} /* ecma_function_bind_merge_arg_lists */

/**
 * Function object creation operation.
 *
 * See also: ECMA-262 v5, 13.2
 *
 * @return pointer to newly created Function object
 */
ecma_object_t*
ecma_op_create_function_object (ecma_collection_header_t *formal_params_collection_p, /**< formal parameters collection
                                                                                       *   Warning:
                                                                                       *     the collection should not
                                                                                       *     be changed / used / freed
                                                                                       *     by caller after passing it
                                                                                       *     to the routine */
                                ecma_object_t *scope_p, /**< function's scope */
                                bool is_decl_in_strict_mode, /**< is function declared in strict mode code? */
                                const bytecode_data_header_t *bytecode_header_p, /**< byte-code */
                                vm_instr_counter_t first_instr_pos) /**< position of first instruction
                                                                     *   of function's body */
{
  bool is_strict_mode_code = is_decl_in_strict_mode;
  bool do_instantiate_arguments_object = true;
  bool is_arguments_moved_to_regs = false;
  bool is_no_lex_env = false;

  vm_instr_counter_t instr_pos = first_instr_pos;

  if (bytecode_header_p->is_strict)
  {
    is_strict_mode_code = true;
  }

  if (!bytecode_header_p->is_ref_arguments_identifier
      && !bytecode_header_p->is_ref_eval_identifier)
  {
    /* the code doesn't use 'arguments' identifier
     * and doesn't perform direct call to eval,
     * so Arguments object can't be referenced */
    do_instantiate_arguments_object = false;
  }

  if (bytecode_header_p->is_args_moved_to_regs)
  {
    is_arguments_moved_to_regs = true;
  }

  if (bytecode_header_p->is_no_lex_env)
  {
    is_no_lex_env = true;
  }

  // 1., 4., 13.
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE);

  ecma_object_t *f = ecma_create_object (prototype_obj_p, true, ECMA_OBJECT_TYPE_FUNCTION);

  ecma_deref_object (prototype_obj_p);

  // 2., 6., 7., 8.
  /*
   * We don't setup [[Get]], [[Call]], [[Construct]], [[HasInstance]] for each function object.
   * Instead we set the object's type to ECMA_OBJECT_TYPE_FUNCTION
   * that defines which version of the routine should be used on demand.
   */

  // 3.
  /*
   * [[Class]] property is not stored explicitly for objects of ECMA_OBJECT_TYPE_FUNCTION type.
   *
   * See also: ecma_object_get_class_name
   */

  // 9.
  ecma_property_t *scope_prop_p = ecma_create_internal_property (f, ECMA_INTERNAL_PROPERTY_SCOPE);
  ECMA_SET_POINTER (scope_prop_p->u.internal_property.value, scope_p);

  // 10., 11.
  if (formal_params_collection_p != NULL
      && formal_params_collection_p->unit_number != 0)
  {
    ecma_property_t *formal_params_prop_p = ecma_create_internal_property (f,
                                                                           ECMA_INTERNAL_PROPERTY_FORMAL_PARAMETERS);
    ECMA_SET_POINTER (formal_params_prop_p->u.internal_property.value, formal_params_collection_p);
  }

  // 12.
  ecma_property_t *bytecode_prop_p = ecma_create_internal_property (f, ECMA_INTERNAL_PROPERTY_CODE_BYTECODE);
  MEM_CP_SET_NON_NULL_POINTER (bytecode_prop_p->u.internal_property.value, bytecode_header_p);

  ecma_property_t *code_prop_p = ecma_create_internal_property (f, ECMA_INTERNAL_PROPERTY_CODE_FLAGS_AND_OFFSET);
  code_prop_p->u.internal_property.value = ecma_pack_code_internal_property_value (is_strict_mode_code,
                                                                                   do_instantiate_arguments_object,
                                                                                   is_arguments_moved_to_regs,
                                                                                   is_no_lex_env,
                                                                                   instr_pos);

  // 14.
  // 15.
  // 16.
  // 17.
  // 18.
  /*
   * 'length' and 'prototype' properties are instantiated lazily
   *
   * See also: ecma_op_function_object_get_own_property
   *           ecma_op_function_try_lazy_instantiate_property
   */

  // 19.
  if (is_strict_mode_code)
  {
    ecma_object_t *thrower_p = ecma_builtin_get (ECMA_BUILTIN_ID_TYPE_ERROR_THROWER);

    ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();
    {
      prop_desc.is_enumerable_defined = true;
      prop_desc.is_enumerable = false;

      prop_desc.is_configurable_defined = true;
      prop_desc.is_configurable = false;

      prop_desc.is_get_defined = true;
      prop_desc.get_p = thrower_p;

      prop_desc.is_set_defined = true;
      prop_desc.set_p = thrower_p;
    }

    ecma_string_t *magic_string_caller_p = ecma_get_magic_string (LIT_MAGIC_STRING_CALLER);
    ecma_op_object_define_own_property (f,
                                        magic_string_caller_p,
                                        &prop_desc,
                                        false);
    ecma_deref_ecma_string (magic_string_caller_p);

    ecma_string_t *magic_string_arguments_p = ecma_get_magic_string (LIT_MAGIC_STRING_ARGUMENTS);
    ecma_op_object_define_own_property (f,
                                        magic_string_arguments_p,
                                        &prop_desc,
                                        false);
    ecma_deref_ecma_string (magic_string_arguments_p);

    ecma_deref_object (thrower_p);
  }

  return f;
} /* ecma_op_create_function_object */

/**
 * List names of a Function object's lazy instantiated properties,
 * adding them to corresponding string collections
 *
 * See also:
 *          ecma_op_function_try_lazy_instantiate_property
 */
void
ecma_op_function_list_lazy_property_names (bool separate_enumerable, /**< true - list enumerable properties into
                                                                      *          main collection and non-enumerable
                                                                      *          to collection of 'skipped
                                                                      *          non-enumerable' properties,
                                                                      *   false - list all properties into main
                                                                      *           collection.
                                                                      */
                                           ecma_collection_header_t *main_collection_p, /**< 'main' collection */
                                           ecma_collection_header_t *non_enum_collection_p) /**< skipped
                                                                                             *   'non-enumerable'
                                                                                             *   collection */
{
  ecma_collection_header_t *for_enumerable_p = main_collection_p;
  (void) for_enumerable_p;

  ecma_collection_header_t *for_non_enumerable_p = separate_enumerable ? non_enum_collection_p : main_collection_p;

  ecma_string_t *name_p;

  /* 'length' property is non-enumerable (ECMA-262 v5, 13.2.5) */
  name_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);
  ecma_append_to_values_collection (for_non_enumerable_p, ecma_make_string_value (name_p), true);
  ecma_deref_ecma_string (name_p);

  /* 'prototype' property is non-enumerable (ECMA-262 v5, 13.2.18) */
  name_p = ecma_get_magic_string (LIT_MAGIC_STRING_PROTOTYPE);
  ecma_append_to_values_collection (for_non_enumerable_p, ecma_make_string_value (name_p), true);
  ecma_deref_ecma_string (name_p);
} /* ecma_op_function_list_lazy_property_names */

/**
 * Lazy instantation of non-builtin ecma function object's properties
 *
 * Warning:
 *         Only non-configurable properties could be instantiated lazily in this function,
 *         as configurable properties could be deleted and it would be incorrect
 *         to reinstantiate them in the function in second time.
 *
 * @return pointer to newly instantiated property, if a property was instantiated,
 *         NULL - otherwise
 */
static ecma_property_t *
ecma_op_function_try_lazy_instantiate_property (ecma_object_t *obj_p, /**< the function object */
                                                ecma_string_t *property_name_p) /**< property name */
{
  JERRY_ASSERT (!ecma_get_object_is_builtin (obj_p));

  ecma_string_t *magic_string_length_p = ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH);

  bool is_length_property = ecma_compare_ecma_strings (magic_string_length_p, property_name_p);

  ecma_deref_ecma_string (magic_string_length_p);

  if (is_length_property)
  {
    /* ECMA-262 v5, 13.2, 14-15 */

    // 14
    ecma_number_t *len_p = ecma_alloc_number ();

    ecma_property_t *formal_parameters_prop_p = ecma_find_internal_property (obj_p,
                                                                             ECMA_INTERNAL_PROPERTY_FORMAL_PARAMETERS);
    if (formal_parameters_prop_p == NULL)
    {
      ecma_property_t *bytecode_prop_p = ecma_get_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_CODE_BYTECODE);
      ecma_property_t *code_prop_p = ecma_get_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_CODE_FLAGS_AND_OFFSET);

      uint32_t code_prop_value = code_prop_p->u.internal_property.value;

      bool is_strict;
      bool do_instantiate_args_obj;
      bool is_arguments_moved_to_regs;
      bool is_no_lex_env;

      const bytecode_data_header_t *bytecode_header_p;
      bytecode_header_p = MEM_CP_GET_POINTER (const bytecode_data_header_t, bytecode_prop_p->u.internal_property.value);

      vm_instr_counter_t code_first_instr_pos = ecma_unpack_code_internal_property_value (code_prop_value,
                                                                                          &is_strict,
                                                                                          &do_instantiate_args_obj,
                                                                                          &is_arguments_moved_to_regs,
                                                                                          &is_no_lex_env);

      *len_p = vm_get_scope_args_num (bytecode_header_p, code_first_instr_pos);
    }
    else
    {
      ecma_collection_header_t *formal_parameters_p;
      formal_parameters_p = ECMA_GET_NON_NULL_POINTER (ecma_collection_header_t,
                                                       formal_parameters_prop_p->u.internal_property.value);
      *len_p = ecma_uint32_to_number (formal_parameters_p->unit_number);
    }

    // 15
    ecma_property_t *length_prop_p = ecma_create_named_data_property (obj_p,
                                                                      property_name_p,
                                                                      false,
                                                                      false,
                                                                      false);

    ecma_named_data_property_assign_value (obj_p, length_prop_p, ecma_make_number_value (len_p));

    ecma_dealloc_number (len_p);

    JERRY_ASSERT (!ecma_is_property_configurable (length_prop_p));
    return length_prop_p;
  }

  ecma_string_t *magic_string_prototype_p = ecma_get_magic_string (LIT_MAGIC_STRING_PROTOTYPE);

  bool is_prototype_property = ecma_compare_ecma_strings (magic_string_prototype_p, property_name_p);

  ecma_deref_ecma_string (magic_string_prototype_p);

  if (is_prototype_property)
  {
    /* ECMA-262 v5, 13.2, 16-18 */

    // 16.
    ecma_object_t *proto_p = ecma_op_create_object_object_noarg ();

    // 17.
    ecma_string_t *magic_string_constructor_p = ecma_get_magic_string (LIT_MAGIC_STRING_CONSTRUCTOR);
    ecma_builtin_helper_def_prop (proto_p,
                                  magic_string_constructor_p,
                                  ecma_make_object_value (obj_p),
                                  true, /* Writable */
                                  false, /* Enumerable */
                                  true, /* Configurable */
                                  false); /* Failure handling */

    ecma_deref_ecma_string (magic_string_constructor_p);

    // 18.
    ecma_property_t *prototype_prop_p = ecma_create_named_data_property (obj_p,
                                                                         property_name_p,
                                                                         true,
                                                                         false,
                                                                         false);

    ecma_named_data_property_assign_value (obj_p, prototype_prop_p, ecma_make_object_value (proto_p));

    ecma_deref_object (proto_p);

    JERRY_ASSERT (!ecma_is_property_configurable (prototype_prop_p));
    return prototype_prop_p;
  }

  return NULL;
} /* ecma_op_function_try_lazy_instantiate_property */

/**
 * Implementation-defined extension of [[GetOwnProperty]] ecma function object's operation
 *
 * Note:
 *      The [[GetOwnProperty]] is used only for lazy property instantiation,
 *      i.e. externally visible behaviour of [[GetOwnProperty]] is specification-defined
 *
 * @return pointer to a property - if it already existed
 *           or was lazy instantiated in context of
 *           current invocation,
 *         NULL (i.e. ecma-undefined) - otherwise.
 */
ecma_property_t*
ecma_op_function_object_get_own_property (ecma_object_t *obj_p, /**< the function object */
                                          ecma_string_t *property_name_p) /**< property name */
{
  JERRY_ASSERT (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_FUNCTION);

  ecma_property_t *prop_p = ecma_op_general_object_get_own_property (obj_p, property_name_p);

  if (prop_p != NULL)
  {
    return prop_p;
  }
  else if (!ecma_get_object_is_builtin (obj_p))
  {
    prop_p = ecma_op_function_try_lazy_instantiate_property (obj_p, property_name_p);

    /*
     * Only non-configurable properties could be instantiated lazily in the function,
     * as configurable properties could be deleted and it would be incorrect
     * to reinstantiate them in the function in second time.
     */
    JERRY_ASSERT (prop_p == NULL || !ecma_is_property_configurable (prop_p));
  }

  return prop_p;
} /* ecma_op_function_object_get_own_property */

/**
 * External function object creation operation.
 *
 * Note:
 *      external function object is implementation-defined object type
 *      that represent functions implemented in native code, using Embedding API
 *
 * @return pointer to newly created external function object
 */
ecma_object_t*
ecma_op_create_external_function_object (ecma_external_pointer_t code_p) /**< pointer to external native handler */
{
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE);

  ecma_object_t *function_obj_p = ecma_create_object (prototype_obj_p, true, ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);

  ecma_deref_object (prototype_obj_p);

  /*
   * [[Class]] property is not stored explicitly for objects of ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION type.
   *
   * See also: ecma_object_get_class_name
   */

  bool is_created = ecma_create_external_pointer_property (function_obj_p,
                                                           ECMA_INTERNAL_PROPERTY_NATIVE_CODE,
                                                           (ecma_external_pointer_t) code_p);
  JERRY_ASSERT (is_created);

  ecma_string_t *magic_string_prototype_p = ecma_get_magic_string (LIT_MAGIC_STRING_PROTOTYPE);
  ecma_builtin_helper_def_prop (function_obj_p,
                                magic_string_prototype_p,
                                ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED),
                                true, /* Writable */
                                false, /* Enumerable */
                                false, /* Configurable */
                                false); /* Failure handling */

  ecma_deref_ecma_string (magic_string_prototype_p);

  return function_obj_p;
} /* ecma_op_create_external_function_object */

/**
 * Setup variables for arguments listed in formal parameter list,
 * and, if necessary, Arguments object with 'arguments' binding.
 *
 * See also:
 *          Declaration binding instantiation (ECMA-262 v5, 10.5), block 4 and 7
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
ecma_function_call_setup_args_variables (ecma_object_t *func_obj_p, /**< Function object */
                                         ecma_object_t *env_p, /**< lexical environment */
                                         ecma_collection_header_t *arg_collection_p, /**< arguments collection */
                                         bool is_strict, /**< flag indicating strict mode */
                                         bool do_instantiate_arguments_object) /**< flag indicating whether
                                                                                *   Arguments object should be
                                                                                *   instantiated */
{
  ecma_collection_header_t *formal_parameters_p = NULL;

  ecma_property_t *formal_parameters_prop_p = ecma_find_internal_property (func_obj_p,
                                                                           ECMA_INTERNAL_PROPERTY_FORMAL_PARAMETERS);

  if (formal_parameters_prop_p != NULL)
  {
    formal_parameters_p = ECMA_GET_POINTER (ecma_collection_header_t,
                                            formal_parameters_prop_p->u.internal_property.value);

    ecma_length_t formal_parameters_count = formal_parameters_p->unit_number;

    ecma_collection_iterator_t arguments_iterator, formal_params_iterator;

    ecma_collection_iterator_init (&arguments_iterator, arg_collection_p);
    ecma_collection_iterator_init (&formal_params_iterator, formal_parameters_p);

    /*
     * Formal parameter list is stored in reversed order
     *
     * Although, specification defines ascending order of formal parameters list enumeration,
     * implementation enumerates the parameters in descending order.
     *
     * In the case, redundant SetMutableBinding invocation could be avoided.
     */
    for (ecma_length_t n = 0;
         n < formal_parameters_count;
         n++)
    {
      ecma_value_t arg_value;
      if (ecma_collection_iterator_next (&arguments_iterator))
      {
        arg_value = *arguments_iterator.current_value_p;
      }
      else
      {
        arg_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
      }

      bool is_moved = ecma_collection_iterator_next (&formal_params_iterator);
      JERRY_ASSERT (is_moved);

      ecma_value_t formal_parameter_name_value = *formal_params_iterator.current_value_p;
      ecma_string_t *formal_parameter_name_string_p = ecma_get_string_from_value (formal_parameter_name_value);

      bool arg_already_declared = ecma_op_has_binding (env_p, formal_parameter_name_string_p);
      if (!arg_already_declared)
      {
        ecma_completion_value_t completion = ecma_op_create_mutable_binding (env_p,
                                                                             formal_parameter_name_string_p,
                                                                             false);
        if (ecma_is_completion_value_throw (completion))
        {
          return completion;
        }

        JERRY_ASSERT (ecma_is_completion_value_empty (completion));
      }

      ecma_completion_value_t completion = ecma_op_set_mutable_binding (env_p,
                                                                        formal_parameter_name_string_p,
                                                                        arg_value,
                                                                        is_strict);

      if (ecma_is_completion_value_throw (completion))
      {
        return completion;
      }

      JERRY_ASSERT (ecma_is_completion_value_empty (completion));
    }
  }

  if (do_instantiate_arguments_object)
  {
    /*
     * According to ECMA-262 v5, 10.5, the Arguments object should be instantiated
     * after instantiating declared functions, and only if there is no binding named 'arguments'
     * by that time.
     *
     * However, we can setup Arguments object and 'arguments' binding here, because:
     *  - instantiation of Arguments object itself doesn't have any side effects;
     *  - if 'arguments' is name of a declared function in current scope,
     *    value of the binding would be overwritten, execution would proceed in correct state.
     *  - declaration of function, named 'arguments', is considered to be unrecommended (and so, rare) case,
     *    so instantiation of Arguments object here, in general, is supposed to not affect resource consumption
     *    significantly.
     */

    ecma_string_t *arguments_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_ARGUMENTS);

    bool binding_already_declared = ecma_op_has_binding (env_p, arguments_string_p);

    if (!binding_already_declared)
    {
      ecma_object_t *args_obj_p = ecma_op_create_arguments_object (func_obj_p,
                                                                   env_p,
                                                                   formal_parameters_p,
                                                                   arg_collection_p,
                                                                   is_strict);

      if (is_strict)
      {
        ecma_op_create_immutable_binding (env_p, arguments_string_p);
        ecma_op_initialize_immutable_binding (env_p, arguments_string_p, ecma_make_object_value (args_obj_p));
      }
      else
      {
        ecma_completion_value_t completion = ecma_op_create_mutable_binding (env_p,
                                                                             arguments_string_p,
                                                                             false);
        JERRY_ASSERT (ecma_is_completion_value_empty (completion));

        completion = ecma_op_set_mutable_binding (env_p,
                                                  arguments_string_p,
                                                  ecma_make_object_value (args_obj_p),
                                                  false);
        JERRY_ASSERT (ecma_is_completion_value_empty (completion));
      }

      ecma_deref_object (args_obj_p);
    }

    ecma_deref_ecma_string (arguments_string_p);
  }

  return ecma_make_empty_completion_value ();
} /* ecma_function_call_setup_args_variables */

/**
 * [[Call]] implementation for Function objects,
 * created through 13.2 (ECMA_OBJECT_TYPE_FUNCTION)
 * or 15.3.4.5 (ECMA_OBJECT_TYPE_BOUND_FUNCTION),
 * and for built-in Function objects
 * from section 15 (ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION).
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_function_has_instance (ecma_object_t *func_obj_p, /**< Function object */
                               ecma_value_t value) /**< argument 'V' */
{
  JERRY_ASSERT (func_obj_p != NULL
                && !ecma_is_lexical_environment (func_obj_p));

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_FUNCTION)
  {
    if (!ecma_is_value_object (value))
    {
      return ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE);
    }

    ecma_object_t* v_obj_p = ecma_get_object_from_value (value);

    ecma_string_t *prototype_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_PROTOTYPE);

    ECMA_TRY_CATCH (prototype_obj_value,
                    ecma_op_object_get (func_obj_p, prototype_magic_string_p),
                    ret_value);

    if (!ecma_is_value_object (prototype_obj_value))
    {
      ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
    else
    {
      ecma_object_t *prototype_obj_p = ecma_get_object_from_value (prototype_obj_value);
      JERRY_ASSERT (prototype_obj_p != NULL);

      do
      {
        v_obj_p = ecma_get_object_prototype (v_obj_p);

        if (v_obj_p == NULL)
        {
          ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE);

          break;
        }
        else if (v_obj_p == prototype_obj_p)
        {
          ret_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);

          break;
        }
      } while (true);
    }

    ECMA_FINALIZE (prototype_obj_value);

    ecma_deref_ecma_string (prototype_magic_string_p);
  }
  else if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION ||
           ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION)
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);

    /* 1. */
    ecma_property_t *target_function_prop_p;
    target_function_prop_p = ecma_get_internal_property (func_obj_p,
                                                         ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_TARGET_FUNCTION);

    ecma_object_t *target_func_obj_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t,
                                                                  target_function_prop_p->u.internal_property.value);

    /* 3. */
    ret_value = ecma_op_object_has_instance (target_func_obj_p, value);
  }

  return ret_value;
} /* ecma_op_function_has_instance */

/**
 * Function call helper with arguments list specified in array instead of collection
 *
 * See also:
 *          ecma_op_function_call
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_function_call_array_args (ecma_object_t *func_obj_p, /**< Function object */
                                  ecma_value_t this_arg_value, /**< 'this' argument's value */
                                  const ecma_value_t* arguments_list_p, /**< arguments list */
                                  ecma_length_t arguments_list_len) /**< length of arguments list */
{
  if (arguments_list_len == 0)
  {
    return ecma_op_function_call (func_obj_p, this_arg_value, NULL);
  }
  else
  {
    ecma_collection_header_t *arg_collection_p = ecma_new_values_collection (arguments_list_p,
                                                                             arguments_list_len,
                                                                             true);

    ecma_completion_value_t ret_value = ecma_op_function_call (func_obj_p,
                                                               this_arg_value,
                                                               arg_collection_p);

    ecma_free_values_collection (arg_collection_p, true);

    return ret_value;
  }
} /* ecma_op_function_call_array_args */

/**
 * [[Call]] implementation for Function objects,
 * created through 13.2 (ECMA_OBJECT_TYPE_FUNCTION)
 * or 15.3.4.5 (ECMA_OBJECT_TYPE_BOUND_FUNCTION),
 * and for built-in Function objects
 * from section 15 (ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION).
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_function_call (ecma_object_t *func_obj_p, /**< Function object */
                       ecma_value_t this_arg_value, /**< 'this' argument's value */
                       ecma_collection_header_t* arg_collection_p) /**< arguments list */
{
  JERRY_ASSERT (func_obj_p != NULL
                && !ecma_is_lexical_environment (func_obj_p));
  JERRY_ASSERT (ecma_op_is_callable (ecma_make_object_value (func_obj_p)));

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_FUNCTION)
  {
    if (unlikely (ecma_get_object_is_builtin (func_obj_p)))
    {
      ret_value = ecma_builtin_dispatch_call (func_obj_p,
                                              this_arg_value,
                                              arg_collection_p);
    }
    else
    {
      /* Entering Function Code (ECMA-262 v5, 10.4.3) */
      ecma_property_t *scope_prop_p = ecma_get_internal_property (func_obj_p, ECMA_INTERNAL_PROPERTY_SCOPE);
      ecma_property_t *bytecode_prop_p = ecma_get_internal_property (func_obj_p, ECMA_INTERNAL_PROPERTY_CODE_BYTECODE);
      ecma_property_t *code_prop_p = ecma_get_internal_property (func_obj_p,
                                                                 ECMA_INTERNAL_PROPERTY_CODE_FLAGS_AND_OFFSET);

      ecma_object_t *scope_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t,
                                                          scope_prop_p->u.internal_property.value);
      uint32_t code_prop_value = code_prop_p->u.internal_property.value;

      // 8.
      bool is_strict;
      bool do_instantiate_args_obj;
      bool is_arguments_moved_to_regs;
      bool is_no_lex_env;

      const bytecode_data_header_t *bytecode_data_p;
      bytecode_data_p = MEM_CP_GET_POINTER (const bytecode_data_header_t, bytecode_prop_p->u.internal_property.value);

      vm_instr_counter_t code_first_instr_pos = ecma_unpack_code_internal_property_value (code_prop_value,
                                                                                          &is_strict,
                                                                                          &do_instantiate_args_obj,
                                                                                          &is_arguments_moved_to_regs,
                                                                                          &is_no_lex_env);

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
        this_binding = ecma_make_object_value (ecma_builtin_get (ECMA_BUILTIN_ID_GLOBAL));
      }
      else
      {
        // 3., 4.
        ecma_completion_value_t completion = ecma_op_to_object (this_arg_value);
        JERRY_ASSERT (ecma_is_completion_value_normal (completion));

        this_binding = ecma_get_completion_value_value (completion);
      }

      // 5.
      ecma_object_t *local_env_p;
      if (is_no_lex_env)
      {
        local_env_p = scope_p;
      }
      else
      {
        local_env_p = ecma_create_decl_lex_env (scope_p);
      }

      if (is_arguments_moved_to_regs)
      {
        ecma_completion_value_t completion = vm_run_from_pos (bytecode_data_p,
                                                              code_first_instr_pos,
                                                              this_binding,
                                                              local_env_p,
                                                              is_strict,
                                                              false,
                                                              arg_collection_p);

        if (ecma_is_completion_value_return (completion))
        {
          ret_value = ecma_make_normal_completion_value (ecma_get_completion_value_value (completion));
        }
        else
        {
          ret_value = completion;
        }
      }
      else
      {
        // 9.
        ECMA_TRY_CATCH (args_var_declaration_ret,
                        ecma_function_call_setup_args_variables (func_obj_p,
                                                                 local_env_p,
                                                                 arg_collection_p,
                                                                 is_strict,
                                                                 do_instantiate_args_obj),
                        ret_value);

        ecma_completion_value_t completion = vm_run_from_pos (bytecode_data_p,
                                                              code_first_instr_pos,
                                                              this_binding,
                                                              local_env_p,
                                                              is_strict,
                                                              false,
                                                              NULL);

        if (ecma_is_completion_value_return (completion))
        {
          ret_value = ecma_make_normal_completion_value (ecma_get_completion_value_value (completion));
        }
        else
        {
          ret_value = completion;
        }

        ECMA_FINALIZE (args_var_declaration_ret);
      }

      if (is_no_lex_env)
      {
        /* do nothing */
      }
      else
      {
        ecma_deref_object (local_env_p);
      }

      ecma_free_value (this_binding, true);
    }
  }
  else if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION)
  {
    ret_value = ecma_builtin_dispatch_call (func_obj_p,
                                            this_arg_value,
                                            arg_collection_p);
  }
  else if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION)
  {
    ecma_external_pointer_t handler_p;
    bool is_retrieved = ecma_get_external_pointer_value (func_obj_p,
                                                         ECMA_INTERNAL_PROPERTY_NATIVE_CODE,
                                                         &handler_p);
    JERRY_ASSERT (is_retrieved);

    ret_value = jerry_dispatch_external_function (func_obj_p,
                                                  handler_p,
                                                  this_arg_value,
                                                  arg_collection_p);
  }
  else
  {
    JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);

    /* 2-3. */
    ecma_property_t *bound_this_prop_p;
    ecma_property_t *target_function_prop_p;

    bound_this_prop_p = ecma_get_internal_property (func_obj_p, ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_BOUND_THIS);
    target_function_prop_p = ecma_get_internal_property (func_obj_p,
                                                         ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_TARGET_FUNCTION);

    ecma_object_t *target_func_obj_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t,
                                                                  target_function_prop_p->u.internal_property.value);

    /* 4. */
    ecma_collection_header_t *merged_arg_collection_p = ecma_function_bind_merge_arg_lists (func_obj_p,
                                                                                            arg_collection_p);

    ecma_value_t bound_this_value = bound_this_prop_p->u.internal_property.value;

    /* 5. */
    ret_value = ecma_op_function_call (target_func_obj_p,
                                       bound_this_value,
                                       merged_arg_collection_p);

    ecma_free_values_collection (merged_arg_collection_p, true);
  }

  JERRY_ASSERT (!ecma_is_completion_value_empty (ret_value));

  return ret_value;
} /* ecma_op_function_call */

/**
 * [[Construct]] implementation for Function objects (13.2.2),
 * created through 13.2 (ECMA_OBJECT_TYPE_FUNCTION) and
 * externally defined (host) functions (ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION).
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
static ecma_completion_value_t
ecma_op_function_construct_simple_or_external (ecma_object_t *func_obj_p, /**< Function object */
                                               ecma_collection_header_t* arg_collection_p) /**< arguments
                                                                                            *   collection */
{
  JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_FUNCTION
                || ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ecma_string_t *prototype_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_PROTOTYPE);

  // 5.
  ECMA_TRY_CATCH (func_obj_prototype_prop_value,
                  ecma_op_object_get (func_obj_p,
                                      prototype_magic_string_p),
                  ret_value);

  // 1., 2., 4.
  ecma_object_t *obj_p;
  if (ecma_is_value_object (func_obj_prototype_prop_value))
  {
    //  6.
    obj_p = ecma_create_object (ecma_get_object_from_value (func_obj_prototype_prop_value),
                                true,
                                ECMA_OBJECT_TYPE_GENERAL);
  }
  else
  {
    // 7.
    ecma_object_t *prototype_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);

    obj_p = ecma_create_object (prototype_p, true, ECMA_OBJECT_TYPE_GENERAL);

    ecma_deref_object (prototype_p);
  }

  // 3.
  /*
   * [[Class]] property of ECMA_OBJECT_TYPE_GENERAL type objects
   * without ECMA_INTERNAL_PROPERTY_CLASS internal property
   * is "Object".
   *
   * See also: ecma_object_get_class_name.
   */

  // 8.
  ECMA_TRY_CATCH (call_completion,
                  ecma_op_function_call (func_obj_p,
                                         ecma_make_object_value (obj_p),
                                         arg_collection_p),
                  ret_value);

  ecma_value_t obj_value;

  // 9.
  if (ecma_is_value_object (call_completion))
  {
    obj_value = ecma_copy_value (call_completion, true);
  }
  else
  {
    // 10.
    ecma_ref_object (obj_p);
    obj_value = ecma_make_object_value (obj_p);
  }

  ret_value = ecma_make_normal_completion_value (obj_value);

  ECMA_FINALIZE (call_completion);

  ecma_deref_object (obj_p);

  ECMA_FINALIZE (func_obj_prototype_prop_value);

  ecma_deref_ecma_string (prototype_magic_string_p);

  return ret_value;
} /* ecma_op_function_construct_simple_or_external */

/**
 * [[Construct]] implementation:
 *   13.2.2 - for Function objects, created through 13.2 (ECMA_OBJECT_TYPE_FUNCTION),
 *            and externally defined host functions (ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);
 *   15.3.4.5.1 - for Function objects, created through 15.3.4.5 (ECMA_OBJECT_TYPE_BOUND_FUNCTION).
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_function_construct (ecma_object_t *func_obj_p, /**< Function object */
                            ecma_collection_header_t *arg_collection_p) /**< arguments collection */
{
  JERRY_ASSERT (func_obj_p != NULL
                && !ecma_is_lexical_environment (func_obj_p));
  JERRY_ASSERT (ecma_is_constructor (ecma_make_object_value (func_obj_p)));

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_FUNCTION)
  {
    if (unlikely (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_FUNCTION
                  && ecma_get_object_is_builtin (func_obj_p)))
    {
      ret_value = ecma_builtin_dispatch_construct (func_obj_p, arg_collection_p);
    }
    else
    {
      ret_value = ecma_op_function_construct_simple_or_external (func_obj_p, arg_collection_p);
    }
  }
  else if (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION)
  {
    ret_value = ecma_op_function_construct_simple_or_external (func_obj_p, arg_collection_p);
  }
  else
  {
    JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_BOUND_FUNCTION);

    /* 1. */
    ecma_property_t *target_function_prop_p;
    target_function_prop_p = ecma_get_internal_property (func_obj_p,
                                                         ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_TARGET_FUNCTION);

    ecma_object_t *target_func_obj_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t,
                                                                  target_function_prop_p->u.internal_property.value);

    /* 2. */
    if (!ecma_is_constructor (ecma_make_object_value (target_func_obj_p)))
    {
      ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
    else
    {
      /* 4. */
      ecma_collection_header_t *merged_arg_collection_p = ecma_function_bind_merge_arg_lists (func_obj_p,
                                                                                              arg_collection_p);

      /* 5. */
      ret_value = ecma_op_function_construct (target_func_obj_p,
                                              merged_arg_collection_p);

      ecma_free_values_collection (merged_arg_collection_p, true);
    }
  }

  return ret_value;
} /* ecma_op_function_construct */

/**
 * Function declaration.
 *
 * See also: ECMA-262 v5, 10.5 - Declaration binding instantiation (block 5).
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_op_function_declaration (ecma_object_t *lex_env_p, /**< lexical environment */
                              ecma_string_t *function_name_p, /**< function name */
                              const bytecode_data_header_t *bytecode_data_p, /**< byte-code data */
                              vm_instr_counter_t function_first_instr_pos, /**< position of first instruction
                                                                            *   of function code */
                              ecma_collection_header_t *formal_params_collection_p, /**< formal parameters collection
                                                                                     *   Warning:
                                                                                     *     the collection should not
                                                                                     *     be changed / used / freed
                                                                                     *     by caller after passing it
                                                                                     *     to the routine */
                              bool is_decl_in_strict_mode, /**< flag, indicating if function is
                                                            *   declared in strict mode code */
                              bool is_configurable_bindings) /**< flag indicating whether function
                                                              *   is declared in eval code */
{
  // b.
  ecma_object_t *func_obj_p = ecma_op_create_function_object (formal_params_collection_p,
                                                              lex_env_p,
                                                              is_decl_in_strict_mode,
                                                              bytecode_data_p,
                                                              function_first_instr_pos);

  // c.
  bool func_already_declared = ecma_op_has_binding (lex_env_p, function_name_p);

  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  // d.
  if (!func_already_declared)
  {
    ecma_completion_value_t completion = ecma_op_create_mutable_binding (lex_env_p,
                                                                         function_name_p,
                                                                         is_configurable_bindings);

    JERRY_ASSERT (ecma_is_completion_value_empty (completion));
  }
  else if (ecma_is_lexical_environment_global (lex_env_p))
  {
    // e.
    ecma_object_t *glob_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_GLOBAL);

    ecma_property_t *existing_prop_p = ecma_op_object_get_property (glob_obj_p, function_name_p);

    if (ecma_is_property_configurable (existing_prop_p))
    {
      ecma_completion_value_t completion;
      completion = ecma_builtin_helper_def_prop (glob_obj_p,
                                                 function_name_p,
                                                 ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED),
                                                 true, /* Writable */
                                                 true, /* Enumerable */
                                                 is_configurable_bindings, /* Configurable */
                                                 true); /* Failure handling */

      JERRY_ASSERT (ecma_is_completion_value_normal_true (completion));
    }
    else if (existing_prop_p->type == ECMA_PROPERTY_NAMEDACCESSOR)
    {
      ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
    else
    {
      JERRY_ASSERT (existing_prop_p->type == ECMA_PROPERTY_NAMEDDATA);

      if (!ecma_is_property_writable (existing_prop_p)
          || !ecma_is_property_enumerable (existing_prop_p))
      {
        ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
      }
    }

    ecma_deref_object (glob_obj_p);
  }

  if (ecma_is_completion_value_empty (ret_value))
  {
    // f.
    ret_value = ecma_op_set_mutable_binding (lex_env_p,
                                             function_name_p,
                                             ecma_make_object_value (func_obj_p),
                                             is_decl_in_strict_mode);
  }
  else
  {
    JERRY_ASSERT (ecma_is_completion_value_throw (ret_value));
  }

  ecma_deref_object (func_obj_p);

  return ret_value;
} /* ecma_op_function_declaration */

/**
 * @}
 * @}
 */
