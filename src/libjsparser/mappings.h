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

#ifndef MAPPINGS_H
#define MAPPINGS_H

#ifndef JERRY_NDEBUG
#include "../jerry-libc.h"
#include "allocator.h"
#include <stdarg.h>

static inline void *
memset (void *s, int n1, size_t n2)
{
  return __memset (s, n1, n2);
}

static inline int 
memcmp (const void *s1, const void *s2, size_t n)
{
  return __memcmp (s1, s2, n);
}

static inline void *
memcpy (void *s1, const void *s2, size_t n)
{
  return __memcpy (s1, s2, n);
}

static inline int 
printf (const char *s, ...)
{  
  va_list args;
  va_start (args, s);
  int ret = __printf (s, args);
  va_end (args);
  return ret;
}

static inline int 
putchar (int c)
{
  return __putchar (c);
}

static inline void 
exit (int status)
{
  return __exit (status);
}

static inline int 
strcmp (const char *s1, const char *s2)
{
  return __strcmp (s1, s2);
}

static inline int 
strncmp (const char *s1, const char *s2, size_t n)
{
  return __strncmp (s1, s2, n);
}

static inline char *
strncpy (char *s1, const char *s2, size_t n)
{
  return __strncpy (s1, s2, n);
}

static inline float 
strtof (const char *s1, char **s2)
{
  return __strtof (s1, s2);
}

static inline size_t 
strlen (const char *s)
{
  return __strlen (s);
}

static inline int 
isspace (int s)
{
  return __isspace (s);
}

static inline int 
isupper (int s)
{
  return __isupper (s);
}

static inline int 
islower (int s)
{
  return __islower (s);
}

static inline int 
isalpha (int s)
{
  return __isalpha (s);
}

static inline int 
isdigit (int s)
{
  return __isdigit (s);
}

static inline int 
isxdigit (int s)
{
  return __isxdigit (s);
}

#else
#undef NULL
#include "../globals.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#endif

#endif // MAPPINGS_H
