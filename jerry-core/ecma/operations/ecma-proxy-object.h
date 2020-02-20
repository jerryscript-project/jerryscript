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

#ifndef ECMA_PROXY_OBJECT_H
#define ECMA_PROXY_OBJECT_H

#include "ecma-globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaproxyobject ECMA Proxy object related routines
 * @{
 */

#if ENABLED (JERRY_ES2015_BUILTIN_PROXY)

ecma_object_t *
ecma_proxy_create (ecma_value_t target,
                   ecma_value_t handler);

ecma_object_t *
ecma_proxy_create_revocable (ecma_value_t target,
                             ecma_value_t handler);

ecma_value_t
ecma_proxy_revoke_cb (const ecma_value_t function_obj,
                      const ecma_value_t this_val,
                      const ecma_value_t args_p[],
                      const ecma_length_t args_count);

jmem_cpointer_t
ecma_proxy_object_prototype_to_cp (ecma_value_t proto);

ecma_value_t
ecma_proxy_object_find (ecma_object_t *obj_p,
                        ecma_string_t *prop_name_p);

/* Interal operations */

ecma_value_t
ecma_proxy_object_get_prototype_of (ecma_object_t *obj_p);

ecma_value_t
ecma_proxy_object_set_prototype_of (ecma_object_t *obj_p,
                                    ecma_value_t proto);

ecma_value_t
ecma_proxy_object_is_extensible (ecma_object_t *obj_p);

ecma_value_t
ecma_proxy_object_prevent_extensions (ecma_object_t *obj_p);

ecma_value_t
ecma_proxy_object_get_own_property_descriptor (ecma_object_t *obj_p,
                                               ecma_string_t *prop_name_p,
                                               ecma_property_descriptor_t *prop_desc_p);

ecma_value_t
ecma_proxy_object_define_own_property (ecma_object_t *obj_p,
                                       ecma_string_t *prop_name_p,
                                       const ecma_property_descriptor_t *prop_desc_p);

ecma_value_t
ecma_proxy_object_has (ecma_object_t *obj_p,
                       ecma_string_t *prop_name_p);

ecma_value_t
ecma_proxy_object_get (ecma_object_t *obj_p,
                       ecma_string_t *prop_name_p,
                       ecma_value_t receiver);

ecma_value_t
ecma_proxy_object_set (ecma_object_t *obj_p,
                       ecma_string_t *prop_name_p,
                       ecma_value_t name,
                       ecma_value_t receiver);

ecma_value_t
ecma_proxy_object_delete_property (ecma_object_t *obj_p,
                                   ecma_string_t *prop_name_p);

ecma_value_t
ecma_proxy_object_enumerate (ecma_object_t *obj_p);

ecma_collection_t *
ecma_proxy_object_own_property_keys (ecma_object_t *obj_p);

ecma_value_t
ecma_proxy_object_call (ecma_object_t *obj_p,
                        ecma_value_t this_argument,
                        const ecma_value_t *args_p,
                        ecma_length_t argc);

ecma_value_t
ecma_proxy_object_construct (ecma_object_t *obj_p,
                             ecma_object_t *new_target_p,
                             const ecma_value_t *args_p,
                             ecma_length_t argc);

#endif /* ENABLED (JERRY_ES2015_BUILTIN_PROXY) */

/**
 * @}
 * @}
 */

#endif /* !ECMA_PROXY_OBJECT_H */
