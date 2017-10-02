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
#include "ecma-function-object.h"
#include "ecma-lcache.h"
#include "ecma-lex-env.h"
#include "ecma-string-object.h"
#include "ecma-objects-arguments.h"
#include "ecma-objects-general.h"
#include "ecma-objects.h"

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
#include "ecma-typedarray-object.h"
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */

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
  JERRY_ASSERT (property_name_p != NULL);
  JERRY_ASSERT (options == ECMA_PROPERTY_GET_NO_OPTIONS || property_ref_p != NULL);

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
      if (ecma_string_is_length (property_name_p))
      {
        ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

        if (options & ECMA_PROPERTY_GET_VALUE)
        {
          property_ref_p->virtual_value = ecma_make_uint32_value (ext_object_p->u.array.length);
        }

        return ext_object_p->u.array.length_prop;
      }
      break;
    }
#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
    case ECMA_OBJECT_TYPE_PSEUDO_ARRAY:
    {
      /* ES2015 9.4.5.1 */
      if (ecma_is_typedarray (ecma_make_object_value (object_p)))
      {
        if (ECMA_STRING_GET_CONTAINER (property_name_p) == ECMA_STRING_CONTAINER_UINT32_IN_DESC)
        {
          ecma_value_t value = ecma_op_typedarray_get_index_prop (object_p, property_name_p->u.uint32_number);

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
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
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
    else if (ecma_is_normal_or_arrow_function (type))
    {
      if (ecma_string_is_length (property_name_p))
      {
        if (options & ECMA_PROPERTY_GET_VALUE)
        {
          /* Get length virtual property. */
          const ecma_compiled_code_t *bytecode_data_p;

#ifndef CONFIG_DISABLE_ES2015_ARROW_FUNCTION
          if (type != ECMA_OBJECT_TYPE_ARROW_FUNCTION)
          {
            ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) object_p;
            bytecode_data_p = ECMA_GET_INTERNAL_VALUE_POINTER (const ecma_compiled_code_t,
                                                               ext_func_p->u.function.bytecode_cp);
          }
          else
          {
            ecma_arrow_function_t *arrow_func_p = (ecma_arrow_function_t *) object_p;
            bytecode_data_p = ECMA_GET_NON_NULL_POINTER (const ecma_compiled_code_t,
                                                         arrow_func_p->bytecode_cp);
          }
#else /* CONFIG_DISABLE_ES2015_ARROW_FUNCTION */
          ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) object_p;
          bytecode_data_p = ECMA_GET_INTERNAL_VALUE_POINTER (const ecma_compiled_code_t,
                                                             ext_func_p->u.function.bytecode_cp);
#endif /* !CONFIG_DISABLE_ES2015_ARROW_FUNCTION */

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
           && property_ref_p != NULL)
  {
    ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

    if (ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_ARGUMENTS)
    {
      uint32_t index = ecma_string_get_array_index (property_name_p);

      if (index != ECMA_STRING_NOT_ARRAY_INDEX)
      {
        if (index < ext_object_p->u.pseudo_array.u1.length)
        {
          jmem_cpointer_t *arg_Literal_p = (jmem_cpointer_t *) (ext_object_p + 1);

          if (arg_Literal_p[index] != JMEM_CP_NULL)
          {
            ecma_string_t *arg_name_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_string_t,
                                                                      arg_Literal_p[index]);

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
ecma_property_t
ecma_op_object_get_property (ecma_object_t *object_p, /**< the object */
                             ecma_string_t *property_name_p, /**< property name */
                             ecma_property_ref_t *property_ref_p, /**< property reference */
                             uint32_t options) /**< option bits */
{
  /* Circular reference is possible in JavaScript and testing it is complicated. */
  int max_depth = ECMA_PROPERTY_SEARCH_DEPTH_LIMIT;

  do
  {
    ecma_property_t property = ecma_op_object_get_own_property (object_p,
                                                                property_name_p,
                                                                property_ref_p,
                                                                options);

    if (property != ECMA_PROPERTY_TYPE_NOT_FOUND && property != ECMA_PROPERTY_TYPE_NOT_FOUND_AND_STOP)
    {
      return property;
    }

    if (--max_depth == 0 || property == ECMA_PROPERTY_TYPE_NOT_FOUND_AND_STOP)
    {
      break;
    }

    object_p = ecma_get_object_prototype (object_p);
  }
  while (object_p != NULL);

  return ECMA_PROPERTY_TYPE_NOT_FOUND;
} /* ecma_op_object_get_property */

/**
 * Checks whether an object (excluding prototypes) has a named property
 *
 * @return true - if property is found
 *         false - otherwise
 */
inline bool __attr_always_inline___
ecma_op_object_has_own_property (ecma_object_t *object_p, /**< the object */
                                 ecma_string_t *property_name_p) /**< property name */
{
  ecma_property_t property = ecma_op_object_get_own_property (object_p,
                                                              property_name_p,
                                                              NULL,
                                                              ECMA_PROPERTY_GET_NO_OPTIONS);

  return property != ECMA_PROPERTY_TYPE_NOT_FOUND && property != ECMA_PROPERTY_TYPE_NOT_FOUND_AND_STOP;
} /* ecma_op_object_has_own_property */

/**
 * Checks whether an object (including prototypes) has a named property
 *
 * @return true - if property is found
 *         false - otherwise
 */
inline bool __attr_always_inline___
ecma_op_object_has_property (ecma_object_t *object_p, /**< the object */
                             ecma_string_t *property_name_p) /**< property name */
{
  ecma_property_t property = ecma_op_object_get_property (object_p,
                                                          property_name_p,
                                                          NULL,
                                                          ECMA_PROPERTY_GET_NO_OPTIONS);
  return property != ECMA_PROPERTY_TYPE_NOT_FOUND;
} /* ecma_op_object_has_property */

/**
 * Search the value corresponding to a property name
 *
 * Note: search includes prototypes
 *
 * @return ecma value if property is found
 *         ECMA_SIMPLE_VALUE_NOT_FOUND if property is not found
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
      if (ecma_string_is_length (property_name_p))
      {
        ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

        return ecma_make_uint32_value (ext_object_p->u.array.length);
      }
      break;
    }
    case ECMA_OBJECT_TYPE_PSEUDO_ARRAY:
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

      if (ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_ARGUMENTS)
      {
        uint32_t index = ecma_string_get_array_index (property_name_p);

        if (index != ECMA_STRING_NOT_ARRAY_INDEX)
        {
          if (index < ext_object_p->u.pseudo_array.u1.length)
          {
            jmem_cpointer_t *arg_Literal_p = (jmem_cpointer_t *) (ext_object_p + 1);

            if (arg_Literal_p[index] != JMEM_CP_NULL)
            {
              ecma_string_t *arg_name_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_string_t,
                                                                        arg_Literal_p[index]);

              ecma_object_t *lex_env_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                                          ext_object_p->u.pseudo_array.u2.lex_env_cp);

              JERRY_ASSERT (lex_env_p != NULL
                            && ecma_is_lexical_environment (lex_env_p));

              return ecma_op_get_binding_value (lex_env_p, arg_name_p, true);
            }
          }
        }
      }
#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
      /* ES2015 9.4.5.4 */
      if (ecma_is_typedarray (ecma_make_object_value (object_p)))
      {
        if (ECMA_STRING_GET_CONTAINER (property_name_p) == ECMA_STRING_CONTAINER_UINT32_IN_DESC)
        {
          return ecma_op_typedarray_get_index_prop (object_p, property_name_p->u.uint32_number);
        }

        ecma_number_t num = ecma_string_to_number (property_name_p);
        ecma_string_t *num_to_str = ecma_new_ecma_string_from_number (num);

        if (ecma_compare_ecma_strings (property_name_p, num_to_str))
        {
          ecma_deref_ecma_string (num_to_str);

          return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
        }

        ecma_deref_ecma_string (num_to_str);
      }
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */

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
    else if (ecma_is_normal_or_arrow_function (type))
    {
      if (ecma_string_is_length (property_name_p))
      {
        /* Get length virtual property. */
        const ecma_compiled_code_t *bytecode_data_p;

#ifndef CONFIG_DISABLE_ES2015_ARROW_FUNCTION
        if (type != ECMA_OBJECT_TYPE_ARROW_FUNCTION)
        {
          ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) object_p;
          bytecode_data_p = ECMA_GET_INTERNAL_VALUE_POINTER (const ecma_compiled_code_t,
                                                             ext_func_p->u.function.bytecode_cp);
        }
        else
        {
          ecma_arrow_function_t *arrow_func_p = (ecma_arrow_function_t *) object_p;
          bytecode_data_p = ECMA_GET_NON_NULL_POINTER (const ecma_compiled_code_t,
                                                       arrow_func_p->bytecode_cp);
        }
#else /* CONFIG_DISABLE_ES2015_ARROW_FUNCTION */
        ecma_extended_object_t *ext_func_p = (ecma_extended_object_t *) object_p;
        bytecode_data_p = ECMA_GET_INTERNAL_VALUE_POINTER (const ecma_compiled_code_t,
                                                           ext_func_p->u.function.bytecode_cp);
#endif /* !CONFIG_DISABLE_ES2015_ARROW_FUNCTION */

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
      return ecma_make_simple_value (ECMA_SIMPLE_VALUE_NOT_FOUND);
    }
  }

  ecma_property_value_t *prop_value_p = ECMA_PROPERTY_VALUE_PTR (property_p);

  if (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA)
  {
    return ecma_fast_copy_value (prop_value_p->value);
  }

  JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR);

  ecma_object_t *getter_p = ecma_get_named_accessor_property_getter (prop_value_p);

  if (getter_p == NULL)
  {
    return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
  }

  return ecma_op_function_call (getter_p, base_value, NULL, 0);
} /* ecma_op_object_find_own */

/**
 * Search the value corresponding to a property name
 *
 * Note: search includes prototypes
 *
 * @return ecma value if property is found
 *         ECMA_SIMPLE_VALUE_NOT_FOUND if property is not found
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_object_find (ecma_object_t *object_p, /**< the object */
                     ecma_string_t *property_name_p) /**< property name */
{
  /* Circular reference is possible in JavaScript and testing it is complicated. */
  int max_depth = ECMA_PROPERTY_SEARCH_DEPTH_LIMIT;

  ecma_value_t base_value = ecma_make_object_value (object_p);
  do
  {
    ecma_value_t value = ecma_op_object_find_own (base_value, object_p, property_name_p);

    if (ecma_is_value_found (value))
    {
      return value;
    }

    if (--max_depth == 0)
    {
      break;
    }

    object_p = ecma_get_object_prototype (object_p);
  }
  while (object_p != NULL);

  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_NOT_FOUND);
} /* ecma_op_object_find */

/**
 * Get own property by name
 *
 * Note: property must be an existing data property
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
inline ecma_value_t __attr_always_inline___
ecma_op_object_get_own_data_prop (ecma_object_t *object_p, /**< the object */
                                  ecma_string_t *property_name_p) /**< property name */
{
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
 * [[Get]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 8
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_object_get (ecma_object_t *object_p, /**< the object */
                    ecma_string_t *property_name_p) /**< property name */
{
  /* Circular reference is possible in JavaScript and testing it is complicated. */
  int max_depth = ECMA_PROPERTY_SEARCH_DEPTH_LIMIT;

  ecma_value_t base_value = ecma_make_object_value (object_p);
  do
  {
    ecma_value_t value = ecma_op_object_find_own (base_value, object_p, property_name_p);

    if (ecma_is_value_found (value))
    {
      return value;
    }

    if (--max_depth == 0)
    {
      break;
    }

    object_p = ecma_get_object_prototype (object_p);
  }
  while (object_p != NULL);

  return ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
} /* ecma_op_object_get */

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
 *         Returns with ECMA_SIMPLE_VALUE_TRUE if the operation is
 *         successful. Otherwise it returns with an error object
 *         or ECMA_SIMPLE_VALUE_FALSE.
 *
 *         Note: even if is_throw is false, the setter can throw an
 *         error, and this function returns with that error.
 */
ecma_value_t
ecma_op_object_put (ecma_object_t *object_p, /**< the object */
                    ecma_string_t *property_name_p, /**< property name */
                    ecma_value_t value, /**< ecma value */
                    bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT (object_p != NULL
                && !ecma_is_lexical_environment (object_p));
  JERRY_ASSERT (property_name_p != NULL);

  ecma_object_t *setter_p = NULL;
  ecma_object_type_t type = ecma_get_object_type (object_p);

  switch (type)
  {
    case ECMA_OBJECT_TYPE_ARRAY:
    {
      if (ecma_string_is_length (property_name_p))
      {
        ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

        if (ecma_is_property_writable (ext_object_p->u.array.length_prop))
        {
          return ecma_op_array_object_set_length (object_p, value, 0);
        }

        return ecma_reject (is_throw);
      }
      break;
    }
    case ECMA_OBJECT_TYPE_PSEUDO_ARRAY:
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;

      if (ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_ARGUMENTS)
      {
        uint32_t index = ecma_string_get_array_index (property_name_p);

        if (index != ECMA_STRING_NOT_ARRAY_INDEX)
        {
          if (index < ext_object_p->u.pseudo_array.u1.length)
          {
            jmem_cpointer_t *arg_Literal_p = (jmem_cpointer_t *) (ext_object_p + 1);

            if (arg_Literal_p[index] != JMEM_CP_NULL)
            {
              ecma_string_t *arg_name_p = JMEM_CP_GET_NON_NULL_POINTER (ecma_string_t,
                                                                        arg_Literal_p[index]);

              ecma_object_t *lex_env_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_object_t,
                                                                          ext_object_p->u.pseudo_array.u2.lex_env_cp);

              JERRY_ASSERT (lex_env_p != NULL
                            && ecma_is_lexical_environment (lex_env_p));

              ecma_op_set_mutable_binding (lex_env_p, arg_name_p, value, true);
              return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
            }
          }
        }
      }
#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
      /* ES2015 9.4.5.5 */
      if (ecma_is_typedarray (ecma_make_object_value (object_p)))
      {
        if (ECMA_STRING_GET_CONTAINER (property_name_p) == ECMA_STRING_CONTAINER_UINT32_IN_DESC)
        {
          bool set_status = ecma_op_typedarray_set_index_prop (object_p, property_name_p->u.uint32_number, value);

          if (set_status)
          {
            return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
          }

          return ecma_reject (is_throw);
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
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
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
    else if (ecma_is_normal_or_arrow_function (type))
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
        return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
      }
    }
    else
    {
      JERRY_ASSERT (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR);

      setter_p = ecma_get_named_accessor_property_setter (ECMA_PROPERTY_VALUE_PTR (property_p));
    }
  }
  else
  {
    ecma_object_t *proto_p = ecma_get_object_prototype (object_p);
    bool create_new_property = true;

    if (proto_p != NULL)
    {
      ecma_property_ref_t property_ref = { NULL };

      ecma_property_t inherited_property = ecma_op_object_get_property (proto_p,
                                                                        property_name_p,
                                                                        &property_ref,
                                                                        ECMA_PROPERTY_GET_NO_OPTIONS);

      if (inherited_property != ECMA_PROPERTY_TYPE_NOT_FOUND)
      {
        if (ECMA_PROPERTY_GET_TYPE (inherited_property) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR)
        {
          setter_p = ecma_get_named_accessor_property_setter (property_ref.value_p);
          create_new_property = false;
        }
        else
        {
          create_new_property = ecma_is_property_writable (inherited_property);
        }
      }
    }

    if (create_new_property
        && ecma_get_object_extensible (object_p))
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
                                               true, /* Writable */
                                               true, /* Enumerable */
                                               true, /* Configurable */
                                               is_throw); /* Failure handling */
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
          if (!ecma_is_property_writable (ext_object_p->u.array.length_prop))
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
      return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
    }
  }

  if (setter_p == NULL)
  {
    return ecma_reject (is_throw);
  }

  ecma_value_t ret_value = ecma_op_function_call (setter_p,
                                                  ecma_make_object_value (object_p),
                                                  &value,
                                                  1);

  if (!ECMA_IS_VALUE_ERROR (ret_value))
  {
    ecma_fast_free_value (ret_value);
    ret_value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
  }

  return ret_value;
} /* ecma_op_object_put */

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
   *   [ECMA_OBJECT_TYPE_ARROW_FUNCTION]    = &ecma_op_general_object_default_value
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
                                    const ecma_property_descriptor_t *property_desc_p, /**< property
                                                                                        *   descriptor */
                                    bool is_throw) /**< flag that controls failure handling */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));
  JERRY_ASSERT (property_name_p != NULL);

  const ecma_object_type_t type = ecma_get_object_type (obj_p);

  switch (type)
  {
    case ECMA_OBJECT_TYPE_GENERAL:
    case ECMA_OBJECT_TYPE_CLASS:
    case ECMA_OBJECT_TYPE_FUNCTION:
#ifndef CONFIG_DISABLE_ES2015_ARROW_FUNCTION
    case ECMA_OBJECT_TYPE_ARROW_FUNCTION:
#endif /* !CONFIG_DISABLE_ES2015_ARROW_FUNCTION */
    case ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    {
      return ecma_op_general_object_define_own_property (obj_p,
                                                         property_name_p,
                                                         property_desc_p,
                                                         is_throw);
    }

    case ECMA_OBJECT_TYPE_ARRAY:
    {
      return ecma_op_array_object_define_own_property (obj_p,
                                                       property_name_p,
                                                       property_desc_p,
                                                       is_throw);
    }

    case ECMA_OBJECT_TYPE_PSEUDO_ARRAY:
    {
      ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;

      if (ext_object_p->u.pseudo_array.type == ECMA_PSEUDO_ARRAY_ARGUMENTS)
      {
        return ecma_op_arguments_object_define_own_property (obj_p,
                                                             property_name_p,
                                                             property_desc_p,
                                                             is_throw);
      }
#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
      /* ES2015 9.4.5.3 */
      if (ecma_is_typedarray (ecma_make_object_value (obj_p)))
      {
        if (ECMA_STRING_GET_CONTAINER (property_name_p) == ECMA_STRING_CONTAINER_UINT32_IN_DESC)
        {
          bool define_status = ecma_op_typedarray_define_index_prop (obj_p,
                                                                     property_name_p->u.uint32_number,
                                                                     property_desc_p);

          if (define_status)
          {
            return ecma_make_simple_value (ECMA_SIMPLE_VALUE_TRUE);
          }

          return ecma_reject (is_throw);
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

      return ecma_op_general_object_define_own_property (obj_p,
                                                         property_name_p,
                                                         property_desc_p,
                                                         is_throw);

#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
      break;
    }
    default:
    {
      JERRY_ASSERT (false);
    }
  }

  JERRY_UNREACHABLE ();

  return ecma_reject (is_throw);
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
bool
ecma_op_object_get_own_property_descriptor (ecma_object_t *object_p, /**< the object */
                                            ecma_string_t *property_name_p, /**< property name */
                                            ecma_property_descriptor_t *prop_desc_p) /**< property descriptor */
{
  ecma_property_ref_t property_ref;

  ecma_property_t property = ecma_op_object_get_own_property (object_p,
                                                              property_name_p,
                                                              &property_ref,
                                                              ECMA_PROPERTY_GET_VALUE);

  if (property == ECMA_PROPERTY_TYPE_NOT_FOUND || property == ECMA_PROPERTY_TYPE_NOT_FOUND_AND_STOP)
  {
    return false;
  }

  *prop_desc_p = ecma_make_empty_property_descriptor ();

  prop_desc_p->is_enumerable = ecma_is_property_enumerable (property);
  prop_desc_p->is_enumerable_defined = true;
  prop_desc_p->is_configurable = ecma_is_property_configurable (property);
  prop_desc_p->is_configurable_defined = true;

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

    prop_desc_p->is_value_defined = true;
    prop_desc_p->is_writable = ecma_is_property_writable (property);
    prop_desc_p->is_writable_defined = true;
  }
  else
  {
    prop_desc_p->get_p = ecma_get_named_accessor_property_getter (property_ref.value_p);
    prop_desc_p->is_get_defined = true;

    if (prop_desc_p->get_p != NULL)
    {
      ecma_ref_object (prop_desc_p->get_p);
    }

    prop_desc_p->set_p = ecma_get_named_accessor_property_setter (property_ref.value_p);
    prop_desc_p->is_set_defined = true;

    if (prop_desc_p->set_p != NULL)
    {
      ecma_ref_object (prop_desc_p->set_p);
    }
  }

  return true;
} /* ecma_op_object_get_own_property_descriptor */

/**
 * [[HasInstance]] ecma object's operation
 *
 * See also:
 *          ECMA-262 v5, 8.6.2; ECMA-262 v5, Table 9
 */
ecma_value_t
ecma_op_object_has_instance (ecma_object_t *obj_p, /**< the object */
                             ecma_value_t value) /**< argument 'V' */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));

  const ecma_object_type_t type = ecma_get_object_type (obj_p);

  if (ecma_is_normal_or_arrow_function (type)
      || type == ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION
      || type == ECMA_OBJECT_TYPE_BOUND_FUNCTION)
  {
    return ecma_op_function_has_instance (obj_p, value);
  }

  JERRY_ASSERT_OBJECT_TYPE_IS_VALID (type);

  return ecma_raise_type_error (ECMA_ERR_MSG ("Expected a function object."));
} /* ecma_op_object_has_instance */

