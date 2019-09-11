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

#if ENABLED (JERRY_BUILTIN_REGEXP)

/* ECMA-262 v5, 15.10.6.1 */
OBJECT_VALUE (LIT_MAGIC_STRING_CONSTRUCTOR,
              ECMA_BUILTIN_ID_REGEXP,
              ECMA_PROPERTY_CONFIGURABLE_WRITABLE)

#if ENABLED (JERRY_ES2015)
ACCESSOR_READ_ONLY (LIT_MAGIC_STRING_FLAGS,
                    ecma_builtin_regexp_prototype_get_flags,
                    ECMA_PROPERTY_FIXED)

ACCESSOR_READ_ONLY (LIT_MAGIC_STRING_SOURCE,
                    ecma_builtin_regexp_prototype_get_source,
                    ECMA_PROPERTY_FIXED)

ACCESSOR_READ_ONLY (LIT_MAGIC_STRING_GLOBAL,
                    ecma_builtin_regexp_prototype_get_global,
                    ECMA_PROPERTY_FIXED)

ACCESSOR_READ_ONLY (LIT_MAGIC_STRING_IGNORECASE_UL,
                    ecma_builtin_regexp_prototype_get_ignorecase,
                    ECMA_PROPERTY_FIXED)

ACCESSOR_READ_ONLY (LIT_MAGIC_STRING_MULTILINE,
                    ecma_builtin_regexp_prototype_get_multiline,
                    ECMA_PROPERTY_FIXED)
#else /* !ENABLED (JERRY_ES2015) */
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
#endif /* ENABLED (JERRY_ES2015) */

/* ECMA-262 v5, 15.10.7.5 */
NUMBER_VALUE (LIT_MAGIC_STRING_LASTINDEX_UL,
              0,
              ECMA_PROPERTY_FLAG_WRITABLE)

#if ENABLED (JERRY_BUILTIN_ANNEXB)
ROUTINE (LIT_MAGIC_STRING_COMPILE, ecma_builtin_regexp_prototype_compile, 2, 1)
#endif /* ENABLED (JERRY_BUILTIN_ANNEXB) */
ROUTINE (LIT_MAGIC_STRING_EXEC, ecma_builtin_regexp_prototype_exec, 1, 1)
ROUTINE (LIT_MAGIC_STRING_TEST, ecma_builtin_regexp_prototype_test, 1, 1)
ROUTINE (LIT_MAGIC_STRING_TO_STRING_UL, ecma_builtin_regexp_prototype_to_string, 0, 0)

#endif /* ENABLED (JERRY_BUILTIN_REGEXP) */

#include "ecma-builtin-helpers-macro-undefs.inc.h"
