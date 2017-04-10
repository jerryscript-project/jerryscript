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
#include "ecma-globals.h"
#include "ecma-helpers.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

/**
 * Create internal property with specified identifier and store native handle/pointer of the object.
 *
 * Note:
 *      property identifier should be one of the following:
 *        - LIT_INTERNAL_MAGIC_STRING_NATIVE_HANDLE
 *        - LIT_INTERNAL_MAGIC_STRING_NATIVE_POINTER
 *
 * @return true - if property was just created with specified value,
 *         false - otherwise, if property existed before the call, it's value was updated
 */
static bool
ecma_create_native_property (ecma_object_t *obj_p, /**< object to create property in */
                             lit_magic_string_id_t id, /**< identifier of internal
                                                        *   property to create */
                             void *package_p) /**< value to store in the property */
{
  JERRY_ASSERT (id == LIT_INTERNAL_MAGIC_STRING_NATIVE_HANDLE
                || id == LIT_INTERNAL_MAGIC_STRING_NATIVE_POINTER);

  ecma_string_t name;
  ecma_init_ecma_magic_string (&name, id);

  ecma_property_t *property_p = ecma_find_named_property (obj_p, &name);
  bool is_new = (property_p == NULL);
  ecma_property_value_t *value_p;

  if (is_new)
  {
    value_p = ecma_create_named_data_property (obj_p, &name, ECMA_PROPERTY_FLAG_WRITABLE, NULL);
  }
  else
  {
    value_p = ECMA_PROPERTY_VALUE_PTR (property_p);
    ecma_free_native_package_property (property_p, id);
  }

  JERRY_ASSERT (ECMA_STRING_IS_REF_EQUALS_TO_ONE (&name));

  JERRY_STATIC_ASSERT (sizeof (uint32_t) <= sizeof (value_p->value),
                       size_of_internal_property_value_must_be_greater_than_or_equal_to_4_bytes);

  ECMA_SET_INTERNAL_VALUE_POINTER (value_p->value, package_p);

  return is_new;
} /* ecma_create_native_property */

/**
 * Create a native handle property to store the native handle and its free callback.
 *
 * @return true - if property was just created with specified value,
 *         false - otherwise, if property existed before the call, it's value was updated
 */
bool
ecma_create_native_handle_property (ecma_object_t *obj_p, /**< object to create property in */
                                    ecma_external_pointer_t handle_p, /**< native handle */
                                    ecma_external_pointer_t free_cb) /**< native handle's free callback*/
{
  ecma_native_handle_package_t *package_p;
  package_p = jmem_heap_alloc_block (sizeof (ecma_native_handle_package_t));
  package_p->handle_p = handle_p;
  package_p->free_cb = free_cb;

  return ecma_create_native_property (obj_p,
                                      LIT_INTERNAL_MAGIC_STRING_NATIVE_HANDLE,
                                      package_p);
} /* ecma_create_native_handle_property */

/**
 * Create a native pointer property to store the native pointer and its type info.
 *
 * @return true - if property was just created with specified value,
 *         false - otherwise, if property existed before the call, it's value was updated
 */
bool
ecma_create_native_pointer_property (ecma_object_t *obj_p, /**< object to create property in */
                                     ecma_external_pointer_t native_p, /**< native pointer */
                                     ecma_external_pointer_t info_p) /**< native pointer's type info */
{
  ecma_native_pointer_package_t *package_p;
  package_p = jmem_heap_alloc_block (sizeof (ecma_native_pointer_package_t));
  package_p->native_p = native_p;
  package_p->info_p = info_p;

  return ecma_create_native_property (obj_p,
                                      LIT_INTERNAL_MAGIC_STRING_NATIVE_POINTER,
                                      package_p);
} /* ecma_create_native_pointer_property */

/**
 * Get value of native package stored in the object's property with specified identifier
 *
 * Note:
 *      property identifier should be one of the following:
 *        - LIT_INTERNAL_MAGIC_STRING_NATIVE_HANDLE
 *        - LIT_INTERNAL_MAGIC_STRING_NATIVE_POINTER
 *
 * @return true - if property exists and it's value is returned through out_pointer_p,
 *         false - otherwise (value returned through out_pointer_p is NULL)
 */
bool
ecma_get_native_package_value (ecma_object_t *obj_p, /**< object to get property value from */
                               lit_magic_string_id_t id, /**< identifier of internal property
                                                          *   to get value from */
                               void **out_pointer_p) /**< [out] value of the native package */
{
  JERRY_ASSERT (id == LIT_INTERNAL_MAGIC_STRING_NATIVE_HANDLE
                || id == LIT_INTERNAL_MAGIC_STRING_NATIVE_POINTER);

  ecma_string_t name;
  ecma_init_ecma_magic_string (&name, id);

  ecma_property_t *property_p = ecma_find_named_property (obj_p, &name);

  JERRY_ASSERT (ECMA_STRING_IS_REF_EQUALS_TO_ONE (&name));

  if (property_p == NULL)
  {
    *out_pointer_p = NULL;

    return false;
  }

  ecma_property_value_t *value_p = ECMA_PROPERTY_VALUE_PTR (property_p);

  *out_pointer_p = ECMA_GET_INTERNAL_VALUE_POINTER (void, value_p->value);

  return true;
} /* ecma_get_native_package_value */

/**
 * Free the allocated native package struct.
 *
 * Note:
 *      property identifier should be one of the following:
 *        - LIT_INTERNAL_MAGIC_STRING_NATIVE_HANDLE
 *        - LIT_INTERNAL_MAGIC_STRING_NATIVE_POINTER
 */
void
ecma_free_native_package_property (ecma_property_t *prop_p, /**< native property */
                                          lit_magic_string_id_t id) /**< identifier of internal */
{
  JERRY_ASSERT (id == LIT_INTERNAL_MAGIC_STRING_NATIVE_HANDLE
              || id == LIT_INTERNAL_MAGIC_STRING_NATIVE_POINTER);

  ecma_property_value_t *value_p = ECMA_PROPERTY_VALUE_PTR (prop_p);
  void *package_p;

  package_p = ECMA_GET_INTERNAL_VALUE_POINTER (void, value_p->value);

  if (id == LIT_INTERNAL_MAGIC_STRING_NATIVE_HANDLE)
  {
    jmem_heap_free_block (package_p, sizeof (ecma_native_handle_package_t));
    return;
  }

  jmem_heap_free_block (package_p, sizeof (ecma_native_pointer_package_t));
} /* ecma_free_native_package_property */

/**
 * @}
 * @}
 */