/**
 * Object's isPrototypeOf operation
 *
 * See also:
 *          ECMA-262 v5, 15.2.4.6; 3
 *
 * @return true - if the target object is prototype of the base object
 *         false - if the target object is not prototype of the base object
 */
bool
ecma_op_object_is_prototype_of (ecma_object_t *base_p, /**< base object */
                                ecma_object_t *target_p) /**< target object */
{
  do
  {
    target_p = ecma_get_object_prototype (target_p);
    if (target_p == NULL)
    {
      return false;
    }
    else if (target_p == base_p)
    {
      return true;
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
 * @return collection of strings - property names
 */
ecma_collection_header_t *
ecma_op_object_get_property_names (ecma_object_t *obj_p, /**< object */
                                   bool is_array_indices_only, /**< true - exclude properties with names
                                                                *          that are not indices */
                                   bool is_enumerable_only, /**< true - exclude non-enumerable properties */
                                   bool is_with_prototype_chain) /**< true - list properties from prototype chain,
                                                                  *   false - list only own properties */
{
  JERRY_ASSERT (obj_p != NULL
                && !ecma_is_lexical_environment (obj_p));

  ecma_collection_header_t *ret_p = ecma_new_strings_collection (NULL, 0);
  ecma_collection_header_t *skipped_non_enumerable_p = ecma_new_strings_collection (NULL, 0);

  const ecma_object_type_t type = ecma_get_object_type (obj_p);
  const bool obj_is_builtin = ecma_get_object_is_builtin (obj_p);

  const size_t bitmap_row_size = sizeof (uint32_t) * JERRY_BITSINBYTE;
  uint32_t names_hashes_bitmap[ECMA_OBJECT_HASH_BITMAP_SIZE / bitmap_row_size];

  memset (names_hashes_bitmap, 0, sizeof (names_hashes_bitmap));

  for (ecma_object_t *prototype_chain_iter_p = obj_p;
       prototype_chain_iter_p != NULL;
       prototype_chain_iter_p = is_with_prototype_chain ? ecma_get_object_prototype (prototype_chain_iter_p)
                                                        : NULL)
  {
    ecma_length_t string_named_properties_count = 0;
    ecma_length_t array_index_named_properties_count = 0;

    ecma_collection_header_t *prop_names_p = ecma_new_strings_collection (NULL, 0);

    if (obj_is_builtin)
    {
      ecma_builtin_list_lazy_property_names (obj_p,
                                             is_enumerable_only,
                                             prop_names_p,
                                             skipped_non_enumerable_p);
    }
    else
    {
      switch (type)
      {
        case ECMA_OBJECT_TYPE_GENERAL:
        {
          break;
        }
        case ECMA_OBJECT_TYPE_PSEUDO_ARRAY:
        {
#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
          if (ecma_is_typedarray (ecma_make_object_value (obj_p)))
          {
            ecma_op_typedarray_list_lazy_property_names (obj_p, prop_names_p);
          }
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
          break;
        }
        case ECMA_OBJECT_TYPE_FUNCTION:
#ifndef CONFIG_DISABLE_ES2015_ARROW_FUNCTION
        case ECMA_OBJECT_TYPE_ARROW_FUNCTION:
#endif /* !CONFIG_DISABLE_ES2015_ARROW_FUNCTION */
        {
          ecma_op_function_list_lazy_property_names (obj_p,
                                                     is_enumerable_only,
                                                     prop_names_p,
                                                     skipped_non_enumerable_p);
          break;
        }
        case ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION:
        {
          ecma_op_external_function_list_lazy_property_names (is_enumerable_only,
                                                              prop_names_p,
                                                              skipped_non_enumerable_p);
          break;
        }
        case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
        {
          ecma_op_bound_function_list_lazy_property_names (is_enumerable_only,
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
                                                     is_enumerable_only,
                                                     prop_names_p,
                                                     skipped_non_enumerable_p);
          }

          break;
        }
        case ECMA_OBJECT_TYPE_ARRAY:
        {
          ecma_op_array_list_lazy_property_names (obj_p,
                                                  is_enumerable_only,
                                                  prop_names_p,
                                                  skipped_non_enumerable_p);
          break;
        }
        default:
        {
          JERRY_UNREACHABLE ();
          break;
        }
      }
    }

    ecma_collection_iterator_t iter;
    ecma_collection_iterator_init (&iter, prop_names_p);

    uint32_t own_names_hashes_bitmap[ECMA_OBJECT_HASH_BITMAP_SIZE / bitmap_row_size];
    memset (own_names_hashes_bitmap, 0, sizeof (own_names_hashes_bitmap));

    while (ecma_collection_iterator_next (&iter))
    {
      ecma_string_t *name_p = ecma_get_string_from_value (*iter.current_value_p);

      uint8_t hash = (uint8_t) name_p->hash;
      uint32_t bitmap_row = (uint32_t) (hash / bitmap_row_size);
      uint32_t bitmap_column = (uint32_t) (hash % bitmap_row_size);

      if ((own_names_hashes_bitmap[bitmap_row] & (1u << bitmap_column)) == 0)
      {
        own_names_hashes_bitmap[bitmap_row] |= (1u << bitmap_column);
      }
    }

    ecma_property_header_t *prop_iter_p = ecma_get_property_list (prototype_chain_iter_p);

    if (prop_iter_p != NULL && prop_iter_p->types[0] == ECMA_PROPERTY_TYPE_HASHMAP)
    {
      prop_iter_p = ECMA_GET_POINTER (ecma_property_header_t,
                                      prop_iter_p->next_property_cp);
    }

    while (prop_iter_p != NULL)
    {
      JERRY_ASSERT (ECMA_PROPERTY_IS_PROPERTY_PAIR (prop_iter_p));

      for (int i = 0; i < ECMA_PROPERTY_PAIR_ITEM_COUNT; i++)
      {
        ecma_property_t *property_p = prop_iter_p->types + i;

        if (ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDDATA
            || ECMA_PROPERTY_GET_TYPE (*property_p) == ECMA_PROPERTY_TYPE_NAMEDACCESSOR)
        {
          ecma_property_pair_t *prop_pair_p = (ecma_property_pair_t *) prop_iter_p;

          if (ECMA_PROPERTY_GET_NAME_TYPE (*property_p) == ECMA_STRING_CONTAINER_MAGIC_STRING
              && prop_pair_p->names_cp[i] >= LIT_NON_INTERNAL_MAGIC_STRING__COUNT)
          {
            /* Internal properties are never enumerated. */
            continue;
          }

          ecma_string_t *name_p = ecma_string_from_property_name (*property_p,
                                                                  prop_pair_p->names_cp[i]);

          if (!(is_enumerable_only && !ecma_is_property_enumerable (*property_p)))
          {
            uint8_t hash = (uint8_t) name_p->hash;
            uint32_t bitmap_row = (uint32_t) (hash / bitmap_row_size);
            uint32_t bitmap_column = (uint32_t) (hash % bitmap_row_size);

            bool is_add = true;

            if ((own_names_hashes_bitmap[bitmap_row] & (1u << bitmap_column)) != 0)
            {
              ecma_collection_iterator_init (&iter, prop_names_p);

              while (ecma_collection_iterator_next (&iter))
              {
                ecma_string_t *name2_p = ecma_get_string_from_value (*iter.current_value_p);

                if (ecma_compare_ecma_strings (name_p, name2_p))
                {
                  is_add = false;
                  break;
                }
              }
            }

            if (is_add)
            {
              own_names_hashes_bitmap[bitmap_row] |= (1u << bitmap_column);

              ecma_append_to_values_collection (prop_names_p,
                                                ecma_make_string_value (name_p),
                                                true);
            }
          }
          else
          {
            JERRY_ASSERT (is_enumerable_only && !ecma_is_property_enumerable (*property_p));

            ecma_append_to_values_collection (skipped_non_enumerable_p,
                                              ecma_make_string_value (name_p),
                                              true);
          }

          ecma_deref_ecma_string (name_p);
        }
      }

      prop_iter_p = ECMA_GET_POINTER (ecma_property_header_t,
                                      prop_iter_p->next_property_cp);
    }

    ecma_collection_iterator_init (&iter, prop_names_p);
    while (ecma_collection_iterator_next (&iter))
    {
      ecma_string_t *name_p = ecma_get_string_from_value (*iter.current_value_p);

      uint32_t index = ecma_string_get_array_index (name_p);

      if (index != ECMA_STRING_NOT_ARRAY_INDEX)
      {
        /* The name is a valid array index. */
        array_index_named_properties_count++;
      }
      else if (!is_array_indices_only)
      {
        string_named_properties_count++;
      }
    }

    /* Second pass: collecting property names into arrays. */
    JMEM_DEFINE_LOCAL_ARRAY (names_p,
                             array_index_named_properties_count + string_named_properties_count,
                             ecma_string_t *);
    JMEM_DEFINE_LOCAL_ARRAY (array_index_names_p, array_index_named_properties_count, uint32_t);

    uint32_t name_pos = array_index_named_properties_count + string_named_properties_count;
    uint32_t array_index_name_pos = 0;

    ecma_collection_iterator_init (&iter, prop_names_p);
    while (ecma_collection_iterator_next (&iter))
    {
      ecma_string_t *name_p = ecma_get_string_from_value (*iter.current_value_p);

      uint32_t index = ecma_string_get_array_index (name_p);

      if (index != ECMA_STRING_NOT_ARRAY_INDEX)
      {
        JERRY_ASSERT (array_index_name_pos < array_index_named_properties_count);

        uint32_t insertion_pos = 0;
        while (insertion_pos < array_index_name_pos
               && index < array_index_names_p[insertion_pos])
        {
          insertion_pos++;
        }

        if (insertion_pos == array_index_name_pos)
        {
          array_index_names_p[array_index_name_pos++] = index;
        }
        else
        {
          JERRY_ASSERT (insertion_pos < array_index_name_pos);
          JERRY_ASSERT (index >= array_index_names_p[insertion_pos]);

          uint32_t move_pos = array_index_name_pos++;

          while (move_pos > insertion_pos)
          {
            array_index_names_p[move_pos] = array_index_names_p[move_pos - 1u];

            move_pos--;
          }

          array_index_names_p[insertion_pos] = index;
        }
      }
      else if (!is_array_indices_only)
      {
        /*
         * Filling from end to begin, as list of object's properties is sorted
         * in order that is reverse to properties creation order
         */

        JERRY_ASSERT (name_pos > 0
                      && name_pos <= array_index_named_properties_count + string_named_properties_count);
        ecma_ref_ecma_string (name_p);
        names_p[--name_pos] = name_p;
      }
    }

    for (uint32_t i = 0; i < array_index_named_properties_count; i++)
    {
      JERRY_ASSERT (name_pos > 0
                    && name_pos <= array_index_named_properties_count + string_named_properties_count);
      names_p[--name_pos] = ecma_new_ecma_string_from_uint32 (array_index_names_p[i]);
    }

    JERRY_ASSERT (name_pos == 0);

    JMEM_FINALIZE_LOCAL_ARRAY (array_index_names_p);

    ecma_free_values_collection (prop_names_p, true);

    /* Third pass:
     *   embedding own property names of current object of prototype chain to aggregate property names collection */
    for (uint32_t i = 0;
         i < array_index_named_properties_count + string_named_properties_count;
         i++)
    {
      bool is_append = true;

      ecma_string_t *name_p = names_p[i];

      uint8_t hash = (uint8_t) name_p->hash;
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
        ecma_collection_iterator_init (&iter, ret_p);

        while (ecma_collection_iterator_next (&iter))
        {
          ecma_string_t *iter_name_p = ecma_get_string_from_value (*iter.current_value_p);

          if (ecma_compare_ecma_strings (name_p, iter_name_p))
          {
            is_append = false;
            break;
          }
        }
      }

      if (is_append)
      {
        ecma_collection_iterator_init (&iter, skipped_non_enumerable_p);
        while (ecma_collection_iterator_next (&iter))
        {
          ecma_string_t *iter_name_p = ecma_get_string_from_value (*iter.current_value_p);

          if (ecma_compare_ecma_strings (name_p, iter_name_p))
          {
            is_append = false;
            break;
          }
        }
      }

      if (is_append)
      {
        JERRY_ASSERT ((names_hashes_bitmap[bitmap_row] & (1u << bitmap_column)) != 0);

        ecma_append_to_values_collection (ret_p, ecma_make_string_value (names_p[i]), true);
      }

      ecma_deref_ecma_string (name_p);
    }

    JMEM_FINALIZE_LOCAL_ARRAY (names_p);
  }

  ecma_free_values_collection (skipped_non_enumerable_p, true);

  return ret_p;
} /* ecma_op_object_get_property_names */

/**
 * The function is used in the assert of ecma_object_get_class_name
 */
inline static bool
ecma_object_check_class_name_is_object (ecma_object_t *obj_p) /**< object */
{
#ifndef JERRY_NDEBUG
  return (ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_GLOBAL)
#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_PROMISE_PROTOTYPE)
#endif /* !CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
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
#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
          || ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_FLOAT64ARRAY_PROTOTYPE)
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64 */
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
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
      return ext_object_p->u.class_prop.class_id;
    }
    case ECMA_OBJECT_TYPE_PSEUDO_ARRAY:
    {
      ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

      switch (ext_obj_p->u.pseudo_array.type)
      {
        case ECMA_PSEUDO_ARRAY_ARGUMENTS:
        {
          return LIT_MAGIC_STRING_ARGUMENTS_UL;
        }
#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
        case ECMA_PSEUDO_ARRAY_TYPEDARRAY:
        case ECMA_PSEUDO_ARRAY_TYPEDARRAY_WITH_INFO:
        {
          return ext_obj_p->u.pseudo_array.u1.class_id;
        }
#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
        default:
        {
          JERRY_UNREACHABLE ();
        }
      }

      JERRY_UNREACHABLE ();
      break;
    }
    case ECMA_OBJECT_TYPE_FUNCTION:
