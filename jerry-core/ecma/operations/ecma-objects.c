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

#include "ecma-array-object.h"
#include "ecma-builtins.h"
#include "ecma-builtin-helpers.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-function-object.h"
#include "ecma-lex-env.h"
#include "ecma-string-object.h"
#include "ecma-objects-arguments.h"
#include "ecma-objects-general.h"
#include "ecma-objects.h"
#include "ecma-proxy-object.h"
#include "jcontext.h"

#if ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY)
#include "ecma-typedarray-object.h"
#include "ecma-arraybuffer-object.h"
#include "ecma-try-catch-macro.h"
#endif /* ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaobjectsinternalops ECMA objects' operations
 * @{
 */

/**
 * Hash bitmap size for ecma objects
 */
#define ECMA_OBJECT_HASH_BITMAP_SIZE 256

/**
 * Assert that specified object type value is valid
 *
 * @param type object's implementation-defined type
 */
#ifndef JERRY_NDEBUG
#define JERRY_ASSERT_OBJECT_TYPE_IS_VALID(type) \
  JERRY_ASSERT (type < ECMA_OBJECT_TYPE__MAX);
#else /* JERRY_NDEBUG */
#define JERRY_ASSERT_OBJECT_TYPE_IS_VALID(type)
#endif /* !JERRY_NDEBUG */

/**
 * [[GetOwnProperty]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * @return pointer to a property - if it exists,
 *         NULL (i.e. ecma-undefined) - otherwise.
 */
ecma_property_t
ecma_op_object_get_own_property (ecma_object_t *object_p, /**< the object */
                                 ecma_string_t *property_name_p, /**< property name */
                                 ecma_property_ref_t *property_ref_p, /**< property reference */
                                 uint32_t options) /**< option bits */
{
  JERRY_ASSERT (object_p != NULL
                && !ecma_is_lexical_environment (object_p));
#if ENABLED (JERRY_ES2015_BUILTIN_PROXY)
  JERRY_ASSERT (!ECMA_OBJECT_IS_PROXY (object_p));
#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROXY) */
  JERRY_ASSERT (property_name_p != NULL);
  JERRY_ASSERT (options == ECMA_PROPERTY_GET_NO_OPTIONS
                || options == ECMA_PROPERTY_GET_HAS_OWN_PROP
                || property_ref_p != NULL);

  ecma_object_type_t type = ecma_get_object_type (object_p);

  switch (type)
  {
    case ECMA_OBJECT_TYPE_CLASS:
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

      if (ext_object_p->u.class_prop.class_id == LIT_MAGIC_STRING_STRING_UL)
      {
        if (ecma_string_is_length (property_name_p))
        {
          if (options & ECMA_PROPERTY_GET_VALUE)
          {
            ecma_value_t prim_value_p = ext_object_p->u.class_prop.u.value;
            ecma_string_t *prim_value_str_p = ecma_get_string_from_value (prim_value_p);

            ecma_length_t length = ecma_string_get_length (prim_value_str_p);
            property_ref_p->virtual_value = ecma_make_uint32_value (length);
          }

          return ECMA_PROPERTY_TYPE_VIRTUAL;
        }

        uint32_t index = ecma_string_get_array_index (property_name_p);

        if (index != ECMA_STRING_NOT_ARRAY_INDEX)
        {
          ecma_value_t prim_value_p = ext_object_p->u.class_prop.u.value;
          ecma_string_t *prim_value_str_p = ecma_get_string_from_value (prim_value_p);

          if (index < ecma_string_get_length (prim_value_str_p))
          {
            if (options & ECMA_PROPERTY_GET_VALUE)
            {
              ecma_char_t char_at_idx = ecma_string_get_char_at_pos (prim_value_str_p, index);
              ecma_string_t *char_str_p = ecma_new_ecma_string_from_code_unit (char_at_idx);
              property_ref_p->virtual_value = ecma_make_string_value (char_str_p);
            }

            return ECMA_PROPERTY_FLAG_ENUMERABLE | ECMA_PROPERTY_TYPE_VIRTUAL;
          }
        }
      }
      break;
    }
    case ECMA_OBJECT_TYPE_ARRAY:
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

      if (ecma_string_is_length (property_name_p))
      {
        if (options & ECMA_PROPERTY_GET_VALUE)
        {
          property_ref_p->virtual_value = ecma_make_uint32_value (ext_object_p->u.array.length);
        }

        return ext_object_p->u.array.u.length_prop & (ECMA_PROPERTY_TYPE_VIRTUAL | ECMA_PROPERTY_FLAG_WRITABLE);
      }

      if (ecma_op_array_is_fast_array (ext_object_p))
      {
        uint32_t index = ecma_string_get_array_index (property_name_p);

        if (index != ECMA_STRING_NOT_ARRAY_INDEX)
        {
          if (JERRY_LIKELY (index < ext_object_p->u.array.length))
          {
            ecma_value_t *values_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, object_p->u1.property_list_cp);

            if (ecma_is_value_array_hole (values_p[index]))
            {
              return ECMA_PROPERTY_TYPE_NOT_FOUND;
            }

            if (options & ECMA_PROPERTY_GET_VALUE)
            {
              property_ref_p->virtual_value = ecma_fast_copy_value (values_p[index]);
            }

            return (ecma_property_t) (ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE | ECMA_PROPERTY_TYPE_VIRTUAL);
          }
        }

        return ECMA_PROPERTY_TYPE_NOT_FOUND;
      }

      break;
    }
#if ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY)
    case ECMA_OBJECT_TYPE_PSEUDO_ARRAY:
    {
      /* ES2015 9.4.5.1 */
      if (ecma_object_is_typedarray (object_p))
      {
#if ENABLED (JERRY_ES2015)
        if (ecma_prop_name_is_symbol (property_name_p))
        {
          break;
        }
#endif /* ENABLED (JERRY_ES2015) */

        uint32_t array_index = ecma_string_get_array_index (property_name_p);

        if (array_index != ECMA_STRING_NOT_ARRAY_INDEX)
        {
          ecma_typedarray_info_t info = ecma_typedarray_get_info (object_p);
          ecma_value_t value = ECMA_VALUE_UNDEFINED;

          if (array_index < info.length)
          {
            ecma_length_t byte_pos = array_index << info.shift;
            ecma_number_t num = ecma_get_typedarray_element (info.buffer_p + byte_pos, info.id);
            value = ecma_make_number_value (num);
          }

          if (!ecma_is_value_undefined (value))
          {
            if (options & ECMA_PROPERTY_GET_VALUE)
            {
              property_ref_p->virtual_value = value;
            }
            else
            {
              ecma_fast_free_value (value);
            }

            return ECMA_PROPERTY_ENUMERABLE_WRITABLE | ECMA_PROPERTY_TYPE_VIRTUAL;
          }

          return ECMA_PROPERTY_TYPE_NOT_FOUND_AND_STOP;
        }

        ecma_number_t num = ecma_string_to_number (property_name_p);
        ecma_string_t *num_to_str = ecma_new_ecma_string_from_number (num);

        if (ecma_compare_ecma_strings (property_name_p, num_to_str))
        {
          ecma_deref_ecma_string (num_to_str);

          return ECMA_PROPERTY_TYPE_NOT_FOUND_AND_STOP;
        }

        ecma_deref_ecma_string (num_to_str);
      }

      break;
    }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */
    default:
    {
      break;
    }
  }

  ecma_property_t *property_p = ecma_find_named_property (object_p, property_name_p);

  if (property_p == NULL)
  {
    if (ecma_get_object_is_builtin (object_p))
    {
      property_p = ecma_builtin_try_to_instantiate_property (object_p, property_name_p);
    }
    else if (type == ECMA_OBJECT_TYPE_FUNCTION)
    {
      if (ecma_string_is_length (property_name_p))
      {
        if (options & ECMA_PROPERTY_GET_VALUE)
        {
          /* Get length virtual property. */
          ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) object_p;
          const ecma_compiled_code_t *bytecode_data_p = ecma_op_function_get_compiled_code (ext_func_p);

          uint32_t len;
          if (bytecode_data_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
          {
            cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) bytecode_data_p;
            len = args_p->argument_end;
          }
          else
          {
            cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) bytecode_data_p;
            len = args_p->argument_end;
          }

          property_ref_p->virtual_value = ecma_make_uint32_value (len);
        }

        return ECMA_PROPERTY_TYPE_VIRTUAL;
      }

      /* Get prototype physical property. */
      property_p = ecma_op_function_try_to_lazy_instantiate_property (object_p, property_name_p);
    }
    else if (type == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION)
    {
      property_p = ecma_op_external_function_try_to_lazy_instantiate_property (object_p, property_name_p);
    }
    else if (type == ECMA_OBJECT_TYPE_BOUND_FUNCTION)
    {
      property_p = ecma_op_bound_function_try_to_lazy_instantiate_property (object_p, property_name_p);
    }

    if (property_p == NULL)
    {
      return ECMA_PROPERTY_TYPE_NOT_FOUND;
    }
  }
  else if (type == ECMA_OBJECT_TYPE_PSEUDO_ARRAY
           && (options & ECMA_PROPERTY_GET_HAS_OWN_PROP))
  {
    ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

    if (ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_ARGUMENTS)
    {
      uint32_t index = ecma_string_get_array_index (property_name_p);

      if (index != ECMA_STRING_NOT_ARRAY_INDEX
          && index < ext_object_p->u.pseudo_array.u1.length)
      {
        ecma_value_t *arg_Literal_p = (ecma_value_t *) (ext_object_p + 1);

        if (arg_Literal_p[index] != ECMA_VALUE_EMPTY)
        {
          ecma_string_t *arg_name_p = ecma_get_string_from_value (arg_Literal_p[index]);

          ecma_object_t *lex_env_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                                      ext_object_p->u.pseudo_array.u2.lex_env_cp);

          JERRY_ASSERT (lex_env_p != NULL
                        && ecma_is_lexical_environment (lex_env_p));

          ecma_value_t binding_value = ecma_op_get_binding_value (lex_env_p, arg_name_p, true);

          ecma_named_data_property_assign_value (object_p,
                                                 ECMA_PROPERTY_VALUE_PTR (property_p),
                                                 binding_value);
          ecma_free_value (binding_value);
        }
      }
    }
  }

  if (options & ECMA_PROPERTY_GET_EXT_REFERENCE)
  {
    ((ecma_extended_property_ref_t *) property_ref_p)->property_p = property_p;
  }

  if (property_ref_p != NULL)
  {
    property_ref_p->value_p = ECMA_PROPERTY_VALUE_PTR (property_p);
  }

  return *property_p;
} /* ecma_op_object_get_own_property */

/**
 * [[GetProperty]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * @return pointer to a property - if it exists,
 *         NULL (i.e. ecma-undefined) - otherwise.
 */
static ecma_property_t
ecma_op_object_get_property (ecma_object_t *object_p, /**< the object */
                             ecma_string_t *property_name_p, /**< property name */
                             ecma_property_ref_t *property_ref_p, /**< property reference */
                             uint32_t options) /**< option bits */
{
  while (true)
  {
    ecma_property_t property = ecma_op_object_get_own_property (object_p,
                                                                property_name_p,
                                                                property_ref_p,
                                                                options);

    if (property != ECMA_PROPERTY_TYPE_NOT_FOUND && property != ECMA_PROPERTY_TYPE_NOT_FOUND_AND_STOP)
    {
      return property;
    }

    if (property == ECMA_PROPERTY_TYPE_NOT_FOUND_AND_STOP)
    {
      break;
    }

    if (object_p->u2.prototype_cp == JMEM_CP_NULL)
    {
      break;
    }

    object_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, object_p->u2.prototype_cp);
  }

  return ECMA_PROPERTY_TYPE_NOT_FOUND;
} /* ecma_op_object_get_property */

