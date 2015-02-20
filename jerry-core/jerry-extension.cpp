/* Copyright 2015 Samsung Electronics Co., Ltd.
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

#include "config.h"
#include "jerry-extension.h"
#include "jrt.h"

/** \addtogroup jerry Jerry engine extension interface
 * @{
 */

/**
 * Buffer of character data (used for exchange between core and extensions' routines)
 */
char jerry_extension_characters_buffer [CONFIG_EXTENSION_CHAR_BUFFER_SIZE];

/**
 * Extend Global scope with specified extension object
 *
 * After extension the object is accessible through non-configurable property
 * with name equal to builtin_object_name converted to ecma chars.
 */
void
jerry_extend_with (const char *builtin_object_name, /**< name of the extension object */
                   const jerry_extension_descriptor_t *desc_p) /**< description of the extension object */
{
  JERRY_UNIMPLEMENTED_REF_UNUSED_VARS (builtin_object_name, desc_p);
} /* jerry_extend_with */

/**
 * @}
 */
