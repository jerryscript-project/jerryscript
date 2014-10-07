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
  NAME.data[VAR]

  #define MAX_TEMP_NAME() \
  GLOBAL(temp_names, max_temp_name)

  #define MIN_TEMP_NAME() \
  GLOBAL(temp_names, min_temp_name)

  #define TEMP_NAME() \
  GLOBAL(temp_names, temp_name)

  void
  parser_init (void)
  {
    STACK_INIT(uint8_t, temp_names)
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
DEFINE_LINKED_LIST_TYPE (NAME, TYPE) \
DEFINE_LIST_FREE (NAME) \
DEFINE_LIST_ELEMENT (NAME, TYPE) \
DEFINE_SET_LIST_ELEMENT (NAME, TYPE) \
typedef TYPE      NAME##_stack_value_type; \
typedef DATA_TYPE NAME##_stack_data_type; \
typedef struct \
{ \
  DATA_TYPE length; \
  DATA_TYPE current; \
  DATA_TYPE block_len; \
  uint8_t *blocks; \
  uint8_t *last; \
} \
__packed \
NAME##_stack;

#define STACK_INIT(TYPE, NAME) \
do { \
  size_t stack_size = mem_heap_recommend_allocation_size (sizeof (TYPE) * NAME##_global_size); \
  NAME.blocks = NAME.last = mem_heap_alloc_block (stack_size, MEM_HEAP_ALLOC_SHORT_TERM); \
  __memset (NAME.blocks, 0, stack_size); \
  NAME.current = NAME##_global_size; \
  NAME.length = NAME.block_len = (NAME##_stack_data_type) ((stack_size-sizeof (NAME##_linked_list)) / sizeof (TYPE)); \
} while (0)

#define STACK_FREE(NAME) \
do { \
  free_##NAME##_linked_list (NAME.blocks); \
  NAME.length = NAME.current = 0; \
} while (0)

#ifdef JERRY_NDEBUG
#define DEFINE_INCREASE_STACK_SIZE(NAME, TYPE) \
static void increase_##NAME##_stack_size (void) __unused; \
static void \
increase_##NAME##_stack_size (void) { \
  if (NAME.current == NAME.length && ((NAME##_linked_list *) NAME.last)->next == NULL) \
  { \
    size_t temp_size = mem_heap_recommend_allocation_size ((size_t) (sizeof (NAME##_linked_list))); \
    JERRY_ASSERT ((temp_size-sizeof (NAME##_linked_list)) / sizeof (TYPE) == NAME.block_len); \
    NAME##_linked_list *temp = (NAME##_linked_list *) mem_heap_alloc_block ( \
      temp_size, MEM_HEAP_ALLOC_SHORT_TERM); \
    if (temp == NULL) \
    { \
      jerry_exit (ERR_MEMORY); \
    } \
    __memset (temp, 0, temp_size); \
    ((NAME##_linked_list *) NAME.last)->next = temp; \
    temp->prev = (NAME##_linked_list *) NAME.last; \
    NAME.last = (uint8_t *) temp; \
    NAME.length = (NAME##_stack_data_type) (NAME.length + NAME.block_len); \
  } \
  NAME.current = (NAME##_stack_data_type) (NAME.current + 1); \
}
#else
#define DEFINE_INCREASE_STACK_SIZE(NAME, TYPE) \
static void increase_##NAME##_stack_size (void) __unused; \
static void \
increase_##NAME##_stack_size (void) { \
  if (NAME.current == NAME.length && ((NAME##_linked_list *) NAME.last)->next == NULL) \
  { \
    size_t temp_size = mem_heap_recommend_allocation_size ((size_t) (sizeof (NAME##_linked_list))); \
    JERRY_ASSERT ((temp_size-sizeof (NAME##_linked_list)) / sizeof (TYPE) == NAME.block_len); \
    NAME##_linked_list *temp = (NAME##_linked_list *) mem_heap_alloc_block ( \
      temp_size, MEM_HEAP_ALLOC_SHORT_TERM); \
    if (temp == NULL) \
    { \
      mem_heap_print (true, false, true); \
      JERRY_UNREACHABLE (); \
    } \
    __memset (temp, 0, temp_size); \
    ((NAME##_linked_list *) NAME.last)->next = temp; \
    temp->prev = (NAME##_linked_list *) NAME.last; \
    NAME.last = (uint8_t *) temp; \
    NAME.length = (NAME##_stack_data_type) (NAME.length + NAME.block_len); \
  } \
  NAME.current = (NAME##_stack_data_type) (NAME.current + 1); \
}
#endif

#define DEFINE_DECREASE_STACE_SIZE(NAME, TYPE) \
static void decrease_##NAME##_stack_size (void) __unused; \
static void \
decrease_##NAME##_stack_size (void) { \
  JERRY_ASSERT (NAME.current - 1 >= NAME##_global_size); \
  NAME.current = (NAME##_stack_data_type) (NAME.current - 1); \
}

#define DEFINE_STACK_ELEMENT(NAME, TYPE) \
static TYPE NAME##_stack_element (NAME##_stack_data_type) __unused; \
static TYPE NAME##_stack_element (NAME##_stack_data_type elem) { \
  JERRY_ASSERT (elem < NAME.current); \
  NAME##_linked_list *block = (NAME##_linked_list *) NAME.blocks; \
  for (NAME##_stack_data_type i = 0; i < elem / NAME.block_len; i++) { \
    JERRY_ASSERT (block->next); \
    block = block->next; \
  } \
  return NAME##_list_element ((uint8_t *) block, (uint8_t) ((elem)%NAME.block_len)); \
}

#define DEFINE_SET_STACK_ELEMENT(NAME, TYPE) \
static void set_##NAME##_stack_element (NAME##_stack_data_type, TYPE) __unused; \
static void set_##NAME##_stack_element (NAME##_stack_data_type elem, TYPE value) { \
  JERRY_ASSERT (elem < NAME.current); \
  NAME##_linked_list *block = (NAME##_linked_list *) NAME.blocks; \
  for (NAME##_stack_data_type i = 0; i < elem / NAME.block_len; i++) { \
    JERRY_ASSERT (block->next); \
    block = block->next; \
  } \
  set_##NAME##_list_element ((uint8_t *) block, (uint8_t) ((elem)%NAME.block_len), value); \
}

#define DEFINE_STACK_HEAD(NAME, TYPE) \
static TYPE NAME##_stack_head (NAME##_stack_data_type) __unused; \
static TYPE NAME##_stack_head (NAME##_stack_data_type elem) { \
  JERRY_ASSERT (elem <= NAME.current); \
  JERRY_ASSERT (elem <= NAME.block_len); \
  if ((NAME.current % NAME.block_len) < elem) { \
    return NAME##_list_element ((uint8_t *) ((NAME##_linked_list *) NAME.last)->prev, \
                                (uint8_t) ((NAME.current % NAME.block_len) - elem + NAME.block_len)); \
  } else { \
    return NAME##_list_element (NAME.last, (uint8_t) ((NAME.current % NAME.block_len) - elem)); \
  } \
}

#define DEFINE_SET_STACK_HEAD(NAME, DATA_TYPE, TYPE) \
static void set_##NAME##_stack_head (DATA_TYPE, TYPE) __unused; \
static void set_##NAME##_stack_head (DATA_TYPE elem, TYPE value) { \
  JERRY_ASSERT (elem <= NAME.current); \
  JERRY_ASSERT (elem <= NAME.block_len); \
  if ((NAME.current % NAME.block_len) < elem) { \
    set_##NAME##_list_element ((uint8_t *) ((NAME##_linked_list *) NAME.last)->prev, \
                               (uint8_t) ((NAME.current % NAME.block_len) - elem + NAME.block_len), value); \
  } else { \
    set_##NAME##_list_element (NAME.last, (uint8_t) ((NAME.current % NAME.block_len) - elem), value); \
  } \
}

#define DEFINE_STACK_PUSH(NAME, TYPE) \
static void NAME##_stack_push (TYPE) __unused; \
static void NAME##_stack_push (TYPE value) { \
  if (NAME.current % NAME.block_len == 0 && NAME.current != 0) { \
    increase_##NAME##_stack_size (); \
    set_##NAME##_list_element (NAME.last, (uint8_t) ((NAME.current - 1) % NAME.block_len), value); \
  } else { \
    set_##NAME##_list_element (NAME.last, (uint8_t) ((NAME.current) % NAME.block_len), value); \
    increase_##NAME##_stack_size (); \
  } \
}

#define DEFINE_CONVERT_TO_RAW_DATA(NAME, TYPE) \
static TYPE *convert_##NAME##_to_raw_data (void) __unused; \
static TYPE *convert_##NAME##_to_raw_data (void) { \
  size_t size = mem_heap_recommend_allocation_size ( \
      ((size_t) (NAME.current + 1) * sizeof (NAME##_stack_value_type))); \
  TYPE *DATA = (TYPE *) mem_heap_alloc_block (size, MEM_HEAP_ALLOC_LONG_TERM); \
  __memset (DATA, 0, size); \
  for (NAME##_stack_data_type i = 0; i < NAME.current; i++) { \
    DATA[i] = STACK_ELEMENT (NAME, i); \
  } \
  return DATA; \
}

#define STACK_PUSH(NAME, VALUE) \
do { NAME##_stack_push (VALUE); } while (0)

#define STACK_POP(NAME, VALUE) \
do { \
  decrease_##NAME##_stack_size (); \
  VALUE = NAME##_list_element (NAME.last, (uint8_t) (NAME.current % NAME.block_len)); \
} while (0)

#define STACK_DROP(NAME, I) \
do { \
  JERRY_ASSERT ((I) >= 0); \
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

#define STACK_ITERATE_VARG(NAME, FUNC, ...) \
do { for (NAME##_stack_data_type i = 0; i < NAME.current; i++) { \
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
