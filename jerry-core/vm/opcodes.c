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

#include "ecma-alloc.h"
#include "ecma-array-object.h"
#include "ecma-builtins.h"
#include "ecma-builtin-helpers.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-iterator-object.h"
#include "ecma-lex-env.h"
#include "ecma-objects.h"
#include "ecma-promise-object.h"
#include "ecma-proxy-object.h"
#include "ecma-try-catch-macro.h"
#include "jcontext.h"
#include "opcodes.h"
#include "vm-defines.h"
#include "vm-stack.h"

/** \addtogroup vm Virtual machine
 * @{
 *
 * \addtogroup vm_opcodes Opcodes
 * @{
 */

/**
 * 'Variable declaration' opcode handler.
 *
 * See also: ECMA-262 v5, 10.5 - Declaration binding instantiation (block 8).
 *
 * @return ECMA_VALUE_ERROR - if no the operation fails
 *         ECMA_VALUE_EMPTY - otherwise
 */
inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
vm_var_decl (ecma_object_t *lex_env_p, /**< target lexical environment */
             ecma_string_t *var_name_str_p, /**< variable name */
             bool is_configurable_bindings) /**< true if the binding can be deleted */
{
  ecma_value_t has_binding = ecma_op_has_binding (lex_env_p, var_name_str_p);

#if ENABLED (JERRY_BUILTIN_PROXY)
  if (ECMA_IS_VALUE_ERROR (has_binding))
  {
    return has_binding;
  }
#endif /* ENABLED (JERRY_BUILTIN_PROXY) */

  if (ecma_is_value_false (has_binding))
  {
    ecma_value_t completion_value = ecma_op_create_mutable_binding (lex_env_p,
                                                                    var_name_str_p,
                                                                    is_configurable_bindings);

    JERRY_ASSERT (ecma_is_value_empty (completion_value));

    /* Skipping SetMutableBinding as we have already checked that there were not
     * any binding with specified name in current lexical environment
     * and CreateMutableBinding sets the created binding's value to undefined */
    JERRY_ASSERT (ecma_is_value_undefined (ecma_op_get_binding_value (lex_env_p,
                                                                      var_name_str_p,
                                                                      vm_is_strict_mode ())));
  }

  return ECMA_VALUE_EMPTY;
} /* vm_var_decl */

/**
 * Set var binding to a function literal value.
 *
 * @return ECMA_VALUE_ERROR - if no the operation fails
 *         ECMA_VALUE_EMPTY - otherwise
 */
inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
vm_set_var (ecma_object_t *lex_env_p, /**< target lexical environment */
            ecma_string_t *var_name_str_p, /**< variable name */
            bool is_strict, /**< true, if the engine is in strict mode */
            ecma_value_t lit_value) /**< function value */
{
  ecma_value_t put_value_result;
  put_value_result = ecma_op_put_value_lex_env_base (lex_env_p, var_name_str_p, is_strict, lit_value);

  JERRY_ASSERT (ecma_is_value_boolean (put_value_result)
                || ecma_is_value_empty (put_value_result)
                || ECMA_IS_VALUE_ERROR (put_value_result));

  ecma_free_value (lit_value);

  return put_value_result;
} /* vm_set_var */

/**
 * 'typeof' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
opfunc_typeof (ecma_value_t left_value) /**< left value */
{
  return ecma_make_magic_string_value (ecma_get_typeof_lit_id (left_value));
} /* opfunc_typeof */

/**
 * Update data property for object literals.
 */
void
opfunc_set_data_property (ecma_object_t *object_p, /**< object */
                          ecma_string_t *prop_name_p, /**< data property name */
                          ecma_value_t value) /**< new value */
{
  JERRY_ASSERT (!ecma_op_object_is_fast_array (object_p));

  ecma_property_t *property_p = ecma_find_named_property (object_p, prop_name_p);
  ecma_property_value_t *prop_value_p;

  if (property_p == NULL)
  {
    prop_value_p = ecma_create_named_data_property (object_p,
                                                    prop_name_p,
                                                    ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                    NULL);
  }
  else
  {
    prop_value_p = ECMA_PROPERTY_VALUE_PTR (property_p);

    if (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR)
    {
#if ENABLED (JERRY_CPOINTER_32_BIT)
      ecma_getter_setter_pointers_t *getter_setter_pair_p;
      getter_setter_pair_p = ECMA_GET_NON_NULL_POINTER (ecma_getter_setter_pointers_t,
                                                        ECMA_PROPERTY_VALUE_PTR (property_p)->getter_setter_pair_cp);
      jmem_pools_free (getter_setter_pair_p, sizeof (ecma_getter_setter_pointers_t));
#endif /* ENABLED (JERRY_CPOINTER_32_BIT) */

      ECMA_CHANGE_PROPERTY_TYPE (property_p);
      *property_p = (uint8_t) (*property_p | ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);
      prop_value_p->value = ecma_copy_value_if_not_object (value);
      return;
    }
  }

  ecma_named_data_property_assign_value (object_p, prop_value_p, value);
} /* opfunc_set_data_property */

/**
 * Update getter or setter for object literals.
 */
void
opfunc_set_accessor (bool is_getter, /**< is getter accessor */
                     ecma_value_t object, /**< object value */
                     ecma_string_t *accessor_name_p, /**< accessor name */
                     ecma_value_t accessor) /**< accessor value */
{
  ecma_object_t *object_p = ecma_get_object_from_value (object);

  JERRY_ASSERT (!ecma_op_object_is_fast_array (object_p));

  ecma_property_t *property_p = ecma_find_named_property (object_p, accessor_name_p);
  ecma_object_t *accessor_p = ecma_get_object_from_value (accessor);

  ecma_object_t *getter_func_p = NULL;
  ecma_object_t *setter_func_p = NULL;

  if (is_getter)
  {
    getter_func_p = accessor_p;
  }
  else
  {
    setter_func_p = accessor_p;
  }

  if (property_p == NULL)
  {
    ecma_create_named_accessor_property (object_p,
                                         accessor_name_p,
                                         getter_func_p,
                                         setter_func_p,
                                         ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE,
                                         NULL);
  }
  else
  {
    ecma_property_value_t *prop_value_p = ECMA_PROPERTY_VALUE_PTR (property_p);

    if (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA)
    {
#if ENABLED (JERRY_CPOINTER_32_BIT)
      ecma_getter_setter_pointers_t *getter_setter_pair_p;
      getter_setter_pair_p = jmem_pools_alloc (sizeof (ecma_getter_setter_pointers_t));
#endif /* ENABLED (JERRY_CPOINTER_32_BIT) */

      ecma_free_value_if_not_object (prop_value_p->value);
      ECMA_CHANGE_PROPERTY_TYPE (property_p);
      *property_p = (uint8_t) (*property_p & ~ECMA_PROPERTY_FLAG_WRITABLE);

#if ENABLED (JERRY_CPOINTER_32_BIT)
      ECMA_SET_POINTER (getter_setter_pair_p->getter_cp, getter_func_p);
      ECMA_SET_POINTER (getter_setter_pair_p->setter_cp, setter_func_p);
      ECMA_SET_NON_NULL_POINTER (prop_value_p->getter_setter_pair_cp, getter_setter_pair_p);
#else /* !ENABLED (JERRY_CPOINTER_32_BIT) */
      ECMA_SET_POINTER (prop_value_p->getter_setter_pair.getter_cp, getter_func_p);
      ECMA_SET_POINTER (prop_value_p->getter_setter_pair.setter_cp, setter_func_p);
#endif /* ENABLED (JERRY_CPOINTER_32_BIT) */
      return;
    }

    if (is_getter)
    {
      ecma_set_named_accessor_property_getter (object_p, prop_value_p, accessor_p);
    }
    else
    {
      ecma_set_named_accessor_property_setter (object_p, prop_value_p, accessor_p);
    }
  }
} /* opfunc_set_accessor */

