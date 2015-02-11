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

#ifndef JERRY_GLOBALS_H
#define JERRY_GLOBALS_H

#include "jrt_types.h"

/**
 * Attributes
 */
#define __unused __attribute__((unused))
#define __used __attribute__((used))
#define __packed __attribute__((packed))
#define __noreturn __attribute__((noreturn))
#define __noinline __attribute__((noinline))
#define __used __attribute__((used))
#ifndef __attribute_always_inline__
# define __attribute_always_inline__ __attribute__((always_inline))
#endif /* !__attribute_always_inline__ */
#ifndef __attribute_const__
# define __attribute_const__ __attribute__((const))
#endif /* !__attribute_const__ */
#ifndef __attribute_pure__
# define __attribute_pure__ __attribute__((pure))
#endif /* !__attribute_pure__ */

/**
 * Constants
 */
#define JERRY_BITSINBYTE 8

/**
 * Error codes
 *
 * TODO: Move to jerry.h
 */
typedef enum
{
  ERR_OK = 0,
  ERR_IO = -1,
  ERR_BUFFER_SIZE = -2,
  ERR_SEVERAL_FILES = -3,
  ERR_NO_FILES = -4,
  ERR_NON_CHAR = -5,
  ERR_UNCLOSED = -6,
  ERR_INT_LITERAL = -7,
  ERR_STRING = -8,
  ERR_PARSER = -9,
  ERR_OUT_OF_MEMORY = -10,
  ERR_SYSCALL = -11,
  ERR_UNHANDLED_EXCEPTION = -12,
  ERR_UNIMPLEMENTED_CASE = -118,
  ERR_FAILED_ASSERTION_IN_SCRIPT = -119,
  ERR_FAILED_INTERNAL_ASSERTION = -120,
} jerry_err_t;

/**
 * Asserts
 *
 * Warning:
 *         Don't use JERRY_STATIC_ASSERT in headers, because
 *         __LINE__ may be the same for asserts in a header
 *         and in an implementation file.
 */
#define JERRY_STATIC_ASSERT_GLUE_(a, b) a ## b
#define JERRY_STATIC_ASSERT_GLUE(a, b) JERRY_STATIC_ASSERT_GLUE_ (a, b)
#define JERRY_STATIC_ASSERT(x) \
  typedef char JERRY_STATIC_ASSERT_GLUE (static_assertion_failed_, __LINE__) \
  [ (x) ? 1 : -1 ] __unused

#define CALL_PRAGMA(x) _Pragma (#x)

#ifdef JERRY_PRINT_TODO
#define TODO(x) CALL_PRAGMA (message ("TODO - " #x))
#else /* !JERRY_PRINT_TODO */
#define TODO(X)
#endif /* !JERRY_PRINT_TODO */

#ifdef JERRY_PRINT_FIXME
#define FIXME(x) CALL_PRAGMA (message ("FIXME - " #x))
#else /* !JERRY_PRINT_FIXME */
#define FIXME(X)
#endif /* !JERRY_PRINT_FIXME */

/**
 * Variable that must not be referenced.
 *
 * May be used for static assertion checks.
 */
extern uint32_t jerry_unreferenced_expression;

extern void __noreturn jerry_assert_fail (const char *assertion, const char *file, const char *function,
                                          const uint32_t line);
extern void __noreturn jerry_unreachable (const char *comment, const char *file, const char *function,
                                          const uint32_t line);
extern void __noreturn jerry_unimplemented (const char *comment, const char *file, const char *function,
                                            const uint32_t line);

#ifndef JERRY_NDEBUG
#define JERRY_ASSERT(x) do { if (__builtin_expect (!(x), 0)) { \
    jerry_assert_fail (#x, __FILE__, __FUNCTION__, __LINE__); } } while (0)
#else /* !JERRY_NDEBUG */
#define JERRY_ASSERT(x) do { if (false) { (void)(x); } } while (0)
#endif /* !JERRY_NDEBUG */

/**
 * Mark for unreachable points and unimplemented cases
 */
template<typename... values> extern void jerry_ref_unused_variables (const values & ... unused);

#if !defined (JERRY_NDEBUG) && defined (__TARGET_HOST)
#define JERRY_UNREACHABLE() \
  do \
  { \
    jerry_unreachable (NULL, __FILE__, __FUNCTION__, __LINE__); \
  } while (0)

#define JERRY_UNIMPLEMENTED(comment) \
  do \
  { \
    jerry_unimplemented (comment, __FILE__, __FUNCTION__, __LINE__); \
  } while (0)

#define JERRY_UNIMPLEMENTED_REF_UNUSED_VARS(comment, ...) \
  do \
  { \
    jerry_unimplemented (comment, __FILE__, __FUNCTION__, __LINE__); \
    if (false) \
    { \
      jerry_ref_unused_variables (0, __VA_ARGS__); \
    } \
  } while (0)
#else /* !JERRY_NDEBUG && __TARGET_HOST */
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
    jerry_unimplemented (comment, NULL, NULL, 0); \
    if (false) \
    { \
      jerry_ref_unused_variables (0, __VA_ARGS__); \
    } \
  } while (0)
#endif /* JERRY_NDEBUG || !TARGET_HOST */
/**
 * Conditions' likeliness, unlikeliness.
 */
#define likely(x) __builtin_expect (!!(x), 1)
#define unlikely(x) __builtin_expect (!!(x) , 0)

/**
 * Exit
 */
extern void __noreturn jerry_exit (jerry_err_t code);

/**
 * sizeof, offsetof, ...
 */
#define JERRY_SIZE_OF_STRUCT_MEMBER(struct_name, member_name) sizeof (((struct_name*)NULL)->member_name)

/**
 * Alignment
 */

/**
 * Aligns @value to @alignment.
 *
 * Returns maximum positive value, that divides @alignment and is less than or equal to @value
 */
#define JERRY_ALIGNDOWN(value, alignment) ((alignment) * ((value) / (alignment)))

/**
 * Aligns @value to @alignment.
 *
 * Returns minimum positive value, that divides @alignment and is more than or equal to @value
 */
#define JERRY_ALIGNUP(value, alignment) ((alignment) * (((value) + (alignment) - 1) / (alignment)))

/**
 * min, max
 */
#define JERRY_MIN(v1, v2) ((v1 < v2) ? v1 : v2)
#define JERRY_MAX(v1, v2) ((v1 < v2) ? v2 : v1)

#endif /* !JERRY_GLOBALS_H */
