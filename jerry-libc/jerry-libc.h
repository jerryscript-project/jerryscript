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

/**
 * Jerry libc declarations
 */
#ifndef JERRY_LIBC_H
#define JERRY_LIBC_H

#include "jrt.h"

typedef void _FILE;

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

extern void* memset (void *s, int c, size_t n);
extern int memcmp (const void *s1, const void *s2, size_t n);
extern void* memcpy (void *s1, const void *s2, size_t n);
extern void* memmove (void *dest, const void *src, size_t n);
extern int printf (const char *format, ...);
extern int putchar (int);
extern "C" void __noreturn exit (int);

extern int strcmp (const char *, const char *);
extern int strncmp (const char *, const char *, size_t);
extern char* strncpy (char *, const char *, size_t);
extern float __strtof (const char *, char **);
extern size_t strlen (const char *);

extern int isspace (int);
extern int isupper (int);
extern int islower (int);
extern int isalpha (int);
extern int isdigit (int);
extern int isxdigit (int);

/**
 * 'whence' argument of fseek that identifies position
 * the 'offset' argument is added to.
 */
typedef enum
{
  __SEEK_SET, /**< relative to begin of file */
  __SEEK_CUR, /**< relative to current position */
  __SEEK_END /**< relative to end of file */
} _whence_t;

extern _FILE* fopen (const char *, const char *);
extern int fclose (_FILE *);
extern int fseek (_FILE *, long offset, _whence_t);
extern long ftell (_FILE *);
extern void rewind (_FILE *);
extern size_t fread (void *, size_t, size_t, _FILE *);
extern size_t fwrite (const void *, size_t, size_t, _FILE *);
extern int fprintf (_FILE *, const char *, ...);

extern void jrt_set_mem_limits (size_t data_size, size_t stack_size);

#define HUGE_VAL        (1e37f)

#endif /* JERRY_LIBC_H */