/**
 * Deletes an object property.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
vm_op_delete_prop (ecma_value_t object, /**< base object */
                   ecma_value_t property, /**< property name */
                   bool is_strict) /**< strict mode */
{
#if !ENABLED (JERRY_ESNEXT)
  if (ecma_is_value_undefined (object))
  {
    return ECMA_VALUE_TRUE;
  }
#endif /* !ENABLED (JERRY_ESNEXT) */

  ecma_value_t check_coercible = ecma_op_check_object_coercible (object);
  if (ECMA_IS_VALUE_ERROR (check_coercible))
  {
    return check_coercible;
  }
  JERRY_ASSERT (check_coercible == ECMA_VALUE_EMPTY);

  ecma_string_t *name_string_p = ecma_op_to_property_key (property);

  if (JERRY_UNLIKELY (name_string_p == NULL))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t obj_value = ecma_op_to_object (object);
  /* The ecma_op_check_object_coercible call already checked the op_to_object error cases. */
  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (obj_value));
  JERRY_ASSERT (ecma_is_value_object (obj_value));
  ecma_object_t *obj_p = ecma_get_object_from_value (obj_value);
  JERRY_ASSERT (!ecma_is_lexical_environment (obj_p));

  ecma_value_t delete_op_ret = ecma_op_object_delete (obj_p, name_string_p, is_strict);
  JERRY_ASSERT (ecma_is_value_boolean (delete_op_ret) || ECMA_IS_VALUE_ERROR (delete_op_ret));
  ecma_deref_object (obj_p);
  ecma_deref_ecma_string (name_string_p);

  return delete_op_ret;
} /* vm_op_delete_prop */

/**
 * Deletes a variable.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
vm_op_delete_var (ecma_value_t name_literal, /**< name literal */
                  ecma_object_t *lex_env_p) /**< lexical environment */
{
  ecma_value_t completion_value = ECMA_VALUE_EMPTY;

  ecma_string_t *var_name_str_p = ecma_get_string_from_value (name_literal);

  ecma_object_t *ref_base_lex_env_p = ecma_op_resolve_reference_base (lex_env_p, var_name_str_p);

#if ENABLED (JERRY_BUILTIN_PROXY)
  if (JERRY_UNLIKELY (ref_base_lex_env_p == ECMA_OBJECT_POINTER_ERROR))
  {
    return ECMA_VALUE_ERROR;
  }
#endif /* ENABLED (JERRY_BUILTIN_PROXY) */

  if (ref_base_lex_env_p == NULL)
  {
    completion_value = ECMA_VALUE_TRUE;
  }
  else
  {
    JERRY_ASSERT (ecma_is_lexical_environment (ref_base_lex_env_p));

    completion_value = ecma_op_delete_binding (ref_base_lex_env_p, var_name_str_p);
  }

  return completion_value;
} /* vm_op_delete_var */

/**
 * 'for-in' opcode handler
 *
 * See also:
 *          ECMA-262 v5, 12.6.4
 *
 * @return chain list of property names
 */
ecma_collection_t *
opfunc_for_in (ecma_value_t left_value, /**< left value */
               ecma_value_t *result_obj_p) /**< expression object */
{
  /* 3. */
  if (ecma_is_value_undefined (left_value)
      || ecma_is_value_null (left_value))
  {
    return NULL;
  }

  /* 4. */
  ecma_value_t obj_expr_value = ecma_op_to_object (left_value);
  /* ecma_op_to_object will only raise error on null/undefined values but those are handled above. */
  JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (obj_expr_value));
  ecma_object_t *obj_p = ecma_get_object_from_value (obj_expr_value);
  ecma_collection_t *prop_names_p = ecma_op_object_enumerate (obj_p);

  if (prop_names_p->item_count != 0)
  {
    *result_obj_p = ecma_make_object_value (obj_p);
    return prop_names_p;
  }

  ecma_deref_object (obj_p);
  ecma_collection_destroy (prop_names_p);

  return NULL;
} /* opfunc_for_in */

#if ENABLED (JERRY_ESNEXT)

/**
 * 'VM_OC_APPEND_ARRAY' opcode handler specialized for spread objects
 *
 * @return ECMA_VALUE_ERROR - if the operation failed
 *         ECMA_VALUE_EMPTY, otherwise
 */
static ecma_value_t JERRY_ATTR_NOINLINE
opfunc_append_to_spread_array (ecma_value_t *stack_top_p, /**< current stack top */
                               uint16_t values_length) /**< number of elements to set */
{
  JERRY_ASSERT (!(values_length & OPFUNC_HAS_SPREAD_ELEMENT));

  ecma_object_t *array_obj_p = ecma_get_object_from_value (stack_top_p[-1]);
  JERRY_ASSERT (ecma_get_object_type (array_obj_p) == ECMA_OBJECT_TYPE_ARRAY);

  ecma_extended_object_t *ext_array_obj_p = (ecma_extended_object_t *) array_obj_p;
  uint32_t old_length = ext_array_obj_p->u.array.length;

  for (uint32_t i = 0, idx = old_length; i < values_length; i++, idx++)
  {
    if (ecma_is_value_array_hole (stack_top_p[i]))
    {
      continue;
    }

    if (stack_top_p[i] == ECMA_VALUE_SPREAD_ELEMENT)
    {
      i++;
      ecma_value_t ret_value = ECMA_VALUE_ERROR;
      ecma_value_t spread_value = stack_top_p[i];

      ecma_value_t next_method;
      ecma_value_t iterator = ecma_op_get_iterator (spread_value, ECMA_VALUE_SYNC_ITERATOR, &next_method);

      if (!ECMA_IS_VALUE_ERROR (iterator))
      {
        while (true)
        {
          ecma_value_t next_value = ecma_op_iterator_step (iterator, next_method);

          if (ECMA_IS_VALUE_ERROR (next_value))
          {
            break;
          }

          if (ecma_is_value_false (next_value))
          {
            idx--;
            ret_value = ECMA_VALUE_EMPTY;
            break;
          }

          ecma_value_t value = ecma_op_iterator_value (next_value);

          ecma_free_value (next_value);

          if (ECMA_IS_VALUE_ERROR (value))
          {
            break;
          }

          ecma_value_t put_comp;
          put_comp = ecma_builtin_helper_def_prop_by_index (array_obj_p,
                                                            idx++,
                                                            value,
                                                            ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);

          JERRY_ASSERT (ecma_is_value_true (put_comp));
          ecma_free_value (value);
        }
      }

      ecma_free_value (iterator);
      ecma_free_value (next_method);
      ecma_free_value (spread_value);

      if (ECMA_IS_VALUE_ERROR (ret_value))
      {
        for (uint32_t k = i + 1; k < values_length; k++)
        {
          ecma_free_value (stack_top_p[k]);
        }

        return ret_value;
      }
    }
    else
    {
      ecma_value_t put_comp = ecma_builtin_helper_def_prop_by_index (array_obj_p,
                                                                     idx,
                                                                     stack_top_p[i],
                                                                     ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE);
      JERRY_ASSERT (ecma_is_value_true (put_comp));
      ecma_free_value (stack_top_p[i]);
    }
  }

  return ECMA_VALUE_EMPTY;
} /* opfunc_append_to_spread_array */