/**
 * Generic [[HasProperty]] operation
 *
 * See also:
 *          ECMAScript v6, 9.1.7.1
 *
 * @return ECMA_VALUE_ERROR - if the operation fails
 *         ECMA_VALUE_{TRUE_FALSE} - whether the property is found
 */
inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_op_object_has_property (ecma_object_t *object_p, /**< the object */
                             ecma_string_t *property_name_p) /**< property name */
{
  while (true)
  {
#if ENABLED (JERRY_ES2015_BUILTIN_PROXY)
    if (ECMA_OBJECT_IS_PROXY (object_p))
    {
      return ecma_proxy_object_has (object_p, property_name_p);
    }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROXY) */

    /* 2 - 3. */
    if (ecma_op_ordinary_object_has_own_property (object_p, property_name_p))
    {
      return ECMA_VALUE_TRUE;
    }

    jmem_cpointer_t proto_cp = ecma_op_ordinary_object_get_prototype_of (object_p);

    /* 7. */
    if (proto_cp == JMEM_CP_NULL)
    {
      return ECMA_VALUE_FALSE;
    }

    object_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, proto_cp);
  }
} /* ecma_op_object_has_property */

/**
 * Search the value corresponding to a property name
 *
 * Note: search includes prototypes
 *
 * @return ecma value if property is found
 *         ECMA_VALUE_NOT_FOUND if property is not found
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_object_find_own (ecma_value_t base_value, /**< base value */
                         ecma_object_t *object_p, /**< target object */
                         ecma_string_t *property_name_p) /**< property name */
{
  JERRY_ASSERT (object_p != NULL
                && !ecma_is_lexical_environment (object_p));
  JERRY_ASSERT (property_name_p != NULL);
  JERRY_ASSERT (!ECMA_OBJECT_IS_PROXY (object_p));

  ecma_object_type_t type = ecma_get_object_type (object_p);

  switch (type)
  {
    case ECMA_OBJECT_TYPE_CLASS:
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

      if (ext_object_p->u.class_prop.class_id == LIT_MAGIC_STRING_STRING_UL)
      {
        if (ecma_string_is_length (property_name_p))
        {
          ecma_value_t prim_value_p = ext_object_p->u.class_prop.u.value;

          ecma_string_t *prim_value_str_p = ecma_get_string_from_value (prim_value_p);
          ecma_length_t length = ecma_string_get_length (prim_value_str_p);

          return ecma_make_uint32_value (length);
        }

        uint32_t index = ecma_string_get_array_index (property_name_p);

        if (index != ECMA_STRING_NOT_ARRAY_INDEX)
        {
          ecma_value_t prim_value_p = ext_object_p->u.class_prop.u.value;

          ecma_string_t *prim_value_str_p = ecma_get_string_from_value (prim_value_p);

          if (index < ecma_string_get_length (prim_value_str_p))
          {
            ecma_char_t char_at_idx = ecma_string_get_char_at_pos (prim_value_str_p, index);
            return ecma_make_string_value (ecma_new_ecma_string_from_code_unit (char_at_idx));
          }
        }
      }

      break;
    }
    case ECMA_OBJECT_TYPE_ARRAY:
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

      if (ecma_string_is_length (property_name_p))
      {
        return ecma_make_uint32_value (ext_object_p->u.array.length);
      }

      if (JERRY_LIKELY (ecma_op_array_is_fast_array (ext_object_p)))
      {
        uint32_t index = ecma_string_get_array_index (property_name_p);

        if (JERRY_LIKELY (index != ECMA_STRING_NOT_ARRAY_INDEX))
        {
          if (JERRY_LIKELY (index < ext_object_p->u.array.length))
          {
            ecma_value_t *values_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, object_p->u1.property_list_cp);

            return (ecma_is_value_array_hole (values_p[index]) ? ECMA_VALUE_NOT_FOUND
                                                               : ecma_fast_copy_value (values_p[index]));
          }
        }
        return ECMA_VALUE_NOT_FOUND;
      }

      break;
    }
    case ECMA_OBJECT_TYPE_PSEUDO_ARRAY:
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

      if (ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_ARGUMENTS)
      {
        uint32_t index = ecma_string_get_array_index (property_name_p);

        if (index != ECMA_STRING_NOT_ARRAY_INDEX
            && index < ext_object_p->u.pseudo_array.u1.length)
        {
          ecma_value_t *arg_Literal_p = (ecma_value_t *) (ext_object_p + 1);

          if (arg_Literal_p[index] != ECMA_VALUE_EMPTY)
          {
            ecma_string_t *arg_name_p = ecma_get_string_from_value (arg_Literal_p[index]);

            ecma_object_t *lex_env_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                                        ext_object_p->u.pseudo_array.u2.lex_env_cp);

            JERRY_ASSERT (lex_env_p != NULL
                          && ecma_is_lexical_environment (lex_env_p));

            return ecma_op_get_binding_value (lex_env_p, arg_name_p, true);
          }
        }
      }
#if ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY)
      /* ES2015 9.4.5.4 */
      if (ecma_object_is_typedarray (object_p))
      {
#if ENABLED (JERRY_ES2015)
        if (ecma_prop_name_is_symbol (property_name_p))
        {
          break;
        }
#endif /* ENABLED (JERRY_ES2015) */

        uint32_t array_index = ecma_string_get_array_index (property_name_p);

        if (array_index != ECMA_STRING_NOT_ARRAY_INDEX)
        {
          ecma_typedarray_info_t info = ecma_typedarray_get_info (object_p);

          if (array_index >= info.length)
          {
            return ECMA_VALUE_UNDEFINED;
          }

          ecma_length_t byte_pos = array_index << info.shift;
          ecma_number_t num = ecma_get_typedarray_element (info.buffer_p + byte_pos, info.id);
          return ecma_make_number_value (num);
        }

        ecma_number_t num = ecma_string_to_number (property_name_p);
        ecma_string_t *num_to_str = ecma_new_ecma_string_from_number (num);

        if (ecma_compare_ecma_strings (property_name_p, num_to_str))
        {
          ecma_deref_ecma_string (num_to_str);

          return ECMA_VALUE_UNDEFINED;
        }

        ecma_deref_ecma_string (num_to_str);
      }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */

      break;
    }
    default:
    {
      break;
    }
  }

  ecma_property_t *property_p = ecma_find_named_property (object_p, property_name_p);

  if (property_p == NULL)
  {
    if (ecma_get_object_is_builtin (object_p))
    {
      property_p = ecma_builtin_try_to_instantiate_property (object_p, property_name_p);
    }
    else if (type == ECMA_OBJECT_TYPE_FUNCTION)
    {
      if (ecma_string_is_length (property_name_p))
      {
        /* Get length virtual property. */
        ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) object_p;
        const ecma_compiled_code_t *bytecode_data_p = ecma_op_function_get_compiled_code (ext_func_p);

        uint32_t len;
        if (bytecode_data_p->status_flags & CBC_CODE_FLAGS_UINT16_ARGUMENTS)
        {
          cbc_uint16_arguments_t *args_p = (cbc_uint16_arguments_t *) bytecode_data_p;
          len = args_p->argument_end;
        }
        else
        {
          cbc_uint8_arguments_t *args_p = (cbc_uint8_arguments_t *) bytecode_data_p;
          len = args_p->argument_end;
        }

        return ecma_make_uint32_value (len);
      }

      /* Get prototype physical property. */
      property_p = ecma_op_function_try_to_lazy_instantiate_property (object_p, property_name_p);
    }
    else if (type == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION)
    {
      property_p = ecma_op_external_function_try_to_lazy_instantiate_property (object_p, property_name_p);
    }
    else if (type == ECMA_OBJECT_TYPE_BOUND_FUNCTION)
    {
      property_p = ecma_op_bound_function_try_to_lazy_instantiate_property (object_p, property_name_p);
    }

    if (property_p == NULL)
    {
      return ECMA_VALUE_NOT_FOUND;
    }
  }

  ecma_property_value_t *prop_value_p = ECMA_PROPERTY_VALUE_PTR (property_p);

  if (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA)
  {
    return ecma_fast_copy_value (prop_value_p->value);
  }

  JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR);

  ecma_getter_setter_pointers_t *get_set_pair_p = ecma_get_named_accessor_property (prop_value_p);

  if (get_set_pair_p->getter_cp == JMEM_CP_NULL)
  {
    return ECMA_VALUE_UNDEFINED;
  }

  ecma_object_t *getter_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, get_set_pair_p->getter_cp);

  return ecma_op_function_call (getter_p, base_value, NULL, 0);
} /* ecma_op_object_find_own */

/**
 * Search the value corresponding to an uint32_t property index
 *
 * Note: this method falls back to the general ecma_op_object_find
 *
 * @return ecma value if property is found
 *         ECMA_VALUE_NOT_FOUND if property is not found
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_object_find_by_uint32_index (ecma_object_t *object_p, /**< the object */
                                     uint32_t index) /**< property index */
{
  if (JERRY_LIKELY (index <= ECMA_DIRECT_STRING_MAX_IMM))
  {
    return ecma_op_object_find (object_p, ECMA_CREATE_DIRECT_UINT32_STRING (index));
  }

  ecma_string_t *index_str_p = ecma_new_non_direct_string_from_uint32 (index);
  ecma_value_t ret_value = ecma_op_object_find (object_p, index_str_p);
  ecma_deref_ecma_string (index_str_p);

  return ret_value;
} /* ecma_op_object_find_by_uint32_index */

/**
 * Search the value corresponding to an ecma_number_t property index
 *
 * Note: this method falls back to the general ecma_op_object_find
 *
 * @return ecma value if property is found
 *         ECMA_VALUE_NOT_FOUND if property is not found
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_object_find_by_number_index (ecma_object_t *object_p, /**< the object */
                                     ecma_number_t index) /**< property index */
{
  ecma_string_t *index_str_p = ecma_new_ecma_string_from_number (index);
  ecma_value_t ret_value = ecma_op_object_find (object_p, index_str_p);
  ecma_deref_ecma_string (index_str_p);

  return ret_value;
} /* ecma_op_object_find_by_number_index */

/**
 * Search the value corresponding to a property name
 *
 * Note: search includes prototypes
 *
 * @return ecma value if property is found
 *         ECMA_VALUE_NOT_FOUND if property is not found
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_object_find (ecma_object_t *object_p, /**< the object */
                     ecma_string_t *property_name_p) /**< property name */
{
  ecma_value_t base_value = ecma_make_object_value (object_p);

  while (true)
  {
#if ENABLED (JERRY_ES2015_BUILTIN_PROXY)
    if (ECMA_OBJECT_IS_PROXY (object_p))
    {
      return ecma_proxy_object_find (object_p, property_name_p);
    }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROXY) */

    ecma_value_t value = ecma_op_object_find_own (base_value, object_p, property_name_p);

    if (ecma_is_value_found (value))
    {
      return value;
    }

    if (object_p->u2.prototype_cp == JMEM_CP_NULL)
    {
      break;
    }

    object_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, object_p->u2.prototype_cp);
  }

  return ECMA_VALUE_NOT_FOUND;
} /* ecma_op_object_find */

/**
 * Get own property by name
 *
 * Note: property must be an existing data property
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_op_object_get_own_data_prop (ecma_object_t *object_p, /**< the object */
                                  ecma_string_t *property_name_p) /**< property name */
{
  JERRY_ASSERT (ecma_is_lexical_environment (object_p)
                || !ecma_op_object_is_fast_array (object_p));

  ecma_value_t result = ecma_op_object_find_own (ecma_make_object_value (object_p),
                                                 object_p,
                                                 property_name_p);

#ifndef JERRY_NDEBUG
  /* Because ecma_op_object_find_own might create a property
   * this check is executed after the function return. */
  ecma_property_t *property_p = ecma_find_named_property (object_p,
                                                          property_name_p);

  JERRY_ASSERT (property_p != NULL
                && ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA
                && !ecma_is_property_configurable (*property_p));
#endif /* !JERRY_NDEBUG */

  return result;
} /* ecma_op_object_get_own_data_prop */

