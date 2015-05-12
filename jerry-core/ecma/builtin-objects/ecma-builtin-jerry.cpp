/* Copyright 2015 Samsung Electronics Co., Ltd.
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

#include <string.h>

#include "ecma-alloc.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-extension.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "ecma-objects-general.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/**
 * List of registered extensions
 */
static jerry_extension_descriptor_t *jerry_extensions_list_p = NULL;

/**
 * Index to assign to next registered extension
 */
static uint32_t jerry_extensions_next_index = 0;

/**
 * If the property's name is one of built-in properties of the built-in object
 * that is not instantiated yet, instantiate the property and
 * return pointer to the instantiated property.
 *
 * @return pointer property, if one was instantiated,
 *         NULL - otherwise.
 */
ecma_property_t*
ecma_builtin_jerry_try_to_instantiate_property (ecma_object_t *obj_p, /**< object */
                                                ecma_string_t *extension_name_p) /**< property's name */
{
  JERRY_ASSERT (ecma_builtin_is (obj_p, ECMA_BUILTIN_ID_JERRY));
  JERRY_ASSERT (ecma_find_named_property (obj_p, extension_name_p) == NULL);

  ssize_t req_buffer_size = ecma_string_to_zt_string (extension_name_p, NULL, 0);
  JERRY_ASSERT (req_buffer_size < 0);

  ecma_property_t *prop_p = NULL;

  MEM_DEFINE_LOCAL_ARRAY (extension_name_zt_buf_p, -req_buffer_size, uint8_t);

  req_buffer_size = ecma_string_to_zt_string (extension_name_p, extension_name_zt_buf_p, -req_buffer_size);
  JERRY_ASSERT (req_buffer_size > 0);

#if CONFIG_ECMA_CHAR_ENCODING != CONFIG_ECMA_CHAR_ASCII
  JERRY_UNIMPLEMENTED ("Only ASCII encoding support is implemented.");
#else /* CONFIG_ECMA_CHAR_ENCODING == CONFIG_ECMA_CHAR_ASCII */
  const char *name_p = (const char*) extension_name_zt_buf_p;

  jerry_extension_descriptor_t *desc_p;
  for (desc_p = jerry_extensions_list_p;
       desc_p != NULL;
       desc_p = desc_p->next_p)
  {
    if (!strcmp (name_p, desc_p->name_p))
    {
      break;
    }
  }

  if (desc_p == NULL)
  {
    /* no extension with specified name was found */
    prop_p = NULL;
  }
  else
  {
    prop_p = ecma_create_named_data_property (obj_p,
                                              extension_name_p,
                                              ECMA_PROPERTY_NOT_WRITABLE,
                                              ECMA_PROPERTY_NOT_ENUMERABLE,
                                              ECMA_PROPERTY_NOT_CONFIGURABLE);

    ecma_object_t *extension_object_p = ecma_create_object (NULL,
                                                            false,
                                                            ECMA_OBJECT_TYPE_EXTENSION);
    ecma_set_object_is_builtin (extension_object_p, true);
    ecma_property_t *extension_id_prop_p = ecma_create_internal_property (extension_object_p,
                                                                          ECMA_INTERNAL_PROPERTY_EXTENSION_ID);
    extension_id_prop_p->u.internal_property.value = desc_p->index;

    ecma_named_data_property_assign_value (obj_p, prop_p, ecma_make_object_value (extension_object_p));

    ecma_deref_object (extension_object_p);
  }
#endif /* CONFIG_ECMA_CHAR_ENCODING == CONFIG_ECMA_CHAR_ASCII */

  MEM_FINALIZE_LOCAL_ARRAY (extension_name_zt_buf_p);

  return prop_p;
} /* ecma_builtin_jerry_try_to_instantiate_property */

/**
 * Stub for dispatcher of the built-in's routines
 *
 * Warning:
 *         does not return (the stub should be unreachable)
 */
