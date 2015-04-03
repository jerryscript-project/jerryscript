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

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmahelpers Helpers for operations with ECMA data types
 * @{
 */

#include "ecma-alloc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"

/**
 * Create internal property with specified identifier and store external pointer in the property.
 *
 * Note:
 *      property identifier should be one of the following:
 *        - ECMA_INTERNAL_PROPERTY_NATIVE_CODE
 */
void
ecma_create_external_pointer_property (ecma_object_t *obj_p, /**< object to create property in */
                                       ecma_internal_property_id_t id, /**< identifier of internal
                                                                        *   property to create */
                                       ecma_external_pointer_t ptr_value) /**< value to store in the property */
{
  JERRY_ASSERT (id == ECMA_INTERNAL_PROPERTY_NATIVE_CODE);

  ecma_property_t *prop_p = ecma_create_internal_property (obj_p, id);
  JERRY_STATIC_ASSERT (sizeof (uint32_t) <= sizeof (prop_p->u.internal_property.value));

  if (sizeof (ecma_external_pointer_t) == sizeof (uint32_t))
  {
    prop_p->u.internal_property.value = (uint32_t) ptr_value;
  }
  else
  {
    ecma_external_pointer_t *handler_p = ecma_alloc_external_pointer ();
    *handler_p = ptr_value;

    ECMA_SET_NON_NULL_POINTER (prop_p->u.internal_property.value, handler_p);
  }
} /* ecma_create_external_pointer_property */

/**
 * Get value of external pointer stored in the object's property with specified identifier
 *
 * Note:
 *      property identifier should be one of the following:
 *        - ECMA_INTERNAL_PROPERTY_NATIVE_CODE
 *
 * @return value of the external pointer
 */
ecma_external_pointer_t
ecma_get_external_pointer_value (ecma_object_t *obj_p, /**< object to get property value from */
                                 ecma_internal_property_id_t id) /**< identifier of internal property
                                                                  *   to get value from */
{
  JERRY_ASSERT (id == ECMA_INTERNAL_PROPERTY_NATIVE_CODE);

  ecma_property_t* prop_p = ecma_get_internal_property (obj_p, id);
  JERRY_STATIC_ASSERT (sizeof (uint32_t) <= sizeof (prop_p->u.internal_property.value));

  if (sizeof (ecma_external_pointer_t) == sizeof (uint32_t))
  {
    return (ecma_external_pointer_t) prop_p->u.internal_property.value;
  }
  else
  {
    ecma_external_pointer_t *handler_p = ECMA_GET_NON_NULL_POINTER (ecma_external_pointer_t,
                                                                    prop_p->u.internal_property.value);
    return *handler_p;
  }
} /* ecma_get_external_pointer_value */

/**
 * Free memory associated with external pointer stored in the property
 *
 * Note:
 *      property identifier should be one of the following:
 *        - ECMA_INTERNAL_PROPERTY_NATIVE_CODE
 */
void
ecma_free_external_pointer_in_property (ecma_property_t *prop_p) /**< internal property */
{
  JERRY_ASSERT (prop_p->u.internal_property.type == ECMA_INTERNAL_PROPERTY_NATIVE_CODE);

  if (sizeof (ecma_external_pointer_t) == sizeof (uint32_t))
  {
    /* no additional memory was allocated for the pointer storage */
  }
  else
  {
    ecma_external_pointer_t *handler_p = ECMA_GET_NON_NULL_POINTER (ecma_external_pointer_t,
                                                                    prop_p->u.internal_property.value);

    ecma_dealloc_external_pointer (handler_p);
  }
} /* ecma_free_external_pointer_in_property */

/**
 * @}
 * @}
 */
