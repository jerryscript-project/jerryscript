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
 * Jerry libc declarations
 */
#ifndef JERRY_LIBC_H
#define JERRY_LIBC_H

#include "globals.h"

typedef void _FILE;

#ifdef LIBC_RAW
/**
 * stdin file descriptor
 */
#define LIBC_STDIN   (_FILE*)0

/**
 * stdout file descriptor
 */
#define LIBC_STDOUT  (_FILE*)1

/**
 * stderr file descriptor
 */
#define LIBC_STDERR  (_FILE*)2
#else /* !LIBC_RAW */
extern const _FILE **libc_stdin;
extern const _FILE **libc_stdout;
extern const _FILE **libc_stderr;

/**
 * stdin file descriptor
 */
#define LIBC_STDIN   ((_FILE*)*libc_stdin)

/**
 * stdout file descriptor
 */
#define LIBC_STDOUT  ((_FILE*)*libc_stdout)

/**
 * stderr file descriptor
 */
#define LIBC_STDERR  ((_FILE*)*libc_stderr)
#endif /* !LIBC_RAW */

extern void* __memset (void *s, int c, size_t n);
extern int __memcmp (const void *s1, const void *s2, size_t n);
extern void* __memcpy (void *s1, const void *s2, size_t n);
extern void* __memmove (void *dest, const void *src, size_t n);
extern int __printf (const char *format, ...);
extern int __putchar (int);
extern void __noreturn __exit (int);

extern int __strcmp (const char *, const char *);
extern int __strncmp (const char *, const char *, size_t);
extern char* __strncpy (char *, const char *, size_t);
extern float __strtof (const char *, char **);
extern size_t __strlen (const char *);

extern int __isspace (int);
extern int __isupper (int);
extern int __islower (int);
extern int __isalpha (int);
extern int __isdigit (int);
extern int __isxdigit (int);

/**
 * 'whence' argument of __fseek that identifies position
 * the 'offset' argument is added to.
 */
typedef enum
{
  __SEEK_SET, /**< relative to begin of file */
  __SEEK_CUR, /**< relative to current position */
  __SEEK_END /**< relative to end of file */
} _whence_t;

extern _FILE* __fopen (const char *, const char *);
extern int __fclose (_FILE *);
extern int __fseek (_FILE *, long offset, _whence_t);
extern long __ftell (_FILE *);
extern void __rewind (_FILE *);
extern size_t __fread (void *, size_t, size_t, _FILE *);
extern size_t __fwrite (const void *, size_t, size_t, _FILE *);
extern int __fprintf (_FILE *, const char *, ...);

extern void jrt_set_mem_limits (size_t data_size, size_t stack_size);

#define HUGE_VAL        (1e37f)

#endif /* JERRY_LIBC_H */
