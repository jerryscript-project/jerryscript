/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#ifndef ARRAY_LIST_H
#define ARRAY_LIST_H

#include "jrt.h"

typedef uint8_t *array_list;
#define null_list NULL

array_list array_list_init (uint8_t);
void array_list_free (array_list);
array_list array_list_append (array_list, void *);
void array_list_drop_last (array_list);
void *array_list_element (array_list, size_t);
void array_list_set_element (array_list, size_t, void *);
void *array_list_last_element (array_list, size_t);
void array_list_set_last_element (array_list, size_t, void *);
size_t array_list_len (array_list);

#endif /* ARRAY_LIST_H */
