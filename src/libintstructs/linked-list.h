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

#define DEFINE_LINKED_LIST_TYPE(NAME, TYPE) \
typedef struct NAME##_linked_list \
{ \
  struct NAME##_linked_list *next; \
  struct NAME##_linked_list *prev; \
} \
__packed \
NAME##_linked_list;

#define DEFINE_LIST_FREE(NAME) \
static void free_##NAME##_linked_list (uint8_t *) __unused;\
static void free_##NAME##_linked_list (uint8_t *list) { \
  if (((NAME##_linked_list *)list)->next) { \
    free_##NAME##_linked_list ((uint8_t *) ((NAME##_linked_list *)list)->next); \
  } \
  mem_heap_free_block (list); \
}

#define DEFINE_LIST_ELEMENT(NAME, TYPE) \
static TYPE NAME##_list_element (uint8_t *, uint8_t) __unused; \
static TYPE NAME##_list_element (uint8_t * raw, uint8_t i) { \
  return ((TYPE *) (raw + sizeof (NAME##_linked_list)))[i]; \
}

#define DEFINE_SET_LIST_ELEMENT(NAME, TYPE) \
static void set_##NAME##_list_element (uint8_t *, uint8_t, TYPE) __unused; \
static void set_##NAME##_list_element (uint8_t * raw, uint8_t i, TYPE value) { \
  ((TYPE *) (raw + sizeof (NAME##_linked_list)))[i] = value; \
}

#endif /* LINKED_LIST_H */
