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
 *     @(#)s_scalbn.c 1.3 95/01/18
 */

#include "jerry-libm-internal.h"

/* scalbn(x,n) returns x* 2**n  computed by  exponent
 * manipulation rather than by actually performing an
 * exponentiation or a multiplication.
 */

#define two54  1.80143985094819840000e+16 /* 0x43500000, 0x00000000 */
#define twom54 5.55111512312578270212e-17 /* 0x3C900000, 0x00000000 */
#define huge   1.0e+300
#define tiny   1.0e-300

double
scalbn (double x, int n)
{
  int k, hx, lx;

  hx = __HI (x);
  lx = __LO (x);
  k = (hx & 0x7ff00000) >> 20; /* extract exponent */
  if (k == 0) /* 0 or subnormal x */
  {
    if ((lx | (hx & 0x7fffffff)) == 0) /* +-0 */
    {
      return x;
    }
    x *= two54;
    hx = __HI (x);
    k = ((hx & 0x7ff00000) >> 20) - 54;
    if (n < -50000) /*underflow */
    {
      return tiny * x;
    }
  }
  if (k == 0x7ff) /* NaN or Inf */
  {
    return x + x;
  }
  k = k + n;
  if (k > 0x7fe) /* overflow  */
  {
    return huge * copysign (huge, x);
  }
  if (k > 0) /* normal result */
  {
    __HI (x) = (hx & 0x800fffff) | (k << 20);
    return x;
  }
  if (k <= -54)
  {
    if (n > 50000) /* in case integer overflow in n + k */
    {
      return huge * copysign (huge, x); /*overflow */
    }
    else
    {
      return tiny * copysign (tiny, x); /*underflow */
    }
  }
  k += 54; /* subnormal result */
  __HI (x) = (hx & 0x800fffff) | (k << 20);
  return x * twom54;
} /* scalbn */

#undef two54
#undef twom54
#undef huge
#undef tiny
