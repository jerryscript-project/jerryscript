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

/** \addtogroup ecma ---TODO---
 * @{
 *
 * \addtogroup ecmagc Garbage collector
 * @{
 */

#ifndef ECMA_GC_H
#define ECMA_GC_H

#include "ecma-globals.h"

/**
 * GC generation identifier
 */
typedef enum
{
  ECMA_GC_GEN_0, /**< generation 0 */
  ECMA_GC_GEN_1, /**< generation 1 */
  ECMA_GC_GEN_2, /**< generation 2 */
  ECMA_GC_GEN_COUNT /**< generations' number */
} ecma_gc_gen_t;

extern void ecma_gc_init (void);
extern void ecma_init_gc_info (ecma_object_t *object_p);
extern void ecma_ref_object (ecma_object_t *object_p);
extern void ecma_deref_object (ecma_object_t *object_p);
extern void ecma_gc_update_may_ref_younger_object_flag_by_value (ecma_object_t *obj_p, ecma_value_t value);
extern void ecma_gc_update_may_ref_younger_object_flag_by_object (ecma_object_t *obj_p, ecma_object_t *ref_obj_p);
extern void ecma_gc_run (ecma_gc_gen_t max_gen_to_collect);

#endif /* !ECMA_GC_H */

/**
 * @}
 * @}
 */
