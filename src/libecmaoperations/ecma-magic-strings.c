/* Copyright 2014 Samsung Electronics Co., Ltd.
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

#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-magic-strings.h"

/** \addtogroup ecma ---TODO---
 * @{
 *
 * \addtogroup ecmamagicstrings Collection of magic string constants used in ECMA
 * @{
 */

/**
 * Get specified magic string as zero-terminated string
 *
 * @return pointer to zero-terminated magic string
 */
const ecma_char_t*
ecma_get_magic_string_zt (ecma_magic_string_id_t id) /**< magic string id */
{
  TODO(Support UTF-16);

  switch (id)
  {
    case ECMA_MAGIC_STRING_ARGUMENTS: return (ecma_char_t*) "arguments";
    case ECMA_MAGIC_STRING_EVAL: return (ecma_char_t*) "eval";
    case ECMA_MAGIC_STRING_PROTOTYPE: return (ecma_char_t*) "prototype";
    case ECMA_MAGIC_STRING_CONSTRUCTOR: return (ecma_char_t*) "constructor";
    case ECMA_MAGIC_STRING_CALLER: return (ecma_char_t*) "caller";
    case ECMA_MAGIC_STRING_CALLEE: return (ecma_char_t*) "callee";
    case ECMA_MAGIC_STRING_UNDEFINED: return (ecma_char_t*) "undefined";
    case ECMA_MAGIC_STRING_LENGTH: return (ecma_char_t*) "length";
    case ECMA_MAGIC_STRING_NAN: return (ecma_char_t*) "NaN";
    case ECMA_MAGIC_STRING_INFINITY: return (ecma_char_t*) "Infinity";
  }

  JERRY_UNREACHABLE();
} /* ecma_get_magic_string_zt */

/**
 * Get specified magic string
 *
 * @return ecma-string containing specified magic string
 */
ecma_string_t*
ecma_get_magic_string (ecma_magic_string_id_t id) /**< magic string id */
{
  return ecma_new_ecma_string (ecma_get_magic_string_zt (id));
} /* ecma_get_magic_string */

/**
 * @}
 * @}
 */
