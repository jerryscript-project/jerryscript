/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged.
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
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-objects.h"
#include "ecma-regexp-object.h"
#include "ecma-try-catch-macro.h"

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaregexpobject ECMA RegExp object related routines
 * @{
 */

/**
 * RegExp object creation operation.
 *
 * See also: ECMA-262 v5, 15.10.4.1
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
ecma_op_create_regexp_object (const ecma_value_t *arguments_list_p, /**< list of arguments that
                                                                         are passed to RegExp constructor */
                              ecma_length_t arguments_list_len) /**< length of the arguments' list */
{
  ecma_completion_value_t ret_value;

  JERRY_ASSERT (arguments_list_len <= 2 && arguments_list_p != NULL);

#ifndef CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN
  ecma_object_t *regexp_prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_REGEXP_PROTOTYPE);
#else /* !CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */
  ecma_object_t *regexp_prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);
#endif /* CONFIG_ECMA_COMPACT_PROFILE_DISABLE_REGEXP_BUILTIN */

  ecma_object_t *obj_p = ecma_create_object (regexp_prototype_obj_p, true, ECMA_OBJECT_TYPE_GENERAL);
  ecma_deref_object (regexp_prototype_obj_p);

  ECMA_TRY_CATCH (regexp_str_value,
                  ecma_op_to_string (arguments_list_p[0]),
                  ret_value);

  /* Set source property. ECMA-262 v5, 15.10.7.1 */
  ecma_string_t *magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_SOURCE);
  ecma_property_t *source_prop_p = ecma_create_named_data_property (obj_p,
                                                                    magic_string_p,
                                                                    false, false, false);

  ecma_string_t *source_string_p = ecma_get_string_from_value (regexp_str_value);
  ecma_set_named_data_property_value (source_prop_p,
                                      ecma_make_string_value (ecma_copy_or_ref_ecma_string (source_string_p)));

  /* Set global property. ECMA-262 v5, 15.10.7.2*/
  magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_GLOBAL);
  ecma_property_t *global_prop_p = ecma_create_named_data_property (obj_p,
                                                                    magic_string_p,
                                                                    false, false, false);
  ecma_set_named_data_property_value (global_prop_p, ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE));

  /* Set ignoreCase property. ECMA-262 v5, 15.10.7.3*/
  magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_IGNORECASE);
  ecma_property_t *ignorecase_prop_p = ecma_create_named_data_property (obj_p,
                                                                        magic_string_p,
                                                                        false, false, false);
  ecma_set_named_data_property_value (ignorecase_prop_p, ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE));

  /* Set multiline property. ECMA-262 v5, 15.10.7.4*/
  magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_MULTILINE);
  ecma_property_t *multiline_prop_p = ecma_create_named_data_property (obj_p,
                                                                       magic_string_p,
                                                                       false, false, false);
  ecma_set_named_data_property_value (multiline_prop_p, ecma_make_simple_value (ECMA_SIMPLE_VALUE_FALSE));

  /* Set lastIndex property. ECMA-262 v5, 15.10.7.5*/
  magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_LASTINDEX);
  ecma_property_t *lastindex_prop_p = ecma_create_named_data_property (obj_p,
                                                                       magic_string_p,
                                                                       true, false, false);
  ecma_number_t *lastindex_num_p = ecma_alloc_number ();
  *lastindex_num_p = ECMA_NUMBER_ZERO;
  ecma_set_named_data_property_value (lastindex_prop_p, ecma_make_number_value(lastindex_num_p));

  ecma_deref_ecma_string (magic_string_p);
  ret_value = ecma_make_normal_completion_value (ecma_make_object_value (obj_p));

  ECMA_FINALIZE (regexp_str_value);

  return ret_value;
}

/**
 * @}
 * @}
 */
