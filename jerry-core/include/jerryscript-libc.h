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

#ifndef JERRYSCRIPT_LIBC_H //{
#define JERRYSCRIPT_LIBC_H

#if defined JERRY_USING_LIBDL && JERRY_USING_LIBDL

//#define LIBC(sym) jerry_syms[JERY_SYM_##sym];
#define LIBC(sym) sym

#else

#define LIBC(sym) sym

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <setjmp.h>
#include <float.h>
#include <ctype.h>
#include <stdarg.h>
#include <math.h>

#ifdef JERRY_DEBUGGER

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>

#endif

#include <assert.h>

#ifdef HAVE_TIME_H
#include <time.h>
#elif defined (HAVE_UNISTD_H)
#include <unistd.h>
#endif /* HAVE_TIME_H */

//default-date.c
#ifdef HAVE_TM_GMTOFF
#include <time.h>
#endif /* HAVE_TM_GMTOFF */
#ifdef __GNUC__
#include <sys/time.h>
#endif /* __GNUC__ */

#endif//JERRY_USING_LIBDL

#endif //} JERRYSCRIPT_LIBC_H
