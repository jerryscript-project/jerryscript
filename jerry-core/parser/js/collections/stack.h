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

/**
  This file contains macros to define and use stacks.
  Use macro STACK or STATIC_STACK to create stack variable and define all necessaty routines.
  Also, define variable with name NAME##_global_size. If the variable more than 0,
  first NAME##_global_size element will remain untouched during STACK_PUSH and STACK_POP operations.
  Before using the stack, init it by calling STACK_INIT macro.
  Use macros STACK_PUSH, STACK_POP, STACK_DROP, STACK_CLEAN and STACK_HEAD to manipulate the stack.
  DO NOT FORGET to free stack memory by calling STACK_FREE macro.
  For check usage of stack during a function, use STACK_DECLARE_USAGE and STACK_CHECK_USAGE macros.

  For the purpose of memory fragmentation reduction, the memory is allocated by chunks and them are
  used to store data. The chunks are connected to each other in manner of double-linked list.

  Macro STACK_CONVERT_TO_RAW_DATA allocates memory, so use it after finishing working with the stack.

  Example (parser.c):

  enum
  {
    temp_name,
    min_temp_name,
    max_temp_name,
    temp_names_global_size
  };
  STACK (temp_names, uint8_t, uint8_t)

  #define GLOBAL(NAME, VAR) \
  STACK_ELEMENT (NAME, VAR)

  #define MAX_TEMP_NAME() \
  GLOBAL (temp_names, max_temp_name)

  #define MIN_TEMP_NAME() \
  GLOBAL (temp_names, min_temp_name)

  #define TEMP_NAME() \
  GLOBAL (temp_names, temp_name)

  void
  parser_init (void)
  {
    STACK_INIT (temp_names)
  }

  void
  parser_free (void)
  {
    STACK_FREE (temp_names)
  }
*/
#ifndef STACK_H
#define STACK_H

#include "array-list.h"

#define DEFINE_STACK_TYPE(NAME, TYPE) \
typedef TYPE      NAME##_stack_value_type; \
typedef struct \
{ \
  array_list data; \
} \
NAME##_stack;

#define STACK_INIT(NAME) \
do { \
  NAME.data = array_list_init (sizeof (NAME##_stack_value_type)); \
} while (0)

#define STACK_FREE(NAME) \
do { \
  array_list_free (NAME.data); \
  NAME.data = null_list; \
} while (0)

#define DEFINE_STACK_ELEMENT(NAME, TYPE) \
static TYPE NAME##_stack_element (size_t) __attr_unused___; \
static TYPE NAME##_stack_element (size_t elem) { \
  return *((TYPE *) array_list_element (NAME.data, elem)); \
}

#define DEFINE_SET_STACK_ELEMENT(NAME, TYPE) \
static void set_##NAME##_stack_element (size_t, TYPE) __attr_unused___; \
static void set_##NAME##_stack_element (size_t elem, TYPE value) { \
  array_list_set_element (NAME.data, elem, &value); \
}

#define DEFINE_STACK_HEAD(NAME, TYPE) \
static TYPE NAME##_stack_head (size_t) __attr_unused___; \
static TYPE NAME##_stack_head (size_t elem) { \
  return *((TYPE *) array_list_last_element (NAME.data, elem)); \
}

#define DEFINE_SET_STACK_HEAD(NAME, TYPE) \
static void set_##NAME##_stack_head (size_t, TYPE) __attr_unused___; \
static void set_##NAME##_stack_head (size_t elem, TYPE value) { \
  array_list_set_last_element (NAME.data, elem, &value); \
}

#define DEFINE_STACK_PUSH(NAME, TYPE) \
static void NAME##_stack_push (TYPE) __attr_unused___; \
static void NAME##_stack_push (TYPE value) { \
  NAME.data = array_list_append (NAME.data, &value); \
}

