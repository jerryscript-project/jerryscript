/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lcache.h"
#include "ecma-lex-env.h"
#include "ecma-objects.h"
#include "ecma-objects-arguments.h"
#include "ecma-objects-general.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmafunctionobject ECMA Function object related routines
 * @{
 */

/**
 * Arguments object creation operation.
 *
 * See also: ECMA-262 v5, 10.6
 */
void
ecma_op_create_arguments_object (ecma_object_t *func_obj_p, /**< callee function */
                                 ecma_object_t *lex_env_p, /**< lexical environment the Arguments
                                                                object is created for */
                                 const ecma_value_t *arguments_list_p, /**< arguments list */
                                 ecma_length_t arguments_number, /**< length of arguments list */
                                 const ecma_compiled_code_t *bytecode_data_p) /**< byte code */
{
  // 2., 3., 6.
  ecma_object_t *prototype_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);

  ecma_object_t *obj_p = ecma_create_object (prototype_p, false, true, ECMA_OBJECT_TYPE_GENERAL);

  ecma_deref_object (prototype_p);

  // 11.a, 11.b
  for (ecma_length_t indx = 0;
       indx < arguments_number;
       indx++)
  {
    ecma_value_t completion;
    ecma_string_t *indx_string_p = ecma_new_ecma_string_from_uint32 (indx);

    completion = ecma_builtin_helper_def_prop (obj_p,
                                               indx_string_p,
                                               arguments_list_p[indx],
                                               true, /* Writable */
                                               true, /* Enumerable */
                                               true, /* Configurable */
                                               false); /* Failure handling */

    JERRY_ASSERT (ecma_is_value_true (completion));

    ecma_deref_ecma_string (indx_string_p);
  }

  bool is_strict = (bytecode_data_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE) != 0;

  // 1.

  // 7.
  ecma_string_t *length_magic_string_p = ecma_new_ecma_length_string ();
  ecma_value_t completion = ecma_builtin_helper_def_prop (obj_p,
                                                          length_magic_string_p,
                                                          ecma_make_uint32_value (arguments_number),
                                                          true, /* Writable */
                                                          false, /* Enumerable */
                                                          true, /* Configurable */
                                                          false); /* Failure handling */

  JERRY_ASSERT (ecma_is_value_true (completion));
  ecma_deref_ecma_string (length_magic_string_p);

  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();

  ecma_length_t formal_params_number;
  jmem_cpointer_t *literal_p;

  if (bytecode_data_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) bytecode_data_p;
    uint8_t *byte_p = (uint8_t *) bytecode_data_p;

    formal_params_number = args_p->argument_end;
    literal_p = (jmem_cpointer_t *) (byte_p + sizeof (cbc_uint16_arguments_t));
  }
  else
  {
    cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) bytecode_data_p;
    uint8_t *byte_p = (uint8_t *) bytecode_data_p;

    formal_params_number = args_p->argument_end;
    literal_p = (jmem_cpointer_t *) (byte_p + sizeof (cbc_uint8_arguments_t));
  }

  if (!is_strict
      && arguments_number > 0
      && formal_params_number > 0)
  {
    // 8.
    ecma_object_t *map_p = ecma_op_create_object_object_noarg ();

    // 11.c
    for (uint32_t indx = 0;
         indx < formal_params_number;
         indx++)
    {
      // i.
      if (literal_p[indx] == JMEM_CP_NULL)
      {
        continue;
      }

      ecma_string_t *name_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_string_t, literal_p[indx]);
      ecma_string_t *indx_string_p = ecma_new_ecma_string_from_uint32 ((uint32_t) indx);

      prop_desc.is_value_defined = true;
      prop_desc.value = ecma_make_string_value (name_p);

      prop_desc.is_configurable_defined = true;
      prop_desc.is_configurable = true;

      completion = ecma_op_object_define_own_property (map_p,
                                                       indx_string_p,
                                                       &prop_desc,
                                                       false);
      JERRY_ASSERT (ecma_is_value_true (completion));

      ecma_deref_ecma_string (indx_string_p);
    }

    // 12.
    ecma_set_object_type (obj_p, ECMA_OBJECT_TYPE_ARGUMENTS);

    ecma_value_t *parameters_map_prop_p = ecma_create_internal_property (obj_p,
                                                                         ECMA_INTERNAL_PROPERTY_PARAMETERS_MAP);
    ECMA_SET_INTERNAL_VALUE_POINTER (*parameters_map_prop_p, map_p);

    ecma_value_t *scope_prop_p = ecma_create_internal_property (map_p,
                                                                ECMA_INTERNAL_PROPERTY_SCOPE);
    ECMA_SET_INTERNAL_VALUE_POINTER (*scope_prop_p, lex_env_p);

    ecma_deref_object (map_p);
  }
  else
  {
    // 4.
    ecma_value_t *class_prop_p = ecma_create_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_CLASS);
    *class_prop_p = LIT_MAGIC_STRING_ARGUMENTS_UL;
  }

  // 13.
  if (!is_strict)
  {
    ecma_string_t *callee_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_CALLEE);

    completion = ecma_builtin_helper_def_prop (obj_p,
                                               callee_magic_string_p,
                                               ecma_make_object_value (func_obj_p),
                                               true, /* Writable */
                                               false, /* Enumerable */
                                               true, /* Configurable */
                                               false); /* Failure handling */

    JERRY_ASSERT (ecma_is_value_true (completion));

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

    ecma_string_t *callee_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_CALLEE);

    completion = ecma_op_object_define_own_property (obj_p,
                                                     callee_magic_string_p,
                                                     &prop_desc,
                                                     false);
    JERRY_ASSERT (ecma_is_value_true (completion));
    ecma_deref_ecma_string (callee_magic_string_p);

    ecma_string_t *caller_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_CALLER);
    completion = ecma_op_object_define_own_property (obj_p,
                                                     caller_magic_string_p,
                                                     &prop_desc,
                                                     false);
    JERRY_ASSERT (ecma_is_value_true (completion));
    ecma_deref_ecma_string (caller_magic_string_p);

    ecma_deref_object (thrower_p);
  }

  ecma_string_t *arguments_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_ARGUMENTS);

  if (is_strict)
  {
    ecma_op_create_immutable_binding (lex_env_p,
                                      arguments_string_p,
                                      ecma_make_object_value (obj_p));
  }
  else
  {
    ecma_value_t completion = ecma_op_create_mutable_binding (lex_env_p,
                                                              arguments_string_p,
                                                              false);
    JERRY_ASSERT (ecma_is_value_empty (completion));

    completion = ecma_op_set_mutable_binding (lex_env_p,
                                              arguments_string_p,
                                              ecma_make_object_value (obj_p),
                                              false);

    JERRY_ASSERT (ecma_is_value_empty (completion));
  }

  ecma_deref_ecma_string (arguments_string_p);
  ecma_deref_object (obj_p);
} /* ecma_op_create_arguments_object */