/**
 * Spread function call/construct arguments into an ecma-collection
 *
 * @return NULL - if the operation failed
 *         pointer to the ecma-collection with the spreaded arguments, otherwise
 */
JERRY_ATTR_NOINLINE ecma_collection_t *
opfunc_spread_arguments (ecma_value_t *stack_top_p, /**< pointer to the current stack top */
                         uint8_t arguments_list_len) /**< number of arguments */
{
  ecma_collection_t *buff_p = ecma_new_collection ();

  for (uint32_t i = 0; i < arguments_list_len; i++)
  {
    ecma_value_t arg = *stack_top_p++;

    if (arg != ECMA_VALUE_SPREAD_ELEMENT)
    {
      ecma_collection_push_back (buff_p, arg);
      continue;
    }

    ecma_value_t ret_value = ECMA_VALUE_ERROR;
    ecma_value_t spread_value = *stack_top_p++;
    i++;

    ecma_value_t next_method;
    ecma_value_t iterator = ecma_op_get_iterator (spread_value, ECMA_VALUE_SYNC_ITERATOR, &next_method);

    if (!ECMA_IS_VALUE_ERROR (iterator))
    {
      while (true)
      {
        ecma_value_t next_value = ecma_op_iterator_step (iterator, next_method);

        if (ECMA_IS_VALUE_ERROR (next_value))
        {
          break;
        }

        if (ecma_is_value_false (next_value))
        {
          ret_value = ECMA_VALUE_EMPTY;
          break;
        }

        ecma_value_t value = ecma_op_iterator_value (next_value);

        ecma_free_value (next_value);

        if (ECMA_IS_VALUE_ERROR (value))
        {
          break;
        }

        ecma_collection_push_back (buff_p, value);
      }
    }

    ecma_free_value (iterator);
    ecma_free_value (next_method);
    ecma_free_value (spread_value);

    if (ECMA_IS_VALUE_ERROR (ret_value))
    {
      for (uint32_t k = i + 1; k < arguments_list_len; k++)
      {
        ecma_free_value (*stack_top_p++);
      }

      ecma_collection_free (buff_p);
      buff_p = NULL;
      break;
    }
  }

  return buff_p;
} /* opfunc_spread_arguments */

#endif /* ENABLED (JERRY_ESNEXT) */

/**
 * 'VM_OC_APPEND_ARRAY' opcode handler, for setting array object properties
 *
 * @return ECMA_VALUE_ERROR - if the operation failed
 *         ECMA_VALUE_EMPTY, otherwise
 */
ecma_value_t JERRY_ATTR_NOINLINE
opfunc_append_array (ecma_value_t *stack_top_p, /**< current stack top */
                     uint16_t values_length) /**< number of elements to set
                                              *   with potential OPFUNC_HAS_SPREAD_ELEMENT flag */
{
#if ENABLED (JERRY_ESNEXT)
  if (values_length >= OPFUNC_HAS_SPREAD_ELEMENT)
  {
    return opfunc_append_to_spread_array (stack_top_p, (uint16_t) (values_length & ~OPFUNC_HAS_SPREAD_ELEMENT));
  }
#endif /* ENABLED (JERRY_ESNEXT) */

  ecma_object_t *array_obj_p = ecma_get_object_from_value (stack_top_p[-1]);
  JERRY_ASSERT (ecma_get_object_type (array_obj_p) == ECMA_OBJECT_TYPE_ARRAY);

  ecma_extended_object_t *ext_array_obj_p = (ecma_extended_object_t *) array_obj_p;
  uint32_t old_length = ext_array_obj_p->u.array.length;

  if (JERRY_LIKELY (ecma_op_array_is_fast_array (ext_array_obj_p)))
  {
    uint32_t filled_holes = 0;
    ecma_value_t *values_p = ecma_fast_array_extend (array_obj_p, old_length + values_length);

    for (uint32_t i = 0; i < values_length; i++)
    {
      values_p[old_length + i] = stack_top_p[i];

      if (!ecma_is_value_array_hole (stack_top_p[i]))
      {
        filled_holes++;

        if (ecma_is_value_object (stack_top_p[i]))
        {
          ecma_deref_object (ecma_get_object_from_value (stack_top_p[i]));
        }
      }
    }

    ext_array_obj_p->u.array.u.hole_count -= filled_holes * ECMA_FAST_ARRAY_HOLE_ONE;

    if (JERRY_UNLIKELY ((values_length - filled_holes) > ECMA_FAST_ARRAY_MAX_NEW_HOLES_COUNT))
    {
      ecma_fast_array_convert_to_normal (array_obj_p);
    }
  }
  else
  {
    for (uint32_t i = 0; i < values_length; i++)
    {
      if (!ecma_is_value_array_hole (stack_top_p[i]))
      {
        ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (old_length + i);

        ecma_property_value_t *prop_value_p;

        prop_value_p = ecma_create_named_data_property (array_obj_p,
                                                        index_str_p,
                                                        ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                        NULL);

        ecma_deref_ecma_string (index_str_p);
        prop_value_p->value = stack_top_p[i];

        if (ecma_is_value_object (stack_top_p[i]))
        {
          ecma_free_value (stack_top_p[i]);
        }

      }

      ext_array_obj_p->u.array.length = old_length + values_length;
    }
  }

  return ECMA_VALUE_EMPTY;
} /* opfunc_append_array */

#if ENABLED (JERRY_ESNEXT)

