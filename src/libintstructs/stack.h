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
  STACK(temp_names, uint8_t, uint8_t)

  #define GLOBAL(NAME, VAR) \
  STACK_ELEMENT (NAME, VAR)

  #define MAX_TEMP_NAME() \
  GLOBAL(temp_names, max_temp_name)

  #define MIN_TEMP_NAME() \
  GLOBAL(temp_names, min_temp_name)

  #define TEMP_NAME() \
  GLOBAL(temp_names, temp_name)

  void
  parser_init (void)
  {
    STACK_INIT(temp_names)
  }

  void
  parser_free (void)
  {
    STACK_FREE(temp_names)
  }
*/
#ifndef STACK_H
#define STACK_H

#include "linked-list.h"

#define DEFINE_STACK_TYPE(NAME, DATA_TYPE, TYPE) \
typedef TYPE      NAME##_stack_value_type; \
typedef DATA_TYPE NAME##_stack_data_type; \
typedef struct \
{ \
  DATA_TYPE length; \
  DATA_TYPE current; \
  DATA_TYPE block_len; \
  linked_list blocks; \
} \
__packed \
NAME##_stack;

#define STACK_INIT(NAME) \
do { \
  NAME.blocks = linked_list_init (sizeof (NAME##_stack_value_type)); \
  NAME.current = NAME##_global_size; \
  NAME.length = NAME.block_len = ((linked_list_header *) NAME.blocks)->block_size / sizeof (NAME##_stack_value_type); \
} while (0)

#define STACK_FREE(NAME) \
do { \
  linked_list_free (NAME.blocks); \
  NAME.length = NAME.current = 0; \
  NAME.blocks = NULL; \
} while (0)

#define DEFINE_INCREASE_STACK_SIZE(NAME, TYPE) \
static void increase_##NAME##_stack_size (void) __unused; \
static void \
increase_##NAME##_stack_size (void) { \
  linked_list_set_element (NAME.blocks, sizeof (TYPE), NAME.current, NULL); \
  if (NAME.current >= NAME.length) \
  { \
    NAME.length = (NAME##_stack_data_type) (NAME.length + NAME.block_len); \
  } \
  NAME.current = (NAME##_stack_data_type) (NAME.current + 1); \
}

#define DEFINE_DECREASE_STACE_SIZE(NAME, TYPE) \
static void decrease_##NAME##_stack_size (void) __unused; \
static void \
decrease_##NAME##_stack_size (void) { \
  JERRY_ASSERT (NAME.current > NAME##_global_size); \
  NAME.current = (NAME##_stack_data_type) (NAME.current - 1); \
}

#define DEFINE_STACK_ELEMENT(NAME, TYPE) \
static TYPE NAME##_stack_element (NAME##_stack_data_type) __unused; \
static TYPE NAME##_stack_element (NAME##_stack_data_type elem) { \
  JERRY_ASSERT (elem < NAME.current); \
  return *((TYPE *) linked_list_element (NAME.blocks, sizeof (TYPE), elem)); \
}

#define DEFINE_SET_STACK_ELEMENT(NAME, TYPE) \
static void set_##NAME##_stack_element (NAME##_stack_data_type, TYPE) __unused; \
static void set_##NAME##_stack_element (NAME##_stack_data_type elem, TYPE value) { \
  JERRY_ASSERT (elem < NAME.current); \
  linked_list_set_element (NAME.blocks, sizeof (TYPE), elem, &value); \
}

#define DEFINE_STACK_HEAD(NAME, TYPE) \
static TYPE NAME##_stack_head (NAME##_stack_data_type) __unused; \
static TYPE NAME##_stack_head (NAME##_stack_data_type elem) { \
  JERRY_ASSERT (elem <= NAME.current); \
  return *((TYPE *) linked_list_element (NAME.blocks, sizeof (TYPE), (size_t) (NAME.current - elem))); \
}

#define DEFINE_SET_STACK_HEAD(NAME, DATA_TYPE, TYPE) \
static void set_##NAME##_stack_head (DATA_TYPE, TYPE) __unused; \
static void set_##NAME##_stack_head (DATA_TYPE elem, TYPE value) { \
  JERRY_ASSERT (elem <= NAME.current); \
  linked_list_set_element (NAME.blocks, sizeof (TYPE), (size_t) (NAME.current - elem), &value); \
}

#define DEFINE_STACK_PUSH(NAME, TYPE) \
static void NAME##_stack_push (TYPE) __unused; \
static void NAME##_stack_push (TYPE value) { \
  linked_list_set_element (NAME.blocks, sizeof (TYPE), NAME.current, &value); \
  increase_##NAME##_stack_size (); \
}

