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
 * Function.prototype built-in description
 */

#include "ecma-builtin-helpers-macro-defines.inc.h"

/* Object properties:
 *  (property name, object pointer getter) */

/* ECMA-262 v5, 15.3.4.1 */
OBJECT_VALUE (LIT_MAGIC_STRING_CONSTRUCTOR,
              ECMA_BUILTIN_ID_FUNCTION,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

/* Number properties:
 *  (property name, object pointer getter) */

/* ECMA-262 v5, 15.3.4 */
NUMBER_VALUE (LIT_MAGIC_STRING_LENGTH,
              0,
              ECMA_PROPERTY_FIXED)

/* Routine properties:
 *  (property name, C routine name, arguments number or NON_FIXED, value of the routine's length property) */
ROUTINE (LIT_MAGIC_STRING_TO_STRING_UL, ecma_builtin_function_prototype_object_to_string, 0, 0)
ROUTINE (LIT_MAGIC_STRING_APPLY, ecma_builtin_function_prototype_object_apply, 2, 2)
ROUTINE (LIT_MAGIC_STRING_CALL, ecma_builtin_function_prototype_object_call, NON_FIXED, 1)
ROUTINE (LIT_MAGIC_STRING_BIND, ecma_builtin_function_prototype_object_bind, NON_FIXED, 1)

#include "ecma-builtin-helpers-macro-undefs.inc.h"
