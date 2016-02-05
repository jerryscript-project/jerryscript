/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2015-2016 University of Szeged.
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
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"
#include "opcodes.h"
#include "vm-defines.h"

/** \addtogroup vm Virtual machine
 * @{
 *
 * \addtogroup vm_opcodes Opcodes
 * @{
 */

/**
 * 'Function call' opcode handler.
 *
 * See also: ECMA-262 v5, 11.2.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_call_n (ecma_value_t this_value, /**< this object value */
               ecma_value_t func_value, /**< function object value */
               const ecma_value_t *arguments_list_p, /**< stack pointer */
               ecma_length_t arguments_list_len) /**< number of arguments */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (!ecma_op_is_callable (func_value))
  {
    return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }

  ecma_object_t *func_obj_p = ecma_get_object_from_value (func_value);

  ret_value = ecma_op_function_call_array_args (func_obj_p,
                                                this_value,
                                                arguments_list_p,
                                                arguments_list_len);

  return ret_value;
} /* opfunc_call_n */

/**
 * 'Constructor call' opcode handler.
 *
 * See also: ECMA-262 v5, 11.2.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_construct_n (ecma_value_t constructor_value, /**< constructor object value */
                    uint8_t args_num, /**< number of arguments */
                    ecma_value_t *stack_p) /**< stack pointer */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();
  ecma_collection_header_t *arg_collection_p = ecma_new_values_collection (NULL, 0, true);

  for (int i = 0; i < args_num; i++)
  {
    ecma_append_to_values_collection (arg_collection_p, stack_p[i], true);
  }

  if (!ecma_is_constructor (constructor_value))
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    ecma_object_t *constructor_obj_p = ecma_get_object_from_value (constructor_value);

    ECMA_TRY_CATCH (construction_ret_value,
                    ecma_op_function_construct (constructor_obj_p,
                                                arg_collection_p),
                    ret_value);

    ret_value = ecma_make_normal_completion_value (ecma_copy_value (construction_ret_value, true));

    ECMA_FINALIZE (construction_ret_value);
  }

  ecma_free_values_collection (arg_collection_p, true);

  return ret_value;
} /* opfunc_construct_n */

/**
 * 'Variable declaration' opcode handler.
 *
 * See also: ECMA-262 v5, 10.5 - Declaration binding instantiation (block 8).
 *
 * @return completion value
 *         Returned value is simple and so need not be freed.
 *         However, ecma_free_completion_value may be called for it, but it is a no-op.
 */
ecma_completion_value_t
vm_var_decl (vm_frame_ctx_t *frame_ctx_p, /**< interpreter context */
             ecma_string_t *var_name_str_p) /**< variable name */
{
  if (!ecma_op_has_binding (frame_ctx_p->lex_env_p, var_name_str_p))
  {
    const bool is_configurable_bindings = frame_ctx_p->is_eval_code;

    ecma_completion_value_t completion = ecma_op_create_mutable_binding (frame_ctx_p->lex_env_p,
                                                                         var_name_str_p,
                                                                         is_configurable_bindings);

    JERRY_ASSERT (ecma_is_completion_value_empty (completion));

    /* Skipping SetMutableBinding as we have already checked that there were not
     * any binding with specified name in current lexical environment
     * and CreateMutableBinding sets the created binding's value to undefined */
    JERRY_ASSERT (ecma_is_completion_value_normal_simple_value (ecma_op_get_binding_value (frame_ctx_p->lex_env_p,
                                                                                           var_name_str_p,
                                                                                           true),
                                                                ECMA_SIMPLE_VALUE_UNDEFINED));
  }
  return ecma_make_empty_completion_value ();
} /* vm_var_decl */

/**
 * 'Logical NOT Operator' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4.9
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_logical_not (ecma_value_t left_value) /**< left value */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ecma_simple_value_t old_value = ECMA_SIMPLE_VALUE_TRUE;
  ecma_completion_value_t to_bool_value = ecma_op_to_boolean (left_value);

  if (ecma_is_value_true (ecma_get_completion_value_value (to_bool_value)))
  {
    old_value = ECMA_SIMPLE_VALUE_FALSE;
  }

  ret_value = ecma_make_simple_completion_value (old_value);

  return ret_value;
} /* opfunc_logical_not */

/**
 * 'typeof' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_typeof (ecma_value_t left_value) /**< left value */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ecma_string_t *type_str_p = NULL;

  if (ecma_is_value_undefined (left_value))
  {
    type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_UNDEFINED);
  }
  else if (ecma_is_value_null (left_value))
  {
    type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_OBJECT);
  }
  else if (ecma_is_value_boolean (left_value))
  {
    type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_BOOLEAN);
  }
  else if (ecma_is_value_number (left_value))
  {
    type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_NUMBER);
  }
  else if (ecma_is_value_string (left_value))
  {
    type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_STRING);
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_object (left_value));

    if (ecma_op_is_callable (left_value))
    {
      type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_FUNCTION);
    }
    else
    {
      type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_OBJECT);
    }
  }

  ret_value = ecma_make_normal_completion_value (ecma_make_string_value (type_str_p));

  return ret_value;
} /* opfunc_typeof */

/**
 * Update getter or setter for object literals.
 */