/**
 * Create an executable object using the current frame context
 *
 * @return executable object
 */
vm_executable_object_t *
opfunc_create_executable_object (vm_frame_ctx_t *frame_ctx_p, /**< frame context */
                                 vm_create_executable_object_type_t type) /**< executable object type */
{
  const ecma_compiled_code_t *bytecode_header_p = frame_ctx_p->bytecode_header_p;
  size_t size, register_end;

  ecma_bytecode_ref ((ecma_compiled_code_t *) bytecode_header_p);

  if (bytecode_header_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) bytecode_header_p;
    register_end = (size_t) args_p->register_end;
    size = (register_end + (size_t) args_p->stack_limit) * sizeof (ecma_value_t);
  }
  else
  {
    cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) bytecode_header_p;
    register_end = (size_t) args_p->register_end;
    size = (register_end + (size_t) args_p->stack_limit) * sizeof (ecma_value_t);
  }

  size_t total_size = JERRY_ALIGNUP (sizeof (vm_executable_object_t) + size, sizeof (uintptr_t));

  ecma_object_t *proto_p = NULL;
  /* Async function objects are not accessible, so their class_id is not relevant. */
  uint16_t class_id = LIT_MAGIC_STRING_GENERATOR_UL;

  if (type == VM_CREATE_EXECUTABLE_OBJECT_GENERATOR)
  {
    ecma_builtin_id_t default_proto_id = ECMA_BUILTIN_ID_GENERATOR_PROTOTYPE;

    if (CBC_FUNCTION_GET_TYPE (frame_ctx_p->bytecode_header_p->status_flags) == CBC_FUNCTION_ASYNC_GENERATOR)
    {
      default_proto_id = ECMA_BUILTIN_ID_ASYNC_GENERATOR_PROTOTYPE;
      class_id = LIT_MAGIC_STRING_ASYNC_GENERATOR_UL;
    }

    proto_p = ecma_op_get_prototype_from_constructor (JERRY_CONTEXT (current_function_obj_p), default_proto_id);
  }

  ecma_object_t *object_p = ecma_create_object (proto_p,
                                                total_size,
                                                ECMA_OBJECT_TYPE_CLASS);

  vm_executable_object_t *executable_object_p = (vm_executable_object_t *) object_p;

  if (type == VM_CREATE_EXECUTABLE_OBJECT_GENERATOR)
  {
    ecma_deref_object (proto_p);
  }

  executable_object_p->extended_object.u.class_prop.class_id = class_id;
  executable_object_p->extended_object.u.class_prop.extra_info = 0;
  ECMA_SET_INTERNAL_VALUE_ANY_POINTER (executable_object_p->extended_object.u.class_prop.u.head, NULL);

  JERRY_ASSERT (!frame_ctx_p->is_eval_code);

  vm_frame_ctx_t *new_frame_ctx_p = &(executable_object_p->frame_ctx);
  *new_frame_ctx_p = *frame_ctx_p;

  /* The old register values are discarded. */
  ecma_value_t *new_registers_p = VM_GET_REGISTERS (new_frame_ctx_p);
  memcpy (new_registers_p, VM_GET_REGISTERS (frame_ctx_p), size);

  size_t stack_top = (size_t) (frame_ctx_p->stack_top_p - VM_GET_REGISTERS (frame_ctx_p));
  ecma_value_t *new_stack_top_p = new_registers_p + stack_top;

  new_frame_ctx_p->stack_top_p = new_stack_top_p;

  /* Initial state is "not running", so all object references are released. */

  if (frame_ctx_p->context_depth > 0)
  {
    JERRY_ASSERT (type != VM_CREATE_EXECUTABLE_OBJECT_GENERATOR);

    ecma_value_t *register_end_p = new_registers_p + register_end;

    JERRY_ASSERT (register_end_p <= new_stack_top_p);

    while (new_registers_p < register_end_p)
    {
      ecma_deref_if_object (*new_registers_p++);
    }

    vm_ref_lex_env_chain (frame_ctx_p->lex_env_p,
                          frame_ctx_p->context_depth,
                          new_registers_p,
                          false);

    new_registers_p += frame_ctx_p->context_depth;

    JERRY_ASSERT (new_registers_p <= new_stack_top_p);
  }

  while (new_registers_p < new_stack_top_p)
  {
    ecma_deref_if_object (*new_registers_p++);
  }

  JERRY_ASSERT (new_frame_ctx_p->block_result == ECMA_VALUE_UNDEFINED);

  new_frame_ctx_p->this_binding = ecma_copy_value_if_not_object (new_frame_ctx_p->this_binding);

  JERRY_CONTEXT (vm_top_context_p) = new_frame_ctx_p->prev_context_p;

  return executable_object_p;
} /* opfunc_create_executable_object */

/**
 * Byte code which resumes an executable object with throw
 */
const uint8_t opfunc_resume_executable_object_with_throw[1] =
{
  CBC_THROW
};

/**
 * Byte code which resumes an executable object with return
 */
const uint8_t opfunc_resume_executable_object_with_return[2] =
{
  CBC_EXT_OPCODE, CBC_EXT_RETURN
};

/**
 * Resume the execution of an inactive executable object
 *
 * @return value provided by the execution
 */
