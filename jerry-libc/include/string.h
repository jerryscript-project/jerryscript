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

#ifndef JERRY_LIBC_STRING_H
#define JERRY_LIBC_STRING_H

#include <stddef.h>

#ifdef __cplusplus
# define EXTERN_C "C"
#else /* !__cplusplus */
# define EXTERN_C
#endif /* !__cplusplus */

extern EXTERN_C int    memcmp (const void *s1, const void *s2, size_t n);
extern EXTERN_C void*  memcpy (void *dest, const void *src, size_t n);
extern EXTERN_C void*  memset (void *s, int c, size_t n);
extern EXTERN_C int    strcmp (const char *s1, const char *s2);
extern EXTERN_C size_t strlen (const char *s);
extern EXTERN_C void*  memmove (void *dest, const void *src, size_t n);
extern EXTERN_C int    strncmp (const char *s1, const char *s2, size_t n);
extern EXTERN_C char*  strncpy (char *dest, const char *src, size_t n);

#endif /* !JERRY_LIBC_STRING_H */
