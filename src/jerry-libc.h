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

extern void *libc_memset(void *s, int c, size_t n);
extern int libc_memcmp(const void *s1, const void *s2, size_t n);
extern void libc_memcpy(void *s1, const void *s2, size_t n);
extern int libc_printf(const char *format, ...);

#endif /* JERRY_LIBC_H */
