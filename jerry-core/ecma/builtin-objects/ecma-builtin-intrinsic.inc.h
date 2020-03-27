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
 * Intrinsic built-in description
 */

#include "ecma-builtin-helpers-macro-defines.inc.h"

#if ENABLED (JERRY_ES2015)

/* ECMA-262 v6, 19.4.2.2 */
SYMBOL_VALUE (LIT_GLOBAL_SYMBOL_HAS_INSTANCE,
              LIT_MAGIC_STRING_HAS_INSTANCE)

/* ECMA-262 v6, 19.4.2.3 */
SYMBOL_VALUE (LIT_GLOBAL_SYMBOL_IS_CONCAT_SPREADABLE,
              LIT_MAGIC_STRING_IS_CONCAT_SPREADABLE)

/* ECMA-262 v6, 19.4.2.4 */
SYMBOL_VALUE (LIT_GLOBAL_SYMBOL_ITERATOR,
              LIT_MAGIC_STRING_ITERATOR)

/* ECMA-262 v6, 19.4.2.6 */
SYMBOL_VALUE (LIT_GLOBAL_SYMBOL_MATCH,
              LIT_MAGIC_STRING_MATCH)

/* ECMA-262 v6, 19.4.2.8 */
SYMBOL_VALUE (LIT_GLOBAL_SYMBOL_REPLACE,
              LIT_MAGIC_STRING_REPLACE)

/* ECMA-262 v6, 19.4.2.9 */
SYMBOL_VALUE (LIT_GLOBAL_SYMBOL_SEARCH,
              LIT_MAGIC_STRING_SEARCH)

/* ECMA-262 v6, 19.4.2.10 */
SYMBOL_VALUE (LIT_GLOBAL_SYMBOL_SPECIES,
              LIT_MAGIC_STRING_SPECIES)

/* ECMA-262 v6, 19.4.2.11 */
SYMBOL_VALUE (LIT_GLOBAL_SYMBOL_SPLIT,
              LIT_MAGIC_STRING_SPLIT)

/* ECMA-262 v6, 19.4.2.12 */
SYMBOL_VALUE (LIT_GLOBAL_SYMBOL_TO_PRIMITIVE,
              LIT_MAGIC_STRING_TO_PRIMITIVE)

/* ECMA-262 v6, 19.4.2.13 */
SYMBOL_VALUE (LIT_GLOBAL_SYMBOL_TO_STRING_TAG,
              LIT_MAGIC_STRING_TO_STRING_TAG)

/* ECMA-262 v6, 19.4.2.14 */
SYMBOL_VALUE (LIT_GLOBAL_SYMBOL_UNSCOPABLES,
              LIT_MAGIC_STRING_UNSCOPABLES)
/* Routine properties:
 *  (property name, C routine name, arguments number or NON_FIXED, value of the routine's length property) */
ROUTINE (LIT_MAGIC_STRING_PARSE_FLOAT, ECMA_INTRINSIC_PARSE_FLOAT, 1, 1)
ROUTINE (LIT_MAGIC_STRING_PARSE_INT, ECMA_INTRINSIC_PARSE_INT, 2, 2)
ROUTINE (LIT_INTERNAL_MAGIC_STRING_ARRAY_PROTOTYPE_VALUES, ECMA_INTRINSIC_ARRAY_PROTOTYPE_VALUES, 0, 0)
#endif /* ENABLED (JERRY_ES2015) */
#include "ecma-builtin-helpers-macro-undefs.inc.h"
