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

#ifndef ECMA_MAGIC_STRINGS_H
#define ECMA_MAGIC_STRINGS_H

#include "ecma-globals.h"

/** \addtogroup ecma ---TODO---
 * @{
 * 
 * \addtogroup ecmamagicstrings Collection of magic string constants used in ECMA
 * @{
 */

/**
 * Identifiers of ECMA magic string constants
 */
typedef enum
{
  ECMA_MAGIC_STRING_ARGUMENTS, /**< "arguments" */
  ECMA_MAGIC_STRING_EVAL, /**< "eval" */
  ECMA_MAGIC_STRING_PROTOTYPE, /**< "prototype" */
  ECMA_MAGIC_STRING_CONSTRUCTOR, /**< "constructor" */
  ECMA_MAGIC_STRING_CALLER /**< "caller" */
} ecma_magic_string_id_t;

extern const ecma_char_t* ecma_get_magic_string( ecma_magic_string_id_t id);

/**
 * @}
 * @}
 */

#endif /* ECMA_MAGIC_STRINGS_H */
