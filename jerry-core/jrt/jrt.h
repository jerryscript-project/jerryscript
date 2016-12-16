/* Copyright JS Foundation and other contributors, http://js.foundation
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

#ifndef JRT_H
#define JRT_H

#include <stdio.h>
#include <string.h>

#include "jerry-api.h"
#include "jerry-port.h"
#include "jrt-types.h"

/*
 * Attributes
 */
#define __noreturn __attribute__((noreturn))
#define __attr_noinline___ __attribute__((noinline))
#define __attr_return_value_should_be_checked___ __attribute__((warn_unused_result))
#define __attr_hot___ __attribute__((hot))
#ifndef __attr_always_inline___
# define __attr_always_inline___ __attribute__((always_inline))
#endif /* !__attr_always_inline___ */
#ifndef __attr_const___
# define __attr_const___ __attribute__((const))
#endif /* !__attr_const___ */
#ifndef __attr_pure___
# define __attr_pure___ __attribute__((pure))
#endif /* !__attr_pure___ */

/*
 * Conditions' likeliness, unlikeliness.
 */
#define likely(x) __builtin_expect (!!(x), 1)
#define unlikely(x) __builtin_expect (!!(x) , 0)

/*
 * Normally compilers store const(ant)s in ROM. Thus saving RAM.
 * But if your compiler does not support it then the directive below can force it.
 *
 * For the moment it is mainly meant for the following targets:
 *      - ESP8266
 */
#ifndef JERRY_CONST_DATA
# define JERRY_CONST_DATA
#endif /* JERRY_CONST_DATA */

/*
 * Constants
 */
#define JERRY_BITSINBYTE 8

/*
 * Make sure unused parameters, variables, or expressions trigger no compiler warning.
 */
#define JERRY_UNUSED(x) ((void) (x))

/*
 * Asserts
 *
 * Warning:
 *         Don't use JERRY_STATIC_ASSERT in headers, because
 *         __LINE__ may be the same for asserts in a header
 *         and in an implementation file.
 */
#define JERRY_STATIC_ASSERT_GLUE_(a, b, c) a ## b ## _ ## c
#define JERRY_STATIC_ASSERT_GLUE(a, b, c) JERRY_STATIC_ASSERT_GLUE_ (a, b, c)
#define JERRY_STATIC_ASSERT(x, msg) \
  enum { JERRY_STATIC_ASSERT_GLUE (static_assertion_failed_, __LINE__, msg) = 1 / (!!(x)) }

#ifndef JERRY_NDEBUG
void __noreturn jerry_assert_fail (const char *assertion, const char *file, const char *function, const uint32_t line);
void __noreturn jerry_unreachable (const char *file, const char *function, const uint32_t line);

#define JERRY_ASSERT(x) \
  do \
  { \
    if (unlikely (!(x))) \
    { \
      jerry_assert_fail (#x, __FILE__, __func__, __LINE__); \
    } \
  } while (0)

#define JERRY_UNREACHABLE() \
  do \
  { \
    jerry_unreachable (__FILE__, __func__, __LINE__); \
  } while (0)
#else /* JERRY_NDEBUG */
#define JERRY_ASSERT(x) \
  do \
  { \
    if (false) \
    { \
      JERRY_UNUSED (x); \
    } \
  } while (0)

#define JERRY_UNREACHABLE() __builtin_unreachable ()
#endif /* !JERRY_NDEBUG */

/**
 * Exit on fatal error
 */
void __noreturn jerry_fatal (jerry_fatal_code_t code);

/*
 * Logging
 */
#define JERRY_ERROR_MSG(...) jerry_port_log (JERRY_LOG_LEVEL_ERROR, __VA_ARGS__)
#define JERRY_WARNING_MSG(...) jerry_port_log (JERRY_LOG_LEVEL_WARNING, __VA_ARGS__)
#define JERRY_DEBUG_MSG(...) jerry_port_log (JERRY_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define JERRY_TRACE_MSG(...) jerry_port_log (JERRY_LOG_LEVEL_TRACE, __VA_ARGS__)

/**
 * Size of struct member
 */
#define JERRY_SIZE_OF_STRUCT_MEMBER(struct_name, member_name) sizeof (((struct_name *) NULL)->member_name)

/**
 * Aligns @a value to @a alignment. @a must be the power of 2.
 *
 * Returns minimum positive value, that divides @a alignment and is more than or equal to @a value
 */
#define JERRY_ALIGNUP(value, alignment) (((value) + ((alignment) - 1)) & ~((alignment) - 1))

/*
 * min, max
 */
#define JERRY_MIN(v1, v2) (((v1) < (v2)) ? (v1) : (v2))
#define JERRY_MAX(v1, v2) (((v1) < (v2)) ? (v2) : (v1))

#endif /* !JRT_H */
