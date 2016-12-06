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

#ifndef JERRY_LIBC_STDIO_H
#define JERRY_LIBC_STDIO_H

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

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
 * I/O routines
 */
int vfprintf (FILE *stream, const char *format, va_list ap);
FILE *fopen (const char *path, const char *mode);
int fclose (FILE *fp);
size_t fread (void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite (const void *ptr, size_t size, size_t nmemb, FILE *stream);
int printf (const char *format, ...);
int fprintf (FILE *stream, const char *format, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !JERRY_LIBC_STDIO_H */
