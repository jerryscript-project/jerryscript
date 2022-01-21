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

#include "ecma-arguments-object.h"

#include "ecma-alloc.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lcache.h"
#include "ecma-lex-env.h"
#include "ecma-objects-general.h"
#include "ecma-objects.h"
#include "ecma-ordinary-object.h"

#include "jrt.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaargumentsobject ECMA arguments object related routines
 * @{
 */

/**
 * Arguments object creation operation.
 *
 * See also: ECMA-262 v5, 10.6
 *
 * @return ecma value of arguments object
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_create_arguments_object (vm_frame_ctx_shared_args_t *shared_p, /**< shared context data */
                                 ecma_object_t *lex_env_p) /**< lexical environment the Arguments
                                                            *   object is created for */
{
  ecma_object_t *func_obj_p = shared_p->header.function_object_p;
  const ecma_compiled_code_t *bytecode_data_p = shared_p->header.bytecode_header_p;
  uint16_t formal_params_number;

  if (bytecode_data_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
  {
    formal_params_number = ((cbc_uint16_arguments_t *) bytecode_data_p)->argument_end;
  }
  else
  {
    formal_params_number = ((cbc_uint8_arguments_t *) bytecode_data_p)->argument_end;
  }

  uint32_t object_size = sizeof (ecma_unmapped_arguments_t);
  uint32_t saved_arg_count = JERRY_MAX (shared_p->arg_list_len, formal_params_number);

  if (bytecode_data_p->status_flags & CBC_CODE_FLAGS_MAPPED_ARGUMENTS_NEEDED)
  {
    object_size = sizeof (ecma_mapped_arguments_t);
  }

  ecma_object_t *obj_p = ecma_create_object (ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE),
                                             object_size + (saved_arg_count * sizeof (ecma_value_t)),
                                             ECMA_OBJECT_TYPE_CLASS);

  ecma_unmapped_arguments_t *arguments_p = (ecma_unmapped_arguments_t *) obj_p;

  arguments_p->header.u.cls.type = ECMA_OBJECT_CLASS_ARGUMENTS;
  arguments_p->header.u.cls.u1.arguments_flags = ECMA_ARGUMENTS_OBJECT_NO_FLAGS;
  arguments_p->header.u.cls.u2.formal_params_number = formal_params_number;
  arguments_p->header.u.cls.u3.arguments_number = 0;
  arguments_p->callee = ecma_make_object_value (func_obj_p);

  ecma_value_t *argv_p = (ecma_value_t *) (((uint8_t *) obj_p) + object_size);

  for (uint32_t i = 0; i < shared_p->arg_list_len; i++)
  {
    argv_p[i] = ecma_copy_value_if_not_object (shared_p->arg_list_p[i]);
  }

  for (uint32_t i = shared_p->arg_list_len; i < saved_arg_count; i++)
  {
    argv_p[i] = ECMA_VALUE_UNDEFINED;
  }

  arguments_p->header.u.cls.u3.arguments_number = shared_p->arg_list_len;

  if (bytecode_data_p->status_flags & CBC_CODE_FLAGS_MAPPED_ARGUMENTS_NEEDED)
  {
    ecma_mapped_arguments_t *mapped_arguments_p = (ecma_mapped_arguments_t *) obj_p;

    ECMA_SET_INTERNAL_VALUE_POINTER (mapped_arguments_p->lex_env, lex_env_p);
    arguments_p->header.u.cls.u1.arguments_flags |= ECMA_ARGUMENTS_OBJECT_MAPPED;

#if JERRY_SNAPSHOT_EXEC
    if (bytecode_data_p->status_flags & CBC_CODE_FLAGS_STATIC_FUNCTION)
    {
      arguments_p->header.u.cls.u1.arguments_flags |= ECMA_ARGUMENTS_OBJECT_STATIC_BYTECODE;
      mapped_arguments_p->u.byte_code_p = (ecma_compiled_code_t *) bytecode_data_p;
    }
    else
#endif /* JERRY_SNAPSHOT_EXEC */
    {
      ECMA_SET_INTERNAL_VALUE_POINTER (mapped_arguments_p->u.byte_code, bytecode_data_p);
    }

    /* Static snapshots are not ref counted. */
    if ((bytecode_data_p->status_flags & CBC_CODE_FLAGS_STATIC_FUNCTION) == 0)
    {
      ecma_bytecode_ref ((ecma_compiled_code_t *) bytecode_data_p);
    }

    ecma_value_t *formal_parameter_start_p;
    formal_parameter_start_p = ecma_compiled_code_resolve_arguments_start ((ecma_compiled_code_t *) bytecode_data_p);

    for (uint32_t i = 0; i < formal_params_number; i++)
    {
      /* For legacy (non-strict) argument definition the trailing duplicated arguments cannot be lazy instantiated
         E.g: function f (a,a,a,a) {} */
      if (JERRY_UNLIKELY (ecma_is_value_empty (formal_parameter_start_p[i])))
      {
        ecma_property_value_t *prop_value_p;
        ecma_string_t *prop_name_p = ecma_new_ecma_string_from_uint32 (i);

        prop_value_p =
          ecma_create_named_data_property (obj_p, prop_name_p, ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE, NULL);

        ecma_deref_ecma_string (prop_name_p);

        prop_value_p->value = argv_p[i];
        argv_p[i] = ECMA_VALUE_EMPTY;
      }
    }
  }

  return ecma_make_object_value (obj_p);
} /* ecma_op_create_arguments_object */

