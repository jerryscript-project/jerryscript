/* Copyright 2014-2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged.

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

#include "jerry.h"
#include "jrt-types.h"

/**
 * Attributes
 */
#define __noreturn __attribute__((noreturn))
#define __attr_noinline___ __attribute__((noinline))
#define __attr_return_value_should_be_checked___ __attribute__((warn_unused_result))
#ifndef __attr_always_inline___
# define __attr_always_inline___ __attribute__((always_inline))
#endif /* !__attr_always_inline___ */
#ifndef __attr_const___
# define __attr_const___ __attribute__((const))
#endif /* !__attr_const___ */
#ifndef __attr_pure___
# define __attr_pure___ __attribute__((pure))
#endif /* !__attr_pure___ */

#ifndef __GNUC__
#define __extension__
#endif /* !__GNUC__ */

/**
 * Constants
 */
#define JERRY_BITSINBYTE 8

/**
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

/**
 * Variable that must not be referenced.
 *
 * May be used for static assertion checks.
 */
extern uint32_t jerry_unreferenced_expression;

extern void __noreturn jerry_assert_fail (const char *, const char *, const char *, const uint32_t);
extern void __noreturn jerry_unreachable (const char *, const char *, const char *, const uint32_t);
extern void __noreturn jerry_unimplemented (const char *, const char *, const char *, const uint32_t);

#ifndef JERRY_NDEBUG
#define JERRY_ASSERT(x) do { if (__builtin_expect (!(x), 0)) { \
    jerry_assert_fail (#x, __FILE__, __func__, __LINE__); } } while (0)
#else /* JERRY_NDEBUG */
#define JERRY_ASSERT(x) do { if (false) { (void)(x); } } while (0)
#endif /* !JERRY_NDEBUG */

#define JERRY_UNUSED(x) ((void) (x))

#ifdef JERRY_ENABLE_LOG
#define JERRY_LOG(lvl, ...) \
  do \
  { \
    if (lvl <= jerry_debug_level && jerry_log_file) \
    { \
      jerry_port_logmsg (jerry_log_file, __VA_ARGS__); \
    } \
  } \
  while (0)

#define JERRY_DLOG(...) JERRY_LOG (1, __VA_ARGS__)
#define JERRY_DDLOG(...) JERRY_LOG (2, __VA_ARGS__)
#define JERRY_DDDLOG(...) JERRY_LOG (3, __VA_ARGS__)
#else /* !JERRY_ENABLE_LOG */
#define JERRY_DLOG(...) \
  do \
  { \
    if (false) \
    { \
      jerry_ref_unused_variables (0, __VA_ARGS__); \
    } \
  } while (0)
#define JERRY_DDLOG(...) JERRY_DLOG (__VA_ARGS__)
#define JERRY_DDDLOG(...) JERRY_DLOG (__VA_ARGS__)
#endif /* JERRY_ENABLE_LOG */

#define JERRY_ERROR_MSG(...) jerry_port_errormsg (__VA_ARGS__)
#define JERRY_WARNING_MSG(...) JERRY_ERROR_MSG (__VA_ARGS__)

/**
 * Mark for unreachable points and unimplemented cases
 */
extern void jerry_ref_unused_variables (void *, ...);

#ifndef JERRY_NDEBUG
#define JERRY_UNREACHABLE() \
  do \
  { \
    jerry_unreachable (NULL, __FILE__, __func__, __LINE__); \
  } while (0)

#define JERRY_UNIMPLEMENTED(comment) \
  do \
  { \
    jerry_unimplemented (comment, __FILE__, __func__, __LINE__); \
  } while (0)

#define JERRY_UNIMPLEMENTED_REF_UNUSED_VARS(comment, ...) \
  do \
  { \
    if (false) \
    { \
      jerry_ref_unused_variables (0, __VA_ARGS__); \
    } \
    jerry_unimplemented (comment, __FILE__, __func__, __LINE__); \
  } while (0)
#else /* JERRY_NDEBUG */
#define JERRY_UNREACHABLE() \
  do \
  { \
    jerry_unreachable (NULL, NULL, NULL, 0); \
  } while (0)

#define JERRY_UNIMPLEMENTED(comment) \
  do \
  { \
    jerry_unimplemented (comment, NULL, NULL, 0); \
  } while (0)

#define JERRY_UNIMPLEMENTED_REF_UNUSED_VARS(comment, ...) \
  do \
  { \
    if (false) \
    { \
      jerry_ref_unused_variables (0, __VA_ARGS__); \
    } \
    jerry_unimplemented (comment, NULL, NULL, 0); \
  } while (0)
#endif /* !JERRY_NDEBUG */

/**
 * Conditions' likeliness, unlikeliness.
 */
#define likely(x) __builtin_expect (!!(x), 1)
#define unlikely(x) __builtin_expect (!!(x) , 0)

/**
 * Exit
 */
extern void __noreturn jerry_fatal (jerry_fatal_code_t);

/**
 * sizeof, offsetof, ...
 */
#define JERRY_SIZE_OF_STRUCT_MEMBER(struct_name, member_name) sizeof (((struct_name *) NULL)->member_name)

/**
 * Alignment
 */

/**
 * Aligns @a value to @a alignment. @a must be the power of 2.
 *
 * Returns minimum positive value, that divides @a alignment and is more than or equal to @a value
 */
#define JERRY_ALIGNUP(value, alignment) (((value) + ((alignment) - 1)) & ~((alignment) - 1))

/**
 * min, max
 */
#define JERRY_MIN(v1, v2) (((v1) < (v2)) ? (v1) : (v2))
#define JERRY_MAX(v1, v2) (((v1) < (v2)) ? (v2) : (v1))


extern bool jrt_read_from_buffer_by_offset (const uint8_t *, size_t, size_t *, void *, size_t);

extern bool jrt_write_to_buffer_by_offset (uint8_t *, size_t, size_t *, const void *, size_t);

#endif /* !JRT_H */
