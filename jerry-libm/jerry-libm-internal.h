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
 *
 * This file is based on work under the following copyright and permission
 * notice:
 *
 *     Copyright (C) 2004 by Sun Microsystems, Inc. All rights reserved.
 *
 *     Permission to use, copy, modify, and distribute this
 *     software is freely granted, provided that this notice
 *     is preserved.
 *
 *     @(#)fdlibm.h 1.5 04/04/22
 */

#ifndef JERRY_LIBM_INTERNAL_H
#define JERRY_LIBM_INTERNAL_H

/* Sometimes it's necessary to define __LITTLE_ENDIAN explicitly
   but these catch some common cases. */

#if (defined (i386) || defined (__i386) || defined (__i386__) || \
     defined (i486) || defined (__i486) || defined (__i486__) || \
     defined (intel) || defined (x86) || defined (i86pc) || \
     defined (__alpha) || defined (__osf__) || \
     defined (__x86_64__) || defined (__arm__) || defined (__aarch64__))
#define __LITTLE_ENDIAN
#endif

#ifdef __LITTLE_ENDIAN
#define __HI(x) *(1 + (int *) &x)
#define __LO(x) *(int *) &x
#else /* !__LITTLE_ENDIAN */
#define __HI(x) *(int *) &x
#define __LO(x) *(1 + (int *) &x)
#endif /* __LITTLE_ENDIAN */

/*
 * ANSI/POSIX
 */
double acos (double x);
double asin (double x);
double atan (double x);
double atan2 (double y, double x);
double cos (double x);
double sin (double x);
double tan (double x);

double exp (double x);
double log (double x);

double pow (double x, double y);
double sqrt (double x);

double ceil (double x);
double fabs (double x);
double floor (double x);
double fmod (double x, double y);

int isnan (double x);
int finite (double x);

double nextafter (double x, double y);

/*
 * Functions callable from C, intended to support IEEE arithmetic.
 */
double copysign (double x, double y);
double scalbn (double x, int n);

#endif /* !JERRY_LIBM_INTERNAL_H */
