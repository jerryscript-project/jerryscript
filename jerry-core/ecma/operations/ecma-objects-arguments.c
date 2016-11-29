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
  bool is_strict = (bytecode_data_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE) != 0;

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

  ecma_object_t *prototype_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);

  ecma_object_t *obj_p;

  if (!is_strict && arguments_number > 0 && formal_params_number > 0)
  {
    size_t formal_params_size = formal_params_number * sizeof (jmem_cpointer_t);

    obj_p = ecma_create_object (prototype_p,
                                sizeof (ecma_extended_object_t) + formal_params_size,
                                ECMA_OBJECT_TYPE_ARGUMENTS);

    ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

    ECMA_SET_INTERNAL_VALUE_POINTER (ext_object_p->u.arguments.lex_env_cp, lex_env_p);

    ext_object_p->u.arguments.length = formal_params_number;

    jmem_cpointer_t *arg_Literal_p = (jmem_cpointer_t *) (ext_object_p + 1);

    memcpy (arg_Literal_p, literal_p, formal_params_size);

    for (ecma_length_t i = 0; i < formal_params_number; i++)
    {
      if (arg_Literal_p[i] != JMEM_CP_NULL)
      {
        ecma_string_t *name_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_string_t, arg_Literal_p[i]);
        ecma_ref_ecma_string (name_p);
      }
    }
  }
  else
  {
    obj_p = ecma_create_object (prototype_p, sizeof (ecma_extended_object_t), ECMA_OBJECT_TYPE_CLASS);

    ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;
    ext_object_p->u.class_prop.class_id = LIT_MAGIC_STRING_ARGUMENTS_UL;
  }

  ecma_deref_object (prototype_p);

  ecma_property_value_t *prop_value_p;

  /* 11.a, 11.b */
  for (ecma_length_t indx = 0;
       indx < arguments_number;
       indx++)
  {
    ecma_string_t *indx_string_p = ecma_new_ecma_string_from_uint32 (indx);

    prop_value_p = ecma_create_named_data_property (obj_p,
                                                    indx_string_p,
                                                    ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                    NULL);

    prop_value_p->value = ecma_copy_value_if_not_object (arguments_list_p[indx]);

    ecma_deref_ecma_string (indx_string_p);
  }

  /* 7. */
  ecma_string_t *length_magic_string_p = ecma_new_ecma_length_string ();

  prop_value_p = ecma_create_named_data_property (obj_p,
                                                  length_magic_string_p,
                                                  ECMA_PROPERTY_CONFIGURABLE_WRITABLE,
                                                  NULL);

  prop_value_p->value = ecma_make_uint32_value (arguments_number);

  ecma_deref_ecma_string (length_magic_string_p);

  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();

  /* 13. */
  if (!is_strict)
  {
    ecma_string_t *callee_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_CALLEE);

    prop_value_p = ecma_create_named_data_property (obj_p,
                                                    callee_magic_string_p,
                                                    ECMA_PROPERTY_CONFIGURABLE_WRITABLE,
                                                    NULL);

    prop_value_p->value = ecma_make_object_value (func_obj_p);

    ecma_deref_ecma_string (callee_magic_string_p);
  }
  else
  {
    ecma_object_t *thrower_p = ecma_builtin_get (ECMA_BUILTIN_ID_TYPE_ERROR_THROWER);

    /* 14. */
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

    ecma_value_t completion = ecma_op_object_define_own_property (obj_p,
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
ecma_op_arguments_object_define_own_property (ecma_object_t *object_p, /**< the object */
                                              ecma_string_t *property_name_p, /**< property name */
                                              const ecma_property_descriptor_t *property_desc_p, /**< property
                                                                                                  *   descriptor */
                                              bool is_throw) /**< flag that controls failure handling */
{
  /* 3. */
  ecma_value_t ret_value = ecma_op_general_object_define_own_property (object_p,
                                                                       property_name_p,
                                                                       property_desc_p,
                                                                       is_throw);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    return ret_value;
  }

  uint32_t index = ecma_string_get_array_index (property_name_p);

  if (index == ECMA_STRING_NOT_ARRAY_INDEX)
  {
    return ret_value;
  }

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

  if (index >= ext_object_p->u.arguments.length)
  {
    return ret_value;
  }

  jmem_cpointer_t *arg_Literal_p = (jmem_cpointer_t *) (ext_object_p + 1);

  if (arg_Literal_p[index] == JMEM_CP_NULL)
  {
    return ret_value;
  }

  ecma_string_t *name_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_string_t, arg_Literal_p[index]);

  if (property_desc_p->is_get_defined
      || property_desc_p->is_set_defined)
  {
    ecma_deref_ecma_string (name_p);
    arg_Literal_p[index] = JMEM_CP_NULL;
  }
  else
  {
    if (property_desc_p->is_value_defined)
    {
      /* emulating execution of function described by MakeArgSetter */
      ecma_object_t *lex_env_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                                  ext_object_p->u.arguments.lex_env_cp);

      ecma_value_t completion = ecma_op_set_mutable_binding (lex_env_p,
                                                             name_p,
                                                             property_desc_p->value,
                                                             true);

      JERRY_ASSERT (ecma_is_value_empty (completion));
    }

    if (property_desc_p->is_writable_defined
        && !property_desc_p->is_writable)
    {
      ecma_deref_ecma_string (name_p);
      arg_Literal_p[index] = JMEM_CP_NULL;
    }
  }

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
ecma_op_arguments_object_delete (ecma_object_t *object_p, /**< the object */
                                 ecma_string_t *property_name_p, /**< property name */
                                 bool is_throw) /**< flag that controls failure handling */
{
  /* 3. */
  ecma_value_t ret_value = ecma_op_general_object_delete (object_p, property_name_p, is_throw);

  if (ECMA_IS_VALUE_ERROR (ret_value))
  {
    return ret_value;
  }

  JERRY_ASSERT (ecma_is_value_boolean (ret_value));

  if (ecma_is_value_true (ret_value))
  {
    uint32_t index = ecma_string_get_array_index (property_name_p);

    if (index != ECMA_STRING_NOT_ARRAY_INDEX)
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

      if (index < ext_object_p->u.arguments.length)
      {
        jmem_cpointer_t *arg_Literal_p = (jmem_cpointer_t *) (ext_object_p + 1);

        if (arg_Literal_p[index] != JMEM_CP_NULL)
        {
          ecma_string_t *name_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_string_t, arg_Literal_p[index]);
          ecma_deref_ecma_string (name_p);
          arg_Literal_p[index] = JMEM_CP_NULL;
        }
      }
    }

    ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
  }

  return ret_value;
} /* ecma_op_arguments_object_delete */

/**
 * @}
 * @}
 */
