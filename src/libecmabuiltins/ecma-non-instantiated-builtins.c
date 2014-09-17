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

#include "ecma-globals.h"
#include "ecma-non-instantiated-builtins.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 */

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
 * @}
 * @}
 */
