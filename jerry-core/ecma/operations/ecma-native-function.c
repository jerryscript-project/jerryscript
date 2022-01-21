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

#include "ecma-native-function.h"

#include "ecma-builtins.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-objects.h"
#include "ecma-ordinary-object.h"

#include "jcontext.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmanativefunction ECMA native function related routines
 * @{
 */

#if JERRY_ESNEXT

/**
 * Create built-in native handler object.
 *
 * @return pointer to newly created native handler object
 */
ecma_object_t *
ecma_native_function_create (ecma_native_handler_id_t id, /**< handler id */
                             size_t object_size) /**< created object size */
{
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE);

  ecma_object_t *function_obj_p = ecma_create_object (prototype_obj_p, object_size, ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION);

  ecma_extended_object_t *ext_func_obj_p = (ecma_extended_object_t *) function_obj_p;
  ext_func_obj_p->u.built_in.id = ECMA_BUILTIN_ID_HANDLER;
  ext_func_obj_p->u.built_in.routine_id = (uint8_t) id;
  ext_func_obj_p->u.built_in.u2.routine_flags = ECMA_NATIVE_HANDLER_FLAGS_NONE;

#if JERRY_BUILTIN_REALMS
  ECMA_SET_INTERNAL_VALUE_POINTER (ext_func_obj_p->u.built_in.realm_value, ecma_builtin_get_global ());
#endif /* JERRY_BUILTIN_REALMS */

  return function_obj_p;
} /* ecma_native_function_create */

#endif /* JERRY_ESNEXT */

/**
 * ecma native function object's [[GetOwnProperty]] internal method
 *
 * @return ecma property descriptor t
 */
ecma_property_descriptor_t
ecma_native_function_get_own_property (ecma_object_t *obj_p, /**< the object */
                                       ecma_string_t *property_name_p) /**< property name */
{
  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();

  prop_desc.u.property_p = ecma_find_named_property (obj_p, property_name_p);

  if (prop_desc.u.property_p != NULL)
  {
    prop_desc.flags =
      ECMA_PROP_DESC_PROPERTY_FOUND | ECMA_PROPERTY_TO_PROPERTY_DESCRIPTOR_FLAGS (prop_desc.u.property_p);
    return prop_desc;
  }

  if (ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_PROTOTYPE))
  {
#if JERRY_BUILTIN_REALMS
    ecma_global_object_t *global_object_p =
      ECMA_GET_INTERNAL_VALUE_POINTER (ecma_global_object_t, ((ecma_native_function_t *) obj_p)->realm_value);
    ecma_object_t *prototype_p = ecma_builtin_get_from_realm (global_object_p, ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);
#else /* !JERRY_BUILTIN_REALMS */
    ecma_object_t *prototype_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);
#endif /* JERRY_BUILTIN_REALMS */

    ecma_object_t *proto_object_p = ecma_create_object (prototype_p, 0, ECMA_OBJECT_TYPE_GENERAL);

    ecma_property_value_t *constructor_prop_value_p;
    constructor_prop_value_p = ecma_create_named_data_property (proto_object_p,
                                                                ecma_get_magic_string (LIT_MAGIC_STRING_CONSTRUCTOR),
                                                                ECMA_PROPERTY_CONFIGURABLE_WRITABLE,
                                                                NULL);

    constructor_prop_value_p->value = ecma_make_object_value (obj_p);

    prop_desc.flags = ECMA_PROP_DESC_PROPERTY_FOUND | ECMA_PROP_DESC_DATA_WRITABLE;
    ecma_property_value_t *prototype_prop_value_p;
    prototype_prop_value_p = ecma_create_named_data_property (obj_p,
                                                              ecma_get_magic_string (LIT_MAGIC_STRING_PROTOTYPE),
                                                              ECMA_PROPERTY_BUILT_IN_WRITABLE,
                                                              &prop_desc.u.property_p);

    prototype_prop_value_p->value = ecma_make_object_value (proto_object_p);

    ecma_deref_object (proto_object_p);
    return prop_desc;
  }

  return prop_desc;
} /* ecma_native_function_get_own_property */

/**
 * List lazy instantiated property names for native function object's
 */
void
ecma_native_object_list_lazy_property_keys (ecma_object_t *object_p, /**< function object */
                                            ecma_collection_t *prop_names_p, /**< prop name collection */
                                            ecma_property_counter_t *prop_counter_p, /**< property counters */
                                            jerry_property_filter_t filter) /**< property name
                                                                             *   filter options */
{
  JERRY_UNUSED (object_p);

  if (filter & JERRY_PROPERTY_FILTER_EXCLUDE_STRINGS)
  {
    return;
  }

  /* 'prototype' property is non-enumerable (ECMA-262 v5, 13.2.18) */
  ecma_collection_push_back (prop_names_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_PROTOTYPE));
  prop_counter_p->string_named_props++;
} /* ecma_native_object_list_lazy_property_keys */

