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
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "jrt-bit-fields.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 */

/**
 * Initialize ECMA built-in objects
 */
void
ecma_init_builtins (void)
{
  ecma_builtin_init_global_object ();
} /* ecma_init_builtins */

/**
 * Finalize ECMA built-in objects
 */
void
ecma_finalize_builtins (void)
{
  ecma_builtin_finalize_global_object ();
} /* ecma_finalize_builtins */

/**
 * If the property's name is one of built-in properties of the object
 * that is not instantiated yet, instantiate the property and
 * return pointer to the instantiated property.
 *
 * @return pointer property, if one was instantiated,
 *         NULL - otherwise.
 */
ecma_property_t*
ecma_object_try_to_get_non_instantiated_property (ecma_object_t *object_p, /**< object */
                                                  ecma_string_t *string_p) /**< property's name */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (object_p, string_p);
} /* ecma_object_try_to_get_non_instantiated_property */

/**
 * Construct a Function object for specified built-in routine
 *
 * See also: ECMA-262 v5, 15
 *
 * @return pointer to constructed Function object
 */
ecma_object_t*
ecma_builtin_make_function_object_for_routine (ecma_builtin_id_t builtin_id, /**< identifier of built-in object
                                                                                  that initially contains property
                                                                                  with the routine */
                                               uint32_t routine_id) /**< identifier of the built-in object's
                                                                         routine property
                                                                         (one of ecma_builtin_{*}_property_id_t) */
{
  FIXME(Setup prototype of Function object to built-in Function prototype object (15.3.3.1));

  ecma_object_t *func_obj_p = ecma_create_object (NULL, true, ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION);

  uint64_t packed_value = jrt_set_bit_field_value (0,
                                                   builtin_id,
                                                   ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_OBJECT_ID_POS,
                                                   ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_OBJECT_ID_WIDTH);
  packed_value = jrt_set_bit_field_value (packed_value,
                                          routine_id,
                                          ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_ROUTINE_ID_POS,
                                          ECMA_BUILTIN_ROUTINE_ID_BUILT_IN_ROUTINE_ID_WIDTH);
  ecma_property_t *routine_id_prop_p = ecma_create_internal_property (func_obj_p,
                                                                      ECMA_INTERNAL_PROPERTY_BUILT_IN_ROUTINE_ID);

  JERRY_ASSERT ((uint32_t) packed_value == packed_value);
  routine_id_prop_p->u.internal_property.value = (uint32_t) packed_value;

  ecma_number_t* len_p = ecma_alloc_number ();
  *len_p = ecma_uint32_to_number (ecma_builtin_get_routine_parameters_number (builtin_id, routine_id));

  ecma_property_descriptor_t length_prop_desc = ecma_make_empty_property_descriptor ();
  length_prop_desc.is_value_defined = true;
  length_prop_desc.value = ecma_make_number_value (len_p);
  length_prop_desc.is_writable_defined = false;
  length_prop_desc.writable = ECMA_PROPERTY_NOT_WRITABLE;
  length_prop_desc.is_enumerable_defined = false;
  length_prop_desc.enumerable = ECMA_PROPERTY_NOT_ENUMERABLE;
  length_prop_desc.is_configurable_defined = false;
  length_prop_desc.configurable = ECMA_PROPERTY_NOT_CONFIGURABLE;

  ecma_string_t* magic_string_length_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LENGTH);
  ecma_completion_value_t completion = ecma_op_object_define_own_property (func_obj_p,
                                                                           magic_string_length_p,
                                                                           length_prop_desc,
                                                                           false);
  ecma_deref_ecma_string (magic_string_length_p);

  JERRY_ASSERT (ecma_is_completion_value_normal_true (completion)
                || ecma_is_completion_value_normal_false (completion));

  ecma_dealloc_number (len_p);
  len_p = NULL;

  return func_obj_p;
} /* ecma_builtin_make_function_object_for_routine */

/**
 * Get parameters number of the built-in routine
 *
 * @return number of parameters of the routine according to ECMA-262 v5 specification
 */
