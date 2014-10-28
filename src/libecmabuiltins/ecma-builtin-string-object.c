/* Copyright 2014 Samsung Electronics Co., Ltd.
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
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-string-object.h"
#include "ecma-try-catch-macro.h"
#include "globals.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup string ECMA String object built-in
 * @{
 */

/**
 * List of the String object built-in object value properties in format 'macro (name, value)'.
 */
#define ECMA_BUILTIN_STRING_OBJECT_OBJECT_VALUES_PROPERTY_LIST(macro) \
  macro (ECMA_MAGIC_STRING_PROTOTYPE, ecma_builtin_get (ECMA_BUILTIN_ID_STRING_PROTOTYPE))

/**
 * List of the String object built-in routine properties in format
 * 'macro (name, C function name, arguments number of the routine, length value of the routine)'.
 */
#define ECMA_BUILTIN_STRING_OBJECT_ROUTINES_PROPERTY_LIST(macro) \
  macro (ECMA_MAGIC_STRING_FROM_CHAR_CODE_UL, \
         ecma_builtin_string_object_from_char_code, \
         NON_FIXED, \
         1)

/**
 * List of the String object's built-in property names
 */
static const ecma_magic_string_id_t ecma_builtin_string_property_names[] =
{
#define VALUE_PROP_LIST(name, value) name,
#define ROUTINE_PROP_LIST(name, c_function_name, args_number, length) name,
  ECMA_BUILTIN_STRING_OBJECT_OBJECT_VALUES_PROPERTY_LIST (VALUE_PROP_LIST)
  ECMA_BUILTIN_STRING_OBJECT_ROUTINES_PROPERTY_LIST (ROUTINE_PROP_LIST)
#undef VALUE_PROP_LIST
#undef ROUTINE_PROP_LIST
};

/**
 * Number of the String object's built-in properties
 */
static const ecma_length_t ecma_builtin_string_property_number = (sizeof (ecma_builtin_string_property_names) /
                                                                  sizeof (ecma_magic_string_id_t));

/**
 * The String object's 'fromCharCode' routine
 *
 * See also:
 *          ECMA-262 v5, 15.5.3.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
static ecma_completion_value_t
ecma_builtin_string_object_from_char_code (ecma_value_t args[], /**< arguments list */
                                           ecma_length_t args_number) /**< number of arguments */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  size_t zt_str_buffer_size = sizeof (ecma_char_t) * (args_number + 1u);

  ecma_char_t *ret_zt_str_p = mem_heap_alloc_block (zt_str_buffer_size,
                                                    MEM_HEAP_ALLOC_SHORT_TERM);
  ret_zt_str_p [args_number] = ECMA_CHAR_NULL;

  for (ecma_length_t arg_index = 0;
       arg_index < args_number;
       arg_index++)
  {
    ECMA_TRY_CATCH (arg_num_value,
                    ecma_op_to_number (args[arg_index]),
                    ret_value);

    JERRY_ASSERT (arg_num_value.u.value.value_type == ECMA_TYPE_NUMBER);
    ecma_number_t *arg_num_p = ECMA_GET_POINTER (arg_num_value.u.value.value);

    uint32_t uint32_char_code = ecma_number_to_uint32 (*arg_num_p);
    uint16_t uint16_char_code = (uint16_t) uint32_char_code;

#if CONFIG_ECMA_CHAR_ENCODING == CONFIG_ECMA_CHAR_ASCII
    if ((uint16_char_code >> JERRY_BITSINBYTE) != 0)
    {
      ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
    }
    else
    {
      ret_zt_str_p [arg_index] = (ecma_char_t) uint16_char_code;
    }
#elif CONFIG_ECMA_CHAR_ENCODING == CONFIG_ECMA_CHAR_UTF16
    ret_zt_str_p [arg_index] = (ecma_char_t) uint16_char_code;
#endif /* CONFIG_ECMA_CHAR_ENCODING == CONFIG_ECMA_CHAR_UTF16 */

    ECMA_FINALIZE (arg_num_value);

    if (ecma_is_completion_value_throw (ret_value))
    {
      mem_heap_free_block (ret_zt_str_p);

      return ret_value;
    }

    JERRY_ASSERT (ecma_is_completion_value_empty (ret_value));
  }

  ecma_string_t *ret_str_p = ecma_new_ecma_string (ret_zt_str_p);

  mem_heap_free_block (ret_zt_str_p);

  return ecma_make_normal_completion_value (ecma_make_string_value (ret_str_p));
} /* ecma_builtin_string_object_from_char_code */

/**
 * If the property's name is one of built-in properties of the String object
 * that is not instantiated yet, instantiate the property and
 * return pointer to the instantiated property.
 *
 * @return pointer property, if one was instantiated,
 *         NULL - otherwise.
 */
