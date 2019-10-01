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
 *     Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 *     Developed at SunSoft, a Sun Microsystems, Inc. business.
 *     Permission to use, copy, modify, and distribute this
 *     software is freely granted, provided that this notice
 *     is preserved.
 *
 *     @(#)s_copysign.c 1.3 95/01/18
 */

#include "jerry-libm-internal.h"

/* copysign(x,y) returns a value with the magnitude of x and
 * with the sign bit of y.
 */

double
copysign (double x, double y)
{
  double_accessor ret;
  ret.dbl = x;
  ret.as_int.hi = (__HI (x) & 0x7fffffff) | (__HI (y) & 0x80000000);
  return ret.dbl;
} /* copysign */