ecma_length_t
ecma_builtin_get_routine_parameters_number (ecma_builtin_id_t builtin_id, /**< identifier of built-in object
                                                                               that initially contains property
                                                                               with the routine */
                                            uint32_t routine_id) /**< identifier of the built-in object's
                                                                      routine property
                                                                      (one of ecma_builtin_{*}_property_id_t) */
{
  switch (builtin_id)
  {
    case ECMA_BUILTIN_ID_GLOBAL:
    {
      JERRY_ASSERT (routine_id < ECMA_BUILTIN_GLOBAL_DETAIL_ID__COUNT);

      return ecma_builtin_global_get_routine_parameters_number ((ecma_builtin_global_detail_id_t) routine_id);
    }
    case ECMA_BUILTIN_ID_OBJECT:
    case ECMA_BUILTIN_ID_OBJECT_PROTOTYPE:
    case ECMA_BUILTIN_ID_FUNCTION:
    case ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE:
    case ECMA_BUILTIN_ID_ARRAY:
    case ECMA_BUILTIN_ID_ARRAY_PROTOTYPE:
    case ECMA_BUILTIN_ID_STRING:
    case ECMA_BUILTIN_ID_STRING_PROTOTYPE:
    case ECMA_BUILTIN_ID_BOOLEAN:
    case ECMA_BUILTIN_ID_BOOLEAN_PROTOTYPE:
    case ECMA_BUILTIN_ID_NUMBER:
    case ECMA_BUILTIN_ID_NUMBER_PROTOTYPE:
    case ECMA_BUILTIN_ID_DATE:
    case ECMA_BUILTIN_ID_REGEXP:
    case ECMA_BUILTIN_ID_REGEXP_PROTOTYPE:
    case ECMA_BUILTIN_ID_ERROR:
    case ECMA_BUILTIN_ID_ERROR_PROTOTYPE:
    case ECMA_BUILTIN_ID_EVAL_ERROR:
    case ECMA_BUILTIN_ID_RANGE_ERROR:
    case ECMA_BUILTIN_ID_REFERENCE_ERROR:
    case ECMA_BUILTIN_ID_SYNTAX_ERROR:
    case ECMA_BUILTIN_ID_TYPE_ERROR:
    case ECMA_BUILTIN_ID_SYNTAX_URI_ERROR:
    case ECMA_BUILTIN_ID_MATH:
    case ECMA_BUILTIN_ID_JSON:
    {
      JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (routine_id);
    }
  }

  JERRY_UNREACHABLE ();
} /* ecma_builtin_get_routine_parameters_number */

/**
 * Dispatcher of built-in routines
 *
 * @return completion-value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
ecma_builtin_dispatch_routine (ecma_builtin_id_t builtin_object_id, /**< built-in object' identifier */
                               uint32_t builtin_routine_id, /**< identifier of the built-in object's routine
                                                                 property (one of ecma_builtin_{*}_property_id_t) */
                               ecma_value_t arguments_list [], /**< list of arguments passed to routine */
                               ecma_length_t arguments_number) /**< length of arguments' list */
{
  switch (builtin_object_id)
  {
    case ECMA_BUILTIN_ID_GLOBAL:
    {
      JERRY_ASSERT (builtin_routine_id < ECMA_BUILTIN_GLOBAL_DETAIL_ID__COUNT);

      return ecma_builtin_global_dispatch_routine ((ecma_builtin_global_detail_id_t) builtin_routine_id,
                                                   arguments_list,
                                                   arguments_number);
    }
    case ECMA_BUILTIN_ID_OBJECT:
    case ECMA_BUILTIN_ID_OBJECT_PROTOTYPE:
    case ECMA_BUILTIN_ID_FUNCTION:
    case ECMA_BUILTIN_ID_FUNCTION_PROTOTYPE:
    case ECMA_BUILTIN_ID_ARRAY:
    case ECMA_BUILTIN_ID_ARRAY_PROTOTYPE:
    case ECMA_BUILTIN_ID_STRING:
    case ECMA_BUILTIN_ID_STRING_PROTOTYPE:
    case ECMA_BUILTIN_ID_BOOLEAN:
    case ECMA_BUILTIN_ID_BOOLEAN_PROTOTYPE:
    case ECMA_BUILTIN_ID_NUMBER:
    case ECMA_BUILTIN_ID_NUMBER_PROTOTYPE:
    case ECMA_BUILTIN_ID_DATE:
    case ECMA_BUILTIN_ID_REGEXP:
    case ECMA_BUILTIN_ID_REGEXP_PROTOTYPE:
    case ECMA_BUILTIN_ID_ERROR:
    case ECMA_BUILTIN_ID_ERROR_PROTOTYPE:
    case ECMA_BUILTIN_ID_EVAL_ERROR:
    case ECMA_BUILTIN_ID_RANGE_ERROR:
    case ECMA_BUILTIN_ID_REFERENCE_ERROR:
    case ECMA_BUILTIN_ID_SYNTAX_ERROR:
    case ECMA_BUILTIN_ID_TYPE_ERROR:
    case ECMA_BUILTIN_ID_SYNTAX_URI_ERROR:
    case ECMA_BUILTIN_ID_MATH:
    case ECMA_BUILTIN_ID_JSON:
    {
      JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (builtin_routine_id,
                                           arguments_list,
                                           arguments_number);
    }
  }

  JERRY_UNREACHABLE ();
} /* ecma_builtin_dispatch_routine */

/**
 * @}
 * @}
 */
