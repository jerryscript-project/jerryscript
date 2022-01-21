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

#include "ecma-class-object.h"

#include "ecma-arguments-object.h"
#include "ecma-globals.h"
#include "ecma-objects-general.h"
#include "ecma-objects.h"
#include "ecma-ordinary-object.h"
#include "ecma-string-object.h"
#include "ecma-typedarray-object.h"

#include "jcontext.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaclassobject ECMA Class object related routines
 * @{
 */

/**
 * Virtual internal method function table for class objects
 */
static const ecma_internal_method_table_t CLASS_OBJ_VTABLE[] = {
/* These objects require custom property resolving. */
#if JERRY_BUILTIN_TYPEDARRAY
  ECMA_TYPEDARRAY_OBJ_VTABLE,
#endif /* JERRY_BUILTIN_TYPEDARRAY */
#if JERRY_MODULE_SYSTEM
  ECMA_MODULE_NAMESPACE_OBJ_VTABLE,
#endif /* JERRY_MODULE_SYSTEM */
  ECMA_STRING_OBJ_VTABLE,
  ECMA_ARGUMENTS_OBJ_VTABLE,
};

/**
 * Resolve the virtual function table for the given class object
 */
#define ECMA_CLASS_OBJECT_RESOLVE_VTABLE(obj_p)                                      \
  ((((ecma_extended_object_t *) (obj_p))->u.cls.type <= ECMA_OBJECT_CLASS_ARGUMENTS) \
     ? &CLASS_OBJ_VTABLE[((ecma_extended_object_t *) obj_p)->u.cls.type]             \
     : &VTABLE_CONTAINER[ECMA_OBJECT_TYPE_GENERAL])

/**
 * Helper function for calling the given class object's [[GetOwnProperty]] internal method
 *
 * @return ecma property descriptor
 */
ecma_property_descriptor_t
ecma_class_object_get_own_property (ecma_object_t *obj_p, /**< object */
                                    ecma_string_t *property_name_p) /**< property name */
{
  uint8_t type = ((ecma_extended_object_t *) obj_p)->u.cls.type;

  if (type <= ECMA_OBJECT_CLASS_ARGUMENTS)
  {
    return (&CLASS_OBJ_VTABLE[type])->get_own_property (obj_p, property_name_p);
  }

  return ecma_ordinary_object_get_own_property (obj_p, property_name_p);
} /* ecma_class_object_get_own_property */

/**
 * Helper function for calling the given class object's [[DefineOwnProperty]] internal method
 *
 * @return ecma value
 */
ecma_value_t
ecma_class_object_define_own_property (ecma_object_t *obj_p, /**< object */
                                       ecma_string_t *property_name_p, /**< property name */
                                       const ecma_property_descriptor_t *property_desc_p) /**< property
                                                                                           *   descriptor */
{
  uint8_t type = ((ecma_extended_object_t *) obj_p)->u.cls.type;

  if (type <= ECMA_OBJECT_CLASS_ARGUMENTS)
  {
    return (&CLASS_OBJ_VTABLE[type])->define_own_property (obj_p, property_name_p, property_desc_p);
  }

  return ecma_ordinary_object_define_own_property (obj_p, property_name_p, property_desc_p);
} /* ecma_class_object_define_own_property */

/**
 * Helper function for calling the given class object's [[Get]] internal method
 *
 * @return ecma value
 */
ecma_value_t
ecma_class_object_get (ecma_object_t *obj_p, /**< object */
                       ecma_string_t *property_name_p, /**< property name */
                       ecma_value_t receiver) /**< receiver */
{
  uint8_t type = ((ecma_extended_object_t *) obj_p)->u.cls.type;

  if (type <= ECMA_OBJECT_CLASS_ARGUMENTS)
  {
    return (&CLASS_OBJ_VTABLE[type])->get (obj_p, property_name_p, receiver);
  }

  return ecma_ordinary_object_get (obj_p, property_name_p, receiver);
} /* ecma_class_object_get */

/**
 * Helper function for calling the given class object's [[Set]] internal method
 *
 * @return ecma value
 */
ecma_value_t
ecma_class_object_set (ecma_object_t *obj_p, /**< object */
                       ecma_string_t *property_name_p, /**< property name */
                       ecma_value_t value, /**< ecma value */
                       ecma_value_t receiver, /**< receiver */
                       bool is_throw) /**< flag that controls failure handling */
{
  uint8_t type = ((ecma_extended_object_t *) obj_p)->u.cls.type;

  if (type <= ECMA_OBJECT_CLASS_ARGUMENTS)
  {
    return (&CLASS_OBJ_VTABLE[type])->set (obj_p, property_name_p, value, receiver, is_throw);
  }

  return ecma_ordinary_object_set (obj_p, property_name_p, value, receiver, is_throw);
} /* ecma_class_object_set */

/**
 * Helper function for calling the given class object's [[Delete]] internal method
 *
 * @return ecma value
 */
ecma_value_t
ecma_class_object_delete_property (ecma_object_t *obj_p, /**< object */
                                   ecma_string_t *property_name_p, /**< property name */
                                   bool is_strict) /**< flag that controls failure handling */
{
  uint8_t type = ((ecma_extended_object_t *) obj_p)->u.cls.type;

  if (type <= ECMA_OBJECT_CLASS_ARGUMENTS)
  {
    return (&CLASS_OBJ_VTABLE[type])->delete (obj_p, property_name_p, is_strict);
  }

  return ecma_ordinary_object_delete (obj_p, property_name_p, is_strict);
} /* ecma_class_object_delete_property */

/**
 * Helper function for calling the given class object's 'list lazy property keys' internal method
 */
void
ecma_class_object_list_lazy_property_keys (ecma_object_t *obj_p, /**< a built-in object */
                                           ecma_collection_t *prop_names_p, /**< prop name collection */
                                           ecma_property_counter_t *prop_counter_p, /**< property counters */
                                           jerry_property_filter_t filter) /**< name filters */
{
  uint8_t type = ((ecma_extended_object_t *) obj_p)->u.cls.type;

  if (type <= ECMA_OBJECT_CLASS_ARGUMENTS)
  {
    (&CLASS_OBJ_VTABLE[type])->list_lazy_property_keys (obj_p, prop_names_p, prop_counter_p, filter);
  }
  else
  {
    ecma_ordinary_object_list_lazy_property_keys (obj_p, prop_names_p, prop_counter_p, filter);
  }
} /* ecma_class_object_list_lazy_property_keys */

/**
 * Helper function for calling the given class object's [[Delete]] internal method
 */
void
ecma_class_object_delete_lazy_property (ecma_object_t *obj_p, /**< object */
                                        ecma_string_t *property_name_p) /**< property name */
{
  uint8_t type = ((ecma_extended_object_t *) obj_p)->u.cls.type;

  if (type <= ECMA_OBJECT_CLASS_ARGUMENTS)
  {
    CLASS_OBJ_VTABLE[type].delete_lazy_property (obj_p, property_name_p);
  }
  else
  {
    ecma_ordinary_object_delete_lazy_property (obj_p, property_name_p);
  }
} /* ecma_class_object_delete_lazy_property */

/**
 * @}
 * @}
 */
