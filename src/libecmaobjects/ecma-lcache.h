/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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
extern void ecma_lcache_invalidate_all (void);
extern void ecma_lcache_insert (const ecma_object_ptr_t& object_p, ecma_string_t *prop_name_p, ecma_property_t *prop_p);
extern bool
ecma_lcache_lookup (const ecma_object_ptr_t& object_p,
                    const ecma_string_t *prop_name_p,
                    ecma_property_t **prop_p_p);
extern void
ecma_lcache_invalidate (const ecma_object_ptr_t& object_p,
                        ecma_string_t *prop_name_arg_p,
                        ecma_property_t *prop_p);

/**
 * @}
 * @}
 */

#endif /* ECMA_LCACHE_H */