/**
 * [[Get]] operation of ecma object
 *
 * This function returns the value of a named property, or undefined
 * if the property is not found in the prototype chain. If the property
 * is an accessor, it calls the "get" callback function and returns
 * with its result (including error throws).
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_op_object_get (ecma_object_t *object_p, /**< the object */
                    ecma_string_t *property_name_p) /**< property name */
{
  return ecma_op_object_get_with_receiver (object_p, property_name_p, ecma_make_object_value (object_p));
} /* ecma_op_object_get */

/**
 * [[Get]] operation of ecma object with the specified receiver
 *
 * This function returns the value of a named property, or undefined
 * if the property is not found in the prototype chain. If the property
 * is an accessor, it calls the "get" callback function and returns
 * with its result (including error throws).
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_object_get_with_receiver (ecma_object_t *object_p, /**< the object */
                                  ecma_string_t *property_name_p, /**< property name */
                                  ecma_value_t receiver) /**< receiver to invoke getter function */
{
  while (true)
  {
#if ENABLED (JERRY_ES2015_BUILTIN_PROXY)
    if (ECMA_OBJECT_IS_PROXY (object_p))
    {
      return ecma_proxy_object_get (object_p, property_name_p, receiver);
    }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROXY) */

    ecma_value_t value = ecma_op_object_find_own (receiver, object_p, property_name_p);

    if (ecma_is_value_found (value))
    {
      return value;
    }

    jmem_cpointer_t proto_cp = ecma_op_ordinary_object_get_prototype_of (object_p);

    if (proto_cp == JMEM_CP_NULL)
    {
      break;
    }

    object_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, proto_cp);
  }

  return ECMA_VALUE_UNDEFINED;
} /* ecma_op_object_get_with_receiver */

/**
 * [[Get]] operation of ecma object specified for uint32_t property index
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_object_get_by_uint32_index (ecma_object_t *object_p, /**< the object */
                                    uint32_t index) /**< property index */
{
  if (JERRY_LIKELY (index <= ECMA_DIRECT_STRING_MAX_IMM))
  {
    return ecma_op_object_get (object_p, ECMA_CREATE_DIRECT_UINT32_STRING (index));
  }

  ecma_string_t *index_str_p = ecma_new_non_direct_string_from_uint32 (index);
  ecma_value_t ret_value = ecma_op_object_get (object_p, index_str_p);
  ecma_deref_ecma_string (index_str_p);

  return ret_value;
} /* ecma_op_object_get_by_uint32_index */

/**
 * Perform ToLength(O.[[Get]]("length")) operation
 *
 * The property is converted to uint32 during the operation
 *
 * @return ECMA_VALUE_ERROR - if there was any error during the operation
 *         ECMA_VALUE_EMPTY - otherwise
 */
ecma_value_t
ecma_op_object_get_length (ecma_object_t *object_p, /**< the object */
                           uint32_t *length_p) /**< [out] length value converted to uint32 */
{
  if (JERRY_LIKELY (ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_ARRAY))
  {
    *length_p = ecma_array_get_length (object_p);
    return ECMA_VALUE_EMPTY;
  }

#if ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY)
  if (ecma_object_is_typedarray (object_p))
  {
    *length_p = ecma_typedarray_get_length (object_p);
    return ECMA_VALUE_EMPTY;
  }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */

  ecma_value_t len_value = ecma_op_object_get_by_magic_id (object_p, LIT_MAGIC_STRING_LENGTH);
  ecma_value_t len_number = ecma_op_to_length (len_value, length_p);
  ecma_free_value (len_value);

  JERRY_ASSERT (ECMA_IS_VALUE_ERROR (len_number) || ecma_is_value_empty (len_number));

  return len_number;
} /* ecma_op_object_get_length */

/**
 * [[Get]] operation of ecma object where the property name is a magic string
 *
 * This function returns the value of a named property, or undefined
 * if the property is not found in the prototype chain. If the property
 * is an accessor, it calls the "get" callback function and returns
 * with its result (including error throws).
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_op_object_get_by_magic_id (ecma_object_t *object_p, /**< the object */
                                lit_magic_string_id_t property_id) /**< property magic string id */
{
  return ecma_op_object_get (object_p, ecma_get_magic_string (property_id));
} /* ecma_op_object_get_by_magic_id */

#if ENABLED (JERRY_ES2015)
/**
 * [[Get]] a well-known symbol by the given property id
 *
 * @return pointer to the requested well-known symbol
 */
ecma_string_t *
ecma_op_get_global_symbol (lit_magic_string_id_t property_id) /**< property symbol id */
{
  ecma_value_t symbol_value = ecma_op_object_get_by_magic_id (ecma_builtin_get (ECMA_BUILTIN_ID_INTRINSIC_OBJECT),
                                                              property_id);
  JERRY_ASSERT (ecma_is_value_symbol (symbol_value));

  return ecma_get_symbol_from_value (symbol_value);
} /* ecma_op_get_global_symbol */

/**
 * [[Get]] operation of ecma object where the property is a well-known symbol
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_object_get_by_symbol_id (ecma_object_t *object_p, /**< the object */
                                 lit_magic_string_id_t property_id) /**< property symbol id */
{
  ecma_string_t *symbol_p = ecma_op_get_global_symbol (property_id);
  ecma_value_t ret_value = ecma_op_object_get (object_p, symbol_p);
  ecma_deref_ecma_string (symbol_p);

  return ret_value;
} /* ecma_op_object_get_by_symbol_id */

/**
 * GetMethod operation
 *
 * See also: ECMA-262 v6, 7.3.9
 *
 * Note:
 *      Returned value must be freed with ecma_free_value.
 *
 * @return iterator function object - if success
 *         raised error - otherwise
 */
static ecma_value_t
ecma_op_get_method (ecma_value_t value, /**< ecma value */
                    ecma_string_t *prop_name_p) /** property name */
{
  /* 2. */
  ecma_value_t obj_value = ecma_op_to_object (value);

  if (ECMA_IS_VALUE_ERROR (obj_value))
  {
    return obj_value;
  }

  ecma_object_t *obj_p = ecma_get_object_from_value (obj_value);
  ecma_value_t func;

  func = ecma_op_object_get (obj_p, prop_name_p);
  ecma_deref_object (obj_p);

  /* 3. */
  if (ECMA_IS_VALUE_ERROR (func))
  {
    return func;
  }

  /* 4. */
  if (ecma_is_value_undefined (func) || ecma_is_value_null (func))
  {
    return ECMA_VALUE_UNDEFINED;
  }

  /* 5. */
  if (!ecma_op_is_callable (func))
  {
    ecma_free_value (func);
    return ecma_raise_type_error (ECMA_ERR_MSG ("Iterator is not callable."));
  }

  /* 6. */
  return func;
} /* ecma_op_get_method */

/**
 * GetMethod operation when the property is a well-known symbol
 *
 * See also: ECMA-262 v6, 7.3.9
 *
 * Note:
 *      Returned value must be freed with ecma_free_value.
 *
 * @return iterator function object - if success
 *         raised error - otherwise
 */
ecma_value_t
ecma_op_get_method_by_symbol_id (ecma_value_t value, /**< ecma value */
                                 lit_magic_string_id_t symbol_id) /**< property symbol id */
{
  ecma_string_t *prop_name_p = ecma_op_get_global_symbol (symbol_id);
  ecma_value_t ret_value = ecma_op_get_method (value, prop_name_p);
  ecma_deref_ecma_string (prop_name_p);

  return ret_value;
} /* ecma_op_get_method_by_symbol_id */

/**
 * GetMethod operation when the property is a magic string
 *
 * See also: ECMA-262 v6, 7.3.9
 *
 * Note:
 *      Returned value must be freed with ecma_free_value.
 *
 * @return iterator function object - if success
 *         raised error - otherwise
 */
ecma_value_t
ecma_op_get_method_by_magic_id (ecma_value_t value, /**< ecma value */
                                lit_magic_string_id_t magic_id) /**< property magic id */
{
  return ecma_op_get_method (value, ecma_get_magic_string (magic_id));
} /* ecma_op_get_method_by_magic_id */
#endif /* ENABLED (JERRY_ES2015) */

/**
 * [[Put]] ecma general object's operation specialized for uint32_ property index
 *
 * Note: This function falls back to the general ecma_op_object_put
 *
 * @return ecma value
 *         The returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_object_put_by_uint32_index (ecma_object_t *object_p, /**< the object */
                                    uint32_t index, /**< property index */
                                    ecma_value_t value, /**< ecma value */
                                    bool is_throw) /**< flag that controls failure handling */
{
  if (JERRY_LIKELY (index <= ECMA_DIRECT_STRING_MAX_IMM))
  {
    return ecma_op_object_put (object_p,
                               ECMA_CREATE_DIRECT_UINT32_STRING (index),
                               value,
                               is_throw);
  }

  ecma_string_t *index_str_p = ecma_new_non_direct_string_from_uint32 (index);
  ecma_value_t ret_value = ecma_op_object_put (object_p, index_str_p, value, is_throw);
  ecma_deref_ecma_string (index_str_p);

  return ret_value;
} /* ecma_op_object_put_by_uint32_index */

/**
 * [[Put]] ecma general object's operation specialized for ecma_number_ property index
 *
 * Note: This function falls back to the general ecma_op_object_put
 *
 * @return ecma value
 *         The returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_op_object_put_by_number_index (ecma_object_t *object_p, /**< the object */
                                    ecma_number_t index, /**< property index */
                                    ecma_value_t value, /**< ecma value */
                                    bool is_throw) /**< flag that controls failure handling */
{
  ecma_string_t *index_str_p = ecma_new_ecma_string_from_number (index);
  ecma_value_t ret_value = ecma_op_object_put (object_p, index_str_p, value, is_throw);
  ecma_deref_ecma_string (index_str_p);

  return ret_value;
} /* ecma_op_object_put_by_number_index */

/**
 * [[Put]] ecma general object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 8.12.5
 *          Also incorporates [[CanPut]] ECMA-262 v5, 8.12.4
 *
 * @return ecma value
 *         The returned value must be freed with ecma_free_value.
 *
 *         Returns with ECMA_VALUE_TRUE if the operation is
 *         successful. Otherwise it returns with an error object
 *         or ECMA_VALUE_FALSE.
 *
 *         Note: even if is_throw is false, the setter can throw an
 *         error, and this function returns with that error.
 */
inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_op_object_put (ecma_object_t *object_p, /**< the object */
                    ecma_string_t *property_name_p, /**< property name */
                    ecma_value_t value, /**< ecma value */
                    bool is_throw) /**< flag that controls failure handling */
{
  return ecma_op_object_put_with_receiver (object_p,
                                           property_name_p,
                                           value,
                                           ecma_make_object_value (object_p),
                                           is_throw);
} /* ecma_op_object_put */

/**
 * [[Put]] ecma general object's operation with given receiver
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *          ECMA-262 v5, 8.12.5
 *          Also incorporates [[CanPut]] ECMA-262 v5, 8.12.4
 *
 * @return ecma value
 *         The returned value must be freed with ecma_free_value.
 *
 *         Returns with ECMA_VALUE_TRUE if the operation is
 *         successful. Otherwise it returns with an error object
 *         or ECMA_VALUE_FALSE.
 *
 *         Note: even if is_throw is false, the setter can throw an
 *         error, and this function returns with that error.
 */
