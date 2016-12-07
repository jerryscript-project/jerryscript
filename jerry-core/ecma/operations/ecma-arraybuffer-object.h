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

#ifndef ECMA_ARRAYBUFFER_OBJECT_H
#define ECMA_ARRAYBUFFER_OBJECT_H

#include "ecma-globals.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaarraybufferobject ECMA Arraybuffer object related routines
 * @{
 */

extern ecma_value_t
ecma_op_create_arraybuffer_object (const ecma_value_t *, ecma_length_t);

/**
 * Helper functions for arraybuffer.
 */
extern ecma_object_t *
ecma_arraybuffer_new_object (ecma_length_t);
extern lit_utf8_byte_t *
ecma_arraybuffer_get_buffer (ecma_object_t *) __attr_pure___ __attr_always_inline___;
extern ecma_length_t
ecma_arraybuffer_get_length (ecma_object_t *) __attr_pure___ __attr_always_inline___;

/**
 * @}
 * @}
 */

#endif /* !ECMA_ARRAYBUFFER_OBJECT_H */
