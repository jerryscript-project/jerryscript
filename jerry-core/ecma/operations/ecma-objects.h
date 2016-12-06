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

#ifndef ECMA_OBJECTS_H
#define ECMA_OBJECTS_H

#include "ecma-conversion.h"
#include "ecma-globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaobjectsinternalops ECMA objects' operations
 * @{
 */

extern ecma_property_t ecma_op_object_get_own_property (ecma_object_t *, ecma_string_t *,
                                                        ecma_property_ref_t *, uint32_t);
extern ecma_property_t ecma_op_object_get_property (ecma_object_t *, ecma_string_t *,
                                                    ecma_property_ref_t *, uint32_t);
extern bool ecma_op_object_has_own_property (ecma_object_t *, ecma_string_t *);
extern bool ecma_op_object_has_property (ecma_object_t *, ecma_string_t *);
extern ecma_value_t ecma_op_object_find_own (ecma_value_t, ecma_object_t *, ecma_string_t *);
extern ecma_value_t ecma_op_object_find (ecma_object_t *, ecma_string_t *);
extern ecma_value_t ecma_op_object_get_own_data_prop (ecma_object_t *, ecma_string_t *);
extern ecma_value_t ecma_op_object_get (ecma_object_t *, ecma_string_t *);
extern ecma_value_t ecma_op_object_put (ecma_object_t *, ecma_string_t *, ecma_value_t, bool);
extern ecma_value_t ecma_op_object_delete (ecma_object_t *, ecma_string_t *, bool);
extern ecma_value_t ecma_op_object_default_value (ecma_object_t *, ecma_preferred_type_hint_t);
extern ecma_value_t ecma_op_object_define_own_property (ecma_object_t *, ecma_string_t *,
                                                        const ecma_property_descriptor_t *, bool);
extern bool ecma_op_object_get_own_property_descriptor (ecma_object_t *, ecma_string_t *,
                                                        ecma_property_descriptor_t *);
extern ecma_value_t ecma_op_object_has_instance (ecma_object_t *, ecma_value_t);
extern bool ecma_op_object_is_prototype_of (ecma_object_t *, ecma_object_t *);
extern ecma_collection_header_t * ecma_op_object_get_property_names (ecma_object_t *, bool, bool, bool);

extern lit_magic_string_id_t ecma_object_get_class_name (ecma_object_t *);
extern bool ecma_object_class_is (ecma_object_t *, uint32_t);

/**
 * @}
 * @}
 */

#endif /* !ECMA_OBJECTS_H */