ecma_value_t
ecma_op_object_put_with_receiver (ecma_object_t *object_p, /**< the object */
                                  ecma_string_t *property_name_p, /**< property name */
                                  ecma_value_t value, /**< ecma value */
                                  ecma_value_t receiver, /**< receiver */
                                  bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT (object_p != NULL
                && !ecma_is_lexical_environment (object_p));
  JERRY_ASSERT (property_name_p != NULL);

#if ENABLED (JERRY_ES2015_BUILTIN_PROXY)
  if (ECMA_OBJECT_IS_PROXY (object_p))
  {
    return ecma_proxy_object_set (object_p, property_name_p, value, receiver);
  }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROXY) */

  ecma_object_type_t type = ecma_get_object_type (object_p);

  switch (type)
  {
    case ECMA_OBJECT_TYPE_ARRAY:
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

      if (ecma_string_is_length (property_name_p))
      {
        if (ecma_is_property_writable (ext_object_p->u.array.u.length_prop))
        {
          return ecma_op_array_object_set_length (object_p, value, 0);
        }

        return ecma_reject (is_throw);
      }

      if (JERRY_LIKELY (ecma_op_array_is_fast_array (ext_object_p)))
      {
        if (JERRY_UNLIKELY (!ecma_op_ordinary_object_is_extensible (object_p)))
        {
          return ecma_reject (is_throw);
        }

        uint32_t index = ecma_string_get_array_index (property_name_p);

        if (JERRY_UNLIKELY (index == ECMA_STRING_NOT_ARRAY_INDEX))
        {
          ecma_fast_array_convert_to_normal (object_p);
        }
        else if (ecma_fast_array_set_property (object_p, index, value))
        {
          return ECMA_VALUE_TRUE;
        }
      }

      JERRY_ASSERT (!ecma_op_object_is_fast_array (object_p));

      break;
    }
    case ECMA_OBJECT_TYPE_PSEUDO_ARRAY:
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

      if (ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_ARGUMENTS)
      {
        uint32_t index = ecma_string_get_array_index (property_name_p);

        if (index != ECMA_STRING_NOT_ARRAY_INDEX
            && index < ext_object_p->u.pseudo_array.u1.length)
        {
          ecma_value_t *arg_Literal_p = (ecma_value_t *) (ext_object_p + 1);

          if (arg_Literal_p[index] != ECMA_VALUE_EMPTY)
          {
            ecma_string_t *arg_name_p = ecma_get_string_from_value (arg_Literal_p[index]);

            ecma_object_t *lex_env_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                                        ext_object_p->u.pseudo_array.u2.lex_env_cp);

            JERRY_ASSERT (lex_env_p != NULL
                          && ecma_is_lexical_environment (lex_env_p));

            ecma_op_set_mutable_binding (lex_env_p, arg_name_p, value, true);
            return ECMA_VALUE_TRUE;
          }
        }
      }
#if ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY)
      /* ES2015 9.4.5.5 */
      if (ecma_object_is_typedarray (object_p))
      {
#if ENABLED (JERRY_ES2015)
        if (ecma_prop_name_is_symbol (property_name_p))
        {
          break;
        }
#endif /* ENABLED (JERRY_ES2015) */

        uint32_t array_index = ecma_string_get_array_index (property_name_p);

        if (array_index != ECMA_STRING_NOT_ARRAY_INDEX)
        {
          ecma_number_t num_var;
          ecma_value_t error = ecma_get_number (value, &num_var);

          if (ECMA_IS_VALUE_ERROR (error))
          {
            jcontext_release_exception ();
            return ecma_reject (is_throw);
          }

          ecma_typedarray_info_t info = ecma_typedarray_get_info (object_p);

          if (array_index >= info.length)
          {
            return ecma_reject (is_throw);
          }

          ecma_length_t byte_pos = array_index << info.shift;
          ecma_set_typedarray_element (info.buffer_p + byte_pos, num_var, info.id);

          return ECMA_VALUE_TRUE;
        }

        ecma_number_t num = ecma_string_to_number (property_name_p);
        ecma_string_t *num_to_str = ecma_new_ecma_string_from_number (num);

        if (ecma_compare_ecma_strings (property_name_p, num_to_str))
        {
          ecma_deref_ecma_string (num_to_str);

          return ecma_reject (is_throw);
        }

        ecma_deref_ecma_string (num_to_str);
      }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */
      break;
    }
    default:
    {
      break;
    }
  }

  ecma_property_t *property_p = ecma_find_named_property (object_p, property_name_p);

  if (property_p == NULL)
  {
    if (type == ECMA_OBJECT_TYPE_CLASS)
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

      if (ext_object_p->u.class_prop.class_id == LIT_MAGIC_STRING_STRING_UL)
      {
        uint32_t index = ecma_string_get_array_index (property_name_p);

        if (index != ECMA_STRING_NOT_ARRAY_INDEX)
        {
          ecma_value_t prim_value_p = ext_object_p->u.class_prop.u.value;
          ecma_string_t *prim_value_str_p = ecma_get_string_from_value (prim_value_p);

          if (index < ecma_string_get_length (prim_value_str_p))
          {
            return ecma_reject (is_throw);
          }
        }
      }
    }

    if (ecma_get_object_is_builtin (object_p))
    {
      property_p = ecma_builtin_try_to_instantiate_property (object_p, property_name_p);
    }
    else if (type == ECMA_OBJECT_TYPE_FUNCTION)
    {
      if (ecma_string_is_length (property_name_p))
      {
        return ecma_reject (is_throw);
      }

      /* Get prototype physical property. */
      property_p = ecma_op_function_try_to_lazy_instantiate_property (object_p, property_name_p);
    }
    else if (type == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION)
    {
      property_p = ecma_op_external_function_try_to_lazy_instantiate_property (object_p, property_name_p);
    }
    else if (type == ECMA_OBJECT_TYPE_BOUND_FUNCTION)
    {
      property_p = ecma_op_bound_function_try_to_lazy_instantiate_property (object_p, property_name_p);
    }
  }

  jmem_cpointer_t setter_cp = JMEM_CP_NULL;

  if (property_p != NULL)
  {
    if (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA)
    {
      if (ecma_is_property_writable (*property_p))
      {
        /* There is no need for special casing arrays here because changing the
         * value of an existing property never changes the length of an array. */
        ecma_named_data_property_assign_value (object_p,
                                               ECMA_PROPERTY_VALUE_PTR (property_p),
                                               value);
        return ECMA_VALUE_TRUE;
      }
    }
    else
    {
      JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR);

      ecma_getter_setter_pointers_t *get_set_pair_p;
      get_set_pair_p = ecma_get_named_accessor_property (ECMA_PROPERTY_VALUE_PTR (property_p));
      setter_cp = get_set_pair_p->setter_cp;
    }
  }
  else
  {
    bool create_new_property = true;

    jmem_cpointer_t proto_cp = ecma_op_ordinary_object_get_prototype_of (object_p);

    if (proto_cp != JMEM_CP_NULL)
    {
      ecma_property_ref_t property_ref = { NULL };
      ecma_object_t *proto_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, proto_cp);

      ecma_property_t inherited_property = ecma_op_object_get_property (proto_p,
                                                                        property_name_p,
                                                                        &property_ref,
                                                                        ECMA_PROPERTY_GET_NO_OPTIONS);

      if (inherited_property != ECMA_PROPERTY_TYPE_NOT_FOUND)
      {
        if (ECMA_PROPERTY_GET_TYPE (inherited_property) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR)
        {
          setter_cp = ecma_get_named_accessor_property (property_ref.value_p)->setter_cp;
          create_new_property = false;
        }
        else
        {
          create_new_property = ecma_is_property_writable (inherited_property);
        }
      }
    }

    if (create_new_property
        && ecma_op_ordinary_object_is_extensible (object_p))
    {
      const ecma_object_type_t obj_type = ecma_get_object_type (object_p);

      if (obj_type == ECMA_OBJECT_TYPE_PSEUDO_ARRAY)
      {
        ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

        if (ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_ARGUMENTS)
        {
          return ecma_builtin_helper_def_prop (object_p,
                                               property_name_p,
                                               value,
                                               ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE | ECMA_IS_THROW);
        }
      }

      uint32_t index = ecma_string_get_array_index (property_name_p);

      if (obj_type == ECMA_OBJECT_TYPE_ARRAY
          && index != ECMA_STRING_NOT_ARRAY_INDEX)
      {
        ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

        if (index < UINT32_MAX
            && index >= ext_object_p->u.array.length)
        {
          if (!ecma_is_property_writable (ext_object_p->u.array.u.length_prop))
          {
            return ecma_reject (is_throw);
          }

          ext_object_p->u.array.length = index + 1;
        }
      }

      ecma_property_value_t *new_prop_value_p;
      new_prop_value_p = ecma_create_named_data_property (object_p,
                                                          property_name_p,
                                                          ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE,
                                                          NULL);

      JERRY_ASSERT (ecma_is_value_undefined (new_prop_value_p->value));
      new_prop_value_p->value = ecma_copy_value_if_not_object (value);
      return ECMA_VALUE_TRUE;
    }
  }

  if (setter_cp == JMEM_CP_NULL)
  {
    return ecma_reject (is_throw);
  }

  ecma_value_t ret_value = ecma_op_function_call (ECMA_GET_NON_NULL_POINTER (ecma_object_t, setter_cp),
                                                  receiver,
                                                  &value,
                                                  1);

  if (!ECMA_IS_VALUE_ERROR (ret_value))
  {
    ecma_fast_free_value (ret_value);
    ret_value = ECMA_VALUE_TRUE;
  }

  return ret_value;
} /* ecma_op_object_put_with_receiver */

/**
 * [[Delete]] ecma object's operation specialized for uint32_t property index
 *
 * Note:
 *      This method falls back to the general ecma_op_object_delete
 *
 * @return true - if deleted successfully
 *         false - or type error otherwise (based in 'is_throw')
 */
ecma_value_t
ecma_op_object_delete_by_uint32_index (ecma_object_t *obj_p, /**< the object */
                                       uint32_t index, /**< property index */
                                       bool is_throw) /**< flag that controls failure handling */
{
  if (JERRY_LIKELY (index <= ECMA_DIRECT_STRING_MAX_IMM))
  {
    return ecma_op_object_delete (obj_p, ECMA_CREATE_DIRECT_UINT32_STRING (index), is_throw);
  }

  ecma_string_t *index_str_p = ecma_new_non_direct_string_from_uint32 (index);
  ecma_value_t ret_value = ecma_op_object_delete (obj_p, index_str_p, is_throw);
  ecma_deref_ecma_string (index_str_p);

  return ret_value;
} /* ecma_op_object_delete_by_uint32_index */

/**
 * [[Delete]] ecma object's operation specialized for ecma_number_t property index
 *
 * Note:
 *      This method falls back to the general ecma_op_object_delete
 *
 * @return true - if deleted successfully
 *         false - or type error otherwise (based in 'is_throw')
 */
ecma_value_t
ecma_op_object_delete_by_number_index (ecma_object_t *obj_p, /**< the object */
                                       ecma_number_t index, /**< property index */
                                       bool is_throw) /**< flag that controls failure handling */
{
  ecma_string_t *index_str_p = ecma_new_ecma_string_from_number (index);
  ecma_value_t ret_value = ecma_op_object_delete (obj_p, index_str_p, is_throw);
  ecma_deref_ecma_string (index_str_p);

  return ret_value;
} /* ecma_op_object_delete_by_number_index */

/**
 * [[Delete]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * Note:
 *      returned value must be freed with ecma_free_value
 *
 * @return true - if deleted successfully
 *         false - or type error otherwise (based in 'is_throw')
 */
ecma_value_t
ecma_op_object_delete (ecma_object_t *obj_p, /**< the object */
                       ecma_string_t *property_name_p, /**< property name */
                       bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  if (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_PSEUDO_ARRAY)
  {
    ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

    if (ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_ARGUMENTS)
    {
      return ecma_op_arguments_object_delete (obj_p,
                                              property_name_p,
                                              is_throw);
    }
  }

#if ENABLED (JERRY_ES2015_BUILTIN_PROXY)
  if (ECMA_OBJECT_IS_PROXY (obj_p))
  {
    return ecma_proxy_object_delete_property (obj_p, property_name_p);
  }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROXY) */

  JERRY_ASSERT_OBJECT_TYPE_IS_VALID (ecma_get_object_type (obj_p));

  return ecma_op_general_object_delete (obj_p,
                                        property_name_p,
                                        is_throw);
} /* ecma_op_object_delete */

