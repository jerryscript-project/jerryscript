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

#if ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY)

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmaarraybufferobject ECMA ArrayBuffer object related routines
 * @{
 */

ecma_value_t
ecma_op_create_arraybuffer_object (const ecma_value_t *, ecma_length_t);

/**
 * Helper functions for arraybuffer.
 */
ecma_object_t *
ecma_arraybuffer_new_object (ecma_length_t lengh);
ecma_object_t *
ecma_arraybuffer_new_object_external (ecma_length_t length,
                                      void *buffer_p,
                                      ecma_object_native_free_callback_t free_cb);
lit_utf8_byte_t * JERRY_ATTR_PURE
ecma_arraybuffer_get_buffer (ecma_object_t *obj_p);
ecma_length_t JERRY_ATTR_PURE
ecma_arraybuffer_get_length (ecma_object_t *obj_p);
bool JERRY_ATTR_PURE
ecma_arraybuffer_is_detached (ecma_object_t *obj_p);
bool JERRY_ATTR_PURE
ecma_arraybuffer_is_detachable (ecma_object_t *obj_p);
bool
ecma_arraybuffer_detach (ecma_object_t *obj_p);
bool
ecma_is_arraybuffer (ecma_value_t val);

/**
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */
#endif /* !ECMA_ARRAYBUFFER_OBJECT_H */
