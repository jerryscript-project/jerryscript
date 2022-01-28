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
 * %AsyncFunction% built-in description
 */

#include "ecma-builtin-helpers-macro-defines.inc.h"

/* ECMA-262 v11, 25.7.2 */
STRING_VALUE (LIT_MAGIC_STRING_NAME, LIT_MAGIC_STRING_ASYNC_FUNCTION_UL, ECMA_PROPERTY_FLAG_CONFIGURABLE)

/* ECMA-262 v11, 25.7.2.1 */
NUMBER_VALUE (LIT_MAGIC_STRING_LENGTH, 1, ECMA_PROPERTY_FLAG_CONFIGURABLE)

/* ECMA-262 v10, 25.7.2.2 */
OBJECT_VALUE (LIT_MAGIC_STRING_PROTOTYPE, ECMA_BUILTIN_ID_ASYNC_FUNCTION_PROTOTYPE, ECMA_PROPERTY_FIXED)

/* ECMA-262 v11, 25.7.3.2 */
STRING_VALUE (LIT_GLOBAL_SYMBOL_TO_STRING_TAG, LIT_MAGIC_STRING_ASYNC_FUNCTION_UL, ECMA_PROPERTY_FLAG_CONFIGURABLE)

#include "ecma-builtin-helpers-macro-undefs.inc.h"