/**
 * Get the formal parameter name corresponding to the given property index
 *
 * @return pointer to the formal parameter name
 */
static ecma_string_t *
ecma_op_arguments_object_get_formal_parameter (ecma_mapped_arguments_t *mapped_arguments_p, /**< mapped arguments
                                                                                             *   object */
                                               uint32_t index) /**< formal parameter index */
{
  JERRY_ASSERT (mapped_arguments_p->unmapped.header.u.cls.u1.arguments_flags & ECMA_ARGUMENTS_OBJECT_MAPPED);
  JERRY_ASSERT (index < mapped_arguments_p->unmapped.header.u.cls.u2.formal_params_number);

  ecma_compiled_code_t *byte_code_p;

#if JERRY_SNAPSHOT_EXEC
  if (mapped_arguments_p->unmapped.header.u.cls.u1.arguments_flags & ECMA_ARGUMENTS_OBJECT_STATIC_BYTECODE)
  {
    byte_code_p = mapped_arguments_p->u.byte_code_p;
  }
  else
#endif /* JERRY_SNAPSHOT_EXEC */
  {
    byte_code_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_compiled_code_t, mapped_arguments_p->u.byte_code);
  }

  ecma_value_t *formal_param_names_p = ecma_compiled_code_resolve_arguments_start (byte_code_p);

  return ecma_get_string_from_value (formal_param_names_p[index]);
} /* ecma_op_arguments_object_get_formal_parameter */

/**
 * ecma arguments object's [[GetOwnProperty]] internal method
 *
 * See also:
 *          ECMA-262 v12, 10.4.4.1
 *
 * @return ecma property descriptor t
 */
ecma_property_descriptor_t
ecma_arguments_object_get_own_property (ecma_object_t *obj_p, /**< the object */
                                        ecma_string_t *property_name_p) /**< property name */

