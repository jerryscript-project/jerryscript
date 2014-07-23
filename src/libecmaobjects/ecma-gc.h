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

/**
 * Garbage collector interface
 */

#include "ecma-globals.h"

extern void ecma_gc_init( void);
extern void ecma_ref_object(ecma_object_t *pObject);
extern void ecma_deref_object(ecma_object_t *pObject);
extern void ecma_gc_run( void);

#endif /* !ECMA_GC_H */

/**
 * @}
 * @}
 */