/**
 * [[DefaultValue]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_object_default_value (ecma_object_t *obj_p, /**< the object */
                              ecma_preferred_type_hint_t hint) /**< hint on preferred result type */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));

  JERRY_ASSERT_OBJECT_TYPE_IS_VALID (ecma_get_object_type (obj_p));

  /*
   * typedef ecma_property_t * (*default_value_ptr_t) (ecma_object_t *, ecma_string_t *);
   * static const default_value_ptr_t default_value [ECMA_OBJECT_TYPE__COUNT] =
   * {
   *   [ECMA_OBJECT_TYPE_GENERAL]           = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_CLASS]             = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_FUNCTION]          = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION] = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_ARRAY]             = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_BOUND_FUNCTION]    = &ecma_op_general_object_default_value,
   *   [ECMA_OBJECT_TYPE_PSEUDO_ARRAY]      = &ecma_op_general_object_default_value
   * };
   *
   * return default_value[type] (obj_p, property_name_p);
   */

  return ecma_op_general_object_default_value (obj_p, hint);
} /* ecma_op_object_default_value */

/**
 * [[DefineOwnProperty]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_object_define_own_property (ecma_object_t *obj_p, /**< the object */
                                    ecma_string_t *property_name_p, /**< property name */
                                    const ecma_property_descriptor_t *property_desc_p) /**< property
                                                                                        *   descriptor */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  const ecma_object_type_t type = ecma_get_object_type (obj_p);

#if ENABLED (JERRY_ES2015_BUILTIN_PROXY)
  if (ECMA_OBJECT_IS_PROXY (obj_p))
  {
    return ecma_proxy_object_define_own_property (obj_p, property_name_p, property_desc_p);
  }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROXY) */

  switch (type)
  {
    case ECMA_OBJECT_TYPE_GENERAL:
    case ECMA_OBJECT_TYPE_CLASS:
    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    {
      return ecma_op_general_object_define_own_property (obj_p,
                                                         property_name_p,
                                                         property_desc_p);
    }

    case ECMA_OBJECT_TYPE_ARRAY:
    {
      return ecma_op_array_object_define_own_property (obj_p,
                                                       property_name_p,
                                                       property_desc_p);
    }

    default:
    {
      JERRY_ASSERT (type == ECMA_OBJECT_TYPE_PSEUDO_ARRAY);

      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

#if ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY)
      if (ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_ARGUMENTS)
      {
#else /* !ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */
        JERRY_ASSERT (ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_ARGUMENTS);
#endif /* ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */
        return ecma_op_arguments_object_define_own_property (obj_p,
                                                             property_name_p,
                                                             property_desc_p);
#if ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY)
      }
      /* ES2015 9.4.5.3 */
      if (ecma_object_is_typedarray (obj_p))
      {
#if ENABLED (JERRY_ES2015)
        if (ecma_prop_name_is_symbol (property_name_p))
        {
          return ecma_op_general_object_define_own_property (obj_p,
                                                             property_name_p,
                                                             property_desc_p);
        }
#endif /* ENABLED (JERRY_ES2015) */
        uint32_t array_index = ecma_string_get_array_index (property_name_p);

        if (array_index != ECMA_STRING_NOT_ARRAY_INDEX)
        {
          bool define_status = ecma_op_typedarray_define_index_prop (obj_p,
                                                                     array_index,
                                                                     property_desc_p);

          if (define_status)
          {
            return ECMA_VALUE_TRUE;
          }

          return ecma_reject (property_desc_p->flags & ECMA_PROP_IS_THROW);
        }

        ecma_number_t num = ecma_string_to_number (property_name_p);
        ecma_string_t *num_to_str = ecma_new_ecma_string_from_number (num);

        if (ecma_compare_ecma_strings (property_name_p, num_to_str))
        {
          ecma_deref_ecma_string (num_to_str);

          return ecma_reject (property_desc_p->flags & ECMA_PROP_IS_THROW);
        }

        ecma_deref_ecma_string (num_to_str);
      }

      return ecma_op_general_object_define_own_property (obj_p,
                                                         property_name_p,
                                                         property_desc_p);
#else /* !ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */
      break;
#endif /* ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */
    }
  }
} /* ecma_op_object_define_own_property */

/**
 * Get property descriptor from specified property
 *
 * depending on the property type the following fields are set:
 *   - for named data properties: { [Value], [Writable], [Enumerable], [Configurable] };
 *   - for named accessor properties: { [Get] - if defined,
 *                                      [Set] - if defined,
 *                                      [Enumerable], [Configurable]
 *                                    }.
 *
 * @return true - if property found
 *         false - otherwise
 */
ecma_value_t
ecma_op_object_get_own_property_descriptor (ecma_object_t *object_p, /**< the object */
                                            ecma_string_t *property_name_p, /**< property name */
                                            ecma_property_descriptor_t *prop_desc_p) /**< property descriptor */
{
#if ENABLED (JERRY_ES2015_BUILTIN_PROXY)
  if (ECMA_OBJECT_IS_PROXY (object_p))
  {
    return ecma_proxy_object_get_own_property_descriptor (object_p, property_name_p, prop_desc_p);
  }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROXY) */

  ecma_property_ref_t property_ref;

  ecma_property_t property = ecma_op_object_get_own_property (object_p,
                                                              property_name_p,
                                                              &property_ref,
                                                              ECMA_PROPERTY_GET_VALUE);

  if (property == ECMA_PROPERTY_TYPE_NOT_FOUND || property == ECMA_PROPERTY_TYPE_NOT_FOUND_AND_STOP)
  {
    return ECMA_VALUE_FALSE;
  }

  *prop_desc_p = ecma_make_empty_property_descriptor ();

  uint32_t flags = ecma_is_property_enumerable (property) ? ECMA_PROP_IS_ENUMERABLE : ECMA_PROP_NO_OPTS;
  flags |= ecma_is_property_configurable (property) ? ECMA_PROP_IS_CONFIGURABLE: ECMA_PROP_NO_OPTS;

  prop_desc_p->flags = (uint16_t) (ECMA_PROP_IS_ENUMERABLE_DEFINED | ECMA_PROP_IS_CONFIGURABLE_DEFINED | flags);

  ecma_property_types_t type = ECMA_PROPERTY_GET_TYPE (property);

  if (type != ECMA_PROPERTY_TYPE_NAMEDACCESSOR)
  {
    if (type == ECMA_PROPERTY_TYPE_NAMEDDATA)
    {
      prop_desc_p->value = ecma_copy_value (property_ref.value_p->value);
    }
    else
    {
      JERRY_ASSERT (type == ECMA_PROPERTY_TYPE_VIRTUAL);
      prop_desc_p->value = property_ref.virtual_value;
    }

    prop_desc_p->flags |= (ECMA_PROP_IS_VALUE_DEFINED | ECMA_PROP_IS_WRITABLE_DEFINED);
    prop_desc_p->flags = (uint16_t) (prop_desc_p->flags | (ecma_is_property_writable (property) ? ECMA_PROP_IS_WRITABLE
                                                                                                : ECMA_PROP_NO_OPTS));
  }
  else
  {

    ecma_getter_setter_pointers_t *get_set_pair_p = ecma_get_named_accessor_property (property_ref.value_p);
    prop_desc_p->flags |= (ECMA_PROP_IS_GET_DEFINED | ECMA_PROP_IS_SET_DEFINED);

    if (get_set_pair_p->getter_cp == JMEM_CP_NULL)
    {
      prop_desc_p->get_p = NULL;
    }
    else
    {
      prop_desc_p->get_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, get_set_pair_p->getter_cp);
      ecma_ref_object (prop_desc_p->get_p);
    }

    if (get_set_pair_p->setter_cp == JMEM_CP_NULL)
    {
      prop_desc_p->set_p = NULL;
    }
    else
    {
      prop_desc_p->set_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, get_set_pair_p->setter_cp);
      ecma_ref_object (prop_desc_p->set_p);
    }
  }

  return ECMA_VALUE_TRUE;
} /* ecma_op_object_get_own_property_descriptor */

/**
 * [[HasInstance]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 9
 *
 * @return ecma value containing a boolean value or an error
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_object_has_instance (ecma_object_t *obj_p, /**< the object */
                             ecma_value_t value) /**< argument 'V' */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));

  JERRY_ASSERT_OBJECT_TYPE_IS_VALID (ecma_get_object_type (obj_p));

  if (ecma_op_object_is_callable (obj_p))
  {
    return ecma_op_function_has_instance (obj_p, value);
  }

  return ecma_raise_type_error (ECMA_ERR_MSG ("Expected a function object."));
} /* ecma_op_object_has_instance */

/**
 * Object's isPrototypeOf operation
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.6; 3
 *
 * @return ECMA_VALUE_ERROR - if the operation fails
 *         ECMA_VALUE_TRUE - if the target object is prototype of the base object
 *         ECMA_VALUE_FALSE - if the target object is not prototype of the base object
 */
ecma_value_t
ecma_op_object_is_prototype_of (ecma_object_t *base_p, /**< base object */
                                ecma_object_t *target_p) /**< target object */
{
  do
  {
    jmem_cpointer_t target_cp;
#if ENABLED (JERRY_ES2015_BUILTIN_PROXY)
    if (ECMA_OBJECT_IS_PROXY (target_p))
    {
      ecma_value_t target_proto = ecma_proxy_object_get_prototype_of (target_p);

      if (ECMA_IS_VALUE_ERROR (target_proto))
      {
        return target_proto;
      }
      target_cp = ecma_proxy_object_prototype_to_cp (target_proto);
    }
    else
    {
#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROXY) */
      target_cp = ecma_op_ordinary_object_get_prototype_of (target_p);
#if ENABLED (JERRY_ES2015_BUILTIN_PROXY)
    }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROXY) */

    if (target_cp == JMEM_CP_NULL)
    {
      return ECMA_VALUE_FALSE;
    }

    target_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t, target_cp);

    if (target_p == base_p)
    {
      return ECMA_VALUE_TRUE;
    }
  } while (true);
} /* ecma_op_object_is_prototype_of */

/**
 * Get collection of property names
 *
 * Order of names in the collection:
 *  - integer indices in ascending order
 *  - other indices in creation order (for built-ins: the order of the properties are listed in specification).
 *
 * Note:
 *      Implementation of the routine assumes that new properties are appended to beginning of corresponding object's
 *      property list, and the list is not reordered (in other words, properties are stored in order that is reversed
 *      to the properties' addition order).
 *
 * @return NULL - if the Proxy.[[OwnPropertyKeys]] operation raises error
 *         collection of property names - otherwise
 */
ecma_collection_t *
ecma_op_object_get_property_names (ecma_object_t *obj_p, /**< object */
                                   uint32_t opts) /**< any combination of ecma_list_properties_options_t values  */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));

#if ENABLED (JERRY_ES2015_BUILTIN_PROXY)
  if (ECMA_OBJECT_IS_PROXY (obj_p))
  {
    return ecma_proxy_object_own_property_keys (obj_p);
  }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROXY) */

  if (ecma_op_object_is_fast_array (obj_p))
  {
    return ecma_fast_array_get_property_names (obj_p, opts);
  }

  ecma_collection_t *ret_p = ecma_new_collection ();
  ecma_collection_t *skipped_non_enumerable_p = ecma_new_collection ();

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  const bool obj_is_builtin = ecma_get_object_is_builtin (obj_p);
  const bool is_enumerable_only = (opts & ECMA_LIST_ENUMERABLE) != 0;
  const bool is_array_indices_only = (opts & ECMA_LIST_ARRAY_INDICES) != 0;
  const bool is_with_prototype_chain = (opts & ECMA_LIST_PROTOTYPE) != 0;