/**
 * Helper function to invoke native function with the prepaired jerry call info
 *
 * @return the result of the function call.
 */
static ecma_value_t
ecma_native_function_call_helper (ecma_object_t *func_obj_p, /**< Function object */
                                  jerry_call_info_t *call_info_p, /**< Function object */
                                  const ecma_value_t *arguments_list_p, /**< arguments list */
                                  uint32_t arguments_list_len) /**< length of arguments list */
{
  ecma_native_function_t *native_function_p = (ecma_native_function_t *) func_obj_p;

#if JERRY_BUILTIN_REALMS
  ecma_global_object_t *saved_global_object_p = JERRY_CONTEXT (global_object_p);
  JERRY_CONTEXT (global_object_p) =
    ECMA_GET_INTERNAL_VALUE_POINTER (ecma_global_object_t, native_function_p->realm_value);
#endif /* JERRY_BUILTIN_REALMS */

  call_info_p->function = ecma_make_object_value (func_obj_p);

  JERRY_ASSERT (native_function_p->native_handler_cb != NULL);
  ecma_value_t ret_value = native_function_p->native_handler_cb (call_info_p, arguments_list_p, arguments_list_len);
#if JERRY_BUILTIN_REALMS
  JERRY_CONTEXT (global_object_p) = saved_global_object_p;
#endif /* JERRY_BUILTIN_REALMS */

  if (JERRY_UNLIKELY (ecma_is_value_exception (ret_value)))
  {
    ecma_throw_exception (ret_value);
    return ECMA_VALUE_ERROR;
  }

#if JERRY_DEBUGGER
  JERRY_DEBUGGER_CLEAR_FLAGS (JERRY_DEBUGGER_VM_EXCEPTION_THROWN);
#endif /* JERRY_DEBUGGER */
  return ret_value;
} /* ecma_native_function_call_helper */

/**
 * Perform a native C method call which was registered via the API.
 *
 * @return the result of the function call.
 */
ecma_value_t
ecma_native_function_call (ecma_object_t *func_obj_p, /**< Function object */
                           ecma_value_t this_arg_value, /**< 'this' argument's value */
                           const ecma_value_t *arguments_list_p, /**< arguments list */
                           uint32_t arguments_list_len) /**< length of arguments list */
{
  jerry_call_info_t call_info;
  call_info.this_value = this_arg_value;
  call_info.new_target = ECMA_VALUE_UNDEFINED;

  return ecma_native_function_call_helper (func_obj_p, &call_info, arguments_list_p, arguments_list_len);
} /* ecma_native_function_call */

/**
 * ecma native function object's [[Construct]] internal method
 *
 * @return ecma value t
 */
ecma_value_t
ecma_native_function_construct (ecma_object_t *func_obj_p, /**< Function object */
                                ecma_object_t *new_target_p, /**< new target */
                                const ecma_value_t *arguments_list_p, /**< arguments list */
                                uint32_t arguments_list_len) /**< length of arguments list */
{
  JERRY_ASSERT (ecma_get_object_type (func_obj_p) == ECMA_OBJECT_TYPE_NATIVE_FUNCTION);

  ecma_object_t *proto_p = ecma_op_get_prototype_from_constructor (new_target_p, ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);

  if (JERRY_UNLIKELY (proto_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_object_t *new_this_obj_p = ecma_create_object (proto_p, 0, ECMA_OBJECT_TYPE_GENERAL);
  ecma_value_t this_arg = ecma_make_object_value (new_this_obj_p);
  ecma_deref_object (proto_p);

  jerry_call_info_t call_info;
  call_info.this_value = this_arg;

#if JERRY_ESNEXT
  ecma_object_t *old_new_target_p = JERRY_CONTEXT (current_new_target_p);
  JERRY_CONTEXT (current_new_target_p) = new_target_p;
  call_info.new_target = ecma_make_object_value (new_target_p);
#else /* JERRY_ESNEXT */
  call_info.new_target = ECMA_VALUE_UNDEFINED;
#endif /* JERRY_ESNEXT */

  ecma_value_t ret_value =
    ecma_native_function_call_helper (func_obj_p, &call_info, arguments_list_p, arguments_list_len);

#if JERRY_ESNEXT
  JERRY_CONTEXT (current_new_target_p) = old_new_target_p;
#endif /* JERRY_ESNEXT */

  if (ECMA_IS_VALUE_ERROR (ret_value) || ecma_is_value_object (ret_value))
  {
    ecma_deref_object (new_this_obj_p);
    return ret_value;
  }

  ecma_free_value (ret_value);

  return this_arg;
} /* ecma_native_function_construct */

/**
 * @}
 * @}
 */