ecma_value_t
opfunc_resume_executable_object (vm_executable_object_t *executable_object_p, /**< executable object */
                                 ecma_value_t value) /**< value pushed onto the stack (takes the reference) */
{
  const ecma_compiled_code_t *bytecode_header_p = executable_object_p->frame_ctx.bytecode_header_p;
  ecma_value_t *register_p = VM_GET_REGISTERS (&executable_object_p->frame_ctx);
  ecma_value_t *register_end_p;

  if (bytecode_header_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) bytecode_header_p;
    register_end_p = register_p + args_p->register_end;
  }
  else
  {
    cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) bytecode_header_p;
    register_end_p = register_p + args_p->register_end;
  }

  ecma_value_t *stack_top_p = executable_object_p->frame_ctx.stack_top_p;

  if (value != ECMA_VALUE_EMPTY)
  {
    *stack_top_p = value;
    executable_object_p->frame_ctx.stack_top_p = stack_top_p + 1;
  }

  if (executable_object_p->frame_ctx.context_depth > 0)
  {
    while (register_p < register_end_p)
    {
      ecma_ref_if_object (*register_p++);
    }

    vm_ref_lex_env_chain (executable_object_p->frame_ctx.lex_env_p,
                          executable_object_p->frame_ctx.context_depth,
                          register_p,
                          true);

    register_p += executable_object_p->frame_ctx.context_depth;
  }

  while (register_p < stack_top_p)
  {
    ecma_ref_if_object (*register_p++);
  }

  ecma_ref_if_object (executable_object_p->frame_ctx.block_result);

  JERRY_ASSERT (ECMA_EXECUTABLE_OBJECT_IS_SUSPENDED (executable_object_p->extended_object.u.class_prop.extra_info));

  executable_object_p->extended_object.u.class_prop.extra_info |= ECMA_EXECUTABLE_OBJECT_RUNNING;

  executable_object_p->frame_ctx.prev_context_p = JERRY_CONTEXT (vm_top_context_p);
  JERRY_CONTEXT (vm_top_context_p) = &executable_object_p->frame_ctx;

  /* inside the generators the "new.target" is always "undefined" as it can't be invoked with "new" */
  ecma_object_t *old_new_target = JERRY_CONTEXT (current_new_target);
  JERRY_CONTEXT (current_new_target) = NULL;

  ecma_value_t result = vm_execute (&executable_object_p->frame_ctx);

  JERRY_CONTEXT (current_new_target) = old_new_target;
  executable_object_p->extended_object.u.class_prop.extra_info &= (uint16_t) ~ECMA_EXECUTABLE_OBJECT_RUNNING;

  if (executable_object_p->frame_ctx.call_operation != VM_EXEC_RETURN)
  {
    JERRY_ASSERT (executable_object_p->frame_ctx.call_operation == VM_NO_EXEC_OP);

    /* All resources are released. */
    executable_object_p->extended_object.u.class_prop.extra_info |= ECMA_EXECUTABLE_OBJECT_COMPLETED;
    return result;
  }

  JERRY_CONTEXT (vm_top_context_p) = executable_object_p->frame_ctx.prev_context_p;

  register_p = VM_GET_REGISTERS (&executable_object_p->frame_ctx);
  stack_top_p = executable_object_p->frame_ctx.stack_top_p;

  if (executable_object_p->frame_ctx.context_depth > 0)
  {
    while (register_p < register_end_p)
    {
      ecma_deref_if_object (*register_p++);
    }

    vm_ref_lex_env_chain (executable_object_p->frame_ctx.lex_env_p,
                          executable_object_p->frame_ctx.context_depth,
                          register_p,
                          false);

    register_p += executable_object_p->frame_ctx.context_depth;
  }

  while (register_p < stack_top_p)
  {
    ecma_deref_if_object (*register_p++);
  }

  ecma_deref_if_object (executable_object_p->frame_ctx.block_result);

  return result;
} /* opfunc_resume_executable_object */

/**
 * Fulfill the next promise of the async generator with the value
 */
void
opfunc_async_generator_yield (ecma_extended_object_t *async_generator_object_p, /**< async generator object */
                              ecma_value_t value) /**< value (takes the reference) */
{
  ecma_async_generator_task_t *task_p;
  task_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_async_generator_task_t,
                                            async_generator_object_p->u.class_prop.u.head);

  ecma_value_t iter_result = ecma_create_iter_result_object (value, ECMA_VALUE_FALSE);
  ecma_fulfill_promise (task_p->promise, iter_result);

  ecma_free_value (iter_result);
  ecma_free_value (value);

  ecma_value_t next = task_p->next;
  async_generator_object_p->u.class_prop.u.head = next;

  JERRY_ASSERT (task_p->operation_value == ECMA_VALUE_UNDEFINED);
  jmem_heap_free_block (task_p, sizeof (ecma_async_generator_task_t));

  if (!ECMA_IS_INTERNAL_VALUE_NULL (next))
  {
    ecma_value_t executable_object = ecma_make_object_value ((ecma_object_t *) async_generator_object_p);
    ecma_enqueue_promise_async_generator_job (executable_object);
  }
} /* opfunc_async_generator_yield */

/**
 * Creates a new executable object and awaits for the value
 *
 * Note:
 *   extra_flags can be used to set additional extra_info flags
 *
 * @return a new Promise object on success, error otherwise
 */
ecma_value_t
opfunc_async_create_and_await (vm_frame_ctx_t *frame_ctx_p, /**< frame context */
                               ecma_value_t value, /**< awaited value (takes reference) */
                               uint16_t extra_flags) /**< extra flags */
{
  JERRY_ASSERT (frame_ctx_p->block_result == ECMA_VALUE_UNDEFINED);
  JERRY_ASSERT (CBC_FUNCTION_GET_TYPE (frame_ctx_p->bytecode_header_p->status_flags) == CBC_FUNCTION_ASYNC
                || CBC_FUNCTION_GET_TYPE (frame_ctx_p->bytecode_header_p->status_flags) == CBC_FUNCTION_ASYNC_ARROW);

  ecma_object_t *promise_p = ecma_builtin_get (ECMA_BUILTIN_ID_PROMISE);
  ecma_value_t result = ecma_promise_reject_or_resolve (ecma_make_object_value (promise_p), value, true);
  ecma_free_value (value);

  if (ECMA_IS_VALUE_ERROR (result))
  {
    return result;
  }

  vm_executable_object_t *executable_object_p;
  executable_object_p = opfunc_create_executable_object (frame_ctx_p, VM_CREATE_EXECUTABLE_OBJECT_ASYNC);

  executable_object_p->extended_object.u.class_prop.extra_info |= extra_flags;

  ecma_promise_async_then (result, ecma_make_object_value ((ecma_object_t *) executable_object_p));
  ecma_deref_object ((ecma_object_t *) executable_object_p);
  ecma_free_value (result);

  ecma_object_t *old_new_target_p = JERRY_CONTEXT (current_new_target);
  JERRY_CONTEXT (current_new_target) = promise_p;

  result = ecma_op_create_promise_object (ECMA_VALUE_EMPTY, ECMA_PROMISE_EXECUTOR_EMPTY);

  JERRY_ASSERT (ecma_is_value_object (result));
  executable_object_p->frame_ctx.block_result = result;

  JERRY_CONTEXT (current_new_target) = old_new_target_p;
  return result;
} /* opfunc_async_create_and_await */

/**
 * Implicit class constructor handler when the classHeritage is not present.
 *
 * See also: ECMAScript v6, 14.5.14.10.b.i
 *
 * @return ECMA_VALUE_ERROR - if the function was invoked without 'new'
 *         ECMA_VALUE_UNDEFINED - otherwise
 */
static ecma_value_t
ecma_op_implicit_constructor_handler_cb (const ecma_value_t function_obj, /**< the function itself */
                                         const ecma_value_t this_val, /**< this_arg of the function */
                                         const ecma_value_t args_p[], /**< argument list */
                                         const uint32_t args_count) /**< argument number */
{
  JERRY_UNUSED_4 (function_obj, this_val, args_p, args_count);

  if (JERRY_CONTEXT (current_new_target) == NULL)
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Class constructor cannot be invoked without 'new'."));
  }

  return ECMA_VALUE_UNDEFINED;
} /* ecma_op_implicit_constructor_handler_cb */

/**
 * Implicit class constructor handler when the classHeritage is present.
 *
 * See also: ECMAScript v6, 14.5.14.10.a.i
 *
 * @return ECMA_VALUE_ERROR - if the operation fails
 *         result of the super call - otherwise
 */