ecma_completion_value_t
ecma_builtin_jerry_dispatch_routine (uint16_t builtin_routine_id, /**< built-in wide identifier of routine */
                                     ecma_value_t this_arg_value __attr_unused___, /**< 'this' argument value */
                                     const ecma_value_t arguments_list[], /**< list of arguments
                                                                           *   passed to routine */
                                     ecma_length_t arguments_number) /**< length of arguments' list */
{
  uint32_t extension_object_index = builtin_routine_id / ECMA_EXTENSION_MAX_FUNCTIONS_IN_EXTENSION;
  uint32_t function_index = builtin_routine_id % ECMA_EXTENSION_MAX_FUNCTIONS_IN_EXTENSION;

  jerry_extension_descriptor_t *desc_p;
  for (desc_p = jerry_extensions_list_p;
       desc_p != NULL;
       desc_p = desc_p->next_p)
  {
    if (desc_p->index == extension_object_index)
    {
      break;
    }
  }
  JERRY_ASSERT (desc_p != NULL);

  JERRY_ASSERT (function_index < desc_p->functions_count);
  jerry_extension_function_t *function_p = &desc_p->functions_p[function_index];

  bool throw_type_error = false;
  if (function_p->args_number != arguments_number)
  {
    throw_type_error = true;
  }
  else
  {
    uint32_t arg_index;
    for (arg_index = 0; arg_index < function_p->args_number; arg_index++)
    {
      jerry_api_value_t *arg_p = &function_p->args_p[arg_index];
      const ecma_value_t arg_value = arguments_list[arg_index];

      if (arg_p->type == JERRY_API_DATA_TYPE_BOOLEAN)
      {
        if (!ecma_is_value_boolean (arg_value))
        {
          break;
        }
        else
        {
          arg_p->v_bool = ecma_is_value_true (arg_value);
        }
      }
      else if (arg_p->type == JERRY_API_DATA_TYPE_FLOAT32
               || arg_p->type == JERRY_API_DATA_TYPE_FLOAT64
               || arg_p->type == JERRY_API_DATA_TYPE_UINT32)
      {
        if (!ecma_is_value_number (arg_value))
        {
          break;
        }
        else
        {
          ecma_number_t num_value = *ecma_get_number_from_value (arg_value);
          if (arg_p->type == JERRY_API_DATA_TYPE_FLOAT32)
          {
            arg_p->v_float32 = (float) num_value;
          }
          else if (arg_p->type == JERRY_API_DATA_TYPE_FLOAT64)
          {
#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32
            JERRY_UNREACHABLE ();
#elif CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
            arg_p->v_float64 = num_value;
#endif /* CONFIG_ECMA_NUMBER_TYPE ==  CONFIG_ECMA_NUMBER_FLOAT64 */
          }
          else if (arg_p->type == JERRY_API_DATA_TYPE_UINT32)
          {
            arg_p->v_uint32 = ecma_number_to_uint32 (num_value);
          }
        }
      }
      else if (arg_p->type == JERRY_API_DATA_TYPE_STRING)
      {
        if (!ecma_is_value_string (arg_value))
        {
          break;
        }
        else
        {
          arg_p->v_string = ecma_get_string_from_value (arg_value);
        }
      }
      else
      {
        JERRY_ASSERT (arg_p->type == JERRY_API_DATA_TYPE_OBJECT);

        if (!ecma_is_value_object (arg_value))
        {
          break;
        }
        else
        {
          arg_p->v_object = ecma_get_object_from_value (arg_value);
        }
      }
    }

    uint32_t initialized_args_count = arg_index;

    if (initialized_args_count != function_p->args_number)
    {
      throw_type_error = true;
    }
    else
    {
      function_p->function_wrapper_p (function_p);
    }

    for (arg_index = 0;
         arg_index < initialized_args_count;
         arg_index++)
    {
      jerry_api_value_t *arg_p = &function_p->args_p[arg_index];

      if (arg_p->type == JERRY_API_DATA_TYPE_STRING)
      {
        arg_p->v_string = NULL;
      }
      else if (arg_p->type == JERRY_API_DATA_TYPE_OBJECT)
      {
        arg_p->v_object = NULL;
      }
      else
      {
        JERRY_ASSERT (arg_p->type == JERRY_API_DATA_TYPE_BOOLEAN
                      || arg_p->type == JERRY_API_DATA_TYPE_FLOAT32
                      || arg_p->type == JERRY_API_DATA_TYPE_FLOAT64
                      || arg_p->type == JERRY_API_DATA_TYPE_UINT32);
      }
    }
  }

  if (throw_type_error)
  {
    return ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    jerry_api_value_t& ret_value = function_p->ret_value;
    ecma_completion_value_t completion;

    if (ret_value.type == JERRY_API_DATA_TYPE_VOID)
    {
      completion = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
    else if (ret_value.type == JERRY_API_DATA_TYPE_BOOLEAN)
    {
      if (ret_value.v_bool)
      {
        completion = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
      }
      else
      {
        completion = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_FALSE);
      }
    }
    else if (ret_value.type == JERRY_API_DATA_TYPE_UINT32
             || ret_value.type == JERRY_API_DATA_TYPE_FLOAT32
             || ret_value.type == JERRY_API_DATA_TYPE_FLOAT64)
    {
      ecma_number_t* num_value_p = ecma_alloc_number ();
      if (ret_value.type == JERRY_API_DATA_TYPE_FLOAT32)
      {
        *num_value_p = ret_value.v_float32;
      }
      else if (ret_value.type == JERRY_API_DATA_TYPE_FLOAT64)
      {
#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32
        JERRY_UNREACHABLE ();
#elif CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
        *num_value_p = ret_value.v_float64;
#endif /* CONFIG_ECMA_NUMBER_TYPE ==  CONFIG_ECMA_NUMBER_FLOAT64 */
      }
      else
      {
        JERRY_ASSERT (ret_value.type == JERRY_API_DATA_TYPE_UINT32);

        *num_value_p = ecma_uint32_to_number (ret_value.v_uint32);
      }

      completion = ecma_make_normal_completion_value (ecma_make_number_value (num_value_p));
    }
    else if (ret_value.type == JERRY_API_DATA_TYPE_STRING)
    {
      completion = ecma_make_normal_completion_value (ecma_make_string_value (ret_value.v_string));

      ret_value.v_string = NULL;
    }
    else
    {
      JERRY_ASSERT (ret_value.type == JERRY_API_DATA_TYPE_OBJECT);

      completion = ecma_make_normal_completion_value (ecma_make_object_value (ret_value.v_object));

      ret_value.v_object = NULL;
    }

    return completion;
  }
} /* ecma_builtin_jerry_dispatch_routine */

