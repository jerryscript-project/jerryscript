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
 * Create internal property with specified identifier and store external pointer in the property.
 *
 * Note:
 *      property identifier should be one of the following:
 *        - ECMA_INTERNAL_PROPERTY_NATIVE_HANDLE;
 *        - ECMA_INTERNAL_PROPERTY_FREE_CALLBACK.
 *
 * @return true - if property was just created with specified value,
 *         false - otherwise, if property existed before the call, it's value was updated.
 */
bool
ecma_create_external_pointer_property (ecma_object_t *obj_p, /**< object to create property in */
                                       ecma_internal_property_id_t id, /**< identifier of internal
                                                                        *   property to create */
                                       ecma_external_pointer_t ptr_value) /**< value to store in the property */
{
  JERRY_ASSERT (id == ECMA_INTERNAL_PROPERTY_NATIVE_HANDLE
                || id == ECMA_INTERNAL_PROPERTY_FREE_CALLBACK);

  ecma_value_t *prop_p = ecma_find_internal_property (obj_p, id);
  bool is_new = (prop_p == NULL);

  if (is_new)
  {
    prop_p = ecma_create_internal_property (obj_p, id);
  }

  JERRY_STATIC_ASSERT (sizeof (uint32_t) <= sizeof (ECMA_PROPERTY_VALUE_PTR (prop_p)->value),
                       size_of_internal_property_value_must_be_greater_than_or_equal_to_4_bytes);

#ifdef ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY

  *prop_p = (ecma_value_t) ptr_value;

#else /* !ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */

  ecma_external_pointer_t *handler_p;

  if (is_new)
  {
    handler_p = ecma_alloc_external_pointer ();

    ECMA_SET_NON_NULL_POINTER (*prop_p, handler_p);
  }
  else
  {
    handler_p = ECMA_GET_NON_NULL_POINTER (ecma_external_pointer_t, *prop_p);
  }

  *handler_p = ptr_value;

#endif /* ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */

  return is_new;
} /* ecma_create_external_pointer_property */

/**
 * Get value of external pointer stored in the object's property with specified identifier
 *
 * Note:
 *      property identifier should be one of the following:
 *        - ECMA_INTERNAL_PROPERTY_NATIVE_HANDLE;
 *        - ECMA_INTERNAL_PROPERTY_FREE_CALLBACK.
 *
 * @return true - if property exists and it's value is returned through out_pointer_p,
 *         false - otherwise (value returned through out_pointer_p is NULL).
 */
bool
ecma_get_external_pointer_value (ecma_object_t *obj_p, /**< object to get property value from */
                                 ecma_internal_property_id_t id, /**< identifier of internal property
                                                                  *   to get value from */
                                 ecma_external_pointer_t *out_pointer_p) /**< [out] value of the external pointer */
{
  JERRY_ASSERT (id == ECMA_INTERNAL_PROPERTY_NATIVE_HANDLE
                || id == ECMA_INTERNAL_PROPERTY_FREE_CALLBACK);

  ecma_value_t *prop_p = ecma_find_internal_property (obj_p, id);

  if (prop_p == NULL)
  {
    *out_pointer_p = (ecma_external_pointer_t) NULL;

    return false;
  }

#ifdef ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY

  *out_pointer_p = *prop_p;

#else /* !ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */

  *out_pointer_p = *ECMA_GET_NON_NULL_POINTER (ecma_external_pointer_t, *prop_p);

#endif /* ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */

  return true;
} /* ecma_get_external_pointer_value */

/**
 * Free memory associated with external pointer stored in the property
 *
 * Note:
 *      property identifier should be one of the following:
 *        - ECMA_INTERNAL_PROPERTY_NATIVE_HANDLE;
 *        - ECMA_INTERNAL_PROPERTY_FREE_CALLBACK.
 */
void
ecma_free_external_pointer_in_property (ecma_property_t *prop_p) /**< internal property */
{
  JERRY_ASSERT (ECMA_PROPERTY_GET_INTERNAL_PROPERTY_TYPE (prop_p) == ECMA_INTERNAL_PROPERTY_NATIVE_HANDLE
                || ECMA_PROPERTY_GET_INTERNAL_PROPERTY_TYPE (prop_p) == ECMA_INTERNAL_PROPERTY_FREE_CALLBACK);

#ifdef ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY

  /* no additional memory was allocated for the pointer storage */

#else /* !ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */

  ecma_external_pointer_t *handler_p = ECMA_GET_NON_NULL_POINTER (ecma_external_pointer_t,
                                                                  ECMA_PROPERTY_VALUE_PTR (prop_p)->value);

  ecma_dealloc_external_pointer (handler_p);

#endif /* ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY */

} /* ecma_free_external_pointer_in_property */

/**
 * @}
 * @}
 */