#define STACK_PUSH(NAME, VALUE) \
do { NAME##_stack_push (VALUE); } while (0)

#define STACK_DROP(NAME, I) \
do { \
  for (size_t i = 0, till = (size_t) (I); i < till; i++) { \
    array_list_drop_last (NAME.data); } } while (0)

#define STACK_CLEAN(NAME) \
STACK_DROP (NAME, NAME.current - NAME##_global_size);

#define STACK_HEAD(NAME, I) \
NAME##_stack_head ((size_t) (I))

#define STACK_SET_HEAD(NAME, I, VALUE) \
do { set_##NAME##_stack_head ((size_t) (I), VALUE); } while (0)

#define STACK_INCR_HEAD(NAME, I) \
do { STACK_SET_HEAD (NAME, I, (NAME##_stack_value_type) (STACK_HEAD (NAME, I) + 1)); } while (0)

#define STACK_DECR_HEAD(NAME, I) \
do { STACK_SET_HEAD (NAME, I, (NAME##_stack_value_type) (STACK_HEAD (NAME, I) - 1)); } while (0)

#define STACK_TOP(NAME) \
STACK_HEAD (NAME, 1)

#define STACK_SWAP(NAME) \
do { \
  NAME##_stack_value_type temp = STACK_TOP (NAME); \
  STACK_SET_HEAD (NAME, 1, STACK_HEAD (NAME, 2)); \
  STACK_SET_HEAD (NAME, 2, temp); \
} while (0)

#define STACK_SIZE(NAME) \
array_list_len (NAME.data)

#define STACK_ELEMENT(NAME, I) \
NAME##_stack_element ((size_t) (I))

#define STACK_SET_ELEMENT(NAME, I, VALUE) \
do { set_##NAME##_stack_element ((size_t) I, VALUE); } while (0)

#define STACK_CONVERT_TO_RAW_DATA(NAME, DATA) \
do { DATA = convert_##NAME##_to_raw_data (); } while (0)

#define STACK_INCR_ELEMENT(NAME, I) \
do { STACK_SET_ELEMENT (NAME, I, (NAME##_stack_value_type) (STACK_ELEMENT (NAME, I) + 1)); } while (0)

#define STACK_DECR_ELEMENT(NAME, I) \
do { STACK_SET_ELEMENT (NAME, I, (NAME##_stack_value_type) (STACK_ELEMENT (NAME, I) - 1)); } while (0)

#define STACK_ITERATE(NAME, VAL, FROM) \
for (size_t NAME##_i = FROM; \
     NAME##_i < array_list_len (NAME.data); \
     NAME##_i++) \
{ \
  NAME##_stack_value_type VAL = STACK_ELEMENT (NAME, NAME##_i);

#define STACK_ITERATE_END() \
}

#define STACK_ITERATE_VARG_SET(NAME, FUNC, FROM, ...) \
do { for (size_t i = FROM; i < array_list_len (NAME.data); i++) { \
  STACK_SET_ELEMENT (NAME, i, FUNC (STACK_ELEMENT (NAME, i), __VA_ARGS__)); \
} } while (0)

#define STACK(NAME, TYPE) \
DEFINE_STACK_TYPE (NAME, TYPE) \
NAME##_stack NAME; \
DEFINE_STACK_ELEMENT (NAME, TYPE) \
DEFINE_SET_STACK_ELEMENT (NAME, TYPE) \
DEFINE_STACK_HEAD (NAME, TYPE) \
DEFINE_SET_STACK_HEAD (NAME, TYPE) \
DEFINE_STACK_PUSH (NAME, TYPE)

#define STATIC_STACK(NAME, TYPE) \
DEFINE_STACK_TYPE (NAME, TYPE) \
static NAME##_stack NAME; \
DEFINE_STACK_ELEMENT (NAME, TYPE) \
DEFINE_SET_STACK_ELEMENT (NAME, TYPE) \
DEFINE_STACK_HEAD (NAME, TYPE) \
DEFINE_SET_STACK_HEAD (NAME, TYPE) \
DEFINE_STACK_PUSH (NAME, TYPE)

#ifndef JERRY_NDEBUG
#define STACK_DECLARE_USAGE(NAME) \
size_t NAME##_current = array_list_len (NAME.data);
#define STACK_CHECK_USAGE(NAME) \
do { \
  JERRY_ASSERT (array_list_len (NAME.data) == NAME##_current); \
} while (0)
#else
#define STACK_DECLARE_USAGE(NAME) ;
#define STACK_CHECK_USAGE(NAME) ;
#endif /* JERRY_NDEBUG */

#endif /* STACK_H */
