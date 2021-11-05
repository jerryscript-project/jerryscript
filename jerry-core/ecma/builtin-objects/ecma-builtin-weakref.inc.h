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
 * WeakRef built-in description
 */

#include "ecma-builtin-helpers-macro-defines.inc.h"

#if JERRY_BUILTIN_WEAKREF

/* Number properties:
 *  (property name, number value, writable, enumerable, configurable) */

NUMBER_VALUE (LIT_MAGIC_STRING_LENGTH, 1, ECMA_PROPERTY_FLAG_CONFIGURABLE)

STRING_VALUE (LIT_MAGIC_STRING_NAME, LIT_MAGIC_STRING_WEAKREF_UL, ECMA_PROPERTY_FLAG_CONFIGURABLE)

/* Object properties:
 *  (property name, object pointer getter) */

OBJECT_VALUE (LIT_MAGIC_STRING_PROTOTYPE, ECMA_BUILTIN_ID_WEAKREF_PROTOTYPE, ECMA_PROPERTY_FIXED)

#endif /* JERRY_BUILTIN_WEAKREF */

#include "ecma-builtin-helpers-macro-undefs.inc.h"