{
  ecma_property_descriptor_t prop_desc = ecma_make_empty_property_descriptor ();

  prop_desc.u.property_p = ecma_find_named_property (obj_p, property_name_p);

  if (prop_desc.u.property_p != NULL)
  {
    ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

    if (ext_object_p->u.cls.u1.arguments_flags & ECMA_ARGUMENTS_OBJECT_MAPPED)
    {
      uint32_t index = ecma_string_get_array_index (property_name_p);

      if (index < ext_object_p->u.cls.u2.formal_params_number)
      {
        ecma_mapped_arguments_t *mapped_arguments_p = (ecma_mapped_arguments_t *) ext_object_p;

        ecma_value_t *argv_p = (ecma_value_t *) (mapped_arguments_p + 1);

        if (!ecma_is_value_empty (argv_p[index]) && argv_p[index] != ECMA_VALUE_ARGUMENT_NO_TRACK)
        {
#if JERRY_LCACHE
          /* Mapped arguments initialized properties MUST not be lcached */
          if (ecma_is_property_lcached (prop_desc.u.property_p))
          {
            jmem_cpointer_t prop_name_cp;

            if (JERRY_UNLIKELY (ECMA_IS_DIRECT_STRING (property_name_p)))
            {
              prop_name_cp = (jmem_cpointer_t) ECMA_GET_DIRECT_STRING_VALUE (property_name_p);
            }
            else
            {
              ECMA_SET_NON_NULL_POINTER (prop_name_cp, property_name_p);
            }
            ecma_lcache_invalidate (obj_p, prop_name_cp, prop_desc.u.property_p);
          }
#endif /* JERRY_LCACHE */

          ecma_string_t *name_p = ecma_op_arguments_object_get_formal_parameter (mapped_arguments_p, index);
          ecma_object_t *lex_env_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t, mapped_arguments_p->lex_env);

          ecma_value_t binding_value = ecma_op_get_binding_value (lex_env_p, name_p, true);

          ecma_named_data_property_assign_value (obj_p,
                                                 ECMA_PROPERTY_VALUE_PTR (prop_desc.u.property_p),
                                                 binding_value);
          ecma_free_value (binding_value);
        }
      }
    }

    prop_desc.flags =
      ECMA_PROP_DESC_PROPERTY_FOUND | ECMA_PROPERTY_TO_PROPERTY_DESCRIPTOR_FLAGS (prop_desc.u.property_p);
    return prop_desc;
  }

  ecma_unmapped_arguments_t *arguments_p = (ecma_unmapped_arguments_t *) obj_p;
  ecma_value_t *argv_p = (ecma_value_t *) (arguments_p + 1);
  uint32_t arguments_number = arguments_p->header.u.cls.u3.arguments_number;
  uint8_t flags = arguments_p->header.u.cls.u1.arguments_flags;

  if (flags & ECMA_ARGUMENTS_OBJECT_MAPPED)
  {
    argv_p = (ecma_value_t *) (((ecma_mapped_arguments_t *) obj_p) + 1);
  }

  uint32_t index = ecma_string_get_array_index (property_name_p);

  if (index != ECMA_STRING_NOT_ARRAY_INDEX)
  {
    if (index >= arguments_number || ecma_is_value_empty (argv_p[index]))
    {
      return prop_desc;
    }

    JERRY_ASSERT (argv_p[index] != ECMA_VALUE_ARGUMENT_NO_TRACK);

    prop_desc.flags = ECMA_PROP_DESC_PROPERTY_FOUND | ECMA_PROP_DESC_DATA_CONFIGURABLE_ENUMERABLE_WRITABLE;
    ecma_property_value_t *prop_value_p =
      ecma_create_named_data_property (obj_p,
                                       property_name_p,
                                       ECMA_PROPERTY_BUILT_IN_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                       &prop_desc.u.property_p);

    /* Passing the reference */
    prop_value_p->value = argv_p[index];

    argv_p[index] = ECMA_VALUE_UNDEFINED;
    return prop_desc;
  }

  if (ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_LENGTH)
      && !(flags & ECMA_ARGUMENTS_OBJECT_LENGTH_INITIALIZED))
  {
    prop_desc.flags = ECMA_PROP_DESC_PROPERTY_FOUND | ECMA_PROP_DESC_DATA_CONFIGURABLE_WRITABLE;

    ecma_property_value_t *prop_value_p =
      ecma_create_named_data_property (obj_p,
                                       ecma_get_magic_string (LIT_MAGIC_STRING_LENGTH),
                                       ECMA_PROPERTY_BUILT_IN_CONFIGURABLE_WRITABLE,
                                       &prop_desc.u.property_p);

    prop_value_p->value = ecma_make_uint32_value (arguments_number);
    return prop_desc;
  }

  if (ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_CALLEE)
      && !(flags & ECMA_ARGUMENTS_OBJECT_CALLEE_INITIALIZED))
  {
    if (flags & ECMA_ARGUMENTS_OBJECT_MAPPED)
    {
      prop_desc.flags = ECMA_PROP_DESC_PROPERTY_FOUND | ECMA_PROP_DESC_DATA_CONFIGURABLE_WRITABLE;
      ecma_property_value_t *prop_value_p =
        ecma_create_named_data_property (obj_p,
                                         property_name_p,
                                         ECMA_PROPERTY_BUILT_IN_CONFIGURABLE_WRITABLE,
                                         &prop_desc.u.property_p);

      prop_value_p->value = arguments_p->callee;
    }
    else
    {
      ecma_object_t *thrower_p = ecma_builtin_get (ECMA_BUILTIN_ID_TYPE_ERROR_THROWER);

      prop_desc.flags = ECMA_PROP_DESC_PROPERTY_FOUND;
      ecma_create_named_accessor_property (obj_p,
                                           ecma_get_magic_string (LIT_MAGIC_STRING_CALLEE),
                                           thrower_p,
                                           thrower_p,
                                           ECMA_PROPERTY_BUILT_IN_FIXED,
                                           &prop_desc.u.property_p);
    }
    return prop_desc;
  }

