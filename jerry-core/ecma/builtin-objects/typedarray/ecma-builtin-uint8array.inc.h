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

/*
 * Uint8Array description
 */

#if JERRY_BUILTIN_TYPEDARRAY

#define TYPEDARRAY_BYTES_PER_ELEMENT 1
#define TYPEDARRAY_MAGIC_STRING_ID   LIT_MAGIC_STRING_UINT8_ARRAY_UL
#define TYPEDARRAY_BUILTIN_ID        ECMA_BUILTIN_ID_UINT8ARRAY_PROTOTYPE
#include "ecma-builtin-typedarray-template.inc.h"

#endif /* JERRY_BUILTIN_TYPEDARRAY */
