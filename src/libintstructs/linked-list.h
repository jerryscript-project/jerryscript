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

#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "globals.h"

#define LINKED_LIST_MAGIC 0x42

typedef struct linked_list_header
{
  struct linked_list_header *next;
  struct linked_list_header *prev;
  uint8_t magic;
  uint8_t block_size;
  uint8_t used_size;
}
__packed
linked_list_header;

typedef uint8_t* linked_list;
#define null_list NULL

linked_list linked_list_init (size_t);
void linked_list_free (linked_list);
void *linked_list_element (linked_list, size_t, size_t);
void linked_list_set_element (linked_list, size_t, size_t, void *);

#endif /* LINKED_LIST_H */