#if !JERRY_ESNEXT
  if (property_name_p == ecma_get_magic_string (LIT_MAGIC_STRING_CALLER))
  {
    if (arguments_p->header.u.cls.u1.arguments_flags & ECMA_ARGUMENTS_OBJECT_MAPPED)
    {
      return prop_desc;
    }

    ecma_object_t *thrower_p = ecma_builtin_get (ECMA_BUILTIN_ID_TYPE_ERROR_THROWER);

    prop_desc.flags = ECMA_PROP_DESC_PROPERTY_FOUND;
    ecma_create_named_accessor_property (obj_p,
                                         ecma_get_magic_string (LIT_MAGIC_STRING_CALLER),
                                         thrower_p,
                                         thrower_p,
                                         ECMA_PROPERTY_BUILT_IN_FIXED,
                                         &prop_desc.u.property_p);
    return prop_desc;
  }
#else /* JERRY_ESNEXT */
  if (ecma_op_compare_string_to_global_symbol (property_name_p, LIT_GLOBAL_SYMBOL_ITERATOR)
      && !(flags & ECMA_ARGUMENTS_OBJECT_ITERATOR_INITIALIZED))
  {
    prop_desc.flags = ECMA_PROP_DESC_PROPERTY_FOUND | ECMA_PROP_DESC_DATA_CONFIGURABLE_WRITABLE;
    ecma_property_value_t *prop_value_p = ecma_create_named_data_property (obj_p,
                                                                           property_name_p,
                                                                           ECMA_PROPERTY_BUILT_IN_CONFIGURABLE_WRITABLE,
                                                                           &prop_desc.u.property_p);

    prop_value_p->value = ecma_op_object_get_by_magic_id (ecma_builtin_get (ECMA_BUILTIN_ID_INTRINSIC_OBJECT),
                                                          LIT_INTERNAL_MAGIC_STRING_ARRAY_PROTOTYPE_VALUES);

    JERRY_ASSERT (ecma_is_value_object (prop_value_p->value));
    ecma_deref_object (ecma_get_object_from_value (prop_value_p->value));
    return prop_desc;
  }
#endif /* !JERRY_ESNEXT */

  return prop_desc;
} /* ecma_arguments_object_get_own_property */

/**
 * ecma arguments object's [[Get]] internal method
 *
 * See also:
 *          ECMA-262 v12, 10.4.4.3
 *
 * @return ecma value t
 */
