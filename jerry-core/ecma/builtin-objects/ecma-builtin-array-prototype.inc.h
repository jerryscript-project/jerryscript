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
 * Array.prototype built-in description
 */

#include "ecma-builtin-helpers-macro-defines.inc.h"

#ifndef CONFIG_DISABLE_ARRAY_BUILTIN

/* Object properties:
 *  (property name, object pointer getter) */

/* ECMA-262 v5, 15.4.4.1 */
OBJECT_VALUE (LIT_MAGIC_STRING_CONSTRUCTOR,
              ECMA_BUILTIN_ID_ARRAY,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

/* Number properties:
 *  (property name, object pointer getter) */

/* ECMA-262 v5, 15.4.4 */
NUMBER_VALUE (LIT_MAGIC_STRING_LENGTH,
              0,
              ECMA_PROPERTY_FLAG_WRITABLE)

/* Routine properties:
 *  (property name, C routine name, arguments number or NON_FIXED, value of the routine's length property) */
ROUTINE (LIT_MAGIC_STRING_TO_STRING_UL, ecma_builtin_array_prototype_object_to_string, 0, 0)
ROUTINE (LIT_MAGIC_STRING_TO_LOCALE_STRING_UL, ecma_builtin_array_prototype_object_to_locale_string, 0, 0)
ROUTINE (LIT_MAGIC_STRING_CONCAT, ecma_builtin_array_prototype_object_concat, NON_FIXED, 1)
ROUTINE (LIT_MAGIC_STRING_JOIN, ecma_builtin_array_prototype_join, 1, 1)
ROUTINE (LIT_MAGIC_STRING_POP, ecma_builtin_array_prototype_object_pop, 0, 0)
ROUTINE (LIT_MAGIC_STRING_PUSH, ecma_builtin_array_prototype_object_push, NON_FIXED, 1)
ROUTINE (LIT_MAGIC_STRING_REVERSE, ecma_builtin_array_prototype_object_reverse, 0, 0)
ROUTINE (LIT_MAGIC_STRING_SHIFT, ecma_builtin_array_prototype_object_shift, 0, 0)
ROUTINE (LIT_MAGIC_STRING_SLICE, ecma_builtin_array_prototype_object_slice, 2, 2)
ROUTINE (LIT_MAGIC_STRING_SORT, ecma_builtin_array_prototype_object_sort, 1, 1)
ROUTINE (LIT_MAGIC_STRING_SPLICE, ecma_builtin_array_prototype_object_splice, NON_FIXED, 2)
ROUTINE (LIT_MAGIC_STRING_UNSHIFT, ecma_builtin_array_prototype_object_unshift, NON_FIXED, 1)
ROUTINE (LIT_MAGIC_STRING_INDEX_OF_UL, ecma_builtin_array_prototype_object_index_of, 2, 1)
ROUTINE (LIT_MAGIC_STRING_LAST_INDEX_OF_UL, ecma_builtin_array_prototype_object_last_index_of, NON_FIXED, 1)
ROUTINE (LIT_MAGIC_STRING_EVERY, ecma_builtin_array_prototype_object_every, 2, 1)
ROUTINE (LIT_MAGIC_STRING_SOME, ecma_builtin_array_prototype_object_some, 2, 1)
ROUTINE (LIT_MAGIC_STRING_FOR_EACH_UL, ecma_builtin_array_prototype_object_for_each, 2, 1)
ROUTINE (LIT_MAGIC_STRING_MAP, ecma_builtin_array_prototype_object_map, 2, 1)
ROUTINE (LIT_MAGIC_STRING_FILTER, ecma_builtin_array_prototype_object_filter, 2, 1)
ROUTINE (LIT_MAGIC_STRING_REDUCE, ecma_builtin_array_prototype_object_reduce, NON_FIXED, 1)
ROUTINE (LIT_MAGIC_STRING_REDUCE_RIGHT_UL, ecma_builtin_array_prototype_object_reduce_right, NON_FIXED, 1)

#endif /* !CONFIG_DISABLE_ARRAY_BUILTIN */

#include "ecma-builtin-helpers-macro-undefs.inc.h"