void
opfunc_set_accessor (bool is_getter, /**< is getter accessor */
                     ecma_value_t object, /**< object value */
                     ecma_value_t accessor_name, /**< accessor name value */
                     ecma_value_t accessor) /**< accessor value */
{
  ecma_object_t *object_p = ecma_get_object_from_value (object);
  ecma_string_t *accessor_name_p = ecma_get_string_from_value (accessor_name);
  ecma_property_t *property_p = ecma_find_named_property (object_p, accessor_name_p);

  if (property_p != NULL && property_p->type != ECMA_PROPERTY_NAMEDACCESSOR)
  {
    ecma_delete_property (object_p, property_p);
    property_p = NULL;
  }

  if (property_p == NULL)
  {
    ecma_object_t *getter_func_p = NULL;
    ecma_object_t *setter_func_p = NULL;

    if (is_getter)
    {
      getter_func_p = ecma_get_object_from_value (accessor);
    }
    else
    {
      setter_func_p = ecma_get_object_from_value (accessor);
    }

    ecma_create_named_accessor_property (object_p,
                                         accessor_name_p,
                                         getter_func_p,
                                         setter_func_p,
                                         true,
                                         true);
  }
  else if (is_getter)
  {
    ecma_object_t *getter_func_p = ecma_get_object_from_value (accessor);

    ecma_set_named_accessor_property_getter (object_p,
                                             property_p,
                                             getter_func_p);
  }
  else
  {
    ecma_object_t *setter_func_p = ecma_get_object_from_value (accessor);

    ecma_set_named_accessor_property_setter (object_p,
                                             property_p,
                                             setter_func_p);
  }
} /* opfunc_set_accessor */

/**
 * Deletes an object property.
 *
 * @return completion value
 */
ecma_completion_value_t
vm_op_delete_prop (ecma_value_t object, /**< base object */
                   ecma_value_t property, /**< property name */
                   bool is_strict) /**< strict mode */
{
  ecma_completion_value_t completion_value = ecma_make_empty_completion_value ();

  if (ecma_is_value_undefined (object))
  {
    completion_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
  }
  else
  {
    completion_value = ecma_make_empty_completion_value ();

    ECMA_TRY_CATCH (check_coercible_ret,
                    ecma_op_check_object_coercible (object),
                    completion_value);
    ECMA_TRY_CATCH (str_name_value,
                    ecma_op_to_string (property),
                    completion_value);

    JERRY_ASSERT (ecma_is_value_string (str_name_value));
    ecma_string_t *name_string_p = ecma_get_string_from_value (str_name_value);

    ECMA_TRY_CATCH (obj_value, ecma_op_to_object (object), completion_value);

    JERRY_ASSERT (ecma_is_value_object (obj_value));
    ecma_object_t *obj_p = ecma_get_object_from_value (obj_value);
    JERRY_ASSERT (!ecma_is_lexical_environment (obj_p));

    ECMA_TRY_CATCH (delete_op_ret_val,
                    ecma_op_object_delete (obj_p, name_string_p, is_strict),
                    completion_value);

    completion_value = ecma_make_normal_completion_value (delete_op_ret_val);

    ECMA_FINALIZE (delete_op_ret_val);
    ECMA_FINALIZE (obj_value);
    ECMA_FINALIZE (str_name_value);
    ECMA_FINALIZE (check_coercible_ret);
  }

  return completion_value;
} /* vm_op_delete_prop */

/**
 * Deletes a variable.
 *
 * @return completion value
 */
ecma_completion_value_t
vm_op_delete_var (lit_cpointer_t name_literal, /**< name literal */
                  ecma_object_t *lex_env_p, /**< lexical environment */
                  bool is_strict) /**< strict mode */
{
  ecma_completion_value_t completion_value = ecma_make_empty_completion_value ();

  ecma_string_t *var_name_str_p;

  var_name_str_p = ecma_new_ecma_string_from_lit_cp (name_literal);
  ecma_reference_t ref = ecma_op_get_identifier_reference (lex_env_p,
                                                           var_name_str_p,
                                                           is_strict);

  JERRY_ASSERT (!ref.is_strict);

  if (ecma_is_value_undefined (ref.base))
  {
    completion_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
  }
  else
  {
    ecma_object_t *ref_base_lex_env_p = ecma_op_resolve_reference_base (lex_env_p, var_name_str_p);

    JERRY_ASSERT (ecma_is_lexical_environment (ref_base_lex_env_p));

    ECMA_TRY_CATCH (delete_op_ret_val,
                    ecma_op_delete_binding (ref_base_lex_env_p,
                                            ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                                       ref.referenced_name_cp)),
                    completion_value);

    completion_value = ecma_make_normal_completion_value (delete_op_ret_val);

    ECMA_FINALIZE (delete_op_ret_val);

  }

  ecma_free_reference (ref);
  ecma_deref_ecma_string (var_name_str_p);

  return completion_value;
} /* vm_op_delete_var */

/**
 * 'for-in' opcode handler
 *
 * See also:
 *          ECMA-262 v5, 12.6.4
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_collection_header_t *
opfunc_for_in (ecma_value_t left_value, /**< left value */
               ecma_value_t *result_obj_p) /**< expression object */
{
  ecma_completion_value_t compl_val = ecma_make_empty_completion_value ();
  ecma_collection_header_t *prop_names_p = NULL;

  /* 3. */
  if (!ecma_is_value_undefined (left_value)
      && !ecma_is_value_null (left_value))
  {
    /* 4. */
    ECMA_TRY_CATCH (obj_expr_value,
                    ecma_op_to_object (left_value),
                    compl_val);

    ecma_object_t *obj_p = ecma_get_object_from_value (obj_expr_value);
    prop_names_p = ecma_op_object_get_property_names (obj_p, false, true, true);

    if (prop_names_p->unit_number != 0)
    {
      ecma_ref_object (obj_p);
      *result_obj_p = ecma_make_object_value (obj_p);
    }
    else
    {
      ecma_dealloc_collection_header (prop_names_p);
      prop_names_p = NULL;
    }

    ECMA_FINALIZE (obj_expr_value);
  }

  JERRY_ASSERT (ecma_is_completion_value_empty (compl_val));

  return prop_names_p;
} /* opfunc_for_in */

/**
 * @}
 * @}
 */
