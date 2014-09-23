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

#define DEFINE_STACK_TYPE(NAME, DATA_TYPE, TYPE) \
typedef struct \
{ \
  DATA_TYPE length; \
  DATA_TYPE current; \
  TYPE *data; \
} \
__packed \
NAME##_stack;

#define STACK_INIT(TYPE, NAME) \
do { \
  size_t stack_size = mem_heap_recommend_allocation_size (sizeof (TYPE) * NAME##_global_size); \
  NAME.data = (TYPE *) mem_heap_alloc_block (stack_size, MEM_HEAP_ALLOC_SHORT_TERM); \
  NAME.current = NAME##_global_size; \
  NAME.length = (__typeof__ (NAME.length)) (stack_size / sizeof (TYPE)); \
} while (0)

#define STACK_FREE(NAME) \
do { \
  mem_heap_free_block ((uint8_t *) NAME.data); \
  NAME.length = NAME.current = 0; \
} while (0)

/* In most cases (for example, in parser) default size of stack is enough.
   However, in serializer, there is a need for reallocation of memory.
   Do this in several steps:
    1) Allocate already allocated memory size twice.
    2) Copy used memory to the last.
    3) Dealocate first two chuncks.
    4) Allocate new memory. (It must point to the memory before increasing).
    5) Copy data back.
    6) Free temp buffer.  */
#define DEFINE_INCREASE_STACK_SIZE(NAME, TYPE) \
static void increase_##NAME##_stack_size (__typeof__ (NAME.length)) __unused; \
static void \
increase_##NAME##_stack_size (__typeof__ (NAME.length) elements_count) { \
  if (NAME.current + elements_count >= NAME.length) \
  { \
    size_t old_size = NAME.length * sizeof (TYPE); \
    size_t temp1_size = mem_heap_recommend_allocation_size ( \
                        (size_t) (elements_count * sizeof (TYPE))); \
    size_t new_size = mem_heap_recommend_allocation_size ( \
                      (size_t) (temp1_size + old_size)); \
    TYPE *temp1 = (TYPE *) mem_heap_alloc_block (temp1_size, MEM_HEAP_ALLOC_SHORT_TERM); \
    TYPE *temp2 = (TYPE *) mem_heap_alloc_block (old_size, MEM_HEAP_ALLOC_SHORT_TERM); \
    if (temp2 == NULL) \
    { \
      mem_heap_print (true, false, true); \
      JERRY_UNREACHABLE (); \
    } \
    __memcpy (temp2, NAME.data, old_size); \
    mem_heap_free_block ((uint8_t *) NAME.data); \
    mem_heap_free_block ((uint8_t *) temp1); \
    NAME.data = (TYPE *) mem_heap_alloc_block (new_size, MEM_HEAP_ALLOC_SHORT_TERM); \
    if (NAME.data == NULL) \
    { \
      __printf ("old_size: %d\ntemp1_size: %d\nnew_size: %d\n", old_size, temp1_size, new_size); \
      mem_heap_print (true, false, true); \
      JERRY_UNREACHABLE (); \
    } \
    __memcpy (NAME.data, temp2, old_size); \
    mem_heap_free_block ((uint8_t *) temp2); \
    NAME.length = (__typeof__ (NAME.length)) (new_size / sizeof (TYPE)); \
  } \
  NAME.current = (__typeof__ (NAME.current)) (NAME.current + elements_count); \
}

#define DEFINE_DECREASE_STACE_SIZE(NAME) \
static void decrease_##NAME##_stack_size (uint8_t) __unused; \
static void \
decrease_##NAME##_stack_size (uint8_t elements_count) { \
  JERRY_ASSERT (NAME.current - elements_count >= NAME##_global_size); \
  NAME.current = (__typeof__ (NAME.current)) (NAME.current - elements_count); \
}

#define STACK_PUSH(NAME, VALUE) \
do { \
  increase_##NAME##_stack_size (1); \
  NAME.data[NAME.current - 1] = VALUE; \
} while (0)

#define STACK_POP(NAME, VALUE) \
do { \
  decrease_##NAME##_stack_size (1); \
  VALUE = NAME.data[NAME.current]; \
} while (0)

#define STACK_DROP(NAME, I) \
do { decrease_##NAME##_stack_size (I); } while (0)

#define STACK_CLEAN(NAME) \
STACK_DROP (NAME, NAME.current - NAME##_global_size);

#define STACK_HEAD(NAME, I) \
NAME.data[NAME.current - I]

#define STACK_SIZE(NAME) \
NAME.current

#define STACK_ELEMENT(NAME, I) \
NAME.data[I]

#define STACK_RAW_DATA(NAME) \
NAME.data

#define STACK(NAME, DATA_TYPE, TYPE) \
DEFINE_STACK_TYPE(NAME, DATA_TYPE, TYPE) \
NAME##_stack NAME; \
DEFINE_DECREASE_STACE_SIZE (NAME) \
DEFINE_INCREASE_STACK_SIZE (NAME, TYPE)

#define STATIC_STACK(NAME, DATA_TYPE, TYPE) \
DEFINE_STACK_TYPE(NAME, DATA_TYPE, TYPE) \
static NAME##_stack NAME; \
DEFINE_DECREASE_STACE_SIZE (NAME) \
DEFINE_INCREASE_STACK_SIZE (NAME, TYPE)

#ifndef JERRY_NDEBUG
#define STACK_DECLARE_USAGE(NAME) \
__typeof__(NAME.current) NAME##_current = NAME.current;
#define STACK_CHECK_USAGE(NAME) \
do { \
  JERRY_ASSERT (NAME.current == NAME##_current); \
} while (0);
#else
#define STACK_DECLARE_USAGE(NAME) ;
#define STACK_CHECK_USAGE(NAME) ;
#endif /* JERRY_NDEBUG */

#endif /* STACK_H */