/**
 * Get value of function's argument mapped to index of Arguments object.
 *
 * Note:
 *      The procedure emulates execution of function described by MakeArgGetter
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
void
ecma_arguments_update_mapped_arg_value (ecma_object_t *object_p, /**< the object */
                                        ecma_string_t *property_name_p, /**< property name */
                                        ecma_property_t *property_p) /**< property value */
{
  ecma_value_t *map_prop_p = ecma_get_internal_property (object_p, ECMA_INTERNAL_PROPERTY_PARAMETERS_MAP);
  ecma_object_t *map_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t, *map_prop_p);

  ecma_value_t arg_name = ecma_op_object_find (map_p, property_name_p);

  if (!ecma_is_value_found (arg_name))
  {
    return;
  }

  ecma_value_t *scope_prop_p = ecma_get_internal_property (map_p, ECMA_INTERNAL_PROPERTY_SCOPE);
  ecma_object_t *lex_env_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t, *scope_prop_p);

  JERRY_ASSERT (lex_env_p != NULL
                && ecma_is_lexical_environment (lex_env_p));

  ecma_string_t *arg_name_p = ecma_get_string_from_value (arg_name);
  ecma_value_t value = ecma_op_get_binding_value (lex_env_p, arg_name_p, true);
  ecma_deref_ecma_string (arg_name_p);

  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (value));

  ecma_named_data_property_assign_value (object_p, ECMA_PROPERTY_VALUE_PTR (property_p), value);
  ecma_free_value (value);

  /* These properties cannot be cached. This is a temporary
   * workaround until the property management is fully rewritten. */
  if (ecma_is_property_lcached (property_p))
  {
    ecma_lcache_invalidate (object_p, property_name_p, property_p);
  }
} /* ecma_arguments_update_mapped_arg_value */

