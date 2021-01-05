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
 *     @(#)e_acosh.c 1.3 95/01/18
 */

#include "jerry-math-internal.h"

/* acosh(x)
 * Method :
 *  Based on
 *    acosh(x) = log [ x + sqrt(x * x - 1) ]
 *  we have
 *    acosh(x) := log(x) + ln2, if x is large; else
 *    acosh(x) := log(2x - 1 / (sqrt(x * x - 1) + x)), if x > 2; else
 *    acosh(x) := log1p(t + sqrt(2.0 * t + t * t)); where t = x - 1.
 *
 * Special cases:
 *  acosh(x) is NaN with signal if x < 1.
 *  acosh(NaN) is NaN without signal.
 */

#define one 1.0
#define ln2 6.93147180559945286227e-01 /* 0x3FE62E42, 0xFEFA39EF */

double
acosh (double x)
{
  double t;
  int hx;
  hx = __HI (x);
  if (hx < 0x3ff00000)
  {
    /* x < 1 */
    return NAN;
  }
  else if (hx >= 0x41b00000)
  {
    /* x > 2**28 */
    if (hx >= 0x7ff00000)
    {
      /* x is inf of NaN */
      return x + x;
    }
    else
    {
      /* acosh(huge) = log(2x) */
      return log (x) + ln2;
    }
  }
  else if (((hx - 0x3ff00000) | __LO (x)) == 0)
  {
    /* acosh(1) = 0 */
    return 0.0;
  }
  else if (hx > 0x40000000)
  {
    /* 2**28 > x > 2 */
    t = x * x;
    return log (2.0 * x - one / (x + sqrt (t - one)));
  }
  else
  {
    /* 1 < x < 2 */
    t = x - one;
    return log1p (t + sqrt (2.0 * t + t * t));
  }
} /* acosh */

#undef one
#undef ln2
