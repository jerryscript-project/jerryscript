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

#include <stdio.h>
#include <string.h>

#include "jerry.h"
#include "jrt-types.h"

/**
 * Attributes
 */
#define __attr_unused___ __attribute__((unused))
#define __attr_used___ __attribute__((used))
#define __attr_packed___ __attribute__((packed))
#define __noreturn __attribute__((noreturn))
#define __attr_noinline___ __attribute__((noinline))
#define __attr_used___ __attribute__((used))
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
#define JERRY_STATIC_ASSERT_GLUE_(a, b) a ## b
#define JERRY_STATIC_ASSERT_GLUE(a, b) JERRY_STATIC_ASSERT_GLUE_ (a, b)
#define JERRY_STATIC_ASSERT(x) \
  typedef char JERRY_STATIC_ASSERT_GLUE (static_assertion_failed_, __LINE__) \
  [ (x) ? 1 : -1 ] __attr_unused___

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

extern void __noreturn jerry_assert_fail (const char *, const char *, const char *, const uint32_t);
extern void __noreturn jerry_unreachable (const char *, const char *, const char *, const uint32_t);
extern void __noreturn jerry_unimplemented (const char *, const char *, const char *, const uint32_t);

#if !defined (JERRY_NDEBUG) || !defined (JERRY_DISABLE_HEAVY_DEBUG)
#define JERRY_ASSERT(x) do { if (__builtin_expect (!(x), 0)) { \
    jerry_assert_fail (#x, __FILE__, __func__, __LINE__); } } while (0)
#else /* !JERRY_NDEBUG || !JERRY_DISABLE_HEAVY_DEBUG*/
#define JERRY_ASSERT(x) do { if (false) { (void)(x); } } while (0)
#endif /* JERRY_NDEBUG && JERRY_DISABLE_HEAVY_NEBUG */

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
#endif /* !JERRY_ENABLE_LOG */

#define JERRY_ERROR_MSG(...) jerry_port_errormsg (__VA_ARGS__)
#define JERRY_WARNING_MSG(...) JERRY_ERROR_MSG (__VA_ARGS__)

/**
 * Mark for unreachable points and unimplemented cases
 */
template<typename... values> extern void jerry_ref_unused_variables (const values & ... unused);

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
#else /* !JERRY_NDEBUG */
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
#endif /* JERRY_NDEBUG */

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
#define JERRY_SIZE_OF_STRUCT_MEMBER(struct_name, member_name) sizeof (((struct_name *)NULL)->member_name)

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

/**
 * Placement new operator (constructs an object on a pre-allocated buffer)
 *
 * Our version of the libc library doesn't support calling the constructors and destructors of the static variables.
 * It is proposed to use placement new operator. Generally it is available via #include <new>,
 * To fix the unavailability of the header in some configurations placement new operator is implemented here.
 */
inline void *operator new (size_t, void *where)
{
  return where;
} /* operator new */

/**
 * Read data from a specified buffer.
 *
 * Note:
 *      Offset is in-out and is incremented if the read operation completes successfully.
 *
 * @return true, if read was successful, i.e. offset + data_size doesn't exceed buffer size,
 *         false - otherwise.
 */
inline bool
jrt_read_from_buffer_by_offset (const uint8_t *buffer_p, /**< buffer */
                                size_t buffer_size, /**< size of buffer */
                                size_t *in_out_buffer_offset_p, /**< in: offset to read from,
                                                                 *   out: offset, incremented on sizeof (T) */
                                void *out_data_p, /**< out: data */
                                size_t out_data_size) /**< size of the readable data */
{
  if (*in_out_buffer_offset_p + out_data_size > buffer_size)
  {
    return false;
  }

  memcpy (out_data_p, buffer_p + *in_out_buffer_offset_p, out_data_size);
  *in_out_buffer_offset_p += out_data_size;

  return true;
} /* jrt_read_from_buffer_by_offset */

/**
 * Write data to a specified buffer.
 *
 * Note:
 *      Offset is in-out and is incremented if the write operation completes successfully.
 *
 * @return true, if write was successful, i.e. offset + data_size doesn't exceed buffer size,
 *         false - otherwise.
 */
inline bool
jrt_write_to_buffer_by_offset (uint8_t *buffer_p, /**< buffer */
                               size_t buffer_size, /**< size of buffer */
                               size_t *in_out_buffer_offset_p, /**< in: offset to read from,
                                                                *   out: offset, incremented on sizeof (T) */
                               void *data_p, /**< data */
                               size_t data_size) /**< size of the writable data */
{
  if (*in_out_buffer_offset_p + data_size > buffer_size)
  {
    return false;
  }

  memcpy (buffer_p + *in_out_buffer_offset_p, data_p, data_size);
  *in_out_buffer_offset_p += data_size;

  return true;
} /* jrt_write_to_buffer_by_offset */

#endif /* !JERRY_GLOBALS_H */
