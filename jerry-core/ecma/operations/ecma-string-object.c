/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.
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
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-objects-general.h"
#include "ecma-string-object.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmastringobject ECMA String object related routines
 * @{
 */

/**
 * String object creation operation.
 *
 * See also: ECMA-262 v5, 15.5.2.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_op_create_string_object (const ecma_value_t *arguments_list_p, /**< list of arguments that
                                                                         are passed to String constructor */
                              ecma_length_t arguments_list_len) /**< length of the arguments' list */
{
  JERRY_ASSERT (arguments_list_len == 0
                || arguments_list_p != NULL);

  ecma_string_t *prim_prop_str_value_p;

  if (arguments_list_len == 0)
  {
    prim_prop_str_value_p = ecma_new_ecma_string_from_magic_string_id (LIT_MAGIC_STRING__EMPTY);
  }
  else
  {
    ecma_value_t to_str_arg_value = ecma_op_to_string (arguments_list_p[0]);

    if (ECMA_IS_VALUE_ERROR (to_str_arg_value))
    {
      return to_str_arg_value;
    }
    else
    {
      JERRY_ASSERT (!ECMA_IS_VALUE_ERROR (to_str_arg_value));
      JERRY_ASSERT (ecma_is_value_string (to_str_arg_value));

      prim_prop_str_value_p = ecma_get_string_from_value (to_str_arg_value);
    }
  }

#ifndef CONFIG_DISABLE_STRING_BUILTIN
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_STRING_PROTOTYPE);
#else /* CONFIG_DISABLE_STRING_BUILTIN */
  ecma_object_t *prototype_obj_p = ecma_builtin_get (ECMA_BUILTIN_ID_OBJECT_PROTOTYPE);
#endif /* !CONFIG_DISABLE_STRING_BUILTIN */

  ecma_object_t *object_p = ecma_create_object (prototype_obj_p,
                                                sizeof (ecma_extended_object_t),
                                                ECMA_OBJECT_TYPE_CLASS);

  ecma_deref_object (prototype_obj_p);

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) object_p;
  ext_object_p->u.class_prop.class_id = LIT_MAGIC_STRING_STRING_UL;
  ext_object_p->u.class_prop.value = ecma_make_string_value (prim_prop_str_value_p);

  return ecma_make_object_value (object_p);
} /* ecma_op_create_string_object */

/**
 * List names of a String object's lazy instantiated properties
 *
 * @return string values collection
 */
void
ecma_op_string_list_lazy_property_names (ecma_object_t *obj_p, /**< a String object */
                                         bool separate_enumerable, /**< true -  list enumerable properties
                                                                    *           into main collection,
                                                                    *           and non-enumerable to collection of
                                                                    *           'skipped non-enumerable' properties,
                                                                    *   false - list all properties into main
                                                                    *           collection.
                                                                    */
                                         ecma_collection_header_t *main_collection_p, /**< 'main'
                                                                                       *   collection */
                                         ecma_collection_header_t *non_enum_collection_p) /**< skipped
                                                                                           *   'non-enumerable'
                                                                                           *   collection
                                                                                                        */
{
  JERRY_ASSERT (ecma_get_object_type (obj_p) == ECMA_OBJECT_TYPE_CLASS);

  ecma_collection_header_t *for_enumerable_p = main_collection_p;

  ecma_collection_header_t *for_non_enumerable_p = separate_enumerable ? non_enum_collection_p : main_collection_p;

  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) obj_p;
  JERRY_ASSERT (ext_object_p->u.class_prop.class_id == LIT_MAGIC_STRING_STRING_UL);

  ecma_string_t *prim_value_str_p = ecma_get_string_from_value (ext_object_p->u.class_prop.value);

  ecma_length_t length = ecma_string_get_length (prim_value_str_p);

  for (ecma_length_t i = 0; i < length; i++)
  {
    ecma_string_t *name_p = ecma_new_ecma_string_from_uint32 (i);

    /* the properties are enumerable (ECMA-262 v5, 15.5.5.2.9) */
    ecma_append_to_values_collection (for_enumerable_p, ecma_make_string_value (name_p), true);

    ecma_deref_ecma_string (name_p);
  }

  ecma_string_t *length_str_p = ecma_new_ecma_length_string ();
  ecma_append_to_values_collection (for_non_enumerable_p, ecma_make_string_value (length_str_p), true);
  ecma_deref_ecma_string (length_str_p);
} /* ecma_op_string_list_lazy_property_names */

/**
 * @}
 * @}
 */
