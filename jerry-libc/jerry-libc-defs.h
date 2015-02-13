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
#include <stdlib.h>

/**
 * Attributes
 */
#define __attr_unused___   __attribute__((unused))
#define __attr_used___     __attribute__((used))
#define __attr_noreturn___ __attribute__((noreturn))
#define __attr_noinline___ __attribute__((noinline))

/**
 * Constants
 */
#define LIBC_FATAL_ERROR_EXIT_CODE (2)

/**
 * Assertions
 */
extern void __attr_noreturn___
libc_fatal (const char *msg,
            const char *file_name,
            const char *function_name,
            const int line_number);

#ifndef LIBC_NDEBUG
# define LIBC_ASSERT(x) do { if (__builtin_expect (!(x), 0)) { \
     libc_fatal (#x, __FILE__, __FUNCTION__, __LINE__); } } while (0)
# define LIBC_UNREACHABLE() \
   do \
   { \
     libc_fatal ("Code is unreachable", __FILE__, __FUNCTION__, __LINE__); \
   } while (0)
#else /* !LIBC_NDEBUG */
# define LIBC_ASSERT(x) do { if (false) { (void)(x); } } while (0)
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
__attr_used___ __VA_ARGS__ \
{ \
  LIBC_UNREACHABLE (); \
}

/**
 * Libc redefinitions
 */

/* Ensuring no macro implementation of variables / functions are in effect */
#undef vfprintf
#undef fprintf
#undef printf
#undef isspace
#undef isalpha
#undef islower
#undef isupper
#undef isdigit
#undef isxdigit
#undef memset
#undef memcmp
#undef memcpy
#undef memmove
#undef strcmp
#undef strncmp
#undef strncpy
#undef strlen
#undef putchar
#undef puts
#undef exit
#undef fopen
#undef rewind
#undef fclose
#undef fseek
#undef ftell
#undef fread
#undef fwrite
#undef stdin
#undef stdout
#undef stderr

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

#endif /* !DEFS_H */