ecma_value_t
ecma_arguments_object_get (ecma_object_t *obj_p, /**< the object */
                           ecma_string_t *property_name_p, /**< property name */
                           ecma_value_t receiver) /**< receiver */

{
  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

  if (!(ext_object_p->u.cls.u1.arguments_flags & ECMA_ARGUMENTS_OBJECT_MAPPED))
  {
    return ecma_ordinary_object_get (obj_p, property_name_p, receiver);
  }

  uint32_t index = ecma_string_get_array_index (property_name_p);

  if (index < ext_object_p->u.cls.u2.formal_params_number)
  {
    ecma_mapped_arguments_t *mapped_arguments_p = (ecma_mapped_arguments_t *) ext_object_p;

    ecma_value_t *argv_p = (ecma_value_t *) (mapped_arguments_p + 1);

    /* 2. */
    if (!ecma_is_value_empty (argv_p[index]) && argv_p[index] != ECMA_VALUE_ARGUMENT_NO_TRACK)
    {
      /* 3. */
      ecma_string_t *name_p = ecma_op_arguments_object_get_formal_parameter (mapped_arguments_p, index);
      ecma_object_t *lex_env_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t, mapped_arguments_p->lex_env);

      return ecma_op_get_binding_value (lex_env_p, name_p, true);
    }
  }

  /* 4. */
  return ecma_ordinary_object_get (obj_p, property_name_p, receiver);
} /* ecma_arguments_object_get */

/**
 * ecma arguments object's [[Set]] internal method
 *
 * See also:
 *          ECMA-262 v12, 10.4.4.4
 *
 * @return ecma value t
 */
ecma_value_t
ecma_arguments_object_set (ecma_object_t *obj_p, /**< the object */
                           ecma_string_t *property_name_p, /**< property name */
                           ecma_value_t value, /**< ecma value */
                           ecma_value_t receiver, /**< receiver */
                           bool is_throw) /**< flag that controls failure handling */
{
  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

  /* 1. */
  if (!(ext_object_p->u.cls.u1.arguments_flags & ECMA_ARGUMENTS_OBJECT_MAPPED))
  {
    return ecma_ordinary_object_set (obj_p, property_name_p, value, receiver, is_throw);
  }

  uint32_t index = ecma_string_get_array_index (property_name_p);

  if (index < ext_object_p->u.cls.u2.formal_params_number)
  {
    ecma_mapped_arguments_t *mapped_arguments_p = (ecma_mapped_arguments_t *) ext_object_p;

    ecma_value_t *argv_p = (ecma_value_t *) (mapped_arguments_p + 1);

    /* 2.b */
    if (!ecma_is_value_empty (argv_p[index]) && argv_p[index] != ECMA_VALUE_ARGUMENT_NO_TRACK)
    {
      /* 3.a */
      ecma_string_t *name_p = ecma_op_arguments_object_get_formal_parameter (mapped_arguments_p, index);
      ecma_object_t *lex_env_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t, mapped_arguments_p->lex_env);
      ecma_op_set_mutable_binding (lex_env_p, name_p, value, true);
      return ECMA_VALUE_TRUE;
    }
  }

  /* 4. */
  return ecma_ordinary_object_set (obj_p, property_name_p, value, receiver, is_throw);
} /* ecma_arguments_object_set */

/**
 * ecma arguments object's [[DefineOwnProperty]] internal method
 *
 * See also:
 *          ECMA-262 v12, 10.4.4.2
 *
 * @return ecma value t
 */