#if ENABLED (JERRY_ES2015)
  const bool is_symbols = (opts & ECMA_LIST_SYMBOLS) != 0;
  const bool is_symbols_only = (opts & ECMA_LIST_SYMBOLS_ONLY) != 0;
#endif /* ENABLED (JERRY_ES2015) */

  const size_t bitmap_row_size = sizeof (uint32_t) * JERRY_BITSINBYTE;
  const size_t names_hashes_bitmap_size = ECMA_OBJECT_HASH_BITMAP_SIZE / bitmap_row_size;
  JERRY_VLA (uint32_t, names_hashes_bitmap, names_hashes_bitmap_size);

  memset (names_hashes_bitmap, 0, names_hashes_bitmap_size * sizeof (names_hashes_bitmap[0]));

  ecma_object_t *prototype_chain_iter_p = obj_p;

  while (true)
  {
    ecma_length_t string_named_properties_count = 0;
    ecma_length_t array_index_named_properties_count = 0;
#if ENABLED (JERRY_ES2015)
    ecma_length_t symbol_named_properties_count = 0;
#endif /* ENABLED (JERRY_ES2015) */
    ecma_collection_t *prop_names_p = ecma_new_collection ();

#if ENABLED (JERRY_ES2015)
    if (JERRY_LIKELY (!is_symbols_only))
    {
#endif /* ENABLED (JERRY_ES2015) */

      if (obj_is_builtin)
      {
        ecma_builtin_list_lazy_property_names (obj_p,
                                               opts,
                                               prop_names_p,
                                               skipped_non_enumerable_p);
      }
      else
      {
        switch (type)
        {
          case ECMA_OBJECT_TYPE_PSEUDO_ARRAY:
          {
  #if ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY)
            if (ecma_object_is_typedarray (obj_p))
            {
              ecma_op_typedarray_list_lazy_property_names (obj_p, prop_names_p);
            }
  #endif /* ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */
            break;
          }
          case ECMA_OBJECT_TYPE_FUNCTION:
          {
            ecma_op_function_list_lazy_property_names (obj_p,
                                                       opts,
                                                       prop_names_p,
                                                       skipped_non_enumerable_p);
            break;
          }
          case ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION:
          {
            ecma_op_external_function_list_lazy_property_names (opts,
                                                                prop_names_p,
                                                                skipped_non_enumerable_p);
            break;
          }
          case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
          {
            ecma_op_bound_function_list_lazy_property_names (opts,
                                                             prop_names_p,
                                                             skipped_non_enumerable_p);
            break;
          }
          case ECMA_OBJECT_TYPE_CLASS:
          {
            ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

            if (ext_object_p->u.class_prop.class_id == LIT_MAGIC_STRING_STRING_UL)
            {
              ecma_op_string_list_lazy_property_names (obj_p,
                                                       opts,
                                                       prop_names_p,
                                                       skipped_non_enumerable_p);
            }

            break;
          }
          case ECMA_OBJECT_TYPE_ARRAY:
          {
            ecma_op_array_list_lazy_property_names (obj_p,
                                                    opts,
                                                    prop_names_p,
                                                    skipped_non_enumerable_p);
            break;
          }
          default:
          {
            JERRY_ASSERT (type == ECMA_OBJECT_TYPE_GENERAL);

            break;
          }
        }
      }
#if ENABLED (JERRY_ES2015)
    }
#endif /* ENABLED (JERRY_ES2015) */

    ecma_value_t *buffer_p = prop_names_p->buffer_p;

    const size_t own_names_hashes_bitmap_size = ECMA_OBJECT_HASH_BITMAP_SIZE / bitmap_row_size;
    JERRY_VLA (uint32_t, own_names_hashes_bitmap, own_names_hashes_bitmap_size);
    memset (own_names_hashes_bitmap, 0, own_names_hashes_bitmap_size * sizeof (own_names_hashes_bitmap[0]));

    for (uint32_t i = 0; i < prop_names_p->item_count; i++)
    {
      ecma_string_t *name_p = ecma_get_string_from_value (buffer_p[i]);

      if (ecma_string_get_array_index (name_p) != ECMA_STRING_NOT_ARRAY_INDEX)
      {
        array_index_named_properties_count++;
      }
#if ENABLED (JERRY_ES2015)
      else if (ecma_prop_name_is_symbol (name_p))
      {
        symbol_named_properties_count++;
      }
#endif /* ENABLED (JERRY_ES2015) */
      else
      {
        string_named_properties_count++;
      }

#if ENABLED (JERRY_ES2015)
      /* Symbols are never lazy listed */
      JERRY_ASSERT (!ecma_prop_name_is_symbol (name_p));
#endif /* ENABLED (JERRY_ES2015) */

      uint8_t hash = (uint8_t) ecma_string_hash (name_p);
      uint32_t bitmap_row = (uint32_t) (hash / bitmap_row_size);
      uint32_t bitmap_column = (uint32_t) (hash % bitmap_row_size);

      if ((own_names_hashes_bitmap[bitmap_row] & (1u << bitmap_column)) == 0)
      {
        own_names_hashes_bitmap[bitmap_row] |= (1u << bitmap_column);
      }
    }

    jmem_cpointer_t prop_iter_cp = prototype_chain_iter_p->u1.property_list_cp;

    if (ecma_op_object_is_fast_array (prototype_chain_iter_p) && prop_iter_cp != JMEM_CP_NULL)
    {
      ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) prototype_chain_iter_p;

      uint32_t length = ext_obj_p->u.array.length;
      array_index_named_properties_count = length - ecma_fast_array_get_hole_count (prototype_chain_iter_p);

      ecma_value_t *values_p = ECMA_GET_NON_NULL_POINTER (ecma_value_t, prop_iter_cp);

      for (uint32_t i = 0; i < length; i++)
      {
        if (ecma_is_value_array_hole (values_p[i]))
        {
          continue;
        }

        ecma_string_t *index_str_p = ecma_new_ecma_string_from_uint32 (i);

        uint8_t hash = (uint8_t) ecma_string_hash (index_str_p);
        uint32_t bitmap_row = (uint32_t) (hash / bitmap_row_size);
        uint32_t bitmap_column = (uint32_t) (hash % bitmap_row_size);

        bool is_add = true;

        if ((own_names_hashes_bitmap[bitmap_row] & (1u << bitmap_column)) != 0)
        {
          buffer_p = prop_names_p->buffer_p;

          for (uint32_t j = 0; j < prop_names_p->item_count; j++)
          {
            ecma_string_t *current_name_p = ecma_get_prop_name_from_value (buffer_p[j]);

            if (ecma_compare_ecma_strings (index_str_p, current_name_p))
            {
              is_add = false;
              break;
            }
          }
        }

        if (is_add)
        {
          own_names_hashes_bitmap[bitmap_row] |= (1u << bitmap_column);

          ecma_collection_push_back (prop_names_p, ecma_make_string_value (index_str_p));
        }
      }
    }
    else
    {
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

        for (int i = 0; i < ECMA_PROPERTY_PAIR_ITEM_COUNT; i++)
        {
          ecma_property_t *property_p = prop_iter_p->types + i;

          if (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA
              || ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR)
          {
            ecma_property_pair_t *prop_pair_p = (ecma_property_pair_t *) prop_iter_p;

            if (ECMA_PROPERTY_GET_NAME_TYPE (*property_p) == ECMA_DIRECT_STRING_MAGIC
                && prop_pair_p->names_cp[i] >= LIT_NON_INTERNAL_MAGIC_STRING__COUNT
                && prop_pair_p->names_cp[i] < LIT_MAGIC_STRING__COUNT)
            {
              /* Internal properties are never enumerated. */
              continue;
            }

            ecma_string_t *name_p = ecma_string_from_property_name (*property_p,
                                                                    prop_pair_p->names_cp[i]);

            if (!(is_enumerable_only && !ecma_is_property_enumerable (*property_p)))
            {
  #if ENABLED (JERRY_ES2015)
              /* We skip the current property in the following cases:
                 1. We don't want to list symbols (is_symbols and is_symbols_only are false)
                    and the current property is a symbol.
                 2. We only want to list symbols (is_symbols_only is true) and the current
                    property is NOT a symbol. */
              bool is_symbol = ecma_prop_name_is_symbol (name_p);
              if ((!(is_symbols || is_symbols_only) && is_symbol) || (is_symbols_only && !is_symbol))
              {
                ecma_deref_ecma_string (name_p);
                continue;
              }
  #endif /* ENABLED (JERRY_ES2015) */

              uint8_t hash = (uint8_t) ecma_string_hash (name_p);
              uint32_t bitmap_row = (uint32_t) (hash / bitmap_row_size);
              uint32_t bitmap_column = (uint32_t) (hash % bitmap_row_size);

              bool is_add = true;

              if ((own_names_hashes_bitmap[bitmap_row] & (1u << bitmap_column)) != 0)
              {
                buffer_p = prop_names_p->buffer_p;

                for (uint32_t j = 0; j < prop_names_p->item_count; j++)
                {
                  ecma_string_t *current_name_p = ecma_get_prop_name_from_value (buffer_p[j]);

                  if (ecma_compare_ecma_strings (name_p, current_name_p))
                  {
                    is_add = false;
                    break;
                  }
                }
              }

              if (is_add)
              {
                if (ecma_string_get_array_index (name_p) != ECMA_STRING_NOT_ARRAY_INDEX)
                {
                  /* The name is a valid array index. */
                  array_index_named_properties_count++;
                }
                else if (!is_array_indices_only)
                {
  #if ENABLED (JERRY_ES2015)
                  if (ecma_prop_name_is_symbol (name_p))
                  {
                    symbol_named_properties_count++;
                  }
                  else
                  {
  #endif /* ENABLED (JERRY_ES2015) */
                    string_named_properties_count++;
  #if ENABLED (JERRY_ES2015)
                  }
  #endif /* ENABLED (JERRY_ES2015) */
                }
                else
                {
                  ecma_deref_ecma_string (name_p);
                  continue;
                }

                own_names_hashes_bitmap[bitmap_row] |= (1u << bitmap_column);

                ecma_collection_push_back (prop_names_p, ecma_make_prop_name_value (name_p));
              }
              else
              {
                ecma_deref_ecma_string (name_p);
              }
            }
            else
            {
              JERRY_ASSERT (is_enumerable_only && !ecma_is_property_enumerable (*property_p));

              ecma_collection_push_back (skipped_non_enumerable_p, ecma_make_prop_name_value (name_p));
            }
          }
        }

        prop_iter_cp = prop_iter_p->next_property_cp;
      }
    }

    ecma_length_t all_properties_count = array_index_named_properties_count + string_named_properties_count;

#if ENABLED (JERRY_ES2015)
    all_properties_count += symbol_named_properties_count;
#endif /* ENABLED (JERRY_ES2015) */

    /* Second pass: collecting property names into an array. */
    JMEM_DEFINE_LOCAL_ARRAY (names_p, all_properties_count, ecma_string_t *);

    ecma_string_t **string_names_p = names_p + array_index_named_properties_count;
#if ENABLED (JERRY_ES2015)
    ecma_string_t **symbol_names_p = string_names_p + string_named_properties_count;
#endif /* ENABLED (JERRY_ES2015) */

    uint32_t array_index_name_pos = 0;
    uint32_t string_name_pos = string_named_properties_count;
#if ENABLED (JERRY_ES2015)
    uint32_t symbol_name_pos = symbol_named_properties_count;
