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

#ifndef ECMA_OBJECTS_PROPERTIES_H
#define ECMA_OBJECTS_PROPERTIES_H

#include "ecma-conversion.h"
#include "ecma-globals.h"

/** \addtogroup ecma ---TODO---
 * @{
 * 
 * \addtogroup ecmaobjectsinternalops ECMA objects' operations
 * @{
 */

extern ecma_completion_value_t ecma_op_object_get( ecma_object_t *obj_p, const ecma_char_t *property_name_p);
extern ecma_property_t *ecma_op_object_get_own_property( ecma_object_t *obj_p, const ecma_char_t *property_name_p);
extern ecma_property_t *ecma_op_object_get_property( ecma_object_t *obj_p, const ecma_char_t *property_name_p);
extern ecma_completion_value_t ecma_op_object_put( ecma_object_t *obj_p,
                                                   const ecma_char_t *property_name_p,
                                                   ecma_value_t value,
                                                   bool is_throw);
extern bool ecma_op_object_can_put( ecma_object_t *obj_p, const ecma_char_t *property_name_p);
extern bool ecma_op_object_has_property( ecma_object_t *obj_p, const ecma_char_t *property_name_p);
extern ecma_completion_value_t ecma_op_object_delete( ecma_object_t *obj_p,
                                                      const ecma_char_t *property_name_p,
                                                      bool is_throw);
extern ecma_completion_value_t ecma_op_object_default_value( ecma_object_t *obj_p, ecma_preferred_type_hint_t hint);
extern ecma_completion_value_t ecma_op_object_define_own_property( ecma_object_t *obj_p,
                                                                   const ecma_char_t *property_name_p,
                                                                   ecma_property_descriptor_t property_desc,
                                                                   bool is_throw);

/**
 * @}
 * @}
 */

#endif /* !ECMA_OBJECTS_PROPERTIES_H */