bool
ecma_extension_register (jerry_extension_descriptor_t *extension_desc_p) /**< extension description */
{
  if (jerry_extensions_next_index >= ECMA_EXTENSION_MAX_NUMBER_OF_EXTENSIONS)
  {
    return false;
  }

  if (extension_desc_p->functions_count > ECMA_EXTENSION_MAX_FUNCTIONS_IN_EXTENSION)
  {
    return false;
  }

  /* Check names intersection */
  for (uint32_t i = 0; i < extension_desc_p->fields_count; i++)
  {
    for (uint32_t j = 0; j < extension_desc_p->fields_count; j++)
    {
      if (i != j
          && !strcmp (extension_desc_p->fields_p[i].field_name_p,
                      extension_desc_p->fields_p[j].field_name_p))
      {
        return false;
      }
    }
  }

  for (uint32_t i = 0; i < extension_desc_p->functions_count; i++)
  {
    if (extension_desc_p->functions_p[i].args_number >= ECMA_EXTENSION_MAX_ARGUMENTS_IN_FUNCTION)
    {
      return false;
    }

#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32
    /* Check if we can represent the arguments' values */
    for (uint32_t j = 0; j < extension_desc_p->functions_p[i].args_number; j++)
    {
      if (extension_desc_p->functions_p[i].args_p[j].type == JERRY_API_DATA_TYPE_FLOAT64)
      {
        return false;
      }
    }
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32 */

    for (uint32_t j = 0; j < extension_desc_p->functions_count; j++)
    {
      if (i != j
          && !strcmp (extension_desc_p->functions_p[i].function_name_p,
                      extension_desc_p->functions_p[j].function_name_p))
      {
        return false;
      }
    }
  }

  for (uint32_t i = 0; i < extension_desc_p->fields_count; i++)
  {
#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32
    /* Check if we can represent the field's value */

    if (extension_desc_p->fields_p[i].type == JERRY_API_DATA_TYPE_FLOAT64)
    {
      return false;
    }

    if (extension_desc_p->fields_p[i].type == JERRY_API_DATA_TYPE_UINT32
        && (ecma_number_to_uint32 (ecma_uint32_to_number (extension_desc_p->fields_p[i].v_uint32))
            != extension_desc_p->fields_p[i].v_uint32))
    {
      return false;
    }
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32 */

    for (uint32_t j = 0; j < extension_desc_p->functions_count; j++)
    {
      if (!strcmp (extension_desc_p->fields_p[i].field_name_p,
                   extension_desc_p->functions_p[j].function_name_p))
      {
        return false;
      }
    }
  }

  for (jerry_extension_descriptor_t *desc_iter_p = jerry_extensions_list_p;
       desc_iter_p != NULL;
       desc_iter_p = desc_iter_p->next_p)
  {
    if (desc_iter_p == extension_desc_p
        || !strcmp (desc_iter_p->name_p, extension_desc_p->name_p))
    {
      /* The extension already registered or an extension with the same name already registered */
      return false;
    }
  }

  extension_desc_p->next_p = jerry_extensions_list_p;
  jerry_extensions_list_p = extension_desc_p;
  extension_desc_p->index = jerry_extensions_next_index++;

  return true;
} /* ecma_extension_register */

