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
 * Create a native pointer property to store the native pointer and its type info.
 *
 * @return true - if property was just created with specified value,
 *         false - otherwise, if property existed before the call, it's value was updated
 */
bool
ecma_create_native_pointer_property (ecma_object_t *obj_p, /**< object to create property in */
                                     void *native_p, /**< native pointer */
                                     void *info_p) /**< native pointer's type info */
{
  ecma_string_t *name_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_NATIVE_POINTER);
  ecma_property_t *property_p = ecma_find_named_property (obj_p, name_p);

  bool is_new = (property_p == NULL);

  ecma_native_pointer_t *native_pointer_p;

  if (is_new)
  {
    ecma_property_value_t *value_p;
    value_p = ecma_create_named_data_property (obj_p, name_p, ECMA_PROPERTY_FLAG_WRITABLE, &property_p);

    ECMA_CONVERT_DATA_PROPERTY_TO_INTERNAL_PROPERTY (property_p);

    native_pointer_p = jmem_heap_alloc_block (sizeof (ecma_native_pointer_t));

    ECMA_SET_INTERNAL_VALUE_POINTER (value_p->value, native_pointer_p);
  }
  else
  {
    ecma_property_value_t *value_p = ECMA_PROPERTY_VALUE_PTR (property_p);

    native_pointer_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_native_pointer_t, value_p->value);
  }

  native_pointer_p->data_p = native_p;
  native_pointer_p->info_p = info_p;

  return is_new;
} /* ecma_create_native_pointer_property */

/**
 * Get value of native package stored in the object's property with specified identifier
 *
 * Note:
 *      property identifier should be one of the following:
 *        - LIT_INTERNAL_MAGIC_STRING_NATIVE_POINTER
 *
 * @return native pointer data if property exists
 *         NULL otherwise
 */
ecma_native_pointer_t *
ecma_get_native_pointer_value (ecma_object_t *obj_p) /**< object to get property value from */
{
  ecma_string_t *name_p = ecma_get_magic_string (LIT_INTERNAL_MAGIC_STRING_NATIVE_POINTER);
  ecma_property_t *property_p = ecma_find_named_property (obj_p, name_p);

  if (property_p == NULL)
  {
    return NULL;
  }

  ecma_property_value_t *value_p = ECMA_PROPERTY_VALUE_PTR (property_p);

  return ECMA_GET_INTERNAL_VALUE_POINTER (ecma_native_pointer_t, value_p->value);
} /* ecma_get_native_pointer_value */

/**
 * @}
 * @}
 */