/**
 * [[DefineOwnProperty]] ecma Arguments object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 10.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_arguments_object_define_own_property (ecma_object_t *obj_p, /**< the object */
                                              ecma_string_t *property_name_p, /**< property name */
                                              const ecma_property_descriptor_t *property_desc_p, /**< property
                                                                                                  *   descriptor */
                                              bool is_throw) /**< flag that controls failure handling */
{
  // 1.
  ecma_value_t *map_prop_p = ecma_get_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_PARAMETERS_MAP);
  ecma_object_t *map_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t, *map_prop_p);

  // 3.
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_TRY_CATCH (defined,
                  ecma_op_general_object_define_own_property (obj_p,
                                                              property_name_p,
                                                              property_desc_p,
                                                              is_throw),
                  ret_value);

  // 5.
  if (ecma_op_object_has_own_property (map_p, property_name_p))
  {
    // a.
    if (property_desc_p->is_get_defined
        || property_desc_p->is_set_defined)
    {
      ecma_value_t completion = ecma_op_object_delete (map_p, property_name_p, false);

      JERRY_ASSERT (ecma_is_value_true (completion));

      // 6.
      ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
    }
    else
    {
      // b.

      ecma_value_t completion = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

      // i.
      if (property_desc_p->is_value_defined)
      {
        /* emulating execution of function described by MakeArgSetter */
        ecma_value_t *scope_prop_p = ecma_get_internal_property (map_p, ECMA_INTERNAL_PROPERTY_SCOPE);
        ecma_object_t *lex_env_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t, *scope_prop_p);

        ecma_property_value_t *arg_name_value_p = ecma_get_named_data_property (map_p, property_name_p);

        completion = ecma_op_set_mutable_binding (lex_env_p,
                                                  ecma_get_string_from_value (arg_name_value_p->value),
                                                  property_desc_p->value,
                                                  true);

        JERRY_ASSERT (ecma_is_value_empty (completion));
      }

      // ii.
      if (property_desc_p->is_writable_defined
          && !property_desc_p->is_writable)
      {
        completion = ecma_op_object_delete (map_p,
                                            property_name_p,
                                            false);

        JERRY_ASSERT (ecma_is_value_true (completion));
      }

      // 6.
      ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
    }
  }
  else
  {
    ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
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
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_arguments_object_delete (ecma_object_t *obj_p, /**< the object */
                                 ecma_string_t *property_name_p, /**< property name */
                                 bool is_throw) /**< flag that controls failure handling */
{
  // 1.
  ecma_value_t *map_prop_p = ecma_get_internal_property (obj_p, ECMA_INTERNAL_PROPERTY_PARAMETERS_MAP);
  ecma_object_t *map_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t, *map_prop_p);

  // 3.
  ecma_value_t ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);

  ECMA_TRY_CATCH (delete_in_args_ret,
                  ecma_op_general_object_delete (obj_p,
                                                 property_name_p,
                                                 is_throw),
                  ret_value);

  if (ecma_is_value_true (delete_in_args_ret))
  {
    if (ecma_op_object_has_own_property (map_p, property_name_p))
    {
      ecma_value_t delete_in_map_completion = ecma_op_object_delete (map_p, property_name_p, false);
      JERRY_ASSERT (ecma_is_value_true (delete_in_map_completion));
    }

    ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_boolean (delete_in_args_ret));

    ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE);
  }

  ECMA_FINALIZE (delete_in_args_ret);

  return ret_value;
} /* ecma_op_arguments_object_delete */

/**
 * @}
 * @}
 */