static ecma_value_t
ecma_op_implicit_constructor_handler_heritage_cb (const ecma_value_t function_obj, /**< the function itself */
                                                  const ecma_value_t this_val, /**< this_arg of the function */
                                                  const ecma_value_t args_p[], /**< argument list */
                                                  const uint32_t args_count) /**< argument number */
{
  JERRY_UNUSED_4 (function_obj, this_val, args_p, args_count);

  if (JERRY_CONTEXT (current_new_target) == NULL)
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Class constructor cannot be invoked without 'new'."));
  }

  ecma_object_t *func_obj_p = ecma_get_object_from_value (function_obj);
  ecma_value_t super_ctor = ecma_op_function_get_super_constructor (func_obj_p);

  if (ECMA_IS_VALUE_ERROR (super_ctor))
  {
    return super_ctor;
  }

  ecma_object_t *super_ctor_p = ecma_get_object_from_value (super_ctor);

  ecma_value_t result = ecma_op_function_construct (super_ctor_p,
                                                    JERRY_CONTEXT (current_new_target),
                                                    args_p,
                                                    args_count);

  if (ecma_is_value_object (result))
  {
    ecma_value_t proto_value = ecma_op_object_get_by_magic_id (JERRY_CONTEXT (current_new_target),
                                                               LIT_MAGIC_STRING_PROTOTYPE);
    if (ECMA_IS_VALUE_ERROR (proto_value))
    {
      ecma_free_value (result);
      result = ECMA_VALUE_ERROR;
    }
    else if (ecma_is_value_object (proto_value))
    {
      ECMA_SET_POINTER (ecma_get_object_from_value (result)->u2.prototype_cp,
                        ecma_get_object_from_value (proto_value));
    }
    ecma_free_value (proto_value);
  }

  ecma_deref_object (super_ctor_p);

  return result;
} /* ecma_op_implicit_constructor_handler_heritage_cb */

/**
 * Create implicit class constructor
 *
 * See also: ECMAScript v6, 14.5.14
 *
 * @return - new external function ecma-object
 */
ecma_value_t
opfunc_create_implicit_class_constructor (uint8_t opcode) /**< current cbc opcode */
{
  /* 8. */
  ecma_object_t *func_obj_p = ecma_create_object (ecma_builtin_get (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE),
                                                  sizeof (ecma_extended_object_t),
                                                  ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION);

  ecma_extended_object_t *ext_func_obj_p = (ecma_extended_object_t *) func_obj_p;

  /* 10.a.i */
  if (opcode == CBC_EXT_PUSH_IMPLICIT_CONSTRUCTOR_HERITAGE)
  {
    ext_func_obj_p->u.external_handler_cb = ecma_op_implicit_constructor_handler_heritage_cb;
  }
  /* 10.b.i */
  else
  {
    ext_func_obj_p->u.external_handler_cb = ecma_op_implicit_constructor_handler_cb;
  }

  ecma_property_value_t *prop_value_p;
  prop_value_p = ecma_create_named_data_property (func_obj_p,
                                                  ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH),
                                                  ECMA_PROPERTY_FLAG_CONFIGURABLE,
                                                  NULL);

  prop_value_p->value = ecma_make_uint32_value (0);

  return ecma_make_object_value (func_obj_p);
} /* opfunc_create_implicit_class_constructor */

/**
 * Set the [[HomeObject]] attribute of the given functon object
 */
inline void JERRY_ATTR_ALWAYS_INLINE
opfunc_set_home_object (ecma_object_t *func_p, /**< function object */
                        ecma_object_t *parent_env_p) /**< parent environment */
{
  JERRY_ASSERT (ecma_is_lexical_environment (parent_env_p));

  if (ecma_get_object_type (func_p) == ECMA_OBJECT_TYPE_FUNCTION)
  {
    JERRY_ASSERT (!ecma_get_object_is_builtin (func_p));

    ECMA_SET_NON_NULL_POINTER_TAG (((ecma_extended_object_t *) func_p)->u.function.scope_cp, parent_env_p, 0);
  }
} /* opfunc_set_home_object */

/**
 * ClassDefinitionEvaluation environment initialization part
 *
 * See also: ECMAScript v6, 14.5.14
 *
 * @return - ECMA_VALUE_ERROR - if the operation fails
 *           ECMA_VALUE_EMPTY - otherwise
 */
void
opfunc_push_class_environment (vm_frame_ctx_t *frame_ctx_p, /**< frame context */
                               ecma_value_t **vm_stack_top, /**< VM stack top */
                               ecma_value_t class_name) /**< class name */
{
  JERRY_ASSERT (ecma_is_value_undefined (class_name) || ecma_is_value_string (class_name));
  ecma_object_t *class_env_p = ecma_create_decl_lex_env (frame_ctx_p->lex_env_p);

  /* 4.a */
  if (!ecma_is_value_undefined (class_name))
  {
    ecma_op_create_immutable_binding (class_env_p,
                                      ecma_get_string_from_value (class_name),
                                      ECMA_VALUE_UNINITIALIZED);
  }
  frame_ctx_p->lex_env_p = class_env_p;

  *(*vm_stack_top)++ = ECMA_VALUE_RELEASE_LEX_ENV;
} /* opfunc_push_class_environment */

/**
 * ClassDefinitionEvaluation object initialization part
 *
 * See also: ECMAScript v6, 14.5.14
 *
 * @return - ECMA_VALUE_ERROR - if the operation fails
 *           ECMA_VALUE_EMPTY - otherwise
 */