/**
 * [[GetOwnProperty]] Implementation extension object's operation
 *
 * @return property descriptor
 */
ecma_property_t*
ecma_op_extension_object_get_own_property (ecma_object_t *obj_p, /**< the extension object */
                                           ecma_string_t *property_name_p) /**< property name */
{
  JERRY_ASSERT (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_EXTENSION);

  // 1.
  ecma_property_t *prop_p = ecma_op_general_object_get_own_property (obj_p, property_name_p);

  // 2.
  if (prop_p != NULL)
  {
    return prop_p;
  }

#if CONFIG_ECMA_CHAR_ENCODING != CONFIG_ECMA_CHAR_ASCII
  JERRY_UNIMPLEMENTED ("Only ASCII encoding support is implemented.");
#else /* CONFIG_ECMA_CHAR_ENCODING == CONFIG_ECMA_CHAR_ASCII */
  ssize_t req_buffer_size = ecma_string_to_zt_string (property_name_p, NULL, 0);
  JERRY_ASSERT (req_buffer_size < 0);

  MEM_DEFINE_LOCAL_ARRAY (property_name_zt_buf_p, -req_buffer_size, uint8_t);

  req_buffer_size = ecma_string_to_zt_string (property_name_p, property_name_zt_buf_p, -req_buffer_size);
  JERRY_ASSERT (req_buffer_size > 0);

  const char *name_p = (const char*) property_name_zt_buf_p;

  ecma_property_t *extension_id_prop_p = ecma_get_internal_property (obj_p,
                                                                     ECMA_INTERNAL_PROPERTY_EXTENSION_ID);
  uint32_t extension_object_index = extension_id_prop_p->u.internal_property.value;

  jerry_extension_descriptor_t *desc_p;
  for (desc_p = jerry_extensions_list_p;
       desc_p != NULL;
       desc_p = desc_p->next_p)
  {
    if (desc_p->index == extension_object_index)
    {
      break;
    }
  }
  JERRY_ASSERT (desc_p != NULL);

  uint32_t field_index;
  for (field_index = 0;
       field_index < desc_p->fields_count;
       field_index++)
  {
    if (!strcmp (name_p, desc_p->fields_p[field_index].field_name_p))
    {
      break;
    }
  }

  if (field_index < desc_p->fields_count)
  {
    const jerry_extension_field_t *field_p = &desc_p->fields_p[field_index];

    ecma_value_t value;
    prop_p = ecma_create_named_data_property (obj_p,
                                              property_name_p,
                                              ECMA_PROPERTY_NOT_WRITABLE,
                                              ECMA_PROPERTY_NOT_ENUMERABLE,
                                              ECMA_PROPERTY_NOT_CONFIGURABLE);

    if (field_p->type == JERRY_API_DATA_TYPE_UNDEFINED)
    {
      value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_UNDEFINED);
    }
    else if (field_p->type == JERRY_API_DATA_TYPE_NULL)
    {
      value = ecma_make_simple_value (ECMA_SIMPLE_VALUE_NULL);
    }
    else if (field_p->type == JERRY_API_DATA_TYPE_BOOLEAN)
    {
      value = ecma_make_simple_value (field_p->v_bool ? ECMA_SIMPLE_VALUE_TRUE : ECMA_SIMPLE_VALUE_FALSE);
    }
    else if (field_p->type == JERRY_API_DATA_TYPE_FLOAT32)
    {
      ecma_number_t *num_p = ecma_alloc_number ();
      *num_p = field_p->v_float32;
      value = ecma_make_number_value (num_p);
    }
#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
    else if (field_p->type == JERRY_API_DATA_TYPE_FLOAT64)
    {
      JERRY_UNREACHABLE ();
      ecma_number_t *num_p = ecma_alloc_number ();
      *num_p = field_p->v_float64;
      value = ecma_make_number_value (num_p);
    }
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64 */
    else if (field_p->type == JERRY_API_DATA_TYPE_UINT32)
    {
      ecma_number_t *num_p = ecma_alloc_number ();
      *num_p = ecma_uint32_to_number (field_p->v_uint32);
      JERRY_ASSERT (ecma_number_to_uint32 (*num_p) == field_p->v_uint32);
      value = ecma_make_number_value (num_p);
    }
    else
    {
      JERRY_ASSERT (field_p->type == JERRY_API_DATA_TYPE_STRING);
      const ecma_char_t *string_p = (const ecma_char_t*) field_p->v_string;
      ecma_string_t *str_p = ecma_new_ecma_string (string_p);
      value = ecma_make_string_value (str_p);
    }

    ecma_named_data_property_assign_value (obj_p, prop_p, value);
    ecma_free_value (value, true);
  }
  else
  {
    uint32_t function_index;
    for (function_index = 0;
         function_index < desc_p->functions_count;
         function_index++)
    {
      if (!strcmp (name_p, desc_p->functions_p[function_index].function_name_p))
      {
        break;
      }
    }

    if (function_index < desc_p->functions_count)
    {
      const jerry_extension_function_t *function_p = &desc_p->functions_p[function_index];

      /* Currently, combined identifier of extension object and extension function should fit in uint16_t. */
      JERRY_STATIC_ASSERT (ECMA_EXTENSION_MAX_NUMBER_OF_EXTENSIONS * ECMA_EXTENSION_MAX_FUNCTIONS_IN_EXTENSION
                           < (1ull << (sizeof (uint16_t) * JERRY_BITSINBYTE)));

      uint32_t routine_id = desc_p->index * ECMA_EXTENSION_MAX_FUNCTIONS_IN_EXTENSION + function_index;
      JERRY_ASSERT ((uint16_t) routine_id == routine_id);
      JERRY_STATIC_ASSERT ((ecma_number_t) ECMA_EXTENSION_MAX_ARGUMENTS_IN_FUNCTION
                           == ECMA_EXTENSION_MAX_ARGUMENTS_IN_FUNCTION);
      ecma_number_t args_number = ecma_uint32_to_number (function_p->args_number);
      JERRY_ASSERT (function_p->args_number == ecma_number_to_uint32 (args_number));
      ecma_object_t *func_obj_p = ecma_builtin_make_function_object_for_routine (ECMA_BUILTIN_ID_JERRY,
                                                                                 (uint16_t) routine_id,
                                                                                 args_number);

      prop_p = ecma_create_named_data_property (obj_p,
                                                property_name_p,
                                                ECMA_PROPERTY_NOT_WRITABLE,
                                                ECMA_PROPERTY_NOT_ENUMERABLE,
                                                ECMA_PROPERTY_NOT_CONFIGURABLE);

      ecma_named_data_property_assign_value (obj_p, prop_p, ecma_make_object_value (func_obj_p));

      ecma_deref_object (func_obj_p);
    }
  }

  MEM_FINALIZE_LOCAL_ARRAY (property_name_zt_buf_p);

  return prop_p;
#endif /* CONFIG_ECMA_CHAR_ENCODING == CONFIG_ECMA_CHAR_ASCII */
} /* ecma_op_extension_object_get_own_property */
