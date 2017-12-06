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
 * RegExp.prototype built-in description
 */

#include "ecma-builtin-helpers-macro-defines.inc.h"

#ifndef CONFIG_DISABLE_REGEXP_BUILTIN

/* ECMA-262 v5, 15.10.6.1 */
OBJECT_VALUE (LIT_MAGIC_STRING_CONSTRUCTOR,
              ECMA_BUILTIN_ID_REGEXP,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

/* ECMA-262 v5, 15.10.7.1 */
STRING_VALUE (LIT_MAGIC_STRING_SOURCE,
              LIT_MAGIC_STRING_EMPTY_NON_CAPTURE_GROUP,
              ECMA_PROPERTY_FIXED)

/* ECMA-262 v5, 15.10.7.2 */
SIMPLE_VALUE (LIT_MAGIC_STRING_GLOBAL,
              ECMA_VALUE_FALSE,
              ECMA_PROPERTY_FIXED)

/* ECMA-262 v5, 15.10.7.3 */
SIMPLE_VALUE (LIT_MAGIC_STRING_IGNORECASE_UL,
              ECMA_VALUE_FALSE,
              ECMA_PROPERTY_FIXED)

/* ECMA-262 v5, 15.10.7.4 */
SIMPLE_VALUE (LIT_MAGIC_STRING_MULTILINE,
              ECMA_VALUE_FALSE,
              ECMA_PROPERTY_FIXED)

/* ECMA-262 v5, 15.10.7.5 */
NUMBER_VALUE (LIT_MAGIC_STRING_LASTINDEX_UL,
              0,
              ECMA_PROPERTY_FLAG_WRITABLE)

#ifndef CONFIG_DISABLE_ANNEXB_BUILTIN
ROUTINE (LIT_MAGIC_STRING_COMPILE, ecma_builtin_regexp_prototype_compile, 2, 1)
#endif /* !CONFIG_DISABLE_ANNEXB_BUILTIN */
ROUTINE (LIT_MAGIC_STRING_EXEC, ecma_builtin_regexp_prototype_exec, 1, 1)
ROUTINE (LIT_MAGIC_STRING_TEST, ecma_builtin_regexp_prototype_test, 1, 1)
ROUTINE (LIT_MAGIC_STRING_TO_STRING_UL, ecma_builtin_regexp_prototype_to_string, 0, 0)

#endif /* !CONFIG_DISABLE_REGEXP_BUILTIN */

#include "ecma-builtin-helpers-macro-undefs.inc.h"
