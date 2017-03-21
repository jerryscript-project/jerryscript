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

#ifndef ECMA_LCACHE_H
#define ECMA_LCACHE_H

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmalcache Property lookup cache
 * @{
 */

void ecma_lcache_init (void);
void ecma_lcache_insert (ecma_object_t *object_p, jmem_cpointer_t name_cp, ecma_property_t *prop_p);
ecma_property_t *ecma_lcache_lookup (ecma_object_t *object_p, const ecma_string_t *prop_name_p);
void ecma_lcache_invalidate (ecma_object_t *object_p, jmem_cpointer_t name_cp, ecma_property_t *prop_p);
void ecma_lcache_update (ecma_object_t *object_p, jmem_cpointer_t name_cp,
                         ecma_property_t *old_prop_p, ecma_property_t *new_prop_p);

/**
 * @}
 * @}
 */

#endif /* !ECMA_LCACHE_H */