ecma_property_t*
ecma_builtin_string_try_to_instantiate_property (ecma_object_t *obj_p, /**< object */
                                                 ecma_string_t *prop_name_p) /**< property's name */
{
  JERRY_ASSERT (ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_STRING));
  JERRY_ASSERT (ecma_find_named_property (obj_p, prop_name_p) == NULL);

  ecma_magic_string_id_t id;

  if (!ecma_is_string_magic (prop_name_p, &id))
  {
    return NULL;
  }

  int32_t index = ecma_builtin_bin_search_for_magic_string_id_in_array (ecma_builtin_string_property_names,
                                                                        ecma_builtin_string_property_number,
                                                                        id);

  if (index == -1)
  {
    return NULL;
  }

  JERRY_ASSERT (index >= 0 && (uint32_t) index < sizeof (uint64_t) * JERRY_BITSINBYTE);

  uint32_t bit;
  ecma_internal_property_id_t mask_prop_id;

  if (index >= 32)
  {
    mask_prop_id = ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_32_63;
    bit = (uint32_t) 1u << (index - 32);
  }
  else
  {
    mask_prop_id = ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_0_31;
    bit = (uint32_t) 1u << index;
  }

  ecma_property_t *mask_prop_p = ecma_find_internal_property (obj_p, mask_prop_id);
  if (mask_prop_p == NULL)
  {
    mask_prop_p = ecma_create_internal_property (obj_p, mask_prop_id);
    mask_prop_p->u.internal_property.value = 0;
  }

  uint32_t bit_mask = mask_prop_p->u.internal_property.value;

  if (bit_mask & bit)
  {
    return NULL;
  }

  bit_mask |= bit;

  mask_prop_p->u.internal_property.value = bit_mask;

  ecma_value_t value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_EMPTY);
  ecma_property_writable_value_t writable = ECMA_PROPERTY_WRITABLE;
  ecma_property_enumerable_value_t enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;
  ecma_property_configurable_value_t configurable = ECMA_PROPERTY_CONFIGURABLE;

  switch (id)
  {
#define CASE_ROUTINE_PROP_LIST(name, c_function_name, args_number, length) case name: \
    { \
      ecma_object_t *func_obj_p = ecma_builtin_make_function_object_for_routine (ECMA_BUILTIN_ID_STRING, id, length); \
      \
      value = ecma_make_object_value (func_obj_p); \
      \
      break; \
    }
    ECMA_BUILTIN_STRING_OBJECT_ROUTINES_PROPERTY_LIST (CASE_ROUTINE_PROP_LIST)
#undef CASE_ROUTINE_PROP_LIST
#define CASE_VALUE_PROP_LIST(name, value) case name:
    ECMA_BUILTIN_STRING_OBJECT_OBJECT_VALUES_PROPERTY_LIST (CASE_VALUE_PROP_LIST)
#undef CASE_VALUE_PROP_LIST
    {
      writable = ECMA_PROPERTY_NOT_WRITABLE;
      enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;
      configurable = ECMA_PROPERTY_NOT_CONFIGURABLE;

      switch (id)
      {
#define CASE_OBJECT_VALUE_PROP_LIST(name, value_obj) case name: { value = ecma_make_object_value (value_obj); break; }
        ECMA_BUILTIN_STRING_OBJECT_OBJECT_VALUES_PROPERTY_LIST (CASE_OBJECT_VALUE_PROP_LIST)
#undef CASE_OBJECT_VALUE_PROP_LIST
        default:
        {
          JERRY_UNREACHABLE ();
        }
      }

      break;
    }

    default:
    {
      JERRY_UNREACHABLE ();
    }
  }

  ecma_property_t *prop_p = ecma_create_named_data_property (obj_p,
                                                             prop_name_p,
                                                             writable,
                                                             enumerable,
                                                             configurable);

  prop_p->u.named_data_property.value = ecma_copy_value (value, false);
  ecma_gc_update_may_ref_younger_object_flag_by_value (obj_p,
                                                       prop_p->u.named_data_property.value);

  ecma_free_value (value, true);

  return prop_p;
} /* ecma_builtin_string_try_to_instantiate_property */

/**
 * Dispatcher of the String object's built-in routines
 *
 * @return completion-value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_builtin_string_dispatch_routine (ecma_magic_string_id_t builtin_routine_id, /**< String object's
                                                                                      built-in routine's name */
                                      ecma_value_t this_arg_value __unused, /**< 'this' argument value */
                                      ecma_value_t arguments_list [], /**< list of arguments passed to routine */
                                      ecma_length_t arguments_number) /**< length of arguments' list */
{
  switch (builtin_routine_id)
  {
#define ROUTINE_ARG_LIST_NON_FIXED arguments_list, arguments_number
#define CASE_ROUTINE_PROP_LIST(name, c_function_name, args_number, length) \
       case name: \
       { \
         return c_function_name (ROUTINE_ARG_LIST_ ## args_number); \
       }
    ECMA_BUILTIN_STRING_OBJECT_ROUTINES_PROPERTY_LIST (CASE_ROUTINE_PROP_LIST)
#undef CASE_ROUTINE_PROP_LIST
#undef ROUTINE_ARG_LIST_NON_FIXED

    default:
    {
      JERRY_UNREACHABLE ();
    }
  }
} /* ecma_builtin_string_dispatch_routine */

/**
 * Handle calling [[Call]] of built-in String object
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_string_dispatch_call (ecma_value_t *arguments_list_p, /**< arguments list */
                                   ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  ecma_completion_value_t ret_value;

  if (arguments_list_len == 0)
  {
    ecma_string_t *str_p = ecma_new_ecma_string_from_magic_string_id (ECMA_MAGIC_STRING__EMPTY);
    ecma_value_t str_value = ecma_make_string_value (str_p);

    ret_value = ecma_make_normal_completion_value (str_value);
  }
  else
  {
    ret_value = ecma_op_to_string (arguments_list_p [0]);
  }

  return ret_value;
} /* ecma_builtin_string_dispatch_call */

/**
 * Handle calling [[Construct]] of built-in String object
 *
 * @return completion-value
 */
ecma_completion_value_t
ecma_builtin_string_dispatch_construct (ecma_value_t *arguments_list_p, /**< arguments list */
                                        ecma_length_t arguments_list_len) /**< number of arguments */
{
  JERRY_ASSERT (arguments_list_len == 0 || arguments_list_p != NULL);

  return ecma_op_create_string_object (arguments_list_p, arguments_list_len);
} /* ecma_builtin_string_dispatch_construct */

/**
 * @}
 * @}
 * @}
 */