#define DEFINE_CONVERT_TO_RAW_DATA(NAME, TYPE) \
static TYPE *convert_##NAME##_to_raw_data (void) __unused; \
static TYPE *convert_##NAME##_to_raw_data (void) { \
  size_t size = mem_heap_recommend_allocation_size ( \
      ((size_t) (NAME.current + 1) * sizeof (NAME##_stack_value_type))); \
  TYPE *DATA = (TYPE *) mem_heap_alloc_block (size, MEM_HEAP_ALLOC_LONG_TERM); \
  if (DATA == NULL) \
  { \
    __printf ("Out of memory\n"); \
    JERRY_UNREACHABLE (); \
  } \
  __memset (DATA, 0, size); \
  for (NAME##_stack_data_type i = 0; i < NAME.current; i++) { \
    DATA[i] = STACK_ELEMENT (NAME, i); \
  } \
  return DATA; \
}

#define STACK_PUSH(NAME, VALUE) \
do { NAME##_stack_push (VALUE); } while (0)

#define STACK_DROP(NAME, I) \
do { \
  for (size_t i = 0, till = (size_t) (I); i < till; i++) { \
    decrease_##NAME##_stack_size (); } } while (0)

#define STACK_CLEAN(NAME) \
STACK_DROP (NAME, NAME.current - NAME##_global_size);

#define STACK_HEAD(NAME, I) \
NAME##_stack_head ((NAME##_stack_data_type) (I))

#define STACK_SET_HEAD(NAME, I, VALUE) \
do { set_##NAME##_stack_head ((NAME##_stack_data_type) (I), VALUE); } while (0)

#define STACK_INCR_HEAD(NAME, I) \
do { STACK_SET_HEAD (NAME, I, (NAME##_stack_value_type) (STACK_HEAD (NAME, I) + 1)); } while (0)

#define STACK_DECR_HEAD(NAME, I) \
do { STACK_SET_HEAD (NAME, I, (NAME##_stack_value_type) (STACK_HEAD (NAME, I) - 1)); } while (0)

#define STACK_TOP(NAME) \
STACK_HEAD (NAME, 1)

#define STACK_SWAP(NAME) \
do { \
  NAME##_stack_value_type temp = STACK_TOP(NAME); \
  STACK_SET_HEAD(NAME, 1, STACK_HEAD(NAME, 2)); \
  STACK_SET_HEAD(NAME, 2, temp); \
} while (0)

#define STACK_SIZE(NAME) \
NAME.current

#define STACK_ELEMENT(NAME, I) \
NAME##_stack_element ((NAME##_stack_data_type) (I))

#define STACK_SET_ELEMENT(NAME, I, VALUE) \
do { set_##NAME##_stack_element ((NAME##_stack_data_type) I, VALUE); } while (0);

#define STACK_CONVERT_TO_RAW_DATA(NAME, DATA) \
do { DATA = convert_##NAME##_to_raw_data (); } while (0)

#define STACK_INCR_ELEMENT(NAME, I) \
do { STACK_SET_ELEMENT (NAME, I, (NAME##_stack_value_type) (STACK_ELEMENT(NAME, I) + 1)); } while (0)

#define STACK_DECR_ELEMENT(NAME, I) \
do { STACK_SET_ELEMENT (NAME, I, (NAME##_stack_value_type) (STACK_ELEMENT(NAME, I) - 1)); } while (0)

#define STACK_ITERATE_VARG(NAME, FUNC, FROM, ...) \
do { for (NAME##_stack_data_type i = FROM; i < NAME.current; i++) { \
  FUNC (STACK_ELEMENT (NAME, i), __VA_ARGS__); \
} } while (0)

#define STACK(NAME, DATA_TYPE, TYPE) \
DEFINE_STACK_TYPE(NAME, DATA_TYPE, TYPE) \
NAME##_stack NAME; \
DEFINE_DECREASE_STACE_SIZE (NAME, TYPE) \
DEFINE_INCREASE_STACK_SIZE (NAME, TYPE) \
DEFINE_STACK_ELEMENT(NAME, TYPE) \
DEFINE_SET_STACK_ELEMENT(NAME, TYPE) \
DEFINE_STACK_HEAD(NAME, TYPE) \
DEFINE_CONVERT_TO_RAW_DATA(NAME, TYPE) \
DEFINE_SET_STACK_HEAD(NAME, DATA_TYPE, TYPE) \
DEFINE_STACK_PUSH(NAME, TYPE)

#define STATIC_STACK(NAME, DATA_TYPE, TYPE) \
DEFINE_STACK_TYPE(NAME, DATA_TYPE, TYPE) \
static NAME##_stack NAME; \
DEFINE_DECREASE_STACE_SIZE (NAME, TYPE) \
DEFINE_INCREASE_STACK_SIZE (NAME, TYPE) \
DEFINE_STACK_ELEMENT(NAME, TYPE) \
DEFINE_SET_STACK_ELEMENT(NAME, TYPE) \
DEFINE_STACK_HEAD(NAME, TYPE) \
DEFINE_CONVERT_TO_RAW_DATA(NAME, TYPE) \
DEFINE_SET_STACK_HEAD(NAME, DATA_TYPE, TYPE) \
DEFINE_STACK_PUSH(NAME, TYPE)

#ifndef JERRY_NDEBUG
#define STACK_DECLARE_USAGE(NAME) \
NAME##_stack_data_type NAME##_current = NAME.current;
#define STACK_CHECK_USAGE(NAME) \
do { \
  JERRY_ASSERT (NAME.current == NAME##_current); \
} while (0);
#else
#define STACK_DECLARE_USAGE(NAME) ;
#define STACK_CHECK_USAGE(NAME) ;
#endif /* JERRY_NDEBUG */

#endif /* STACK_H */
