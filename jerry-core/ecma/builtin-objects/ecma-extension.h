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

#ifndef ECMA_EXTENSION_H
#define ECMA_EXTENSION_H

#include "ecma-globals.h"
#include "jerry-extension.h"

/**
 * Maximum number of registered extensions
 */
#define ECMA_EXTENSION_MAX_NUMBER_OF_EXTENSIONS (CONFIG_EXTENSION_MAX_NUMBER_OF_EXTENSIONS)

/**
 * Maximum number of functions in an extension
 */
#define ECMA_EXTENSION_MAX_FUNCTIONS_IN_EXTENSION (CONFIG_EXTENSION_MAX_FUNCTIONS_IN_EXTENSION)

/**
 * Maximum number of arguments in a function
 */
#define ECMA_EXTENSION_MAX_ARGUMENTS_IN_FUNCTION (CONFIG_EXTENSION_MAX_ARGUMENTS_IN_FUNCTION)

extern bool ecma_extension_register (jerry_extension_descriptor_t *extension_desc_p);
extern ecma_property_t* ecma_op_extension_object_get_own_property (ecma_object_t*, ecma_string_t*);

#endif /* ECMA_EXTENSION_H */