#endif /* ENABLED (JERRY_ES2015) */

    buffer_p = prop_names_p->buffer_p;

    for (uint32_t i = 0; i < prop_names_p->item_count; i++)
    {
      ecma_string_t *name_p = ecma_get_prop_name_from_value (buffer_p[i]);
      ecma_ref_ecma_string (name_p);

      uint32_t index = ecma_string_get_array_index (name_p);

      if (index != ECMA_STRING_NOT_ARRAY_INDEX)
      {
        JERRY_ASSERT (array_index_name_pos < array_index_named_properties_count);

        uint32_t insertion_pos = 0;
        while (insertion_pos < array_index_name_pos
               && index > ecma_string_get_array_index (names_p[insertion_pos]))
        {
          insertion_pos++;
        }

        if (insertion_pos == array_index_name_pos)
        {
          names_p[array_index_name_pos++] = name_p;
        }
        else
        {
          JERRY_ASSERT (insertion_pos < array_index_name_pos);
          JERRY_ASSERT (index <= ecma_string_get_array_index (names_p[insertion_pos]));

          uint32_t move_pos = array_index_name_pos++;

          while (move_pos > insertion_pos)
          {
            names_p[move_pos] = names_p[move_pos - 1u];

            move_pos--;
          }

          names_p[insertion_pos] = name_p;
        }
      }
#if ENABLED (JERRY_ES2015)
      else if (ecma_prop_name_is_symbol (name_p))
      {
        // Put in the symbols in reverse order.
        JERRY_ASSERT (symbol_name_pos > 0);
        JERRY_ASSERT (symbol_name_pos <= symbol_named_properties_count);

        symbol_names_p[--symbol_name_pos] = name_p;
      }
#endif /* ENABLED (JERRY_ES2015) */
      else
      {
        // Put in the strings in reverse order.
        JERRY_ASSERT (string_name_pos > 0);
        JERRY_ASSERT (string_name_pos <= string_named_properties_count);

        string_names_p[--string_name_pos] = name_p;
      }
    }

    JERRY_ASSERT (array_index_name_pos == array_index_named_properties_count);
    JERRY_ASSERT (string_name_pos == 0);
#if ENABLED (JERRY_ES2015)
    JERRY_ASSERT (symbol_name_pos == 0);
#endif /* ENABLED (JERRY_ES2015) */

    ecma_collection_free (prop_names_p);

    /* Third pass:
     *   embedding own property names of current object of prototype chain to aggregate property names collection */
    for (uint32_t i = 0; i < all_properties_count; i++)
    {
      bool is_append = true;

      ecma_string_t *name_p = names_p[i];

      uint8_t hash = (uint8_t) ecma_string_hash (name_p);
      uint32_t bitmap_row = (uint32_t) (hash / bitmap_row_size);
      uint32_t bitmap_column = (uint32_t) (hash % bitmap_row_size);

      if ((names_hashes_bitmap[bitmap_row] & (1u << bitmap_column)) == 0)
      {
        /* This hash has not been used before (for non-skipped). */
        names_hashes_bitmap[bitmap_row] |= (1u << bitmap_column);
      }
      else
      {
        /* Name with same hash has already occured. */
        buffer_p = ret_p->buffer_p;

        for (uint32_t j = 0; j < ret_p->item_count; j++)
        {
          ecma_string_t *current_name_p = ecma_get_prop_name_from_value (buffer_p[j]);

          if (ecma_compare_ecma_strings (name_p, current_name_p))
          {
            is_append = false;
            break;
          }
        }
      }

      if (is_append)
      {
        buffer_p = skipped_non_enumerable_p->buffer_p;

        for (uint32_t j = 0; j < skipped_non_enumerable_p->item_count; j++)
        {
          ecma_string_t *current_name_p = ecma_get_prop_name_from_value (buffer_p[j]);

          if (ecma_compare_ecma_strings (name_p, current_name_p))
          {
            is_append = false;
            break;
          }
        }
      }

      if (is_append)
      {
        JERRY_ASSERT ((names_hashes_bitmap[bitmap_row] & (1u << bitmap_column)) != 0);

        ecma_collection_push_back (ret_p, ecma_make_prop_name_value (name_p));
      }
      else
      {
        ecma_deref_ecma_string (name_p);
      }

    }

    JMEM_FINALIZE_LOCAL_ARRAY (names_p);

    if (!is_with_prototype_chain || prototype_chain_iter_p->u2.prototype_cp == JMEM_CP_NULL)
    {
      break;
    }

    prototype_chain_iter_p = ECMA_GET_NON_NULL_POINTER (ecma_object_t,
                                                        prototype_chain_iter_p->u2.prototype_cp);
  }

  ecma_collection_free (skipped_non_enumerable_p);

  return ret_p;
} /* ecma_op_object_get_property_names */

/**
 * The function is used in the assert of ecma_object_get_class_name
 *
 * @return true  - if class name is an object
 *         false - otherwise
 */
inline static bool
ecma_object_check_class_name_is_object (ecma_object_t *obj_p) /**< object */
{
#ifndef JERRY_NDEBUG
  return (ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_GLOBAL)
#if ENABLED (JERRY_ES2015_BUILTIN_PROMISE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_PROMISE_PROTOTYPE)
#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROMISE) */
#if ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_ARRAYBUFFER_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_TYPEDARRAY_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_INT8ARRAY_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_UINT8ARRAY_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_INT16ARRAY_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_UINT16ARRAY_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_INT32ARRAY_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_UINT32ARRAY_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_FLOAT32ARRAY_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_UINT8CLAMPEDARRAY_PROTOTYPE)
#if ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_FLOAT64ARRAY_PROTOTYPE)
#endif /* ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */
#endif /* ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */
#if ENABLED (JERRY_ES2015)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_ARRAY_ITERATOR_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_ITERATOR_PROTOTYPE)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_STRING_ITERATOR_PROTOTYPE)
#endif /* ENABLED (JERRY_ES2015) */
#if ENABLED (JERRY_ES2015_BUILTIN_MAP)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_MAP_PROTOTYPE)
#if ENABLED (JERRY_ES2015)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_MAP_ITERATOR_PROTOTYPE)
#endif /* ENABLED (JERRY_ES2015) */
#endif /* ENABLED (JERRY_ES2015_BUILTIN_MAP) */
#if ENABLED (JERRY_ES2015_BUILTIN_SET)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_SET_PROTOTYPE)
#if ENABLED (JERRY_ES2015)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_SET_ITERATOR_PROTOTYPE)
#endif /* ENABLED (JERRY_ES2015) */
#endif /* ENABLED (JERRY_ES2015_BUILTIN_SET) */
#if ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_WEAKMAP_PROTOTYPE)
#endif /* ENABLED (JERRY_ES2015_BUILTIN_WEAKMAP) */
#if ENABLED (JERRY_ES2015_BUILTIN_WEAKSET)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_WEAKSET_PROTOTYPE)
#endif /* ENABLED (JERRY_ES2015_BUILTIN_WEAKSET) */
#if ENABLED (JERRY_ES2015)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_SYMBOL_PROTOTYPE)
#endif /* ENABLED (JERRY_ES2015) */
#if ENABLED (JERRY_ES2015_BUILTIN_DATAVIEW)
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_DATAVIEW_PROTOTYPE)
#endif /* ENABLED (JERRY_ES2015_BUILTIN_DATAVIEW) */
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_OBJECT_PROTOTYPE));
#else /* JERRY_NDEBUG */
  JERRY_UNUSED (obj_p);
  return true;
#endif /* !JERRY_NDEBUG */
} /* ecma_object_check_class_name_is_object */

/**
 * Get [[Class]] string of specified object
 *
 * @return class name magic string
 */
lit_magic_string_id_t
ecma_object_get_class_name (ecma_object_t *obj_p) /**< object */
{
  ecma_object_type_t type = ecma_get_object_type (obj_p);

  switch (type)
  {
    case ECMA_OBJECT_TYPE_ARRAY:
    {
      return LIT_MAGIC_STRING_ARRAY_UL;
    }
    case ECMA_OBJECT_TYPE_CLASS:
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;
      return (lit_magic_string_id_t) ext_object_p->u.class_prop.class_id;
    }
    case ECMA_OBJECT_TYPE_PSEUDO_ARRAY:
    {
      ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

      switch (ext_obj_p->u.pseudo_array.type)
      {
#if ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY)
        case ECMA_PSEUDO_ARRAY_TYPEDARRAY:
        case ECMA_PSEUDO_ARRAY_TYPEDARRAY_WITH_INFO:
        {
          return (lit_magic_string_id_t) ext_obj_p->u.pseudo_array.u1.class_id;
        }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */
#if ENABLED (JERRY_ES2015)
        case ECMA_PSEUDO_ARRAY_ITERATOR:
        {
          return LIT_MAGIC_STRING_ARRAY_ITERATOR_UL;
        }
        case ECMA_PSEUDO_SET_ITERATOR:
        {
          return LIT_MAGIC_STRING_SET_ITERATOR_UL;
        }
        case ECMA_PSEUDO_MAP_ITERATOR:
        {
          return LIT_MAGIC_STRING_MAP_ITERATOR_UL;
        }
#endif /* ENABLED (JERRY_ES2015) */
#if ENABLED (JERRY_ES2015)
        case ECMA_PSEUDO_STRING_ITERATOR:
        {
          return LIT_MAGIC_STRING_STRING_ITERATOR_UL;
        }
#endif /* ENABLED (JERRY_ES2015) */
        default:
        {
          JERRY_ASSERT (ext_obj_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_ARGUMENTS);

          return LIT_MAGIC_STRING_ARGUMENTS_UL;
        }
      }

      break;
    }
    case ECMA_OBJECT_TYPE_FUNCTION:
    case ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    {
      return LIT_MAGIC_STRING_FUNCTION_UL;
    }
    default:
    {
      JERRY_ASSERT (type == ECMA_OBJECT_TYPE_GENERAL || type == ECMA_OBJECT_TYPE_PROXY);

      if (ecma_get_object_is_builtin (obj_p))
      {
        ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

        switch (ext_obj_p->u.built_in.id)
        {
#if ENABLED (JERRY_BUILTIN_MATH)
          case ECMA_BUILTIN_ID_MATH:
          {
            return LIT_MAGIC_STRING_MATH_UL;
          }
#endif /* ENABLED (JERRY_BUILTIN_MATH) */
#if ENABLED (JERRY_ES2015_BUILTIN_REFLECT)
          case ECMA_BUILTIN_ID_REFLECT:
          {
            return LIT_MAGIC_STRING_REFLECT_UL;
          }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_REFLECT) */
#if ENABLED (JERRY_ES2015)
          case ECMA_BUILTIN_ID_GENERATOR:
          {
            return LIT_MAGIC_STRING_GENERATOR_UL;
          }
#endif /* ENABLED (JERRY_ES2015) */
#if ENABLED (JERRY_BUILTIN_JSON)
          case ECMA_BUILTIN_ID_JSON:
          {
            return LIT_MAGIC_STRING_JSON_U;
          }
#endif /* ENABLED (JERRY_BUILTIN_JSON) */
#if ENABLED (JERRY_BUILTIN_ERRORS)
          case ECMA_BUILTIN_ID_EVAL_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_RANGE_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_REFERENCE_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_SYNTAX_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_TYPE_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_URI_ERROR_PROTOTYPE:
#endif /* ENABLED (JERRY_BUILTIN_ERRORS) */
          case ECMA_BUILTIN_ID_ERROR_PROTOTYPE:
          {
            return LIT_MAGIC_STRING_ERROR_UL;
          }
#if ENABLED (JERRY_ES2015_BUILTIN_PROXY)
          case ECMA_BUILTIN_ID_PROXY:
          {
            return LIT_MAGIC_STRING_FUNCTION_UL;
          }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROXY) */
          default:
          {
            JERRY_ASSERT (ecma_object_check_class_name_is_object (obj_p));

            return LIT_MAGIC_STRING_OBJECT_UL;
          }
        }
      }
      else
      {
        return LIT_MAGIC_STRING_OBJECT_UL;
      }
    }
  }
} /* ecma_object_get_class_name */

/**
 * Get value of an object if the class matches
 *
 * @return value of the object if the class matches
 *         ECMA_VALUE_NOT_FOUND otherwise
 */
