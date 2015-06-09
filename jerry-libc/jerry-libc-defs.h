/* Copyright 2015 Samsung Electronics Co., Ltd.
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

#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Attributes
 */
#define __attr_unused___   __attribute__((unused))
#define __attr_used___     __attribute__((used))
#define __attr_noreturn___ __attribute__((noreturn))
#define __attr_noinline___ __attribute__((noinline))

/**
 * Assertions
 */
extern void __attr_noreturn___
libc_fatal (const char *msg,
            const char *file_name,
            const char *function_name,
            const unsigned int line_number);

#ifndef LIBC_NDEBUG
# define LIBC_ASSERT(x) do { if (__builtin_expect (!(x), 0)) { \
     libc_fatal (#x, __FILE__, __func__, __LINE__); } } while (0)
# define LIBC_UNREACHABLE() \
   do \
   { \
     libc_fatal ("Code is unreachable", __FILE__, __func__, __LINE__); \
   } while (0)
#else /* !LIBC_NDEBUG */
# define LIBC_ASSERT(x) do { if (false) { (void) (x); } } while (0)
# define LIBC_UNREACHABLE() \
   do \
   { \
     libc_fatal (NULL, NULL, NULL, 0); \
   } while (0)
#endif /* !LIBC_NDEBUG */

/**
 * Stubs declaration
 */

/**
 * Unreachable stubs for routines that are never called,
 * but referenced from third-party libraries.
 */
#define LIBC_UNREACHABLE_STUB_FOR(...) \
extern __VA_ARGS__; \
__attr_used___ __VA_ARGS__ \
{ \
  LIBC_UNREACHABLE (); \
}

#endif /* !DEFS_H */