ecma_value_t
opfunc_init_class (vm_frame_ctx_t *frame_ctx_p, /**< frame context */
                   ecma_value_t *stack_top_p) /**< stack top */
{
  /* 5.b, 6.e.ii */
  ecma_object_t *ctor_parent_p = ecma_builtin_get (ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE);
  ecma_object_t *proto_parent_p = NULL;
  bool free_proto_parent = false;

  ecma_value_t super_class = stack_top_p[-2];
  ecma_object_t *ctor_p = ecma_get_object_from_value (stack_top_p[-1]);

  bool heritage_present = !ecma_is_value_array_hole (super_class);

  /* 5. ClassHeritage opt is not present */
  if (!heritage_present)
  {
    /* 5.a */
    proto_parent_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);
  }
  else if (!ecma_is_value_null (super_class))
  {
    /* 6.f, 6.g.i */
    if (!ecma_is_constructor (super_class)
        || ecma_op_function_is_generator (ecma_get_object_from_value (super_class)))
    {
      return ecma_raise_type_error ("Class extends value is not a constructor or null");
    }

    ecma_object_t *parent_p = ecma_get_object_from_value (super_class);

    /* 6.g.ii */
    ecma_value_t proto_parent = ecma_op_object_get_by_magic_id (parent_p, LIT_MAGIC_STRING_PROTOTYPE);

    /* 6.g.iii */
    if (ECMA_IS_VALUE_ERROR (proto_parent))
    {
      return proto_parent;
    }

    /* 6.g.iv */
    if (ecma_is_value_object (proto_parent))
    {
      proto_parent_p = ecma_get_object_from_value (proto_parent);
      free_proto_parent = true;
    }
    else if (ecma_is_value_null (proto_parent))
    {
      proto_parent_p = NULL;
    }
    else
    {
      ecma_free_value (proto_parent);
      return ecma_raise_type_error ("Property 'prototype' is not an object or null");
    }

    /* 6.g.v */
    ctor_parent_p = parent_p;
  }

  /* 7. */
  ecma_object_t *proto_p = ecma_create_object (proto_parent_p, 0, ECMA_OBJECT_TYPE_GENERAL);
  ecma_value_t proto = ecma_make_object_value (proto_p);

  ECMA_SET_POINTER (ctor_p->u2.prototype_cp, ctor_parent_p);

  if (free_proto_parent)
  {
    ecma_deref_object (proto_parent_p);
  }
  ecma_free_value (super_class);

  /* 16. */
  ecma_property_value_t *property_value_p;
  property_value_p = ecma_create_named_data_property (ctor_p,
                                                      ecma_get_magic_string (LIT_MAGIC_STRING_PROTOTYPE),
                                                      ECMA_PROPERTY_FIXED,
                                                      NULL);
  property_value_p->value = proto;

  /* 18. */
  property_value_p = ecma_create_named_data_property (proto_p,
                                                      ecma_get_magic_string (LIT_MAGIC_STRING_CONSTRUCTOR),
                                                      ECMA_PROPERTY_CONFIGURABLE_WRITABLE,
                                                      NULL);
  property_value_p->value = ecma_make_object_value (ctor_p);

  if (ecma_get_object_type (ctor_p) == ECMA_OBJECT_TYPE_FUNCTION)
  {
    ecma_object_t *proto_env_p = ecma_create_object_lex_env (frame_ctx_p->lex_env_p,
                                                             proto_p,
                                                             ECMA_LEXICAL_ENVIRONMENT_HOME_OBJECT_BOUND);

    ECMA_SET_NON_NULL_POINTER_TAG (((ecma_extended_object_t *) ctor_p)->u.function.scope_cp, proto_env_p, 0);

    /* 15. set F’s [[ConstructorKind]] internal slot to "derived". */
    if (heritage_present)
    {
      ECMA_SET_THIRD_BIT_TO_POINTER_TAG (((ecma_extended_object_t *) ctor_p)->u.function.scope_cp);
    }

    ecma_deref_object (proto_env_p);
  }

  stack_top_p[-2] = stack_top_p[-1];
  stack_top_p[-1] = proto;

  return ECMA_VALUE_EMPTY;
} /* opfunc_init_class */

/**
 * Set [[Enumerable]] and [[HomeObject]] attributes for all class method
 */
static void
opfunc_set_class_attributes (ecma_object_t *obj_p, /**< object */
                             ecma_object_t *parent_env_p) /**< parent environment */
{
  jmem_cpointer_t prop_iter_cp = obj_p->u1.property_list_cp;

#if ENABLED (JERRY_PROPRETY_HASHMAP)
  if (prop_iter_cp != JMEM_CP_NULL)
  {
    ecma_property_header_t *prop_iter_p = ECMA_GET_NON_NULL_POINTER (ecma_property_header_t, prop_iter_cp);
    if (prop_iter_p->types[0] == ECMA_PROPERTY_TYPE_HASHMAP)
    {
      prop_iter_cp = prop_iter_p->next_property_cp;
    }
  }
#endif /* ENABLED (JERRY_PROPRETY_HASHMAP) */

  while (prop_iter_cp != JMEM_CP_NULL)
  {
    ecma_property_header_t *prop_iter_p = ECMA_GET_NON_NULL_POINTER (ecma_property_header_t, prop_iter_cp);
    JERRY_ASSERT (ECMA_PROPERTY_IS_PROPERTY_PAIR (prop_iter_p));

    ecma_property_pair_t *property_pair_p = (ecma_property_pair_t *) prop_iter_p;

    for (uint32_t index = 0; index < ECMA_PROPERTY_PAIR_ITEM_COUNT; index++)
    {
      uint8_t property = property_pair_p->header.types[index];

      if (ECMA_PROPERTY_GET_TYPE (property) == ECMA_PROPERTY_TYPE_NAMEDDATA)
      {
        if (ecma_is_value_object (property_pair_p->values[index].value)
            && ecma_is_property_enumerable (property))
        {
          property_pair_p->header.types[index] = (uint8_t) (property & ~ECMA_PROPERTY_FLAG_ENUMERABLE);
          opfunc_set_home_object (ecma_get_object_from_value (property_pair_p->values[index].value), parent_env_p);
        }
      }
      else if (ECMA_PROPERTY_GET_TYPE (property) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR)
      {
        property_pair_p->header.types[index] = (uint8_t) (property & ~ECMA_PROPERTY_FLAG_ENUMERABLE);
        ecma_property_value_t *accessor_objs_p = property_pair_p->values + index;

        ecma_getter_setter_pointers_t *get_set_pair_p = ecma_get_named_accessor_property (accessor_objs_p);

        if (get_set_pair_p->getter_cp != JMEM_CP_NULL)
        {
          opfunc_set_home_object (ECMA_GET_NON_NULL_POINTER (ecma_object_t, get_set_pair_p->getter_cp), parent_env_p);
        }

        if (get_set_pair_p->setter_cp != JMEM_CP_NULL)
        {
          opfunc_set_home_object (ECMA_GET_NON_NULL_POINTER (ecma_object_t, get_set_pair_p->setter_cp), parent_env_p);
        }
      }
      else
      {
        JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (property) == ECMA_PROPERTY_TYPE_SPECIAL);

        JERRY_ASSERT (property == ECMA_PROPERTY_TYPE_HASHMAP
                      || property == ECMA_PROPERTY_TYPE_DELETED);
      }
    }

    prop_iter_cp = prop_iter_p->next_property_cp;
  }
} /* opfunc_set_class_attributes */

/**
 * Pop the current lexical environment referenced by the frame context
 */
void
opfunc_pop_lexical_environment (vm_frame_ctx_t *frame_ctx_p) /**< frame context */
{
  ecma_object_t *outer_env_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, frame_ctx_p->lex_env_p->u2.outer_reference_cp);
  ecma_deref_object (frame_ctx_p->lex_env_p);
  frame_ctx_p->lex_env_p = outer_env_p;
} /* opfunc_pop_lexical_environment */

/**
 * ClassDefinitionEvaluation finalization part
 *
 * See also: ECMAScript v6, 14.5.14
 */