inline bool JERRY_ATTR_ALWAYS_INLINE
ecma_object_class_is (ecma_object_t *object_p, /**< object */
                      uint32_t class_id) /**< class id */
{
  if (ecma_get_object_type (object_p) == ECMA_OBJECT_TYPE_CLASS)
  {
    ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

    if (ext_object_p->u.class_prop.class_id == class_id)
    {
      return true;
    }
  }

  return false;
} /* ecma_object_class_is */

/**
 * Checks if the given argument has [[RegExpMatcher]] internal slot
 *
 * @return true - if the given argument is a regexp
 *         false - otherwise
 */
inline bool JERRY_ATTR_ALWAYS_INLINE
ecma_object_is_regexp_object (ecma_value_t arg) /**< argument */
{
  return (ecma_is_value_object (arg)
          && ecma_object_class_is (ecma_get_object_from_value (arg), LIT_MAGIC_STRING_REGEXP_UL));
} /* ecma_object_is_regexp_object */

#if ENABLED (JERRY_ES2015)
/**
 * Object's IsConcatSpreadable operation, used for Array.prototype.concat
 * It checks the argument's [Symbol.isConcatSpreadable] property value
 *
 * See also:
 *          ECMA-262 v6, 22.1.3.1.1;
 *
 * @return ECMA_VALUE_ERROR - if the operation fails
 *         ECMA_VALUE_TRUE - if the argument is concatSpreadable
 *         ECMA_VALUE_FALSE - otherwise
 */
ecma_value_t
ecma_op_is_concat_spreadable (ecma_value_t arg) /**< argument */
{
  if (!ecma_is_value_object (arg))
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_value_t spreadable = ecma_op_object_get_by_symbol_id (ecma_get_object_from_value (arg),
                                                             LIT_GLOBAL_SYMBOL_IS_CONCAT_SPREADABLE);

  if (ECMA_IS_VALUE_ERROR (spreadable))
  {
    return spreadable;
  }

  if (!ecma_is_value_undefined (spreadable))
  {
    const bool to_bool = ecma_op_to_boolean (spreadable);
    ecma_free_value (spreadable);
    return ecma_make_boolean_value (to_bool);
  }

  return (ecma_make_boolean_value (ecma_is_value_array (arg)));
} /* ecma_op_is_concat_spreadable */

/**
 * IsRegExp operation
 *
 * See also:
 *          ECMA-262 v6, 22.1.3.1.1;
 *
 * @return ECMA_VALUE_ERROR - if the operation fails
 *         ECMA_VALUE_TRUE - if the argument is regexp
 *         ECMA_VALUE_FALSE - otherwise
 */
ecma_value_t
ecma_op_is_regexp (ecma_value_t arg) /**< argument */
{
  if (!ecma_is_value_object (arg))
  {
    return ECMA_VALUE_FALSE;
  }

  ecma_value_t is_regexp = ecma_op_object_get_by_symbol_id (ecma_get_object_from_value (arg),
                                                            LIT_GLOBAL_SYMBOL_MATCH);

  if (ECMA_IS_VALUE_ERROR (is_regexp))
  {
    return is_regexp;
  }

  if (!ecma_is_value_undefined (is_regexp))
  {
    const bool to_bool = ecma_op_to_boolean (is_regexp);
    ecma_free_value (is_regexp);
    return ecma_make_boolean_value (to_bool);
  }

  return ecma_make_boolean_value (ecma_object_is_regexp_object (arg));
} /* ecma_op_is_regexp */

/**
 * SpeciesConstructor operation
 * See also:
 *          ECMA-262 v6, 7.3.20;
 *
 * @return ecma_value
 *         returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_species_constructor (ecma_object_t *this_value, /**< This Value */
                             ecma_builtin_id_t default_constructor_id) /**< Builtin ID of default constructor */
{
  ecma_object_t *default_constructor_p = ecma_builtin_get (default_constructor_id);
  ecma_value_t constructor = ecma_op_object_get_by_magic_id (this_value, LIT_MAGIC_STRING_CONSTRUCTOR);
  if (ECMA_IS_VALUE_ERROR (constructor))
  {
    return constructor;
  }

  if (ecma_is_value_undefined (constructor))
  {
    ecma_ref_object (default_constructor_p);
    return ecma_make_object_value (default_constructor_p);
  }

  if (!ecma_is_value_object (constructor))
  {
    ecma_free_value (constructor);
    return ecma_raise_type_error (ECMA_ERR_MSG ("Constructor must be an Object"));
  }

  ecma_object_t *ctor_object_p = ecma_get_object_from_value (constructor);
  ecma_value_t species = ecma_op_object_get_by_symbol_id (ctor_object_p, LIT_GLOBAL_SYMBOL_SPECIES);
  ecma_deref_object (ctor_object_p);

  if (ECMA_IS_VALUE_ERROR (species))
  {
    return species;
  }

  if (ecma_is_value_undefined (species) || ecma_is_value_null (species))
  {
    ecma_ref_object (default_constructor_p);
    return ecma_make_object_value (default_constructor_p);
  }

  if (!ecma_is_constructor (species))
  {
    ecma_free_value (species);
    return ecma_raise_type_error (ECMA_ERR_MSG ("Species must be a Constructor"));
  }

  return species;
} /* ecma_op_species_constructor */

#endif /* ENABLED (JERRY_ES2015) */

/**
 * 7.3.18 Abstract operation Invoke when property name is a magic string
 *
 * @return ecma_value result of the invoked function or raised error
 *         note: returned value must be freed with ecma_free_value
 */
inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_op_invoke_by_magic_id (ecma_value_t object, /**< Object value */
                            lit_magic_string_id_t magic_string_id, /**< Magic string ID */
                            ecma_value_t *args_p, /**< Argument list */
                            ecma_length_t args_len) /**< Argument list length */
{
  return ecma_op_invoke (object, ecma_get_magic_string (magic_string_id), args_p, args_len);
} /* ecma_op_invoke_by_magic_id */

/**
 * 7.3.18 Abstract operation Invoke
 *
 * @return ecma_value result of the invoked function or raised error
 *         note: returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_invoke (ecma_value_t object, /**< Object value */
                ecma_string_t *property_name_p, /**< Property name */
                ecma_value_t *args_p, /**< Argument list */
                ecma_length_t args_len) /**< Argument list length */
{
  /* 3. */
  ecma_value_t object_value = ecma_op_to_object (object);
  if (ECMA_IS_VALUE_ERROR (object_value))
  {
    return object_value;
  }

  ecma_object_t *object_p = ecma_get_object_from_value (object_value);
  ecma_value_t func = ecma_op_object_get (object_p, property_name_p);

  if (ECMA_IS_VALUE_ERROR (func))
  {
    ecma_deref_object (object_p);
    return func;
  }

  /* 4. */
  if (!ecma_op_is_callable (func))
  {
    ecma_free_value (func);
    ecma_deref_object (object_p);
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument is not callable"));
  }

  ecma_object_t *func_obj_p = ecma_get_object_from_value (func);
  ecma_value_t call_result = ecma_op_function_call (func_obj_p, object, args_p, args_len);
  ecma_deref_object (object_p);
  ecma_deref_object (func_obj_p);

  return call_result;
} /* ecma_op_invoke */

/**
 * Ordinary object [[GetPrototypeOf]] operation
 *
 * See also:
 *          ECMAScript v6, 9.1.1
 *
 * @return the value of the [[Prototype]] internal slot of the given object.
 */
inline jmem_cpointer_t JERRY_ATTR_ALWAYS_INLINE
ecma_op_ordinary_object_get_prototype_of (ecma_object_t *obj_p) /**< object */
{
  JERRY_ASSERT (!ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (!ECMA_OBJECT_IS_PROXY (obj_p));

  return obj_p->u2.prototype_cp;
} /* ecma_op_ordinary_object_get_prototype_of */

/**
 * Ordinary object [[SetPrototypeOf]] operation
 *
 * See also:
 *          ECMAScript v6, 9.1.2
 *
 * @return ECMA_VALUE_FALSE - if the operation fails
 *         ECMA_VALUE_TRUE - otherwise
 */
inline ecma_value_t JERRY_ATTR_ALWAYS_INLINE
ecma_op_ordinary_object_set_prototype_of (ecma_object_t *obj_p, /**< base object */
                                          ecma_value_t proto) /**< prototype object */
{
  JERRY_ASSERT (!ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (!ECMA_OBJECT_IS_PROXY (obj_p));

  /* 1. */
  JERRY_ASSERT (ecma_is_value_object (proto) || ecma_is_value_null (proto));

  /* 3. */
  ecma_object_t *current_proto_p = ECMA_GET_POINTER (ecma_object_t, ecma_op_ordinary_object_get_prototype_of (obj_p));
  ecma_object_t *new_proto_p = ecma_is_value_null (proto) ? NULL : ecma_get_object_from_value (proto);

  /* 4. */
  if (new_proto_p == current_proto_p)
  {
    return ECMA_VALUE_TRUE;
  }

  /* 2 - 5. */
  if (!ecma_op_ordinary_object_is_extensible (obj_p))
  {
    return ECMA_VALUE_FALSE;
  }

  /* 6. */
  ecma_object_t *iter_p = new_proto_p;

  /* 7 - 8. */
  while (true)
  {
    /* 8.a */
    if (iter_p == NULL)
    {
      break;
    }

    /* 8.b */
    if (obj_p == iter_p)
    {
      return ECMA_VALUE_FALSE;
    }

    /* 8.c.i */
#if ENABLED (JERRY_ES2015_BUILTIN_PROXY)
    if (ECMA_OBJECT_IS_PROXY (iter_p))
    {
      break;
    }
#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROXY) */

    /* 8.c.ii */
    iter_p = ECMA_GET_POINTER (ecma_object_t, ecma_op_ordinary_object_get_prototype_of (iter_p));
  }

  /* 9. */
  ECMA_SET_POINTER (obj_p->u2.prototype_cp, new_proto_p);

  /* 10. */
  return ECMA_VALUE_TRUE;
} /* ecma_op_ordinary_object_set_prototype_of */

/**
 * [[IsExtensible]] operation for Ordinary object.
 *
 * See also:
 *          ECMAScript v6, 9.1.2
 *
 * @return true  - if object is extensible
 *         false - otherwise
 */
inline bool JERRY_ATTR_PURE
ecma_op_ordinary_object_is_extensible (ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (!ECMA_OBJECT_IS_PROXY (object_p));

  return (object_p->type_flags_refs & ECMA_OBJECT_FLAG_EXTENSIBLE) != 0;
} /* ecma_op_ordinary_object_is_extensible */

/**
 * Set value of [[Extensible]] object's internal property.
 */
void JERRY_ATTR_NOINLINE
ecma_op_ordinary_object_prevent_extensions (ecma_object_t *object_p) /**< object */
{
  JERRY_ASSERT (!ECMA_OBJECT_IS_PROXY (object_p));
  object_p->type_flags_refs = (uint16_t) (object_p->type_flags_refs & ~ECMA_OBJECT_FLAG_EXTENSIBLE);
} /* ecma_op_ordinary_object_prevent_extensions */

/**
 * Checks whether an object (excluding prototypes) has a named property
 *
 * @return true - if property is found
 *         false - otherwise
 */
inline bool JERRY_ATTR_ALWAYS_INLINE
ecma_op_ordinary_object_has_own_property (ecma_object_t *object_p, /**< the object */
                                          ecma_string_t *property_name_p) /**< property name */
{
  JERRY_ASSERT (!ECMA_OBJECT_IS_PROXY (object_p));

  ecma_property_t property = ecma_op_object_get_own_property (object_p,
                                                              property_name_p,
                                                              NULL,
                                                              ECMA_PROPERTY_GET_HAS_OWN_PROP);

  return property != ECMA_PROPERTY_TYPE_NOT_FOUND && property != ECMA_PROPERTY_TYPE_NOT_FOUND_AND_STOP;
} /* ecma_op_ordinary_object_has_own_property */

/**
 * @}
 * @}
 */
