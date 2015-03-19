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

#ifndef JERRY_LIBC_STDIO_H
#define JERRY_LIBC_STDIO_H

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
# define EXTERN_C "C"
#else /* !__cplusplus */
# define EXTERN_C
#endif /* !__cplusplus */

/**
 * File descriptor type
 */
typedef void FILE;

/**
 * Standard file descriptors
 */
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

/**
 * fseek's 'whence' argument values
 */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/**
 * I/O routines
 */
extern EXTERN_C int vfprintf (FILE *stream, const char *format, va_list ap);
extern EXTERN_C int puts (const char *s);
extern EXTERN_C FILE *fopen (const char *path, const char *mode);
extern EXTERN_C int fclose (FILE *fp);
extern EXTERN_C size_t fread (void *ptr, size_t size, size_t nmemb, FILE *stream);
extern EXTERN_C size_t fwrite (const void *ptr, size_t size, size_t nmemb, FILE *stream);
extern EXTERN_C int fseek (FILE *stream, long offset, int whence);
extern EXTERN_C long ftell (FILE *stream);
extern EXTERN_C int printf (const char *format, ...);
extern EXTERN_C void rewind (FILE *stream);
extern EXTERN_C int fprintf (FILE *stream, const char *format, ...);
extern EXTERN_C int putchar (int c);

#endif /* !JERRY_LIBC_STDIO_H */