#ifndef CONFIG_DISABLE_ES2015_ARROW_FUNCTION
    case ECMA_OBJECT_TYPE_ARROW_FUNCTION:
#endif /* !CONFIG_DISABLE_ES2015_ARROW_FUNCTION */
    case ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION:
    case ECMA_OBJECT_TYPE_BOUND_FUNCTION:
    {
      return LIT_MAGIC_STRING_FUNCTION_UL;
    }
    default:
    {
      JERRY_ASSERT (type == ECMA_OBJECT_TYPE_GENERAL);

      if (ecma_get_object_is_builtin (obj_p))
      {
        ecma_extended_object_t *ext_obj_p = (ecma_extended_object_t *) obj_p;

        switch (ext_obj_p->u.built_in.id)
        {
#ifndef CONFIG_DISABLE_MATH_BUILTIN
          case ECMA_BUILTIN_ID_MATH:
          {
            return LIT_MAGIC_STRING_MATH_UL;
          }
#endif /* !CONFIG_DISABLE_MATH_BUILTIN */
#ifndef CONFIG_DISABLE_JSON_BUILTIN
          case ECMA_BUILTIN_ID_JSON:
          {
            return LIT_MAGIC_STRING_JSON_U;
          }
#endif /* !CONFIG_DISABLE_JSON_BUILTIN */
#ifndef CONFIG_DISABLE_ERROR_BUILTINS
          case ECMA_BUILTIN_ID_EVAL_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_RANGE_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_REFERENCE_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_SYNTAX_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_TYPE_ERROR_PROTOTYPE:
          case ECMA_BUILTIN_ID_URI_ERROR_PROTOTYPE:
#endif /* !CONFIG_DISABLE_ERROR_BUILTINS */
          case ECMA_BUILTIN_ID_ERROR_PROTOTYPE:
          {
            return LIT_MAGIC_STRING_ERROR_UL;
          }
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
 *         ECMA_SIMPLE_VALUE_NOT_FOUND otherwise
 */
inline bool __attr_always_inline___
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
 * @}
 * @}
 */
