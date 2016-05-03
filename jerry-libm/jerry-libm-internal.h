/* Copyright 2016 Samsung Electronics Co., Ltd.
 * Copyright 2016 University of Szeged
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
extern double acos (double);
extern double asin (double);
extern double atan (double);
extern double atan2 (double, double);
extern double cos (double);
extern double sin (double);
extern double tan (double);

extern double exp (double);
extern double log (double);

extern double pow (double, double);
extern double sqrt (double);

extern double ceil (double);
extern double fabs (double);
extern double floor (double);
extern double fmod (double, double);

extern int isnan (double);
extern int finite (double);

double nextafter (double, double);

/*
 * Functions callable from C, intended to support IEEE arithmetic.
 */
extern double copysign (double, double);
extern double scalbn (double, int);

#endif /* !JERRY_LIBM_INTERNAL_H */
