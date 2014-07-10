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

extern void *__memset (void *s, int c, size_t n);
extern int __memcmp (const void *s1, const void *s2, size_t n);
extern void *__memcpy (void *s1, const void *s2, size_t n);
extern int __printf (const char *format, ...);
extern int __putchar (int);
extern void __noreturn __exit (int);

extern int __strcmp (const char *, const char *);
extern int __strncmp (const char *, const char *, size_t);
extern char *__strncpy (char *, const char *, size_t);
extern float __strtof (const char *, char **);
extern size_t __strlen (const char *);

extern int __isspace (int);
extern int __isupper (int);
extern int __islower (int);
extern int __isalpha (int);
extern int __isdigit (int);
extern int __isxdigit (int);

#define DBL_MANT_DIG    (  52)
#define DBL_DIG         (  10)
#define DBL_MIN_EXP     (-324)
#define DBL_MAX_EXP     ( 308)
#define HUGE_VAL        (1e37)

#endif /* JERRY_LIBC_H */