ecma_value_t
ecma_arguments_object_define_own_property (ecma_object_t *object_p, /**< the object */
                                           ecma_string_t *property_name_p, /**< property name */
                                           const ecma_property_descriptor_t *property_desc_p) /**< property
                                                                                               *   descriptor */
{
  /* 3. */
  ecma_value_t ret_value = ecma_ordinary_object_define_own_property (object_p, property_name_p, property_desc_p);

  if (ECMA_IS_VALUE_ERROR (ret_value)
      || !(((ecma_extended_object_t *) object_p)->u.cls.u1.arguments_flags & ECMA_ARGUMENTS_OBJECT_MAPPED))
  {
    return ret_value;
  }

  ecma_mapped_arguments_t *mapped_arguments_p = (ecma_mapped_arguments_t *) object_p;
  uint32_t index = ecma_string_get_array_index (property_name_p);

  if (index >= mapped_arguments_p->unmapped.header.u.cls.u2.formal_params_number)
  {
    return ret_value;
  }

  ecma_value_t *argv_p = (ecma_value_t *) (mapped_arguments_p + 1);

  if (ecma_is_value_empty (argv_p[index]) || argv_p[index] == ECMA_VALUE_ARGUMENT_NO_TRACK)
  {
    return ret_value;
  }

  if (property_desc_p->flags & (JERRY_PROP_IS_GET_DEFINED | JERRY_PROP_IS_SET_DEFINED))
  {
    ecma_free_value_if_not_object (argv_p[index]);
    argv_p[index] = ECMA_VALUE_ARGUMENT_NO_TRACK;
    return ret_value;
  }

  if (property_desc_p->flags & JERRY_PROP_IS_VALUE_DEFINED)
  {
    ecma_string_t *name_p = ecma_op_arguments_object_get_formal_parameter (mapped_arguments_p, index);
    ecma_object_t *lex_env_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t, mapped_arguments_p->lex_env);

    ecma_value_t completion = ecma_op_set_mutable_binding (lex_env_p, name_p, property_desc_p->value, true);

    JERRY_ASSERT (ecma_is_value_empty (completion));
  }

  if ((property_desc_p->flags & JERRY_PROP_IS_WRITABLE_DEFINED) && !(property_desc_p->flags & JERRY_PROP_IS_WRITABLE))
  {
    ecma_free_value_if_not_object (argv_p[index]);
    argv_p[index] = ECMA_VALUE_ARGUMENT_NO_TRACK;
  }

  return ret_value;
} /* ecma_arguments_object_define_own_property */

/**
 * List names of an arguments object's lazy instantiated properties
 */
void
ecma_arguments_object_list_lazy_property_keys (ecma_object_t *obj_p, /**< arguments object */
                                               ecma_collection_t *prop_names_p, /**< prop name collection */
                                               ecma_property_counter_t *prop_counter_p, /**< property counters */
                                               jerry_property_filter_t filter) /**< property name filter options */
{
  ecma_unmapped_arguments_t *arguments_p = (ecma_unmapped_arguments_t *) obj_p;

  uint32_t arguments_number = arguments_p->header.u.cls.u3.arguments_number;
  uint8_t flags = arguments_p->header.u.cls.u1.arguments_flags;

  if (!(filter & JERRY_PROPERTY_FILTER_EXCLUDE_INTEGER_INDICES))
  {
    ecma_value_t *argv_p = (ecma_value_t *) (arguments_p + 1);

    if (flags & ECMA_ARGUMENTS_OBJECT_MAPPED)
    {
      argv_p = (ecma_value_t *) (((ecma_mapped_arguments_t *) obj_p) + 1);
    }

    for (uint32_t index = 0; index < arguments_number; index++)
    {
      if (!ecma_is_value_empty (argv_p[index]))
      {
        ecma_string_t *index_string_p = ecma_new_ecma_string_from_uint32 (index);
        ecma_collection_push_back (prop_names_p, ecma_make_string_value (index_string_p));
        prop_counter_p->array_index_named_props++;
      }
    }
  }

  if (!(filter & JERRY_PROPERTY_FILTER_EXCLUDE_STRINGS))
  {
    if (!(flags & ECMA_ARGUMENTS_OBJECT_LENGTH_INITIALIZED))
    {
      ecma_collection_push_back (prop_names_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_LENGTH));
      prop_counter_p->string_named_props++;
    }

    if (!(flags & ECMA_ARGUMENTS_OBJECT_CALLEE_INITIALIZED))
    {
      ecma_collection_push_back (prop_names_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_CALLEE));
      prop_counter_p->string_named_props++;
    }