void
opfunc_finalize_class (vm_frame_ctx_t *frame_ctx_p, /**< frame context */
                       ecma_value_t **vm_stack_top_p, /**< current vm stack top */
                       ecma_value_t class_name) /**< class name */
{
  JERRY_ASSERT (ecma_is_value_undefined (class_name) || ecma_is_value_string (class_name));
  ecma_value_t *stack_top_p = *vm_stack_top_p;

  ecma_object_t *ctor_p = ecma_get_object_from_value (stack_top_p[-2]);
  ecma_object_t *proto_p = ecma_get_object_from_value (stack_top_p[-1]);

  ecma_object_t *class_env_p = frame_ctx_p->lex_env_p;

  /* 23.a */
  if (!ecma_is_value_undefined (class_name))
  {
    ecma_op_initialize_binding (class_env_p, ecma_get_string_from_value (class_name), stack_top_p[-2]);
  }

  ecma_object_t *ctor_env_p = ecma_create_object_lex_env (class_env_p,
                                                          ctor_p,
                                                          ECMA_LEXICAL_ENVIRONMENT_HOME_OBJECT_BOUND);
  ecma_object_t *proto_env_p = ecma_create_object_lex_env (class_env_p,
                                                           proto_p,
                                                           ECMA_LEXICAL_ENVIRONMENT_HOME_OBJECT_BOUND);

  opfunc_set_class_attributes (ctor_p, ctor_env_p);
  opfunc_set_class_attributes (proto_p, proto_env_p);

  ecma_deref_object (proto_env_p);
  ecma_deref_object (ctor_env_p);

  opfunc_pop_lexical_environment (frame_ctx_p);

  ecma_deref_object (proto_p);

  /* only the current class remains on the stack */
  JERRY_ASSERT (stack_top_p[-3] == ECMA_VALUE_RELEASE_LEX_ENV);
  stack_top_p[-3] = stack_top_p[-2];
  *vm_stack_top_p -= 2;
} /* opfunc_finalize_class */

/**
 * MakeSuperPropertyReference operation
 *
 * See also: ECMAScript v6, 12.3.5.3
 *
 * @return ECMA_VALUE_ERROR - if the operation fails
 *         ECMA_VALUE_EMPTY - otherwise
 */
ecma_value_t
opfunc_form_super_reference (ecma_value_t **vm_stack_top_p, /**< current vm stack top */
                             vm_frame_ctx_t *frame_ctx_p, /**< frame context */
                             ecma_value_t prop_name, /**< property name to resolve */
                             uint8_t opcode) /**< current cbc opcode */
{
  ecma_value_t parent = ecma_op_resolve_super_base (frame_ctx_p->lex_env_p);

  if (ECMA_IS_VALUE_ERROR (parent))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Cannot invoke nullable super method."));
  }

  if (ECMA_IS_VALUE_ERROR (ecma_op_check_object_coercible (parent)))
  {
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t *stack_top_p = *vm_stack_top_p;

  if (opcode >= CBC_EXT_SUPER_PROP_ASSIGNMENT_REFERENCE)
  {
    JERRY_ASSERT (opcode == CBC_EXT_SUPER_PROP_ASSIGNMENT_REFERENCE
                  || opcode == CBC_EXT_SUPER_PROP_LITERAL_ASSIGNMENT_REFERENCE);
    *stack_top_p++ = parent;
    *stack_top_p++ = ecma_copy_value (prop_name);
    *vm_stack_top_p = stack_top_p;

    return ECMA_VALUE_EMPTY;
  }

  ecma_object_t *parent_p = ecma_get_object_from_value (parent);
  ecma_string_t *prop_name_p = ecma_op_to_property_key (prop_name);

  if (prop_name_p == NULL)
  {
    ecma_deref_object (parent_p);
    return ECMA_VALUE_ERROR;
  }

  ecma_value_t result = ecma_op_object_get_with_receiver (parent_p, prop_name_p, frame_ctx_p->this_binding);
  ecma_deref_ecma_string (prop_name_p);
  ecma_deref_object (parent_p);

  if (ECMA_IS_VALUE_ERROR (result))
  {
    return result;
  }

  if (opcode == CBC_EXT_SUPER_PROP_LITERAL_REFERENCE || opcode == CBC_EXT_SUPER_PROP_REFERENCE)
  {
    *stack_top_p++ = ecma_copy_value (frame_ctx_p->this_binding);
    *stack_top_p++ = ECMA_VALUE_UNDEFINED;
  }

  *stack_top_p++ = result;
  *vm_stack_top_p = stack_top_p;

  return ECMA_VALUE_EMPTY;
} /* opfunc_form_super_reference */

/**
 * Assignment operation for SuperRefence base
 *
 * @return ECMA_VALUE_ERROR - if the operation fails
 *         ECMA_VALUE_EMPTY - otherwise
 */
ecma_value_t
opfunc_assign_super_reference (ecma_value_t **vm_stack_top_p, /**< vm stack top */
                               vm_frame_ctx_t *frame_ctx_p, /**< frame context */
                               uint32_t opcode_data) /**< opcode data to store the result */
{
  ecma_value_t *stack_top_p = *vm_stack_top_p;

  ecma_value_t base_obj = ecma_op_to_object (stack_top_p[-3]);

  if (ECMA_IS_VALUE_ERROR (base_obj))
  {
    return base_obj;
  }

  ecma_object_t *base_obj_p = ecma_get_object_from_value (base_obj);
  ecma_string_t *prop_name_p = ecma_op_to_property_key (stack_top_p[-2]);

  if (prop_name_p == NULL)
  {
    ecma_deref_object (base_obj_p);
    return ECMA_VALUE_ERROR;
  }

  bool is_strict = (frame_ctx_p->bytecode_header_p->status_flags & CBC_CODE_FLAGS_STRICT_MODE) != 0;

  ecma_value_t result = ecma_op_object_put_with_receiver (base_obj_p,
                                                          prop_name_p,
                                                          stack_top_p[-1],
                                                          frame_ctx_p->this_binding,
                                                          is_strict);

  ecma_deref_ecma_string (prop_name_p);
  ecma_deref_object (base_obj_p);

  if (ECMA_IS_VALUE_ERROR (result))
  {
    return result;
  }

  for (int32_t i = 1; i <= 3; i++)
  {
    ecma_free_value (stack_top_p[-i]);
  }

  stack_top_p -= 3;

  if (opcode_data & VM_OC_PUT_STACK)
  {
    *stack_top_p++ = result;
  }
  else if (opcode_data & VM_OC_PUT_BLOCK)
  {
    ecma_fast_free_value (frame_ctx_p->block_result);
    frame_ctx_p->block_result = result;
  }

  *vm_stack_top_p = stack_top_p;

  return result;
} /* opfunc_assign_super_reference */
#endif /* ENABLED (JERRY_ESNEXT) */

/**
 * @}
 * @}
 */
