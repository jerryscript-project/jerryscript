/* Copyright JS Foundation and other contributors, http://js.foundation
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

#include "ecma-constructor-function.h"

#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-lex-env.h"
#include "ecma-objects.h"
#include "ecma-ordinary-object.h"

#include "jcontext.h"
#include "opcodes.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaconstructorfunctionobject Ecma constructor function object related routines
 * @{
 */

#if JERRY_ESNEXT
/**
 * ecma class implicit constructor object's [[Call]] internal method
 *
 * @return ecma value
 */
ecma_value_t
ecma_constructor_function_call (ecma_object_t *func_obj_p, /**< Function object */
                                ecma_value_t this_value, /**< new target */
                                const ecma_value_t *arguments_list_p, /**< arguments list */
                                uint32_t arguments_list_len) /**< length of arguments list */
{
  JERRY_UNUSED_4 (func_obj_p, this_value, arguments_list_p, arguments_list_len);

  return ecma_raise_type_error (ECMA_ERR_CLASS_CONSTRUCTOR_NEW);
} /* ecma_constructor_function_call */

/**
 * ecma class implicit constructor object's [[Construct]] internal method
 *
 * @return ecma value
 */
ecma_value_t
ecma_constructor_function_construct (ecma_object_t *func_obj_p, /**< Function object */
                                     ecma_object_t *new_target_p, /**< new target */
                                     const ecma_value_t *arguments_list_p, /**< arguments list */
                                     uint32_t arguments_list_len) /**< length of arguments list */
{
  ecma_extended_object_t *constructor_object_p = (ecma_extended_object_t *) func_obj_p;

  if (!(constructor_object_p->u.constructor_function.flags & ECMA_CONSTRUCTOR_FUNCTION_HAS_HERITAGE))
  {
    ecma_object_t *proto_p = ecma_op_get_prototype_from_constructor (new_target_p, ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);

    if (JERRY_UNLIKELY (proto_p == NULL))
    {
      return ECMA_VALUE_ERROR;
    }

    ecma_object_t *new_this_object_p = ecma_create_object (proto_p, 0, ECMA_OBJECT_TYPE_GENERAL);
    ecma_deref_object (proto_p);

    ecma_value_t new_this_value = ecma_make_object_value (new_this_object_p);
    ecma_value_t ret_value = opfunc_init_class_fields (func_obj_p, new_this_value);

    if (ECMA_IS_VALUE_ERROR (ret_value))
    {
      ecma_deref_object (new_this_object_p);
      return ret_value;
    }

    return new_this_value;
  }

  ecma_value_t super_ctor = ecma_op_function_get_super_constructor (func_obj_p);

  if (ECMA_IS_VALUE_ERROR (super_ctor))
  {
    return super_ctor;
  }

  ecma_object_t *super_ctor_p = ecma_get_object_from_value (super_ctor);
  ecma_value_t result =
    ecma_internal_method_construct (super_ctor_p, new_target_p, arguments_list_p, arguments_list_len);
  ecma_deref_object (super_ctor_p);

  if (ecma_is_value_object (result))
  {
    ecma_value_t fields_value = opfunc_init_class_fields (func_obj_p, result);

    if (ECMA_IS_VALUE_ERROR (fields_value))
    {
      ecma_free_value (result);
      return fields_value;
    }
  }

  return result;
} /* ecma_constructor_function_construct */

/**
 * Perform a JavaScript class function object method call.
 *
 * The input function object should be a JavaScript class constructor
 *
 * @return the result of the function call.
 */