#if !JERRY_ESNEXT
    if (!(flags & ECMA_ARGUMENTS_OBJECT_MAPPED))
    {
      ecma_collection_push_back (prop_names_p, ecma_make_magic_string_value (LIT_MAGIC_STRING_CALLER));
      prop_counter_p->string_named_props++;
    }
#endif /* !JERRY_ESNEXT */
  }

#if JERRY_ESNEXT
  if (!(filter & JERRY_PROPERTY_FILTER_EXCLUDE_SYMBOLS) && !(flags & ECMA_ARGUMENTS_OBJECT_ITERATOR_INITIALIZED))
  {
    ecma_string_t *symbol_p = ecma_op_get_global_symbol (LIT_GLOBAL_SYMBOL_ITERATOR);
    ecma_collection_push_back (prop_names_p, ecma_make_symbol_value (symbol_p));
    prop_counter_p->symbol_named_props++;
  }
#endif /* JERRY_ESNEXT */
} /* ecma_arguments_object_list_lazy_property_keys */

/**
 * Delete configurable properties of arguments object
 */
void
ecma_arguments_object_delete_lazy_property (ecma_object_t *object_p, /**< the object */
                                            ecma_string_t *property_name_p) /**< property name */
{
  ecma_unmapped_arguments_t *arguments_p = (ecma_unmapped_arguments_t *) object_p;

  if (ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_LENGTH))
  {
    JERRY_ASSERT (!(arguments_p->header.u.cls.u1.arguments_flags & ECMA_ARGUMENTS_OBJECT_LENGTH_INITIALIZED));

    arguments_p->header.u.cls.u1.arguments_flags |= ECMA_ARGUMENTS_OBJECT_LENGTH_INITIALIZED;
    return;
  }

  if (ecma_compare_ecma_string_to_magic_id (property_name_p, LIT_MAGIC_STRING_CALLEE))
  {
    JERRY_ASSERT (!(arguments_p->header.u.cls.u1.arguments_flags & ECMA_ARGUMENTS_OBJECT_CALLEE_INITIALIZED));
    JERRY_ASSERT (arguments_p->header.u.cls.u1.arguments_flags & ECMA_ARGUMENTS_OBJECT_MAPPED);

    arguments_p->header.u.cls.u1.arguments_flags |= ECMA_ARGUMENTS_OBJECT_CALLEE_INITIALIZED;
    return;
  }

#if JERRY_ESNEXT
  if (ecma_prop_name_is_symbol (property_name_p))
  {
    JERRY_ASSERT (!(arguments_p->header.u.cls.u1.arguments_flags & ECMA_ARGUMENTS_OBJECT_ITERATOR_INITIALIZED));
    JERRY_ASSERT (ecma_op_compare_string_to_global_symbol (property_name_p, LIT_GLOBAL_SYMBOL_ITERATOR));

    arguments_p->header.u.cls.u1.arguments_flags |= ECMA_ARGUMENTS_OBJECT_ITERATOR_INITIALIZED;
    return;
  }
#endif /* JERRY_ESNEXT */

  uint32_t index = ecma_string_get_array_index (property_name_p);

  ecma_value_t *argv_p = (ecma_value_t *) (arguments_p + 1);

  if (arguments_p->header.u.cls.u1.arguments_flags & ECMA_ARGUMENTS_OBJECT_MAPPED)
  {
    argv_p = (ecma_value_t *) (((ecma_mapped_arguments_t *) object_p) + 1);
  }

  JERRY_ASSERT (argv_p[index] == ECMA_VALUE_UNDEFINED || argv_p[index] == ECMA_VALUE_ARGUMENT_NO_TRACK);

  argv_p[index] = ECMA_VALUE_EMPTY;
} /* ecma_arguments_object_delete_lazy_property */

/**
 * @}
 * @}
 */
