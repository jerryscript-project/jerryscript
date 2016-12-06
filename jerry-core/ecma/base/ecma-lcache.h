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

extern void ecma_lcache_init (void);
extern void ecma_lcache_insert (ecma_object_t *, jmem_cpointer_t, ecma_property_t *);
extern ecma_property_t *ecma_lcache_lookup (ecma_object_t *, const ecma_string_t *);
extern void ecma_lcache_invalidate (ecma_object_t *, jmem_cpointer_t, ecma_property_t *);

/**
 * @}
 * @}
 */

#endif /* !ECMA_LCACHE_H */