ecma_value_t JERRY_ATTR_NOINLINE
ecma_function_object_construct_constructor (ecma_object_t *func_obj_p, /**< Function object */
                                            ecma_object_t *new_target_p, /**< new target */
                                            const ecma_value_t *arguments_list_p, /**< arguments list */
                                            uint32_t arguments_list_len) /**< length of arguments list */
{
  ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) func_obj_p;
  ecma_object_t *scope_p = ECMA_GET_NON_NULL_POINTER_FROM_POINTER_TAG (ecma_object_t, ext_func_p->u.function.scope_cp);
  const ecma_compiled_code_t *bytecode_data_p = ecma_op_function_get_compiled_code (ext_func_p);
  uint16_t status_flags = bytecode_data_p->status_flags;

  JERRY_ASSERT (CBC_FUNCTION_GET_TYPE (status_flags) == CBC_FUNCTION_CONSTRUCTOR);

  ecma_value_t this_value;

  if (ECMA_GET_THIRD_BIT_FROM_POINTER_TAG (ext_func_p->u.function.scope_cp))
  {
    this_value = ECMA_VALUE_UNINITIALIZED;
  }
  else
  {
    ecma_object_t *proto_p = ecma_op_get_prototype_from_constructor (new_target_p, ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);

    if (JERRY_UNLIKELY (proto_p == NULL))
    {
      return ECMA_VALUE_ERROR;
    }

    ecma_object_t *new_this_obj_p = ecma_create_object (proto_p, 0, ECMA_OBJECT_TYPE_GENERAL);
    ecma_deref_object (proto_p);
    this_value = ecma_make_object_value (new_this_obj_p);
  }

  if (!(status_flags & CBC_CODE_FLAGS_LEXICAL_ENV_NOT_NEEDED))
  {
    scope_p = ecma_create_decl_lex_env (scope_p);
  }

  ecma_op_create_environment_record (scope_p, this_value, func_obj_p);

  ecma_object_t *old_new_target_p = JERRY_CONTEXT (current_new_target_p);
  JERRY_CONTEXT (current_new_target_p) = new_target_p;

#if JERRY_BUILTIN_REALMS
  ecma_global_object_t *realm_p = ecma_op_function_get_realm (bytecode_data_p);
  ecma_global_object_t *saved_global_object_p = JERRY_CONTEXT (global_object_p);
  JERRY_CONTEXT (global_object_p) = realm_p;
#endif /* !JERRY_BUILTIN_REALMS */

  vm_frame_ctx_shared_args_t shared_args;
  shared_args.header.status_flags = VM_FRAME_CTX_SHARED_HAS_ARG_LIST | VM_FRAME_CTX_SHARED_NON_ARROW_FUNC;
  shared_args.header.function_object_p = func_obj_p;
  shared_args.arg_list_p = arguments_list_p;
  shared_args.arg_list_len = arguments_list_len;
  shared_args.header.bytecode_header_p = bytecode_data_p;

  ecma_value_t result = vm_run (&shared_args.header, this_value, scope_p);

#if JERRY_BUILTIN_REALMS
  JERRY_CONTEXT (global_object_p) = saved_global_object_p;
#endif /* JERRY_BUILTIN_REALMS */

  JERRY_CONTEXT (current_new_target_p) = old_new_target_p;

  /* ECMAScript v6, 9.2.2.13 */
  if (JERRY_UNLIKELY (this_value == ECMA_VALUE_UNINITIALIZED))
  {
    if (!ECMA_IS_VALUE_ERROR (result) && !ecma_is_value_object (result))
    {
      if (!ecma_is_value_undefined (result))
      {
        ecma_free_value (result);
        result = ecma_raise_type_error (ECMA_ERR_DERIVED_CTOR_RETURN_NOR_OBJECT_OR_UNDEFINED);
      }
      else
      {
        result = ecma_op_get_this_binding (scope_p);
      }
    }
  }

  if (!(status_flags & CBC_CODE_FLAGS_LEXICAL_ENV_NOT_NEEDED))
  {
    ecma_deref_object (scope_p);
  }

  if (ECMA_IS_VALUE_ERROR (result) || ecma_is_value_object (result))
  {
    ecma_free_value (this_value);
    return result;
  }

  ecma_free_value (result);
  return this_value;
} /* ecma_function_object_construct_constructor */

#endif /* JERRY_ESNEXT */

/**
 * @}
 * @}
 */
