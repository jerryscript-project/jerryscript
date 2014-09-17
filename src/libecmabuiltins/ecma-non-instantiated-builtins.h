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

#ifndef ECMA_NON_INSTANTIATED_BUILTINS_H
#define ECMA_NON_INSTANTIATED_BUILTINS_H

#include "ecma-globals.h"

extern ecma_property_t* ecma_object_try_to_get_non_instantiated_property (ecma_object_t *object_p,
                                                                          ecma_string_t *string_p);

#endif /* ECMA_NON_INSTANTIATED_BUILTINS_H */
