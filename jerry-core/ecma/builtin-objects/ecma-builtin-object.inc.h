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
 * Object built-in description
 */

#include "ecma-builtin-helpers-macro-defines.inc.h"

/* Number properties:
 *  (property name, number value, writable, enumerable, configurable) */

/* ECMA-262 v5, 15.2.3 */
NUMBER_VALUE (LIT_MAGIC_STRING_LENGTH,
              1,
              ECMA_PROPERTY_FIXED)

/* Object properties:
 *  (property name, object pointer getter) */

/* ECMA-262 v5, 15.2.3.1 */
OBJECT_VALUE (LIT_MAGIC_STRING_PROTOTYPE,
              ECMA_BUILTIN_ID_OBJECT_PROTOTYPE,
              ECMA_PROPERTY_FIXED)

/* Routine properties:
 *  (property name, C routine name, arguments number or NON_FIXED, value of the routine's length property) */
ROUTINE (LIT_MAGIC_STRING_GET_PROTOTYPE_OF_UL, ecma_builtin_object_object_get_prototype_of, 1, 1)
ROUTINE (LIT_MAGIC_STRING_GET_OWN_PROPERTY_NAMES_UL, ecma_builtin_object_object_get_own_property_names, 1, 1)
ROUTINE (LIT_MAGIC_STRING_SEAL, ecma_builtin_object_object_seal, 1, 1)
ROUTINE (LIT_MAGIC_STRING_FREEZE, ecma_builtin_object_object_freeze, 1, 1)
ROUTINE (LIT_MAGIC_STRING_PREVENT_EXTENSIONS_UL, ecma_builtin_object_object_prevent_extensions, 1, 1)
ROUTINE (LIT_MAGIC_STRING_IS_SEALED_UL, ecma_builtin_object_object_is_sealed, 1, 1)
ROUTINE (LIT_MAGIC_STRING_IS_FROZEN_UL, ecma_builtin_object_object_is_frozen, 1, 1)
ROUTINE (LIT_MAGIC_STRING_IS_EXTENSIBLE, ecma_builtin_object_object_is_extensible, 1, 1)
ROUTINE (LIT_MAGIC_STRING_KEYS, ecma_builtin_object_object_keys, 1, 1)
ROUTINE (LIT_MAGIC_STRING_GET_OWN_PROPERTY_DESCRIPTOR_UL, ecma_builtin_object_object_get_own_property_descriptor, 2, 2)
ROUTINE (LIT_MAGIC_STRING_CREATE, ecma_builtin_object_object_create, 2, 2)
ROUTINE (LIT_MAGIC_STRING_DEFINE_PROPERTIES_UL, ecma_builtin_object_object_define_properties, 2, 2)
ROUTINE (LIT_MAGIC_STRING_DEFINE_PROPERTY_UL, ecma_builtin_object_object_define_property, 3, 3)

#ifndef CONFIG_DISABLE_ES2015_BUILTIN
ROUTINE (LIT_MAGIC_STRING_SET_PROTOTYPE_OF_UL, ecma_builtin_object_object_set_prototype_of, 2, 2)
#endif /* !CONFIG_DISABLE_ES2015_BUILTIN */

#include "ecma-builtin-helpers-macro-undefs.inc.h"
